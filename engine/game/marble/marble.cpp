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

Marble::Contact::Contact()
{
    // TODO: Implement
}

Marble::SinglePrecision::SinglePrecision()
{
    // TODO: Implement
}

Marble::StateDelta::StateDelta()
{
    // TODO: Implement
}

Marble::EndPadEffect::EndPadEffect()
{
    // TODO: Implement
}

Marble::PowerUpState::PowerUpState()
{
    // TODO: Implement
}

Marble::PowerUpState::~PowerUpState()
{
    // TODO: Implement
}

//----------------------------------------------------------------------------

SceneObject* Marble::getPad()
{
    // TODO: Implement
    return nullptr;
}

S32 Marble::getPowerUpId()
{
    // TODO: Implement
    return 0;
}

const QuatF& Marble::getGravityFrame()
{
    // TODO: Implement
    return QuatF();
}

U32 Marble::getMaxNaturalBlastEnergy()
{
    // TODO: Implement
    return 0;
}

U32 Marble::getMaxBlastEnergy()
{
    // TODO: Implement
    return 0;
}

F32 Marble::getBlastPercent()
{
    // TODO: Implement
    return 0;
}

F32 Marble::getBlastEnergy() const
{
    // TODO: Implement
    return 0;
}

void Marble::setBlastEnergy(F32)
{
    
}

void Marble::setUseFullMarbleTime(bool)
{
    
}

void Marble::setMarbleTime(U32)
{
    
}

U32 Marble::getMarbleTime()
{
    // TODO: Implement
    return 0;
}

void Marble::setMarbleBonusTime(U32)
{
    
}

U32 Marble::getMarbleBonusTime()
{
    // TODO: Implement
    return 0;
}

U32 Marble::getFullMarbleTime()
{
    // TODO: Implement
    return 0;
}

Marble::Contact& Marble::getLastContact()
{
    // TODO: Implement
    return Marble::Contact();
}

void Marble::setGravityFrame(const QuatF&, bool)
{
    
}

void Marble::onSceneRemove()
{
    Parent::onSceneRemove();
}

void Marble::setPosition(const Point3D&, bool)
{
    
}

void Marble::setPosition(const Point3D&, const AngAxisF&, float)
{
    
}

void Marble::setTransform(const MatrixF& mat)
{
    Parent::setTransform(mat);
}

Point3F& Marble::getPosition()
{
    // TODO: Implement
    return Point3F();
}

void Marble::victorySequence()
{
    
}

void Marble::setMode(U32)
{
    
}

void Marble::setOOB(bool)
{
    
}

void Marble::interpolateTick(F32 delta)
{
    Parent::interpolateTick(delta);
}

S32 Marble::mountPowerupImage(ShapeBaseImageData*)
{
    // TODO: Implement
    return 0;
}

void Marble::updatePowerUpParams()
{
    
}

bool Marble::getForce(Point3F& p1, Point3F* p2)
{
    // TODO: Missing in parent?
    //return Parent::getForce(p1, p2);

    // TODO: Implement
    return false;
}

U32 Marble::packUpdate(NetConnection* conn, U32 mask, BitStream* stream)
{
    return Parent::packUpdate(conn, mask, stream);
}

void Marble::unpackUpdate(NetConnection* conn, BitStream* stream)
{
    Parent::unpackUpdate(conn, stream);
}

U32 Marble::filterMaskBits(U32 i, NetConnection* conn)
{
    // TODO: Missing in parent?
    //return Parent::filterMaskBits(i, conn);

    // TODO: Implement
    return 0;
}

void Marble::writePacketData(GameConnection* conn, BitStream* stream)
{
    Parent::writePacketData(conn, stream);
}

void Marble::readPacketData(GameConnection* conn, BitStream* stream)
{
    Parent::readPacketData(conn, stream);
}

void Marble::renderShadowVolumes(SceneState*)
{

}

void Marble::renderShadow(F32, F32)
{
    
}

void Marble::renderImage(SceneState* state)
{
    Parent::renderImage(state);
}

void Marble::bounceEmitter(F32, const Point3F&)
{
    
}

MatrixF Marble::getShadowTransform() const
{
    // TODO: Missing in parent?
    //return Parent::getShadowTransform();

    // TODO: Implement
    return MatrixF();
}

void Marble::setVelocity(const Point3F& vel)
{
    Parent::setVelocity(vel);
}

Point3F Marble::getVelocity() const
{
    return Parent::getVelocity();
}

Point3F Marble::getShadowScale() const
{
    // TODO: Missing in parent?
    //return Parent::getShadowScale();

    // TODO: Implement
    return Point3F();
}

Point3F Marble::getGravityRenderDir()
{

    // TODO: Implement
    return Point3F();
}

void Marble::getShadowLightVectorHack(Point3F& p)
{
    // TODO: Missing in parent?
    //Parent::getShadowLightVectorHack(p);
}

bool Marble::onSceneAdd(SceneGraph* graph)
{
    return Parent::onSceneAdd(graph);
}

bool Marble::onNewDataBlock(GameBaseData* dptr)
{
    return Parent::onNewDataBlock(dptr);
}

void Marble::onRemove()
{
    Parent::onRemove();
}

bool Marble::updatePadState()
{
    // TODO: Implement
    return false;
}

void Marble::doPowerUpBoost(S32)
{
    
}

void Marble::doPowerUpPower(S32)
{
    
}

void Marble::updatePowerups()
{
    
}

void Marble::updateMass()
{
    Parent::updateMass();
}

void Marble::trailEmitter(U32)
{
    
}

void Marble::updateRollSound(F32, F32)
{
    
}

void Marble::playBounceSound(Marble::Contact&, F64)
{
    
}

void Marble::setPad(SceneObject*)
{
    
}

void Marble::findRenderPos(F32)
{
    
}

void Marble::advanceTime(F32 dt)
{
    Parent::advanceTime(dt);
}

void Marble::computeNetSmooth(F32 f)
{
    // TODO: Missing in parent?
    //Parent::computeNetSmooth(f)
}

void Marble::doPowerUp(S32)
{
    
}

void Marble::prepShadows()
{
    
}

bool Marble::onAdd()
{
    return Parent::onAdd();
}

void Marble::processMoveTriggers(const Move *)
{
    
}

void Marble::processItemsAndTriggers(const Point3F&, const Point3F&)
{
    
}

void Marble::setPowerUpId(U32, bool)
{
    
}

void Marble::processTick(const Move* move)
{
    Parent::processTick(move);
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

bool MarbleData::preload(bool server, char errorBuffer[256])
{
    return Parent::preload(server, errorBuffer);
}

void MarbleData::packData(BitStream* stream)
{
    Parent::packData(stream);
}

void MarbleData::unpackData(BitStream* stream)
{
    Parent::unpackData(stream);
}

//----------------------------------------------------------------------------
