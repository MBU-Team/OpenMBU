//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _SHAPEBASE_H_
#define _SHAPEBASE_H_

#ifndef _GAMEBASE_H_
#include "game/gameBase.h"
#endif
#ifndef _MATERIALLIST_H_
//#include "dgl/materialList.h"
#endif
#ifndef _MOVEMANAGER_H_
#include "game/moveManager.h"
#endif
#ifndef _COLOR_H_
#include "core/color.h"
#endif
#ifndef _CONVEX_H_
#include "collision/convex.h"
#endif
#ifndef _SCENESTATE_H_
#include "sceneGraph/sceneState.h"
#endif
#ifndef _NETSTRINGTABLE_H_
#include "sim/netStringTable.h"
#endif
#ifndef _RENDER_INST_MGR_H_
#include "renderInstance/renderInstMgr.h"
#endif

#include "lightingSystem/sgObjectShadows.h"

class GFXCubemap;
class TSShapeInstance;
//class Shadow;
class SceneState;
class TSShape;
class TSThread;
class GameConnection;
struct CameraScopeQuery;
class ParticleEmitter;
class ParticleEmitterData;
class ProjectileData;
class ExplosionData;
struct DebrisData;
class ShapeBase;
class SFXSource;
class SFXProfile;

typedef void* Light;

extern bool gNoRenderAstrolabe;
extern bool gForceNotHidden;

//--------------------------------------------------------------------------

extern void collisionFilter(SceneObject* object, S32 key);
extern void defaultFilter(SceneObject* object, S32 key);


//--------------------------------------------------------------------------
class ShapeBaseConvex : public Convex
{
    typedef Convex Parent;
    friend class ShapeBase;
    friend class Vehicle;

protected:
    ShapeBase* pShapeBase;
    MatrixF* nodeTransform;

public:
    MatrixF* transform;
    U32         hullId;
    Box3F       box;

public:
    ShapeBaseConvex() { mType = ShapeBaseConvexType; nodeTransform = 0; }
    ShapeBaseConvex(const ShapeBaseConvex& cv) {
        mObject = cv.mObject;
        pShapeBase = cv.pShapeBase;
        hullId = cv.hullId;
        nodeTransform = cv.nodeTransform;
        box = cv.box;
        transform = 0;
    }

    void findNodeTransform();
    const MatrixF& getTransform() const;
    Box3F getBoundingBox() const;
    Box3F getBoundingBox(const MatrixF& mat, const Point3F& scale) const;
    Point3F      support(const VectorF& v) const;
    void         getFeatures(const MatrixF& mat, const VectorF& n, ConvexFeature* cf);
    void         getPolyList(AbstractPolyList* list);
};

//--------------------------------------------------------------------------

struct ShapeBaseImageData : public GameBaseData {
private:
    typedef GameBaseData Parent;

public:
    enum Constants {
        MaxStates = 31,            ///< We get one less than state bits because of
                                      /// the way data is packed.
                                      NumStateBits = 5,
    };
    enum LightType {
        NoLight = 0,
        ConstantLight,
        PulsingLight,
        WeaponFireLight,
        NumLightTypes
    };
    struct StateData {
        StateData();
        const char* name;             ///< State name

        /// @name Transition states
        ///
        /// @{

        ///
        struct Transition {
            S32 loaded[2];             ///< NotLoaded/Loaded
            S32 ammo[2];               ///< Noammo/ammo
            S32 target[2];             ///< target/noTarget
            S32 trigger[2];            ///< Trigger up/down
            S32 wet[2];                ///< wet/notWet
            S32 timeout;               ///< Transition after delay
        } transition;
        bool ignoreLoadedForReady;

        /// @}

        /// @name State attributes
        /// @{

        bool fire;                    ///< Can only have one fire state
        bool ejectShell;              ///< Should we eject a shell casing in this state?
        bool allowImageChange;        ///< Can we switch to another image while in this state?
                                      ///
                                      ///  For instance, if we have a rocket launcher, the player
                                      ///  shouldn't be able to switch out <i>while</i> firing. So,
                                      ///  you'd set allowImageChange to false in firing states, and
                                      ///  true the rest of the time.
        bool scaleAnimation;          ///< Scale animation to fit the state timeout
        bool direction;               ///< Animation direction
        bool waitForTimeout;          ///< Require the timeout to pass before advancing to the next
                                      ///  state.
        F32 timeoutValue;             ///< A timeout value; the effect of this value is determined
                                      ///  by the flags scaleAnimation and waitForTimeout
        F32 energyDrain;              ///< Sets the energy drain rate per second of this state.
                                      ///
                                      ///  Energy is drained at energyDrain units/sec as long as
                                      ///  we are in this state.
        enum LoadedState {
            IgnoreLoaded,              ///< Don't change loaded state.
            Loaded,                    ///< Set us as loaded.
            NotLoaded,                 ///< Set us as not loaded.
            NumLoadedBits = 3          ///< How many bits to allocate to representing this state. (3 states needs 2 bits)
        } loaded;                     ///< Is the image considered loaded?
        enum SpinState {
            IgnoreSpin,                ///< Don't change spin state.
            NoSpin,                    ///< Mark us as having no spin (ie, stop spinning).
            SpinUp,                    ///< Mark us as spinning up.
            SpinDown,                  ///< Mark us as spinning down.
            FullSpin,                  ///< Mark us as being at full spin.
            NumSpinBits = 3            ///< How many bits to allocate to representing this state. (5 states needs 3 bits)
        } spin;                       ///< Spin thread state. (Used to control spinning weapons, e.g. chainguns)
        enum RecoilState {
            NoRecoil,
            LightRecoil,
            MediumRecoil,
            HeavyRecoil,
            NumRecoilBits = 3
        } recoil;                     ///< Recoil thread state.
                                      ///
                                      /// @note At this point, the only check being done is to see if we're in a
                                      ///       state which isn't NoRecoil; ie, no differentiation is made between
                                      ///       Light/Medium/Heavy recoils. Player::onImageRecoil() is the place
                                      ///       where this is handled.
        bool flashSequence;           ///< Is this a muzzle flash sequence?
                                      ///
                                      ///  A muzzle flash sequence is used as a source to randomly display frames from,
                                      ///  so if this is a flashSequence, we'll display a random piece each frame.
        S32 sequence;                 ///< Main thread sequence ID.
                                      ///
                                      ///
        S32 sequenceVis;              ///< Visibility thread sequence.
        const char* script;           ///< Function on datablock to call when we enter this state; passed the id of
                                      ///  the imageSlot.
        ParticleEmitterData* emitter; ///< A particle emitter; this emitter will emit as long as the gun is in this
                                      ///  this state.
        SFXProfile* sound;
        F32 emitterTime;              ///<
        S32 emitterNode;
    };

    /// @name State Data
    /// Individual state data used to initialize struct array
    /// @{
    const char* fireStateName;

    const char* stateName[MaxStates];

    const char* stateTransitionLoaded[MaxStates];
    const char* stateTransitionNotLoaded[MaxStates];
    const char* stateTransitionAmmo[MaxStates];
    const char* stateTransitionNoAmmo[MaxStates];
    const char* stateTransitionTarget[MaxStates];
    const char* stateTransitionNoTarget[MaxStates];
    const char* stateTransitionWet[MaxStates];
    const char* stateTransitionNotWet[MaxStates];
    const char* stateTransitionTriggerUp[MaxStates];
    const char* stateTransitionTriggerDown[MaxStates];
    const char* stateTransitionTimeout[MaxStates];
    F32                     stateTimeoutValue[MaxStates];
    bool                    stateWaitForTimeout[MaxStates];

