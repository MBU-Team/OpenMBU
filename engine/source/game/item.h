//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _ITEM_H_
#define _ITEM_H_

#ifndef _SHAPEBASE_H_
#include "game/shapeBase.h"
#endif
#ifndef _BOXCONVEX_H_
#include "collision/boxConvex.h"
#endif

//----------------------------------------------------------------------------

class TSStatic;

struct ItemData : public ShapeBaseData {
    typedef ShapeBaseData Parent;

    F32 friction;
    F32 elasticity;

    bool sticky;
    F32  gravityMod;
    F32  maxVelocity;

#ifdef MB_ULTRA
    bool addToHUDRadar;
#endif

    S32 dynamicTypeField;

    StringTableEntry pickUpName;

#ifdef MARBLE_BLAST
    S32 powerupId;
#endif
#ifdef MB_ULTRA
    StringTableEntry buddyShapeName;
    StringTableEntry buddySequence;
#endif

    bool        lightOnlyStatic;
    S32         lightType;
    ColorF      lightColor;
    S32         lightTime;
    F32         lightRadius;

#ifdef MB_ULTRA
    F32 scaleFactor;
#endif

    ItemData();
    DECLARE_CONOBJECT(ItemData);
    static void initPersistFields();
    virtual void packData(BitStream* stream);
    virtual void unpackData(BitStream* stream);
};


//----------------------------------------------------------------------------

class Item : public ShapeBase
{
    typedef ShapeBase Parent;

    enum MaskBits {

#ifndef ITEM_FIX_SET_HIDDEN
        HiddenMask = Parent::NextFreeMask,
#endif
        ThrowSrcMask = Parent::NextFreeMask << 1,
        PositionMask = Parent::NextFreeMask << 2,
        RotationMask = Parent::NextFreeMask << 3,
        NextFreeMask = Parent::NextFreeMask << 4
    };

    // Client interpolation data
    struct StateDelta {
        Point3F pos;
        VectorF posVec;
        S32 warpTicks;
        Point3F warpOffset;
        F32     dt;
    };
    StateDelta delta;

    // Static attributes
    ItemData* mDataBlock;
    static F32 mGravity;
    bool mCollideable;
    bool mStatic;
    bool mRotate;
    bool mRotate2;

#ifdef MARBLE_BLAST
    bool mPermanent;
    unsigned int mHiddenTimer;
#endif

    //
    VectorF mVelocity;
    bool mAtRest;

    S32 mAtRestCounter;
    static const S32 csmAtRestTimer;

    bool mInLiquid;

    ShapeBase* mCollisionObject;
    U32 mCollisionTimeout;

#ifdef MB_ULTRA
    SimObjectPtr<TSStatic> mBuddy;
    bool mBuddyOn;
#endif

public:

    void registerLights(LightManager* lightManager, bool lightingScene);
    enum LightType
    {
        NoLight = 0,
        ConstantLight,
        PulsingLight,

        NumLightTypes,
    };

private:
    S32         mDropTime;
    LightInfo   mLight;

public:

    Point3F mStickyCollisionPos;
    Point3F mStickyCollisionNormal;

    //
private:
    OrthoBoxConvex mConvex;
    Box3F          mWorkingQueryBox;

    void updateVelocity(const F32 dt);
    void updatePos(const U32 mask, const F32 dt);
    void updateWorkingCollisionSet(const U32 mask, const F32 dt);
    bool buildPolyList(AbstractPolyList* polyList, const Box3F& box, const SphereF& sphere);
    void buildConvex(const Box3F& box, Convex* convex);
    void onDeleteNotify(SimObject*);

    bool prepRenderImage(SceneState* state, const U32 stateKey, const U32 startZone, const bool modifyBaseZoneState);
    void advanceTime(F32 dt);

public:
    DECLARE_CONOBJECT(Item);


    Item();
    ~Item();
    static void initPersistFields();
    static void consoleInit();

    bool onAdd();
    void onRemove();
    bool onNewDataBlock(GameBaseData* dptr);

    bool isStatic() { return mStatic; }
    bool isRotating() { return mRotate || mRotate2; }
    Point3F getVelocity() const;
    void setVelocity(const VectorF& vel);
    void applyImpulse(const Point3F& pos, const VectorF& vec);
    void setCollisionTimeout(ShapeBase* obj);
    ShapeBase* getCollisionObject() { return mCollisionObject; };

    void processTick(const Move* move);
    void interpolateTick(F32 delta);
    void setTransform(const MatrixF& mat);
    void renderImage(SceneState* state);

    U32  packUpdate(NetConnection* conn, U32 mask, BitStream* stream);
    void unpackUpdate(NetConnection* conn, BitStream* stream);
    
    virtual void writePacketData(GameConnection* conn, BitStream* stream);
    virtual void readPacketData(GameConnection* conn, BitStream* stream);

#ifdef MARBLE_BLAST
    virtual void setHidden(bool hidden);
#endif

#ifdef MB_ULTRA
    void setClientHidden(U32 timer);
    void createBuddy();
    void destroyBuddy();
    void setBuddy(bool on);
#endif
};

#endif
