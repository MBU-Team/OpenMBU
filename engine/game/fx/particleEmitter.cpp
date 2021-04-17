//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "particleEmitter.h"
#include "sceneGraph/sceneGraph.h"
#include "sceneGraph/sceneState.h"
#include "console/consoleTypes.h"
#include "core/bitStream.h"
#include "math/mRandom.h"
#include "gfx/gfxDevice.h"
#include "gfx/primBuilder.h"
#include "renderInstance/renderInstMgr.h"
#include "game/gameProcess.h"

static ParticleEmitterData gDefaultEmitterData;
Point3F ParticleEmitter::mWindVelocity(0.0, 0.0, 0.0);

IMPLEMENT_CO_DATABLOCK_V1(ParticleEmitterData);

//-----------------------------------------------------------------------------
// ParticleEmitterData
//-----------------------------------------------------------------------------
ParticleEmitterData::ParticleEmitterData()
{
    //   VECTOR_SET_ASSOCIATION(particleDataBlocks);
    //   VECTOR_SET_ASSOCIATION(dataBlockIds);

    ejectionPeriodMS = 100;    // 10 Particles Per second
    periodVarianceMS = 0;      // exactly

    ejectionVelocity = 2.0f;   // From 1.0 - 3.0 meters per sec
    velocityVariance = 1.0f;
    ejectionOffset = 0.0f;   // ejection from the emitter point

    thetaMin = 0.0f;   // All heights
    thetaMax = 90.0f;

    phiReferenceVel = 0.0f;   // All directions
    phiVariance = 360.0f;

    lifetimeMS = 0;
    lifetimeVarianceMS = 0;

    overrideAdvance = false;
    orientParticles = false;
    orientOnVelocity = true;
    useEmitterSizes = false;
    useEmitterColors = false;
    particleString = NULL;
    particleDataBlock = NULL;
    partListInitSize = NULL;
}


IMPLEMENT_CONSOLETYPE(ParticleEmitterData)
IMPLEMENT_GETDATATYPE(ParticleEmitterData)
IMPLEMENT_SETDATATYPE(ParticleEmitterData)

//-----------------------------------------------------------------------------
// initPersistFields
//-----------------------------------------------------------------------------
void ParticleEmitterData::initPersistFields()
{
    Parent::initPersistFields();

    addField("ejectionPeriodMS", TypeS32, Offset(ejectionPeriodMS, ParticleEmitterData));
    addField("periodVarianceMS", TypeS32, Offset(periodVarianceMS, ParticleEmitterData));
    addField("ejectionVelocity", TypeF32, Offset(ejectionVelocity, ParticleEmitterData));
    addField("velocityVariance", TypeF32, Offset(velocityVariance, ParticleEmitterData));
    addField("ejectionOffset", TypeF32, Offset(ejectionOffset, ParticleEmitterData));
    addField("thetaMin", TypeF32, Offset(thetaMin, ParticleEmitterData));
    addField("thetaMax", TypeF32, Offset(thetaMax, ParticleEmitterData));
    addField("phiReferenceVel", TypeF32, Offset(phiReferenceVel, ParticleEmitterData));
    addField("phiVariance", TypeF32, Offset(phiVariance, ParticleEmitterData));
    addField("overrideAdvance", TypeBool, Offset(overrideAdvance, ParticleEmitterData));
    addField("orientParticles", TypeBool, Offset(orientParticles, ParticleEmitterData));
    addField("orientOnVelocity", TypeBool, Offset(orientOnVelocity, ParticleEmitterData));
    addField("particles", TypeString, Offset(particleString, ParticleEmitterData));
    addField("lifetimeMS", TypeS32, Offset(lifetimeMS, ParticleEmitterData));
    addField("lifetimeVarianceMS", TypeS32, Offset(lifetimeVarianceMS, ParticleEmitterData));
    addField("useEmitterSizes", TypeBool, Offset(useEmitterSizes, ParticleEmitterData));
    addField("useEmitterColors", TypeBool, Offset(useEmitterColors, ParticleEmitterData));
}

