//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _POWERUP_H_
#define _POWERUP_H_

#ifndef _GAMEBASE_H_
#include "game/gameBase.h"
#endif

#ifndef _SHAPEBASE_H_
#include "game/shapeBase.h"
#endif

class PowerUpData : public GameBaseData
{
private:
    typedef GameBaseData Parent;

public:
    enum
    {
        MaxPowerUps = 10,
    };

    struct ActiveParams
    {
        F32 airAccel;
        F32 gravityMod;
        F32 bounce;
        F32 repulseMax;
        F32 repulseDist;
        F32 massScale;
        F32 sizeScale;

        ActiveParams();
        void init();
    };

    U32 duration[MaxPowerUps];
    U32 activateTime[MaxPowerUps];
    ShapeBaseImageData* image[MaxPowerUps];
    ParticleEmitterData* emitter[MaxPowerUps];
    Point3F boostDir[MaxPowerUps];
    F32 boostAmount[MaxPowerUps];
    F32 boostMassless[MaxPowerUps];
    U32 timeFreeze[MaxPowerUps];
    F32 airAccel[MaxPowerUps];
    F32 gravityMod[MaxPowerUps];
    F32 bounce[MaxPowerUps];
    F32 repulseMax[MaxPowerUps];
    F32 repulseDist[MaxPowerUps];
    bool blastRecharge[MaxPowerUps];
    F32 massScale[MaxPowerUps];
    F32 sizeScale[MaxPowerUps];

public:
    DECLARE_CONOBJECT(PowerUpData);

    PowerUpData();
    static void initPersistFields();

    virtual void packData(BitStream* stream);
    virtual void unpackData(BitStream* stream);
    bool preload(bool server, char errorBuffer[256]);
};

#endif // _POWERUP_H_