//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _H_TRIGGER
#define _H_TRIGGER

#ifndef _PLATFORM_H_
#include "platform/platform.h"
#endif
#ifndef _GAMEBASE_H_
#include "game/gameBase.h"
#endif
#ifndef _MBOX_H_
#include "math/mBox.h"
#endif
#ifndef _EARLYOUTPOLYLIST_H_
#include "collision/earlyOutPolyList.h"
#endif

class Convex;

DefineConsoleType(TypeTriggerPolyhedron)

struct TriggerData : public GameBaseData {
    typedef GameBaseData Parent;

public:
    S32  tickPeriodMS;
#ifdef MB_ULTRA
    bool marbleGravityCheck;
#endif

    TriggerData();
    DECLARE_CONOBJECT(TriggerData);
    bool onAdd();
    static void initPersistFields();
    virtual void packData(BitStream* stream);
    virtual void unpackData(BitStream* stream);
};

class Trigger : public GameBase
{
    typedef GameBase Parent;

    Polyhedron        mTriggerPolyhedron;
    EarlyOutPolyList  mClippedList;
    Vector<GameBase*> mObjects;

    TriggerData* mDataBlock;

    U32               mLastThink;
    U32               mCurrTick;

protected:
    bool onAdd();
    void onRemove();
    void onDeleteNotify(SimObject*);
    bool onNewDataBlock(GameBaseData* dptr);
    void onEditorEnable();
    void onEditorDisable();

    bool testObject(GameBase* enter);
    void processTick(const Move* move);

    Convex* mConvexList;
    void buildConvex(const Box3F& box, Convex* convex);

    // Rendering
protected:
    bool prepRenderImage(SceneState* state, const U32 stateKey, const U32 startZone, const bool modifyBaseZoneState = false);
    void renderObject(SceneState* state);

public:
    void setTransform(const MatrixF& mat);

public:
    Trigger();
    ~Trigger();

    void setTriggerPolyhedron(const Polyhedron&);

    void      potentialEnterObject(GameBase*);
    U32       getNumTriggeringObjects() const;
    GameBase* getObject(const U32);

    DECLARE_CONOBJECT(Trigger);
    static void initPersistFields();

    U32  packUpdate(NetConnection* conn, U32 mask, BitStream* stream);
    void unpackUpdate(NetConnection* conn, BitStream* stream);

    bool castRay(const Point3F& start, const Point3F& end, RayInfo* info);
};

inline U32 Trigger::getNumTriggeringObjects() const
{
    return mObjects.size();
}

inline GameBase* Trigger::getObject(const U32 index)
{
    AssertFatal(index < getNumTriggeringObjects(), "Error, out of range object index");

    return mObjects[index];
}

#endif // _H_TRIGGER