//-----------------------------------------------------------------------------
// packData
//-----------------------------------------------------------------------------
void ParticleEmitterData::packData(BitStream* stream)
{
    Parent::packData(stream);

    stream->writeInt(ejectionPeriodMS, 10);
    stream->writeInt(periodVarianceMS, 10);
    stream->writeInt((S32)(ejectionVelocity * 100), 16);
    stream->writeInt((S32)(velocityVariance * 100), 14);
    if (stream->writeFlag(ejectionOffset != gDefaultEmitterData.ejectionOffset))
        stream->writeInt((S32)(ejectionOffset * 100), 16);
    stream->writeRangedU32((U32)thetaMin, 0, 180);
    stream->writeRangedU32((U32)thetaMax, 0, 180);
    if (stream->writeFlag(phiReferenceVel != gDefaultEmitterData.phiReferenceVel))
        stream->writeRangedU32((U32)phiReferenceVel, 0, 360);
    if (stream->writeFlag(phiVariance != gDefaultEmitterData.phiVariance))
        stream->writeRangedU32((U32)phiVariance, 0, 360);
    stream->writeFlag(overrideAdvance);
    stream->writeFlag(orientParticles);
    stream->writeFlag(orientOnVelocity);
    stream->write(lifetimeMS);
    stream->write(lifetimeVarianceMS);
    stream->writeFlag(useEmitterSizes);
    stream->writeFlag(useEmitterColors);
    stream->write(partDataID);
}

//-----------------------------------------------------------------------------
// unpackData
//-----------------------------------------------------------------------------
void ParticleEmitterData::unpackData(BitStream* stream)
{
    Parent::unpackData(stream);

    ejectionPeriodMS = stream->readInt(10);
    periodVarianceMS = stream->readInt(10);
    ejectionVelocity = stream->readInt(16) / 100.0f;
    velocityVariance = stream->readInt(14) / 100.0f;
    if (stream->readFlag())
        ejectionOffset = stream->readInt(16) / 100.0f;
    else
        ejectionOffset = gDefaultEmitterData.ejectionOffset;

    thetaMin = stream->readRangedU32(0, 180);
    thetaMax = stream->readRangedU32(0, 180);
    if (stream->readFlag())
        phiReferenceVel = stream->readRangedU32(0, 360);
    else
        phiReferenceVel = gDefaultEmitterData.phiReferenceVel;

    if (stream->readFlag())
        phiVariance = stream->readRangedU32(0, 360);
    else
        phiVariance = gDefaultEmitterData.phiVariance;

    overrideAdvance = stream->readFlag();
    orientParticles = stream->readFlag();
    orientOnVelocity = stream->readFlag();
    stream->read(&lifetimeMS);
    stream->read(&lifetimeVarianceMS);
    useEmitterSizes = stream->readFlag();
    useEmitterColors = stream->readFlag();

    stream->read(&partDataID);
}

//-----------------------------------------------------------------------------
// onAdd
//-----------------------------------------------------------------------------
bool ParticleEmitterData::onAdd()
{
    if (Parent::onAdd() == false)
        return false;

    //   if (overrideAdvance == true) {
    //      Con::errorf(ConsoleLogEntry::General, "ParticleEmitterData: Not going to work.  Fix it!");
    //      return false;
    //   }

       // Validate the parameters...
       //
    if (ejectionPeriodMS < 1)
    {
        Con::warnf(ConsoleLogEntry::General, "ParticleEmitterData(%s) period < 1 ms", getName());
        ejectionPeriodMS = 1;
    }
    if (periodVarianceMS >= ejectionPeriodMS)
    {
        Con::warnf(ConsoleLogEntry::General, "ParticleEmitterData(%s) periodVariance >= period", getName());
        periodVarianceMS = ejectionPeriodMS - 1;
    }
    if (ejectionVelocity < 0.0f)
    {
        Con::warnf(ConsoleLogEntry::General, "ParticleEmitterData(%s) ejectionVelocity < 0.0f", getName());
        ejectionVelocity = 0.0f;
    }
    if (velocityVariance > ejectionVelocity)
    {
        Con::warnf(ConsoleLogEntry::General, "ParticleEmitterData(%s) velocityVariance > ejectionVelocity", getName());
        velocityVariance = ejectionVelocity;
    }
    if (ejectionOffset < 0.0f)
    {
        Con::warnf(ConsoleLogEntry::General, "ParticleEmitterData(%s) ejectionOffset < 0", getName());
        ejectionOffset = 0.0f;
    }
    if (thetaMin < 0.0f)
    {
        Con::warnf(ConsoleLogEntry::General, "ParticleEmitterData(%s) thetaMin < 0.0", getName());
        thetaMin = 0.0f;
    }
    if (thetaMax > 180.0f)
    {
        Con::warnf(ConsoleLogEntry::General, "ParticleEmitterData(%s) thetaMax > 180.0", getName());
        thetaMax = 180.0f;
    }
    if (thetaMin > thetaMax)
    {
        Con::warnf(ConsoleLogEntry::General, "ParticleEmitterData(%s) thetaMin > thetaMax", getName());
        thetaMin = thetaMax;
    }
    if (phiVariance < 0.0f || phiVariance > 360.0f)
    {
        Con::warnf(ConsoleLogEntry::General, "ParticleEmitterData(%s) invalid phiVariance", getName());
        phiVariance = phiVariance < 0.0f ? 0.0f : 360.0f;
    }
    if (lifetimeMS < 0)
    {
        Con::warnf(ConsoleLogEntry::General, "ParticleEmitterData(%s) lifetimeMS < 0.0f", getName());
        lifetimeMS = 0;
    }
    if (lifetimeVarianceMS > lifetimeMS)
    {
        Con::warnf(ConsoleLogEntry::General, "ParticleEmitterData(%s) lifetimeVarianceMS >= lifetimeMS", getName());
        lifetimeVarianceMS = lifetimeMS;
    }


    // load the particle datablocks...
    //
    if (particleString != NULL)
    {
        ParticleData* pData = NULL;
        if (Sim::findObject(particleString, pData) == false)
        {
            Con::warnf(ConsoleLogEntry::General, "ParticleEmitterData(%s) unable to find particle datablock: %s", getName(), particleString);
            return false;
        }
        else
        {
            particleDataBlock = pData;
            partDataID = pData->getId();
        }
    }

    return true;
}