    bool                    stateFire[MaxStates];
    bool                    stateEjectShell[MaxStates];
    F32                     stateEnergyDrain[MaxStates];
    bool                    stateAllowImageChange[MaxStates];
    bool                    stateScaleAnimation[MaxStates];
    bool                    stateDirection[MaxStates];
    StateData::LoadedState  stateLoaded[MaxStates];
    StateData::SpinState    stateSpin[MaxStates];
    StateData::RecoilState  stateRecoil[MaxStates];
    const char* stateSequence[MaxStates];
    bool                    stateSequenceRandomFlash[MaxStates];
    bool                    stateIgnoreLoadedForReady[MaxStates];

    SFXProfile* stateSound[MaxStates];
    const char* stateScript[MaxStates];

    ParticleEmitterData* stateEmitter[MaxStates];
    F32                     stateEmitterTime[MaxStates];
    const char* stateEmitterNode[MaxStates];

    /// @}

    //
    bool emap;                       ///< Environment mapping on?
    bool correctMuzzleVector;        ///< Adjust firing vector to eye's LOS point?
    bool firstPerson;                ///< Render the image when in first person?
    bool useEyeOffset;               ///< In first person, should we use the eyeTransform?

    StringTableEntry  shapeName;      ///< Name of shape to render.
    U32               mountPoint;     ///< Mount point for the image.
    MatrixF           mountOffset;    ///< Mount point offset, so we know where the image is.
    MatrixF           eyeOffset;      ///< Offset from eye for first person.

    ProjectileData* projectile;      ///< Information about what projectile this
                                     ///  image fires.

    F32   mass;                      ///< Mass!
    bool  usesEnergy;                ///< Does this use energy instead of ammo?
    F32   minEnergy;                 ///< Minimum energy for the weapon to be operable.
    bool  accuFire;                  ///< Should we automatically make image's aim converge with the crosshair?
    bool  cloakable;                 ///< Is this image cloakable when mounted?

    /// @name Lighting
    /// @{
    S32         lightType;           ///< Indicates the type of the light.
                                     ///
                                     ///  One of: ConstantLight, PulsingLight, WeaponFireLight.
    ColorF      lightColor;
    S32         lightTime;           ///< Indicates the time when the light effect started.
                                     ///
                                     ///  Used by WeaponFireLight to produce a short-duration flash.
    F32         lightRadius;         ///< Extent of light.
    LightInfo   mLight;              ///< Passed to the LightManager; filled out with the above information.
                                     ///
                                     ///  ShapeBaseImageData::registerLights() is responsible for performing
                                     ///  the calculations that prepare mLight.
    /// @}

    /// @name Shape Data
    /// @{
    Resource<TSShape> shape;         ///< Shape handle

    U32 mCRC;                        ///< Checksum of shape.
                                     ///
                                     ///  Calculated by the ResourceManager, see
                                     ///  ResourceManager::load().
    bool computeCRC;                 ///< Should the shape's CRC be checked?
    MatrixF mountTransform;          ///< Transformation to get to the mountNode.
    /// @}

    /// @name Nodes
    /// @{
    S32 retractNode;     ///< Retraction node ID.
                         ///
                         ///  When the player bumps against an object and the image is retracted to
                         ///  avoid having it interpenetrating the object, it is retracted towards
                         ///  this node.
    S32 muzzleNode;      ///< Muzzle node ID.
                         ///
                         ///
    S32 ejectNode;       ///< Ejection node ID.
                         ///
                         ///  The eject node is the node on the image from which shells are ejected.
    S32 emitterNode;     ///< Emitter node ID.
                         ///
                         ///  The emitter node is the node from which particles are emitted.
    /// @}

    /// @name Animation
    /// @{
    S32 spinSequence;                ///< ID of the spin animation sequence.
    S32 ambientSequence;             ///< ID of the ambient animation sequence.

    bool isAnimated;                 ///< This image contains at least one animated states
    bool hasFlash;                   ///< This image contains at least one muzzle flash animation state
    S32 fireState;                   ///< The ID of the fire state.
    /// @}

    /// @name Shell casing data
    /// @{
    DebrisData* casing;              ///< Information about shell casings.

    S32            casingID;            ///< ID of casing datablock.
                                        ///
                                        ///  When the network tells the client about the casing, it
                                        ///  it transmits the ID of the datablock. The datablocks
                                        ///  having previously been transmitted, all the client
                                        ///  needs to do is call Sim::findObject() and look up the
                                        ///  the datablock.

    Point3F        shellExitDir;        ///< Vector along which to eject shells from the image.
    F32            shellExitVariance;   ///< Variance from this vector in degrees.
    F32            shellVelocity;       ///< Velocity with which to eject shell casings.
    /// @}

    /// @name State Array
    ///
    /// State array is initialized onAdd from the individual state
    /// struct array elements.
    ///
    /// @{
    StateData state[MaxStates];   ///< Array of states.
    bool      statesLoaded;       ///< Are the states loaded yet?
    /// @}

    /// @name Infrastructure
    ///
    /// Miscellaneous inherited methods.
    /// @{

    DECLARE_CONOBJECT(ShapeBaseImageData);
    ShapeBaseImageData();
    ~ShapeBaseImageData();
    bool onAdd();
    bool preload(bool server, char errorBuffer[256]);
    S32 lookupState(const char* name);  ///< Get a state by name.
    static void initPersistFields();
    virtual void packData(BitStream* stream);
    virtual void unpackData(BitStream* stream);
    void registerImageLights(LightManager* lightManager, bool lightingScene, const Point3F& objectPosition, U32 startTime);

    /// @}
};


//--------------------------------------------------------------------------
/// @nosubgrouping
struct ShapeBaseData : public GameBaseData {
private:
    typedef GameBaseData Parent;

public:
    /// Various constants relating to the ShapeBaseData
    enum Constants {
        NumMountPoints = 32,
        NumMountPointBits = 5,
        MaxCollisionShapes = 8,
        AIRepairNode = 31
    };


    bool shadowEnable;
    bool shadowCanMove;
    bool shadowCanAnimate;
    bool shadowSelfShadow;
    U32 shadowSize;
    U32 shadowAnimationFrameSkip;
    F32 shadowMaxVisibleDistance;
    F32 shadowProjectionDistance;
    F32 shadowSphereAdjust;
    F32 shadowBias;


    StringTableEntry  shapeName;
    StringTableEntry  cloakTexName;

    bool dynamicReflection;

#ifdef MB_ULTRA
    // Marble Blast
    bool renderGemAura;
    bool glass;
    bool astrolabe;
    bool astrolabePrime;
    StringTableEntry gemAuraTextureName;
    GFXTexHandle gemAuraTexture;
#endif

    /// @name Destruction
    ///
    /// Everyone likes to blow things up!
    /// @{
    DebrisData* debris;
    S32               debrisID;
    StringTableEntry  debrisShapeName;
    Resource<TSShape> debrisShape;

    ExplosionData* explosion;
    S32               explosionID;

    ExplosionData* underwaterExplosion;
    S32               underwaterExplosionID;
    /// @}

