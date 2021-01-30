//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _H_PARTICLE_EMITTER
#define _H_PARTICLE_EMITTER

#ifndef _GAMEBASE_H_
#include "game/gameBase.h"
#endif
#ifndef _COLOR_H_
#include "core/color.h"
#endif

#include "particle.h"
#include "gfx/gfxDevice.h"

class  ParticleData;

//*****************************************************************************
// Particle Emitter Data
//*****************************************************************************
class ParticleEmitterData : public GameBaseData
{
    typedef GameBaseData Parent;

public:
    ParticleEmitterData();
    DECLARE_CONOBJECT(ParticleEmitterData);
    static void initPersistFields();
    void packData(BitStream* stream);
    void unpackData(BitStream* stream);
    bool preload(bool server, char errorBuffer[256]);
    bool onAdd();
    void allocPrimBuffer(S32 overrideSize = -1);

public:
    S32   ejectionPeriodMS;                   ///< Time, in Miliseconds, between particle ejection
    S32   periodVarianceMS;                   ///< Varience in ejection peroid between 0 and n

    F32   ejectionVelocity;                   ///< Ejection velocity
    F32   velocityVariance;                   ///< Variance for velocity between 0 and n
    F32   ejectionOffset;                     ///< Z offset from emitter point to eject from

    F32   thetaMin;                           ///< Minimum angle, from the horizontal plane, to eject from
    F32   thetaMax;                           ///< Maximum angle, from the horizontal plane, to eject from

    F32   phiReferenceVel;                    ///< Reference angle, from the verticle plane, to eject from
    F32   phiVariance;                        ///< Varience from the reference angle, from 0 to n

    U32   lifetimeMS;                         ///< Lifetime of particles
    U32   lifetimeVarianceMS;                 ///< Varience in lifetime from 0 to n

    bool  overrideAdvance;                    ///<
    bool  orientParticles;                    ///< Particles always face the screen
    bool  orientOnVelocity;                   ///< Particles face the screen at the start
    bool  useEmitterSizes;                    ///< Use emitter specified sizes instead of datablock sizes
    bool  useEmitterColors;                   ///< Use emitter specified colors instead of datablock colors

    StringTableEntry      particleString;     ///< Used to load particle data directly from a string
    ParticleData* particleDataBlock;  ///< Datablock for particles
    U32                   partDataID;
    U32                   partListInitSize;   /// initial size of particle list calc'd from datablock info

    GFXPrimitiveBufferHandle   primBuff;

};

DECLARE_CONSOLETYPE(ParticleEmitterData)

//*****************************************************************************
// Particle Emitter
//*****************************************************************************
class ParticleEmitter : public GameBase
{
    typedef GameBase Parent;

public:
    ParticleEmitter();
    ~ParticleEmitter();

    static Point3F mWindVelocity;
    static void setWindVelocity(const Point3F& vel) { mWindVelocity = vel; }

    ColorF getCollectiveColor();

    /// Sets sizes of particles based on sizelist provided
    /// @param   sizeList   List of sizes
    void setSizes(F32* sizeList);

    /// Sets colors for particles based on color list provided
    /// @param   colorList   List of colors
    void setColors(ColorF* colorList);

    ParticleEmitterData* getDataBlock() { return mDataBlock; }
    bool onNewDataBlock(GameBaseData* dptr);

    /// By default, a particle renderer will wait for it's owner to delete it.  When this
    /// is turned on, it will delete itself as soon as it's particle count drops to zero.
    void deleteWhenEmpty();

    /// @name Particle Emission
    /// Main interface for creating particles.  The emitter does _not_ track changes
    ///  in axis or velocity over the course of a single update, so this should be called
    ///  at a fairly fine grain.  The emitter will potentially track the last particle
    ///  to be created into the next call to this function in order to create a uniformly
    ///  random time distribution of the particles.  If the object to which the emitter is
    ///  attached is in motion, it should try to ensure that for call (n+1) to this
    ///  function, start is equal to the end from call (n).  This will ensure a uniform
    ///  spatial distribution.
    /// @{

    void emitParticles(const Point3F& start,
        const Point3F& end,
        const Point3F& axis,
        const Point3F& velocity,
        const U32      numMilliseconds);
    void emitParticles(const Point3F& point,
        const bool     useLastPosition,
        const Point3F& axis,
        const Point3F& velocity,
        const U32      numMilliseconds);
    void emitParticles(const Point3F& rCenter,
        const Point3F& rNormal,
        const F32      radius,
        const Point3F& velocity,
        S32 count);
    /// @}

    bool mDead;

protected:
    /// @name Internal interface
    /// @{

    /// Adds a particle
    /// @param   pos   Initial position of particle
    /// @param   axis
    /// @param   vel   Initial velocity
    /// @param   axisx
    void addParticle(const Point3F& pos, const Point3F& axis, const Point3F& vel, const Point3F& axisx);


    inline void setupBillboard(Particle* part,
        Point3F* basePts,
        MatrixF camView,
        GFXVertexPCT* lVerts);

    inline void setupOriented(Particle* part,
        const Point3F& camPos,
        GFXVertexPCT* lVerts);

    /// Updates the bounding box for the particle system
    void updateBBox();

    /// @}
protected:
    bool onAdd();
    void onRemove();

    void processTick(const Move* move);
    void advanceTime(F32 dt);

    // Rendering
protected:
    bool prepRenderImage(SceneState* state, const U32 stateKey, const U32 startZone, const bool modifyBaseZoneState);
    void prepBatchRender(const Point3F& camPos);
    void copyToVB(const Point3F& camPos);

    // PEngine interface
private:

    void update(U32 ms);
    inline void updateKeyData(Particle* part);


private:
    ParticleEmitterData* mDataBlock;

    U32       mInternalClock;

    U32       mNextParticleTime;

    Point3F   mLastPosition;
    bool      mHasLastPosition;

    bool      mDeleteWhenEmpty;
    bool      mDeleteOnTick;

    S32       mLifetimeMS;
    S32       mElapsedTimeMS;

    F32       sizes[ParticleData::PDC_NUM_KEYS];
    ColorF    colors[ParticleData::PDC_NUM_KEYS];

    GFXVertexBufferHandle<GFXVertexPCT> mVertBuff;
    Vector <Particle> mPartList;
    S32       mLastPartIndex;
    S32       mCurBuffSize;

};

#endif // _H_PARTICLE_EMITTER