//-----------------------------------------------------------------------------
// preload
//-----------------------------------------------------------------------------
bool ParticleEmitterData::preload(bool server, char errorBuffer[256])
{
    if (Parent::preload(server, errorBuffer) == false)
        return false;

    ParticleData* pData = NULL;
    if (Sim::findObject(partDataID, pData) == false)
        Con::warnf(ConsoleLogEntry::General, "ParticleEmitterData(%s) unable to find particle datablock: %d", getName(), partDataID);
    else
        particleDataBlock = pData;

    if (!server)
    {
        allocPrimBuffer();
    }

    return true;
}

//-----------------------------------------------------------------------------
// alloc PrimitiveBuffer
// The datablock allocates this static index buffer because it's the same
// for all of the emitters - each particle quad uses the same index ordering
//-----------------------------------------------------------------------------
void ParticleEmitterData::allocPrimBuffer(S32 overrideSize)
{
    // calculate particle list size
    U32 partLife = particleDataBlock->lifetimeMS;
    U32 maxPartLife = partLife + particleDataBlock->lifetimeVarianceMS;

    partListInitSize = maxPartLife / (ejectionPeriodMS - periodVarianceMS);
    partListInitSize += 8; // add 8 as "fudge factor" to make sure it doesn't realloc if it goes over by 1

    // if override size is specified, then the emitter overran its buffer and needs a larger allocation
    if (overrideSize != -1)
    {
        partListInitSize = overrideSize;
    }

    // create index buffer based on that size
    U32 indexListSize = partListInitSize * 6; // 6 indices per particle
    U16* indices = new U16[indexListSize];

    for (U32 i = 0; i < partListInitSize; i++)
    {
        // this index ordering should be optimal (hopefully) for the vertex cache
        U16* idx = &indices[i * 6];
        volatile U32 offset = i * 4;  // set to volatile to fix VC6 Release mode compiler bug
        idx[0] = 0 + offset;
        idx[1] = 1 + offset;
        idx[2] = 3 + offset;
        idx[3] = 1 + offset;
        idx[4] = 3 + offset;
        idx[5] = 2 + offset;
    }


    U16* ibIndices;
    GFXBufferType bufferType = GFXBufferTypeStatic;

#ifdef TORQUE_OS_XENON
    // Because of the way the volatile buffers work on Xenon this is the only
    // way to do this.
    bufferType = GFXBufferTypeVolatile;
#endif
    primBuff.set(GFX, indexListSize, 0, bufferType);
    primBuff.lock(&ibIndices);
    dMemcpy(ibIndices, indices, indexListSize * sizeof(U16));
    primBuff.unlock();

    delete[] indices;
}


//-----------------------------------------------------------------------------
// ParticleEmitter
//-----------------------------------------------------------------------------
ParticleEmitter::ParticleEmitter()
{
    mDeleteWhenEmpty = false;
    mDeleteOnTick = false;

    mInternalClock = 0;
    mNextParticleTime = 0;

    mLastPosition.set(0, 0, 0);
    mHasLastPosition = false;

    mLifetimeMS = 0;
    mElapsedTimeMS = 0;

    mLastPartIndex = 0;
    mCurBuffSize = 0;

    mDead = false;
}

