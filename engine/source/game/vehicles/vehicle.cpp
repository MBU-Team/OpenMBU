//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "game/vehicles/vehicle.h"

#include "platform/platform.h"
#include "game/game.h"
#include "math/mMath.h"
#include "console/simBase.h"
#include "console/console.h"
#include "console/consoleTypes.h"
#include "collision/clippedPolyList.h"
#include "collision/planeExtractor.h"
#include "game/moveManager.h"
#include "core/bitStream.h"
#include "core/dnet.h"
#include "game/gameConnection.h"
#include "ts/tsShapeInstance.h"
#include "game/fx/particleEmitter.h"
#include "sfx/sfxSystem.h"
#include "math/mathIO.h"
#include "sceneGraph/sceneState.h"
#include "terrain/terrData.h"
#include "materials/materialPropertyMap.h"
#include "game/trigger.h"
#include "game/item.h"
#include "gfx/primBuilder.h"
#include "sim/processList.h"

//----------------------------------------------------------------------------

namespace {

    const U32 sMoveRetryCount = 3;

    // Client prediction
    const S32 sMaxWarpTicks = 3;           // Max warp duration in ticks
    const S32 sMaxPredictionTicks = 30;    // Number of ticks to predict
    const F32 sVehicleGravity = -20;

    // Physics and collision constants
    static F32 sRestTol = 0.5;             // % of gravity energy to be at rest
    static int sRestCount = 10;            // Consecutive ticks before comming to rest

} // namespace {}

// Trigger objects that are not normally collided with.
static U32 sTriggerMask = ItemObjectType |
TriggerObjectType |
CorpseObjectType;

IMPLEMENT_CONOBJECT(VehicleData);

//----------------------------------------------------------------------------

VehicleData::VehicleData()
{
    shadowEnable = true;
    shadowCanMove = true;
    shadowCanAnimate = true;
    shadowSelfShadow = false;
    shadowSize = 128;
    shadowProjectionDistance = 14.0f;


    body.friction = 0;
    body.restitution = 1;

    minImpactSpeed = 25;
    softImpactSpeed = 25;
    hardImpactSpeed = 50;
    minRollSpeed = 0;
    maxSteeringAngle = 0.785; // 45 deg.

    cameraRoll = true;
    cameraLag = 0;
    cameraDecay = 0;
    cameraOffset = 0;

    minDrag = 0;
    maxDrag = 0;
    integration = 1;
    collisionTol = 0.1;
    contactTol = 0.1;
    massCenter.set(0, 0, 0);
    massBox.set(0, 0, 0);

    drag = 0.7;
    density = 4;

    jetForce = 500;
    jetEnergyDrain = 0.8;
    minJetEnergy = 1;

    for (S32 i = 0; i < Body::MaxSounds; i++)
        body.sound[i] = 0;

    dustEmitter = NULL;
    dustID = 0;
    triggerDustHeight = 3.0;
    dustHeight = 1.0;

    dMemset(damageEmitterList, 0, sizeof(damageEmitterList));
    dMemset(damageEmitterIDList, 0, sizeof(damageEmitterIDList));
    dMemset(damageLevelTolerance, 0, sizeof(damageLevelTolerance));
    dMemset(splashEmitterList, 0, sizeof(splashEmitterList));
    dMemset(splashEmitterIDList, 0, sizeof(splashEmitterIDList));

    numDmgEmitterAreas = 0;

    splashFreqMod = 300.0;
    splashVelEpsilon = 0.50;
    exitSplashSoundVel = 2.0;
    softSplashSoundVel = 1.0;
    medSplashSoundVel = 2.0;
    hardSplashSoundVel = 3.0;

    genericShadowLevel = Vehicle_GenericShadowLevel;
    noShadowLevel = Vehicle_NoShadowLevel;

    dMemset(waterSound, 0, sizeof(waterSound));

    collDamageThresholdVel = 20;
    collDamageMultiplier = 0.05;
}


//----------------------------------------------------------------------------

bool VehicleData::preload(bool server, char errorBuffer[256])
{
    if (!Parent::preload(server, errorBuffer))
        return false;

    // Resolve objects transmitted from server
    if (!server) {
        for (S32 i = 0; i < Body::MaxSounds; i++)
            if (body.sound[i])
                Sim::findObject(static_cast<SimObjectId>(reinterpret_cast<size_t>(body.sound[i])), body.sound[i]);
    }

    if (!dustEmitter && dustID != 0)
    {
        if (!Sim::findObject(dustID, dustEmitter))
        {
            Con::errorf(ConsoleLogEntry::General, "VehicleData::preload Invalid packet, bad datablockId(dustEmitter): 0x%x", dustID);
        }
    }

    U32 i;
    for (i = 0; i < VC_NUM_DAMAGE_EMITTERS; i++)
    {
        if (!damageEmitterList[i] && damageEmitterIDList[i] != 0)
        {
            if (!Sim::findObject(damageEmitterIDList[i], damageEmitterList[i]))
            {
                Con::errorf(ConsoleLogEntry::General, "VehicleData::preload Invalid packet, bad datablockId(damageEmitter): 0x%x", damageEmitterIDList[i]);
            }
        }
    }

    for (i = 0; i < VC_NUM_SPLASH_EMITTERS; i++)
    {
        if (!splashEmitterList[i] && splashEmitterIDList[i] != 0)
        {
            if (!Sim::findObject(splashEmitterIDList[i], splashEmitterList[i]))
            {
                Con::errorf(ConsoleLogEntry::General, "VehicleData::preload Invalid packet, bad datablockId(splashEmitter): 0x%x", splashEmitterIDList[i]);
            }
        }
    }

    return true;
}


//----------------------------------------------------------------------------

void VehicleData::packData(BitStream* stream)
{
    S32 i;
    Parent::packData(stream);

    stream->write(body.restitution);
    stream->write(body.friction);
    for (i = 0; i < Body::MaxSounds; i++)
        if (stream->writeFlag(body.sound[i]))
            stream->writeRangedU32(packed ? static_cast<SimObjectId>(reinterpret_cast<size_t>(body.sound[i])) :
                body.sound[i]->getId(), DataBlockObjectIdFirst,
                DataBlockObjectIdLast);

    stream->write(minImpactSpeed);
    stream->write(softImpactSpeed);
    stream->write(hardImpactSpeed);
    stream->write(minRollSpeed);
    stream->write(maxSteeringAngle);

    stream->write(maxDrag);
    stream->write(minDrag);
    stream->write(integration);
    stream->write(collisionTol);
    stream->write(contactTol);
    mathWrite(*stream, massCenter);
    mathWrite(*stream, massBox);

    stream->write(jetForce);
    stream->write(jetEnergyDrain);
    stream->write(minJetEnergy);

    stream->writeFlag(cameraRoll);
    stream->write(cameraLag);
    stream->write(cameraDecay);
    stream->write(cameraOffset);

    stream->write(triggerDustHeight);
    stream->write(dustHeight);

    stream->write(numDmgEmitterAreas);

    stream->write(exitSplashSoundVel);
    stream->write(softSplashSoundVel);
    stream->write(medSplashSoundVel);
    stream->write(hardSplashSoundVel);

    // write the water sound profiles
    for (i = 0; i < MaxSounds; i++)
        if (stream->writeFlag(waterSound[i]))
            stream->writeRangedU32(waterSound[i]->getId(), DataBlockObjectIdFirst, DataBlockObjectIdLast);

    if (stream->writeFlag(dustEmitter))
    {
        stream->writeRangedU32(dustEmitter->getId(), DataBlockObjectIdFirst, DataBlockObjectIdLast);
    }

    for (i = 0; i < VC_NUM_DAMAGE_EMITTERS; i++)
    {
        if (stream->writeFlag(damageEmitterList[i] != NULL))
        {
            stream->writeRangedU32(damageEmitterList[i]->getId(), DataBlockObjectIdFirst, DataBlockObjectIdLast);
        }
    }

    for (i = 0; i < VC_NUM_SPLASH_EMITTERS; i++)
    {
        if (stream->writeFlag(splashEmitterList[i] != NULL))
        {
            stream->writeRangedU32(splashEmitterList[i]->getId(), DataBlockObjectIdFirst, DataBlockObjectIdLast);
        }
    }

    for (int j = 0; j < VC_NUM_DAMAGE_EMITTER_AREAS; j++)
    {
        stream->write(damageEmitterOffset[j].x);
        stream->write(damageEmitterOffset[j].y);
        stream->write(damageEmitterOffset[j].z);
    }

    for (int k = 0; k < VC_NUM_DAMAGE_LEVELS; k++)
    {
        stream->write(damageLevelTolerance[k]);
    }

    stream->write(splashFreqMod);
    stream->write(splashVelEpsilon);

    stream->write(collDamageThresholdVel);
    stream->write(collDamageMultiplier);
}

