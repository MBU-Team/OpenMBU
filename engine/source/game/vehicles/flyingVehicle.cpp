//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "game/vehicles/flyingVehicle.h"

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
#include "game/missionArea.h"

//----------------------------------------------------------------------------

const static U32 sCollisionMoveMask = (TerrainObjectType | InteriorObjectType |
    WaterObjectType | PlayerObjectType |
    StaticShapeObjectType | VehicleObjectType |
    VehicleBlockerObjectType | StaticTSObjectType |
    AtlasObjectType);
static U32 sServerCollisionMask = sCollisionMoveMask; // ItemObjectType
static U32 sClientCollisionMask = sCollisionMoveMask;

static F32 sFlyingVehicleGravity = -20;

// Sound
static F32 sIdleEngineVolume = 0.2;

//
const char* FlyingVehicle::sJetSequence[FlyingVehicle::JetAnimCount] =
{
   "activateBack",
   "maintainBack",
   "activateBot",
   "maintainBot",
};

const char* FlyingVehicleData::sJetNode[FlyingVehicleData::MaxJetNodes] =
{
   "JetNozzle0",  // Thrust Forward
   "JetNozzle1",
   "JetNozzleX",  // Thrust Backward
   "JetNozzleY",
   "JetNozzle2",  // Thrust Downward
   "JetNozzle3",
   "contrail0",   // Trail
   "contrail1",
   "contrail2",
   "contrail3",
};

// Convert thrust direction into nodes & emitters
FlyingVehicle::JetActivation FlyingVehicle::sJetActivation[NumThrustDirections] = {
   { FlyingVehicleData::ForwardJetNode, FlyingVehicleData::ForwardJetEmitter },
   { FlyingVehicleData::BackwardJetNode, FlyingVehicleData::BackwardJetEmitter },
   { FlyingVehicleData::DownwardJetNode, FlyingVehicleData::DownwardJetEmitter },
};


//----------------------------------------------------------------------------

IMPLEMENT_CO_DATABLOCK_V1(FlyingVehicleData);

FlyingVehicleData::FlyingVehicleData()
{
    maneuveringForce = 0;
    horizontalSurfaceForce = 0;
    verticalSurfaceForce = 0;
    autoInputDamping = 1;
    steeringForce = 1;
    steeringRollForce = 1;
    rollForce = 1;
    autoAngularForce = 0;
    rotationalDrag = 0;
    autoLinearForce = 0;
    maxAutoSpeed = 0;
    hoverHeight = 2;
    createHoverHeight = 2;
    maxSteeringAngle = M_PI;
    minTrailSpeed = 1;
    maxSpeed = 100;

    for (S32 k = 0; k < MaxJetNodes; k++)
        jetNode[k] = -1;

    for (S32 j = 0; j < MaxJetEmitters; j++)
        jetEmitter[j] = 0;

    for (S32 i = 0; i < MaxSounds; i++)
        sound[i] = 0;

    vertThrustMultiple = 1.0;
}

bool FlyingVehicleData::preload(bool server, char errorBuffer[256])
{
    if (!Parent::preload(server, errorBuffer))
        return false;

    TSShapeInstance* si = new TSShapeInstance(shape, false);

    // Resolve objects transmitted from server
    if (!server) {
        for (S32 i = 0; i < MaxSounds; i++)
            if (sound[i])
                Sim::findObject(static_cast<SimObjectId>(reinterpret_cast<size_t>(sound[i])), sound[i]);

        for (S32 j = 0; j < MaxJetEmitters; j++)
            if (jetEmitter[j])
                Sim::findObject(static_cast<SimObjectId>(reinterpret_cast<size_t>(jetEmitter[j])), jetEmitter[j]);
    }

    // Extract collision planes from shape collision detail level
    if (collisionDetails[0] != -1)
    {
        MatrixF imat(1);
        PlaneExtractorPolyList polyList;
        polyList.mPlaneList = &rigidBody.mPlaneList;
        polyList.setTransform(&imat, Point3F(1, 1, 1));
        si->animate(collisionDetails[0]);
        si->buildPolyList(&polyList, collisionDetails[0]);
    }

    // Resolve jet nodes
    for (S32 j = 0; j < MaxJetNodes; j++)
        jetNode[j] = shape->findNode(sJetNode[j]);

    //
    maxSpeed = maneuveringForce / minDrag;

    delete si;
    return true;
}