//-----------------------------------------------------------------------------
// ~ParticleEmitter
//-----------------------------------------------------------------------------
ParticleEmitter::~ParticleEmitter()
{
    //   AssertFatal(mParticleListHead == NULL, "Error, particles remain in emitter after remove?");
}

//-----------------------------------------------------------------------------
// onAdd
//-----------------------------------------------------------------------------
bool ParticleEmitter::onAdd()
{
    if (!Parent::onAdd())
        return false;

    removeFromProcessList();

    mLifetimeMS = mDataBlock->lifetimeMS;
    if (mDataBlock->lifetimeVarianceMS)
    {
        mLifetimeMS += S32(gRandGen.randI() % (2 * mDataBlock->lifetimeVarianceMS + 1)) - S32(mDataBlock->lifetimeVarianceMS);
    }

    mPartList.setSize(mDataBlock->partListInitSize);

    F32 radius = 5.0;
    mObjBox.min = Point3F(-radius, -radius, -radius);
    mObjBox.max = Point3F(radius, radius, radius);
    resetWorldBox();

    return true;
}


//-----------------------------------------------------------------------------
// onRemove
//-----------------------------------------------------------------------------
void ParticleEmitter::onRemove()
{
    removeFromScene();

    Parent::onRemove();
}


//-----------------------------------------------------------------------------
// onNewDataBlock
//-----------------------------------------------------------------------------
bool ParticleEmitter::onNewDataBlock(GameBaseData* dptr)
{
    mDataBlock = dynamic_cast<ParticleEmitterData*>(dptr);
    if (!mDataBlock || !Parent::onNewDataBlock(dptr))
        return false;

    scriptOnNewDataBlock();
    return true;
}

ColorF ParticleEmitter::getCollectiveColor()
{
    U32 count = 0;
    ColorF color = ColorF(0.0f, 0.0f, 0.0f);

    U32 numpart = getMin(mLastPartIndex, (mPartList.size() - 1));

    //if(numpart <= 0)
    //	Con::printf("NumParts: %f", numpart);

    for (U32 i = 0; i < numpart; i++)
    {
        color += mPartList[i].color;
        count++;
    }

    if (count > 0)
        color /= F32(count);

    //if(color.red == 0.0f && color.green == 0.0f && color.blue == 0.0f)
    //	color = color;

    return color;
}


//-----------------------------------------------------------------------------
// prepRenderImage
//-----------------------------------------------------------------------------
bool ParticleEmitter::prepRenderImage(SceneState* state, const U32 stateKey,
    const U32 /*startZone*/, const bool /*modifyBaseState*/)
{
    if (isLastState(state, stateKey))
        return false;
    setLastState(state, stateKey);

    // This should be sufficient for most objects that don't manage zones, and
    //  don't need to return a specialized RenderImage...
    if (state->isObjectRendered(this))
    {
        prepBatchRender(state->getCameraPosition());
    }

    return false;
}

//-----------------------------------------------------------------------------
// prepBatchRender
//-----------------------------------------------------------------------------
void ParticleEmitter::prepBatchRender(const Point3F& camPos)
{
    if (mLastPartIndex == 0) return;
    if (mDead) return;

    copyToVB(camPos);

    RenderInst* ri = gRenderInstManager.allocInst();
    ri->vertBuff = &mVertBuff;
    ri->primBuff = &mDataBlock->primBuff;
    ri->matInst = NULL;
    ri->translucent = true;
    ri->type = RenderInstManager::RIT_Translucent;
    ri->calcSortPoint(this, camPos);
    ri->particles = true;


    ri->worldXform = gRenderInstManager.allocXform();
    MatrixF world = GFX->getWorldMatrix();
    *ri->worldXform = world;
    ri->primBuffIndex = mLastPartIndex;
    ri->transFlags = mPartList[0].dataBlock->useInvAlpha;

    ri->miscTex = &*(mPartList[0].dataBlock->textureList[0]);

    gRenderInstManager.addInst(ri);

}

//-----------------------------------------------------------------------------
// setSizes
//-----------------------------------------------------------------------------
void ParticleEmitter::setSizes(F32* sizeList)
{
    for (int i = 0; i < ParticleData::PDC_NUM_KEYS; i++)
    {
        sizes[i] = sizeList[i];
    }
}

