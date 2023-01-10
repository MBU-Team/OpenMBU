//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "game/vehicles/hoverVehicle.h"

#include "core/bitStream.h"
#include "sceneGraph/sceneState.h"
#include "collision/clippedPolyList.h"
#include "collision/planeExtractor.h"
#include "game/moveManager.h"
#include "ts/tsShapeInstance.h"
#include "console/consoleTypes.h"
#include "terrain/terrData.h"
#include "sceneGraph/sceneGraph.h"
#include "sfx/sfxSystem.h"
#include "game/fx/particleEmitter.h"
#include "math/mathIO.h"
#include "materials/materialPropertyMap.h"
#ifdef TORQUE_TERRAIN
#include "terrain/waterBlock.h"
#endif

IMPLEMENT_CO_DATABLOCK_V1(HoverVehicleData);
IMPLEMENT_CO_NETOBJECT_V1(HoverVehicle);

namespace {

    const U32 sIntergrationsPerTick = 1;
    const F32 sHoverVehicleGravity = -20;

    const U32 sCollisionMoveMask = (TerrainObjectType | InteriorObjectType |
        PlayerObjectType | StaticTSObjectType |
        StaticShapeObjectType | VehicleObjectType |
        VehicleBlockerObjectType | AtlasObjectType);

    const U32 sServerCollisionMask = sCollisionMoveMask; // ItemObjectType
    const U32 sClientCollisionMask = sCollisionMoveMask;

    void nonFilter(SceneObject* object, void* key)
    {
        Container::CallbackInfo* info = reinterpret_cast<Container::CallbackInfo*>(key);
        object->buildPolyList(info->polyList, info->boundingBox, info->boundingSphere);
    }

} // namespace {}

const char* HoverVehicle::sJetSequence[HoverVehicle::JetAnimCount] =
{
   "activateBack",
   "maintainBack",
};

const char* HoverVehicleData::sJetNode[HoverVehicleData::MaxJetNodes] =
{
   "JetNozzle0",  // Thrust Forward
   "JetNozzle1",
   "JetNozzleX",  // Thrust Backward
   "JetNozzleX",
   "JetNozzle2",  // Thrust Downward
   "JetNozzle3",
};

// Convert thrust direction into nodes & emitters
HoverVehicle::JetActivation HoverVehicle::sJetActivation[NumThrustDirections] = {
   { HoverVehicleData::ForwardJetNode,  HoverVehicleData::ForwardJetEmitter },
   { HoverVehicleData::BackwardJetNode, HoverVehicleData::BackwardJetEmitter },
   { HoverVehicleData::DownwardJetNode, HoverVehicleData::DownwardJetEmitter },
};

//--------------------------------------------------------------------------
//--------------------------------------
//
HoverVehicleData::HoverVehicleData()
{
    dragForce = 0;
    vertFactor = 0.25;
    floatingThrustFactor = 0.15;

    mainThrustForce = 0;
    reverseThrustForce = 0;
    strafeThrustForce = 0;
    turboFactor = 1.0;

    stabLenMin = 0.5;
    stabLenMax = 2.0;
    stabSpringConstant = 30;
    stabDampingConstant = 10;

    gyroDrag = 10;
    normalForce = 30;
    restorativeForce = 10;
    steeringForce = 25;
    rollForce = 2.5;
    pitchForce = 2.5;

    dustTrailEmitter = NULL;
    dustTrailID = 0;
    dustTrailOffset.set(0.0, 0.0, 0.0);
    dustTrailFreqMod = 15.0;
    triggerTrailHeight = 2.5;

    floatingGravMag = 1;
    brakingForce = 0;
    brakingActivationSpeed = 0;

    for (S32 k = 0; k < MaxJetNodes; k++)
        jetNode[k] = -1;

    for (S32 j = 0; j < MaxJetEmitters; j++)
        jetEmitter[j] = 0;

    for (S32 i = 0; i < MaxSounds; i++)
        sound[i] = 0;
}

HoverVehicleData::~HoverVehicleData()
{

}


