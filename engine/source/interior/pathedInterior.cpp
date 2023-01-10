//--------------------------------------------------------------------------
// PathedInterior.cc:
//
//
//--------------------------------------------------------------------------

#include "interior/pathedInterior.h"

#include "interiorInstance.h"
#include "core/stream.h"
#include "console/consoleTypes.h"
#include "sceneGraph/sceneState.h"
#include "math/mathIO.h"
#include "core/bitStream.h"
#include "interior/interior.h"
#include "sim/simPath.h"
#include "sim/pathManager.h"
#include "core/frameAllocator.h"
#include "sceneGraph/sceneGraph.h"
#include "sfx/sfxSystem.h"
#include "game/gameConnection.h"

#ifdef MARBLE_BLAST
#include "game/marble/marble.h"
#endif

IMPLEMENT_CO_NETOBJECT_V1(PathedInterior);
IMPLEMENT_CO_DATABLOCK_V1(PathedInteriorData);

//--------------------------------------------------------------------------

PathedInteriorData::PathedInteriorData()
{
    for (U32 i = 0; i < MaxSounds; i++)
        sound[i] = NULL;
}

void PathedInteriorData::initPersistFields()
{
    addField("StartSound", TypeSFXProfilePtr, Offset(sound[StartSound], PathedInteriorData));
    addField("SustainSound", TypeSFXProfilePtr, Offset(sound[SustainSound], PathedInteriorData));
    addField("StopSound", TypeSFXProfilePtr, Offset(sound[StopSound], PathedInteriorData));

    Parent::initPersistFields();
}

void PathedInteriorData::packData(BitStream* stream)
{
    for (S32 i = 0; i < MaxSounds; i++)
    {
        if (stream->writeFlag(sound[i]))
            stream->writeRangedU32(packed ? static_cast<SimObjectId>(reinterpret_cast<size_t>(sound[i])) :
                sound[i]->getId(), DataBlockObjectIdFirst, DataBlockObjectIdLast);
    }
    Parent::packData(stream);
}

void PathedInteriorData::unpackData(BitStream* stream)
{
    for (S32 i = 0; i < MaxSounds; i++) {
        sound[i] = NULL;
        if (stream->readFlag())
            sound[i] = (SFXProfile*)stream->readRangedU32(DataBlockObjectIdFirst,
                DataBlockObjectIdLast);
    }
    Parent::unpackData(stream);
}

bool PathedInteriorData::preload(bool server, char errorBuffer[256])
{
    if (!Parent::preload(server, errorBuffer))
        return false;

    // Resolve objects transmitted from server
    if (!server)
    {
        for (S32 i = 0; i < MaxSounds; i++)
            if (sound[i])
                Sim::findObject(static_cast<SimObjectId>(reinterpret_cast<size_t>(sound[i])), sound[i]);
    }
    return true;
}

PathedInterior::PathedInterior()
{
#ifdef MB_ULTRA
    mNetFlags.set(TickLast | HiFiPassive | Ghostable);
    mTypeMask = InteriorObjectType | GameBaseHiFiObjectType;
#else
    mNetFlags.set(Ghostable);
    mTypeMask = InteriorObjectType;
#endif

    mCurrentPosition = 0;
    mTargetPosition = 0;
    mPathKey = Path::NoPathIndex;
    mAdvanceTime = 0.0;
    mStopTime = 1000.0;
    mSustainHandle = 0;
    mLMHandle = 0xFFFFFFFF;

    mNextPathedInterior = NULL;

#ifndef MBU_TEMP_MP_DESYNC_FIX
    // TEMP: Temporary fix for Moving Platform jittering.
    // The following code was used on both MBU(X360) and MBO(PC)
    // While MBO wasn't affected for some reason that we have yet to figure out,
    // Removing this fixes the de-sync with moving platforms.
    static U32 sNextNetUpdateInit;
    U32 netUpdateInit = sNextNetUpdateInit;
    sNextNetUpdateInit += UpdateTicksInc;
    mNextNetUpdate = netUpdateInit % UpdateTicks;
#else
    mNextNetUpdate = 0;
#endif

    mBaseTransform = MatrixF(true);
}