//-----------------------------------------------------------------------------
// setColors
//-----------------------------------------------------------------------------
void ParticleEmitter::setColors(ColorF* colorList)
{
    for (int i = 0; i < ParticleData::PDC_NUM_KEYS; i++)
    {
        colors[i] = colorList[i];
    }
}

//-----------------------------------------------------------------------------
// deleteWhenEmpty
//-----------------------------------------------------------------------------
void ParticleEmitter::deleteWhenEmpty()
{

    SimGroup* cleanup = dynamic_cast<SimGroup*>(Sim::findObject("MissionCleanup"));

    if (cleanup != NULL)
    {
        cleanup->addObject(this);
    }
    else
    {
        Con::errorf("No mission cleanup group?");

        // if no mission cleanup group then it's probably been removed in shutdown
        deleteObject();
    }

    mDeleteWhenEmpty = true;
}

//-----------------------------------------------------------------------------
// emitParticles
//-----------------------------------------------------------------------------
void ParticleEmitter::emitParticles(const Point3F& point,
    const bool     useLastPosition,
    const Point3F& axis,
    const Point3F& velocity,
    const U32      numMilliseconds)
{
    if (mDead) return;

    // lifetime over - no more particles
    if (mLifetimeMS > 0 && mElapsedTimeMS > mLifetimeMS)
    {
        return;
    }

    Point3F realStart;
    if (useLastPosition && mHasLastPosition)
        realStart = mLastPosition;
    else
        realStart = point;

    emitParticles(realStart, point,
        axis,
        velocity,
        numMilliseconds);
}

//-----------------------------------------------------------------------------
// emitParticles
//-----------------------------------------------------------------------------
void ParticleEmitter::emitParticles(const Point3F& start,
    const Point3F& end,
    const Point3F& axis,
    const Point3F& velocity,
    const U32      numMilliseconds)
{
    if (mDead) return;

    // lifetime over - no more particles
    if (mLifetimeMS > 0 && mElapsedTimeMS > mLifetimeMS)
    {
        return;
    }

    U32 currTime = 0;
    bool particlesAdded = false;

    Point3F axisx;
    if (mFabs(axis.z) < 0.9f)
        mCross(axis, Point3F(0, 0, 1), &axisx);
    else
        mCross(axis, Point3F(0, 1, 0), &axisx);
    axisx.normalize();

    if (mNextParticleTime != 0)
    {
        // Need to handle next particle
        //
        if (mNextParticleTime > numMilliseconds)
        {
            // Defer to next update
            //  (Note that this introduces a potential spatial irregularity if the owning
            //   object is accelerating, and updating at a low frequency)
            //
            mNextParticleTime -= numMilliseconds;
            mInternalClock += numMilliseconds;
            mLastPosition = end;
            mHasLastPosition = true;
            return;
        }
        else
        {
            currTime += mNextParticleTime;
            mInternalClock += mNextParticleTime;
            // Emit particle at curr time

            // Create particle at the correct position
            Point3F pos;
            pos.interpolate(start, end, F32(currTime) / F32(numMilliseconds));
            addParticle(pos, axis, velocity, axisx);
            particlesAdded = true;
            mNextParticleTime = 0;
        }
    }

    while (currTime < numMilliseconds)
    {
        S32 nextTime = mDataBlock->ejectionPeriodMS;
        if (mDataBlock->periodVarianceMS != 0)
        {
            nextTime += S32(gRandGen.randI() % (2 * mDataBlock->periodVarianceMS + 1)) -
                S32(mDataBlock->periodVarianceMS);
        }
        AssertFatal(nextTime > 0, "Error, next particle ejection time must always be greater than 0");

        if (currTime + nextTime > numMilliseconds)
        {
            mNextParticleTime = (currTime + nextTime) - numMilliseconds;
            mInternalClock += numMilliseconds - currTime;
            AssertFatal(mNextParticleTime > 0, "Error, should not have deferred this particle!");
            break;
        }

        currTime += nextTime;
        mInternalClock += nextTime;

        // Create particle at the correct position
        Point3F pos;
        pos.interpolate(start, end, F32(currTime) / F32(numMilliseconds));
        addParticle(pos, axis, velocity, axisx);
        particlesAdded = true;
    }

    // DMMFIX: Lame and slow...
    if (particlesAdded == true)
        updateBBox();


    if (mLastPartIndex && mSceneManager == NULL)
    {
        getCurrentClientSceneGraph()->addObjectToScene(this);
        getCurrentClientContainer()->addObject(this);
        getCurrentClientProcessList()->addObject(this);
    }

    mLastPosition = end;
    mHasLastPosition = true;
}

