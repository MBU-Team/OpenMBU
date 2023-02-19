//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _EXPLOSION_H_
#define _EXPLOSION_H_

#ifndef _GAMEBASE_H_
#include "game/gameBase.h"
#endif
#ifndef _TSSHAPE_H_
#include "ts/tsShape.h"
#endif

class ParticleEmitter;
class ParticleEmitterData;
class TSThread;
class SFXProfile;
struct DebrisData;
class ShockwaveData;

//--------------------------------------------------------------------------
class ExplosionData : public GameBaseData {
public:
    typedef GameBaseData Parent;

    enum ExplosionConsts
    {
        EC_NUM_DEBRIS_TYPES = 1,
        EC_NUM_EMITTERS = 4,
        EC_MAX_SUB_EXPLOSIONS = 5,
        EC_NUM_TIME_KEYS = 4,
    };

public:
    StringTableEntry dtsFileName;

    bool faceViewer;

    S32 particleDensity;
    F32 particleRadius;

    SFXProfile* soundProfile;
    ParticleEmitterData* particleEmitter;
    S32                  soundProfileId;
    S32                  particleEmitterId;

    Point3F              explosionScale;
    F32                  playSpeed;

    Resource<TSShape> explosionShape;
    S32               explosionAnimation;

    ParticleEmitterData* emitterList[EC_NUM_EMITTERS];
    S32                     emitterIDList[EC_NUM_EMITTERS];

    ShockwaveData* shockwave;
    S32                     shockwaveID;
    bool                    shockwaveOnTerrain;

    DebrisData* debrisList[EC_NUM_DEBRIS_TYPES];
    S32            debrisIDList[EC_NUM_DEBRIS_TYPES];

    F32            debrisThetaMin;
    F32            debrisThetaMax;
    F32            debrisPhiMin;
    F32            debrisPhiMax;
    S32            debrisNum;
    S32            debrisNumVariance;
    F32            debrisVelocity;
    F32            debrisVelocityVariance;

    // sub - explosions
    ExplosionData* explosionList[EC_MAX_SUB_EXPLOSIONS];
    S32               explosionIDList[EC_MAX_SUB_EXPLOSIONS];

    S32               delayMS;
    S32               delayVariance;
    S32               lifetimeMS;
    S32               lifetimeVariance;

    F32               offset;
    Point3F           sizes[EC_NUM_TIME_KEYS];
    F32               times[EC_NUM_TIME_KEYS];

    // camera shake data
    bool              shakeCamera;
    VectorF           camShakeFreq;
    VectorF           camShakeAmp;
    F32               camShakeDuration;
    F32               camShakeRadius;
    F32               camShakeFalloff;

    S32               impulseMask;
    F32               impulseRadius;
    F32               impulseForce;

    // Dynamic Lighting. The light is smoothly
    // interpolated from start to end time.
    F32               lightStartRadius;
    F32               lightEndRadius;
    ColorF            lightStartColor;
    ColorF            lightEndColor;

    ExplosionData();
    DECLARE_CONOBJECT(ExplosionData);
    bool onAdd();
    bool preload(bool server, char errorBuffer[256]);
    static void  initPersistFields();
    virtual void packData(BitStream* stream);
    virtual void unpackData(BitStream* stream);
};
DECLARE_CONSOLETYPE(ExplosionData)


//--------------------------------------------------------------------------
class Explosion : public GameBase
{
    typedef GameBase Parent;

private:
    ExplosionData* mDataBlock;

    TSShapeInstance* mExplosionInstance;
    TSThread* mExplosionThread;

    ParticleEmitter* mEmitterList[ExplosionData::EC_NUM_EMITTERS];

    U32               mCurrMS;
    U32               mEndingMS;
    F32               mRandAngle;
    LightInfo         mLight;

protected:
    Point3F  mInitialPosition;
    Point3F  mInitialNormal;
    F32      mFade;
    F32      mFog;
    bool     mActive;
    S32      mDelayMS;
    F32      mRandomVal;
    U32      mCollideType;

    bool mInitialPosSet;

protected:
    bool onAdd();
    void onRemove();
    bool explode();

    void processTick(const Move* move);
    void advanceTime(F32 dt);
    void updateEmitters(F32 dt);
    void applyImpulse();
    void launchDebris(Point3F& axis);
    void spawnSubExplosions();
    void setCurrentScale();

    // Rendering
protected:
    bool prepRenderImage(SceneState* state, const U32 stateKey, const U32 startZone, const bool modifyBaseZoneState);
    void prepBatchRender(SceneState* state);
    void prepModelView(SceneState*);
    void registerLights(LightManager* lm, bool lightingScene);

public:
    Explosion();
    ~Explosion();
    void setInitialState(const Point3F& point, const Point3F& normal, const F32 fade = 1.0);

    bool onNewDataBlock(GameBaseData* dptr);
    void setCollideType(U32 cType) { mCollideType = cType; }

    U32 packUpdate(NetConnection *conn, U32 mask, BitStream *stream) override;
    void unpackUpdate(NetConnection *conn, BitStream *stream) override;

    DECLARE_CONOBJECT(Explosion);
    static void initPersistFields();
};

#endif // _H_EXPLOSION

