//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "game/vehicles/wheeledVehicle.h"

#include "platform/platform.h"
#include "game/game.h"
#include "math/mMath.h"
#include "math/mathIO.h"
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
#include "sceneGraph/sceneGraph.h"
#include "sim/decalManager.h"
#include "materials/materialPropertyMap.h"
#include "terrain/terrData.h"

//----------------------------------------------------------------------------

// Collision masks are used to determin what type of objects the
// wheeled vehicle will collide with.
static U32 sClientCollisionMask =
TerrainObjectType | InteriorObjectType |
PlayerObjectType | StaticShapeObjectType |
VehicleObjectType | VehicleBlockerObjectType |
StaticTSObjectType | AtlasObjectType;

// Gravity constant
static F32 sWheeledVehicleGravity = -20;

// Misc. sound constants
static F32 sMinSquealVolume = 0.05;
static F32 sIdleEngineVolume = 0.2;


//----------------------------------------------------------------------------
// Vehicle Tire Data Block
//----------------------------------------------------------------------------

IMPLEMENT_CO_DATABLOCK_V1(WheeledVehicleTire);

WheeledVehicleTire::WheeledVehicleTire()
{
    shape = 0;
    shapeName = "";
    staticFriction = 1;
    kineticFriction = 0.5;
    restitution = 1;
    radius = 0.6;
    lateralForce = 10;
    lateralDamping = 1;
    lateralRelaxation = 1;
    longitudinalForce = 10;
    longitudinalDamping = 1;
    longitudinalRelaxation = 1;
}

bool WheeledVehicleTire::preload(bool server, char errorBuffer[256])
{
    // Load up the tire shape.  ShapeBase has an option to force a
    // CRC check, this is left out here, but could be easily added.
    if (shapeName && shapeName[0])
    {

        // Load up the shape resource
        shape = ResourceManager->load(shapeName);
        if (!bool(shape))
        {
            dSprintf(errorBuffer, 256, "WheeledVehicleTire: Couldn't load shape \"%s\"", shapeName);
            return false;
        }

        // Determinw wheel radius from the shape's bounding box.
        // The tire should be built with it's hub axis along the
        // object's Y axis.
        radius = shape->bounds.len_z() / 2;
    }

    return true;
}

void WheeledVehicleTire::initPersistFields()
{
    Parent::initPersistFields();

    addField("shapeFile", TypeFilename, Offset(shapeName, WheeledVehicleTire));
    addField("mass", TypeF32, Offset(mass, WheeledVehicleTire));
    addField("radius", TypeF32, Offset(radius, WheeledVehicleTire));
    addField("staticFriction", TypeF32, Offset(staticFriction, WheeledVehicleTire));
    addField("kineticFriction", TypeF32, Offset(kineticFriction, WheeledVehicleTire));
    addField("restitution", TypeF32, Offset(restitution, WheeledVehicleTire));
    addField("lateralForce", TypeF32, Offset(lateralForce, WheeledVehicleTire));
    addField("lateralDamping", TypeF32, Offset(lateralDamping, WheeledVehicleTire));
    addField("lateralRelaxation", TypeF32, Offset(lateralRelaxation, WheeledVehicleTire));
    addField("longitudinalForce", TypeF32, Offset(longitudinalForce, WheeledVehicleTire));
    addField("longitudinalDamping", TypeF32, Offset(longitudinalDamping, WheeledVehicleTire));
    addField("logitudinalRelaxation", TypeF32, Offset(longitudinalRelaxation, WheeledVehicleTire));
}

void WheeledVehicleTire::packData(BitStream* stream)
{
    Parent::packData(stream);

    stream->writeString(shapeName);
    stream->write(mass);
    stream->write(staticFriction);
    stream->write(kineticFriction);
    stream->write(restitution);
    stream->write(radius);
    stream->write(lateralForce);
    stream->write(lateralDamping);
    stream->write(lateralRelaxation);
    stream->write(longitudinalForce);
    stream->write(longitudinalDamping);
    stream->write(longitudinalRelaxation);
}

void WheeledVehicleTire::unpackData(BitStream* stream)
{
    Parent::unpackData(stream);

    shapeName = stream->readSTString();
    stream->read(&mass);
    stream->read(&staticFriction);
    stream->read(&kineticFriction);
    stream->read(&restitution);
    stream->read(&radius);
    stream->read(&lateralForce);
    stream->read(&lateralDamping);
    stream->read(&lateralRelaxation);
    stream->read(&longitudinalForce);
    stream->read(&longitudinalDamping);
    stream->read(&longitudinalRelaxation);
}


//----------------------------------------------------------------------------
// Vehicle Spring Data Block
//----------------------------------------------------------------------------

IMPLEMENT_CO_DATABLOCK_V1(WheeledVehicleSpring);

WheeledVehicleSpring::WheeledVehicleSpring()
{
    length = 1;
    force = 10;
    damping = 1;
    antiSway = 1;
}

void WheeledVehicleSpring::initPersistFields()
{
    Parent::initPersistFields();

    addField("length", TypeF32, Offset(length, WheeledVehicleSpring));
    addField("force", TypeF32, Offset(force, WheeledVehicleSpring));
    addField("damping", TypeF32, Offset(damping, WheeledVehicleSpring));
    addField("antiSwayForce", TypeF32, Offset(antiSway, WheeledVehicleSpring));
}

void WheeledVehicleSpring::packData(BitStream* stream)
{
    Parent::packData(stream);

    stream->write(length);
    stream->write(force);
    stream->write(damping);
    stream->write(antiSway);
}

void WheeledVehicleSpring::unpackData(BitStream* stream)
{
    Parent::unpackData(stream);

    stream->read(&length);
    stream->read(&force);
    stream->read(&damping);
    stream->read(&antiSway);
}


//----------------------------------------------------------------------------
// Wheeled Vehicle Data Block
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------

IMPLEMENT_CO_DATABLOCK_V1(WheeledVehicleData);