    /// @name Physical Properties
    /// @{
    F32 mass;
    F32 drag;
    F32 density;
    F32 maxEnergy;
    F32 maxDamage;
    F32 repairRate;                  ///< Rate per tick.

    F32 disabledLevel;
    F32 destroyedLevel;

    S32 shieldEffectLifetimeMS;
    /// @}

    /// @name 3rd Person Camera
    /// @{
    F32 cameraMaxDist;               ///< Maximum distance from eye
    F32 cameraMinDist;               ///< Minumumistance from eye
    /// @}

    /// @name Camera FOV
    ///
    /// These are specified in degrees.
    /// @{
    F32 cameraDefaultFov;            ///< Default FOV.
    F32 cameraMinFov;                ///< Min FOV allowed.
    F32 cameraMaxFov;                ///< Max FOV allowed.
    /// @}

    /// @name Data initialized on preload
    /// @{

    Resource<TSShape> shape;         ///< Shape handle
    U32 mCRC;
    bool computeCRC;

#ifdef MB_ULTRA
    ColorF referenceColor;
#endif

    S32 eyeNode;                         ///< Shape's eye node index
    S32 cameraNode;                      ///< Shape's camera node index
    S32 shadowNode;                      ///< Move shadow center as this node moves
    S32 mountPointNode[NumMountPoints];  ///< Node index of mountPoint
    S32 debrisDetail;                    ///< Detail level used to generate debris
    S32 damageSequence;                  ///< Damage level decals
    S32 hulkSequence;                    ///< Destroyed hulk

    bool              canControl;             // can this object be controlled?
    bool              canObserve;             // may look at object in commander map?
    bool              observeThroughObject;   // observe this object through its camera transform and default fov

    /// @name HUD
    ///
    /// @note This may be only semi-functional.
    /// @{

    enum {
        NumHudRenderImages = 8,
    };

    StringTableEntry  hudImageNameFriendly[NumHudRenderImages];
    StringTableEntry  hudImageNameEnemy[NumHudRenderImages];
    //   TextureHandle     hudImageFriendly[NumHudRenderImages];
    //   TextureHandle     hudImageEnemy[NumHudRenderImages];

    bool              hudRenderCenter[NumHudRenderImages];
    bool              hudRenderModulated[NumHudRenderImages];
    bool              hudRenderAlways[NumHudRenderImages];
    bool              hudRenderDistance[NumHudRenderImages];
    bool              hudRenderName[NumHudRenderImages];
    /// @}

    /// @name Collision Data
    /// @{
    Vector<S32>   collisionDetails;  ///< Detail level used to collide with.
                                     ///
                                     /// These are detail IDs, see TSShape::findDetail()
    Vector<Box3F> collisionBounds;   ///< Detail level bounding boxes.

    Vector<S32>   LOSDetails;        ///< Detail level used to perform line-of-sight queries against.
                                     ///
                                     /// These are detail IDs, see TSShape::findDetail()
    /// @}

    /// @name Shadow Settings
    ///
    /// These are set by derived classes, not by script file.
    /// They control when shadows are rendered (and when generic shadows are substituted)
    ///
    /// @{

    F32 genericShadowLevel;
    F32 noShadowLevel;
    /// @}

    /// @name Misc. Settings
    /// @{
    bool emap;                       ///< Enable environment mapping?
    bool firstPersonOnly;            ///< Do we allow only first person view of this image?
    bool useEyePoint;                ///< Do we use this object's eye point to view from?
    bool aiAvoidThis;                ///< If set, the AI's will try to walk around this object...
                                     ///
                                     ///  @note There isn't really any AI code that can take advantage of this.
    bool isInvincible;               ///< If set, object cannot take damage (won't show up with damage bar either)
    bool renderWhenDestroyed;        ///< If set, will not render this object when destroyed.

    bool inheritEnergyFromMount;

    /// @}

    bool preload(bool server, char errorBuffer[256]);
    void computeAccelerator(U32 i);
    S32  findMountPoint(U32 n);

    /// @name Infrastructure
    /// The derived class should provide the following:
    /// @{
    DECLARE_CONOBJECT(ShapeBaseData);
    ShapeBaseData();
    ~ShapeBaseData();
    static void initPersistFields();
    virtual void packData(BitStream* stream);
    virtual void unpackData(BitStream* stream);
    /// @}
};


//----------------------------------------------------------------------------

/// ShapeBase is the renderable shape from which most of the scriptable objects
/// are derived, including the player, vehicle and items classes.  ShapeBase
/// provides basic shape loading, audio channels, and animation as well as damage
/// (and damage states), energy, and the ability to mount images and objects.
///
/// @nosubgrouping
class ShapeBase : public GameBase
{
    typedef GameBase Parent;
    friend class ShapeBaseConvex;
    friend class ShapeBaseImageData;
    friend void physicalZoneFind(SceneObject*, void*);

public:
    enum PublicConstants {
        ThreadSequenceBits = 6,
        MaxSequenceIndex = (1 << ThreadSequenceBits) - 1,
        EnergyLevelBits = 5,
        DamageLevelBits = 6,
        DamageStateBits = 2,
        // The thread and image limits should not be changed without
        // also changing the ShapeBaseMasks enum values declared
        // further down.
        MaxSoundThreads = 4,            ///< Should be a power of 2
        MaxScriptThreads = 4,            ///< Should be a power of 2
        MaxMountedImages = 4,            ///< Should be a power of 2
        MaxImageEmitters = 3,
        NumImageBits = 3,
        ShieldNormalBits = 8,
        CollisionTimeoutValue = 250      ///< Timeout in ms.
    };

    /// This enum indexes into the sDamageStateName array
    enum DamageState {
        Enabled,
        Disabled,
        Destroyed,
        NumDamageStates,
        NumDamageStateBits = 2,   ///< Should be log2 of the number of states.
    };

private:
    ShapeBaseData* mDataBlock;                ///< Datablock
    //GameConnection*   mControllingClient;        ///< Controlling client
    ShapeBase* mControllingObject;        ///< Controlling object
    bool              mTrigger[MaxTriggerKeys];  ///< What triggers are set, if any.


    /// @name Scripted Sound
    /// @{
    struct Sound {
        bool play;                    ///< Are we playing this sound?
        SimTime timeout;              ///< Time until we stop playing this sound.
        SFXProfile* profile;        ///< Profile on server
        SFXSource* sound;            ///< Handle on client
    };
    Sound mSoundThread[MaxSoundThreads];
    /// @}

    /// @name Scripted Animation Threads
    /// @{

    struct Thread {
        /// State of the animation thread.
        enum State {
            Play, Stop, Pause
        };
        TSThread* thread; ///< Pointer to 3space data.
        U32 state;        ///< State of the thread
                          ///
                          ///  @see Thread::State
        S32 sequence;     ///< The animation sequence which is running in this thread.
        U32 sound;        ///< Handle to sound.
        bool atEnd;       ///< Are we at the end of this thread?
        bool forward;     ///< Are we playing the thread forward? (else backwards)
        float timeScale;
        U32 id;
    };
    Thread mScriptThread[MaxScriptThreads];

    /// @}

public:
    class ThreadCmd
    {
    public:
        int slot;
        int seq;
        float timeScale;
        int delayTicks;

        ThreadCmd()
        {
            slot = 0;
            seq = -1;
            timeScale = 1.0f;
            delayTicks = -1;
        }
    };
private:
    ThreadCmd mThreadCmd;

