//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "platform/platform.h"
#include "core/bitStream.h"
#include "ts/tsShapeInstance.h"
#include "console/consoleInternal.h"
#include "console/consoleTypes.h"
#include "game/fx/particleEmitter.h"
#include "game/shapeBase.h"
#include "game/projectile.h"
#include "game/gameConnection.h"
#include "math/mathIO.h"
#include "game/debris.h"
#include "math/mathUtils.h"
#include "sim/netObject.h"
#include "sfx/sfxSystem.h"

//----------------------------------------------------------------------------

ShapeBaseImageData* InvalidImagePtr = (ShapeBaseImageData*)1;

static EnumTable::Enums enumLoadedStates[] =
{
   { ShapeBaseImageData::StateData::IgnoreLoaded, "Ignore" },
   { ShapeBaseImageData::StateData::Loaded,       "Loaded" },
   { ShapeBaseImageData::StateData::NotLoaded,    "Empty" },
};
static EnumTable EnumLoadedState(3, &enumLoadedStates[0]);

static EnumTable::Enums enumSpinStates[] =
{
   { ShapeBaseImageData::StateData::IgnoreSpin,"Ignore" },
   { ShapeBaseImageData::StateData::NoSpin,    "Stop" },
   { ShapeBaseImageData::StateData::SpinUp,    "SpinUp" },
   { ShapeBaseImageData::StateData::SpinDown,  "SpinDown" },
   { ShapeBaseImageData::StateData::FullSpin,  "FullSpeed" },
};
static EnumTable EnumSpinState(5, &enumSpinStates[0]);

static EnumTable::Enums enumRecoilStates[] =
{
   { ShapeBaseImageData::StateData::NoRecoil,     "NoRecoil" },
   { ShapeBaseImageData::StateData::LightRecoil,  "LightRecoil" },
   { ShapeBaseImageData::StateData::MediumRecoil, "MediumRecoil" },
   { ShapeBaseImageData::StateData::HeavyRecoil,  "HeavyRecoil" },
};
static EnumTable EnumRecoilState(4, &enumRecoilStates[0]);


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

IMPLEMENT_CO_DATABLOCK_V1(ShapeBaseImageData);

ShapeBaseImageData::StateData::StateData()
{
    name = 0;
    transition.loaded[0] = transition.loaded[1] = -1;
    transition.ammo[0] = transition.ammo[1] = -1;
    transition.target[0] = transition.target[1] = -1;
    transition.trigger[0] = transition.trigger[1] = -1;
    transition.wet[0] = transition.wet[1] = -1;
    transition.timeout = -1;
    waitForTimeout = true;
    timeoutValue = 0;
    fire = false;
    energyDrain = 0;
    allowImageChange = true;
    loaded = IgnoreLoaded;
    spin = IgnoreSpin;
    recoil = NoRecoil;
    flashSequence = false;
    sequence = -1;
    sequenceVis = -1;
    sound = 0;
    emitter = NULL;
    script = 0;
    ignoreLoadedForReady = false;
}

static ShapeBaseImageData::StateData gDefaultStateData;

//----------------------------------------------------------------------------

ShapeBaseImageData::ShapeBaseImageData()
{
    emap = false;

    mountPoint = 0;
    mountOffset.identity();
    eyeOffset.identity();
    correctMuzzleVector = true;
    firstPerson = true;
    useEyeOffset = false;
    mass = 0;

    usesEnergy = false;
    minEnergy = 2;
    accuFire = false;

    projectile = NULL;

    cloakable = true;

    lightType = ShapeBaseImageData::NoLight;
    lightColor.set(1.f, 1.f, 1.f, 1.f);
    lightTime = 1000;
    lightRadius = 10.f;

    mountTransform.identity();
    shapeName = "";
    fireState = -1;
    computeCRC = false;

    //
    for (int i = 0; i < MaxStates; i++) {
        stateName[i] = 0;
        stateTransitionLoaded[i] = 0;
        stateTransitionNotLoaded[i] = 0;
        stateTransitionAmmo[i] = 0;
        stateTransitionNoAmmo[i] = 0;
        stateTransitionTarget[i] = 0;
        stateTransitionNoTarget[i] = 0;
        stateTransitionWet[i] = 0;
        stateTransitionNotWet[i] = 0;
        stateTransitionTriggerUp[i] = 0;
        stateTransitionTriggerDown[i] = 0;
        stateTransitionTimeout[i] = 0;
        stateWaitForTimeout[i] = true;
        stateTimeoutValue[i] = 0;
        stateFire[i] = false;
        stateEjectShell[i] = false;
        stateEnergyDrain[i] = 0;
        stateAllowImageChange[i] = true;
        stateScaleAnimation[i] = true;
        stateDirection[i] = true;
        stateLoaded[i] = StateData::IgnoreLoaded;
        stateSpin[i] = StateData::IgnoreSpin;
        stateRecoil[i] = StateData::NoRecoil;
        stateSequence[i] = 0;
        stateSequenceRandomFlash[i] = false;
        stateSound[i] = 0;
        stateScript[i] = 0;
        stateEmitter[i] = 0;
        stateEmitterTime[i] = 0;
        stateEmitterNode[i] = 0;
        stateIgnoreLoadedForReady[i] = false;
    }
    statesLoaded = false;

    casing = NULL;
    casingID = 0;
    shellExitDir.set(1.0, 0.0, 1.0);
    shellExitDir.normalize();
    shellExitVariance = 20.0;
    shellVelocity = 1.0;

}

ShapeBaseImageData::~ShapeBaseImageData()
{

}

bool ShapeBaseImageData::onAdd()
{
    if (!Parent::onAdd())
        return false;

    // Copy state data from the scripting arrays into the
    // state structure array. If we have state data already,
    // we are on the client and need to leave it alone.
    for (U32 i = 0; i < MaxStates; i++) {
        StateData& s = state[i];
        if (statesLoaded == false) {
            s.name = stateName[i];
            s.transition.loaded[0] = lookupState(stateTransitionNotLoaded[i]);
            s.transition.loaded[1] = lookupState(stateTransitionLoaded[i]);
            s.transition.ammo[0] = lookupState(stateTransitionNoAmmo[i]);
            s.transition.ammo[1] = lookupState(stateTransitionAmmo[i]);
            s.transition.target[0] = lookupState(stateTransitionNoTarget[i]);
            s.transition.target[1] = lookupState(stateTransitionTarget[i]);
            s.transition.wet[0] = lookupState(stateTransitionNotWet[i]);
            s.transition.wet[1] = lookupState(stateTransitionWet[i]);
            s.transition.trigger[0] = lookupState(stateTransitionTriggerUp[i]);
            s.transition.trigger[1] = lookupState(stateTransitionTriggerDown[i]);
            s.transition.timeout = lookupState(stateTransitionTimeout[i]);
            s.waitForTimeout = stateWaitForTimeout[i];
            s.timeoutValue = stateTimeoutValue[i];
            s.fire = stateFire[i];
            s.ejectShell = stateEjectShell[i];
            s.energyDrain = stateEnergyDrain[i];
            s.allowImageChange = stateAllowImageChange[i];
            s.scaleAnimation = stateScaleAnimation[i];
            s.direction = stateDirection[i];
            s.loaded = stateLoaded[i];
            s.spin = stateSpin[i];
            s.recoil = stateRecoil[i];
            s.sequence = -1; // Sequence is resolved in load
            s.sequenceVis = -1; // Vis Sequence is resolved in load
            s.sound = stateSound[i];
            s.script = stateScript[i];
            s.emitter = stateEmitter[i];
            s.emitterTime = stateEmitterTime[i];
            s.emitterNode = -1; // Sequnce is resolved in load
        }

        // The first state marked as "fire" is the state entered on the
        // client when it recieves a fire event.
        if (s.fire && fireState == -1)
            fireState = i;
    }

    // Always preload images, this is needed to avoid problems with
    // resolving sequences before transmission to a client.
    return true;
}

