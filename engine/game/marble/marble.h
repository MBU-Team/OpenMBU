//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _MARBLE_H_
#define _MARBLE_H_

#ifndef _SHAPEBASE_H_
#include "game/shapeBase.h"
#endif

#ifndef _DECALMANAGER_H_
#include "sim/decalManager.h"
#endif

#ifndef _POWERUP_H_
#include "game/marble/powerup.h"
#endif

class MarbleData;

class Marble : public ShapeBase
{
private:
    typedef ShapeBase Parent;

    enum MarbleModeFlags
    {
        MoveMode = 0x1,
        RestrictXYZMode = 0x2,
        CameraHoverMode = 0x4,
        TimerMode = 0x8,
        StartingMode = 0x10,
        StoppingMode = 0x20,
        FinishMode = 0x40,
        ActiveModeBits = 0x4,
        ActiveModeMask = 0xF,
        ModeBits = 0x7,
        MaxModeTicks = 0x1F,
    };

    enum MaskBits
    {
        MoveMask = 0x1,
        RenderModeMask = 0x2,
    };

    struct Contact
    {
        SimObject* object;
        Point3D position;
        Point3D normal;
        Point3F actualNormal;
        Point3D surfaceVelocity;
        Point3D surfaceFrictionVelocity;
        F64 staticFriction;
        F64 kineticFriction;
        Point3D vAtC;
        F64 vAtCMag;
        F64 normalForce;
        F64 contactDistance;
        F32 friction;
        F32 restitution;
        F32 force;
        S32 material;
    };

    struct SinglePrecision
    {
        Point3F mPosition;
        Point3F mVelocity;
        Point3F mOmega;
    };

    struct StateDelta
    {
        Point3D pos;
        Point3D posVec;
        F32 prevMouseX;
        F32 prevMouseY;
        Move move;
    };

    struct EndPadEffect
    {
        F32 effectTime;
        Point3F lastCamFocus;
    };

    struct MaterialCollision
    {
        U32 ghostIndex;
        U32 materialId;
        NetObject* object;
    };

    struct PowerUpState
    {
        bool active;
        U32 ticksLeft;
        S32 imageSlot;
        SimObjectPtr<ParticleEmitter> emitter;
    };


    Marble::SinglePrecision mSinglePrecision;
    Vector<Marble::Contact> mContacts;
    Marble::Contact mBestContact;
    Marble::Contact mLastContact;
    Point3F mMovePath[2];
    F32 mMovePathTime[2];
    U32 mMovePathSize;
    Marble::StateDelta delta;
    Point3F mLastRenderPos;
    Point3F mLastRenderVel;
    MarbleData* mDataBlock;
    S32 mBounceEmitDelay;
    U32 mPowerUpId;
    U32 mPowerUpTimer;
    U32 mBlastTimer;
    U32 mBlastEnergy;
    F32 mRenderBlastPercent;
    GFXVertexBufferHandle<GFXVertexP> mVertBuff;
    GFXPrimitiveBufferHandle mPrimBuff;
    U32 mMarbleTime;
    U32 mMarbleBonusTime;
    U32 mFullMarbleTime;
    bool mUseFullMarbleTime;
    U32 mMode;
    U32 mModeTimer;
    SimObjectPtr<AudioStreamSource> mRollHandle;
    SimObjectPtr<AudioStreamSource> mSlipHandle;
    SimObjectPtr<AudioStreamSource> mMegaHandle;
    F32 mRadius;
    QuatF mGravityFrame;
    QuatF mGravityRenderFrame;
    Point3D mVelocity;
    Point3D mPosition;
    Point3D mOmega;
    F32 mMouseX;
    F32 mMouseY;
    bool mControllable;
    bool mOOB;
    Point3F mOOBCamPos;
    U32 mSuperSpeedDoneTime;
    F32 mLastYaw;
    Point3F mNetSmoothPos;
    bool mCenteringCamera;
    F32 mRadsLeftToCenter;
    F32 mRadsStartingToCenter;
    U32 mCheckPointNumber;
    Marble::EndPadEffect mEffect;
    Point3F mLastCamPos;
    F32 mCameraDist;
    bool mCameraInit;
    SceneObject* mPadPtr;
    bool mOnPad;
    Marble::PowerUpState mPowerUpState[10];
    PowerUpData::ActiveParams mPowerUpParams;
    SimObjectPtr<ParticleEmitter> mTrailEmitter;
    SimObjectPtr<ParticleEmitter> mMudEmitter;
    SimObjectPtr<ParticleEmitter> mGrassEmitter;
    Point3F mShadowPoints[33];
    bool mShadowGenerated;
    MatInstance* mStencilMaterial;

public:
    DECLARE_CONOBJECT(Marble);

    Marble();
    ~Marble();

    static void initPersistFields();

    static U32 smEndPadId;
};

class MarbleData : public ShapeBaseData
{
private:
    typedef ShapeBaseData Parent;

    enum Sounds {
        SoftImpactSound,
        HardImpactSound,
        MaxSounds,
    };

    AudioProfile* sound[13];
    F32 maxRollVelocity;
    F32 minVelocityBounceSoft;
    F32 minVelocityBounceHard;
    F32 minVelocityMegaBounceSoft;
    F32 minVelocityMegaBounceHard;
    F32 bounceMinGain;
    F32 bounceMegaMinGain;
    F32 angularAcceleration;
    F32 brakingAcceleration;
    F32 staticFriction;
    F32 kineticFriction;
    F32 bounceKineticFriction;
    F32 gravity;
    F32 size;
    F32 megaSize;
    F32 maxDotSlide;
    F32 bounceRestitution;
    F32 airAcceleration;
    F32 energyRechargeRate;
    F32 jumpImpulse;
    F32 cameraDistance;
    U32 maxJumpTicks;
    F32 maxForceRadius;
    F32 minBounceVel;
    U32 startModeTime;
    U32 stopModeTime;
    F32 minBounceSpeed;
    F32 minTrailSpeed;
    ParticleEmitterData* bounceEmitter;
    ParticleEmitterData* trailEmitter;
    ParticleEmitterData* mudEmitter;
    ParticleEmitterData* grassEmitter;
    PowerUpData* powerUps;
    U32 blastRechargeTime;
    U32 maxNaturalBlastRecharge;
    DecalData* mDecalData;
    S32 mDecalID;

public:
    DECLARE_CONOBJECT(MarbleData);

    MarbleData();

    static void initPersistFields();
};

#endif // _MARBLE_H_