PathedInterior::~PathedInterior()
{
    //
}

PathedInterior* PathedInterior::mClientPathedInteriors = NULL;
PathedInterior* PathedInterior::mServerPathedInteriors = NULL;
//--------------------------------------------------------------------------
void PathedInterior::initPersistFields()
{
    Parent::initPersistFields();

    addField("interiorResource", TypeFilename, Offset(mInteriorResName, PathedInterior));
    addField("interiorIndex", TypeS32, Offset(mInteriorResIndex, PathedInterior));
    addField("basePosition", TypeMatrixPosition, Offset(mBaseTransform, PathedInterior));
    addField("baseRotation", TypeMatrixRotation, Offset(mBaseTransform, PathedInterior));
    addField("baseScale", TypePoint3F, Offset(mBaseScale, PathedInterior));
}


//--------------------------------------------------------------------------
bool PathedInterior::onAdd()
{
    if (!Parent::onAdd())
        return false;

    // Load the interior resource and extract the interior that is us.
    char buffer[256];
    mInteriorRes = ResourceManager->load(mInteriorResName);
    if (bool(mInteriorRes) == false)
        return false;
    if (mInteriorResIndex >= mInteriorRes->getNumSubObjects())
    {
        Con::errorf(ConsoleLogEntry::General, "PathedInterior::onAdd: invalid interior index");
        return false;
    }
    mInterior = mInteriorRes->getSubObject(mInteriorResIndex);
    if (mInterior == NULL)
        return false;

    // Setup bounding information
    mObjBox = mInterior->getBoundingBox();
    resetWorldBox();

    setScale(mBaseScale);
    setTransform(mBaseTransform);

    if (isClientObject() || gSPMode) {
        mNextPathedInterior = mClientPathedInteriors;
        mClientPathedInteriors = this;
        mInterior->prepForRendering(mInteriorRes.getFilePath());
        //      gInteriorLMManager.addInstance(mInterior->getLMHandle(), mLMHandle, NULL, this);
    } else
    {
        mNextPathedInterior = mServerPathedInteriors;
        mServerPathedInteriors = this;
    }

    addToScene();

    mDummyInst = new InteriorInstance;
    mDummyInst->mZoneRefHead = mZoneRefHead;
    mDummyInst->mLMHandle = mLMHandle;
    mDummyInst->mReflectPlanes = mInterior->mReflectPlanes;
    if (!mDummyInst->mReflectPlanes.empty())
    {
        // if any reflective planes, add interior to the reflective set of objects
        SimSet* reflectSet = dynamic_cast<SimSet*>(Sim::findObject("reflectiveSet"));
        reflectSet->addObject((SimObject*)mDummyInst);
    }

    // Install material list
    mDummyInst->mMaterialMaps.push_back(new MaterialList(mInterior->mMaterialList));

    return true;
}

bool PathedInterior::onSceneAdd(SceneGraph* g)
{
    if (!Parent::onSceneAdd(g))
        return false;
    return true;
}

void PathedInterior::onSceneRemove()
{
    Parent::onSceneRemove();
}

bool PathedInterior::onNewDataBlock(GameBaseData* dptr)
{
    mDataBlock = dynamic_cast<PathedInteriorData*>(dptr);
    return Parent::onNewDataBlock(dptr);
}

void PathedInterior::onRemove()
{

    PathedInterior** walk = &mClientPathedInteriors;
    while (*walk)
    {
        if (*walk == this)
        {
            *walk = mNextPathedInterior;
            break;
        }
        walk = &((*walk)->mNextPathedInterior);
    }

    walk = &mServerPathedInteriors;
    while (*walk)
    {
        if (*walk == this)
        {
            *walk = mNextPathedInterior;
            break;
        }
        walk = &((*walk)->mNextPathedInterior);
    }

    if (isClientObject())
    {
        SFX_DELETE(mSustainHandle);

        if(bool(mInteriorRes) && mLMHandle != 0xFFFFFFFF)
        {
            if (mInterior->getLMHandle() != 0xFFFFFFFF)
            gInteriorLMManager.removeInstance(mInterior->getLMHandle(), mLMHandle);
        }
    }
    removeFromScene();
    Parent::onRemove();
}