    /// @name Invincibility
    /// @{
    F32 mInvincibleCount;
    F32 mInvincibleTime;
    F32 mInvincibleSpeed;
    F32 mInvincibleDelta;
    F32 mInvincibleEffect;
    F32 mInvincibleFade;
    bool mInvincibleOn;
    /// @}

protected:
    sgObjectShadows shadows;

    /// @name Mounted Images
    /// @{

    /// An image mounted on a shapebase.
    struct MountedImage {
        ShapeBaseImageData* dataBlock;
        ShapeBaseImageData::StateData* state;
        ShapeBaseImageData* nextImage;
        StringHandle skinNameHandle;
        StringHandle nextSkinNameHandle;

        /// @name State
        ///
        /// Variables tracking the state machine
        /// representing this specific mounted image.
        /// @{

        bool loaded;                  ///< Is the image loaded?
        bool nextLoaded;              ///< Is the next state going to result in the image being loaded?
        F32 delayTime;                ///< Time till next state.
        U32 fireCount;                ///< Fire skip count.
                                      ///
                                      /// This is incremented every time the triggerDown bit is changed,
                                      /// so that the engine won't be too confused if the player toggles the
                                      /// trigger a bunch of times in a short period.
                                      ///
                                      /// @note The network deals with this variable at 3-bit precision, so it
                                      /// can only range 0-7.
                                      ///
                                      /// @see ShapeBase::setImageState()

        bool triggerDown;             ///< Is the trigger down?
        bool ammo;                    ///< Do we have ammo?
                                      ///
                                      /// May be true based on either energy OR ammo.

        bool target;                  ///< Have we acquired a targer?
        bool wet;                     ///< Is the weapon wet?

        /// @}

        /// @name 3space
        ///
        /// Handles to threads and shapeinstances in the 3space system.
        /// @{
        TSShapeInstance* shapeInstance;
        TSThread* ambientThread;
        TSThread* visThread;
        TSThread* animThread;
        TSThread* flashThread;
        TSThread* spinThread;
        /// @}

        /// @name Effects
        ///
        /// Variables relating to lights, sounds, and particles.
        /// @{
        SimTime lightStart;     ///< Starting time for light flashes.
        bool animLoopingSound;  ///< Are we playing a looping sound?
        SFXSource* animSound;  ///< Handle to the current image sound.

        /// Represent the state of a specific particle emitter on the image.
        struct ImageEmitter {
            S32 node;
            F32 time;
            SimObjectPtr<ParticleEmitter> emitter;
        };
        ImageEmitter emitter[MaxImageEmitters];

        /// @}

        //
        MountedImage();
        ~MountedImage();
    };
    MountedImage mMountedImageList[MaxMountedImages];

    /// @}

    /// @name Render settings
    /// @{

    TSShapeInstance* mShapeInstance;
    Convex* mConvexList;
    StringHandle     mSkinNameHandle;

    StringHandle mShapeNameHandle;   ///< Name sent to client
    /// @}

    /// @name Physical Properties
    /// @{

    F32 mEnergy;                     ///< Current enery level.
    F32 mRechargeRate;               ///< Energy recharge rate (in units/tick).
    bool mChargeEnergy;              ///< Are we currently charging?
                                     /// @note Currently unused.

    F32 mMass;                       ///< Mass.
    F32 mOneOverMass;                ///< Inverse of mass.
                                     /// @note This is used to optimize certain physics calculations.

#ifdef MB_ULTRA
    bool mAnimateScale;
    Point3F mRenderScale;
#endif

    /// @}

    /// @name Physical Properties
    ///
    /// Properties for the current object, which are calculated
    /// based on the container we are in.
    ///
    /// @see ShapeBase::updateContainer()
    /// @see ShapeBase::mContainer
    /// @{
    F32 mDrag;                       ///< Drag.
    F32 mBuoyancy;                   ///< Buoyancy factor.
    U32 mLiquidType;                 ///< Type of liquid (if any) we are in.
    F32 mLiquidHeight;               ///< Height of liquid around us (from 0..1).
    F32 mWaterCoverage;              ///< Percent of this object covered by water

    Point3F mAppliedForce;
    F32 mGravityMod;
    /// @}

    F32 mDamageFlash;
    F32 mWhiteOut;

    bool mFlipFadeVal;
    U32 mLightTime;

    /// Last shield direction (cur. unused)
    Point3F mShieldNormal;

    /// Mounted objects
    struct MountInfo {
        ShapeBase* list;              ///< Objects mounted on this object
        ShapeBase* object;            ///< Object this object is mounted on.
        ShapeBase* link;              ///< Link to next object mounted to this object's mount
        U32 node;                     ///< Node point we are mounted to.
    } mMount;

public:
    /// @name Collision Notification
    /// This is used to keep us from spamming collision notifications. When
    /// a collision occurs, we add to this list; then we don't notify anyone
    /// of the collision until the CollisionTimeout expires (which by default
    /// occurs in 1/10 of a second).
    ///
    /// @see notifyCollision(), queueCollision()
    /// @{
    struct CollisionTimeout {
        CollisionTimeout* next;
        ShapeBase* object;
        U32 objectNumber;
        SimTime expireTime;
        VectorF vector;
#ifdef MARBLE_BLAST
        const Material *material;
#endif
    };
    CollisionTimeout* mTimeoutList;
    static CollisionTimeout* sFreeTimeoutList;

    /// Go through all the items in the collision queue and call onCollision on them all
    /// @see onCollision
    void notifyCollision();

#ifdef MARBLE_BLAST
    /// This gets called when an object collides with this object
    /// @param   object   Object colliding with this object
    /// @param   vec   Vector along which collision occured
    /// @param   mat   Material that was collided with
    virtual void onCollision(ShapeBase* object, VectorF vec, const Material* mat);
#else
    /// This gets called when an object collides with this object
    /// @param   object   Object colliding with this object
    /// @param   vec   Vector along which collision occured
    virtual void onCollision(ShapeBase* object, VectorF vec);
#endif

#ifdef MARBLE_BLAST
    /// Add a collision to the queue of collisions waiting to be handled @see onCollision
    /// @param   object    Object collision occurs with
    /// @param   vec       Vector along which collision occurs
    /// @param   surfaceId Id of the collided surface
    void queueCollision(ShapeBase* object, const VectorF& vec, const U32 surfaceId = 0);
#else
    /// Add a collision to the queue of collisions waiting to be handled @see onCollision
    /// @param   object   Object collision occurs with
    /// @param   vec      Vector along which collision occurs
    void queueCollision(ShapeBase* object, const VectorF& vec);
#endif
    /// @}
protected:

    /// @name Damage
    /// @{
    F32  mDamage;
    F32  mRepairRate;
    F32  mRepairReserve;
    bool mRepairDamage;
    DamageState mDamageState;
    TSThread* mDamageThread;
    TSThread* mHulkThread;
    VectorF damageDir;
    bool blowApart;
    /// @}

    /// @name Cloaking
    /// @{
    bool mCloaked;
    F32  mCloakLevel;
    //   TextureHandle mCloakTexture;
       //bool mHidden; ///< in/out of world

       /// @}