void FlyingVehicleData::initPersistFields()
{
    Parent::initPersistFields();

    addField("jetSound", TypeSFXProfilePtr, Offset(sound[JetSound], FlyingVehicleData));
    addField("engineSound", TypeSFXProfilePtr, Offset(sound[EngineSound], FlyingVehicleData));

    addField("maneuveringForce", TypeF32, Offset(maneuveringForce, FlyingVehicleData));
    addField("horizontalSurfaceForce", TypeF32, Offset(horizontalSurfaceForce, FlyingVehicleData));
    addField("verticalSurfaceForce", TypeF32, Offset(verticalSurfaceForce, FlyingVehicleData));
    addField("autoInputDamping", TypeF32, Offset(autoInputDamping, FlyingVehicleData));
    addField("steeringForce", TypeF32, Offset(steeringForce, FlyingVehicleData));
    addField("steeringRollForce", TypeF32, Offset(steeringRollForce, FlyingVehicleData));
    addField("rollForce", TypeF32, Offset(rollForce, FlyingVehicleData));
    addField("autoAngularForce", TypeF32, Offset(autoAngularForce, FlyingVehicleData));
    addField("rotationalDrag", TypeF32, Offset(rotationalDrag, FlyingVehicleData));
    addField("autoLinearForce", TypeF32, Offset(autoLinearForce, FlyingVehicleData));
    addField("maxAutoSpeed", TypeF32, Offset(maxAutoSpeed, FlyingVehicleData));
    addField("hoverHeight", TypeF32, Offset(hoverHeight, FlyingVehicleData));
    addField("createHoverHeight", TypeF32, Offset(createHoverHeight, FlyingVehicleData));

    addField("forwardJetEmitter", TypeParticleEmitterDataPtr, Offset(jetEmitter[ForwardJetEmitter], FlyingVehicleData));
    addField("backwardJetEmitter", TypeParticleEmitterDataPtr, Offset(jetEmitter[BackwardJetEmitter], FlyingVehicleData));
    addField("downJetEmitter", TypeParticleEmitterDataPtr, Offset(jetEmitter[DownwardJetEmitter], FlyingVehicleData));
    addField("trailEmitter", TypeParticleEmitterDataPtr, Offset(jetEmitter[TrailEmitter], FlyingVehicleData));
    addField("minTrailSpeed", TypeF32, Offset(minTrailSpeed, FlyingVehicleData));
    addField("vertThrustMultiple", TypeF32, Offset(vertThrustMultiple, FlyingVehicleData));
}

void FlyingVehicleData::packData(BitStream* stream)
{
    Parent::packData(stream);

    for (S32 i = 0; i < MaxSounds; i++)
    {
        if (stream->writeFlag(sound[i]))
        {
            SimObjectId writtenId = packed ? static_cast<SimObjectId>(reinterpret_cast<size_t>(sound[i])) : sound[i]->getId();
            stream->writeRangedU32(writtenId, DataBlockObjectIdFirst, DataBlockObjectIdLast);
        }
    }

    for (S32 j = 0; j < MaxJetEmitters; j++)
    {
        if (stream->writeFlag(jetEmitter[j]))
        {
            SimObjectId writtenId = packed ? static_cast<SimObjectId>(reinterpret_cast<size_t>(jetEmitter[j])) : jetEmitter[j]->getId();
            stream->writeRangedU32(writtenId, DataBlockObjectIdFirst, DataBlockObjectIdLast);
        }
    }

    stream->write(maneuveringForce);
    stream->write(horizontalSurfaceForce);
    stream->write(verticalSurfaceForce);
    stream->write(autoInputDamping);
    stream->write(steeringForce);
    stream->write(steeringRollForce);
    stream->write(rollForce);
    stream->write(autoAngularForce);
    stream->write(rotationalDrag);
    stream->write(autoLinearForce);
    stream->write(maxAutoSpeed);
    stream->write(hoverHeight);
    stream->write(createHoverHeight);
    stream->write(minTrailSpeed);
    stream->write(vertThrustMultiple);
}