//------------------------------------------------------------------------------
bool PathedInterior::buildPolyListHelper(AbstractPolyList* list, const Box3F& wsBox, const SphereF& __formal, bool render)
{
    if (bool(mInteriorRes) == false)
        return false;
        
    // Setup collision state data
    list->setTransform(render ? &mRenderObjToWorld : &getTransform(), getScale());
    list->setObject(this);

    return mInterior->buildPolyList(list, wsBox, mWorldToObj, getScale());
}

bool PathedInterior::buildPolyList(AbstractPolyList* polyList, const Box3F& box, const SphereF& sphere)
{
    return buildPolyListHelper(polyList, box, sphere, false);
}

bool PathedInterior::buildRenderPolyList(AbstractPolyList* polyList, const Box3F& box, const SphereF& sphere)
{
    return buildPolyListHelper(polyList, box, sphere, true);
}


//--------------------------------------------------------------------------
bool PathedInterior::prepRenderImage(SceneState* state, const U32 stateKey,
    const U32 /*startZone*/, const bool /*modifyBaseState*/)
{
    if (isLastState(state, stateKey))
        return false;
    setLastState(state, stateKey);

    if (mPathKey == Path::NoPathIndex)
        return false;

    // This should be sufficient for most objects that don't manage zones, and
    //  don't need to return a specialized RenderImage...
    bool render = false;
    for (U32 i = 0; i < getNumCurrZones(); i++)
        if (state->getZoneState(getCurrZone(i)).render == true)
            render = true;

    if (render == true) {
        /*
              SceneRenderImage* image = new SceneRenderImage;
              image->obj = this;
              state->insertRenderImage(image);
        */

        if (!isHidden())
        {
            mDummyInst->setTransform(mRenderObjToWorld);
            mDummyInst->setScale(mObjScale);
            GFX->pushWorldMatrix();
            mInterior->prepBatchRender(mDummyInst, state);
            GFX->popWorldMatrix();
        }
    }

    return false;
}

extern ColorF gInteriorFogColor;

void PathedInterior::renderObject(SceneState* state)
{
    /*
       U32 storedWaterMark = FrameAllocator::getWaterMark();
       Point3F worldOrigin;
       getRenderTransform().getColumn(3, &worldOrigin);

       // Set up the model view and the global render state...
       glMatrixMode(GL_PROJECTION);
       glPushMatrix();
       glMatrixMode(GL_MODELVIEW);
       glPushMatrix();

       ColorF oneColor(1,1,1,1);
       glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, oneColor);

       installLights();

    //   dglMultMatrix(&newMat);
       dglMultMatrix(&mRenderObjToWorld);
       glScalef(mObjScale.x, mObjScale.y, mObjScale.z);
       glEnable(GL_CULL_FACE);

       // We need to decide if we're going to use the low-res textures

       //mInterior->setupFog(state);
       //gInteriorFogColor = state->getFogColor();
       mInterior->renderAsShape();

       //mInterior->clearFog();
       uninstallLights();
       glDisable(GL_CULL_FACE);
       glMatrixMode(GL_PROJECTION);
       glPopMatrix();

       glMatrixMode(GL_MODELVIEW);
       glPopMatrix();

       AssertFatal(dglIsInCanonicalState(), "Error, GL not in canonical state on exit");

       FrameAllocator::setWaterMark(storedWaterMark);
       dglSetRenderPrimType(0);

       // Reset the small textures...
       TextureManager::setSmallTexturesActive(false);
    */
}

void PathedInterior::resolvePathKey()
{
    if (mPathKey == Path::NoPathIndex)
    {
        mPathKey = getPathKey();
        Point3F pathPos(0.0, 0.0, 0.0);
        Point3F initialPos = mBaseTransform.getPosition();
        getPathManager()->getPathPosition(mPathKey, 0, pathPos);
        mOffset = initialPos - pathPos;
    }
}