       /// @name Fading
       /// @{
    bool  mFadeOut;
    bool  mFading;
    F32   mFadeVal;
    F32   mFadeElapsedTime;
    F32   mFadeTime;
    F32   mFadeDelay;
public:
    F32   getFadeVal() { return mFadeVal; }
    /// @}
    ///

    U32 mCreateTime;


    /// @name Control info
    /// @{
    F32  mCameraFov;           ///< Camera FOV(in degrees)
    bool mIsControlled;        ///< Client side controlled flag

    /// @}
public:
    static U32 sLastRenderFrame;
protected:

    U32 mLastRenderFrame;
    F32 mLastRenderDistance;
    U32 mSkinHash;


    /// This recalculates the total mass of the object, and all mounted objects
    void updateMass();

    /// @name Image Manipulation
    /// @{

    /// Utility function to call script functions which have to do with ShapeBase
    /// objects.
    /// @param   imageSlot  Image Slot id
    /// @param   function   Function
    void scriptCallback(U32 imageSlot, const char* function);

    /// Assign a ShapeBaseImage to an image slot
    /// @param   imageSlot   Image Slot ID
    /// @param   imageData   ShapeBaseImageData to assign
    /// @param   skinNameHandle Skin texture name
    /// @param   loaded      Is the image loaded?
    /// @param   ammo        Does the image have ammo?
    /// @param   triggerDown Is the trigger on this image down?
    /// @param   target      Does the image have a target?
    virtual void setImage(U32 imageSlot, ShapeBaseImageData* imageData, StringHandle& skinNameHandle,
        bool loaded = true, bool ammo = false, bool triggerDown = false,
        bool target = false);

    /// Clear out an image slot
    /// @param   imageSlot   Image slot id
    void resetImageSlot(U32 imageSlot);

    /// Get the firing action state of the image
    /// @param   imageSlot   Image slot id
    U32  getImageFireState(U32 imageSlot);

    /// Sets the state of the image
    /// @param   imageSlot   Image slot id
    /// @param   state       State id
    /// @param   force       Force image to state or let it finish then change
    void setImageState(U32 imageSlot, U32 state, bool force = false);

    /// Advance animation on a image
    /// @param   imageSlot   Image slot id
    /// @param   dt          Change in time since last animation update
    void updateImageAnimation(U32 imageSlot, F32 dt);

    /// Advance state of image
    /// @param   imageSlot   Image slot id
    /// @param   dt          Change in time since last state update
    void updateImageState(U32 imageSlot, F32 dt);

    /// Start up the particle emitter for the this shapebase
    /// @param   image   Mounted image
    /// @param   state   State of shape base image
    void startImageEmitter(MountedImage& image, ShapeBaseImageData::StateData& state);

    /// Get light information for a mounted image
    /// @param   imageSlot   Image slot id
    Light* getImageLight(U32 imageSlot);

    /// @}

    /// Prune out non looping sounds from the sound manager which have expired
    void updateServerAudio();

    /// Updates the audio state of the supplied sound
    /// @param   st   Sound
    void updateAudioState(Sound& st);

    /// Recalculates the spacial sound based on the current position of the object
    /// emitting the sound.
    void updateAudioPos();

    /// Update bouyency and drag properties
    void updateContainer();

    /// @name Events
    /// @{
    virtual void onDeleteNotify(SimObject*);
    virtual void onImageRecoil(U32 imageSlot, ShapeBaseImageData::StateData::RecoilState);
    virtual void ejectShellCasing(U32 imageSlot);
    virtual void updateDamageLevel();
    virtual void updateDamageState();
    virtual void blowUp();
    virtual void onMount(ShapeBase* obj, S32 node);
    virtual void onUnmount(ShapeBase* obj, S32 node);
    virtual void onImpact(SceneObject* obj, VectorF vec);
    virtual void onImpact(VectorF vec);
    /// @}

public:
    ShapeBase();
    ~ShapeBase();

    TSShapeInstance* getShapeInstance() { return mShapeInstance; }

    /// @name Network state masks
    /// @{

    ///
    enum ShapeBaseMasks {
        NameMask = Parent::NextFreeMask,
        DamageMask = Parent::NextFreeMask << 1,
        NoWarpMask = Parent::NextFreeMask << 2,
        MountedMask = Parent::NextFreeMask << 3,
        CloakMask = Parent::NextFreeMask << 4,
        ShieldMask = Parent::NextFreeMask << 5,
        InvincibleMask = Parent::NextFreeMask << 6,
        SkinMask = Parent::NextFreeMask << 7,
        SoundMaskN = Parent::NextFreeMask << 8,       ///< Extends + MaxSoundThreads bits
        ThreadMaskN = SoundMaskN << MaxSoundThreads,  ///< Extends + MaxScriptThreads bits
        ImageMaskN = ThreadMaskN << MaxScriptThreads, ///< Extends + MaxMountedImage bits
        NextFreeMask = ImageMaskN << MaxMountedImages
    };

    enum BaseMaskConstants {
        SoundMask = (SoundMaskN << MaxSoundThreads) - SoundMaskN,
        ThreadMask = (ThreadMaskN << MaxScriptThreads) - ThreadMaskN,
        ImageMask = (ImageMaskN << MaxMountedImages) - ImageMaskN
    };

    /// @}

    static bool gRenderEnvMaps; ///< Global flag which turns on or off all environment maps
    static F32  sWhiteoutDec;
    static F32  sDamageFlashDec;

    GFXCubemapHandle mDynamicCubemap;

    /// @name Initialization
    /// @{

    bool onAdd();
    void onRemove();
    void onSceneRemove();
    static void consoleInit();
    bool onNewDataBlock(GameBaseData* dptr);

    /// @}

    /// @name Name & Skin tags
    /// @{
    void setShapeName(const char*);
    const char* getShapeName();
    void setSkinName(const char*);
    const char* getSkinName();
    /// @}

    /// @name Basic attributes
    /// @{

    /// Sets the ammount of damage on this object.
    void setDamageLevel(F32 damage);

    /// Changes the object's damage state.
    /// @param   state   New state of the object
    void setDamageState(DamageState state);

    /// Changes the object's damage state, based on a named state.
    /// @see setDamageState
    /// @param   state   New state of the object as a string.
    bool setDamageState(const char* state);

    /// Returns the name of the current damage state as a string.
    const char* getDamageStateName();

    /// Returns the current damage state.
    DamageState getDamageState() { return mDamageState; }

    /// Returns true if the object is destroyed.
    bool isDestroyed() { return mDamageState == Destroyed; }

    /// Sets the rate at which the object regenerates damage.
    ///
    /// @param  rate  Repair rate in units/second.
    void setRepairRate(F32 rate) { mRepairRate = rate; }

    /// Returns damage amount.
    F32  getDamageLevel() { return mDamage; }

    /// Returns the damage percentage.
    ///
    /// @return Damage factor, between 0.0 - 1.0
    F32  getDamageValue();

    /// Returns the rate at which the object regenerates damage
    F32  getRepairRate() { return mRepairRate; }

    /// Adds damage to an object
    /// @param   amount   Amount of of damage to add
    void applyDamage(F32 amount);

    /// Removes damage to an object
    /// @param   amount   Amount to repair object by
    void applyRepair(F32 amount);

    /// Sets the direction from which the damage is coming
    /// @param   vec   Vector indicating the direction of the damage
    void setDamageDir(const VectorF& vec) { damageDir = vec; }