void FlyingVehicleData::unpackData(BitStream* stream)
{
    Parent::unpackData(stream);

    for (S32 i = 0; i < MaxSounds; i++) {
        sound[i] = NULL;
        if (stream->readFlag())
            sound[i] = (SFXProfile*)stream->readRangedU32(DataBlockObjectIdFirst,
                DataBlockObjectIdLast);
    }

    for (S32 j = 0; j < MaxJetEmitters; j++) {
        jetEmitter[j] = NULL;
        if (stream->readFlag())
            jetEmitter[j] = (ParticleEmitterData*)stream->readRangedU32(DataBlockObjectIdFirst,
                DataBlockObjectIdLast);
    }

    stream->read(&maneuveringForce);
    stream->read(&horizontalSurfaceForce);
    stream->read(&verticalSurfaceForce);
    stream->read(&autoInputDamping);
    stream->read(&steeringForce);
    stream->read(&steeringRollForce);
    stream->read(&rollForce);
    stream->read(&autoAngularForce);
    stream->read(&rotationalDrag);
    stream->read(&autoLinearForce);
    stream->read(&maxAutoSpeed);
    stream->read(&hoverHeight);
    stream->read(&createHoverHeight);
    stream->read(&minTrailSpeed);
    stream->read(&vertThrustMultiple);
}


//----------------------------------------------------------------------------

IMPLEMENT_CO_NETOBJECT_V1(FlyingVehicle);

FlyingVehicle::FlyingVehicle()
{
    mSteering.set(0, 0);
    mThrottle = 0;
    mJetting = false;

    mJetSound = 0;
    mEngineSound = 0;

    mBackMaintainOn = false;
    mBottomMaintainOn = false;
    createHeightOn = false;

    for (S32 i = 0; i < JetAnimCount; i++)
        mJetThread[i] = 0;
}

FlyingVehicle::~FlyingVehicle()
{
    
}


//----------------------------------------------------------------------------

bool FlyingVehicle::onAdd()
{
    if (!Parent::onAdd())
        return false;

    addToScene();

    if (isServerObject())
        scriptOnAdd();
    return true;
}

bool FlyingVehicle::onNewDataBlock(GameBaseData* dptr)
{
    mDataBlock = dynamic_cast<FlyingVehicleData*>(dptr);
    if (!mDataBlock || !Parent::onNewDataBlock(dptr))
        return false;

    // Sounds
    if (isGhost())
    {
        // Create the sounds ahead of time.  This reduces runtime
        // costs and makes the system easier to understand.

        SFX_DELETE(mJetSound);
        SFX_DELETE(mEngineSound);

        if (mDataBlock->sound[FlyingVehicleData::EngineSound])
            mEngineSound = SFX->createSource(mDataBlock->sound[FlyingVehicleData::EngineSound], &getTransform());

        if (mDataBlock->sound[FlyingVehicleData::JetSound])
            mJetSound = SFX->createSource(mDataBlock->sound[FlyingVehicleData::JetSound], &getTransform());
    }

    // Jet Sequences
    for (S32 i = 0; i < JetAnimCount; i++) {
        TSShape const* shape = mShapeInstance->getShape();
        mJetSeq[i] = shape->findSequence(sJetSequence[i]);
        if (mJetSeq[i] != -1) {
            if (i == BackActivate || i == BottomActivate) {
                mJetThread[i] = mShapeInstance->addThread();
                mShapeInstance->setSequence(mJetThread[i], mJetSeq[i], 0);
                mShapeInstance->setTimeScale(mJetThread[i], 0);
            }
        }
        else
            mJetThread[i] = 0;
    }

    scriptOnNewDataBlock();
    return true;
}

void FlyingVehicle::onRemove()
{
    scriptOnRemove();
    removeFromScene();
    Parent::onRemove();
}


//----------------------------------------------------------------------------

void FlyingVehicle::advanceTime(F32 dt)
{
    Parent::advanceTime(dt);

    updateEngineSound(1);
    updateJet(dt);
}


//----------------------------------------------------------------------------

void FlyingVehicle::updateMove(const Move* move)
{
    Parent::updateMove(move);

    if (move == &NullMove)
        mSteering.set(0, 0);

    F32 speed = mRigid.linVelocity.len();
    if (speed < mDataBlock->maxAutoSpeed)
        mSteering *= mDataBlock->autoInputDamping;

    // Check the mission area to get the factor for the flight ceiling
    MissionArea* obj = dynamic_cast<MissionArea*>(Sim::findObject("MissionArea"));
    mCeilingFactor = 1.0f;
    if (obj != NULL)
    {
        F32 flightCeiling = obj->getFlightCeiling();
        F32 ceilingRange = obj->getFlightCeilingRange();

        if (mRigid.linPosition.z > flightCeiling)
        {
            // Thrust starts to fade at the ceiling, and is 0 at ceil + range
            if (ceilingRange == 0)
            {
                mCeilingFactor = 0;
            }
            else
            {
                mCeilingFactor = 1.0f - ((mRigid.linPosition.z - flightCeiling) / (flightCeiling + ceilingRange));
                if (mCeilingFactor < 0.0f)
                    mCeilingFactor = 0.0f;
            }
        }
    }

    mThrust.x = move->x;
    mThrust.y = move->y;

    if (mThrust.y != 0.0f)
        if (mThrust.y > 0)
            mThrustDirection = ThrustForward;
        else
            mThrustDirection = ThrustBackward;
    else
        mThrustDirection = ThrustDown;

    if (mCeilingFactor != 1.0f)
        mJetting = false;
}