//--------------------------------------------------------------------------
void HoverVehicleData::initPersistFields()
{
    Parent::initPersistFields();

    addField("dragForce", TypeF32, Offset(dragForce, HoverVehicleData));
    addField("vertFactor", TypeF32, Offset(vertFactor, HoverVehicleData));
    addField("floatingThrustFactor", TypeF32, Offset(floatingThrustFactor, HoverVehicleData));
    addField("mainThrustForce", TypeF32, Offset(mainThrustForce, HoverVehicleData));
    addField("reverseThrustForce", TypeF32, Offset(reverseThrustForce, HoverVehicleData));
    addField("strafeThrustForce", TypeF32, Offset(strafeThrustForce, HoverVehicleData));
    addField("turboFactor", TypeF32, Offset(turboFactor, HoverVehicleData));

    addField("stabLenMin", TypeF32, Offset(stabLenMin, HoverVehicleData));
    addField("stabLenMax", TypeF32, Offset(stabLenMax, HoverVehicleData));
    addField("stabSpringConstant", TypeF32, Offset(stabSpringConstant, HoverVehicleData));
    addField("stabDampingConstant", TypeF32, Offset(stabDampingConstant, HoverVehicleData));

    addField("gyroDrag", TypeF32, Offset(gyroDrag, HoverVehicleData));
    addField("normalForce", TypeF32, Offset(normalForce, HoverVehicleData));
    addField("restorativeForce", TypeF32, Offset(restorativeForce, HoverVehicleData));
    addField("steeringForce", TypeF32, Offset(steeringForce, HoverVehicleData));
    addField("rollForce", TypeF32, Offset(rollForce, HoverVehicleData));
    addField("pitchForce", TypeF32, Offset(pitchForce, HoverVehicleData));

    addField("jetSound", TypeSFXProfilePtr, Offset(sound[JetSound], HoverVehicleData));
    addField("engineSound", TypeSFXProfilePtr, Offset(sound[EngineSound], HoverVehicleData));
    addField("floatSound", TypeSFXProfilePtr, Offset(sound[FloatSound], HoverVehicleData));

    addField("dustTrailEmitter", TypeParticleEmitterDataPtr, Offset(dustTrailEmitter, HoverVehicleData));
    addField("dustTrailOffset", TypePoint3F, Offset(dustTrailOffset, HoverVehicleData));
    addField("triggerTrailHeight", TypeF32, Offset(triggerTrailHeight, HoverVehicleData));
    addField("dustTrailFreqMod", TypeF32, Offset(dustTrailFreqMod, HoverVehicleData));

    addField("floatingGravMag", TypeF32, Offset(floatingGravMag, HoverVehicleData));
    addField("brakingForce", TypeF32, Offset(brakingForce, HoverVehicleData));
    addField("brakingActivationSpeed", TypeF32, Offset(brakingActivationSpeed, HoverVehicleData));

    addField("forwardJetEmitter", TypeParticleEmitterDataPtr, Offset(jetEmitter[ForwardJetEmitter], HoverVehicleData));
}


//--------------------------------------------------------------------------
bool HoverVehicleData::onAdd()
{
    if (!Parent::onAdd())
        return false;

    return true;
}


bool HoverVehicleData::preload(bool server, char errorBuffer[256])
{
    if (Parent::preload(server, errorBuffer) == false)
        return false;

    if (dragForce <= 0.01f) {
        Con::warnf("HoverVehicleData::preload: dragForce must be at least 0.01");
        dragForce = 0.01f;
    }
    if (vertFactor < 0.0f || vertFactor > 1.0f) {
        Con::warnf("HoverVehicleData::preload: vert factor must be [0, 1]");
        vertFactor = vertFactor < 0.0f ? 0.0f : 1.0f;
    }
    if (floatingThrustFactor < 0.0f || floatingThrustFactor > 1.0f) {
        Con::warnf("HoverVehicleData::preload: floatingThrustFactor must be [0, 1]");
        floatingThrustFactor = floatingThrustFactor < 0.0f ? 0.0f : 1.0f;
    }

    maxThrustSpeed = (mainThrustForce + strafeThrustForce) / dragForce;

    massCenter = Point3F(0, 0, 0);

    // Resolve objects transmitted from server
    if (!server) {
        for (S32 i = 0; i < MaxSounds; i++)
            if (sound[i])
                Sim::findObject(static_cast<SimObjectId>(reinterpret_cast<size_t>(sound[i])), sound[i]);
        for (S32 j = 0; j < MaxJetEmitters; j++)
            if (jetEmitter[j])
                Sim::findObject(static_cast<SimObjectId>(reinterpret_cast<size_t>(jetEmitter[j])), jetEmitter[j]);
    }

    if (!dustTrailEmitter && dustTrailID != 0)
    {
        if (!Sim::findObject(dustTrailID, dustTrailEmitter))
        {
            Con::errorf(ConsoleLogEntry::General, "HoverVehicleData::preload Invalid packet, bad datablockId(dustTrailEmitter): 0x%x", dustTrailID);
        }
    }
    // Resolve jet nodes
    for (S32 j = 0; j < MaxJetNodes; j++)
        jetNode[j] = shape->findNode(sJetNode[j]);

    return true;
}


