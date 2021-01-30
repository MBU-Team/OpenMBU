//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _DEBRIS_H_
#define _DEBRIS_H_

#ifndef _GAMEBASE_H_
#include "game/gameBase.h"
#endif

class ParticleEmitterData;
class ParticleEmitter;
class ExplosionData;
class TSPartInstance;
class TSShapeInstance;
class TSShape;

//**************************************************************************
// Debris Data
//**************************************************************************
struct DebrisData : public GameBaseData
{
    typedef GameBaseData Parent;

    //-----------------------------------------------------------------------
    // Data Decs
    //-----------------------------------------------------------------------
    enum DebrisDataConst
    {
        DDC_NUM_EMITTERS = 2,
    };


    //-----------------------------------------------------------------------
    // Debris datablock
    //-----------------------------------------------------------------------
    F32      velocity;
    F32      velocityVariance;
    F32      friction;
    F32      elasticity;
    F32      lifetime;
    F32      lifetimeVariance;
    U32      numBounces;
    U32      bounceVariance;
    F32      minSpinSpeed;
    F32      maxSpinSpeed;
    bool     render2D;
    bool     explodeOnMaxBounce;  // explodes after it has bounced max times
    bool     staticOnMaxBounce;   // becomes static after bounced max times
    bool     snapOnMaxBounce;     // snap into a "resting" position on last bounce
    bool     fade;
    bool     useRadiusMass;       // use mass calculations based on radius
    F32      baseRadius;          // radius at which the standard elasticity and friction apply
    F32      gravModifier;        // how much gravity affects debris
    F32      terminalVelocity;    // max velocity magnitude
    bool     ignoreWater;

    const char* shapeName;
    Resource<TSShape> shape;

    StringTableEntry  textureName;
    //   TextureHandle     texture;


    S32                     explosionId;
    ExplosionData* explosion;
    ParticleEmitterData* emitterList[DDC_NUM_EMITTERS];
    S32                     emitterIDList[DDC_NUM_EMITTERS];

    DebrisData();

    bool        onAdd();
    bool        preload(bool server, char errorBuffer[256]);
    static void initPersistFields();
    void        packData(BitStream* stream);
    void        unpackData(BitStream* stream);

    DECLARE_CONOBJECT(DebrisData);

};
DECLARE_CONSOLETYPE(DebrisData)

//**************************************************************************
// Debris
//**************************************************************************
class Debris : public GameBase
{
    typedef GameBase Parent;

private:
    S32               mNumBounces;
    F32               mSize;
    Point3F           mLastPos;
    Point3F           mVelocity;
    F32               mLifetime;
    DebrisData* mDataBlock;
    F32               mElapsedTime;
    TSShapeInstance* mShape;
    TSPartInstance* mPart;
    MatrixF           mInitialTrans;
    F32               mXRotSpeed;
    F32               mZRotSpeed;
    Point3F           mRotAngles;
    F32               mRadius;
    bool              mStatic;
    F32               mElasticity;
    F32               mFriction;

    ParticleEmitter* mEmitterList[DebrisData::DDC_NUM_EMITTERS];

    bool  bounce(const Point3F& nextPos, F32 dt);
    void  computeNewState(Point3F& newPos, Point3F& newVel, F32 dt);
    void  explode();
    void  render2D();
    void  rotate(F32 dt);

protected:
    virtual void   processTick(const Move* move);
    virtual void   advanceTime(F32 dt);
    bool prepRenderImage(SceneState* state, const U32 stateKey, const U32 startZone, const bool modifyBaseZoneState);
    void prepBatchRender(SceneState* state);


    bool           onAdd();
    void           onRemove();
    void           updateEmitters(Point3F& pos, Point3F& vel, U32 ms);

public:

    Debris();
    ~Debris();

    static void    initPersistFields();

    bool   onNewDataBlock(GameBaseData* dptr);

    void  init(const Point3F& position, const Point3F& velocity);
    void  setLifetime(F32 lifetime) { mLifetime = lifetime; }
    void  setPartInstance(TSPartInstance* part) { mPart = part; }
    void  setSize(F32 size);
    void  setVelocity(const Point3F& vel) { mVelocity = vel; }
    void  setRotAngles(const Point3F& angles) { mRotAngles = angles; }

    DECLARE_CONOBJECT(Debris);

};




#endif