bool ShapeBaseImageData::preload(bool server, char errorBuffer[256])
{
    if (!Parent::preload(server, errorBuffer))
        return false;

    // Resolve objects transmitted from server
    if (!server) {
        if (projectile)
            if (Sim::findObject(static_cast<SimObjectId>(reinterpret_cast<size_t>(projectile)), projectile) == false)
                Con::errorf(ConsoleLogEntry::General, "Error, unable to load projectile for shapebaseimagedata");

        for (U32 i = 0; i < MaxStates; i++) {
            if (state[i].emitter)
                if (!Sim::findObject(static_cast<SimObjectId>(reinterpret_cast<size_t>(state[i].emitter)), state[i].emitter))
                    Con::errorf(ConsoleLogEntry::General, "Error, unable to load emitter for image datablock");
#ifdef MBG_SHAPEBASEFIX
            // For some reason on MBU this breaks the gyrocopter sound, but on MBG not having this causes a crash when using the gyrocopter.
            if (state[i].sound)
                if (!Sim::findObject(SimObjectId(state[i].sound), state[i].sound))
                    Con::errorf(ConsoleLogEntry::General, "Error, unable to load sound profile for image datablock");
#endif
        }
    }

    // Use the first person eye offset if it's set.
    useEyeOffset = !eyeOffset.isIdentity();

    if (shapeName && shapeName[0]) {
        // Resolve shapename
        shape = ResourceManager->load(shapeName, computeCRC);
        if (!bool(shape)) {
            dSprintf(errorBuffer, 256, "Unable to load shape: %s", shapeName);
            return false;
        }
        if (computeCRC)
        {
            Con::printf("Validation required for shape: %s", shapeName);
            if (server)
                mCRC = shape.getCRC();
            else if (mCRC != shape.getCRC())
            {
                dSprintf(errorBuffer, 256, "Shape \"%s\" does not match version on server.", shapeName);
                return false;
            }
        }

        // Resolve nodes & build mount transform
        ejectNode = shape->findNode("ejectPoint");
        muzzleNode = shape->findNode("muzzlePoint");
        retractNode = shape->findNode("retractionPoint");
        mountTransform = mountOffset;
        S32 node = shape->findNode("mountPoint");
        if (node != -1) {
            MatrixF total(1);
            do {
                MatrixF nmat;
                QuatF q;
                TSTransform::setMatrix(shape->defaultRotations[node].getQuatF(&q), shape->defaultTranslations[node], &nmat);
                total.mul(nmat);
                node = shape->nodes[node].parentIndex;
            } while (node != -1);
            total.inverse();
            mountTransform.mul(total);
        }

        // Resolve state sequence names & emitter nodes
        isAnimated = false;
        hasFlash = false;
        for (U32 i = 0; i < MaxStates; i++) {
            StateData& s = state[i];
            if (stateSequence[i] && stateSequence[i][0])
                s.sequence = shape->findSequence(stateSequence[i]);
            if (s.sequence != -1)
            {
                isAnimated = true;
            }

            if (stateSequence[i] && stateSequence[i][0] && stateSequenceRandomFlash[i]) {
                char bufferVis[128];
                dStrncpy(bufferVis, stateSequence[i], 100);
                dStrcat(bufferVis, "_vis");
                s.sequenceVis = shape->findSequence(bufferVis);
            }
            if (s.sequenceVis != -1)
            {
                s.flashSequence = true;
                hasFlash = true;
            }
            s.ignoreLoadedForReady = stateIgnoreLoadedForReady[i];

            if (stateEmitterNode[i] && stateEmitterNode[i][0])
                s.emitterNode = shape->findNode(stateEmitterNode[i]);
            if (s.emitterNode == -1)
                s.emitterNode = muzzleNode;
        }
        ambientSequence = shape->findSequence("ambient");
        spinSequence = shape->findSequence("spin");
    }
    else {
        dSprintf(errorBuffer, sizeof(errorBuffer), "Bad Datablock from server");
        return false;
    }

    if (!casing && casingID != 0)
    {
        if (!Sim::findObject(SimObjectId(casingID), casing))
        {
            Con::errorf(ConsoleLogEntry::General, "ShapeBaseImageData::preload: Invalid packet, bad datablockId(casing): 0x%x", casingID);
        }
    }


    TSShapeInstance* pDummy = new TSShapeInstance(shape, !server);
    delete pDummy;
    return true;
}

S32 ShapeBaseImageData::lookupState(const char* name)
{
    if (!name || !name[0])
        return -1;
    for (U32 i = 0; i < MaxStates; i++)
        if (stateName[i] && !dStricmp(name, stateName[i]))
            return i;
    Con::errorf(ConsoleLogEntry::General, "ShapeBaseImageData:: Could not resolve state \"%s\" for image \"%s\"", name, getName());
    return 0;
}

static EnumTable::Enums imageLightEnum[] =
{
    { ShapeBaseImageData::NoLight,           "NoLight" },
   { ShapeBaseImageData::ConstantLight,     "ConstantLight" },
   { ShapeBaseImageData::PulsingLight,      "PulsingLight" },
   { ShapeBaseImageData::WeaponFireLight,   "WeaponFireLight" }
};

static EnumTable gImageLightTypeTable(ShapeBaseImageData::NumLightTypes, &imageLightEnum[0]);

void ShapeBaseImageData::initPersistFields()
{
    Parent::initPersistFields();

    addField("emap", TypeBool, Offset(emap, ShapeBaseImageData));
    addField("shapeFile", TypeFilename, Offset(shapeName, ShapeBaseImageData));

    addField("projectile", TypeProjectileDataPtr, Offset(projectile, ShapeBaseImageData));

    addField("cloakable", TypeBool, Offset(cloakable, ShapeBaseImageData));

    addField("mountPoint", TypeS32, Offset(mountPoint, ShapeBaseImageData));
    addField("offset", TypeMatrixPosition, Offset(mountOffset, ShapeBaseImageData));
    addField("rotation", TypeMatrixRotation, Offset(mountOffset, ShapeBaseImageData));
    addField("eyeOffset", TypeMatrixPosition, Offset(eyeOffset, ShapeBaseImageData));
    addField("eyeRotation", TypeMatrixRotation, Offset(eyeOffset, ShapeBaseImageData));
    addField("correctMuzzleVector", TypeBool, Offset(correctMuzzleVector, ShapeBaseImageData));
    addField("firstPerson", TypeBool, Offset(firstPerson, ShapeBaseImageData));
    addField("mass", TypeF32, Offset(mass, ShapeBaseImageData));

    addField("usesEnergy", TypeBool, Offset(usesEnergy, ShapeBaseImageData));
    addField("minEnergy", TypeF32, Offset(minEnergy, ShapeBaseImageData));
    addField("accuFire", TypeBool, Offset(accuFire, ShapeBaseImageData));

    addField("lightType", TypeEnum, Offset(lightType, ShapeBaseImageData), 1, &gImageLightTypeTable);
    addField("lightColor", TypeColorF, Offset(lightColor, ShapeBaseImageData));
    addField("lightTime", TypeS32, Offset(lightTime, ShapeBaseImageData));
    addField("lightRadius", TypeF32, Offset(lightRadius, ShapeBaseImageData));

    addField("casing", TypeDebrisDataPtr, Offset(casing, ShapeBaseImageData));
    addField("shellExitDir", TypePoint3F, Offset(shellExitDir, ShapeBaseImageData));
    addField("shellExitVariance", TypeF32, Offset(shellExitVariance, ShapeBaseImageData));
    addField("shellVelocity", TypeF32, Offset(shellVelocity, ShapeBaseImageData));

    // State arrays
    addField("stateName", TypeCaseString, Offset(stateName, ShapeBaseImageData), MaxStates);
    addField("stateTransitionOnLoaded", TypeString, Offset(stateTransitionLoaded, ShapeBaseImageData), MaxStates);
    addField("stateTransitionOnNotLoaded", TypeString, Offset(stateTransitionNotLoaded, ShapeBaseImageData), MaxStates);
    addField("stateTransitionOnAmmo", TypeString, Offset(stateTransitionAmmo, ShapeBaseImageData), MaxStates);
    addField("stateTransitionOnNoAmmo", TypeString, Offset(stateTransitionNoAmmo, ShapeBaseImageData), MaxStates);
    addField("stateTransitionOnTarget", TypeString, Offset(stateTransitionTarget, ShapeBaseImageData), MaxStates);
    addField("stateTransitionOnNoTarget", TypeString, Offset(stateTransitionNoTarget, ShapeBaseImageData), MaxStates);
    addField("stateTransitionOnWet", TypeString, Offset(stateTransitionWet, ShapeBaseImageData), MaxStates);
    addField("stateTransitionOnNotWet", TypeString, Offset(stateTransitionNotWet, ShapeBaseImageData), MaxStates);
    addField("stateTransitionOnTriggerUp", TypeString, Offset(stateTransitionTriggerUp, ShapeBaseImageData), MaxStates);
    addField("stateTransitionOnTriggerDown", TypeString, Offset(stateTransitionTriggerDown, ShapeBaseImageData), MaxStates);
    addField("stateTransitionOnTimeout", TypeString, Offset(stateTransitionTimeout, ShapeBaseImageData), MaxStates);
    addField("stateTimeoutValue", TypeF32, Offset(stateTimeoutValue, ShapeBaseImageData), MaxStates);
    addField("stateWaitForTimeout", TypeBool, Offset(stateWaitForTimeout, ShapeBaseImageData), MaxStates);
    addField("stateFire", TypeBool, Offset(stateFire, ShapeBaseImageData), MaxStates);
    addField("stateEjectShell", TypeBool, Offset(stateEjectShell, ShapeBaseImageData), MaxStates);
    addField("stateEnergyDrain", TypeF32, Offset(stateEnergyDrain, ShapeBaseImageData), MaxStates);
    addField("stateAllowImageChange", TypeBool, Offset(stateAllowImageChange, ShapeBaseImageData), MaxStates);
    addField("stateDirection", TypeBool, Offset(stateDirection, ShapeBaseImageData), MaxStates);
    addField("stateLoadedFlag", TypeEnum, Offset(stateLoaded, ShapeBaseImageData), MaxStates, &EnumLoadedState);
    addField("stateSpinThread", TypeEnum, Offset(stateSpin, ShapeBaseImageData), MaxStates, &EnumSpinState);
    addField("stateRecoil", TypeEnum, Offset(stateRecoil, ShapeBaseImageData), MaxStates, &EnumRecoilState);
    addField("stateSequence", TypeString, Offset(stateSequence, ShapeBaseImageData), MaxStates);
    addField("stateSequenceRandomFlash", TypeBool, Offset(stateSequenceRandomFlash, ShapeBaseImageData), MaxStates);
    addField("stateSound", TypeSFXProfilePtr, Offset(stateSound, ShapeBaseImageData), MaxStates);
    addField("stateScript", TypeCaseString, Offset(stateScript, ShapeBaseImageData), MaxStates);
    addField("stateEmitter", TypeParticleEmitterDataPtr, Offset(stateEmitter, ShapeBaseImageData), MaxStates);
    addField("stateEmitterTime", TypeF32, Offset(stateEmitterTime, ShapeBaseImageData), MaxStates);
    addField("stateEmitterNode", TypeS32, Offset(stateEmitterNode, ShapeBaseImageData), MaxStates);
    addField("stateIgnoreLoadedForReady", TypeBool, Offset(stateIgnoreLoadedForReady, ShapeBaseImageData), MaxStates);
    addField("computeCRC", TypeBool, Offset(computeCRC, ShapeBaseImageData));
}

