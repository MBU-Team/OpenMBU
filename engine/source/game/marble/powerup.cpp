//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "game/marble/powerup.h"

//----------------------------------------------------------------------------

PowerUpData::ActiveParams::ActiveParams()
{
    init();
}

void PowerUpData::ActiveParams::init()
{
    sizeScale = 1.0f;
    massScale = 1.0f;
    gravityMod = 1.0f;
    airAccel = 1.0f;
    bounce = -1.0f;
    repulseDist = 0.0f;
    repulseMax = 0.0f;
}

//----------------------------------------------------------------------------

IMPLEMENT_CO_DATABLOCK_V1(PowerUpData);

PowerUpData::PowerUpData()
{
    for (int i = 0; i < MaxPowerUps; i++)
    {
        duration[i] = 5000;
        airAccel[i] = 1.0f;
        activateTime[i] = 0;
        image[i] = NULL;
        bounce[i] = -1.0f;
        emitter[i] = NULL;
        sizeScale[i] = 1.0f;
        massScale[i] = 1.0f;
        gravityMod[i] = 1.0f;
        repulseDist[i] = 0.0f;
        repulseMax[i] = 0.0f;
        blastRecharge[i] = false;
        boostDir[i] = Point3F(0, 0, 0);
        timeFreeze[i] = 0;
        boostAmount[i] = 0.0f;
        boostMassless[i] = 0.0f;
    }
}

void PowerUpData::initPersistFields()
{
    Parent::initPersistFields();

    addField("emitter", TypeParticleEmitterDataPtr, Offset(emitter, PowerUpData), MaxPowerUps);
    addField("duration", TypeS32, Offset(duration, PowerUpData), MaxPowerUps);
    addField("activateTime", TypeS32, Offset(activateTime, PowerUpData), MaxPowerUps);
    addField("image", TypeGameBaseDataPtr, Offset(image, PowerUpData), MaxPowerUps);
    addField("boostDir", TypePoint3F, Offset(boostDir, PowerUpData), MaxPowerUps);
    addField("boostAmount", TypeF32, Offset(boostAmount, PowerUpData), MaxPowerUps);
    addField("boostMassless", TypeF32, Offset(boostMassless, PowerUpData), MaxPowerUps);
    addField("timeFreeze", TypeS32, Offset(timeFreeze, PowerUpData), MaxPowerUps);
    addField("blastRecharge", TypeBool, Offset(blastRecharge, PowerUpData), MaxPowerUps);
    addField("airAccel", TypeF32, Offset(airAccel, PowerUpData), MaxPowerUps);
    addField("gravityMod", TypeF32, Offset(gravityMod, PowerUpData), MaxPowerUps);
    addField("bounce", TypeF32, Offset(bounce, PowerUpData), MaxPowerUps);
    addField("repulseMax", TypeF32, Offset(repulseMax, PowerUpData), MaxPowerUps);
    addField("repulseDist", TypeF32, Offset(repulseDist, PowerUpData), MaxPowerUps);
    addField("massScale", TypeF32, Offset(massScale, PowerUpData), MaxPowerUps);
    addField("sizeScale", TypeF32, Offset(sizeScale, PowerUpData), MaxPowerUps);
}

//----------------------------------------------------------------------------

bool PowerUpData::preload(bool server, char errorBuffer[256])
{
    bool result = Parent::preload(server, errorBuffer);

    for (int i = 0; i < MaxPowerUps; i++)
    {
        // this is not in mbo but the loop is, so they most likely had this line and commented it.
        //image[i]->preload(server, errorBuffer);
    }

    return result;
}

//----------------------------------------------------------------------------

void PowerUpData::packData(BitStream* stream)
{
    // empty
}

void PowerUpData::unpackData(BitStream* stream)
{
    // empty
}