//--------------------------------------------------------------------------
void HoverVehicleData::packData(BitStream* stream)
{
    Parent::packData(stream);

    stream->write(dragForce);
    stream->write(vertFactor);
    stream->write(floatingThrustFactor);
    stream->write(mainThrustForce);
    stream->write(reverseThrustForce);
    stream->write(strafeThrustForce);
    stream->write(turboFactor);
    stream->write(stabLenMin);
    stream->write(stabLenMax);
    stream->write(stabSpringConstant);
    stream->write(stabDampingConstant);
    stream->write(gyroDrag);
    stream->write(normalForce);
    stream->write(restorativeForce);
    stream->write(steeringForce);
    stream->write(rollForce);
    stream->write(pitchForce);
    mathWrite(*stream, dustTrailOffset);
    stream->write(triggerTrailHeight);
    stream->write(dustTrailFreqMod);

    for (S32 i = 0; i < MaxSounds; i++)
        if (stream->writeFlag(sound[i]))
            stream->writeRangedU32(packed ? static_cast<SimObjectId>(reinterpret_cast<size_t>(sound[i])) :
                sound[i]->getId(), DataBlockObjectIdFirst, DataBlockObjectIdLast);

    for (S32 j = 0; j < MaxJetEmitters; j++)
    {
        if (stream->writeFlag(jetEmitter[j]))
        {
            SimObjectId writtenId = packed ? static_cast<SimObjectId>(reinterpret_cast<size_t>(jetEmitter[j])) : jetEmitter[j]->getId();
            stream->writeRangedU32(writtenId, DataBlockObjectIdFirst, DataBlockObjectIdLast);
        }
    }

    if (stream->writeFlag(dustTrailEmitter))
    {
        stream->writeRangedU32(dustTrailEmitter->getId(), DataBlockObjectIdFirst, DataBlockObjectIdLast);
    }
    stream->write(floatingGravMag);
    stream->write(brakingForce);
    stream->write(brakingActivationSpeed);
}


void HoverVehicleData::unpackData(BitStream* stream)
{
    Parent::unpackData(stream);

    stream->read(&dragForce);
    stream->read(&vertFactor);
    stream->read(&floatingThrustFactor);
    stream->read(&mainThrustForce);
    stream->read(&reverseThrustForce);
    stream->read(&strafeThrustForce);
    stream->read(&turboFactor);
    stream->read(&stabLenMin);
    stream->read(&stabLenMax);
    stream->read(&stabSpringConstant);
    stream->read(&stabDampingConstant);
    stream->read(&gyroDrag);
    stream->read(&normalForce);
    stream->read(&restorativeForce);
    stream->read(&steeringForce);
    stream->read(&rollForce);
    stream->read(&pitchForce);
    mathRead(*stream, &dustTrailOffset);
    stream->read(&triggerTrailHeight);
    stream->read(&dustTrailFreqMod);

    for (S32 i = 0; i < MaxSounds; i++)
        sound[i] = stream->readFlag() ?
        (SFXProfile*)stream->readRangedU32(DataBlockObjectIdFirst,
            DataBlockObjectIdLast) : 0;

    for (S32 j = 0; j < MaxJetEmitters; j++) {
        jetEmitter[j] = NULL;
        if (stream->readFlag())
            jetEmitter[j] = (ParticleEmitterData*)stream->readRangedU32(DataBlockObjectIdFirst,
                DataBlockObjectIdLast);
    }

    if (stream->readFlag())
    {
        dustTrailID = (S32)stream->readRangedU32(DataBlockObjectIdFirst, DataBlockObjectIdLast);
    }
    stream->read(&floatingGravMag);
    stream->read(&brakingForce);
    stream->read(&brakingActivationSpeed);
}