void ShapeBaseImageData::packData(BitStream* stream)
{
    Parent::packData(stream);

    if (stream->writeFlag(computeCRC))
        stream->write(mCRC);

    stream->writeString(shapeName);
    stream->write(mountPoint);
    if (!stream->writeFlag(mountOffset.isIdentity()))
        stream->writeAffineTransform(mountOffset);
    if (!stream->writeFlag(eyeOffset.isIdentity()))
        stream->writeAffineTransform(eyeOffset);

    stream->writeFlag(correctMuzzleVector);
    stream->writeFlag(firstPerson);
    stream->write(mass);
    stream->writeFlag(usesEnergy);
    stream->write(minEnergy);
    stream->writeFlag(hasFlash);
    // Client doesn't need accuFire

    // Write the projectile datablock
    if (stream->writeFlag(projectile))
        stream->writeRangedU32(packed ? static_cast<SimObjectId>(reinterpret_cast<size_t>(projectile)) :
            projectile->getId(), DataBlockObjectIdFirst, DataBlockObjectIdLast);

    stream->writeFlag(cloakable);
    stream->writeRangedU32(lightType, 0, NumLightTypes - 1);
    if (lightType != NoLight)
    {
        stream->write(lightRadius);
        stream->write(lightTime);
        stream->writeFloat(lightColor.red, 7);
        stream->writeFloat(lightColor.green, 7);
        stream->writeFloat(lightColor.blue, 7);
        stream->writeFloat(lightColor.alpha, 7);
    }

    mathWrite(*stream, shellExitDir);
    stream->write(shellExitVariance);
    stream->write(shellVelocity);

    if (stream->writeFlag(casing))
    {
        stream->writeRangedU32(packed ? static_cast<SimObjectId>(reinterpret_cast<size_t>(casing)) :
            casing->getId(), DataBlockObjectIdFirst, DataBlockObjectIdLast);
    }

    for (U32 i = 0; i < MaxStates; i++)
        if (stream->writeFlag(state[i].name && state[i].name[0])) {
            StateData& s = state[i];
            // States info not needed on the client:
            //    s.allowImageChange
            //    s.scriptNames
            // Transitions are inc. one to account for -1 values
            stream->writeString(state[i].name);

            stream->writeInt(s.transition.loaded[0] + 1, NumStateBits);
            stream->writeInt(s.transition.loaded[1] + 1, NumStateBits);
            stream->writeInt(s.transition.ammo[0] + 1, NumStateBits);
            stream->writeInt(s.transition.ammo[1] + 1, NumStateBits);
            stream->writeInt(s.transition.target[0] + 1, NumStateBits);
            stream->writeInt(s.transition.target[1] + 1, NumStateBits);
            stream->writeInt(s.transition.wet[0] + 1, NumStateBits);
            stream->writeInt(s.transition.wet[1] + 1, NumStateBits);
            stream->writeInt(s.transition.trigger[0] + 1, NumStateBits);
            stream->writeInt(s.transition.trigger[1] + 1, NumStateBits);
            stream->writeInt(s.transition.timeout + 1, NumStateBits);

            if (stream->writeFlag(s.timeoutValue != gDefaultStateData.timeoutValue))
                stream->write(s.timeoutValue);

            stream->writeFlag(s.waitForTimeout);
            stream->writeFlag(s.fire);
            stream->writeFlag(s.ejectShell);
            stream->writeFlag(s.scaleAnimation);
            stream->writeFlag(s.direction);
            if (stream->writeFlag(s.energyDrain != gDefaultStateData.energyDrain))
                stream->write(s.energyDrain);

            stream->writeInt(s.loaded, StateData::NumLoadedBits);
            stream->writeInt(s.spin, StateData::NumSpinBits);
            stream->writeInt(s.recoil, StateData::NumRecoilBits);
            if (stream->writeFlag(s.sequence != gDefaultStateData.sequence))
                stream->writeSignedInt(s.sequence, 16);

            if (stream->writeFlag(s.sequenceVis != gDefaultStateData.sequenceVis))
                stream->writeSignedInt(s.sequenceVis, 16);
            stream->writeFlag(s.flashSequence);
            stream->writeFlag(s.ignoreLoadedForReady);

            if (stream->writeFlag(s.emitter)) {
                stream->writeRangedU32(packed ? static_cast<SimObjectId>(reinterpret_cast<size_t>(s.emitter)) :
                    s.emitter->getId(), DataBlockObjectIdFirst, DataBlockObjectIdLast);
                stream->write(s.emitterTime);
                stream->write(s.emitterNode);
            }

            if (stream->writeFlag(s.sound))
                stream->writeRangedU32(packed ? static_cast<SimObjectId>(reinterpret_cast<size_t>(s.sound)) :
                    s.sound->getId(), DataBlockObjectIdFirst, DataBlockObjectIdLast);
        }
}

