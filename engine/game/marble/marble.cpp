//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "game/marble/marble.h"

#include "audio/audioDataBlock.h"

//----------------------------------------------------------------------------

IMPLEMENT_CO_NETOBJECT_V1(Marble);

U32 Marble::smEndPadId = 0;

Marble::Marble()
{
    // TODO: Finish Implementation
    mControllable = true;
}

Marble::~Marble()
{
    // TODO: Implement
}

void Marble::initPersistFields()
{
    Parent::initPersistFields();

    addField("Controllable", TypeBool, Offset(mControllable, Marble));
}

//----------------------------------------------------------------------------

IMPLEMENT_CO_DATABLOCK_V1(MarbleData);

MarbleData::MarbleData()
{
    minVelocityBounceSoft = 2.5f;
    minVelocityBounceHard = 12.0f;
    bounceMinGain = 0.2f;
    minVelocityMegaBounceSoft = 2.5f;
    maxJumpTicks = 3;
    bounceEmitter = NULL;
    minVelocityMegaBounceHard = 8.0f;
    trailEmitter = NULL;
    mudEmitter = NULL;
    bounceMegaMinGain = 0.5f;
    grassEmitter = NULL;
    maxRollVelocity = 25.0f;
    angularAcceleration = 18.0f;
    brakingAcceleration = 8.0f;
    gravity = 20.0f;
    size = 1.0f;
    megaSize = 2.0f;
    staticFriction = 1.0f;
    kineticFriction = 0.89999998f;
    bounceKineticFriction = 0.2f;
    maxDotSlide = 0.1f;
    bounceRestitution = 0.89999998f;
    airAcceleration = 5.0f;
    energyRechargeRate = 1.0f;
    jumpImpulse = 1.0f;
    maxForceRadius = 1.0f;
    cameraDistance = 2.5f;
    minBounceVel = 0.1f;
    minTrailSpeed = 10.0f;
    minBounceSpeed = 1.0f;
    memset(sound, 0, sizeof(sound));
    genericShadowLevel = 0.40000001f;
    noShadowLevel = 0.0099999998f;
    blastRechargeTime = 1;
    maxNaturalBlastRecharge = 1;
    powerUps = NULL;
    mDecalData = NULL;
    mDecalID = 0;
    startModeTime = 320;
    stopModeTime = 32;
}

void MarbleData::initPersistFields()
{
    addField("maxRollVelocity", TypeF32, Offset(maxRollVelocity, MarbleData));
    addField("angularAcceleration", TypeF32, Offset(angularAcceleration, MarbleData));
    addField("brakingAcceleration", TypeF32, Offset(brakingAcceleration, MarbleData));
    addField("staticFriction", TypeF32, Offset(staticFriction, MarbleData));
    addField("kineticFriction", TypeF32, Offset(kineticFriction, MarbleData));
    addField("bounceKineticFriction", TypeF32, Offset(bounceKineticFriction, MarbleData));
    addField("gravity", TypeF32, Offset(gravity, MarbleData));
    addField("size", TypeF32, Offset(size, MarbleData));
    addField("megaSize", TypeF32, Offset(megaSize, MarbleData));
    addField("maxDotSlide", TypeF32, Offset(maxDotSlide, MarbleData));
    addField("bounceRestitution", TypeF32, Offset(bounceRestitution, MarbleData));
    addField("airAcceleration", TypeF32, Offset(airAcceleration, MarbleData));
    addField("energyRechargeRate", TypeF32, Offset(energyRechargeRate, MarbleData));
    addField("jumpImpulse", TypeF32, Offset(jumpImpulse, MarbleData));
    addField("maxForceRadius", TypeF32, Offset(maxForceRadius, MarbleData));
    addField("cameraDistance", TypeF32, Offset(cameraDistance, MarbleData));
    addField("minBounceVel", TypeF32, Offset(minBounceVel, MarbleData));
    addField("minTrailSpeed", TypeF32, Offset(minTrailSpeed, MarbleData));
    addField("minBounceSpeed", TypeF32, Offset(minBounceSpeed, MarbleData));
    addField("bounceEmitter", TypeParticleEmitterDataPtr, Offset(bounceEmitter, MarbleData));
    addField("trailEmitter", TypeParticleEmitterDataPtr, Offset(trailEmitter, MarbleData));
    addField("mudEmitter", TypeParticleEmitterDataPtr, Offset(mudEmitter, MarbleData));
    addField("grassEmitter", TypeParticleEmitterDataPtr, Offset(grassEmitter, MarbleData));
    addField("powerUps", TypeGameBaseDataPtr, Offset(powerUps, MarbleData));
    addField("blastRechargeTime", TypeS32, Offset(blastRechargeTime, MarbleData));
    addField("maxNaturalBlastRecharge", TypeS32, Offset(maxNaturalBlastRecharge, MarbleData));
    addField("RollHardSound", TypeAudioProfilePtr, Offset(sound[0], MarbleData));
    addField("RollMegaSound", TypeAudioProfilePtr, Offset(sound[1], MarbleData));
    addField("RollIceSound", TypeAudioProfilePtr, Offset(sound[2], MarbleData));
    addField("SlipSound", TypeAudioProfilePtr, Offset(sound[3], MarbleData));
    addField("Bounce1", TypeAudioProfilePtr, Offset(sound[4], MarbleData));
    addField("Bounce2", TypeAudioProfilePtr, Offset(sound[5], MarbleData));
    addField("Bounce3", TypeAudioProfilePtr, Offset(sound[6], MarbleData));
    addField("Bounce4", TypeAudioProfilePtr, Offset(sound[7], MarbleData));
    addField("MegaBounce1", TypeAudioProfilePtr, Offset(sound[8], MarbleData));
    addField("MegaBounce2", TypeAudioProfilePtr, Offset(sound[9], MarbleData));
    addField("MegaBounce3", TypeAudioProfilePtr, Offset(sound[10], MarbleData));
    addField("MegaBounce4", TypeAudioProfilePtr, Offset(sound[11], MarbleData));
    addField("JumpSound", TypeAudioProfilePtr, Offset(sound[12], MarbleData));
    Con::addVariable("Game::endPad", TypeS32, &Marble::smEndPadId);

    Parent::initPersistFields();
}

//----------------------------------------------------------------------------