    /// Sets the level of energy for this object
    /// @param   energy   Level of energy to assign to this object
    virtual void setEnergyLevel(F32 energy);

    /// Sets the rate at which the energy replentishes itself
    /// @param   rate   Rate at which energy restores
    void setRechargeRate(F32 rate) { mRechargeRate = rate; }

    /// Returns the amount of energy in the object
    F32  getEnergyLevel();

    /// Returns the percentage of energy, 0.0 - 1.0
    F32  getEnergyValue();

    /// Returns the recharge rate
    F32  getRechargeRate() { return mRechargeRate; }

    /// @}

    /// @name Script sounds
    /// @{

    /// Plays an audio sound from a mounted object
    /// @param   slot    Mount slot ID
    /// @param   profile Audio profile to play
    void playAudio(U32 slot, SFXProfile* profile);

    /// Stops audio from a mounted object
    /// @param   slot   Mount slot ID
    void stopAudio(U32 slot);
    /// @}

    /// @name Script animation
    /// @{

    /// Sets the animation thread for a mounted object
    /// @param   slot   Mount slot ID
    /// @param    seq   Sequance id
    /// @param   reset   Reset the sequence
    bool setThreadSequence(U32 slot, S32 seq, bool reset = true, F32 timeScale = 1.0f);

    bool setTimeScale(U32 slot, F32 timeScale);

    /// Update the animation thread
    /// @param   st   Thread to update
    void updateThread(Thread& st);

    /// Stop the current thread from playing on a mounted object
    /// @param   slot   Mount slot ID
    bool stopThread(U32 slot);

    /// Pause the running animation thread
    /// @param   slot   Mount slot ID
    bool pauseThread(U32 slot);

    /// Start playing the running animation thread again
    /// @param   slot   Mount slot ID
    bool playThread(U32 slot);

    void playThreadDelayed(const ThreadCmd& cmd);

    /// Toggle the thread as reversed or normal (For example, sidestep-right reversed is sidestep-left)
    /// @param   slot   Mount slot ID
    /// @param   forward   True if the animation is to be played normally
    bool setThreadDir(U32 slot, bool forward);

    /// Start the sound associated with an animation thread
    /// @param   thread   Thread
    void startSequenceSound(Thread& thread);

    /// Stop the sound associated with an animation thread
    /// @param   thread   Thread
    void stopThreadSound(Thread& thread);

    /// Advance all animation threads attached to this shapebase
    /// @param   dt   Change in time from last call to this function
    void advanceThreads(F32 dt);
    /// @}

    /// @name Cloaking
    /// @{

    /// Force uncloaking of object
    /// @param   reason   Reason this is being forced to uncloak, this is passed directly to script control
    void forceUncloak(const char* reason);

    /// Set cloaked state of object
    /// @param   cloaked   True if object is cloaked
    void setCloakedState(bool cloaked);

    /// Returns true if object is cloaked
    bool getCloakedState();

    /// Returns level of cloaking, as it's not an instant "now you see it, now you don't"
    F32 getCloakLevel();
    /// @}

    /// @name Mounted objects
    /// @{

    /// Mount an object to a mount point
    /// @param   obj   Object to mount
    /// @param   node   Mount node ID
    virtual void mountObject(ShapeBase* obj, U32 node);

    /// Remove an object mounting
    /// @param   obj   Object to unmount
    void unmountObject(ShapeBase* obj);

    /// Unmount this object from it's mount
    void unmount();

    /// Return the object that this object is mounted to
    ShapeBase* getObjectMount() { return mMount.object; }

    /// Return object link of next object mounted to this object's mount
    ShapeBase* getMountLink() { return mMount.link; }

    /// Returns the list of things mounted along with this object
    ShapeBase* getMountList() { return mMount.list; }

    /// Returns the mount id that this is mounted to
    U32 getMountNode() { return mMount.node; }

    /// Returns true if this object is mounted to anything at all
    bool isMounted() { return mMount.object != 0; }

    /// Returns the number of object mounted along with this
    S32 getMountedObjectCount();

    /// Returns the object mounted at a position in the mount list
    /// @param   idx   Position on the mount list
    ShapeBase* getMountedObject(S32 idx);

    /// Returns the node the object at idx is mounted to
    /// @param   idx   Index
    S32 getMountedObjectNode(S32 idx);

    /// Returns the object a object on the mount list is mounted to
    /// @param   node
    ShapeBase* getMountNodeObject(S32 node);

    /// Returns where the AI should be to repair this object
    ///
    /// @note Legacy code from Tribes 2, but still works
    Point3F getAIRepairPoint();

    /// @}

    /// @name Mounted Images
    /// @{

    /// Mount an image (ShapeBaseImage) onto an image slot
    /// @param   image   ShapeBaseImage to mount
    /// @param   imageSlot Image mount point
    /// @param   loaded    True if weapon is loaded (it assumes it's a weapon)
    /// @param   skinNameHandle   Skin name for object
    virtual bool mountImage(ShapeBaseImageData* image, U32 imageSlot, bool loaded, StringHandle& skinNameHandle);

    /// Unmount an image from a slot
    /// @param   imageSlot   Mount point
    virtual bool unmountImage(U32 imageSlot);

    /// Gets the information on the image mounted in a slot
    /// @param   imageSlot   Mount point
    ShapeBaseImageData* getMountedImage(U32 imageSlot);

    /// Gets the mounted image on on a slot
    /// @param   imageSlot   Mount Point
    MountedImage* getImageStruct(U32 imageSlot);

    TSShapeInstance* getImageShapeInstance(U32 imageSlot)
    {
        const MountedImage& image = mMountedImageList[imageSlot];
        if (image.dataBlock && image.shapeInstance)
            return image.shapeInstance;
        return NULL;
    }

    /// Gets the next image which will be put in an image slot
    /// @see setImageState
    /// @param   imageSlot   mount Point
    ShapeBaseImageData* getPendingImage(U32 imageSlot);


    /// Returns true if the mounted image is firing
    /// @param   imageSlot   Mountpoint
    bool isImageFiring(U32 imageSlot);

    /// This will return true if, when triggered, the object will fire.
    /// @param   imageSlot   mount point
    /// @param   ns          Used internally for recursion, do not mess with
    /// @param   depth       Used internally for recursion, do not mess with
    bool isImageReady(U32 imageSlot, U32 ns = (U32)-1, U32 depth = 0);

    /// Returns true if the specified image is mounted
    /// @param   image   ShapeBase image
    bool isImageMounted(ShapeBaseImageData* image);

    /// Returns the slot which the image specified is mounted on
    /// @param   image   Image to test for
    S32 getMountSlot(ShapeBaseImageData* image);

    /// Returns the skin for the image in a slot
    /// @param   imageSlot   Image slot to get the skin from
    StringHandle getImageSkinTag(U32 imageSlot);

    /// Returns the image state as a string
    /// @param   imageSlot   Image slot to check state
    const char* getImageState(U32 imageSlot);

    /// Sets the trigger state of the image (Ie trigger pulled down on gun)
    /// @param   imageSlot   Image slot
    /// @param   trigger     True if trigger is down
    void setImageTriggerState(U32 imageSlot, bool trigger);

    /// Returns the trigger state of the image
    /// @param   imageSlot   Image slot
    bool getImageTriggerState(U32 imageSlot);

