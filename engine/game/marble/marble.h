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

#ifndef _CONCRETEPOLYLIST_H_
#include "collision/concretePolyList.h"
#endif

#ifndef _H_PATHEDINTERIOR
#include "interior/pathedInterior.h"
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

        Contact();
    };

    struct SinglePrecision
    {
        Point3F mPosition;
        Point3F mVelocity;
        Point3F mOmega;

        SinglePrecision();
    };

    struct StateDelta
    {
        Point3D pos;
        Point3D posVec;
        F32 prevMouseX;
        F32 prevMouseY;
        Move move;

        StateDelta();
    };

    struct EndPadEffect
    {
        F32 effectTime;
        Point3F lastCamFocus;

        EndPadEffect();
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

        PowerUpState();
        ~PowerUpState();
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

    SceneObject* getPad();
    S32 getPowerUpId();
    QuatF const& getGravityFrame();
    U32 getMaxNaturalBlastEnergy();
    U32 getMaxBlastEnergy();
    F32 getBlastPercent();
    F32 getBlastEnergy() const;
    void setBlastEnergy(F32);
    void setUseFullMarbleTime(bool);
    void setMarbleTime(U32);
    U32 getMarbleTime();
    void setMarbleBonusTime(U32);
    U32 getMarbleBonusTime();
    U32 getFullMarbleTime();
    Marble::Contact& getLastContact();
    void setGravityFrame(const QuatF&, bool);
    virtual void onSceneRemove();
    void setPosition(const Point3D&, bool);
    void setPosition(const Point3D&, const AngAxisF&, F32);
    Point3F& getPosition();
    void victorySequence();
    void setMode(U32);
    void setOOB(bool);
    virtual void interpolateTick(F32 delta);
    S32 mountPowerupImage(ShapeBaseImageData*);
    void updatePowerUpParams();
    virtual bool getForce(Point3F&, Point3F*);
    virtual U32 packUpdate(NetConnection* conn, U32 mask, BitStream* stream);
    virtual void unpackUpdate(NetConnection* conn, BitStream* stream);
    virtual U32 filterMaskBits(U32, NetConnection*);
    virtual void writePacketData(GameConnection* conn, BitStream* stream);
    virtual void readPacketData(GameConnection* conn, BitStream* stream);
    void renderShadow(F32, F32);
    virtual void renderImage(SceneState* state);
    void bounceEmitter(F32, const Point3F&);
    virtual MatrixF getShadowTransform() const;
    virtual void setVelocity(const Point3F& vel);
    virtual Point3F getVelocity() const;
    virtual Point3F getShadowScale() const;
    Point3F getGravityRenderDir();
    virtual void getShadowLightVectorHack(Point3F&);
    virtual bool onSceneAdd(SceneGraph* graph);
    virtual bool onNewDataBlock(GameBaseData* dptr);
    virtual void onRemove();
    bool updatePadState();
    void doPowerUpBoost(S32);
    void doPowerUpPower(S32);
    void updatePowerups();
    virtual void updateMass();
    void trailEmitter(U32);
    void updateRollSound(F32, F32);
    void playBounceSound(Marble::Contact&, F64);
    void setPad(SceneObject*);
    void findRenderPos(F32);
    virtual void advanceTime(F32 dt);
    virtual void computeNetSmooth(F32);
    void doPowerUp(S32);
    void prepShadows();
    virtual bool onAdd();
    void processMoveTriggers(const Move*);
    void processItemsAndTriggers(const Point3F&, const Point3F&);
    void setPowerUpId(U32, bool);
    virtual void processTick(const Move* move);

    // Marble Physics
    Point3D getVelocityD() const;
    void setVelocityD(const Point3D&);
    void setVelocityRotD(const Point3D&);
    virtual void applyImpulse(const Point3F& pos, const Point3F& vec);
    void clearMarbleAxis();
    void applyContactForces(const Move*, bool, Point3D&, const Point3D&, F64, Point3D&, Point3D&, F32&);
    void getMarbleAxis(Point3D&, Point3D&, Point3D&);
    const Point3F& getMotionDir();
    bool computeMoveForces(Point3D&, Point3D&, const Move*);
    void velocityCancel(bool, bool, bool&, bool&, Vector<PathedInterior*>&);
    Point3D getExternalForces(const Move*, F64);
    void advancePhysics(const Move*, U32);

    // Marble Collision
    void clearObjectsAndPolys();
    void findObjectsAndPolys(U32, const Box3F&, bool);
    bool testMove(Point3D, Point3D&, F64&, F64, U32, bool);
    void findContacts(U32, const Point3D*, const F32*);
    void computeFirstPlatformIntersect(F64&, Vector<PathedInterior*>&);
    void resetObjectsAndPolys(U32, const Box3F&);

    // Marble Camera
    bool moveCamera(Point3F, Point3F, Point3F&, U32, F32);
    void processCameraMove(const Move*);
    void startCenterCamera();
    bool isCameraClear(Point3F, Point3F);
    void getLookMatrix(MatrixF*);
    void cameraLookAtPt(const Point3F&);
    void resetPlatformsForCamera();
    void getOOBCamera(MatrixF*);
    void setPlatformsForCamera(const Point3F&, const Point3F&, const Point3F&);
    virtual void getCameraTransform(F32* pos, MatrixF* mat);



    static U32 smEndPadId;

private:
    virtual void setTransform(const MatrixF& mat);
    void renderShadowVolumes(SceneState*);

    // Marble Collision
    bool pointWithinPoly(const ConcretePolyList::Poly&, const Point3F&);
    bool pointWithinPolyZ(const ConcretePolyList::Poly&, const Point3F&, const Point3F&);
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

    virtual bool preload(bool server, char errorBuffer[256]);
    virtual void packData(BitStream*);
    virtual void unpackData(BitStream*);
};

#endif // _MARBLE_H_