WheeledVehicleData::WheeledVehicleData()
{
    tireEmitter = 0;
    maxWheelSpeed = 40;
    engineTorque = 1;
    engineBrake = 1;
    brakeTorque = 1;
    brakeLightSequence = -1;

    for (S32 i = 0; i < MaxSounds; i++)
        sound[i] = 0;
}


//----------------------------------------------------------------------------
/** Load the vehicle shape
   Loads and extracts information from the vehicle shape.

   Wheel Sequences
      spring#        Wheel spring motion: time 0 = wheel fully extended,
                     the hub must be displaced, but not directly animated
                     as it will be rotated in code.
   Other Sequences
      steering       Wheel steering: time 0 = full right, 0.5 = center
      breakLight     Break light, time 0 = off, 1 = breaking

   Wheel Nodes
      hub#           Wheel hub

   The steering and animation sequences are optional.
*/
bool WheeledVehicleData::preload(bool server, char errorBuffer[256])
{
    if (!Parent::preload(server, errorBuffer))
        return false;

    // A temporary shape instance is created so that we can
    // animate the shape and extract wheel information.
    TSShapeInstance* si = new TSShapeInstance(shape, false);

    // Resolve objects transmitted from server
    if (!server) {
        for (S32 i = 0; i < MaxSounds; i++)
            if (sound[i])
                Sim::findObject(static_cast<SimObjectId>(reinterpret_cast<size_t>(sound[i])), sound[i]);

        if (tireEmitter)
            Sim::findObject(static_cast<SimObjectId>(reinterpret_cast<size_t>(tireEmitter)), tireEmitter);
    }

    // Extract wheel information from the shape
    TSThread* thread = si->addThread();
    Wheel* wp = wheel;
    char buff[10];
    for (S32 i = 0; i < MaxWheels; i++) {

        // The wheel must have a hub node to operate at all.
        dSprintf(buff, sizeof(buff), "hub%d", i);
        wp->springNode = shape->findNode(buff);
        if (wp->springNode != -1) {

            // Check for spring animation.. If there is none we just grab
            // the current position of the hub. Otherwise we'll animate
            // and get the position at time 0.
            dSprintf(buff, sizeof(buff), "spring%d", i);
            wp->springSequence = shape->findSequence(buff);
            if (wp->springSequence == -1)
                si->mNodeTransforms[wp->springNode].getColumn(3, &wp->pos);
            else {
                si->setSequence(thread, wp->springSequence, 0);
                si->animate();
                si->mNodeTransforms[wp->springNode].getColumn(3, &wp->pos);

                // Determin the length of the animation so we can scale it
                // according the actual wheel position.
                Point3F downPos;
                si->setSequence(thread, wp->springSequence, 1);
                si->animate();
                si->mNodeTransforms[wp->springNode].getColumn(3, &downPos);
                wp->springLength = wp->pos.z - downPos.z;
                if (!wp->springLength)
                    wp->springSequence = -1;
            }

            // Match wheels that are mirrored along the Y axis.
            mirrorWheel(wp);
            wp++;
        }
    }
    wheelCount = wp - wheel;

    // Check for steering. Should think about normalizing the
    // steering animation the way the suspension is, but I don't
    // think it's as critical.
    steeringSequence = shape->findSequence("steering");

    // Brakes
    brakeLightSequence = shape->findSequence("brakelight");

    // Extract collision planes from shape collision detail level
    if (collisionDetails[0] != -1) {
        MatrixF imat(1);
        SphereF sphere;
        sphere.center = shape->center;
        sphere.radius = shape->radius;
        PlaneExtractorPolyList polyList;
        polyList.mPlaneList = &rigidBody.mPlaneList;
        polyList.setTransform(&imat, Point3F(1, 1, 1));
        si->buildPolyList(&polyList, collisionDetails[0]);
    }

    delete si;
    return true;
}


//----------------------------------------------------------------------------
/** Find a matching lateral wheel
   Looks for a matching wheeling mirrored along the Y axis, within some
   tolerance (current 0.5m), if one is found, the two wheels are lined up.
*/
bool WheeledVehicleData::mirrorWheel(Wheel* we)
{
    we->opposite = -1;
    for (Wheel* wp = wheel; wp != we; wp++)
        if (mFabs(wp->pos.y - we->pos.y) < 0.5)
        {
            we->pos.x = -wp->pos.x;
            we->pos.y = wp->pos.y;
            we->pos.z = wp->pos.z;
            we->opposite = wp - wheel;
            wp->opposite = we - wheel;
            return true;
        }
    return false;
}


//----------------------------------------------------------------------------

void WheeledVehicleData::initPersistFields()
{
    Parent::initPersistFields();

    addField("jetSound", TypeSFXProfilePtr, Offset(sound[JetSound], WheeledVehicleData));
    addField("engineSound", TypeSFXProfilePtr, Offset(sound[EngineSound], WheeledVehicleData));
    addField("squealSound", TypeSFXProfilePtr, Offset(sound[SquealSound], WheeledVehicleData));
    addField("WheelImpactSound", TypeSFXProfilePtr, Offset(sound[WheelImpactSound], WheeledVehicleData));

    addField("tireEmitter", TypeParticleEmitterDataPtr, Offset(tireEmitter, WheeledVehicleData));
    addField("maxWheelSpeed", TypeF32, Offset(maxWheelSpeed, WheeledVehicleData));
    addField("engineTorque", TypeF32, Offset(engineTorque, WheeledVehicleData));
    addField("engineBrake", TypeF32, Offset(engineBrake, WheeledVehicleData));
    addField("brakeTorque", TypeF32, Offset(brakeTorque, WheeledVehicleData));
}


//----------------------------------------------------------------------------

void WheeledVehicleData::packData(BitStream* stream)
{
    Parent::packData(stream);

    if (stream->writeFlag(tireEmitter))
        stream->writeRangedU32(packed ? static_cast<SimObjectId>(reinterpret_cast<size_t>(tireEmitter)) :
            tireEmitter->getId(), DataBlockObjectIdFirst, DataBlockObjectIdLast);

    for (S32 i = 0; i < MaxSounds; i++)
        if (stream->writeFlag(sound[i]))
            stream->writeRangedU32(packed ? static_cast<SimObjectId>(reinterpret_cast<size_t>(sound[i])) :
                sound[i]->getId(), DataBlockObjectIdFirst, DataBlockObjectIdLast);

    stream->write(maxWheelSpeed);
    stream->write(engineTorque);
    stream->write(engineBrake);
    stream->write(brakeTorque);
}