    /// Sets the flag if the image uses ammo or energy
    /// @param   imageSlot   Image slot
    /// @param   ammo        True if the weapon uses ammo, not energy
    void setImageAmmoState(U32 imageSlot, bool ammo);

    /// Returns true if the image uses ammo, not energy
    /// @param   imageSlot   Image slot
    bool getImageAmmoState(U32 imageSlot);

    /// Sets the image as wet or not, IE if you wanted a gun not to function underwater
    /// @param   imageSlot   Image slot
    /// @param   wet         True if image is wet
    void setImageWetState(U32 imageSlot, bool wet);

    /// Returns true if image is wet
    /// @param   imageSlot   image slot
    bool getImageWetState(U32 imageSlot);

    /// Sets the flag of if the image is loaded with ammo
    /// @param   imageSlot   Image slot
    /// @param   loaded      True if object is loaded with ammo
    void setImageLoadedState(U32 imageSlot, bool loaded);

    /// Returns true if object is loaded with ammo
    /// @param   imageSlot   Image slot
    bool getImageLoadedState(U32 imageSlot);

    /// Modify muzzle, if needed, to aim at whatever is straight in front of eye.
    /// Returns true if result is actually modified.
    /// @param   muzMat   Muzzle transform (in/out)
    /// @param   result   Corrected muzzle vector (out)
    bool getCorrectedAim(const MatrixF& muzMat, VectorF* result);

    /// Gets the muzzle vector of a specified slot
    /// @param   imageSlot   Image slot to check transform for
    /// @param   vec   Muzzle vector (out)
    virtual void getMuzzleVector(U32 imageSlot, VectorF* vec);

    /// Gets the point of the muzzle of the image
    /// @param   imageSlot   Image slot
    /// @param   pos   Muzzle point (out)
    void getMuzzlePoint(U32 imageSlot, Point3F* pos);

    /// @}

    /// @name Transforms
    /// @{

    /// Gets the minimum viewing distance, maximum viewing distance, camera offsetand rotation
    /// for this object, if the world were to be viewed through its eyes
    /// @param   min   Minimum viewing distance
    /// @param   max   Maximum viewing distance
    /// @param   offset Offset of the camera from the origin in local space
    /// @param   rot   Rotation matrix
    virtual void getCameraParameters(F32* min, F32* max, Point3F* offset, MatrixF* rot);

    /// Gets the camera transform
    /// @todo Find out what pos does
    /// @param   pos   TODO: Find out what this does
    /// @param   mat   Camera transform (out)
    virtual void getCameraTransform(F32* pos, MatrixF* mat);

    /// Gets the index of a node inside a mounted image given the name
    /// @param   imageSlot   Image slot
    /// @param   nodeName    Node name
    S32 getNodeIndex(U32 imageSlot, StringTableEntry nodeName);

    /// @}

    /// @name Object Transforms
    /// @{

    /// Returns the eye transform of this shape, IE the eyes of a player
    /// @param   mat   Eye transform (out)
    virtual void getEyeTransform(MatrixF* mat);

    /// The retraction transform is the muzzle transform in world space.
    ///
    /// If the gun is pushed back, for instance, if the player ran against something,
    /// the muzzle of the gun gets pushed back towards the player, towards this location.
    /// @param   imageSlot   Image slot
    /// @param   mat   Transform (out)
    virtual void getRetractionTransform(U32 imageSlot, MatrixF* mat);

    /// Mount point to world space transform
    /// @param   mountPoint   Mount point
    /// @param   mat    mount point transform (out)
    virtual void getMountTransform(U32 mountPoint, MatrixF* mat);

    /// Muzzle transform of mounted object in world space
    /// @param   imageSlot   Image slot
    /// @param   mat         Muzzle transform (out)
    virtual void getMuzzleTransform(U32 imageSlot, MatrixF* mat);

    /// Gets the transform of a mounted image in world space
    /// @param   imageSlot   Image slot
    /// @param   mat    Transform (out)
    virtual void getImageTransform(U32 imageSlot, MatrixF* mat);

    /// Gets the transform of a node on a mounted image in world space
    /// @param   imageSlot   Image Slot
    /// @param   node    node on image
    /// @param   mat   Transform (out)
    virtual void getImageTransform(U32 imageSlot, S32 node, MatrixF* mat);

    /// Gets the transform of a node on a mounted image in world space
    /// @param   imageSlot   Image Slot
    /// @param   nodeName    Name of node on image
    /// @param   mat         Transform (out)
    virtual void getImageTransform(U32 imageSlot, StringTableEntry nodeName, MatrixF* mat);

    ///@}

    /// @name Render transforms
    /// Render transforms are different from object transforms in that the render transform of an object
    /// is where, in world space, the object is actually rendered. The object transform is the
    /// absolute position of the object, as in where it should be.
    ///
    /// The render transforms typically vary from object transforms due to client side prediction.
    ///
    /// Other than that, these functions are identical to their object-transform counterparts
    ///
    /// @note These are meaningless on the server.
    /// @{
    virtual void getRenderRetractionTransform(U32 index, MatrixF* mat);
    virtual void getRenderMountTransform(U32 index, MatrixF* mat);
    virtual void getRenderMuzzleTransform(U32 index, MatrixF* mat);
    virtual void getRenderImageTransform(U32 imageSlot, MatrixF* mat);
    virtual void getRenderImageTransform(U32 index, S32 node, MatrixF* mat);
    virtual void getRenderImageTransform(U32 index, StringTableEntry nodeName, MatrixF* mat);
    virtual void getRenderMuzzleVector(U32 imageSlot, VectorF* vec);
    virtual void getRenderMuzzlePoint(U32 imageSlot, Point3F* pos);
    virtual void getRenderEyeTransform(MatrixF* mat);
    /// @}



    /// @name Screen Flashing
    /// @{

    /// Returns the level of screenflash that should be used
    virtual F32  getDamageFlash() const;

    /// Sets the flash level
    /// @param   amt   Level of flash
    virtual void setDamageFlash(const F32 amt);

    /// White out is the flash-grenade blindness effect
    /// This returns the level of flash to create
    virtual F32  getWhiteOut() const;

    /// Set the level of flash blindness
    virtual void setWhiteOut(const F32);
    /// @}

    /// @name Invincibility effect
    /// This is the screen effect when invincible in the HUD
    /// @see GameRenderFilters()
    /// @{

    /// Returns the level of invincibility effect
    virtual F32 getInvincibleEffect() const;

    /// Initializes invincibility effect and interpolation parameters
    ///
    /// @param   time   Time it takes to become invincible
    /// @param   speed  Speed at which invincibility effects progress
    virtual void setupInvincibleEffect(F32 time, F32 speed);

    /// Advance invincibility effect animation
    /// @param   dt   Time since last call of this function
    virtual void updateInvincibleEffect(F32 dt);

    /// @}

    /// @name Movement & velocity
    /// @{

    /// Sets the velocity of this object
    /// @param   vel   Velocity vector
    virtual void setVelocity(const VectorF& vel);

    /// Applies an impulse force to this object
    /// @param   pos   Position where impulse came from in world space
    /// @param   vec   Velocity vector (Impulse force F = m * v)
    virtual void applyImpulse(const Point3F& pos, const VectorF& vec);

    virtual F32 getMass() const;

    /// @}

    /// @name Cameras and Control
    /// @{

    /// Assigns this object a controling client
    /// @param   client   New controling client
    //virtual void setControllingClient(GameConnection* client);