//-----------------------------------------------------------------------------
// emitParticles
//-----------------------------------------------------------------------------
void ParticleEmitter::emitParticles(const Point3F& rCenter,
    const Point3F& rNormal,
    const F32      radius,
    const Point3F& velocity,
    S32 count)
{
    if (mDead) return;

    // lifetime over - no more particles
    if (mLifetimeMS > 0 && mElapsedTimeMS > mLifetimeMS)
    {
        return;
    }


    Point3F axisx, axisy;
    Point3F axisz = rNormal;

    if (axisz.isZero())
    {
        axisz.set(0.0, 0.0, 1.0);
    }

    if (mFabs(axisz.z) < 0.98)
    {
        mCross(axisz, Point3F(0, 0, 1), &axisy);
        axisy.normalize();
    }
    else
    {
        mCross(axisz, Point3F(0, 1, 0), &axisy);
        axisy.normalize();
    }
    mCross(axisz, axisy, &axisx);
    axisx.normalize();

    // Should think of a better way to distribute the
    // particles within the hemisphere.
    for (S32 i = 0; i < count; i++)
    {
        Point3F pos = axisx * (radius * (1 - (2 * gRandGen.randF())));
        pos += axisy * (radius * (1 - (2 * gRandGen.randF())));
        pos += axisz * (radius * gRandGen.randF());

        Point3F axis = pos;
        axis.normalize();
        pos += rCenter;

        addParticle(pos, axis, velocity, axisz);
    }

    // Set world bounding box
    mObjBox.min = rCenter - Point3F(radius, radius, radius);
    mObjBox.max = rCenter + Point3F(radius, radius, radius);
    resetWorldBox();

    // Make sure we're part of the world
    if (mLastPartIndex && mSceneManager == NULL)
    {
        getCurrentClientSceneGraph()->addObjectToScene(this);
        getCurrentClientContainer()->addObject(this);
        getCurrentClientProcessList()->addObject(this);
    }

    mHasLastPosition = false;
}

//-----------------------------------------------------------------------------
// updateBBox - SLOW, bad news
//-----------------------------------------------------------------------------
void ParticleEmitter::updateBBox()
{
    Point3F min(1e10, 1e10, 1e10);
    Point3F max(-1e10, -1e10, -1e10);

    for (U32 i = 0; i < mLastPartIndex; i++)
    {
        Particle* part = &mPartList[i];
        min.setMin(part->pos);
        max.setMax(part->pos);
    }

    mObjBox = Box3F(min, max);
    MatrixF temp = getTransform();
    setTransform(temp);
}

//-----------------------------------------------------------------------------
// addParticle
//-----------------------------------------------------------------------------
void ParticleEmitter::addParticle(const Point3F& pos,
    const Point3F& axis,
    const Point3F& vel,
    const Point3F& axisx)
{
    mLastPartIndex++;

    // ARGGH, need to fix this - can get large numbers of particle to draw when dt is high?

    if (mLastPartIndex > mPartList.size() || mLastPartIndex > mDataBlock->partListInitSize)
    {
        mPartList.setSize(mPartList.size() + 16);

        mDataBlock->allocPrimBuffer(mLastPartIndex + 16); // allocate larger primitive buffer or will crash
    }

    Particle* pNew = &mPartList[mLastPartIndex - 1];

    Point3F ejectionAxis = axis;
    F32 theta = (mDataBlock->thetaMax - mDataBlock->thetaMin) * gRandGen.randF() +
        mDataBlock->thetaMin;

    F32 ref = (F32(mInternalClock) / 1000.0) * mDataBlock->phiReferenceVel;
    F32 phi = ref + gRandGen.randF() * mDataBlock->phiVariance;

    // Both phi and theta are in degs.  Create axis angles out of them, and create the
    //  appropriate rotation matrix...
    AngAxisF thetaRot(axisx, theta * (M_PI / 180.0));
    AngAxisF phiRot(axis, phi * (M_PI / 180.0));

    MatrixF temp(true);
    thetaRot.setMatrix(&temp);
    temp.mulP(ejectionAxis);
    phiRot.setMatrix(&temp);
    temp.mulP(ejectionAxis);

    F32 initialVel = mDataBlock->ejectionVelocity;
    initialVel += (mDataBlock->velocityVariance * 2.0f * gRandGen.randF()) - mDataBlock->velocityVariance;

    pNew->pos = pos + (ejectionAxis * mDataBlock->ejectionOffset);
    pNew->vel = ejectionAxis * initialVel;
    pNew->orientDir = ejectionAxis;
    pNew->acc.set(0, 0, 0);
    pNew->currentAge = 0;

    mDataBlock->particleDataBlock->initializeParticle(pNew, vel);
    updateKeyData(pNew);

}