//--------------------------------------------------------------------------
//--------------------------------------
//
HoverVehicle::HoverVehicle()
{
    // Todo: ScopeAlways?
    mNetFlags.set(Ghostable);

    mFloating = false;
    mForwardThrust = 0;
    mReverseThrust = 0;
    mLeftThrust = 0;
    mRightThrust = 0;

    mJetSound = NULL;
    mEngineSound = NULL;
    mFloatSound = NULL;

    mDustTrailEmitter = NULL;

    mBackMaintainOn = false;
    for (S32 i = 0; i < JetAnimCount; i++)
        mJetThread[i] = 0;
}

HoverVehicle::~HoverVehicle()
{
    //
}

//--------------------------------------------------------------------------
bool HoverVehicle::onAdd()
{
    if (!Parent::onAdd())
        return false;

    addToScene();


    if (!isServerObject())
    {
        if (mDataBlock->dustTrailEmitter)
        {
            mDustTrailEmitter = new ParticleEmitter;
            mDustTrailEmitter->onNewDataBlock(mDataBlock->dustTrailEmitter);
            if (!mDustTrailEmitter->registerObject())
            {
                Con::warnf(ConsoleLogEntry::General, "Could not register dust emitter for class: %s", mDataBlock->getName());
                delete mDustTrailEmitter;
                mDustTrailEmitter = NULL;
            }
        }
        // Jet Sequences
        for (S32 i = 0; i < JetAnimCount; i++) {
            TSShape const* shape = mShapeInstance->getShape();
            mJetSeq[i] = shape->findSequence(sJetSequence[i]);
            if (mJetSeq[i] != -1) {
                if (i == BackActivate) {
                    mJetThread[i] = mShapeInstance->addThread();
                    mShapeInstance->setSequence(mJetThread[i], mJetSeq[i], 0);
                    mShapeInstance->setTimeScale(mJetThread[i], 0);
                }
            }
            else
                mJetThread[i] = 0;
        }
    }


    if (isServerObject())
        scriptOnAdd();

    return true;
}


void HoverVehicle::onRemove()
{
    SFX_DELETE(mJetSound);
    SFX_DELETE(mEngineSound);
    SFX_DELETE(mFloatSound);

    scriptOnRemove();
    removeFromScene();
    Parent::onRemove();
}


bool HoverVehicle::onNewDataBlock(GameBaseData* dptr)
{
    mDataBlock = dynamic_cast<HoverVehicleData*>(dptr);
    if (!mDataBlock || !Parent::onNewDataBlock(dptr))
        return false;

    // Todo: Uncomment if this is a "leaf" class
    scriptOnNewDataBlock();

    return true;
}



//--------------------------------------------------------------------------
void HoverVehicle::advanceTime(F32 dt)
{
    Parent::advanceTime(dt);

    // Update jetsound...
    if (mJetSound)
    {
        if (mJetting)
        {
            if (!mJetSound->isPlaying())
                mJetSound->play();

            mJetSound->setTransform(getTransform());
        }
        else
            mJetSound->stop();
    }

    // Update engine sound...
    if (mEngineSound)
    {
        if (!mEngineSound->isPlaying())
            mEngineSound->play();

        mEngineSound->setTransform(getTransform());

        F32 denom = mDataBlock->mainThrustForce + mDataBlock->strafeThrustForce;
        F32 factor = getMin(mThrustLevel, denom) / denom;
        F32 vol = 0.25 + factor * 0.75;
        mEngineSound->setVolume(vol);
    }

    // Are we floating?  If so, start the floating sound...
    if (mFloatSound)
    {
        if (mFloating)
        {
            if (!mFloatSound->isPlaying())
                mFloatSound->play();

            mFloatSound->setTransform(getTransform());
        }
        else
            mFloatSound->stop();
    }

    updateJet(dt);
    updateDustTrail(dt);
}


//--------------------------------------------------------------------------

U32 HoverVehicle::packUpdate(NetConnection* con, U32 mask, BitStream* stream)
{
    U32 retMask = Parent::packUpdate(con, mask, stream);

    //
    stream->writeInt(mThrustDirection, NumThrustBits);

    return retMask;
}

void HoverVehicle::unpackUpdate(NetConnection* con, BitStream* stream)
{
    Parent::unpackUpdate(con, stream);

    mThrustDirection = ThrustDirection(stream->readInt(NumThrustBits));
    //
}