    /// Returns the client controling this object
    //GameConnection* getControllingClient() { return mControllingClient; }

    /// Returns the object controling this object
    ShapeBase* getControllingObject() { return mControllingObject; }

    /// Sets the controling object
    /// @param   obj   New controling object
    virtual void setControllingObject(ShapeBase* obj);

    /// Returns the object this is controling
    virtual ShapeBase* getControlObject();

    /// sets the object this is controling
    /// @param   obj   New controlled object
    virtual void setControlObject(ShapeBase* obj);

    /// Returns true if this object is controled by something
    bool isControlled() { return(mIsControlled); }

    /// Returns true if this object is being used as a camera in first person
    bool isFirstPerson();

    /// Returns true if the camera uses this objects eye point (defined by modeler)
    bool useObjsEyePoint() const;

    /// Returns true if this object can only be used as a first person camera
    bool onlyFirstPerson() const;

    /// Returns the Field of Vision for this object if used as a camera
    virtual F32 getCameraFov();

    /// Returns the default FOV if this object is used as a camera
    virtual F32 getDefaultCameraFov();

    /// Sets the FOV for this object if used as a camera
    virtual void setCameraFov(F32 fov);

    /// Returns true if the FOV supplied is within allowable parameters
    /// @param   fov   FOV to test
    virtual bool isValidCameraFov(F32 fov);
    /// @}


    void processTick(const Move* move);
    void advanceTime(F32 dt);

    /// @name Rendering
    /// @{

    /// Returns the renderable shape of this object
    TSShape const* getShape();

    bool prepRenderImage(SceneState* state, const U32 stateKey, const U32 startZone, const bool modifyBaseZoneState);
    void prepBatchRender(SceneState* state, S32 mountedImageIndex);
    void renderObject(SceneState* state, RenderInst*);
    void renderShadow(SceneState* state, RenderInst* ri);

#ifdef MB_ULTRA
    virtual MatrixF getShadowTransform() const { return mRenderObjToWorld; }
    virtual Point3F getShadowScale() const { return mRenderScale; }
#endif

    virtual Material* getMaterial(U32 material);

    /// Renders a mounted object
    /// @param   state   State of scene
    /// @param   ri      RenderInst data
    virtual void renderMountedImage(SceneState* state, RenderInst* ri) {};

    virtual void renderImage(SceneState* state) {};

    /// Renders the shadow for this object
    /// @param   dist   Distance away from object shadow is rendering on
    /// @param   fogAmount  Amount of fog present
    //void renderShadow(F32 dist, F32 fogAmount);

    /// Draws a wire cube at any point in space with specified parameters
    /// @param   size   Length, width, depth
    /// @param   pos    xyz position in world space
    static void wireCube(const Point3F& size, const Point3F& pos);

    /// This is a callback for objects that have reflections and are added to
    /// the "reflectiveSet" SimSet.
    virtual void updateReflection();

    /// Preprender logic
    virtual void calcClassRenderData();
    /// @}

    /// Control object scoping
    void onCameraScopeQuery(NetConnection* cr, CameraScopeQuery* camInfo);

    /// @name Collision
    /// @{

    /// Casts a ray from start to end, stores gathered information in 'info' returns true if successful
    /// @param   start   Start point for ray
    /// @param   end     End point for ray
    /// @param   info    Information from raycast (out)
    bool castRay(const Point3F& start, const Point3F& end, RayInfo* info);

    /// Builds a polylist of the polygons in this object returns true if successful
    /// @param   polyList   Returned polylist (out)
    /// @param   box        Not used
    /// @param   sphere     Not used
    bool buildPolyList(AbstractPolyList* polyList, const Box3F& box, const SphereF& sphere);

    /// Builds a convex hull for this object @see Convex
    /// @param   box   Bounding box
    /// @param   convex  New convex hull (out)
    void buildConvex(const Box3F& box, Convex* convex);

    /// @}

    /// @name Rendering
    /// @{

    /// Increments the last rendered frame number
    static void incRenderFrame() { sLastRenderFrame++; }

    /// Returns true if the last frame calculated rendered
    bool didRenderLastRender() { return mLastRenderFrame == sLastRenderFrame; }

    /// Sets the state of this object as hidden or not. If an object is hidden
    /// it is removed entirely from collisions, it is not ghosted and is
    /// essentially "non existant" as far as simulation is concerned.
    /// @param   hidden   True if object is to be hidden
    virtual void setHidden(bool hidden);

    virtual void onUnhide();

    U32 getCreateTime() { return mCreateTime; }

    /// Returns true if this object can be damaged
    bool isInvincible();

    /// Start fade of object in/out
    /// @param   fadeTime Time fade should take
    /// @param   fadeDelay Delay before starting fade
    /// @param   fadeOut   True if object is fading out, false if fading in.
    void startFade(F32 fadeTime, F32 fadeDelay = 0.0, bool fadeOut = true);

    /// Traverses mounted objects and registers light sources with the light manager
    /// @param   lightManager   Light manager to register with
    /// @param   lightingScene  Set to true if the scene is being lit, in which case these lights will not be used
    void registerLights(LightManager* lightManager, bool lightingScene);
    /// @}

    /// Returns true if the point specified is in the water
    /// @param   point    Point to test in world space
    bool pointInWater(Point3F& point);

    /// Returns the percentage of this object covered by water
    F32 getWaterCoverage() { return mWaterCoverage; }

    /// Returns the height of the liquid on this object
    F32 getLiquidHeight() { return mLiquidHeight; }

    /// @name Network
    /// @{

    F32 getUpdatePriority(CameraScopeQuery* focusObject, U32 updateMask, S32 updateSkips);
    U32  packUpdate(NetConnection* conn, U32 mask, BitStream* stream);
    void unpackUpdate(NetConnection* conn, BitStream* stream);
    void writePacketData(GameConnection* conn, BitStream* stream);
    void readPacketData(GameConnection* conn, BitStream* stream);
    virtual U32 getPacketDataChecksum(GameConnection* conn);

    /// @}

    DECLARE_CONOBJECT(ShapeBase);
};


//------------------------------------------------------------------------------
// inlines
//------------------------------------------------------------------------------

inline bool ShapeBase::getCloakedState()
{
    return(mCloaked);
}

inline F32 ShapeBase::getCloakLevel()
{
    return(mCloakLevel);
}

inline const char* ShapeBase::getShapeName()
{
    return mShapeNameHandle.getString();
}

inline const char* ShapeBase::getSkinName()
{
    return mSkinNameHandle.getString();
}

/// @name Shadow Detail Constants
///
/// The generic shadow level is the shadow detail at which
/// a generic shadow is drawn (a disk) rather than a generated
/// shadow. The no shadow level is the shadow level at which
/// no shadow is drawn. (Shadow level goes from 0 to 1,
/// higher numbers result in more detailed shadows).
///
/// @{

#define Player_GenericShadowLevel 0.4f
#define Player_NoShadowLevel 0.01f
#define Vehicle_GenericShadowLevel 0.7f
#define Vehicle_NoShadowLevel 0.2f
#define Item_GenericShadowLevel 0.4f
#define Item_NoShadowLevel 0.01f
#define StaticShape_GenericShadowLevel 2.0f
#define StaticShape_NoShadowLevel 2.0f

/// @}
#endif  // _H_SHAPEBASE_