void VehicleData::unpackData(BitStream* stream)
{
    Parent::unpackData(stream);

    stream->read(&body.restitution);
    stream->read(&body.friction);
    S32 i;
    for (i = 0; i < Body::MaxSounds; i++) {
        body.sound[i] = NULL;
        if (stream->readFlag())
            body.sound[i] = (SFXProfile*)stream->readRangedU32(DataBlockObjectIdFirst,
                DataBlockObjectIdLast);
    }

    stream->read(&minImpactSpeed);
    stream->read(&softImpactSpeed);
    stream->read(&hardImpactSpeed);
    stream->read(&minRollSpeed);
    stream->read(&maxSteeringAngle);

    stream->read(&maxDrag);
    stream->read(&minDrag);
    stream->read(&integration);
    stream->read(&collisionTol);
    stream->read(&contactTol);
    mathRead(*stream, &massCenter);
    mathRead(*stream, &massBox);

    stream->read(&jetForce);
    stream->read(&jetEnergyDrain);
    stream->read(&minJetEnergy);

    cameraRoll = stream->readFlag();
    stream->read(&cameraLag);
    stream->read(&cameraDecay);
    stream->read(&cameraOffset);

    stream->read(&triggerDustHeight);
    stream->read(&dustHeight);

    stream->read(&numDmgEmitterAreas);

    stream->read(&exitSplashSoundVel);
    stream->read(&softSplashSoundVel);
    stream->read(&medSplashSoundVel);
    stream->read(&hardSplashSoundVel);

    // write the water sound profiles
    for (i = 0; i < MaxSounds; i++)
        if (stream->readFlag())
        {
            U32 id = stream->readRangedU32(DataBlockObjectIdFirst, DataBlockObjectIdLast);
            waterSound[i] = dynamic_cast<SFXProfile*>(Sim::findObject(id));
        }

    if (stream->readFlag())
    {
        dustID = (S32)stream->readRangedU32(DataBlockObjectIdFirst, DataBlockObjectIdLast);
    }

    for (i = 0; i < VC_NUM_DAMAGE_EMITTERS; i++)
    {
        if (stream->readFlag())
        {
            damageEmitterIDList[i] = stream->readRangedU32(DataBlockObjectIdFirst, DataBlockObjectIdLast);
        }
    }

    for (i = 0; i < VC_NUM_SPLASH_EMITTERS; i++)
    {
        if (stream->readFlag())
        {
            splashEmitterIDList[i] = stream->readRangedU32(DataBlockObjectIdFirst, DataBlockObjectIdLast);
        }
    }

    for (int j = 0; j < VC_NUM_DAMAGE_EMITTER_AREAS; j++)
    {
        stream->read(&damageEmitterOffset[j].x);
        stream->read(&damageEmitterOffset[j].y);
        stream->read(&damageEmitterOffset[j].z);
    }

    for (int k = 0; k < VC_NUM_DAMAGE_LEVELS; k++)
    {
        stream->read(&damageLevelTolerance[k]);
    }

    stream->read(&splashFreqMod);
    stream->read(&splashVelEpsilon);

    stream->read(&collDamageThresholdVel);
    stream->read(&collDamageMultiplier);
}


//----------------------------------------------------------------------------