void ShapeBaseImageData::unpackData(BitStream* stream)
{
    Parent::unpackData(stream);
    computeCRC = stream->readFlag();
    if (computeCRC)
        stream->read(&mCRC);

    shapeName = stream->readSTString();
    stream->read(&mountPoint);
    if (stream->readFlag())
        mountOffset.identity();
    else
        stream->readAffineTransform(&mountOffset);
    if (stream->readFlag())
        eyeOffset.identity();
    else
        stream->readAffineTransform(&eyeOffset);

    correctMuzzleVector = stream->readFlag();
    firstPerson = stream->readFlag();
    stream->read(&mass);
    usesEnergy = stream->readFlag();
    stream->read(&minEnergy);
    hasFlash = stream->readFlag();

    projectile = (stream->readFlag() ?
        (ProjectileData*)stream->readRangedU32(DataBlockObjectIdFirst,
            DataBlockObjectIdLast) : 0);

    cloakable = stream->readFlag();
    lightType = stream->readRangedU32(0, NumLightTypes - 1);
    if (lightType != NoLight)
    {
        stream->read(&lightRadius);
        stream->read(&lightTime);
        lightColor.red = stream->readFloat(7);
        lightColor.green = stream->readFloat(7);
        lightColor.blue = stream->readFloat(7);
        lightColor.alpha = stream->readFloat(7);
    }

    mathRead(*stream, &shellExitDir);
    stream->read(&shellExitVariance);
    stream->read(&shellVelocity);

    if (stream->readFlag())
    {
        casingID = stream->readRangedU32(DataBlockObjectIdFirst, DataBlockObjectIdLast);
    }

    for (U32 i = 0; i < MaxStates; i++) {
        if (stream->readFlag()) {
            StateData& s = state[i];
            // States info not needed on the client:
            //    s.allowImageChange
            //    s.scriptNames
            // Transitions are dec. one to restore -1 values
            s.name = stream->readSTString();

            s.transition.loaded[0] = stream->readInt(NumStateBits) - 1;
            s.transition.loaded[1] = stream->readInt(NumStateBits) - 1;
            s.transition.ammo[0] = stream->readInt(NumStateBits) - 1;
            s.transition.ammo[1] = stream->readInt(NumStateBits) - 1;
            s.transition.target[0] = stream->readInt(NumStateBits) - 1;
            s.transition.target[1] = stream->readInt(NumStateBits) - 1;
            s.transition.wet[0] = stream->readInt(NumStateBits) - 1;
            s.transition.wet[1] = stream->readInt(NumStateBits) - 1;
            s.transition.trigger[0] = stream->readInt(NumStateBits) - 1;
            s.transition.trigger[1] = stream->readInt(NumStateBits) - 1;
            s.transition.timeout = stream->readInt(NumStateBits) - 1;
            if (stream->readFlag())
                stream->read(&s.timeoutValue);
            else
                s.timeoutValue = gDefaultStateData.timeoutValue;

            s.waitForTimeout = stream->readFlag();
            s.fire = stream->readFlag();
            s.ejectShell = stream->readFlag();
            s.scaleAnimation = stream->readFlag();
            s.direction = stream->readFlag();
            if (stream->readFlag())
                stream->read(&s.energyDrain);
            else
                s.energyDrain = gDefaultStateData.energyDrain;

            s.loaded = (StateData::LoadedState)stream->readInt(StateData::NumLoadedBits);
            s.spin = (StateData::SpinState)stream->readInt(StateData::NumSpinBits);
            s.recoil = (StateData::RecoilState)stream->readInt(StateData::NumRecoilBits);
            if (stream->readFlag())
                s.sequence = stream->readSignedInt(16);
            else
                s.sequence = gDefaultStateData.sequence;

            if (stream->readFlag())
                s.sequenceVis = stream->readSignedInt(16);
            else
                s.sequenceVis = gDefaultStateData.sequenceVis;

            s.flashSequence = stream->readFlag();
            s.ignoreLoadedForReady = stream->readFlag();

            if (stream->readFlag()) {
                s.emitter = (ParticleEmitterData*)stream->readRangedU32(DataBlockObjectIdFirst,
                    DataBlockObjectIdLast);
                stream->read(&s.emitterTime);
                stream->read(&s.emitterNode);
            }
            else
                s.emitter = 0;
            s.sound = stream->readFlag() ? (SFXProfile*)stream->readRangedU32(DataBlockObjectIdFirst,
                DataBlockObjectIdLast) : 0;
        }
    }
    statesLoaded = true;
}