void WheeledVehicleData::unpackData(BitStream* stream)
{
    Parent::unpackData(stream);

    tireEmitter = stream->readFlag() ?
        (ParticleEmitterData*)stream->readRangedU32(DataBlockObjectIdFirst,
            DataBlockObjectIdLast) : 0;

    for (S32 i = 0; i < MaxSounds; i++)
        sound[i] = stream->readFlag() ?
        (SFXProfile*)stream->readRangedU32(DataBlockObjectIdFirst,
            DataBlockObjectIdLast) : 0;

    stream->read(&maxWheelSpeed);
    stream->read(&engineTorque);
    stream->read(&engineBrake);
    stream->read(&brakeTorque);
}


//----------------------------------------------------------------------------
// Wheeled Vehicle Class
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------

IMPLEMENT_CO_NETOBJECT_V1(WheeledVehicle);

WheeledVehicle::WheeledVehicle()
{
    mDataBlock = 0;
    mBraking = false;
    mJetSound = 0;
    mEngineSound = 0;
    mSquealSound = 0;
    mTailLightThread = 0;
    mSteeringThread = 0;

    for (S32 i = 0; i < WheeledVehicleData::MaxWheels; i++) {
        mWheel[i].springThread = 0;
        mWheel[i].Dy = mWheel[i].Dx = 0;
        mWheel[i].tire = 0;
        mWheel[i].spring = 0;
        mWheel[i].shapeInstance = 0;
        mWheel[i].steering = 0;
        mWheel[i].powered = true;
        mWheel[i].slipping = false;
    }
}

WheeledVehicle::~WheeledVehicle()
{
}

void WheeledVehicle::initPersistFields()
{
    Parent::initPersistFields();
}


//----------------------------------------------------------------------------

bool WheeledVehicle::onAdd()
{
    if (!Parent::onAdd())
        return false;

    addToScene();
    if (isServerObject())
        scriptOnAdd();
    return true;
}

void WheeledVehicle::onRemove()
{
    // Delete the wheel resources
    if (mDataBlock != NULL) {
        Wheel* wend = &mWheel[mDataBlock->wheelCount];
        for (Wheel* wheel = mWheel; wheel < wend; wheel++) {
            if (!wheel->emitter.isNull())
                wheel->emitter->deleteWhenEmpty();
            delete wheel->shapeInstance;
        }
    }

    // Stop the sounds
    SFX_DELETE(mJetSound);
    SFX_DELETE(mEngineSound);
    SFX_DELETE(mSquealSound);

    //
    scriptOnRemove();
    removeFromScene();
    Parent::onRemove();
}


//----------------------------------------------------------------------------

bool WheeledVehicle::onNewDataBlock(GameBaseData* dptr)
{
    // Delete any existing wheel resources if we're switching
    // datablocks.
    if (mDataBlock)
    {
        Wheel* wend = &mWheel[mDataBlock->wheelCount];
        for (Wheel* wheel = mWheel; wheel < wend; wheel++)
        {
            if (!wheel->emitter.isNull())
            {
                wheel->emitter->deleteWhenEmpty();
                wheel->emitter = 0;
            }
            delete wheel->shapeInstance;
            wheel->shapeInstance = 0;
        }
    }

    // Load up the new datablock
    mDataBlock = dynamic_cast<WheeledVehicleData*>(dptr);
    if (!mDataBlock || !Parent::onNewDataBlock(dptr))
        return false;

    F32 frontStatic = 0;
    F32 backStatic = 0;
    F32 fCount = 0;
    F32 bCount = 0;

    // Set inertial tensor, default for the vehicle is sphere
    if (mDataBlock->massBox.x > 0 && mDataBlock->massBox.y > 0 && mDataBlock->massBox.z > 0)
        mRigid.setObjectInertia(mDataBlock->massBox);
    else
        mRigid.setObjectInertia(mObjBox.max - mObjBox.min);

    // Initialize the wheels...
    for (S32 i = 0; i < mDataBlock->wheelCount; i++)
    {
        Wheel* wheel = &mWheel[i];
        wheel->data = &mDataBlock->wheel[i];
        wheel->tire = 0;
        wheel->spring = 0;

        wheel->surface.contact = false;
        wheel->surface.object = NULL;
        wheel->avel = 0;
        wheel->apos = 0;
        wheel->extension = 1;
        wheel->slip = 0;

        wheel->springThread = 0;
        wheel->emitter = 0;

        // Steering on the front tires by default
        if (wheel->data->pos.y > 0)
            wheel->steering = 1;

        // Build wheel animation threads
        if (wheel->data->springSequence != -1) {
            wheel->springThread = mShapeInstance->addThread();
            mShapeInstance->setSequence(wheel->springThread, wheel->data->springSequence, 0);
        }

        // Each wheel get's it's own particle emitter
        if (mDataBlock->tireEmitter) {
            wheel->emitter = new ParticleEmitter;
            wheel->emitter->onNewDataBlock(mDataBlock->tireEmitter);
            wheel->emitter->registerObject();
        }
    }

    // Steering sequence
    if (mDataBlock->steeringSequence != -1) {
        mSteeringThread = mShapeInstance->addThread();
        mShapeInstance->setSequence(mSteeringThread, mDataBlock->steeringSequence, 0);
    }
    else
        mSteeringThread = 0;

    // Break light sequence
    if (mDataBlock->brakeLightSequence != -1) {
        mTailLightThread = mShapeInstance->addThread();
        mShapeInstance->setSequence(mTailLightThread, mDataBlock->brakeLightSequence, 0);
    }
    else
        mTailLightThread = 0;

    if (isGhost())
    {
        // Create the sounds ahead of time.  This reduces runtime
        // costs and makes the system easier to understand.

        SFX_DELETE(mEngineSound);
        SFX_DELETE(mSquealSound);
        SFX_DELETE(mJetSound);

        if (mDataBlock->sound[WheeledVehicleData::EngineSound])
            mEngineSound = SFX->createSource(mDataBlock->sound[WheeledVehicleData::EngineSound], &getTransform());

        if (mDataBlock->sound[WheeledVehicleData::SquealSound])
            mSquealSound = SFX->createSource(mDataBlock->sound[WheeledVehicleData::SquealSound], &getTransform());

        if (mDataBlock->sound[WheeledVehicleData::JetSound])
            mJetSound = SFX->createSource(mDataBlock->sound[WheeledVehicleData::JetSound], &getTransform());
    }

    scriptOnNewDataBlock();
    return true;
}