PathManager* PathedInterior::getPathManager() const
{
    if (isGhost())
        return gClientPathManager;
    else
        return gServerPathManager;
}


//--------------------------------------------------------------------------
U32 PathedInterior::packUpdate(NetConnection* con, U32 mask, BitStream* stream)
{
    U32 retMask = Parent::packUpdate(con, mask, stream);
    resolvePathKey();

    if (stream->writeFlag(mask & InitialUpdateMask))
    {
        // Inital update...
        stream->writeString(mInteriorResName);
        stream->write(mInteriorResIndex);

        MatrixF mat = mBaseTransform;
        mat.setPosition(mOffset);
        mathWrite(*stream, mat);
        mathWrite(*stream, mBaseScale);

        stream->write(mPathKey);
    }
    if (stream->writeFlag((mask & NewPositionMask) && mPathKey != Path::NoPathIndex))
    {
        stream->write(mCurrentPosition);
        mathWrite(*stream, getTransform().getPosition());
        mathWrite(*stream, mCurrentVelocity);
    }
    if (stream->writeFlag((mask & NewTargetMask) && mPathKey != Path::NoPathIndex))
    {
        if (stream->writeFlag(mTargetPosition < 0))
        {
            stream->writeFlag(mTargetPosition == -1);
        }
        else
            stream->writeInt(mTargetPosition, getPathManager()->getPathTimeBits(mPathKey));
    }
    return retMask;
}

void PathedInterior::unpackUpdate(NetConnection* con, BitStream* stream)
{
    Parent::unpackUpdate(con, stream);

    //MatrixF tempXForm;
    //Point3F tempScale;

    if (stream->readFlag())
    {
        // Initial
        mInteriorResName = stream->readSTString();
        stream->read(&mInteriorResIndex);

        mathRead(*stream, &mBaseTransform);
        mOffset = mBaseTransform.getPosition();

        mathRead(*stream, &mBaseScale);

        stream->read(&mPathKey);
    }
    if (stream->readFlag())
    {
        Point3F pathPos;

        stream->read(&mCurrentPosition);
        mathRead(*stream, &pathPos);
        mathRead(*stream, &mCurrentVelocity);
        mBaseTransform.setPosition(pathPos);
        setTransform(mBaseTransform);
    }
    if (stream->readFlag())
    {
        if (stream->readFlag())
        {
            mTargetPosition = stream->readFlag() ? -1 : -2;
        }
        else
            mTargetPosition = stream->readInt(getPathManager()->getPathTimeBits(mPathKey));
    }
}

void PathedInterior::writePacketData(GameConnection* conn, BitStream* stream)
{
    Parent::writePacketData(conn, stream);
    MatrixF mat = getTransform();
    Point3F curPos = mat.getPosition();

    stream->write(mCurrentPosition);
    stream->write(mTargetPosition);
    mathWrite(*stream, curPos);
    mathWrite(*stream, mCurrentVelocity);
    stream->write(mStopTime);
}

void PathedInterior::readPacketData(GameConnection* conn, BitStream* stream)
{
    Parent::readPacketData(conn, stream);
    stream->read(&mCurrentPosition);
    stream->read(&mTargetPosition);

    Point3F pathPos;
    mathRead(*stream, &pathPos);

    mathRead(*stream, &mCurrentVelocity);

    MatrixF mat = getTransform();
    mat.setPosition(pathPos);

    setTransform(mat);

    stream->read(&mStopTime);
}