//-----------------------------------------------------------------------------
// processTick
//-----------------------------------------------------------------------------
void ParticleEmitter::processTick(const Move*)
{
    if (mDeleteOnTick == true)
    {
        mDead = true;
        deleteObject();
    }
}


//-----------------------------------------------------------------------------
// advanceTime
//-----------------------------------------------------------------------------
void ParticleEmitter::advanceTime(F32 dt)
{
    if (dt < 0.00001) return;

    Parent::advanceTime(dt);

    if (dt > 0.5) dt = 0.5;

    if (mDead) return;

    mElapsedTimeMS += (S32)(dt * 1000.0f);

    U32 numMSToUpdate = (U32)(dt * 1000.0f);
    if (numMSToUpdate == 0) return;

    // remove dead particles
    for (U32 i = 0; i < mLastPartIndex; i++)
    {
        Particle* part = &mPartList[i];
        part->currentAge += numMSToUpdate;
        if (part->currentAge > part->totalLifetime)
        {
            *part = mPartList[mLastPartIndex - 1];
            mLastPartIndex--;
        }
    }


    if (mLastPartIndex <= 0 && mDeleteWhenEmpty)
    {
        mDeleteOnTick = true;
        return;
    }

    if (numMSToUpdate != 0 && mLastPartIndex > 0)
    {
        update(numMSToUpdate);
    }
}

//-----------------------------------------------------------------------------
// Update key related particle data
//-----------------------------------------------------------------------------
void ParticleEmitter::updateKeyData(Particle* part)
{
    F32 t = F32(part->currentAge) / F32(part->totalLifetime);
    AssertFatal(t <= 1.0f, "Out out bounds filter function for particle.");

    for (U32 i = 1; i < ParticleData::PDC_NUM_KEYS; i++)
    {
        if (part->dataBlock->times[i] >= t)
        {
            F32 firstPart = t - part->dataBlock->times[i - 1];
            F32 total = part->dataBlock->times[i] -
                part->dataBlock->times[i - 1];

            firstPart /= total;

            if (mDataBlock->useEmitterColors)
            {
                part->color.interpolate(colors[i - 1], colors[i], firstPart);
            }
            else
            {
                part->color.interpolate(part->dataBlock->colors[i - 1],
                    part->dataBlock->colors[i],
                    firstPart);
            }

            if (mDataBlock->useEmitterSizes)
            {
                part->size = (sizes[i - 1] * (1.0 - firstPart)) +
                    (sizes[i] * firstPart);
            }
            else
            {
                part->size = (part->dataBlock->sizes[i - 1] * (1.0 - firstPart)) +
                    (part->dataBlock->sizes[i] * firstPart);
            }
            break;

        }
    }
}

//-----------------------------------------------------------------------------
// Update particles
//-----------------------------------------------------------------------------
void ParticleEmitter::update(U32 ms)
{

    for (U32 i = 0; i < mLastPartIndex; i++)
    {
        Particle* part = &mPartList[i];

        F32 t = F32(ms) / 1000.0;

        Point3F a = part->acc;
        a -= part->vel * part->dataBlock->dragCoefficient;
        a -= mWindVelocity * part->dataBlock->windCoefficient;
        a += Point3F(0, 0, -9.81) * part->dataBlock->gravityCoefficient;

        part->vel += a * t;
        part->pos += part->vel * t;

        updateKeyData(part);
    }

}