//----------------------------------------------------------------------------

S32 WheeledVehicle::getWheelCount()
{
    // Return # of hubs defined on the car body
    return mDataBlock ? mDataBlock->wheelCount : 0;
}

void WheeledVehicle::setWheelSteering(S32 wheel, F32 steering)
{
    AssertFatal(wheel >= 0 && wheel < WheeledVehicleData::MaxWheels, "Wheel index out of bounds");
    mWheel[wheel].steering = mClampF(steering, -1, 1);
    setMaskBits(WheelMask);
}

void WheeledVehicle::setWheelPowered(S32 wheel, bool powered)
{
    AssertFatal(wheel >= 0 && wheel < WheeledVehicleData::MaxWheels, "Wheel index out of bounds");
    mWheel[wheel].powered = powered;
    setMaskBits(WheelMask);
}

void WheeledVehicle::setWheelTire(S32 wheel, WheeledVehicleTire* tire)
{
    AssertFatal(wheel >= 0 && wheel < WheeledVehicleData::MaxWheels, "Wheel index out of bounds");
    mWheel[wheel].tire = tire;
    setMaskBits(WheelMask);
}

void WheeledVehicle::setWheelSpring(S32 wheel, WheeledVehicleSpring* spring)
{
    AssertFatal(wheel >= 0 && wheel < WheeledVehicleData::MaxWheels, "Wheel index out of bounds");
    mWheel[wheel].spring = spring;
    setMaskBits(WheelMask);
}


//----------------------------------------------------------------------------

void WheeledVehicle::processTick(const Move* move)
{
    Parent::processTick(move);
}

void WheeledVehicle::updateMove(const Move* move)
{
    Parent::updateMove(move);

    // Break on trigger
    mBraking = move->trigger[2];

    // Set the tail brake light thread direction based on the brake state.
    if (mTailLightThread)
        mShapeInstance->setTimeScale(mTailLightThread, mBraking ? 1 : -1);
}


//----------------------------------------------------------------------------

void WheeledVehicle::advanceTime(F32 dt)
{
    Parent::advanceTime(dt);

    // Stick the wheels to the ground.  This is purely so they look
    // good while the vehicle is being interpolated.
    extendWheels();

    // Update wheel angular position and slip, this is a client visual
    // feature only, it has no affect on the physics.
    F32 slipTotal = 0;
    F32 torqueTotal = 0;

    Wheel* wend = &mWheel[mDataBlock->wheelCount];
    for (Wheel* wheel = mWheel; wheel < wend; wheel++)
        if (wheel->tire && wheel->spring) {
            // Update angular position
            wheel->apos += (wheel->avel * dt) / M_2PI;
            wheel->apos -= mFloor(wheel->apos);
            if (wheel->apos < 0)
                wheel->apos = 1 - wheel->apos;

            // Keep track of largest slip
            slipTotal += wheel->slip;
            torqueTotal += wheel->torqueScale;
        }

    // Update the sounds based on wheel slip and torque output
    updateSquealSound(slipTotal / mDataBlock->wheelCount);
    updateEngineSound(sIdleEngineVolume + (1 - sIdleEngineVolume) *
        (1 - (torqueTotal / mDataBlock->wheelCount)));
    updateJetSound();

    updateWheelThreads();
    updateWheelParticles(dt);

    // Update the steering animation: sequence time 0 is full right,
    // and time 0.5 is straight ahead.
    if (mSteeringThread) {
        F32 t = (mSteering.x * mFabs(mSteering.x)) / mDataBlock->maxSteeringAngle;
        mShapeInstance->setPos(mSteeringThread, 0.5 - t * 0.5);
    }

    // Animate the tail light. The direction of the thread is
    // set based on vehicle braking.
    if (mTailLightThread)
        mShapeInstance->advanceTime(dt, mTailLightThread);
}