//----------------------------------------------------------------------------

Point3F JetOffset[4] =
{
   Point3F(-1,-1,0),
   Point3F(+1,-1,0),
   Point3F(-1,+1,0),
   Point3F(+1,+1,0)
};

void FlyingVehicle::updateForces(F32 /*dt*/)
{
    MatrixF currPosMat;
    mRigid.getTransform(&currPosMat);
    mRigid.atRest = false;

    Point3F massCenter;
    currPosMat.mulP(mDataBlock->massCenter, &massCenter);

    Point3F xv, yv, zv;
    currPosMat.getColumn(0, &xv);
    currPosMat.getColumn(1, &yv);
    currPosMat.getColumn(2, &zv);
    F32 speed = mRigid.linVelocity.len();

    Point3F force = Point3F(0, 0, sFlyingVehicleGravity * mRigid.mass * mGravityMod);
    Point3F torque = Point3F(0, 0, 0);

    // Drag at any speed
    force -= mRigid.linVelocity * mDataBlock->minDrag;
    torque -= mRigid.angMomentum * mDataBlock->rotationalDrag;

    // Auto-stop at low speeds
    if (speed < mDataBlock->maxAutoSpeed) {
        F32 autoScale = 1 - speed / mDataBlock->maxAutoSpeed;

        // Gyroscope
        F32 gf = mDataBlock->autoAngularForce * autoScale;
        torque -= xv * gf * mDot(yv, Point3F(0, 0, 1));

        // Manuevering jets
        F32 sf = mDataBlock->autoLinearForce * autoScale;
        force -= yv * sf * mDot(yv, mRigid.linVelocity);
        force -= xv * sf * mDot(xv, mRigid.linVelocity);
    }

    // Hovering Jet
    F32 vf = -sFlyingVehicleGravity * mRigid.mass * mGravityMod;
    F32 h = getHeight();
    if (h <= 1) {
        if (h > 0) {
            vf -= vf * h * 0.1;
        }
        else {
            vf += mDataBlock->jetForce * -h;
        }
    }
    force += zv * vf;

    // Damping "surfaces"
    force -= xv * speed * mDot(xv, mRigid.linVelocity) * mDataBlock->horizontalSurfaceForce;
    force -= zv * speed * mDot(zv, mRigid.linVelocity) * mDataBlock->verticalSurfaceForce;

    // Turbo Jet
    if (mJetting) {
        if (mThrustDirection == ThrustForward)
            force += yv * mDataBlock->jetForce * mCeilingFactor;
        else if (mThrustDirection == ThrustBackward)
            force -= yv * mDataBlock->jetForce * mCeilingFactor;
        else
            force += zv * mDataBlock->jetForce * mDataBlock->vertThrustMultiple * mCeilingFactor;
    }

    // Maneuvering jets
    force += yv * (mThrust.y * mDataBlock->maneuveringForce * mCeilingFactor);
    force += xv * (mThrust.x * mDataBlock->maneuveringForce * mCeilingFactor);

    // Steering
    Point2F steering;
    steering.x = mSteering.x / mDataBlock->maxSteeringAngle;
    steering.x *= mFabs(steering.x);
    steering.y = mSteering.y / mDataBlock->maxSteeringAngle;
    steering.y *= mFabs(steering.y);
    torque -= xv * steering.y * mDataBlock->steeringForce;
    torque -= zv * steering.x * mDataBlock->steeringForce;

    // Roll
    torque += yv * steering.x * mDataBlock->steeringRollForce;
    F32 ar = mDataBlock->autoAngularForce * mDot(xv, Point3F(0, 0, 1));
    ar -= mDataBlock->rollForce * mDot(xv, mRigid.linVelocity);
    torque += yv * ar;

    // Add in force from physical zones...
    force += mAppliedForce;

    // Container buoyancy & drag
    force -= Point3F(0, 0, 1) * (mBuoyancy * sFlyingVehicleGravity * mRigid.mass * mGravityMod);
    force -= mRigid.linVelocity * mDrag;

    //
    mRigid.force = force;
    mRigid.torque = torque;
}