//--------------------------------------------------------------------------
void HoverVehicle::updateMove(const Move* move)
{
    Parent::updateMove(move);

    mForwardThrust = mThrottle > 0.0f ? mThrottle : 0.0f;
    mReverseThrust = mThrottle < 0.0f ? -mThrottle : 0.0f;
    mLeftThrust = move->x < 0.0f ? -move->x : 0.0f;
    mRightThrust = move->x > 0.0f ? move->x : 0.0f;

    mThrustDirection = (!move->y) ? ThrustDown : (move->y > 0) ? ThrustForward : ThrustBackward;
}

F32 HoverVehicle::getBaseStabilizerLength() const
{
    F32 base = mDataBlock->stabLenMin;
    F32 lengthDiff = mDataBlock->stabLenMax - mDataBlock->stabLenMin;
    F32 velLength = mRigid.linVelocity.len();
    F32 minVel = getMin(velLength, mDataBlock->maxThrustSpeed);
    F32 velDiff = mDataBlock->maxThrustSpeed - minVel;
    F32 velRatio = velDiff / mDataBlock->maxThrustSpeed;
    F32 inc = lengthDiff * (1.0 - velRatio);
    base += inc;

    return base;
}


struct StabPoint
{
    Point3F osPoint;           //
    Point3F wsPoint;           //
    F32     extension;
    Point3F wsExtension;       //
    Point3F wsVelocity;        //
};