//----------------------------------------------------------------------------
/** Update the rigid body forces on the vehicle
   This method calculates the forces acting on the body, including gravity,
   suspension & tire forces.
*/
void WheeledVehicle::updateForces(F32 dt)
{
    extendWheels();

    F32 oneOverSprungMass = 1 / (mMass * 0.8);
    F32 aMomentum = mMass / mDataBlock->wheelCount;

    // Get the current matrix and extact vectors
    MatrixF currMatrix;
    mRigid.getTransform(&currMatrix);

    Point3F bx, by, bz;
    currMatrix.getColumn(0, &bx);
    currMatrix.getColumn(1, &by);
    currMatrix.getColumn(2, &bz);

    // Steering angles from current steering wheel position
    F32 quadraticSteering = -(mSteering.x * mFabs(mSteering.x));
    F32 cosSteering, sinSteering;
    mSinCos(quadraticSteering, sinSteering, cosSteering);

    // Calculate Engine and brake torque values used later by in
    // wheel calculations.
    F32 engineTorque, brakeVel;
    if (mBraking)
    {
        brakeVel = (mDataBlock->brakeTorque / aMomentum) * dt;
        engineTorque = 0;
    }
    else
    {
        if (mThrottle)
        {
            engineTorque = mDataBlock->engineTorque * mThrottle;
            brakeVel = 0;
            // Double the engineTorque to help out the jets
            if (mThrottle > 0 && mJetting)
                engineTorque *= 2;
        }
        else
        {
            // Engine break.
            brakeVel = (mDataBlock->engineBrake / aMomentum) * dt;
            engineTorque = 0;
        }
    }

    // Integrate forces, we'll do this ourselves here instead of
    // relying on the rigid class which does it during movement.
    Wheel* wend = &mWheel[mDataBlock->wheelCount];
    mRigid.force.set(0, 0, 0);
    mRigid.torque.set(0, 0, 0);

    // Calculate vertical load for friction.  Divide up the spring
    // forces across all the wheels that are in contact with
    // the ground.
    U32 contactCount = 0;
    F32 verticalLoad = 0;
    for (Wheel* wheel = mWheel; wheel < wend; wheel++)
    {
        if (wheel->tire && wheel->spring && wheel->surface.contact)
        {
            verticalLoad += wheel->spring->force * (1 - wheel->extension);
            contactCount++;
        }
    }
    if (contactCount)
        verticalLoad /= contactCount;

    // Sum up spring and wheel torque forces
    for (Wheel* wheel = mWheel; wheel < wend; wheel++)
    {
        if (!wheel->tire || !wheel->spring)
            continue;

        F32 Fy = 0;
        if (wheel->surface.contact)
        {

            // First, let's compute the wheel's position, and worldspace velocity
            Point3F pos, r, localVel;
            currMatrix.mulP(wheel->data->pos, &pos);
            mRigid.getOriginVector(pos, &r);
            mRigid.getVelocity(r, &localVel);

            // Spring force & damping
            F32 spring = wheel->spring->force * (1 - wheel->extension);
            F32 damping = wheel->spring->damping * -(mDot(bz, localVel) / wheel->spring->length);
            if (damping < 0)
                damping = 0;

            // Anti-sway force based on difference in suspension extension
            F32 antiSway = 0;
            if (wheel->data->opposite != -1) {
                Wheel* oppositeWheel = &mWheel[wheel->data->opposite];
                if (oppositeWheel->surface.contact)
                    antiSway = ((oppositeWheel->extension - wheel->extension) *
                        wheel->spring->antiSway);
                if (antiSway < 0)
                    antiSway = 0;
            }

            // Spring forces act straight up and are applied at the
            // spring's root position.
            Point3F t, forceVector = bz * (spring + damping + antiSway);
            mCross(r, forceVector, &t);
            mRigid.torque += t;
            mRigid.force += forceVector;

            // Tire direction vectors perpendicular to surface normal
            Point3F wheelXVec = bx * cosSteering;
            wheelXVec += by * sinSteering * wheel->steering;
            Point3F tireX, tireY;
            mCross(wheel->surface.normal, wheelXVec, &tireY);
            tireY.normalize();
            mCross(tireY, wheel->surface.normal, &tireX);
            tireX.normalize();

            // Velocity of tire at the surface contact
            Point3F wheelContact, wheelVelocity;
            mRigid.getOriginVector(wheel->surface.pos, &wheelContact);
            mRigid.getVelocity(wheelContact, &wheelVelocity);

            F32 xVelocity = mDot(tireX, wheelVelocity);
            F32 yVelocity = mDot(tireY, wheelVelocity);

            // Tires act as springs and generate lateral and longitudinal
            // forces to move the vehicle. These distortion/spring forces
            // are what convert wheel angular velocity into forces that
            // act on the rigid body.

            // Longitudinal tire deformation force
            F32 ddy = (wheel->avel * wheel->tire->radius - yVelocity) -
                wheel->tire->longitudinalRelaxation *
                mFabs(wheel->avel) * wheel->Dy;
            wheel->Dy += ddy * dt;
            Fy = (wheel->tire->longitudinalForce * wheel->Dy +
                wheel->tire->longitudinalDamping * ddy);

            // Lateral tire deformation force
            F32 ddx = xVelocity - wheel->tire->lateralRelaxation *
                mFabs(wheel->avel) * wheel->Dx;
            wheel->Dx += ddx * dt;
            F32 Fx = -(wheel->tire->lateralForce * wheel->Dx +
                wheel->tire->lateralDamping * ddx);

            // Vertical load on the tire
            verticalLoad = spring + damping + antiSway;
            if (verticalLoad < 0)
                verticalLoad = 0;

            // Adjust tire forces based on friction
            F32 surfaceFriction = 1;
            F32 mu = surfaceFriction * (wheel->slipping) ?
                wheel->tire->kineticFriction :
                wheel->tire->staticFriction;
            F32 Fn = verticalLoad * mu; Fn *= Fn;
            F32 Fw = Fx * Fx + Fy * Fy;
            if (Fw > Fn)
            {
                F32 K = mSqrt(Fn / Fw);
                Fy *= K;
                Fx *= K;
                wheel->Dy *= K;
                wheel->Dx *= K;
                wheel->slip = 1 - K;
                wheel->slipping = true;
            }
            else
            {
                wheel->slipping = false;
                wheel->slip = 0;
            }

            // Tire forces act through the tire direction vectors parallel
            // to the surface and are applied at the wheel hub.
            forceVector = (tireX * Fx) + (tireY * Fy);
            pos -= bz * (wheel->spring->length * wheel->extension);
            mRigid.getOriginVector(pos, &r);
            mCross(r, forceVector, &t);
            mRigid.torque += t;
            mRigid.force += forceVector;
        }
        else
        {
            // Wheel not in contact with the ground
            wheel->torqueScale = 0;
            wheel->slip = 0;

            // Relax the tire deformation
            wheel->Dy += (-wheel->tire->longitudinalRelaxation *
                mFabs(wheel->avel) * wheel->Dy) * dt;
            wheel->Dx += (-wheel->tire->lateralRelaxation *
                mFabs(wheel->avel) * wheel->Dx) * dt;
        }

        // Adjust the wheel's angular velocity based on engine torque
        // and tire deformation forces.
        if (wheel->powered)
        {
            F32 maxAvel = mDataBlock->maxWheelSpeed / wheel->tire->radius;
            wheel->torqueScale = (mFabs(wheel->avel) > maxAvel) ? 0 :
                1 - (mFabs(wheel->avel) / maxAvel);
        }
        else
            wheel->torqueScale = 0;
        wheel->avel += (((wheel->torqueScale * engineTorque) - Fy *
            wheel->tire->radius) / aMomentum) * dt;

        // Adjust the wheel's angular velocity based on break torque.
        // This is done after avel update to make sure we come to a
        // complete stop.
        if (brakeVel > mFabs(wheel->avel))
            wheel->avel = 0;
        else
            if (wheel->avel > 0)
                wheel->avel -= brakeVel;
            else
                wheel->avel += brakeVel;
    }

    // Jet Force
    if (mJetting)
        mRigid.force += by * mDataBlock->jetForce;

    // Container drag & buoyancy
    mRigid.force += Point3F(0, 0, -mBuoyancy * sWheeledVehicleGravity * mRigid.mass);
    mRigid.force -= mRigid.linVelocity * mDrag;
    mRigid.torque -= mRigid.angMomentum * mDrag;

    // If we've added anything other than gravity, then we're no
    // longer at rest. Could test this a little more efficiently...
    if (mRigid.atRest && (mRigid.force.len() || mRigid.torque.len()))
        mRigid.atRest = false;

    // Gravity
    mRigid.force += Point3F(0, 0, sWheeledVehicleGravity * mRigid.mass);

    // Integrate and update velocity
    mRigid.linMomentum += mRigid.force * dt;
    mRigid.angMomentum += mRigid.torque * dt;
    mRigid.updateVelocity();

    // Since we've already done all the work, just need to clear this out.
    mRigid.force.set(0, 0, 0);
    mRigid.torque.set(0, 0, 0);

    // If we're still atRest, make sure we're not accumulating anything
    if (mRigid.atRest)
        mRigid.setAtRest();
}


