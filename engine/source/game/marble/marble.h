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

#ifndef _STATICSHAPE_H_
#include <game/staticShape.h>
#endif

extern Point3F gMarbleMotionDir;
extern bool gMarbleAxisSet;
extern Point3F gWorkGravityDir;
extern Point3F gMarbleSideDir;

// These might be useful later
//#include <cmath>
//#define CheckNAN(c) { if(isnan(c)) __debugbreak(); }
//#define CheckNAN3(c) { CheckNAN(c.x) CheckNAN(c.y) CheckNAN(c.z) }
//#define CheckNAN3p(c) { CheckNAN(c->x) CheckNAN(c->y) CheckNAN(c->z) }
//#define CheckNANBox(c) { Check3(c.min) Check3(c.min) }
//#define CheckNANBoxp(c) { Check3(c->min) Check3(c->min) }
//#define CheckNANAng(c) { CheckNAN(c.axis.x) CheckNAN(c.axis.y) CheckNAN(c.axis.z) CheckNAN(c.angle) }
//#define CheckNANAngp(c) { CheckNAN(c->axis.x) CheckNAN(c->axis.y) CheckNAN(c->axis.z) CheckNAN(c->angle) }

class MarbleData;

class Marble : public ShapeBase
{
private:
    typedef ShapeBase Parent;

    friend class ShapeBase;

public:
    enum MarbleModeFlags
    {
        MoveMode = 0x1,
        RestrictXYZMode = 0x2,
        CameraHoverMode = 0x4,
        TimerMode = 0x8,
        StartingMode = 0x10,
        StoppingMode = 0x20,
        FinishMode = 0x40
    };

    enum MBPhysics
    {
        MBU,
        MBG,
        XNA,
        MBUSlopes,
        MBGSlopes,
        MBPhysics_Count
    };

    enum UpdateMaskBits
    {
        ActiveModeBits = 0x4,
        ActiveModeMask = 0xF,
        ModeBits = 0x7,
        MaxModeTicks = 0x1F
    };
private:

    enum MaskBits
    {
        MoveMask = Parent::NextFreeMask,
        WarpMask = Parent::NextFreeMask << 1,
        PowerUpMask = Parent::NextFreeMask << 2,
        GravityMask = Parent::NextFreeMask << 3,
        GravitySnapMask = Parent::NextFreeMask << 4,
        OOBMask = Parent::NextFreeMask << 5,
        NextFreeMask = Parent::NextFreeMask << 6
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
    SFXSource* mRollHandle;
    SFXSource* mSlipHandle;
    SFXSource* mMegaHandle;
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
    Marble::PowerUpState mPowerUpState[PowerUpData::MaxPowerUps];
    PowerUpData::ActiveParams mPowerUpParams;
    SimObjectPtr<ParticleEmitter> mTrailEmitter;
    SimObjectPtr<ParticleEmitter> mMudEmitter;
    SimObjectPtr<ParticleEmitter> mGrassEmitter;
    Point3F mShadowPoints[33];
    bool mShadowGenerated;
    MatInstance* mStencilMaterial;

    U32 mPhysics;

    F32 mSize;

    Point3F mCameraPosition;

public:
    DECLARE_CONOBJECT(Marble);

    Marble();
    ~Marble();

    static void initPersistFields();

    // consoleInit was not overriden in the original decompile
    // if you want it to match MBO exactly, then remove it.
    static void consoleInit();

