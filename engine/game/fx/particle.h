//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _PARTICLE_H_
#define _PARTICLE_H_

#ifndef _GAMEBASE_H_
#include "game/gameBase.h"
#endif

#include "gfx/gfxTextureHandle.h"

#define MaxParticleSize 50.0

struct Particle;

//*****************************************************************************
// Particle Data
//*****************************************************************************
class ParticleData : public SimDataBlock
{
    typedef SimDataBlock Parent;

public:
    enum PDConst
    {
        PDC_MAX_TEX = 50,
        PDC_NUM_KEYS = 4,
    };

    F32   dragCoefficient;
    F32   windCoefficient;
    F32   gravityCoefficient;

    F32   inheritedVelFactor;
    F32   constantAcceleration;

    S32   lifetimeMS;
    S32   lifetimeVarianceMS;

    F32   spinSpeed;        // degrees per second
    F32   spinRandomMin;
    F32   spinRandomMax;

    bool  useInvAlpha;

    bool  animateTexture;
    U32   numFrames;
    U32   framesPerSec;

    ColorF colors[PDC_NUM_KEYS];
    F32    sizes[PDC_NUM_KEYS];
    F32    times[PDC_NUM_KEYS];

    StringTableEntry  textureNameList[PDC_MAX_TEX];
    GFXTexHandle      textureList[PDC_MAX_TEX];

public:
    ParticleData();
    ~ParticleData();

    // move this procedure to Particle
    void initializeParticle(Particle*, const Point3F&);

    void packData(BitStream* stream);
    void unpackData(BitStream* stream);
    bool onAdd();
    bool preload(bool server, char errorBuffer[256]);
    DECLARE_CONOBJECT(ParticleData);
    static void  initPersistFields();
};


//*****************************************************************************
// Particle
// 
// This structure should be as small as possible.
//*****************************************************************************
struct Particle
{
    Point3F  pos;     // current instantaneous position
    Point3F  vel;     //   "         "         velocity
    Point3F  acc;     // Constant acceleration
    Point3F  orientDir;  // direction particle should go if using oriented particles

    U32           totalLifetime;   // Total ms that this instance should be "live"
    ParticleData* dataBlock;       // datablock that contains global parameters for
                                   //  this instance
    U32       currentAge;


    // are these necessary to store here? - they are interpolated in real time
    ColorF           color;
    F32              size;

    F32              spinSpeed;
};


#endif // _PARTICLE_H_