void HoverVehicle::updateForces(F32 /*dt*/)
{
    Point3F gravForce(0, 0, sHoverVehicleGravity * mRigid.mass * mGravityMod);

    MatrixF currTransform;
    mRigid.getTransform(&currTransform);
    mRigid.atRest = false;

    mThrustLevel = (mForwardThrust * mDataBlock->mainThrustForce +
        mReverseThrust * mDataBlock->reverseThrustForce +
        mLeftThrust * mDataBlock->strafeThrustForce +
        mRightThrust * mDataBlock->strafeThrustForce);

    Point3F thrustForce = ((Point3F(0, 1, 0) * (mForwardThrust * mDataBlock->mainThrustForce)) +
        (Point3F(0, -1, 0) * (mReverseThrust * mDataBlock->reverseThrustForce)) +
        (Point3F(-1, 0, 0) * (mLeftThrust * mDataBlock->strafeThrustForce)) +
        (Point3F(1, 0, 0) * (mRightThrust * mDataBlock->strafeThrustForce)));
    currTransform.mulV(thrustForce);
    if (mJetting)
        thrustForce *= mDataBlock->turboFactor;

    Point3F torque(0, 0, 0);
    Point3F force(0, 0, 0);

    Point3F vel = mRigid.linVelocity;
    F32 baseStabLen = getBaseStabilizerLength();
    Point3F stabExtend(0, 0, -baseStabLen);
    currTransform.mulV(stabExtend);

    StabPoint stabPoints[2];
    stabPoints[0].osPoint = Point3F((mObjBox.min.x + mObjBox.max.x) * 0.5,
        mObjBox.max.y,
        (mObjBox.min.z + mObjBox.max.z) * 0.5);
    stabPoints[1].osPoint = Point3F((mObjBox.min.x + mObjBox.max.x) * 0.5,
        mObjBox.min.y,
        (mObjBox.min.z + mObjBox.max.z) * 0.5);
    U32 j, i;
    for (i = 0; i < 2; i++) {
        currTransform.mulP(stabPoints[i].osPoint, &stabPoints[i].wsPoint);
        stabPoints[i].wsExtension = stabExtend;
        stabPoints[i].extension = baseStabLen;
        stabPoints[i].wsVelocity = mRigid.linVelocity;
    }

    RayInfo rinfo;

    mFloating = true;
    bool reallyFloating = true;
    F32 compression[2] = { 0.0f, 0.0f };
    F32  normalMod[2] = { 0.0f, 0.0f };
    bool normalSet[2] = { false, false };
    Point3F normal[2];

    for (j = 0; j < 2; j++) {
        if (getContainer()->castRay(stabPoints[j].wsPoint, stabPoints[j].wsPoint + stabPoints[j].wsExtension * 2.0,
            AtlasObjectType | TerrainObjectType |
            InteriorObjectType | WaterObjectType, &rinfo))
        {
            reallyFloating = false;

            if (rinfo.t <= 0.5) {
                // Ok, stab is in contact with the ground, let's calc the forces...
                compression[j] = (1.0 - (rinfo.t * 2.0)) * baseStabLen;
            }
            normalSet[j] = true;
            normalMod[j] = rinfo.t < 0.5 ? 1.0 : (1.0 - ((rinfo.t - 0.5) * 2.0));

            normal[j] = rinfo.normal;
        }

#ifdef TORQUE_TERRAIN
        // Check the waterblock directly
        SimpleQueryList sql;
        mSceneManager->getWaterObjectList(sql);
        for (U32 i = 0; i < sql.mList.size(); i++)
        {
            WaterBlock* pBlock = static_cast<WaterBlock*>(sql.mList[i]);
            if (pBlock->isUnderwater(stabPoints[j].wsPoint))
            {
                compression[j] = baseStabLen;
                break;
            }
        }
#endif
    }

    for (j = 0; j < 2; j++) {
        if (compression[j] != 0.0) {
            mFloating = false;

            // Spring force and damping
            Point3F springForce = -stabPoints[j].wsExtension;
            springForce.normalize();
            springForce *= compression[j] * mDataBlock->stabSpringConstant;

            Point3F springDamping = -stabPoints[j].wsExtension;
            springDamping.normalize();
            springDamping *= -getMin(mDot(springDamping, stabPoints[j].wsVelocity), 0.7f) * mDataBlock->stabDampingConstant;

            force += springForce + springDamping;
        }
    }

    // Gravity
    if (reallyFloating == false)
        force += gravForce;
    else
        force += gravForce * mDataBlock->floatingGravMag;

    // Braking
    F32 vellen = mRigid.linVelocity.len();
    if (mThrottle == 0.0f &&
        mLeftThrust == 0.0f &&
        mRightThrust == 0.0f &&
        vellen != 0.0f &&
        vellen < mDataBlock->brakingActivationSpeed)
    {
        Point3F dir = mRigid.linVelocity;
        dir.normalize();
        dir.neg();
        force += dir * mDataBlock->brakingForce;
    }

    // Gyro Drag
    torque = -mRigid.angMomentum * mDataBlock->gyroDrag;

    // Move to proper normal
    Point3F sn, r;
    currTransform.getColumn(2, &sn);
    if (normalSet[0] || normalSet[1]) {
        if (normalSet[0] && normalSet[1]) {
            F32 dot = mDot(normal[0], normal[1]);
            if (dot > 0.999) {
                // Just pick the first normal.  They're too close to call
                if ((sn - normal[0]).lenSquared() > 0.00001) {
                    mCross(sn, normal[0], &r);
                    torque += r * mDataBlock->normalForce * normalMod[0];
                }
            }
            else {
                Point3F rotAxis;
                mCross(normal[0], normal[1], &rotAxis);
                rotAxis.normalize();

                F32 angle = mAcos(dot) * (normalMod[0] / (normalMod[0] + normalMod[1]));
                AngAxisF aa(rotAxis, angle);
                QuatF q(aa);
                MatrixF tempMat(true);
                q.setMatrix(&tempMat);
                Point3F newNormal;
                tempMat.mulV(normal[1], &newNormal);

                if ((sn - newNormal).lenSquared() > 0.00001) {
                    mCross(sn, newNormal, &r);
                    torque += r * (mDataBlock->normalForce * ((normalMod[0] + normalMod[1]) * 0.5));
                }
            }
        }
        else {
            Point3F useNormal;
            F32     useMod;
            if (normalSet[0]) {
                useNormal = normal[0];
                useMod = normalMod[0];
            }
            else {
                useNormal = normal[1];
                useMod = normalMod[1];
            }

            if ((sn - useNormal).lenSquared() > 0.00001) {
                mCross(sn, useNormal, &r);
                torque += r * mDataBlock->normalForce * useMod;
            }
        }
    }
    else {
        if ((sn - Point3F(0, 0, 1)).lenSquared() > 0.00001) {
            mCross(sn, Point3F(0, 0, 1), &r);
            torque += r * mDataBlock->restorativeForce;
        }
    }

    Point3F sn2;
    currTransform.getColumn(0, &sn);
    currTransform.getColumn(1, &sn2);
    mCross(sn, sn2, &r);
    r.normalize();
    torque -= r * (mSteering.x * mDataBlock->steeringForce);

    currTransform.getColumn(0, &sn);
    currTransform.getColumn(2, &sn2);
    mCross(sn, sn2, &r);
    r.normalize();
    torque -= r * (mSteering.x * mDataBlock->rollForce);

    currTransform.getColumn(1, &sn);
    currTransform.getColumn(2, &sn2);
    mCross(sn, sn2, &r);
    r.normalize();
    torque -= r * (mSteering.y * mDataBlock->pitchForce);

    // Apply drag
    Point3F vDrag = mRigid.linVelocity;
    if (!mFloating) {
        vDrag.convolve(Point3F(1, 1, mDataBlock->vertFactor));
    }
    else {
        vDrag.convolve(Point3F(0.25, 0.25, mDataBlock->vertFactor));
    }
    force -= vDrag * mDataBlock->dragForce;

    force += mFloating ? thrustForce * mDataBlock->floatingThrustFactor : thrustForce;

    // Add in physical zone force
    force += mAppliedForce;

    // Container buoyancy & drag
    force += Point3F(0, 0, -mBuoyancy * sHoverVehicleGravity * mRigid.mass * mGravityMod);
    force -= mRigid.linVelocity * mDrag;
    torque -= mRigid.angMomentum * mDrag;

    mRigid.force = force;
    mRigid.torque = torque;
}