//----------------------------------------------------------------------------
/** Extend the wheels
   The wheels are extended until they contact a surface. The extension
   is instantaneous.  The wheels are extended before force calculations and
   also on during client side interpolation (so that the wheels are glued
   to the ground).
*/
void WheeledVehicle::extendWheels(bool clientHack)
{
    disableCollision();

    MatrixF currMatrix;

    if (clientHack)
        currMatrix = getRenderTransform();
    else
        mRigid.getTransform(&currMatrix);


    // Does a single ray cast down for now... this will have to be
    // changed to something a little more complicated to avoid getting
    // stuck in cracks.
    Wheel* wend = &mWheel[mDataBlock->wheelCount];
    for (Wheel* wheel = mWheel; wheel < wend; wheel++)
    {
        if (wheel->tire && wheel->spring)
        {
            wheel->extension = 1;

            // The ray is cast from the spring mount point to the tip of
            // the tire.  If there is a collision the spring extension is
            // adjust to remove the tire radius.
            Point3F sp, vec;
            currMatrix.mulP(wheel->data->pos, &sp);
            currMatrix.mulV(VectorF(0, 0, -wheel->spring->length), &vec);
            F32 ts = wheel->tire->radius / wheel->spring->length;
            Point3F ep = sp + (vec * (1 + ts));
            ts = ts / (1 + ts);

            RayInfo rInfo;
            if (mContainer->castRay(sp, ep, sClientCollisionMask & ~PlayerObjectType, &rInfo))
            {
                wheel->surface.contact = true;
                wheel->extension = (rInfo.t < ts) ? 0 : (rInfo.t - ts) / (1 - ts);
                wheel->surface.normal = rInfo.normal;
                wheel->surface.pos = rInfo.point;
                wheel->surface.material = rInfo.material;
                wheel->surface.object = rInfo.object;
            }
            else
            {
                wheel->surface.contact = false;
                wheel->slipping = true;
            }
        }
    }
    enableCollision();
}


//----------------------------------------------------------------------------
/** Update wheel steering and suspension threads.
   These animations are purely cosmetic and this method is only invoked
   on the client.
*/
void WheeledVehicle::updateWheelThreads()
{
    Wheel* wend = &mWheel[mDataBlock->wheelCount];
    for (Wheel* wheel = mWheel; wheel < wend; wheel++)
    {
        if (wheel->tire && wheel->spring && wheel->springThread)
        {
            // Scale the spring animation time to match the current
            // position of the wheel.  We'll also check to make sure
            // the animation is long enough, if it isn't, just stick
            // it at the end.
            F32 pos = wheel->extension * wheel->spring->length;
            if (pos > wheel->data->springLength)
                pos = 1;
            else
                pos /= wheel->data->springLength;
            mShapeInstance->setPos(wheel->springThread, pos);
        }
    }
}

//----------------------------------------------------------------------------
/** Update wheel particles effects
   These animations are purely cosmetic and this method is only invoked
   on the client.  Particles are emitted as long as the moving.
*/
void WheeledVehicle::updateWheelParticles(F32 dt)
{

    // OMG l33t hax
    extendWheels(true);

    Point3F vel = Parent::getVelocity();
    F32 speed = vel.len();

    // Don't bother if we're not moving.
    if (speed > 1.0f)
    {
        MaterialPropertyMap* matMap = MaterialPropertyMap::get();
        Point3F axis = vel;
        axis.normalize();

        // Only emit dust on the terrain
        Wheel* wend = &mWheel[mDataBlock->wheelCount];
        for (Wheel* wheel = mWheel; wheel < wend; wheel++)
        {
            // Is this wheel in contact with the ground?
            if (wheel->tire && wheel->spring && !wheel->emitter.isNull() &&
                wheel->surface.contact && wheel->surface.object &&
                wheel->surface.object->getTypeMask() & TerrainObjectType)
            {

                TerrainBlock* tBlock = static_cast<TerrainBlock*>(wheel->surface.object);
                S32 mapIndex = tBlock->mMPMIndex[0];

                // Override the dust color with the material property
                const MaterialPropertyMap::MapEntry* pEntry;
                if (matMap && mapIndex != -1 &&
                    (pEntry = matMap->getMapEntryFromIndex(mapIndex)) != 0)
                {
                    ColorF colorList[ParticleData::PDC_NUM_KEYS];
                    for (S32 x = 0; x < 2; ++x)
                        colorList[x].set(pEntry->puffColor[x].red,
                            pEntry->puffColor[x].green,
                            pEntry->puffColor[x].blue,
                            pEntry->puffColor[x].alpha);
                    for (S32 x = 2; x < ParticleData::PDC_NUM_KEYS; ++x)
                        colorList[x].set(1.0, 1.0, 1.0, 0.0);
                    wheel->emitter->setColors(colorList);
                }

                // Emit the dust, the density (time) is scaled by the
                // the vehicles velocity.
                wheel->emitter->emitParticles(wheel->surface.pos, true,
                    axis, vel, (U32)(3/*dt * (speed / mDataBlock->maxWheelSpeed) * 1000 * wheel->slip*/));
            }
        }
    }
}