//----------------------------------------------------------------------------

F32 FlyingVehicle::getHeight()
{
    Point3F sp, ep;
    RayInfo collision;
    F32 height = (createHeightOn) ? mDataBlock->createHoverHeight : mDataBlock->hoverHeight;
    F32 r = 10 + height;
    getTransform().getColumn(3, &sp);
    ep.x = sp.x;
    ep.y = sp.y;
    ep.z = sp.z - r;
    disableCollision();
    if (!mContainer->castRay(sp, ep, sClientCollisionMask, &collision) == true)
        collision.t = 1;
    enableCollision();
    return (r * collision.t - height) / 10;
}


//----------------------------------------------------------------------------
U32 FlyingVehicle::getCollisionMask()
{
    if (isServerObject())
        return sServerCollisionMask;
    else
        return sClientCollisionMask;
}

//----------------------------------------------------------------------------

void FlyingVehicle::updateEngineSound(F32 level)
{
    if (mEngineSound)
    {
        mEngineSound->setTransform(getTransform());
        mEngineSound->setVelocity(getVelocity());
        mEngineSound->setVolume(level);
    }
}

void FlyingVehicle::updateJet(F32 dt)
{
    // Thrust Animation threads
    //  Back
    if (mJetSeq[BackActivate] >= 0) {
        if (!mBackMaintainOn || mThrustDirection != ThrustForward) {
            if (mBackMaintainOn) {
                mShapeInstance->setPos(mJetThread[BackActivate], 1);
                mShapeInstance->destroyThread(mJetThread[BackMaintain]);
                mBackMaintainOn = false;
            }
            mShapeInstance->setTimeScale(mJetThread[BackActivate],
                (mThrustDirection == ThrustForward) ? 1 : -1);
            mShapeInstance->advanceTime(dt, mJetThread[BackActivate]);
        }
        if (mJetSeq[BackMaintain] >= 0 && !mBackMaintainOn &&
            mShapeInstance->getPos(mJetThread[BackActivate]) >= 1.0) {
            mShapeInstance->setPos(mJetThread[BackActivate], 0);
            mShapeInstance->setTimeScale(mJetThread[BackActivate], 0);
            mJetThread[BackMaintain] = mShapeInstance->addThread();
            mShapeInstance->setSequence(mJetThread[BackMaintain], mJetSeq[BackMaintain], 0);
            mShapeInstance->setTimeScale(mJetThread[BackMaintain], 1);
            mBackMaintainOn = true;
        }
        if (mBackMaintainOn)
            mShapeInstance->advanceTime(dt, mJetThread[BackMaintain]);
    }

    // Thrust Animation threads
    //   Bottom
    if (mJetSeq[BottomActivate] >= 0) {
        if (!mBottomMaintainOn || mThrustDirection != ThrustDown || !mJetting) {
            if (mBottomMaintainOn) {
                mShapeInstance->setPos(mJetThread[BottomActivate], 1);
                mShapeInstance->destroyThread(mJetThread[BottomMaintain]);
                mBottomMaintainOn = false;
            }
            mShapeInstance->setTimeScale(mJetThread[BottomActivate],
                (mThrustDirection == ThrustDown && mJetting) ? 1 : -1);
            mShapeInstance->advanceTime(dt, mJetThread[BottomActivate]);
        }
        if (mJetSeq[BottomMaintain] >= 0 && !mBottomMaintainOn &&
            mShapeInstance->getPos(mJetThread[BottomActivate]) >= 1.0) {
            mShapeInstance->setPos(mJetThread[BottomActivate], 0);
            mShapeInstance->setTimeScale(mJetThread[BottomActivate], 0);
            mJetThread[BottomMaintain] = mShapeInstance->addThread();
            mShapeInstance->setSequence(mJetThread[BottomMaintain], mJetSeq[BottomMaintain], 0);
            mShapeInstance->setTimeScale(mJetThread[BottomMaintain], 1);
            mBottomMaintainOn = true;
        }
        if (mBottomMaintainOn)
            mShapeInstance->advanceTime(dt, mJetThread[BottomMaintain]);
    }

    // Jet particles
    for (S32 j = 0; j < NumThrustDirections; j++) {
        JetActivation& jet = sJetActivation[j];
        updateEmitter(mJetting && j == mThrustDirection, dt, mDataBlock->jetEmitter[jet.emitter],
            jet.node, FlyingVehicleData::MaxDirectionJets);
    }

    // Trail jets
    Point3F yv;
    mObjToWorld.getColumn(1, &yv);
    F32 speed = mFabs(mDot(yv, mRigid.linVelocity));
    F32 trail = 0;
    if (speed > mDataBlock->minTrailSpeed) {
        trail = dt;
        if (speed < mDataBlock->maxSpeed)
            trail *= (speed - mDataBlock->minTrailSpeed) / mDataBlock->maxSpeed;
    }
    updateEmitter(trail, trail, mDataBlock->jetEmitter[FlyingVehicleData::TrailEmitter],
        FlyingVehicleData::TrailNode, FlyingVehicleData::MaxTrails);

    // Allocate/Deallocate voice on demand.
    if (!mJetSound)
        return;

    if (!mJetting)
        mJetSound->stop();
    else
    {
        if (!mJetSound->isPlaying())
            mJetSound->play();

        mJetSound->setTransform(getTransform());
        mJetSound->setVelocity(getVelocity());
    }
}