void PathedInterior::processTick(const Move* move)
{
    //NetConnection* conn = NetConnection::getLocalClientConnection();

    computeNextPathStep(TickMs);
    advance(TickMs);
    U32 oldNetUpdate = mNextNetUpdate;
    mNextNetUpdate--;
    if (!oldNetUpdate)
    {
        setMaskBits(NewPositionMask);
#ifdef EXPERIMENTAL_MP_LAG_FIX
        NetConnection* conn = NetConnection::getLocalClientConnection();
        if (conn && conn->isLocalConnection())
            mNextNetUpdate = 8192;
        else
#endif
            mNextNetUpdate = UpdateTicks;
    }
    doSustainSound();
    mCurrentVelocity *= getMin((F32)mStopTime, (F32)TickMs) / (F32)TickMask;
    mStopTime = 1000.0;


    /*if (conn && conn->isLocalConnection() && isServerObject())
    {
        NetConnection* toServer = NetConnection::getConnectionToServer();
        NetConnection* toClient = NetConnection::getLocalClientConnection();

        S32 index = toClient->getGhostIndex(this);

        if (index == -1)
            return;
        PathedInterior* clientPathedInterior = dynamic_cast<PathedInterior*>(toServer->resolveGhost(index));
        if (!clientPathedInterior)
            return;

        if (clientPathedInterior)
        {
            PathedInterior* clientObj = clientPathedInterior;

            clientObj->mInteriorResName = mInteriorResName;
            clientObj->mInteriorResIndex = mInteriorResIndex;

            clientObj->mBaseTransform = mBaseTransform;
            clientObj->mOffset = mOffset;
            clientObj->mBaseScale = mBaseScale;
            clientObj->mPathKey = mPathKey;

            clientObj->mCurrentPosition = mCurrentPosition;

            clientObj->mBaseTransform.setPosition(getTransform().getPosition());
            clientObj->setTransform(clientObj->mBaseTransform);

            clientObj->mTargetPosition = mTargetPosition;
            clientObj->mCurrentVelocity = mCurrentVelocity;
            clientObj->mStopTime = mStopTime;
        }
    }*/
}

void PathedInterior::interpolateTick(F32 delta)
{
    MatrixF mat = getTransform();
    Point3F newPoint = mCurrentVelocity * TickSec;
    newPoint = mat.getPosition() - (newPoint * delta);
    mat.setPosition(newPoint);
    setRenderTransform(mat);
}

void PathedInterior::doSustainSound()
{
    if (mCurrentVelocity.len() == 0.0f || mStopTime < TickMs)
    {
        SFX_DELETE(mSustainHandle);
    } else if (mDataBlock->sound[PathedInteriorData::SustainSound])
    {
        if (isGhost())
        {
            Point3F pos;
            mExtrudedBox.getCenter(&pos);

            MatrixF transform = getTransform();
            transform.setPosition(pos);

            if (!mSustainHandle)
                mSustainHandle = SFX->createSource(mDataBlock->sound[PathedInteriorData::SustainSound], &transform, &mCurrentVelocity);

            if (mSustainHandle)
            {
                mSustainHandle->play();
                mSustainHandle->setTransform(transform);
                mSustainHandle->setVelocity(mCurrentVelocity);
            }
        }
    }
}

void PathedInterior::setStopped()
{
    mStopTime = getMin(mAdvanceTime, mStopTime);
}

void PathedInterior::computeNextPathStep(F64 timeDelta)
{
    mAdvanceTime = 0.0;

    if (mPathKey == Path::NoPathIndex)
        resolvePathKey();

    if (mCurrentPosition == mTargetPosition)
    {
        mExtrudedBox = getWorldBox();
        mCurrentVelocity.set(0, 0, 0);
    }
    else
    {
        F64 delta;
        if (mTargetPosition < 0)
        {
            if (mTargetPosition == -1)
                delta = timeDelta;
            else if (mTargetPosition == -2)
                delta = -timeDelta;
            mCurrentPosition += delta;
            U32 totalTime = getPathManager()->getPathTotalTime(mPathKey);
            while (mCurrentPosition >= totalTime)
                mCurrentPosition -= totalTime;
            while (mCurrentPosition < 0)
                mCurrentPosition += totalTime;
        }
        else
        {
            delta = mTargetPosition - mCurrentPosition;
            if (delta < -timeDelta)
                delta = -timeDelta;
            else if (delta > timeDelta)
                delta = timeDelta;
            mCurrentPosition += delta;
        }

        Point3F curPoint;
        Point3F newPoint(0.0, 0.0, 0.0);
        MatrixF mat = getTransform();
        mat.getColumn(3, &curPoint);
        getPathManager()->getPathPosition(mPathKey, mCurrentPosition, newPoint);
        newPoint += mOffset;

        Point3F displaceDelta = newPoint - curPoint;
        mExtrudedBox = getWorldBox();

        if (displaceDelta.x < 0)
            mExtrudedBox.min.x += displaceDelta.x;
        else
            mExtrudedBox.max.x += displaceDelta.x;
        if (displaceDelta.y < 0)
            mExtrudedBox.min.y += displaceDelta.y;
        else
            mExtrudedBox.max.y += displaceDelta.y;
        if (displaceDelta.z < 0)
            mExtrudedBox.min.z += displaceDelta.z;
        else
            mExtrudedBox.max.z += displaceDelta.z;

        mCurrentVelocity = displaceDelta * 1000 / F32(timeDelta);
    }
}

