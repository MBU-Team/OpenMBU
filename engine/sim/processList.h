#ifndef _PROCESSLIST_H_
#define _PROCESSLIST_H_

#include "platform/platform.h"
#include "console/simBase.h"

#include "game/gameBase.h"

#define TickShift   5
#define TickMs      (1 << TickShift)
#define TickSec     (F32(TickMs) / 1000.0f)
#define TickMask    (TickMs - 1)

/// List to keep track of GameBases to process.
class ProcessList
{
    GameBase head;
    U32 mCurrentTag;
    SimTime mLastTick;
    SimTime mLastTime;
    SimTime mLastDelta;
    bool mIsServer;
    bool mDirty;
    static bool mDebugControlSync;

    void orderList();
    void advanceObjects();

public:
    SimTime getLastTime() { return mLastTime; }
    ProcessList(bool isServer);
    void markDirty() { mDirty = true; }
    bool isDirty() { return mDirty; }
    void setDirty(bool dirty) { mDirty = dirty; }
    void addObject(GameBase* obj) {
        obj->plLinkBefore(&head);
    }

    F32 getLastDelta() { return mLastDelta; }
    F32 getLastInterpDelta() { return mLastDelta / F32(TickMs); }

    void dumpToConsole();

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