void VehicleData::initPersistFields()
{
    Parent::initPersistFields();

    addField("jetForce", TypeF32, Offset(jetForce, VehicleData));
    addField("jetEnergyDrain", TypeF32, Offset(jetEnergyDrain, VehicleData));
    addField("minJetEnergy", TypeF32, Offset(minJetEnergy, VehicleData));

    addField("massCenter", TypePoint3F, Offset(massCenter, VehicleData));
    addField("massBox", TypePoint3F, Offset(massBox, VehicleData));
    addField("bodyRestitution", TypeF32, Offset(body.restitution, VehicleData));
    addField("bodyFriction", TypeF32, Offset(body.friction, VehicleData));
    addField("softImpactSound", TypeSFXProfilePtr, Offset(body.sound[Body::SoftImpactSound], VehicleData));
    addField("hardImpactSound", TypeSFXProfilePtr, Offset(body.sound[Body::HardImpactSound], VehicleData));

    addField("minImpactSpeed", TypeF32, Offset(minImpactSpeed, VehicleData));
    addField("softImpactSpeed", TypeF32, Offset(softImpactSpeed, VehicleData));
    addField("hardImpactSpeed", TypeF32, Offset(hardImpactSpeed, VehicleData));
    addField("minRollSpeed", TypeF32, Offset(minRollSpeed, VehicleData));
    addField("maxSteeringAngle", TypeF32, Offset(maxSteeringAngle, VehicleData));

    addField("maxDrag", TypeF32, Offset(maxDrag, VehicleData));
    addField("minDrag", TypeF32, Offset(minDrag, VehicleData));
    addField("integration", TypeS32, Offset(integration, VehicleData));
    addField("collisionTol", TypeF32, Offset(collisionTol, VehicleData));
    addField("contactTol", TypeF32, Offset(contactTol, VehicleData));

    addField("cameraRoll", TypeBool, Offset(cameraRoll, VehicleData));
    addField("cameraLag", TypeF32, Offset(cameraLag, VehicleData));
    addField("cameraDecay", TypeF32, Offset(cameraDecay, VehicleData));
    addField("cameraOffset", TypeF32, Offset(cameraOffset, VehicleData));

    addField("dustEmitter", TypeParticleEmitterDataPtr, Offset(dustEmitter, VehicleData));
    addField("triggerDustHeight", TypeF32, Offset(triggerDustHeight, VehicleData));
    addField("dustHeight", TypeF32, Offset(dustHeight, VehicleData));

    addField("damageEmitter", TypeParticleEmitterDataPtr, Offset(damageEmitterList, VehicleData), VC_NUM_DAMAGE_EMITTERS);
    addField("splashEmitter", TypeParticleEmitterDataPtr, Offset(splashEmitterList, VehicleData), VC_NUM_SPLASH_EMITTERS);
    addField("damageEmitterOffset", TypePoint3F, Offset(damageEmitterOffset, VehicleData), VC_NUM_DAMAGE_EMITTER_AREAS);
    addField("damageLevelTolerance", TypeF32, Offset(damageLevelTolerance, VehicleData), VC_NUM_DAMAGE_LEVELS);
    addField("numDmgEmitterAreas", TypeF32, Offset(numDmgEmitterAreas, VehicleData));

    addField("splashFreqMod", TypeF32, Offset(splashFreqMod, VehicleData));
    addField("splashVelEpsilon", TypeF32, Offset(splashVelEpsilon, VehicleData));

    addField("exitSplashSoundVelocity", TypeF32, Offset(exitSplashSoundVel, VehicleData));
    addField("softSplashSoundVelocity", TypeF32, Offset(softSplashSoundVel, VehicleData));
    addField("mediumSplashSoundVelocity", TypeF32, Offset(medSplashSoundVel, VehicleData));
    addField("hardSplashSoundVelocity", TypeF32, Offset(hardSplashSoundVel, VehicleData));
    addField("exitingWater", TypeSFXProfilePtr, Offset(waterSound[ExitWater], VehicleData));
    addField("impactWaterEasy", TypeSFXProfilePtr, Offset(waterSound[ImpactSoft], VehicleData));
    addField("impactWaterMedium", TypeSFXProfilePtr, Offset(waterSound[ImpactMedium], VehicleData));
    addField("impactWaterHard", TypeSFXProfilePtr, Offset(waterSound[ImpactHard], VehicleData));
    addField("waterWakeSound", TypeSFXProfilePtr, Offset(waterSound[Wake], VehicleData));

    addField("collDamageThresholdVel", TypeF32, Offset(collDamageThresholdVel, VehicleData));
    addField("collDamageMultiplier", TypeF32, Offset(collDamageMultiplier, VehicleData));
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
IMPLEMENT_CONOBJECT(Vehicle);

Vehicle::Vehicle()
{
    mDataBlock = 0;
    mTypeMask |= VehicleObjectType;

    mDelta.pos = Point3F(0, 0, 0);
    mDelta.posVec = Point3F(0, 0, 0);
    mDelta.warpTicks = mDelta.warpCount = 0;
    mDelta.dt = 1;
    mDelta.move = NullMove;
    mPredictionCount = 0;
    mDelta.cameraOffset.set(0, 0, 0);
    mDelta.cameraVec.set(0, 0, 0);
    mDelta.cameraRot.set(0, 0, 0);
    mDelta.cameraRotVec.set(0, 0, 0);

    mRigid.linPosition.set(0, 0, 0);
    mRigid.linVelocity.set(0, 0, 0);
    mRigid.angPosition.identity();
    mRigid.angVelocity.set(0, 0, 0);
    mRigid.linMomentum.set(0, 0, 0);
    mRigid.angMomentum.set(0, 0, 0);
    mContacts.count = 0;

    mSteering.set(0, 0);
    mThrottle = 0;
    mJetting = false;

    mCameraOffset.set(0, 0, 0);

    dMemset(mDustEmitterList, 0, sizeof(mDustEmitterList));
    dMemset(mDamageEmitterList, 0, sizeof(mDamageEmitterList));
    dMemset(mSplashEmitterList, 0, sizeof(mSplashEmitterList));

    mDisableMove = false;
    restCount = 0;

    inLiquid = false;
    waterWakeHandle = 0;
}

U32 Vehicle::getCollisionMask()
{
    AssertFatal(false, "Vehicle::getCollisionMask is pure virtual!");
    return 0;
}

Point3F Vehicle::getVelocity() const
{
    return mRigid.linVelocity;
}

//----------------------------------------------------------------------------

bool Vehicle::onAdd()
{
    if (!Parent::onAdd())
        return false;

    // When loading from a mission script, the base SceneObject's transform
    // will have been set and needs to be transfered to the rigid body.
    mRigid.setTransform(mObjToWorld);

    // Initialize interpolation vars.
    mDelta.rot[1] = mDelta.rot[0] = mRigid.angPosition;
    mDelta.pos = mRigid.linPosition;
    mDelta.posVec = Point3F(0, 0, 0);

    // Create Emitters on the client
    if (isClientObject())
    {
        if (mDataBlock->dustEmitter)
        {
            for (int i = 0; i < VehicleData::VC_NUM_DUST_EMITTERS; i++)
            {
                mDustEmitterList[i] = new ParticleEmitter;
                mDustEmitterList[i]->onNewDataBlock(mDataBlock->dustEmitter);
                if (!mDustEmitterList[i]->registerObject())
                {
                    Con::warnf(ConsoleLogEntry::General, "Could not register dust emitter for class: %s", mDataBlock->getName());
                    delete mDustEmitterList[i];
                    mDustEmitterList[i] = NULL;
                }
            }
        }

        U32 j;
        for (j = 0; j < VehicleData::VC_NUM_DAMAGE_EMITTERS; j++)
        {
            if (mDataBlock->damageEmitterList[j])
            {
                mDamageEmitterList[j] = new ParticleEmitter;
                mDamageEmitterList[j]->onNewDataBlock(mDataBlock->damageEmitterList[j]);
                if (!mDamageEmitterList[j]->registerObject())
                {
                    Con::warnf(ConsoleLogEntry::General, "Could not register damage emitter for class: %s", mDataBlock->getName());
                    delete mDamageEmitterList[j];
                    mDamageEmitterList[j] = NULL;
                }

            }
        }

        for (j = 0; j < VehicleData::VC_NUM_SPLASH_EMITTERS; j++)
        {
            if (mDataBlock->splashEmitterList[j])
            {
                mSplashEmitterList[j] = new ParticleEmitter;
                mSplashEmitterList[j]->onNewDataBlock(mDataBlock->splashEmitterList[j]);
                if (!mSplashEmitterList[j]->registerObject())
                {
                    Con::warnf(ConsoleLogEntry::General, "Could not register splash emitter for class: %s", mDataBlock->getName());
                    delete mSplashEmitterList[j];
                    mSplashEmitterList[j] = NULL;
                }

            }
        }
    }

    // Create a new convex.
    AssertFatal(mDataBlock->collisionDetails[0] != -1, "Error, a vehicle must have a collision-1 detail!");
    mConvex.mObject = this;
    mConvex.pShapeBase = this;
    mConvex.hullId = 0;
    mConvex.box = mObjBox;
    mConvex.box.min.convolve(mObjScale);
    mConvex.box.max.convolve(mObjScale);
    mConvex.findNodeTransform();

    return true;
}

void Vehicle::onRemove()
{
    U32 i = 0;
    for (i = 0; i < VehicleData::VC_NUM_DUST_EMITTERS; i++)
    {
        if (mDustEmitterList[i])
        {
            mDustEmitterList[i]->deleteWhenEmpty();
            mDustEmitterList[i] = NULL;
        }
    }

    for (i = 0; i < VehicleData::VC_NUM_DAMAGE_EMITTERS; i++)
    {
        if (mDamageEmitterList[i])
        {
            mDamageEmitterList[i]->deleteWhenEmpty();
            mDamageEmitterList[i] = NULL;
        }
    }

    for (i = 0; i < VehicleData::VC_NUM_SPLASH_EMITTERS; i++)
    {
        if (mSplashEmitterList[i])
        {
            mSplashEmitterList[i]->deleteWhenEmpty();
            mSplashEmitterList[i] = NULL;
        }
    }

    Parent::onRemove();
}


//----------------------------------------------------------------------------

void Vehicle::processTick(const Move* move)
{
    Parent::processTick(move);

    // Warp to catch up to server
    if (mDelta.warpCount < mDelta.warpTicks)
    {
        mDelta.warpCount++;

        // Set new pos.
        mObjToWorld.getColumn(3, &mDelta.pos);
        mDelta.pos += mDelta.warpOffset;
        mDelta.rot[0] = mDelta.rot[1];
        mDelta.rot[1].interpolate(mDelta.warpRot[0], mDelta.warpRot[1], F32(mDelta.warpCount) / mDelta.warpTicks);
        setPosition(mDelta.pos, mDelta.rot[1]);

        // Pos backstepping
        mDelta.posVec.x = -mDelta.warpOffset.x;
        mDelta.posVec.y = -mDelta.warpOffset.y;
        mDelta.posVec.z = -mDelta.warpOffset.z;
    }
    else
    {
        if (!move)
        {
            if (isGhost())
            {
                // If we haven't run out of prediction time,
                // predict using the last known move.
                if (mPredictionCount-- <= 0)
                    return;
                move = &mDelta.move;
            }
            else
                move = &NullMove;
        }

        // Process input move
        updateMove(move);

        // Save current rigid state interpolation
        mDelta.posVec = mRigid.linPosition;
        mDelta.rot[0] = mRigid.angPosition;

        // Update the physics based on the integration rate
        S32 count = mDataBlock->integration;
        updateWorkingCollisionSet(getCollisionMask());
        for (U32 i = 0; i < count; i++)
            updatePos(TickSec / count);

        // Wrap up interpolation info
        mDelta.pos = mRigid.linPosition;
        mDelta.posVec -= mRigid.linPosition;
        mDelta.rot[1] = mRigid.angPosition;

        // Update container database
        setPosition(mRigid.linPosition, mRigid.angPosition);
        setMaskBits(PositionMask);
        updateContainer();
    }
}

void Vehicle::interpolateTick(F32 dt)
{
    Parent::interpolateTick(dt);

    if (dt == 0.0f)
        setRenderPosition(mDelta.pos, mDelta.rot[1]);
    else
    {
        QuatF rot;
        rot.interpolate(mDelta.rot[1], mDelta.rot[0], dt);
        Point3F pos = mDelta.pos + mDelta.posVec * dt;
        setRenderPosition(pos, rot);
    }
    mDelta.dt = dt;
}

void Vehicle::advanceTime(F32 dt)
{
    Parent::advanceTime(dt);

    updateLiftoffDust(dt);
    updateDamageSmoke(dt);
    updateFroth(dt);

    // Update 3rd person camera offset.  Camera update is done
    // here as it's a client side only animation.
    mCameraOffset -=
        (mCameraOffset * mDataBlock->cameraDecay +
            mRigid.linVelocity * mDataBlock->cameraLag) * dt;
}


//----------------------------------------------------------------------------

bool Vehicle::onNewDataBlock(GameBaseData* dptr)
{
    mDataBlock = dynamic_cast<VehicleData*>(dptr);
    if (!mDataBlock || !Parent::onNewDataBlock(dptr))
        return false;

    // Update Rigid Info
    mRigid.mass = mDataBlock->mass;
    mRigid.oneOverMass = 1 / mRigid.mass;
    mRigid.friction = mDataBlock->body.friction;
    mRigid.restitution = mDataBlock->body.restitution;
    mRigid.setCenterOfMass(mDataBlock->massCenter);

    // Ignores massBox, just set sphere for now. Derived objects
    // can set what they want.
    mRigid.setObjectInertia();

    return true;
}


//----------------------------------------------------------------------------

void Vehicle::getCameraParameters(F32* min, F32* max, Point3F* off, MatrixF* rot)
{
    *min = mDataBlock->cameraMinDist;
    *max = mDataBlock->cameraMaxDist;

    off->set(0, 0, mDataBlock->cameraOffset);
    rot->identity();
}


//----------------------------------------------------------------------------

void Vehicle::getCameraTransform(F32* pos, MatrixF* mat)
{
    // Returns camera to world space transform
    // Handles first person / third person camera position
    if (isServerObject() && mShapeInstance)
        mShapeInstance->animateNodeSubtrees(true);

    if (*pos == 0) {
        getRenderEyeTransform(mat);
        return;
    }

    // Get the shape's camera parameters.
    F32 min, max;
    MatrixF rot;
    Point3F offset;
    getCameraParameters(&min, &max, &offset, &rot);

    // Start with the current eye position
    MatrixF eye;
    getRenderEyeTransform(&eye);

    // Build a transform that points along the eye axis
    // but where the Z axis is always up.
    if (mDataBlock->cameraRoll)
        mat->mul(eye, rot);
    else
    {
        MatrixF cam(1);
        VectorF x, y, z(0, 0, 1);
        eye.getColumn(1, &y);
        mCross(y, z, &x);
        x.normalize();
        mCross(x, y, &z);
        z.normalize();
        cam.setColumn(0, x);
        cam.setColumn(1, y);
        cam.setColumn(2, z);
        mat->mul(cam, rot);
    }

    // Camera is positioned straight back along the eye's -Y axis.
    // A ray is cast to make sure the camera doesn't go through
    // anything solid.
    VectorF vp, vec;
    vp.x = vp.z = 0;
    vp.y = -(max - min) * *pos;
    eye.mulV(vp, &vec);

    // Use the camera node as the starting position if it exists.
    Point3F osp, sp;
    if (mDataBlock->cameraNode != -1)
    {
        mShapeInstance->mNodeTransforms[mDataBlock->cameraNode].getColumn(3, &osp);
        getRenderTransform().mulP(osp, &sp);
    }
    else
        eye.getColumn(3, &sp);

    // Make sure we don't hit ourself...
    disableCollision();
    if (isMounted())
        getObjectMount()->disableCollision();

    // Cast the ray into the container database to see if we're going
    // to hit anything.
    RayInfo collision;
    Point3F ep = sp + vec + offset + mCameraOffset;
    if (mContainer->castRay(sp, ep,
        ~(WaterObjectType | GameBaseObjectType | DefaultObjectType),
        &collision) == true) {

        // Shift the collision point back a little to try and
        // avoid clipping against the front camera plane.
        F32 t = collision.t - (-mDot(vec, collision.normal) / vec.len()) * 0.1;
        if (t > 0.0f)
            ep = sp + offset + mCameraOffset + (vec * t);
        else
            eye.getColumn(3, &ep);
    }
    mat->setColumn(3, ep);

    // Re-enable our collision.
    if (isMounted())
        getObjectMount()->enableCollision();
    enableCollision();
}


//----------------------------------------------------------------------------

void Vehicle::getVelocity(const Point3F& r, Point3F* v)
{
    mRigid.getVelocity(r, v);
}

void Vehicle::applyImpulse(const Point3F& pos, const Point3F& impulse)
{
    Point3F r;
    mRigid.getOriginVector(pos, &r);
    mRigid.applyImpulse(r, impulse);
}


//----------------------------------------------------------------------------

void Vehicle::updateMove(const Move* move)
{
    mDelta.move = *move;

    // Image Triggers
    if (mDamageState == Enabled) {
        setImageTriggerState(0, move->trigger[0]);
        setImageTriggerState(1, move->trigger[1]);
    }

    // Throttle
    if (!mDisableMove)
        mThrottle = move->y;

    // Steering
    if (move != &NullMove) {
        F32 y = move->yaw;
        mSteering.x = mClampF(mSteering.x + y, -mDataBlock->maxSteeringAngle,
            mDataBlock->maxSteeringAngle);
        F32 p = move->pitch;
        mSteering.y = mClampF(mSteering.y + p, -mDataBlock->maxSteeringAngle,
            mDataBlock->maxSteeringAngle);
    }
    else {
        mSteering.x = 0;
        mSteering.y = 0;
    }

    // Jetting flag
    if (move->trigger[3]) {
        if (!mJetting && getEnergyLevel() >= mDataBlock->minJetEnergy)
            mJetting = true;
        if (mJetting) {
            F32 newEnergy = getEnergyLevel() - mDataBlock->jetEnergyDrain;
            if (newEnergy < 0) {
                newEnergy = 0;
                mJetting = false;
            }
            setEnergyLevel(newEnergy);
        }
    }
    else
        mJetting = false;
}


//----------------------------------------------------------------------------

void Vehicle::setPosition(const Point3F& pos, const QuatF& rot)
{
    MatrixF mat;
    rot.setMatrix(&mat);
    mat.setColumn(3, pos);
    Parent::setTransform(mat);
}

void Vehicle::setRenderPosition(const Point3F& pos, const QuatF& rot)
{
    MatrixF mat;
    rot.setMatrix(&mat);
    mat.setColumn(3, pos);
    Parent::setRenderTransform(mat);
}

void Vehicle::setTransform(const MatrixF& newMat)
{
    mRigid.setTransform(newMat);
    Parent::setTransform(newMat);
    mRigid.atRest = false;
    mContacts.count = 0;
}


//-----------------------------------------------------------------------------

void Vehicle::disableCollision()
{
    Parent::disableCollision();
    for (ShapeBase* ptr = getMountList(); ptr; ptr = ptr->getMountLink())
        ptr->disableCollision();
}

void Vehicle::enableCollision()
{
    Parent::enableCollision();
    for (ShapeBase* ptr = getMountList(); ptr; ptr = ptr->getMountLink())
        ptr->enableCollision();
}


//----------------------------------------------------------------------------
/** Update the physics
*/

void Vehicle::updatePos(F32 dt)
{
    Point3F origVelocity = mRigid.linVelocity;

    // Update internal forces acting on the body.
    mRigid.clearForces();
    updateForces(dt);

    // Update collision information based on our current pos.
    bool collided = false;
    if (!mRigid.atRest) {
        collided = updateCollision(dt);

        // Now that all the forces have been processed, lets
        // see if we're at rest.  Basically, if the kinetic energy of
        // the vehicles is less than some percentage of the energy added
        // by gravity for a short period, we're considered at rest.
        // This should really be part of the rigid class...
        if (mCollisionList.count) {
            F32 k = mRigid.getKineticEnergy();
            F32 G = sVehicleGravity * dt;
            F32 Kg = 0.5 * mRigid.mass * G * G;
            if (k < sRestTol * Kg && ++restCount > sRestCount)
                mRigid.setAtRest();
        }
        else
            restCount = 0;
    }

    // Integrate forward
    if (!mRigid.atRest)
        mRigid.integrate(dt);

    // Deal with client and server scripting, sounds, etc.
    if (isServerObject()) {

        // Check triggers and other objects that we normally don't
        // collide with.  This function must be called before notifyCollision
        // as it will queue collision.
        checkTriggers();

        // Invoke the onCollision notify callback for all the objects
        // we've just hit.
        notifyCollision();

        // Server side impact script callback
        if (collided) {
            VectorF collVec = mRigid.linVelocity - origVelocity;
            F32 collSpeed = collVec.len();
            if (collSpeed > mDataBlock->minImpactSpeed)
                onImpact(collVec);
        }

        // Water script callbacks
        if (!inLiquid && mWaterCoverage != 0.0f) {
            Con::executef(mDataBlock, 4, "onEnterLiquid", scriptThis(), Con::getFloatArg(mWaterCoverage), Con::getIntArg(mLiquidType));
            inLiquid = true;
        }
        else if (inLiquid && mWaterCoverage == 0.0f) {
            Con::executef(mDataBlock, 3, "onLeaveLiquid", scriptThis(), Con::getIntArg(mLiquidType));
            inLiquid = false;
        }

    }
    else {

        // Play impact sounds on the client.
        if (collided) {
            F32 collSpeed = (mRigid.linVelocity - origVelocity).len();
            S32 impactSound = -1;
            if (collSpeed >= mDataBlock->hardImpactSpeed)
                impactSound = VehicleData::Body::HardImpactSound;
            else
                if (collSpeed >= mDataBlock->softImpactSpeed)
                    impactSound = VehicleData::Body::SoftImpactSound;

            if (impactSound != -1 && mDataBlock->body.sound[impactSound] != NULL)
                SFX->playOnce(mDataBlock->body.sound[impactSound], &getTransform());
        }

        // Water volume sounds
        F32 vSpeed = getVelocity().len();
        if (!inLiquid && mWaterCoverage >= 0.8f) {
            if (vSpeed >= mDataBlock->hardSplashSoundVel)
                SFX->playOnce(mDataBlock->waterSound[VehicleData::ImpactHard], &getTransform());
            else
                if (vSpeed >= mDataBlock->medSplashSoundVel)
                    SFX->playOnce(mDataBlock->waterSound[VehicleData::ImpactMedium], &getTransform());
                else
                    if (vSpeed >= mDataBlock->softSplashSoundVel)
                        SFX->playOnce(mDataBlock->waterSound[VehicleData::ImpactSoft], &getTransform());
            inLiquid = true;
        }
        else
            if (inLiquid && mWaterCoverage < 0.8f) {
                if (vSpeed >= mDataBlock->exitSplashSoundVel)
                    SFX->playOnce(mDataBlock->waterSound[VehicleData::ExitWater], &getTransform());
                inLiquid = false;
            }
    }
}


//----------------------------------------------------------------------------

void Vehicle::updateForces(F32 /*dt*/)
{
    // Nothing here.
}


//-----------------------------------------------------------------------------
/** Update collision information
   Update the convex state and check for collisions. If the object is in
   collision, impact and contact forces are generated.
*/

bool Vehicle::updateCollision(F32 dt)
{
    // Update collision information
    MatrixF mat, cmat;
    mConvex.transform = &mat;
    mRigid.getTransform(&mat);
    cmat = mConvex.getTransform();

    mCollisionList.count = 0;
    CollisionState* state = mConvex.findClosestState(cmat, getScale(), mDataBlock->collisionTol);
    if (state && state->dist <= mDataBlock->collisionTol)
    {
        //resolveDisplacement(ns,state,dt);
        mConvex.getCollisionInfo(cmat, getScale(), &mCollisionList, mDataBlock->collisionTol);
    }

    // Resolve collisions
    bool collided = resolveCollision(mRigid, mCollisionList);
    resolveContacts(mRigid, mCollisionList, dt);
    return collided;
}


//----------------------------------------------------------------------------
/** Resolve collision impacts
   Handle collision impacts, as opposed to contacts. Impulses are calculated based
   on standard collision resolution formulas.
*/
bool Vehicle::resolveCollision(Rigid& ns, CollisionList& cList)
{
    // Apply impulses to resolve collision
    bool colliding, collided = false;

    do
    {
        colliding = false;
        for (S32 i = 0; i < cList.count; i++)
        {
            Collision& c = cList.collision[i];
            if (c.distance < mDataBlock->collisionTol)
            {
                // Velocity into surface
                Point3F v, r;
                ns.getOriginVector(c.point, &r);
                ns.getVelocity(r, &v);
                F32 vn = mDot(v, c.normal);

                // Only interested in velocities greater than sContactTol,
                // velocities less than that will be dealt with as contacts
                // "constraints".
                if (vn < -mDataBlock->contactTol)
                {

                    // Apply impulses to the rigid body to keep it from
                    // penetrating the surface.
                    ns.resolveCollision(cList.collision[i].point,
                        cList.collision[i].normal);
                    colliding = collided = true;

                    // Keep track of objects we collide with
                    if (!isGhost() && c.object->getTypeMask() & ShapeBaseObjectType)
                    {
                        ShapeBase* col = static_cast<ShapeBase*>(c.object);
                        queueCollision(col, v - col->getVelocity());
                    }
                }
            }
        }
    } while (colliding);

    return collided;
}

//----------------------------------------------------------------------------
/** Resolve contact forces
   Resolve contact forces using the "penalty" method. Forces are generated based
   on the depth of penetration and the moment of inertia at the point of contact.
*/
bool Vehicle::resolveContacts(Rigid& ns, CollisionList& cList, F32 dt)
{
    // Use spring forces to manage contact constraints.
    bool collided = false;
    Point3F t, p(0, 0, 0), l(0, 0, 0);
    for (S32 i = 0; i < cList.count; i++)
    {
        Collision& c = cList.collision[i];
        if (c.distance < mDataBlock->collisionTol)
        {

            // Velocity into the surface
            Point3F v, r;
            ns.getOriginVector(c.point, &r);
            ns.getVelocity(r, &v);
            F32 vn = mDot(v, c.normal);

            // Only interested in velocities less than mDataBlock->contactTol,
            // velocities greater than that are dealt with as collisions.
            if (mFabs(vn) < mDataBlock->contactTol)
            {
                collided = true;

                // Penetration force. This is actually a spring which
                // will seperate the body from the collision surface.
                F32 zi = 2 * mFabs(mRigid.getZeroImpulse(r, c.normal));
                F32 s = (mDataBlock->collisionTol - c.distance) * zi - ((vn / mDataBlock->contactTol) * zi);
                Point3F f = c.normal * s;

                // Friction impulse, calculated as a function of the
                // amount of force it would take to stop the motion
                // perpendicular to the normal.
                Point3F uv = v - (c.normal * vn);
                F32 ul = uv.len();
                if (s > 0 && ul)
                {
                    uv /= -ul;
                    F32 u = ul * ns.getZeroImpulse(r, uv);
                    s *= mRigid.friction;
                    if (u > s)
                        u = s;
                    f += uv * u;
                }

                // Accumulate forces
                p += f;
                mCross(r, f, &t);
                l += t;
            }
        }
    }

    // Contact constraint forces act over time...
    ns.linMomentum += p * dt;
    ns.angMomentum += l * dt;
    ns.updateVelocity();
    return true;
}


//----------------------------------------------------------------------------

bool Vehicle::resolveDisplacement(Rigid& ns, CollisionState* state, F32 dt)
{
    SceneObject* obj = (state->a->getObject() == this) ?
        state->b->getObject() : state->a->getObject();

    if (obj->isDisplacable() && ((obj->getTypeMask() & ShapeBaseObjectType) != 0))
    {
        // Try to displace the object by the amount we're trying to move
        Point3F objNewMom = ns.linVelocity * obj->getMass() * 1.1;
        Point3F objOldMom = obj->getMomentum();
        Point3F objNewVel = objNewMom / obj->getMass();

        Point3F myCenter;
        Point3F theirCenter;
        getWorldBox().getCenter(&myCenter);
        obj->getWorldBox().getCenter(&theirCenter);
        if (mDot(myCenter - theirCenter, objNewMom) >= 0.0f || objNewVel.len() < 0.01)
        {
            objNewMom = (theirCenter - myCenter);
            objNewMom.normalize();
            objNewMom *= 1.0f * obj->getMass();
            objNewVel = objNewMom / obj->getMass();
        }

        obj->setMomentum(objNewMom);
        if (obj->displaceObject(objNewVel * 1.1 * dt) == true)
        {
            // Queue collision and change in velocity
            VectorF dv = (objOldMom - objNewMom) / obj->getMass();
            queueCollision(static_cast<ShapeBase*>(obj), dv);
            return true;
        }
    }

    return false;
}


//----------------------------------------------------------------------------

void Vehicle::updateWorkingCollisionSet(const U32 mask)
{
    // First, we need to adjust our velocity for possible acceleration.  It is assumed
    // that we will never accelerate more than 20 m/s for gravity, plus 30 m/s for
    // jetting, and an equivalent 10 m/s for vehicle accel.  We also assume that our
    // working list is updated on a Tick basis, which means we only expand our box by
    // the possible movement in that tick, plus some extra for caching purposes
    Box3F convexBox = mConvex.getBoundingBox(getTransform(), getScale());
    F32 len = (mRigid.linVelocity.len() + 50) * TickSec;
    F32 l = (len * 1.1) + 0.1;  // fudge factor
    convexBox.min -= Point3F(l, l, l);
    convexBox.max += Point3F(l, l, l);

    disableCollision();
    mConvex.updateWorkingList(convexBox, mask);
    enableCollision();
}


//----------------------------------------------------------------------------
/** Check collisions with trigger and items
   Perform a container search using the current bounding box
   of the main body, wheels are not included.  This method should
   only be called on the server.
*/
void Vehicle::checkTriggers()
{
    Box3F bbox = mConvex.getBoundingBox(getTransform(), getScale());
    gServerContainer.findObjects(bbox, sTriggerMask, findCallback, this);
}

/** The callback used in by the checkTriggers() method.
   The checkTriggers method uses a container search which will
   invoke this callback on each obj that matches.
*/
void Vehicle::findCallback(SceneObject* obj, void* key)
{
    Vehicle* vehicle = reinterpret_cast<Vehicle*>(key);
    U32 objectMask = obj->getTypeMask();

    // Check: triggers, corpses and items, basically the same things
    // that the player class checks for
    if (objectMask & TriggerObjectType) {
        Trigger* pTrigger = static_cast<Trigger*>(obj);
        pTrigger->potentialEnterObject(vehicle);
    }
    else if (objectMask & CorpseObjectType) {
        ShapeBase* col = static_cast<ShapeBase*>(obj);
        vehicle->queueCollision(col, vehicle->getVelocity() - col->getVelocity());
    }
    else if (objectMask & ItemObjectType) {
        Item* item = static_cast<Item*>(obj);
        if (vehicle != item->getCollisionObject())
            vehicle->queueCollision(item, vehicle->getVelocity() - item->getVelocity());
    }
}


//----------------------------------------------------------------------------

void Vehicle::writePacketData(GameConnection* connection, BitStream* stream)
{
    Parent::writePacketData(connection, stream);
    mathWrite(*stream, mSteering);

    mathWrite(*stream, mRigid.linPosition);
    mathWrite(*stream, mRigid.angPosition);
    mathWrite(*stream, mRigid.linMomentum);
    mathWrite(*stream, mRigid.angMomentum);
    stream->writeFlag(mRigid.atRest);
    stream->writeFlag(mContacts.count == 0);

    stream->writeFlag(mDisableMove);
    stream->setCompressionPoint(mRigid.linPosition);
}

void Vehicle::readPacketData(GameConnection* connection, BitStream* stream)
{
    Parent::readPacketData(connection, stream);
    mathRead(*stream, &mSteering);

    mathRead(*stream, &mRigid.linPosition);
    mathRead(*stream, &mRigid.angPosition);
    mathRead(*stream, &mRigid.linMomentum);
    mathRead(*stream, &mRigid.angMomentum);
    mRigid.atRest = stream->readFlag();
    if (stream->readFlag())
        mContacts.count = 0;
    mRigid.updateInertialTensor();
    mRigid.updateVelocity();
    mRigid.updateCenterOfMass();

    mDisableMove = stream->readFlag();
    stream->setCompressionPoint(mRigid.linPosition);
}


//----------------------------------------------------------------------------

U32 Vehicle::packUpdate(NetConnection* con, U32 mask, BitStream* stream)
{
    U32 retMask = Parent::packUpdate(con, mask, stream);

    stream->writeFlag(mJetting);

    // The rest of the data is part of the control object packet update.
    // If we're controlled by this client, we don't need to send it.
    if (stream->writeFlag(getControllingClient() == con && !(mask & InitialUpdateMask)))
        return retMask;

    F32 yaw = (mSteering.x + mDataBlock->maxSteeringAngle) / (2 * mDataBlock->maxSteeringAngle);
    F32 pitch = (mSteering.y + mDataBlock->maxSteeringAngle) / (2 * mDataBlock->maxSteeringAngle);
    stream->writeFloat(yaw, 9);
    stream->writeFloat(pitch, 9);
    mDelta.move.pack(stream);

    if (stream->writeFlag(mask & PositionMask))
    {
        stream->writeCompressedPoint(mRigid.linPosition);
        mathWrite(*stream, mRigid.angPosition);
        mathWrite(*stream, mRigid.linMomentum);
        mathWrite(*stream, mRigid.angMomentum);
        stream->writeFlag(mRigid.atRest);
    }

    // send energy only to clients which need it
    bool found = false;
    if (mask & EnergyMask)
    {
        for (ShapeBase* ptr = getMountList(); ptr; ptr = ptr->getMountLink())
        {
            GameConnection* controllingClient = ptr->getControllingClient();
            if (controllingClient == con)
            {
                if (controllingClient->getControlObject() != this)
                    found = true;
                break;
            }
        }
    }

    // write it...
    if (stream->writeFlag(found))
        stream->writeFloat(mClampF(getEnergyValue(), 0.f, 1.f), 8);

    return retMask;
}

void Vehicle::unpackUpdate(NetConnection* con, BitStream* stream)
{
    Parent::unpackUpdate(con, stream);

    mJetting = stream->readFlag();

    if (stream->readFlag())
        return;

    F32 yaw = stream->readFloat(9);
    F32 pitch = stream->readFloat(9);
    mSteering.x = (2 * yaw * mDataBlock->maxSteeringAngle) - mDataBlock->maxSteeringAngle;
    mSteering.y = (2 * pitch * mDataBlock->maxSteeringAngle) - mDataBlock->maxSteeringAngle;
    mDelta.move.unpack(stream);

    if (stream->readFlag())
    {
        mPredictionCount = sMaxPredictionTicks;
        F32 speed = mRigid.linVelocity.len();
        mDelta.warpRot[0] = mRigid.angPosition;

        // Read in new position and momentum values
        stream->readCompressedPoint(&mRigid.linPosition);
        mathRead(*stream, &mRigid.angPosition);
        mathRead(*stream, &mRigid.linMomentum);
        mathRead(*stream, &mRigid.angMomentum);
        mRigid.atRest = stream->readFlag();
        mRigid.updateVelocity();

        if (isProperlyAdded())
        {
            // Determine number of ticks to warp based on the average
            // of the client and server velocities.
            Point3F cp = mDelta.pos + mDelta.posVec * mDelta.dt;
            mDelta.warpOffset = mRigid.linPosition - cp;

            // Calc the distance covered in one tick as the average of
            // the old speed and the new speed from the server.
            F32 dt, as = (speed + mRigid.linVelocity.len()) * 0.5 * TickSec;

            // Cal how many ticks it will take to cover the warp offset.
            // If it's less than what's left in the current tick, we'll just
            // warp in the remaining time.
            if (!as || (dt = mDelta.warpOffset.len() / as) > sMaxWarpTicks)
                dt = mDelta.dt + sMaxWarpTicks;
            else
                dt = (dt <= mDelta.dt) ? mDelta.dt : mCeil(dt - mDelta.dt) + mDelta.dt;

            // Adjust current frame interpolation
            if (mDelta.dt) {
                mDelta.pos = cp + (mDelta.warpOffset * (mDelta.dt / dt));
                mDelta.posVec = (cp - mDelta.pos) / mDelta.dt;
                QuatF cr;
                cr.interpolate(mDelta.rot[1], mDelta.rot[0], mDelta.dt);
                mDelta.rot[1].interpolate(cr, mRigid.angPosition, mDelta.dt / dt);
                mDelta.rot[0].extrapolate(mDelta.rot[1], cr, mDelta.dt);
            }

            // Calculated multi-tick warp
            mDelta.warpCount = 0;
            mDelta.warpTicks = (S32)(mFloor(dt));
            if (mDelta.warpTicks)
            {
                mDelta.warpOffset = mRigid.linPosition - mDelta.pos;
                mDelta.warpOffset /= mDelta.warpTicks;
                mDelta.warpRot[0] = mDelta.rot[1];
                mDelta.warpRot[1] = mRigid.angPosition;
            }
        }
        else
        {
            // Set the vehicle to the server position
            mDelta.dt = 0;
            mDelta.pos = mRigid.linPosition;
            mDelta.posVec.set(0, 0, 0);
            mDelta.rot[1] = mDelta.rot[0] = mRigid.angPosition;
            mDelta.warpCount = mDelta.warpTicks = 0;
            setPosition(mRigid.linPosition, mRigid.angPosition);
        }
    }

    // energy?
    if (stream->readFlag())
        setEnergyLevel(stream->readFloat(8) * mDataBlock->maxEnergy);
}


//----------------------------------------------------------------------------

void Vehicle::initPersistFields()
{
    Parent::initPersistFields();

    addField("disableMove", TypeBool, Offset(mDisableMove, Vehicle));
}


void Vehicle::mountObject(ShapeBase* obj, U32 node)
{
    Parent::mountObject(obj, node);

    // Clear objects off the working list that are from objects mounted to us.
    //  (This applies mostly to players...)
    for (CollisionWorkingList* itr = mConvex.getWorkingList().wLink.mNext;
        itr != &mConvex.getWorkingList();
        itr = itr->wLink.mNext)
    {
        if (itr->mConvex->getObject() == obj)
        {
            CollisionWorkingList* cl = itr;
            itr = itr->wLink.mPrev;
            cl->free();
        }
    }
}

//----------------------------------------------------------------------------

void Vehicle::updateLiftoffDust(F32 dt)
{
    if (!mDustEmitterList[0]) return;

    Point3F startPos = getPosition();
    Point3F endPos = startPos + Point3F(0.0, 0.0, -mDataBlock->triggerDustHeight);


    RayInfo rayInfo;
    if (!getContainer()->castRay(startPos, endPos, AtlasObjectType | TerrainObjectType, &rayInfo))
    {
        return;
    }

    TerrainBlock* tBlock = static_cast<TerrainBlock*>(rayInfo.object);
    S32 mapIndex = tBlock->mMPMIndex[0];

    MaterialPropertyMap* pMatMap = MaterialPropertyMap::get();
    const MaterialPropertyMap::MapEntry* pEntry = pMatMap->getMapEntryFromIndex(mapIndex);

    if (pEntry)
    {
        S32 x;
        ColorF colorList[ParticleData::PDC_NUM_KEYS];

        for (x = 0; x < 2; ++x)
            colorList[x].set(pEntry->puffColor[x].red, pEntry->puffColor[x].green, pEntry->puffColor[x].blue, pEntry->puffColor[x].alpha);
        for (x = 2; x < ParticleData::PDC_NUM_KEYS; ++x)
            colorList[x].set(1.0, 1.0, 1.0, 0.0);

        mDustEmitterList[0]->setColors(colorList);
    }
    Point3F contactPoint = rayInfo.point + Point3F(0.0, 0.0, mDataBlock->dustHeight);
    mDustEmitterList[0]->emitParticles(contactPoint, contactPoint, rayInfo.normal, getVelocity(), (U32)(dt * 1000));
}


//----------------------------------------------------------------------------

void Vehicle::updateDamageSmoke(F32 dt)
{

    for (S32 j = VehicleData::VC_NUM_DAMAGE_LEVELS - 1; j >= 0; j--)
    {
        F32 damagePercent = mDamage / mDataBlock->maxDamage;
        if (damagePercent >= mDataBlock->damageLevelTolerance[j])
        {
            for (int i = 0; i < mDataBlock->numDmgEmitterAreas; i++)
            {
                MatrixF trans = getTransform();
                Point3F offset = mDataBlock->damageEmitterOffset[i];
                trans.mulP(offset);
                Point3F emitterPoint = offset;

                if (pointInWater(offset))
                {
                    U32 emitterOffset = VehicleData::VC_BUBBLE_EMITTER;
                    if (mDamageEmitterList[emitterOffset])
                    {
                        mDamageEmitterList[emitterOffset]->emitParticles(emitterPoint, emitterPoint, Point3F(0.0, 0.0, 1.0), getVelocity(), (U32)(dt * 1000));
                    }
                }
                else
                {
                    if (mDamageEmitterList[j])
                    {
                        mDamageEmitterList[j]->emitParticles(emitterPoint, emitterPoint, Point3F(0.0, 0.0, 1.0), getVelocity(), (U32)(dt * 1000));
                    }
                }
            }
            break;
        }
    }

}


//--------------------------------------------------------------------------
void Vehicle::updateFroth(F32 dt)
{
    // update bubbles
    Point3F moveDir = getVelocity();

    Point3F contactPoint;
    if (!collidingWithWater(contactPoint))
    {
        if (waterWakeHandle)
            waterWakeHandle->stop();
        return;
    }

    F32 speed = moveDir.len();
    if (speed < mDataBlock->splashVelEpsilon) speed = 0.0;

    U32 emitRate = (U32)(speed * mDataBlock->splashFreqMod * dt);

    U32 i;
    if (waterWakeHandle)
    {
        if (!waterWakeHandle->isPlaying())
            waterWakeHandle->play();

        waterWakeHandle->setTransform(getTransform());
        waterWakeHandle->setVelocity(getVelocity());
    }

    for (i = 0; i < VehicleData::VC_NUM_SPLASH_EMITTERS; i++)
    {
        if (mSplashEmitterList[i])
        {
            mSplashEmitterList[i]->emitParticles(contactPoint, contactPoint, Point3F(0.0, 0.0, 1.0),
                moveDir, emitRate);
        }
    }

}


//--------------------------------------------------------------------------
// Returns true if vehicle is intersecting a water surface (roughly)
//--------------------------------------------------------------------------
bool Vehicle::collidingWithWater(Point3F& waterHeight)
{
    Point3F curPos = getPosition();

    F32 height = mFabs(mObjBox.max.z - mObjBox.min.z);

    RayInfo rInfo;
    if (getCurrentClientContainer()->castRay(curPos + Point3F(0.0, 0.0, height), curPos, WaterObjectType, &rInfo))
    {
        waterHeight = rInfo.point;
        return true;
    }

    return false;
}

void Vehicle::setEnergyLevel(F32 energy)
{
    Parent::setEnergyLevel(energy);
    setMaskBits(EnergyMask);
}

//-----------------------------------------------------------------------------
//
void Vehicle::renderImage(SceneState* state)
{
    Parent::renderImage(state);

    if (gShowBoundingBox)
    {
        GFX->setLightingEnable(false);

        GFX->pushWorldMatrix();

        GFX->multWorld(getRenderTransform());

        // Box for the center of Mass
        GFX->setZEnable(false);
        GFX->drawWireCube(Point3F(0.1, 0.1, 0.1), mDataBlock->massCenter, ColorI(255, 255, 255));

        GFX->popWorldMatrix();

        // Collision points...
        for (int i = 0; i < mCollisionList.count; i++)
        {
            Collision& collision = mCollisionList.collision[i];
            GFX->drawWireCube(Point3F(0.05, 0.05, 0.05), collision.point, ColorI(0, 0, 255));
        }

        // Render the normals as one big batch... 
        PrimBuild::begin(GFXLineList, mCollisionList.count * 2);
        for (int i = 0; i < mCollisionList.count; i++)
        {

            Collision& collision = mCollisionList.collision[i];
            PrimBuild::color3f(1, 1, 1);
            PrimBuild::vertex3fv(collision.point);
            PrimBuild::vertex3fv(collision.point + collision.normal * 0.05);
        }
        PrimBuild::end();

        // Build and render the collision polylist which is returned
        // in the server's world space.
        ClippedPolyList polyList;
        polyList.mPlaneList.setSize(6);
        polyList.mPlaneList[0].set(getWorldBox().min, VectorF(-1, 0, 0));
        polyList.mPlaneList[1].set(getWorldBox().min, VectorF(0, -1, 0));
        polyList.mPlaneList[2].set(getWorldBox().min, VectorF(0, 0, -1));
        polyList.mPlaneList[3].set(getWorldBox().max, VectorF(1, 0, 0));
        polyList.mPlaneList[4].set(getWorldBox().max, VectorF(0, 1, 0));
        polyList.mPlaneList[5].set(getWorldBox().max, VectorF(0, 0, 1));
        Box3F dummyBox;
        SphereF dummySphere;
        buildPolyList(&polyList, dummyBox, dummySphere);
        polyList.render();

        //
        GFX->setZEnable(true);
        GFX->setLightingEnable(false);
    }
}

//-----------------------------------------------------------------------------
// Demonstrates adding rendering to mounted objects.
//
void Vehicle::renderMountedImage(SceneState* state, RenderInst* ri)
{
    Parent::renderMountedImage(state, ri);

    if (gShowBoundingBox)
    {
        U32 index = ri->mountedObjIdx;
        if (mMountedImageList[index].dataBlock != NULL)
        {
            Point3F muzzlePoint, muzzleVector, endpoint;
            getMuzzlePoint(index, &muzzlePoint);
            getMuzzleVector(index, &muzzleVector);
            endpoint = muzzlePoint + muzzleVector * 250;

            // Lighting has been installed by ShapeBase::renderObject.  Switch lighting
            // off while rendering the debug graphics.

            GFX->setLightingEnable(false);
            GFX->setAlphaBlendEnable(false);
            GFX->setSrcBlend(GFXBlendSrcAlpha);
            GFX->setDestBlend(GFXBlendInvSrcAlpha);

            // glLineWidth(1);

            PrimBuild::begin(GFXLineList, 2);

            PrimBuild::color4f(0, 1, 0, 1);
            PrimBuild::vertex3fv(muzzlePoint);
            PrimBuild::vertex3fv(endpoint);

            PrimBuild::end();

            GFX->setAlphaBlendEnable(false);
            GFX->setLightingEnable(true);
        }
    }
}