Point3F PathedInterior::getVelocity() const
{
    return mCurrentVelocity;
}

PathedInterior* PathedInterior::getPathedInteriors(NetObject* obj)
{
    if (obj->isGhost())
        return mClientPathedInteriors;

    return mServerPathedInteriors;
}

void PathedInterior::pushTickState()
{
    MatrixF mat = getTransform();
    Point3F curPos = mat.getPosition();
    mSavedState.pathPosition = mCurrentPosition;

#ifdef MARBLE_BLAST
#ifdef MB_PHYSICS_SWITCHABLE
    if (!Marble::smTrapLaunch)
    {
        // This line was added in MBO to fix trap launch
        mSavedState.stopTime = mStopTime;
    }
#elif defined(MBO_PHYSICS)
    // This line was added in MBO to fix trap launch
    mSavedState.stopTime = mStopTime;
#endif
#else
    mSavedState.stopTime = mStopTime;
#endif
    mSavedState.extrudedBox = mExtrudedBox;
    mSavedState.velocity = mCurrentVelocity;
    mSavedState.worldPosition = curPos;
    mSavedState.targetPos = mTargetPosition;
}

void PathedInterior::popTickState()
{
    mCurrentPosition = mSavedState.pathPosition;
    mExtrudedBox = mSavedState.extrudedBox;
    mCurrentVelocity = mSavedState.velocity;
    resetTickState(true);
#ifdef MARBLE_BLAST
#ifdef MB_PHYSICS_SWITCHABLE
    if (!Marble::smTrapLaunch)
    {
        // This line was added in MBO to fix trap launch
        mStopTime = mSavedState.stopTime;
    }
#elif defined(MBO_PHYSICS)
    // This line was added in MBO to fix trap launch
    mStopTime = mSavedState.stopTime;
#endif
#else
    mStopTime = mSavedState.stopTime;
#endif
    mTargetPosition = mSavedState.targetPos;
}

void PathedInterior::resetTickState(bool setT)
{
    mAdvanceTime = 0.0;
    Point3F curPos = mSavedState.worldPosition;
    if (setT)
    {
        MatrixF mat = getTransform();
        mat.setPosition(curPos);
        setTransform(mat);
    } else
    {
        mObjToWorld.setPosition(curPos);
    }
}

bool PathedInterior::castRay(const Point3F& start, const Point3F& end, RayInfo* info)
{
    info->object = this;
    if (!mInterior->castRay(start, end, info))
        return false;

    PlaneF fakePlane(info->normal.x, info->normal.y, info->normal.z, 0.0f);

    PlaneF result;
    mTransformPlane(getTransform(), getScale(), fakePlane, &result);
    info->normal = result;

    return true;
}