void ShapeBaseImageData::registerImageLights(LightManager* lightManager, bool lightingScene, const Point3F& objectPosition, U32 startTime)
{
    if (lightingScene)
        return;

    F32 intensity;

    F32 delta = Sim::getCurrentTime() - startTime;

    switch (lightType)
    {
    case ConstantLight:
        intensity = 1.f;
        break;

    case PulsingLight:
    {
        intensity = 0.5f + 0.5f * mSin(M_PI * delta / F32(lightTime));
        intensity = 0.15f + intensity * 0.85f;
        break;
    }

    case WeaponFireLight:
    {
        if (delta > lightTime)
            return;
        intensity = 1.0 - F32(delta) / F32(lightTime);
        break;
    }

    default:
        intensity = 1.0f;
        return;
    }

    mLight.mColor = lightColor * intensity;
    mLight.mColor.clamp();
    mLight.mType = LightInfo::Point;
    mLight.mRadius = lightRadius;

    //get the light source position
    Point3F mountOffset;
    mountTransform.getColumn(3, &mountOffset);
    mLight.mPos = objectPosition + mountOffset;

    lightManager->sgRegisterGlobalLight(&mLight, this, false);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------

ShapeBase::MountedImage::MountedImage()
{
    shapeInstance = 0;
    state = 0;
    dataBlock = 0;
    nextImage = InvalidImagePtr;
    animSound = 0;
    delayTime = 0;
    ammo = false;
    target = false;
    triggerDown = false;
    loaded = false;
    fireCount = 0;
    wet = false;
}

ShapeBase::MountedImage::~MountedImage()
{
    delete shapeInstance;

    // stop sound
    SFX_DELETE(animSound);

    for (S32 i = 0; i < MaxImageEmitters; i++)
        if (bool(emitter[i].emitter))
            emitter[i].emitter->deleteWhenEmpty();
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// Any item with an item image is selectable

bool ShapeBase::mountImage(ShapeBaseImageData* imageData, U32 imageSlot, bool loaded, StringHandle& skinNameHandle)
{
    AssertFatal(imageSlot < MaxMountedImages, "Out of range image slot");

    MountedImage& image = mMountedImageList[imageSlot];
    if (image.dataBlock) {
        if ((image.dataBlock == imageData) && (image.skinNameHandle == skinNameHandle)) {
            // Image already loaded
            image.nextImage = InvalidImagePtr;
            return true;
        }
    }
    //
    setImage(imageSlot, imageData, skinNameHandle, loaded);

    //see if the image has a light source
    if (imageData->lightType != ShapeBaseImageData::NoLight)
        Sim::getLightSet()->addObject(this);

    return true;
}

bool ShapeBase::unmountImage(U32 imageSlot)
{
    AssertFatal(imageSlot < MaxMountedImages, "Out of range image slot");

    bool returnValue = false;
    MountedImage& image = mMountedImageList[imageSlot];
    if (image.dataBlock)
    {
        StringHandle temp;
        setImage(imageSlot, 0, temp);
        returnValue = true;
    }

    //see if we're still part of the light group
    bool found = false;
    for (S32 i = 0; i < MaxMountedImages; i++)
    {
        ShapeBaseImageData* imageData = getMountedImage(i);
        if (imageData != NULL && imageData->lightType != ShapeBaseImageData::NoLight)
        {
            found = true;
            break;
        }
    }
    if (!found)
        Sim::getLightSet()->removeObject(this);


    return returnValue;
}


//----------------------------------------------------------------------------

ShapeBaseImageData* ShapeBase::getMountedImage(U32 imageSlot)
{
    AssertFatal(imageSlot < MaxMountedImages, "Out of range image slot");

    return mMountedImageList[imageSlot].dataBlock;
}


ShapeBase::MountedImage* ShapeBase::getImageStruct(U32 imageSlot)
{
    return &mMountedImageList[imageSlot];
}


ShapeBaseImageData* ShapeBase::getPendingImage(U32 imageSlot)
{
    ShapeBaseImageData* data = mMountedImageList[imageSlot].nextImage;
    return (data == InvalidImagePtr) ? 0 : data;
}

bool ShapeBase::isImageFiring(U32 imageSlot)
{
    MountedImage& image = mMountedImageList[imageSlot];
    return image.dataBlock && image.state->fire;
}

bool ShapeBase::isImageReady(U32 imageSlot, U32 ns, U32 depth)
{
    // Will pressing the trigger lead to a fire state?
    MountedImage& image = mMountedImageList[imageSlot];
    if (depth++ > 5 || !image.dataBlock)
        return false;
    ShapeBaseImageData::StateData& stateData = (ns == -1) ?
        *image.state : image.dataBlock->state[ns];
    if (stateData.fire)
        return true;

    // Try the transitions...
    if (stateData.ignoreLoadedForReady == true) {
        if ((ns = stateData.transition.loaded[true]) != -1)
            if (isImageReady(imageSlot, ns, depth))
                return true;
    }
    else {
        if ((ns = stateData.transition.loaded[image.loaded]) != -1)
            if (isImageReady(imageSlot, ns, depth))
                return true;
    }
    if ((ns = stateData.transition.ammo[image.ammo]) != -1)
        if (isImageReady(imageSlot, ns, depth))
            return true;
    if ((ns = stateData.transition.target[image.target]) != -1)
        if (isImageReady(imageSlot, ns, depth))
            return true;
    if ((ns = stateData.transition.wet[image.wet]) != -1)
        if (isImageReady(imageSlot, ns, depth))
            return true;
    if ((ns = stateData.transition.trigger[1]) != -1)
        if (isImageReady(imageSlot, ns, depth))
            return true;
    if ((ns = stateData.transition.timeout) != -1)
        if (isImageReady(imageSlot, ns, depth))
            return true;
    return false;
}

bool ShapeBase::isImageMounted(ShapeBaseImageData* imageData)
{
    for (U32 i = 0; i < MaxMountedImages; i++)
        if (imageData == mMountedImageList[i].dataBlock)
            return true;
    return false;
}

S32 ShapeBase::getMountSlot(ShapeBaseImageData* imageData)
{
    for (U32 i = 0; i < MaxMountedImages; i++)
        if (imageData == mMountedImageList[i].dataBlock)
            return i;
    return -1;
}

StringHandle ShapeBase::getImageSkinTag(U32 imageSlot)
{
    MountedImage& image = mMountedImageList[imageSlot];
    return image.dataBlock ? image.skinNameHandle : StringHandle();
}

const char* ShapeBase::getImageState(U32 imageSlot)
{
    MountedImage& image = mMountedImageList[imageSlot];
    return image.dataBlock ? image.state->name : 0;
}

void ShapeBase::setImageAmmoState(U32 imageSlot, bool ammo)
{
    MountedImage& image = mMountedImageList[imageSlot];
    if (image.dataBlock && !image.dataBlock->usesEnergy && image.ammo != ammo) {
        setMaskBits(ImageMaskN << imageSlot);
        image.ammo = ammo;
    }
}

bool ShapeBase::getImageAmmoState(U32 imageSlot)
{
    MountedImage& image = mMountedImageList[imageSlot];
    if (!image.dataBlock)
        return false;
    return image.ammo;
}

void ShapeBase::setImageWetState(U32 imageSlot, bool wet)
{
    MountedImage& image = mMountedImageList[imageSlot];
    if (image.dataBlock && image.wet != wet) {
        setMaskBits(ImageMaskN << imageSlot);
        image.wet = wet;
    }
}

bool ShapeBase::getImageWetState(U32 imageSlot)
{
    MountedImage& image = mMountedImageList[imageSlot];
    if (!image.dataBlock)
        return false;
    return image.wet;
}

void ShapeBase::setImageLoadedState(U32 imageSlot, bool loaded)
{
    MountedImage& image = mMountedImageList[imageSlot];
    if (image.dataBlock && image.loaded != loaded) {
        setMaskBits(ImageMaskN << imageSlot);
        image.loaded = loaded;
    }
}

bool ShapeBase::getImageLoadedState(U32 imageSlot)
{
    MountedImage& image = mMountedImageList[imageSlot];
    if (!image.dataBlock)
        return false;
    return image.loaded;
}

void ShapeBase::getMuzzleVector(U32 imageSlot, VectorF* vec)
{
    MatrixF mat;
    getMuzzleTransform(imageSlot, &mat);

    MountedImage& image = mMountedImageList[imageSlot];
    if (image.dataBlock->correctMuzzleVector)
        if (GameConnection* gc = getControllingClient())
            if (gc->isFirstPerson() && !gc->isAIControlled())
                if (getCorrectedAim(mat, vec))
                    return;

    mat.getColumn(1, vec);
}

void ShapeBase::getMuzzlePoint(U32 imageSlot, Point3F* pos)
{
    MatrixF mat;
    getMuzzleTransform(imageSlot, &mat);
    mat.getColumn(3, pos);
}


void ShapeBase::getRenderMuzzleVector(U32 imageSlot, VectorF* vec)
{
    MatrixF mat;
    getRenderMuzzleTransform(imageSlot, &mat);

    MountedImage& image = mMountedImageList[imageSlot];
    if (image.dataBlock->correctMuzzleVector)
        if (GameConnection* gc = getControllingClient())
            if (gc->isFirstPerson() && !gc->isAIControlled())
                if (getCorrectedAim(mat, vec))
                    return;

    mat.getColumn(1, vec);
}

void ShapeBase::getRenderMuzzlePoint(U32 imageSlot, Point3F* pos)
{
    MatrixF mat;
    getRenderMuzzleTransform(imageSlot, &mat);
    mat.getColumn(3, pos);
}

//----------------------------------------------------------------------------

void ShapeBase::scriptCallback(U32 imageSlot, const char* function)
{
    MountedImage& image = mMountedImageList[imageSlot];
    char buff1[32];
    dSprintf(buff1, sizeof(buff1), "%d", imageSlot);
    Con::executef(image.dataBlock, 3, function, scriptThis(), buff1);
}


//----------------------------------------------------------------------------

void ShapeBase::getMountTransform(U32 mountPoint, MatrixF* mat)
{
    // Returns mount point to world space transform
    if (mountPoint < ShapeBaseData::NumMountPoints) {
        S32 ni = mDataBlock->mountPointNode[mountPoint];
        if (ni != -1) {
            MatrixF mountTransform = mShapeInstance->mNodeTransforms[ni];
            const Point3F& scale = getScale();

            // The position of the mount point needs to be scaled.
            Point3F position = mountTransform.getPosition();
            position.convolve(scale);
            mountTransform.setPosition(position);

            // Also we would like the object to be scaled to the model.
            mat->mul(mObjToWorld, mountTransform);
            return;
        }
    }
    *mat = mObjToWorld;
}

void ShapeBase::getImageTransform(U32 imageSlot, MatrixF* mat)
{
    // Image transform in world space
    MountedImage& image = mMountedImageList[imageSlot];
    if (image.dataBlock) {
        ShapeBaseImageData& data = *image.dataBlock;

        MatrixF nmat;
        if (data.useEyeOffset && isFirstPerson()) {
            getEyeTransform(&nmat);
            mat->mul(nmat, data.eyeOffset);
        }
        else {
            getMountTransform(image.dataBlock->mountPoint, &nmat);
            mat->mul(nmat, data.mountTransform);
        }
    }
    else
        *mat = mObjToWorld;
}

void ShapeBase::getImageTransform(U32 imageSlot, S32 node, MatrixF* mat)
{
    // Muzzle transform in world space
    MountedImage& image = mMountedImageList[imageSlot];
    if (image.dataBlock) {
        if (node != -1) {
            MatrixF imat;
            getImageTransform(imageSlot, &imat);
            mat->mul(imat, image.shapeInstance->mNodeTransforms[node]);
        }
        else
            getImageTransform(imageSlot, mat);
    }
    else
        *mat = mObjToWorld;
}

void ShapeBase::getImageTransform(U32 imageSlot, StringTableEntry nodeName, MatrixF* mat)
{
    getImageTransform(imageSlot, getNodeIndex(imageSlot, nodeName), mat);
}

void ShapeBase::getMuzzleTransform(U32 imageSlot, MatrixF* mat)
{
    // Muzzle transform in world space
    MountedImage& image = mMountedImageList[imageSlot];
    if (image.dataBlock)
        getImageTransform(imageSlot, image.dataBlock->muzzleNode, mat);
    else
        *mat = mObjToWorld;
}


//----------------------------------------------------------------------------

void ShapeBase::getRenderMountTransform(U32 mountPoint, MatrixF* mat)
{
    // Returns mount point to world space transform
    if (mountPoint < ShapeBaseData::NumMountPoints) {
        S32 ni = mDataBlock->mountPointNode[mountPoint];
        if (ni != -1) {
            MatrixF mountTransform = mShapeInstance->mNodeTransforms[ni];
            const Point3F& scale = getScale();

            // The position of the mount point needs to be scaled.
            Point3F position = mountTransform.getPosition();
            position.convolve(scale);
            mountTransform.setPosition(position);

            // Also we would like the object to be scaled to the model.
            mountTransform.scale(scale);
            mat->mul(getRenderTransform(), mountTransform);
            return;
        }
    }
    *mat = getRenderTransform();
}


void ShapeBase::getRenderImageTransform(U32 imageSlot, MatrixF* mat)
{
    // Image transform in world space
    MountedImage& image = mMountedImageList[imageSlot];
    if (image.dataBlock) {
        ShapeBaseImageData& data = *image.dataBlock;

        MatrixF nmat;
        if (data.useEyeOffset && isFirstPerson()) {
            getRenderEyeTransform(&nmat);
            mat->mul(nmat, data.eyeOffset);
        }
        else {
            getRenderMountTransform(data.mountPoint, &nmat);
            mat->mul(nmat, data.mountTransform);
        }
    }
    else
        *mat = getRenderTransform();
}

void ShapeBase::getRenderImageTransform(U32 imageSlot, S32 node, MatrixF* mat)
{
    // Muzzle transform in world space
    MountedImage& image = mMountedImageList[imageSlot];
    if (image.dataBlock) {
        if (node != -1) {
            MatrixF imat;
            getRenderImageTransform(imageSlot, &imat);
            mat->mul(imat, image.shapeInstance->mNodeTransforms[node]);
        }
        else
            getRenderImageTransform(imageSlot, mat);
    }
    else
        *mat = getRenderTransform();
}

void ShapeBase::getRenderImageTransform(U32 imageSlot, StringTableEntry nodeName, MatrixF* mat)
{
    getRenderImageTransform(imageSlot, getNodeIndex(imageSlot, nodeName), mat);
}

void ShapeBase::getRenderMuzzleTransform(U32 imageSlot, MatrixF* mat)
{
    // Muzzle transform in world space
    MountedImage& image = mMountedImageList[imageSlot];
    if (image.dataBlock)
        getRenderImageTransform(imageSlot, image.dataBlock->muzzleNode, mat);
    else
        *mat = getRenderTransform();
}


void ShapeBase::getRetractionTransform(U32 imageSlot, MatrixF* mat)
{
    // Muzzle transform in world space
    MountedImage& image = mMountedImageList[imageSlot];
    if (image.dataBlock) {
        if (image.dataBlock->retractNode != -1)
            getImageTransform(imageSlot, image.dataBlock->retractNode, mat);
        else
            getImageTransform(imageSlot, image.dataBlock->muzzleNode, mat);
    }
    else {
        *mat = getTransform();
    }
}


void ShapeBase::getRenderRetractionTransform(U32 imageSlot, MatrixF* mat)
{
    // Muzzle transform in world space
    MountedImage& image = mMountedImageList[imageSlot];
    if (image.dataBlock) {
        if (image.dataBlock->retractNode != -1)
            getRenderImageTransform(imageSlot, image.dataBlock->retractNode, mat);
        else
            getRenderImageTransform(imageSlot, image.dataBlock->muzzleNode, mat);
    }
    else {
        *mat = getRenderTransform();
    }
}


//----------------------------------------------------------------------------

S32 ShapeBase::getNodeIndex(U32 imageSlot, StringTableEntry nodeName)
{
    MountedImage& image = mMountedImageList[imageSlot];
    if (image.dataBlock)
        return image.dataBlock->shape->findNode(nodeName);
    else
        return -1;
}

// Modify muzzle if needed to aim at whatever is straight in front of eye.  Let the
// caller know if we actually modified the result.
bool ShapeBase::getCorrectedAim(const MatrixF& muzzleMat, VectorF* result)
{
    const F32 pullInD = 6.0;
    const F32 maxAdjD = 500;

    VectorF  aheadVec(0, maxAdjD, 0);

    MatrixF  eyeMat;
    Point3F  eyePos;
    getEyeTransform(&eyeMat);
    eyeMat.getColumn(3, &eyePos);
    eyeMat.mulV(aheadVec);
    Point3F  aheadPoint = (eyePos + aheadVec);

    // Should we check if muzzle point is really close to eye?  Does that happen?
    Point3F  muzzlePos;
    muzzleMat.getColumn(3, &muzzlePos);

    Point3F  collidePoint;
    VectorF  collideVector;
    disableCollision();
    RayInfo rinfo;
    if (getContainer()->castRay(eyePos, aheadPoint, 0xFFFFFFFF, &rinfo))
        collideVector = ((collidePoint = rinfo.point) - eyePos);
    else
        collideVector = ((collidePoint = aheadPoint) - eyePos);
    enableCollision();

    // For close collision we want to NOT aim at ground since we're bending
    // the ray here as it is.  But we don't want to pop, so adjust continuously.
    F32   lenSq = collideVector.lenSquared();
    if (lenSq < (pullInD * pullInD) && lenSq > 0.04)
    {
        F32   len = mSqrt(lenSq);
        F32   mid = pullInD;    // (pullInD + len) / 2.0;
        // This gives us point beyond to focus on-
        collideVector *= (mid / len);
        collidePoint = (eyePos + collideVector);
    }

    VectorF  muzzleToCollide = (collidePoint - muzzlePos);
    lenSq = muzzleToCollide.lenSquared();
    if (lenSq > 0.04)
    {
        muzzleToCollide *= (1 / mSqrt(lenSq));
        *result = muzzleToCollide;
        return true;
    }
    return false;
}

//----------------------------------------------------------------------------

void ShapeBase::updateMass()
{
    if (mDataBlock) {
        F32 imass = 0;
        for (U32 i = 0; i < MaxMountedImages; i++) {
            MountedImage& image = mMountedImageList[i];
            if (image.dataBlock)
                imass += image.dataBlock->mass;
        }
        //
        mMass = mDataBlock->mass + imass;
        mOneOverMass = 1 / mMass;
    }
}

void ShapeBase::onImageRecoil(U32, ShapeBaseImageData::StateData::RecoilState)
{
}


//----------------------------------------------------------------------------

void ShapeBase::setImage(U32 imageSlot, ShapeBaseImageData* imageData, StringHandle& skinNameHandle, bool loaded, bool ammo, bool triggerDown, bool target)
{
    AssertFatal(imageSlot < MaxMountedImages, "Out of range image slot");

    MountedImage& image = mMountedImageList[imageSlot];

    // If we already have this datablock...
    if (image.dataBlock == imageData) {
        // Mark that there is not a datablock change pending.
        image.nextImage = InvalidImagePtr;
        // Change the skin handle if necessary.
        if (image.skinNameHandle != skinNameHandle) {
            if (!isGhost()) {
                // Serverside, note the skin handle and tell the client.
                image.skinNameHandle = skinNameHandle;
                setMaskBits(ImageMaskN << imageSlot);
            }
            else {
                // Clientside, do the reskin.
                image.skinNameHandle = skinNameHandle;
                if (image.shapeInstance) {
                    image.shapeInstance->reSkin(skinNameHandle);
                }
            }
        }
        return;
    }

    // Check to see if we need to delay image changes until state change.
    if (!isGhost()) {
        if (imageData && image.dataBlock && !image.state->allowImageChange) {
            image.nextImage = imageData;
            image.nextSkinNameHandle = skinNameHandle;
            image.nextLoaded = loaded;
            return;
        }
    }

    // Mark that updates are happenin'.
    setMaskBits(ImageMaskN << imageSlot);

    // Notify script unmount since we're swapping datablocks.
    if (image.dataBlock && !isGhost()) {
        scriptCallback(imageSlot, "onUnmount");
    }

    // Stop anything currently going on with the image.
    resetImageSlot(imageSlot);

    // If we're just unselecting the current shape without swapping
    // in a new one, then bail.
    if (!imageData) {
        return;
    }

    // Otherwise, init the new shape.
    image.dataBlock = imageData;
    image.state = &image.dataBlock->state[0];
    image.skinNameHandle = skinNameHandle;
    image.shapeInstance = new TSShapeInstance(image.dataBlock->shape, isClientObject());
    if (isClientObject() || gSPMode) {
        if (image.shapeInstance) {
            image.shapeInstance->cloneMaterialList();
            image.shapeInstance->reSkin(skinNameHandle);
        }
    }
    image.loaded = loaded;
    image.ammo = ammo;
    image.triggerDown = triggerDown;
    image.target = target;

    // The server needs the shape loaded for muzzle mount nodes
    // but it doesn't need to run any of the animations.
    image.ambientThread = 0;
    image.animThread = 0;
    image.flashThread = 0;
    image.spinThread = 0;
    if (isGhost() || gSPMode) {
        if (image.dataBlock->isAnimated) {
            image.animThread = image.shapeInstance->addThread();
            image.shapeInstance->setTimeScale(image.animThread, 0);
        }
        if (image.dataBlock->hasFlash) {
            image.flashThread = image.shapeInstance->addThread();
            image.shapeInstance->setTimeScale(image.flashThread, 0);
        }
        if (image.dataBlock->ambientSequence != -1) {
            image.ambientThread = image.shapeInstance->addThread();
            image.shapeInstance->setTimeScale(image.ambientThread, 1);
            image.shapeInstance->setSequence(image.ambientThread,
                image.dataBlock->ambientSequence, 0);
        }
        if (image.dataBlock->spinSequence != -1) {
            image.spinThread = image.shapeInstance->addThread();
            image.shapeInstance->setTimeScale(image.spinThread, 1);
            image.shapeInstance->setSequence(image.spinThread,
                image.dataBlock->spinSequence, 0);
        }
    }

    // Set the image to its starting state.
    setImageState(imageSlot, 0, true);

    // Update the mass for the mount object.
    updateMass();

    // Notify script mount.
    if (!isGhost()) {
        scriptCallback(imageSlot, "onMount");
    }

    // Done.
}


//----------------------------------------------------------------------------

void ShapeBase::resetImageSlot(U32 imageSlot)
{
    AssertFatal(imageSlot < MaxMountedImages, "Out of range image slot");

    // Clear out current image
    MountedImage& image = mMountedImageList[imageSlot];
    delete image.shapeInstance;
    image.shapeInstance = 0;
    SFX_DELETE(image.animSound);
    for (S32 i = 0; i < MaxImageEmitters; i++) {
        MountedImage::ImageEmitter& em = image.emitter[i];
        if (bool(em.emitter)) {
            em.emitter->deleteWhenEmpty();
            em.emitter = 0;
        }
    }
    image.dataBlock = 0;
    image.nextImage = InvalidImagePtr;
    image.skinNameHandle = StringHandle();
    image.nextSkinNameHandle = StringHandle();
    image.state = 0;
    image.delayTime = 0;
    image.ammo = false;
    image.triggerDown = false;
    image.loaded = false;
    image.lightStart = 0;
    //   image.light.fLight.fType = TSLight::LightInvalid;
    updateMass();
}


//----------------------------------------------------------------------------

bool ShapeBase::getImageTriggerState(U32 imageSlot)
{
    if (isGhost() || !mMountedImageList[imageSlot].dataBlock)
        return false;
    return mMountedImageList[imageSlot].triggerDown;
}

void ShapeBase::setImageTriggerState(U32 imageSlot, bool trigger)
{
    if (isGhost() || !mMountedImageList[imageSlot].dataBlock)
        return;
    MountedImage& image = mMountedImageList[imageSlot];

    if (trigger) {
        if (!image.triggerDown && image.dataBlock) {
            image.triggerDown = true;
            if (!isGhost()) {
                setMaskBits(ImageMaskN << imageSlot);
                updateImageState(imageSlot, 0);
            }
        }
    }
    else
        if (image.triggerDown) {
            image.triggerDown = false;
            if (!isGhost()) {
                setMaskBits(ImageMaskN << imageSlot);
                updateImageState(imageSlot, 0);
            }
        }
}


//----------------------------------------------------------------------------

U32 ShapeBase::getImageFireState(U32 imageSlot)
{
    MountedImage& image = mMountedImageList[imageSlot];
    // If there is no fire state, then try state 0
    if (image.dataBlock && image.dataBlock->fireState != -1)
        return image.dataBlock->fireState;
    return 0;
}


//----------------------------------------------------------------------------

void ShapeBase::setImageState(U32 imageSlot, U32 newState, bool force)
{
    if (!mMountedImageList[imageSlot].dataBlock)
        return;
    MountedImage& image = mMountedImageList[imageSlot];


    // The client never enters the initial fire state on its own, but it
    //  will continue to set that state...
    if (isGhost() && !force && newState == image.dataBlock->fireState) {
        if (image.state != &image.dataBlock->state[newState])
            return;
    }

    // Eject shell casing on every state change
    ShapeBaseImageData::StateData& nextStateData = image.dataBlock->state[newState];
    if ((isGhost() || gSPMode) && nextStateData.ejectShell) {
        ejectShellCasing(imageSlot);
    }


    // Server must animate the shape if it is a firestate...
    if (newState == image.dataBlock->fireState && isServerObject())
        mShapeInstance->animate();

    // If going back into the same state, just reset the timer
    // and invoke the script callback
    if (!force && image.state == &image.dataBlock->state[newState]) {
        image.delayTime = image.state->timeoutValue;
        if (image.state->script && !isGhost())
            scriptCallback(imageSlot, image.state->script);

        // If this is a flash sequence, we need to select a new position for the
        //  animation if we're returning to that state...
        if (image.animThread && image.state->sequence != -1 && image.state->flashSequence) {
            F32 randomPos = Platform::getRandom();
            image.shapeInstance->setPos(image.animThread, randomPos);
            image.shapeInstance->setTimeScale(image.animThread, 0);
            if (image.flashThread)
                image.shapeInstance->setPos(image.flashThread, 0);
        }

        return;
    }

    F32 lastDelay = image.delayTime;
    ShapeBaseImageData::StateData& lastState = *image.state;
    image.state = &image.dataBlock->state[newState];

    //
    // Do state cleanup first...
    //
    ShapeBaseImageData& imageData = *image.dataBlock;
    ShapeBaseImageData::StateData& stateData = *image.state;

    // Stop any looping sounds or animations use in the last state.
    SFX_DELETE(image.animSound);

    // Mount pending images
    if (image.nextImage != InvalidImagePtr && stateData.allowImageChange) {
        setImage(imageSlot, image.nextImage, image.nextSkinNameHandle, image.nextLoaded);
        return;
    }

    // Reset cyclic sequences back to the first frame to turn it off
    // (the first key frame should be it's off state).
    if (image.animThread && image.dataBlock->shape->sequences[image.shapeInstance->getSequence(image.animThread)].isCyclic()) {
        image.shapeInstance->setPos(image.animThread, 0);
        image.shapeInstance->setTimeScale(image.animThread, 0);
    }
    if (image.flashThread) {
        image.shapeInstance->setPos(image.flashThread, 0);
        image.shapeInstance->setTimeScale(image.flashThread, 0);
    }

    // Check for immediate transitions
    S32 ns;
    if ((ns = stateData.transition.loaded[image.loaded]) != -1) {
        setImageState(imageSlot, ns);
        return;
    }
    //if (!imageData.usesEnergy)
    if ((ns = stateData.transition.ammo[image.ammo]) != -1) {
        setImageState(imageSlot, ns);
        return;
    }
    if ((ns = stateData.transition.target[image.target]) != -1) {
        setImageState(imageSlot, ns);
        return;
    }
    if ((ns = stateData.transition.wet[image.wet]) != -1) {
        setImageState(imageSlot, ns);
        return;
    }
    if ((ns = stateData.transition.trigger[image.triggerDown]) != -1) {
        setImageState(imageSlot, ns);
        return;
    }

    //
    // Initialize the new state...
    //
    image.delayTime = stateData.timeoutValue;
    if (stateData.loaded != ShapeBaseImageData::StateData::IgnoreLoaded)
        image.loaded = stateData.loaded == ShapeBaseImageData::StateData::Loaded;
    if (!isGhost() && newState == imageData.fireState) {
        setMaskBits(ImageMaskN << imageSlot);
        image.fireCount = (image.fireCount + 1) & 0x7;
    }

    // Apply recoil
    if (stateData.recoil != ShapeBaseImageData::StateData::NoRecoil)
        onImageRecoil(imageSlot, stateData.recoil);

    // Play sound
    if (stateData.sound && (isGhost() || gSPMode)) {
        const Point3F& velocity = getVelocity();
        image.animSound = SFX->createSource(stateData.sound, &getRenderTransform(), &velocity);

        // If we have the source... play it.
        if (image.animSound)
        {
            image.animLoopingSound = image.animSound->isLooping();
            image.animSound->play();
        }
    }

    // Play animation
    if (image.animThread && stateData.sequence != -1) {
        image.shapeInstance->setSequence(image.animThread, stateData.sequence,
            stateData.direction ? 0 : 1);
        if (stateData.flashSequence == false) {
            F32 timeScale = (stateData.scaleAnimation && stateData.timeoutValue) ?
                image.shapeInstance->getDuration(image.animThread) / stateData.timeoutValue :
                1;
            image.shapeInstance->setTimeScale(image.animThread, stateData.direction ? timeScale :
                -timeScale);
        }
        else {
            F32 randomPos = Platform::getRandom();
            image.shapeInstance->setPos(image.animThread, randomPos);
            image.shapeInstance->setTimeScale(image.animThread, 0);

            image.shapeInstance->setSequence(image.flashThread, stateData.sequenceVis, 0);
            image.shapeInstance->setPos(image.flashThread, 0);
            F32 timeScale = (stateData.scaleAnimation && stateData.timeoutValue) ?
                image.shapeInstance->getDuration(image.animThread) / stateData.timeoutValue :
                1;
            image.shapeInstance->setTimeScale(image.flashThread, timeScale);
        }
    }

    // Start particle emitter on the client
    if (isGhost() || gSPMode)
        startImageEmitter(image, stateData);

    // Start spin thread
    if (image.spinThread) {
        switch (stateData.spin) {
        case ShapeBaseImageData::StateData::IgnoreSpin:
            image.shapeInstance->setTimeScale(image.spinThread, image.shapeInstance->getTimeScale(image.spinThread));
            break;
        case ShapeBaseImageData::StateData::NoSpin:
            image.shapeInstance->setTimeScale(image.spinThread, 0);
            break;
        case ShapeBaseImageData::StateData::SpinUp:
            if (lastState.spin == ShapeBaseImageData::StateData::SpinDown)
                image.delayTime *= 1.0f - (lastDelay / stateData.timeoutValue);
            break;
        case ShapeBaseImageData::StateData::SpinDown:
            if (lastState.spin == ShapeBaseImageData::StateData::SpinUp)
                image.delayTime *= 1.0f - (lastDelay / stateData.timeoutValue);
            break;
        case ShapeBaseImageData::StateData::FullSpin:
            image.shapeInstance->setTimeScale(image.spinThread, 1);
            break;
        }
    }

    // Script callback on server
    if (stateData.script && stateData.script[0] && !isGhost())
        scriptCallback(imageSlot, stateData.script);

    // If there is a zero timeout, and a timeout transition, then
    // go ahead and transition imediately.
    if (!image.delayTime)
    {
        if ((ns = stateData.transition.timeout) != -1)
        {
            setImageState(imageSlot, ns);
            return;
        }
    }
}


//----------------------------------------------------------------------------

void ShapeBase::updateImageState(U32 imageSlot, F32 dt)
{
    if (!mMountedImageList[imageSlot].dataBlock)
        return;
    MountedImage& image = mMountedImageList[imageSlot];
    ShapeBaseImageData& imageData = *image.dataBlock;
    ShapeBaseImageData::StateData& stateData = *image.state;
    image.delayTime -= dt;

    // Energy management
    if (imageData.usesEnergy) {
        F32 newEnergy = getEnergyLevel() - stateData.energyDrain * dt;
        if (newEnergy < 0)
            newEnergy = 0;
        setEnergyLevel(newEnergy);

        if (!isGhost()) {
            bool ammo = newEnergy > imageData.minEnergy;
            if (ammo != image.ammo) {
                setMaskBits(ImageMaskN << imageSlot);
                image.ammo = ammo;
            }
        }
    }

    // Check for transitions. On some states we must wait for the
    // full timeout value before moving on.
    S32 ns;
    if (image.delayTime <= 0 || !stateData.waitForTimeout) {
        if ((ns = stateData.transition.loaded[image.loaded]) != -1) {
            setImageState(imageSlot, ns);
            return;
        }
        if ((ns = stateData.transition.ammo[image.ammo]) != -1) {
            setImageState(imageSlot, ns);
            return;
        }
        if ((ns = stateData.transition.target[image.target]) != -1) {
            setImageState(imageSlot, ns);
            return;
        }
        if ((ns = stateData.transition.wet[image.wet]) != -1) {
            setImageState(imageSlot, ns);
            return;
        }
        if ((ns = stateData.transition.trigger[image.triggerDown]) != -1) {
            setImageState(imageSlot, ns);
            return;
        }
        if (image.delayTime <= 0 &&
            (ns = stateData.transition.timeout) != -1) {
            setImageState(imageSlot, ns);
            return;
        }
    }

    // Update the spinning thread timeScale
    if (image.spinThread) {
        float timeScale;

        switch (stateData.spin) {
        case ShapeBaseImageData::StateData::IgnoreSpin:
        case ShapeBaseImageData::StateData::NoSpin:
        case ShapeBaseImageData::StateData::FullSpin: {
            timeScale = 0;
            image.shapeInstance->setTimeScale(image.spinThread, image.shapeInstance->getTimeScale(image.spinThread));
            break;
        }

        case ShapeBaseImageData::StateData::SpinUp: {
            timeScale = 1.0f - image.delayTime / stateData.timeoutValue;
            image.shapeInstance->setTimeScale(image.spinThread, timeScale);
            break;
        }

        case ShapeBaseImageData::StateData::SpinDown: {
            timeScale = image.delayTime / stateData.timeoutValue;
            image.shapeInstance->setTimeScale(image.spinThread, timeScale);
            break;
        }
        }
    }
}


//----------------------------------------------------------------------------

void ShapeBase::updateImageAnimation(U32 imageSlot, F32 dt)
{
    if (!mMountedImageList[imageSlot].dataBlock)
        return;
    MountedImage& image = mMountedImageList[imageSlot];

    // Advance animation threads
    if (image.ambientThread)
        image.shapeInstance->advanceTime(dt, image.ambientThread);
    if (image.animThread)
        image.shapeInstance->advanceTime(dt, image.animThread);
    if (image.spinThread)
        image.shapeInstance->advanceTime(dt, image.spinThread);
    if (image.flashThread)
        image.shapeInstance->advanceTime(dt, image.flashThread);

    // Update any playing sound.
    if (image.animSound)
    {
        image.animSound->setTransform(getRenderTransform());
        // TODO: Add velocity here?
    }

    // Particle emission
    for (S32 i = 0; i < MaxImageEmitters; i++) {
        MountedImage::ImageEmitter& em = image.emitter[i];
        if (bool(em.emitter)) {
            if (em.time > 0) {
                em.time -= dt;

                MatrixF mat;
                getRenderImageTransform(imageSlot, em.node, &mat);
                Point3F pos, axis;
                mat.getColumn(3, &pos);
                mat.getColumn(1, &axis);
                em.emitter->emitParticles(pos, true, axis, getVelocity(), (U32)(dt * 1000));
            }
            else {
                em.emitter->deleteWhenEmpty();
                em.emitter = 0;
            }
        }
    }
}


//----------------------------------------------------------------------------

void ShapeBase::startImageEmitter(MountedImage& image, ShapeBaseImageData::StateData& state)
{
    MountedImage::ImageEmitter* bem = 0;
    MountedImage::ImageEmitter* em = image.emitter;
    MountedImage::ImageEmitter* ee = &image.emitter[MaxImageEmitters];

    // If we are already emitting the same particles from the same
    // node, then simply extend the time.  Otherwise, find an empty
    // emitter slot, or grab the one with the least amount of time left.
    for (; em != ee; em++) {
        if (bool(em->emitter)) {
            if (state.emitter == em->emitter->getDataBlock() && state.emitterNode == em->node) {
                if (state.emitterTime > em->time)
                    em->time = state.emitterTime;
                return;
            }
            if (!bem || (bool(bem->emitter) && bem->time > em->time))
                bem = em;
        }
        else
            bem = em;
    }

    bem->time = state.emitterTime;
    bem->node = state.emitterNode;
    bem->emitter = new ParticleEmitter;
    bem->emitter->onNewDataBlock(state.emitter);
    if (!bem->emitter->registerObject())
        delete bem->emitter;
}


//----------------------------------------------------------------------------

Light* ShapeBase::getImageLight(U32 imageSlot)
{
    MountedImage& image = mMountedImageList[imageSlot];
    if (!image.dataBlock)
        return 0;

    ShapeBaseImageData& imageData = *image.dataBlock;
    if (imageData.lightType == ShapeBaseImageData::NoLight)
        return 0;

    F32 intensity;
    F32 delta = Sim::getCurrentTime() - image.lightStart;
    switch (imageData.lightType) {
    case ShapeBaseImageData::ConstantLight:
        intensity = 1.0;
        break;
    case ShapeBaseImageData::WeaponFireLight: {
        if (delta > imageData.lightTime)
            return 0;
        intensity = 1.0 - delta / imageData.lightTime;
        break;
    }
    case ShapeBaseImageData::PulsingLight: {
        intensity = 0.5 + 0.5 * mSin(M_PI * delta / imageData.lightTime);
        intensity = 0.15 + intensity * 0.85;
        break;
    }
    default:
        return 0;
    }

    // If there is no muzzle node on the shape getMuzzleTransform
    // returns the image origin.  So should work fine for either
    // fire or pulsing lights.
    MatrixF mat;
    getMuzzleTransform(imageSlot, &mat);
    return 0;
}

void ShapeBase::registerLights(LightManager* lightManager, bool lightingScene)
{
    //one of the mounted images must have a light source...
    for (S32 i = 0; i < MaxMountedImages; i++)
    {
        ShapeBaseImageData* imageData = getMountedImage(i);
        //      if (imageData != NULL && imageData->lightType != ShapeBaseImageData::NoLight)
        //         imageData->registerImageLights(lightManager, lightingScene, getRenderPosition(), mLightTime);
    }
}


//----------------------------------------------------------------------------

void ShapeBase::ejectShellCasing(U32 imageSlot)
{
    MountedImage& image = mMountedImageList[imageSlot];
    ShapeBaseImageData* imageData = image.dataBlock;

    if (!imageData->casing)
        return;

    MatrixF ejectTrans;
    getImageTransform(imageSlot, imageData->ejectNode, &ejectTrans);

    Point3F ejectDir = imageData->shellExitDir;
    ejectDir.normalize();

    F32 ejectSpread = mDegToRad(imageData->shellExitVariance);
    MatrixF ejectOrient = MathUtils::createOrientFromDir(ejectDir);

    Point3F randomDir;
    randomDir.x = mSin(gRandGen.randF(-ejectSpread, ejectSpread));
    randomDir.y = 1.0;
    randomDir.z = mSin(gRandGen.randF(-ejectSpread, ejectSpread));
    randomDir.normalizeSafe();

    ejectOrient.mulV(randomDir);

    MatrixF imageTrans = getTransform();
    imageTrans.mulV(randomDir);

    Point3F shellVel = randomDir * imageData->shellVelocity;
    Point3F shellPos = ejectTrans.getPosition();


    Debris* casing = new Debris;
    casing->onNewDataBlock(imageData->casing);
    casing->setTransform(imageTrans);

    if (!casing->registerObject())
        delete casing;

    casing->init(shellPos, shellVel);
}
