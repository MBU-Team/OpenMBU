//--------------------------------------------------------------------------
// PathedInterior.cc:
//
//
//--------------------------------------------------------------------------

#include "interior/pathedInterior.h"
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
    addField("StartSound", TypeAudioProfilePtr, Offset(sound[StartSound], PathedInteriorData));
    addField("SustainSound", TypeAudioProfilePtr, Offset(sound[SustainSound], PathedInteriorData));
    addField("StopSound", TypeAudioProfilePtr, Offset(sound[StopSound], PathedInteriorData));

    Parent::initPersistFields();
}

void PathedInteriorData::packData(BitStream* stream)
{
    for (S32 i = 0; i < MaxSounds; i++)
    {
        if (stream->writeFlag(sound[i]))
            stream->writeRangedU32(packed ? SimObjectId(sound[i]) :
                sound[i]->getId(), DataBlockObjectIdFirst, DataBlockObjectIdLast);
    }
    Parent::packData(stream);
}

void PathedInteriorData::unpackData(BitStream* stream)
{
    for (S32 i = 0; i < MaxSounds; i++) {
        sound[i] = NULL;
        if (stream->readFlag())
            sound[i] = (AudioProfile*)stream->readRangedU32(DataBlockObjectIdFirst,
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
                Sim::findObject(SimObjectId(sound[i]), sound[i]);
    }
    return true;
}

PathedInterior::PathedInterior()
{
    // TODO: These are different in MB
    mNetFlags.set(Ghostable);
    mTypeMask = InteriorObjectType;

    mCurrentPosition = 0;
    mTargetPosition = 0;
    mPathKey = 0xFFFFFFFF;
    mAdvanceTime = 0.0;
    mStopTime = 1000.0;
    mSustainHandle = 0;
    mLMHandle = 0xFFFFFFFF;

    mNextPathedInterior = NULL;

    static U32 sNextNetUpdateInit = 0;
    sNextNetUpdateInit += 5;
    mNextNetUpdate = sNextNetUpdateInit & 31;

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
    }

    if (isClientObject())
    {
        Point3F initialPos(0.0, 0.0, 0.0);
        mBaseTransform.getColumn(3, &initialPos);
        Point3F pathPos(0.0, 0.0, 0.0);
        //gClientPathManager->getPathPosition(mPathKey, 0, pathPos);
        mOffset = initialPos - pathPos;
        //gClientPathManager->getPathPosition(mPathKey, mCurrentPosition, pathPos);
        MatrixF mat = getTransform();
        mat.setColumn(3, pathPos + mOffset);
        setTransform(mat);
    }

    addToScene();

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
    if (isClientObject())
    {
        if (mSustainHandle)
        {
            alxStop(mSustainHandle);
            mSustainHandle = 0;
        }
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
bool PathedInterior::buildPolyList(AbstractPolyList* list, const Box3F& wsBox, const SphereF&)
{
    if (bool(mInteriorRes) == false)
        return false;

    // Setup collision state data
    list->setTransform(&getTransform(), getScale());
    list->setObject(this);

    return mInterior->buildPolyList(list, wsBox, mWorldToObj, getScale());
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
    if (mPathKey == 0xFFFFFFFF && !isGhost())
    {
        mPathKey = getPathKey();
        Point3F pathPos(0.0, 0.0, 0.0);
        Point3F initialPos(0.0, 0.0, 0.0);
        mBaseTransform.getColumn(3, &initialPos);
        //gServerPathManager->getPathPosition(mPathKey, 0, pathPos);
        mOffset = initialPos - pathPos;
    }
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

        stream->writeAffineTransform(mBaseTransform);
        mathWrite(*stream, mBaseScale);

        stream->write(mPathKey);
    }
    if (stream->writeFlag((mask & NewPositionMask) && mPathKey != Path::NoPathIndex))
        stream->writeInt(S32(mCurrentPosition), gServerPathManager->getPathTimeBits(mPathKey));
    if (stream->writeFlag((mask & NewTargetMask) && mPathKey != Path::NoPathIndex))
    {
        if (stream->writeFlag(mTargetPosition < 0))
        {
            stream->writeFlag(mTargetPosition == -1);
        }
        else
            stream->writeInt(S32(mTargetPosition), gServerPathManager->getPathTimeBits(mPathKey));
    }
    return retMask;
}

void PathedInterior::unpackUpdate(NetConnection* con, BitStream* stream)
{
    Parent::unpackUpdate(con, stream);

    MatrixF tempXForm;
    Point3F tempScale;

    if (stream->readFlag())
    {
        // Initial
        mInteriorResName = stream->readSTString();
        stream->read(&mInteriorResIndex);

        stream->readAffineTransform(&tempXForm);
        mathRead(*stream, &tempScale);
        mBaseTransform = tempXForm;
        mBaseScale = tempScale;

        stream->read(&mPathKey);
    }
    if (stream->readFlag())
    {
        Point3F pathPos;
        mCurrentPosition = stream->readInt(gClientPathManager->getPathTimeBits(mPathKey));
        if (isProperlyAdded())
        {
            //gClientPathManager->getPathPosition(mPathKey, mCurrentPosition, pathPos);
            MatrixF mat = getTransform();
            mat.setColumn(3, pathPos + mOffset);
            setTransform(mat);
        }
    }
    if (stream->readFlag())
    {
        if (stream->readFlag())
        {
            mTargetPosition = stream->readFlag() ? -1 : -2;
        }
        else
            mTargetPosition = stream->readInt(gClientPathManager->getPathTimeBits(mPathKey));
    }
}

void PathedInterior::processTick(const Move* move)
{
    if (isServerObject())
    {
        U32 timeMs = 32;
        if (mCurrentPosition != mTargetPosition)
        {
            S32 delta;
            if (mTargetPosition == -1)
                delta = timeMs;
            else if (mTargetPosition == -2)
                delta = -timeMs;
            else
            {
                delta = mTargetPosition - mCurrentPosition;
                if (delta < -timeMs)
                    delta = -timeMs;
                else if (delta > timeMs)
                    delta = timeMs;
            }
            mCurrentPosition += delta;
            U32 totalTime = gClientPathManager->getPathTotalTime(mPathKey);
            while (mCurrentPosition > totalTime)
                mCurrentPosition -= totalTime;
            while (mCurrentPosition < 0)
                mCurrentPosition += totalTime;
        }
    }
}

void PathedInterior::setStopped()
{
    mStopTime = getMin(mAdvanceTime, mStopTime);
}

void PathedInterior::computeNextPathStep(U32 timeDelta)
{
    S32 timeMs = timeDelta;
    //mStopped = false;

    if (mCurrentPosition == mTargetPosition)
    {
        mExtrudedBox = getWorldBox();
        mCurrentVelocity.set(0, 0, 0);
    }
    else
    {
        S32 delta;
        if (mTargetPosition < 0)
        {
            if (mTargetPosition == -1)
                delta = timeMs;
            else if (mTargetPosition == -2)
                delta = -timeMs;
            mCurrentPosition += delta;
            U32 totalTime = gClientPathManager->getPathTotalTime(mPathKey);
            while (mCurrentPosition >= totalTime)
                mCurrentPosition -= totalTime;
            while (mCurrentPosition < 0)
                mCurrentPosition += totalTime;
        }
        else
        {
            delta = mTargetPosition - mCurrentPosition;
            if (delta < -timeMs)
                delta = -timeMs;
            else if (delta > timeMs)
                delta = timeMs;
            mCurrentPosition += delta;
        }

        Point3F curPoint;
        Point3F newPoint(0.0, 0.0, 0.0);
        MatrixF mat = getTransform();
        mat.getColumn(3, &curPoint);
        //gClientPathManager->getPathPosition(mPathKey, mCurrentPosition, newPoint);
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
    Point3F pos;
    mExtrudedBox.getCenter(&pos);
    MatrixF mat = getTransform();
    mat.setColumn(3, pos);
    if (!mSustainHandle && mDataBlock->sound[PathedInteriorData::SustainSound])
    {
        //F32 volSave = mDataBlock->sound[PathedInteriorData::SustainSound]->mDescriptionObject->mDescription.mVolume;
        //mDataBlock->sound[PathedInteriorData::SustainSound]->mDescriptionObject->mDescription.mVolume = 0;
        mSustainHandle = alxPlay(mDataBlock->sound[PathedInteriorData::SustainSound], &mat);
        //mDataBlock->sound[PathedInteriorData::SustainSound]->mDescriptionObject->mDescription.mVolume = volSave;
    }
    if (mSustainHandle)
    {
        Point3F pos;
        mExtrudedBox.getCenter(&pos);
        MatrixF mat = getTransform();
        mat.setColumn(3, pos);
        alxSourceMatrixF(mSustainHandle, &mat);
    }
}

Point3F PathedInterior::getVelocity()
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
    // TODO: Implement pushTickState
}

void PathedInterior::popTickState()
{
    // TODO: Implement popTickState
}

void PathedInterior::resetTickState(bool setT)
{
    // TODO: Implement resetTickState
}

void PathedInterior::advance(F64 timeDelta)
{
    //if (mStopped)
    //    return;

    F64 timeMs = timeDelta;
    if (mCurrentVelocity.len() == 0)
    {
        //      if(mSustainHandle)
        //      {
        //         alxStop(mSustainHandle);
        //         mSustainHandle = 0;
        //      }
        return;
    }
    MatrixF mat = getTransform();
    Point3F newPoint;
    mat.getColumn(3, &newPoint);
    newPoint += mCurrentVelocity * timeDelta / 1000.0f;
    //gClientPathManager->getPathPosition(mPathKey, mCurrentPosition, newPoint);
    mat.setColumn(3, newPoint);// + mOffset);
    setTransform(mat);
    setRenderTransform(mat);
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
    if (newPosition > gServerPathManager->getPathTotalTime(mPathKey))
        newPosition = gServerPathManager->getPathTotalTime(mPathKey);
    mCurrentPosition = mTargetPosition = newPosition;
    setMaskBits(NewPositionMask | NewTargetMask);
}

void PathedInterior::setTargetPosition(S32 newPosition)
{
    resolvePathKey();
    if (newPosition < -2)
        newPosition = 0;
    if (newPosition > S32(gServerPathManager->getPathTotalTime(mPathKey)))
        newPosition = gServerPathManager->getPathTotalTime(mPathKey);
    if (mTargetPosition != newPosition)
    {
        mTargetPosition = newPosition;
        setMaskBits(NewTargetMask);
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