//----------------------------------------------------------------------------
/** Update engine sound
   This method is only invoked by clients.
*/
void WheeledVehicle::updateEngineSound(F32 level)
{
    if (!mEngineSound)
        return;

    if (!mEngineSound->isPlaying())
        mEngineSound->play();

    mEngineSound->setTransform(getTransform());
    mEngineSound->setVelocity(getVelocity());
    //mEngineSound->setVolume( level );

    // Adjust pitch
    F32 pitch = ((level - sIdleEngineVolume) * 1.3f);
    if (pitch < 0.4f)
        pitch = 0.4f;

    mEngineSound->setPitch(pitch);
}


//----------------------------------------------------------------------------
/** Update wheel skid sound
   This method is only invoked by clients.
*/
void WheeledVehicle::updateSquealSound(F32 level)
{
    if (!mSquealSound)
        return;

    if (level < sMinSquealVolume)
    {
        mSquealSound->stop();
        return;
    }

    if (!mSquealSound->isPlaying())
        mSquealSound->play();

    mSquealSound->setTransform(getTransform());
    mSquealSound->setVolume(level);
}


//----------------------------------------------------------------------------
/** Update jet sound
   This method is only invoked by clients.
*/
void WheeledVehicle::updateJetSound()
{
    if (!mJetSound)
        return;

    if (!mJetting)
    {
        mJetSound->stop();
        return;
    }

    if (!mJetSound->isPlaying())
        mJetSound->play();

    mJetSound->setTransform(getTransform());
}


//----------------------------------------------------------------------------

U32 WheeledVehicle::getCollisionMask()
{
    return sClientCollisionMask;
}


//----------------------------------------------------------------------------
/** Build a collision polylist
   The polylist is filled with polygons representing the collision volume
   and the wheels.
*/
bool WheeledVehicle::buildPolyList(AbstractPolyList* polyList, const Box3F& box, const SphereF& sphere)
{
    // Parent will take care of body collision.
    Parent::buildPolyList(polyList, box, sphere);

    // Add wheels as boxes.
    Wheel* wend = &mWheel[mDataBlock->wheelCount];
    for (Wheel* wheel = mWheel; wheel < wend; wheel++) {
        if (wheel->tire && wheel->spring) {
            Box3F wbox;
            F32 radius = wheel->tire->radius;
            wbox.min.x = -(wbox.max.x = radius / 2);
            wbox.min.y = -(wbox.max.y = radius);
            wbox.min.z = -(wbox.max.z = radius);
            MatrixF mat = mObjToWorld;

            Point3F sp, vec;
            mObjToWorld.mulP(wheel->data->pos, &sp);
            mObjToWorld.mulV(VectorF(0, 0, -wheel->spring->length), &vec);
            Point3F ep = sp + (vec * wheel->extension);
            mat.setColumn(3, ep);
            polyList->setTransform(&mat, Point3F(1, 1, 1));
            polyList->addBox(wbox);
        }
    }
    return !polyList->isEmpty();
}


//----------------------------------------------------------------------------

void WheeledVehicle::renderImage(SceneState* state)
{
    Parent::renderImage(state);

    // Shape transform
    GFX->pushWorldMatrix();

    MatrixF mat = getRenderTransform();
    mat.scale(mObjScale);
    GFX->setWorldMatrix(mat);

    Wheel* wend = &mWheel[mDataBlock->wheelCount];
    for (Wheel* wheel = mWheel; wheel < wend; wheel++)
    {
        if (wheel->shapeInstance)
        {
            GFX->pushWorldMatrix();

            // Steering & spring extension
            MatrixF hub(EulerF(0, 0, mSteering.x * wheel->steering));
            Point3F pos = wheel->data->pos;
            pos.z -= wheel->spring->length * wheel->extension;
            hub.setColumn(3, pos);

            GFX->multWorld(hub);

            // Wheel rotation
            MatrixF rot(EulerF(wheel->apos * M_2PI, 0, 0));
            GFX->multWorld(rot);

            // Rotation the tire to face the right direction
            // (could pre-calculate this)
            MatrixF wrot(EulerF(0, 0, (wheel->data->pos.x > 0) ? M_PI / 2 : -M_PI / 2));
            GFX->multWorld(wrot);

            // Render!
            wheel->shapeInstance->animate();
            wheel->shapeInstance->render();

            GFX->popWorldMatrix();
        }
    }

    GFX->popWorldMatrix();
}


//----------------------------------------------------------------------------

void WheeledVehicle::writePacketData(GameConnection* connection, BitStream* stream)
{
    Parent::writePacketData(connection, stream);
    stream->writeFlag(mBraking);

    Wheel* wend = &mWheel[mDataBlock->wheelCount];
    for (Wheel* wheel = mWheel; wheel < wend; wheel++)
    {
        stream->write(wheel->avel);
        stream->write(wheel->Dy);
        stream->write(wheel->Dx);
        stream->writeFlag(wheel->slipping);
    }
}

void WheeledVehicle::readPacketData(GameConnection* connection, BitStream* stream)
{
    Parent::readPacketData(connection, stream);
    mBraking = stream->readFlag();

    Wheel* wend = &mWheel[mDataBlock->wheelCount];
    for (Wheel* wheel = mWheel; wheel < wend; wheel++)
    {
        stream->read(&wheel->avel);
        stream->read(&wheel->Dy);
        stream->read(&wheel->Dx);
        wheel->slipping = stream->readFlag();
    }

    // Rigid state is transmitted by the parent...
    setPosition(mRigid.linPosition, mRigid.angPosition);
    mDelta.pos = mRigid.linPosition;
    mDelta.rot[1] = mRigid.angPosition;
}