    SceneObject* getPad();
    S32 getPowerUpId();
    const QuatF& getGravityFrame();
    const QuatF& getGravityRenderFrame() const { return mGravityRenderFrame; }
    const Point3F& getGravityDir(Point3F* result);
    U32 getMaxNaturalBlastEnergy();
    U32 getMaxBlastEnergy();
    F32 getBlastPercent();
    F32 getRenderBlastPercent() { return mRenderBlastPercent; }
    F32 getBlastEnergy() const;
    void setBlastEnergy(F32 energy);
    void setUseFullMarbleTime(bool useFull);
    void setMarbleTime(U32 time);
    U32 getMarbleTime();
    void setMarbleBonusTime(U32 time);
    U32 getMarbleBonusTime();
    U32 getFullMarbleTime();
    Marble::Contact& getLastContact();
    void setGravityFrame(const QuatF& q, bool snap);
    virtual void onSceneRemove();
    void setPosition(const Point3D& pos, bool doWarp);
    void setPosition(const Point3D& pos, const AngAxisF& angAxis, float mouseY);
    Point3F& getPosition();
    void victorySequence();
    void setMode(U32 mode);
    void setPhysics(U32 physics);
    void setSize(F32 size);
    U32 getMode() { return mMode; }
    void setOOB(bool isOOB);
    virtual void interpolateTick(F32 delta);
    S32 mountPowerupImage(ShapeBaseImageData* imageData);
    void updatePowerUpParams();
    virtual bool getForce(Point3F& pos, Point3F* force);
    virtual U32 packUpdate(NetConnection* conn, U32 mask, BitStream* stream);
    virtual void unpackUpdate(NetConnection* conn, BitStream* stream);
    virtual U32 filterMaskBits(U32 mask, NetConnection* connection);
    virtual void writePacketData(GameConnection* conn, BitStream* stream);
    virtual void readPacketData(GameConnection* conn, BitStream* stream);
    void renderShadow(F32 dist, F32 fogAmount);
    void renderShadow(SceneState* state, RenderInst* ri);
    void calcClassRenderData(); // used for marble shadow
    virtual void renderImage(SceneState* state);
    void bounceEmitter(F32 speed, const Point3F& normal);
    virtual void setVelocity(const Point3F& vel);
    virtual Point3F getVelocity() const;
    Point3F getGravityRenderDir();
    virtual MatrixF getShadowTransform() const;
    virtual Point3F getShadowScale() const;
    virtual void getShadowLightVectorHack(Point3F& lightVec);
    virtual bool onSceneAdd(SceneGraph* graph);
    virtual bool onNewDataBlock(GameBaseData* dptr);
    virtual void onRemove();
    bool updatePadState();
    void doPowerUpBoost(S32 powerUpId);
    void doPowerUpPower(S32 powerUpId);
    void updatePowerups();
    virtual void updateMass();
    void trailEmitter(U32 timeDelta);
    void updateRollSound(F32 contactPct, F32 slipAmount);
    void playBounceSound(Marble::Contact& contactSurface, F64 contactVel);
    void setPad(SceneObject* obj);
    void findRenderPos(F32 dt);
    virtual void advanceTime(F32 dt);
    virtual void computeNetSmooth(F32 backDelta);
    void doPowerUp(S32 powerUpId);
    void prepShadows();
    virtual bool onAdd();
    void processMoveTriggers(const Move* move);
    void processItemsAndTriggers(const Point3F& startPos, const Point3F& endPos);
    void setPowerUpId(U32 id, bool reset);
    virtual void processTick(const Move* move);

#ifdef MB_CLIENT_PHYSICS_EVERY_FRAME
    virtual void processPhysicsTick(const Move* move, F32 dt);
#endif

    // Marble Physics
    Point3D getVelocityD() const;
    void setVelocityD(const Point3D& vel);
    void setVelocityRotD(const Point3D& rot);
    virtual void applyImpulse(const Point3F& pos, const Point3F& vec);
    void clearMarbleAxis();
    void applyContactForces(const Move* move, bool isCentered, Point3D& aControl, const Point3D& desiredOmega, F64 timeStep, Point3D& A, Point3D& a, F32& slipAmount);
    void getMarbleAxis(Point3D& sideDir, Point3D& motionDir, Point3D& upDir);
    const Point3F& getMotionDir();
    bool computeMoveForces(Point3D& aControl, Point3D& desiredOmega, const Move* move);
    void velocityCancel(bool surfaceSlide, bool noBounce, bool& bouncedYet, bool& stoppedPaths, Vector<PathedInterior*>& pitrVec);
    Point3D getExternalForces(const Move* move, F64 timeStep);
    void advancePhysics(const Move* move, U32 timeDelta);