//--------------------------------------------------------------------------
U32 HoverVehicle::getCollisionMask()
{
    if (isServerObject())
        return sServerCollisionMask;
    else
        return sClientCollisionMask;
}

void HoverVehicle::updateDustTrail(F32 dt)
{
    if (!mDustTrailEmitter) return;

    // check if close to ground
    Point3F startPos = getPosition();
    Point3F endPos = startPos + Point3F(0.0, 0.0, -mDataBlock->triggerTrailHeight);

    RayInfo rayInfo;
    if (!getContainer()->castRay(startPos, endPos, TerrainObjectType, &rayInfo))
    {
        return;
    }

    VectorF vel = getVelocity();

    TerrainBlock* tBlock = static_cast<TerrainBlock*>(rayInfo.object);
    S32 mapIndex = tBlock->mMPMIndex[0];
    MaterialPropertyMap* pMatMap = MaterialPropertyMap::get();
    const MaterialPropertyMap::MapEntry* pEntry = pMatMap->getMapEntryFromIndex(mapIndex);

    // emit dust if moving
    if (vel.len() > 2.0 && pEntry)
    {
        VectorF axis = vel;
        axis.normalize();

        S32 x;
        ColorF colorList[ParticleData::PDC_NUM_KEYS];

        for (x = 0; x < 2; ++x)
            colorList[x].set(pEntry->puffColor[x].red, pEntry->puffColor[x].green, pEntry->puffColor[x].blue, pEntry->puffColor[x].alpha);
        for (x = 2; x < ParticleData::PDC_NUM_KEYS; ++x)
            colorList[x].set(1.0, 1.0, 1.0, 0.0);

        mDustTrailEmitter->setColors(colorList);

        Point3F contactPoint = rayInfo.point + mDataBlock->dustTrailOffset;
        mDustTrailEmitter->emitParticles(contactPoint, true, axis, vel, (U32)(dt * 1000 * (vel.len() / mDataBlock->dustTrailFreqMod)));
    }

}

void HoverVehicle::updateJet(F32 dt)
{
    if (mJetThread[BackActivate] == NULL)
        return;

    F32 pos = mShapeInstance->getPos(mJetThread[BackActivate]);
    F32 scale = mShapeInstance->getTimeScale(mJetThread[BackActivate]);

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
    }

    if (mJetSeq[BackMaintain] >= 0 && !mBackMaintainOn &&
        mShapeInstance->getPos(mJetThread[BackActivate]) >= 1.0)
    {
        mShapeInstance->setPos(mJetThread[BackActivate], 0);
        mShapeInstance->setTimeScale(mJetThread[BackActivate], 0);
        mJetThread[BackMaintain] = mShapeInstance->addThread();
        mShapeInstance->setSequence(mJetThread[BackMaintain], mJetSeq[BackMaintain], 0);
        mShapeInstance->setTimeScale(mJetThread[BackMaintain], 1);
        mBackMaintainOn = true;
    }

    if (mBackMaintainOn)
        mShapeInstance->advanceTime(dt, mJetThread[BackMaintain]);

    // Jet particles
    for (S32 j = 0; j < NumThrustDirections; j++) {
        JetActivation& jet = sJetActivation[j];
        updateEmitter(mJetting && j == mThrustDirection, dt, mDataBlock->jetEmitter[jet.emitter],
            jet.node, HoverVehicleData::MaxDirectionJets);
    }
}

void HoverVehicle::updateEmitter(bool active, F32 dt, ParticleEmitterData* emitter, S32 idx, S32 count)
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