//----------------------------------------------------------------------------

U32 WheeledVehicle::packUpdate(NetConnection* con, U32 mask, BitStream* stream)
{
    U32 retMask = Parent::packUpdate(con, mask, stream);

    // Update wheel datablock information
    if (stream->writeFlag(mask & WheelMask))
    {
        Wheel* wend = &mWheel[mDataBlock->wheelCount];
        for (Wheel* wheel = mWheel; wheel < wend; wheel++)
        {
            if (stream->writeFlag(wheel->tire && wheel->spring))
            {
                stream->writeRangedU32(wheel->tire->getId(),
                    DataBlockObjectIdFirst, DataBlockObjectIdLast);
                stream->writeRangedU32(wheel->spring->getId(),
                    DataBlockObjectIdFirst, DataBlockObjectIdLast);
                stream->writeFlag(wheel->powered);

                // Steering must be sent with full precision as it's
                // used directly in state force calculations.
                stream->write(wheel->steering);
            }
        }
    }

    // The rest of the data is part of the control object packet update.
    // If we're controlled by this client, we don't need to send it.
    if (stream->writeFlag(getControllingClient() == con && !(mask & InitialUpdateMask)))
        return retMask;

    stream->writeFlag(mBraking);

    if (stream->writeFlag(mask & PositionMask))
    {
        Wheel* wend = &mWheel[mDataBlock->wheelCount];
        for (Wheel* wheel = mWheel; wheel < wend; wheel++)
        {
            stream->write(wheel->avel);
            stream->write(wheel->Dy);
            stream->write(wheel->Dx);
        }
    }
    return retMask;
}

void WheeledVehicle::unpackUpdate(NetConnection* con, BitStream* stream)
{
    Parent::unpackUpdate(con, stream);

    // Update wheel datablock information
    if (stream->readFlag())
    {
        Wheel* wend = &mWheel[mDataBlock->wheelCount];
        for (Wheel* wheel = mWheel; wheel < wend; wheel++)
        {
            if (stream->readFlag())
            {
                SimObjectId tid = stream->readRangedU32(DataBlockObjectIdFirst, DataBlockObjectIdLast);
                SimObjectId sid = stream->readRangedU32(DataBlockObjectIdFirst, DataBlockObjectIdLast);
                if (!Sim::findObject(tid, wheel->tire) || !Sim::findObject(sid, wheel->spring))
                {
                    con->setLastError("Invalid packet WheeledVehicle::unpackUpdate()");
                    return;
                }
                wheel->powered = stream->readFlag();
                stream->read(&wheel->steering);

                // Create an instance of the tire for rendering
                delete wheel->shapeInstance;
                wheel->shapeInstance = (wheel->tire->shape.isNull()) ? 0 :
                    new TSShapeInstance(wheel->tire->shape);
            }
        }
    }

    // After this is data that we only need if we're not the
    // controlling client.
    if (stream->readFlag())
        return;

    mBraking = stream->readFlag();

    if (stream->readFlag())
    {
        Wheel* wend = &mWheel[mDataBlock->wheelCount];
        for (Wheel* wheel = mWheel; wheel < wend; wheel++)
        {
            stream->read(&wheel->avel);
            stream->read(&wheel->Dy);
            stream->read(&wheel->Dx);
        }
    }
}


//----------------------------------------------------------------------------
// Console Methods
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------

ConsoleMethod(WheeledVehicle, setWheelSteering, bool, 4, 4, "obj.setWheelSteering(wheel#,float)")
{
    S32 wheel = dAtoi(argv[2]);
    if (wheel >= 0 && wheel < object->getWheelCount()) {
        object->setWheelSteering(wheel, dAtof(argv[3]));
        return true;
    }
    else
        Con::warnf("setWheelSteering: wheel index out of bounds, vehicle has %d hubs",
            argv[3], object->getWheelCount());
    return false;
}

ConsoleMethod(WheeledVehicle, setWheelPowered, bool, 4, 4, "obj.setWheelPowered(wheel#,bool)")
{
    S32 wheel = dAtoi(argv[2]);
    if (wheel >= 0 && wheel < object->getWheelCount()) {
        object->setWheelPowered(wheel, dAtob(argv[3]));
        return true;
    }
    else
        Con::warnf("setWheelPowered: wheel index out of bounds, vehicle has %d hubs",
            argv[3], object->getWheelCount());
    return false;
}

ConsoleMethod(WheeledVehicle, setWheelTire, bool, 4, 4, "obj.setWheelTire(wheel#,tire)")
{
    WheeledVehicleTire* tire;
    if (Sim::findObject(argv[3], tire)) {
        S32 wheel = dAtoi(argv[2]);
        if (wheel >= 0 && wheel < object->getWheelCount()) {
            object->setWheelTire(wheel, tire);
            return true;
        }
        else
            Con::warnf("setWheelTire: wheel index out of bounds, vehicle has %d hubs",
                argv[3], object->getWheelCount());
    }
    else
        Con::warnf("setWheelTire: %s datablock does not exist (or is not a tire)", argv[3]);
    return false;
}

ConsoleMethod(WheeledVehicle, setWheelSpring, bool, 4, 4, "obj.setWheelSpring(wheel#,spring)")
{
    WheeledVehicleSpring* spring;
    if (Sim::findObject(argv[3], spring)) {
        S32 wheel = dAtoi(argv[2]);
        if (wheel >= 0 && wheel < object->getWheelCount()) {
            object->setWheelSpring(wheel, spring);
            return true;
        }
        else
            Con::warnf("setWheelSpring: wheel index out of bounds, vehicle has %d hubs",
                argv[3], object->getWheelCount());
    }
    else
        Con::warnf("setWheelSpring: %s datablock does not exist (or is not a spring)", argv[3]);
    return false;
}

ConsoleMethod(WheeledVehicle, getWheelCount, S32, 2, 2, "obj.getWheelCount()")
{
    return object->getWheelCount();
}