    // Marble Collision
    void clearObjectsAndPolys();
    void findObjectsAndPolys(U32 collisionMask, const Box3F& testBox, bool testPIs);
    bool testMove(Point3D velocity, Point3D& position, F64& deltaT, F64 radius, U32 collisionMask, bool testPIs);
    void findContacts(U32 contactMask, const Point3D* inPos, const F32* inRad);
    void computeFirstPlatformIntersect(F64& dt, Vector<PathedInterior*>& pitrVec);
    void resetObjectsAndPolys(U32 collisionMask, const Box3F& testBox);

    // Marble Camera
    bool moveCamera(Point3F start, Point3F end, Point3F& result, U32 maxIterations, F32 timeStep);
    void processCameraMove(const Move* move);
    void startCenterCamera();
    bool isCameraClear(Point3F start, Point3F end);
    void getLookMatrix(MatrixF* camMat);
    void cameraLookAtPt(const Point3F& pt);
    void resetPlatformsForCamera();
    void getOOBCamera(MatrixF* mat);
    void setPlatformsForCamera(const Point3F& marblePos, const Point3F& startCam, const Point3F& endCam);
    virtual void getCameraTransform(F32* pos, MatrixF* mat);

    static U32 smEndPadId;
    static SimObjectPtr<StaticShape> smEndPad;
    static Vector<PathedInterior*> smPathItrVec;
    static Vector<Marble*> marbles;
    static ConcretePolyList polyList;

#ifdef MBXP_EMOTIVES
    static bool smUseEmotives;
#endif

#ifdef MB_PHYSICS_SWITCHABLE
    static bool smTrapLaunch;
#endif

private:
    virtual void setTransform(const MatrixF& mat);
    void renderShadowVolumes(SceneState* state);

    // Marble Collision
    bool pointWithinPoly(const ConcretePolyList::Poly& poly, const Point3F& point);
    bool pointWithinPolyZ(const ConcretePolyList::Poly& poly, const Point3F& point, const Point3F& upDir);
};

class MarbleData : public ShapeBaseData
{
private:
    typedef ShapeBaseData Parent;

    friend class Marble;

    enum Sounds {
        RollHard,
        RollMega,
        RollIce,
        Slip,
        Bounce1,
        Bounce2,
        Bounce3,
        Bounce4,
        MegaBounce1,
        MegaBounce2,
        MegaBounce3,
        MegaBounce4,
        Jump,
        MaxSounds,
    };

    SFXProfile* sound[MaxSounds];
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
    //F32 size;
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
    F32 minMediumBounceSpeed;
    F32 minHardBounceSpeed;
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

    F32 cameraLag;
    F32 cameraDecay;

    F32 cameraLagMaxOffset;

    Point3F SoftBounceImpactShakeFreq;
    Point3F SoftBounceImpactShakeAmp;
    F32 SoftBounceImpactShakeDuration;
    F32 SoftBounceImpactShakeFalloff;

    Point3F MediumBounceImpactShakeFreq;
    Point3F MediumBounceImpactShakeAmp;
    F32 MediumBounceImpactShakeDuration;
    F32 MediumBounceImpactShakeFalloff;

    Point3F HardBounceImpactShakeFreq;
    Point3F HardBounceImpactShakeAmp;
    F32 HardBounceImpactShakeDuration;
    F32 HardBounceImpactShakeFalloff;

    F32 slipEmotiveThreshold;

public:
    DECLARE_CONOBJECT(MarbleData);

    MarbleData();

    static void initPersistFields();

    virtual bool preload(bool server, char errorBuffer[256]);
    virtual void packData(BitStream*);
    virtual void unpackData(BitStream*);
};

#endif // _MARBLE_H_