void PathedInterior::advance(F64 timeDelta)
{
    F64 newDelta = timeDelta + mAdvanceTime;
#ifdef MARBLE_BLAST
#ifdef MB_PHYSICS_SWITCHABLE
    if (Marble::smTrapLaunch)
        mAdvanceTime = newDelta; // This line was removed in MBO to fix trap launch
#elif !defined(MBO_PHYSICS)
    mAdvanceTime = newDelta; // This line was removed in MBO to fix trap launch
#endif
#endif
    if (newDelta < mStopTime ||
       (timeDelta = timeDelta - (newDelta - mStopTime), timeDelta > 0.0))
    {
        MatrixF mat = getTransform();
        Point3F newPoint = (F32)timeDelta * mCurrentVelocity;
        newPoint = (newPoint / 1000.0) + mat.getPosition();
        mat.setPosition(newPoint);
        setTransform(mat);
    }
}

U32 PathedInterior::getPathKey()
{
    AssertFatal(isServerObject(), "Error, must be a server object to call this...");

    SimGroup* myGroup = getGroup();
    AssertFatal(myGroup != NULL, "No group for this object?");

    for (SimGroup::iterator itr = myGroup->begin(); itr != myGroup->end(); itr++) {
        Path* pPath = dynamic_cast<Path*>(*itr);
        if (pPath != NULL) {
            U32 pathKey = pPath->getPathIndex();
            AssertFatal(pathKey != Path::NoPathIndex, "Error, path must have event over at this point...");
            return pathKey;
        }
    }

    return Path::NoPathIndex;
}

void PathedInterior::setPathPosition(S32 newPosition)
{
    resolvePathKey();
    if (newPosition < 0)
        newPosition = 0;
    if (newPosition > getPathManager()->getPathTotalTime(mPathKey))
        newPosition = getPathManager()->getPathTotalTime(mPathKey);
    mCurrentPosition = mTargetPosition = newPosition;

    MatrixF mat = getTransform();

    Point3F newPoint;
    getPathManager()->getPathPosition(mPathKey, mCurrentPosition, newPoint);

    newPoint += mOffset;
    mat.setPosition(newPoint);
    setTransform(mat);

    setMaskBits(NewPositionMask | NewTargetMask);
}

void PathedInterior::setTargetPosition(S32 newPosition)
{
    resolvePathKey();
    if (newPosition < -2)
        newPosition = 0;
    if (newPosition > S32(getPathManager()->getPathTotalTime(mPathKey)))
        newPosition = getPathManager()->getPathTotalTime(mPathKey);
    if (mTargetPosition != newPosition)
    {
        mTargetPosition = newPosition;
        setMaskBits(NewPositionMask | NewTargetMask);
    }
}

ConsoleMethod(PathedInterior, setPathPosition, void, 3, 3, "")
{
    ((PathedInterior*)object)->setPathPosition(dAtoi(argv[2]));
}

ConsoleMethod(PathedInterior, setTargetPosition, void, 3, 3, "")
{
    ((PathedInterior*)object)->setTargetPosition(dAtoi(argv[2]));
}


//--------------------------------------------------------------------------
bool PathedInterior::readPI(Stream& stream)
{
    mName = stream.readSTString();
    mInteriorResName = stream.readSTString();
    stream.read(&mInteriorResIndex);
    stream.read(&mPathIndex);
    mathRead(stream, &mOffset);

    U32 numTriggers;
    stream.read(&numTriggers);
    mTriggers.setSize(numTriggers);
    for (U32 i = 0; i < mTriggers.size(); i++)
        mTriggers[i] = stream.readSTString();

    return (stream.getStatus() == Stream::Ok);
}

bool PathedInterior::writePI(Stream& stream) const
{
    stream.writeString(mName);
    stream.writeString(mInteriorResName);
    stream.write(mInteriorResIndex);
    stream.write(mPathIndex);
    mathWrite(stream, mOffset);

    stream.write(mTriggers.size());
    for (U32 i = 0; i < mTriggers.size(); i++)
        stream.writeString(mTriggers[i]);

    return (stream.getStatus() == Stream::Ok);
}

PathedInterior* PathedInterior::clone() const
{
    PathedInterior* pClone = new PathedInterior;

    pClone->mName = mName;
    pClone->mInteriorResName = mInteriorResName;
    pClone->mInteriorResIndex = mInteriorResIndex;
    pClone->mPathIndex = mPathIndex;
    pClone->mOffset = mOffset;

    return pClone;
}


