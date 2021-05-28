#ifndef _PROCESSLIST_H_
#define _PROCESSLIST_H_

#include "platform/platform.h"
#include "console/simBase.h"

#define TickShift   5
#define TickMs      (1 << TickShift)
#define TickSec     (F32(TickMs) / 1000.0f)
#define TickMask    (TickMs - 1)

class GameConnection;
class GameBase;

class ProcessObject
{
    friend class ProcessList;
protected:
    struct Link
    {
        ProcessObject* next;
        ProcessObject* prev;
    };
    U32 mProcessTag;            ///< Tag used to sort objects for processing.
    U32 mNetOrderId;                    
    Link mProcessLink;          ///< Ordered process queue link.                     
    SimObjectPtr<GameBase> mAfterObject;

public:
    ProcessObject();

protected:
    void plUnlink();
    void plLinkAfter(ProcessObject*);
    void plLinkBefore(ProcessObject*);
    void plJoin(ProcessObject*);
};

/// List to keep track of GameBases to process.
class ProcessList
{
    ProcessObject mHead;
    U32 mCurrentTag;
    F32 mLastDelta;
    SimTime mLastTick;
    SimTime mLastTime;
    SimTime mSkipAdvanceObjectsMs;
    bool mIsServer;
    bool mDirty;
    bool mForceHifiReset;
    SimTime mTotalTicks;
    static bool mDebugControlSync;

    void orderList();
    void advanceObjects();

public:
    SimTime getLastTime() { return mLastTime; }
    ProcessList(bool isServer);
    void markDirty() { mDirty = true; }
    bool isDirty() { return mDirty; }
    void setDirty(bool dirty) { mDirty = dirty; }
    void addObject(GameBase* obj);
    GameBase* getGameBase(ProcessObject* obj);

    F32 getLastDelta() { return mLastDelta; }

    void dumpToConsole();

    void skipAdvanceObjects(U32 ms) { mSkipAdvanceObjectsMs+= ms; }
    void ageTickCache(S32 numToAge, S32 len);
    void updateMoveSync(S32 moveDiff);
    void clientCatchup(GameConnection* connection, S32 catchup);
    void forceHifiReset(bool reset) { mForceHifiReset = reset; }
    SimTime getTotalTicks() { return mTotalTicks; }


    /// @name Advancing Time
    /// The advance time functions return true if a tick was processed.
    ///
    /// These functions go through either gServerProcessList or gClientProcessList and
    /// call each GameBase's processTick().
    /// @{

    bool advanceServerTime(SimTime timeDelta);
    bool advanceClientTime(SimTime timeDelta);
    bool advanceSPModeTime(SimTime timeDelta);

    /// @}
};

#endif // _PROCESSLIST_H_