//-----------------------------------------------------------------------------
// Copy particles to vertex buffer
//-----------------------------------------------------------------------------
void ParticleEmitter::copyToVB(const Point3F& camPos)
{
    Particle* partPtr = mPartList.address();

    static Vector<GFXVertexPCT> tempBuff(2048);
    tempBuff.reserve(mLastPartIndex * 4 + 64); // make sure tempBuff is big enough
    GFXVertexPCT* buffPtr = tempBuff.address(); // use direct pointer (faster)

    if (mDataBlock->orientParticles)
    {
        for (U32 i = 0; i < mLastPartIndex; i++, partPtr++, buffPtr += 4)
        {
            setupOriented(partPtr, camPos, buffPtr);
        }
    }
    else
    {
        // somewhat odd ordering so that texture coordinates match the oriented
        // particles
        Point3F basePoints[4];
        basePoints[0] = Point3F(-1.0, 0.0, 1.0);
        basePoints[1] = Point3F(-1.0, 0.0, -1.0);
        basePoints[2] = Point3F(1.0, 0.0, -1.0);
        basePoints[3] = Point3F(1.0, 0.0, 1.0);

        MatrixF camView = GFX->getWorldMatrix();
        camView.transpose();  // inverse - this gets the particles facing camera

        for (U32 i = 0; i < mLastPartIndex; i++, partPtr++, buffPtr += 4)
        {
            setupBillboard(partPtr, basePoints, camView, buffPtr);
        }
    }

    // create new VB if emitter size grows
    if (!mVertBuff || mLastPartIndex > mCurBuffSize)
    {
        mCurBuffSize = mLastPartIndex;
        mVertBuff.set(GFX, mLastPartIndex * 4, GFXBufferTypeDynamic);
    }
    // lock and copy tempBuff to video RAM
    GFXVertexPCT* verts = mVertBuff.lock();
    dMemcpy(verts, tempBuff.address(), mLastPartIndex * 4 * sizeof(GFXVertexPCT));
    mVertBuff.unlock();

}

//-----------------------------------------------------------------------------
// Set up particle for billboard style render
//-----------------------------------------------------------------------------
void ParticleEmitter::setupBillboard(Particle* part,
    Point3F* basePts,
    MatrixF camView,
    GFXVertexPCT* lVerts)
{
    const F32 spinFactor = (1.0 / 1000.0) * (1.0 / 360.0) * M_PI * 2.0;

    F32 width = part->size * 0.5;
    F32 spinAngle = part->spinSpeed * part->currentAge * spinFactor;

    F32 sy, cy;
    mSinCos(spinAngle, sy, cy);

    // fill four verts, use macro and unroll loop
#define fillVert(){ \
      lVerts->point.x = cy * basePts->x - sy * basePts->z;  \
      lVerts->point.y = 0.0;                                \
      lVerts->point.z = sy * basePts->x + cy * basePts->z;  \
      camView.mulV( lVerts->point );                        \
      lVerts->point *= width;                               \
      lVerts->point += part->pos;                           \
      lVerts->color = part->color; }                        \


    fillVert();
    lVerts->texCoord.set(0.0, 0.0);
    ++lVerts;
    ++basePts;

    fillVert();
    lVerts->texCoord.set(0.0, 1.0);
    ++lVerts;
    ++basePts;

    fillVert();
    lVerts->texCoord.set(1.0, 1.0);
    ++lVerts;
    ++basePts;

    fillVert();
    lVerts->texCoord.set(1.0, 0.0);
    ++lVerts;
    ++basePts;
}

//-----------------------------------------------------------------------------
// Set up oriented particle
//-----------------------------------------------------------------------------
void ParticleEmitter::setupOriented(Particle* part,
    const Point3F& camPos,
    GFXVertexPCT* lVerts)
{
    Point3F dir;

    if (mDataBlock->orientOnVelocity)
    {
        // don't render oriented particle if it has no velocity
        if (part->vel.magnitudeSafe() == 0.0) return;
        dir = part->vel;
    }
    else
    {
        dir = part->orientDir;
    }

    Point3F dirFromCam = part->pos - camPos;
    Point3F crossDir;
    mCross(dirFromCam, dir, &crossDir);
    crossDir.normalize();
    dir.normalize();


    F32 width = part->size * 0.5;
    dir *= width;
    crossDir *= width;
    Point3F start = part->pos - dir;
    Point3F end = part->pos + dir;


    lVerts->point = start + crossDir;
    lVerts->color = part->color;
    lVerts->texCoord.set(0.0, 0.0);
    ++lVerts;

    lVerts->point = start - crossDir;
    lVerts->color = part->color;
    lVerts->texCoord.set(0.0, 1.0);
    ++lVerts;

    lVerts->point = end - crossDir;
    lVerts->color = part->color;
    lVerts->texCoord.set(1.0, 1.0);
    ++lVerts;

    lVerts->point = end + crossDir;
    lVerts->color = part->color;
    lVerts->texCoord.set(1.0, 0.0);
    ++lVerts;


}