//----------------------------------------------------------------------------

void FlyingVehicle::updateEmitter(bool active, F32 dt, ParticleEmitterData* emitter, S32 idx, S32 count)
{
    if (!emitter)
        return;
    for (S32 j = idx; j < idx + count; j++)
        if (active) {
            if (mDataBlock->jetNode[j] != -1) {
                if (!bool(mJetEmitter[j])) {
                    mJetEmitter[j] = new ParticleEmitter;
                    mJetEmitter[j]->onNewDataBlock(emitter);
                    mJetEmitter[j]->registerObject();
                }
                MatrixF mat;
                Point3F pos, axis;
                mat.mul(getRenderTransform(),
                    mShapeInstance->mNodeTransforms[mDataBlock->jetNode[j]]);
                mat.getColumn(1, &axis);
                mat.getColumn(3, &pos);
                mJetEmitter[j]->emitParticles(pos, true, axis, getVelocity(), (U32)(dt * 1000));
            }
        }
        else {
            for (S32 j = idx; j < idx + count; j++)
                if (bool(mJetEmitter[j])) {
                    mJetEmitter[j]->deleteWhenEmpty();
                    mJetEmitter[j] = 0;
                }
        }
}


//----------------------------------------------------------------------------

void FlyingVehicle::writePacketData(GameConnection* connection, BitStream* stream)
{
    Parent::writePacketData(connection, stream);
}

void FlyingVehicle::readPacketData(GameConnection* connection, BitStream* stream)
{
    Parent::readPacketData(connection, stream);

    setPosition(mRigid.linPosition, mRigid.angPosition);
    mDelta.pos = mRigid.linPosition;
    mDelta.rot[1] = mRigid.angPosition;
}

U32 FlyingVehicle::packUpdate(NetConnection* con, U32 mask, BitStream* stream)
{
    U32 retMask = Parent::packUpdate(con, mask, stream);

    // The rest of the data is part of the control object packet update.
    // If we're controlled by this client, we don't need to send it.
    if (getControllingClient() == con && !(mask & InitialUpdateMask))
        return retMask;

    stream->writeFlag(createHeightOn);

    stream->writeInt(mThrustDirection, NumThrustBits);

    return retMask;
}

void FlyingVehicle::unpackUpdate(NetConnection* con, BitStream* stream)
{
    Parent::unpackUpdate(con, stream);

    if (getControllingClient() == con)
        return;

    createHeightOn = stream->readFlag();

    mThrustDirection = ThrustDirection(stream->readInt(NumThrustBits));
}

void FlyingVehicle::initPersistFields()
{
    Parent::initPersistFields();
}

ConsoleMethod(FlyingVehicle, useCreateHeight, void, 3, 3, "(bool enabled)"
    "Should the vehicle temporarily use the create height specified in the datablock? This can help avoid problems with spawning.")
{
    object->useCreateHeight(dAtob(argv[2]));
}

void FlyingVehicle::useCreateHeight(bool val)
{
    createHeightOn = val;
    setMaskBits(HoverHeight);
}
