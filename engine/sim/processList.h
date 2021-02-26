#ifndef _PROCESSLIST_H_
#define _PROCESSLIST_H_

#include "platform/platform.h"
#include "console/simBase.h"

//----------------------------------------------------------------------------

#define TickShift   5
#define TickMs      (1 << TickShift)
#define TickSec     (F32(TickMs) / 1000.0f)
#define TickMask    (TickMs - 1)

//----------------------------------------------------------------------------

class ProcessObject
{
    friend class ProcessList;
    friend class ProcessList;
    friend class ClientProcessList;
    friend class ServerProcessList;
    friend class SPModeProcessList;

public:

    ProcessObject() { mProcessTag = 0; mProcessLink.next = mProcessLink.prev = this; mOrderGUID = 0; }
    virtual ProcessObject* getAfterObject() { return NULL; }

protected:

    struct Link
    {
        ProcessObject* next;
        ProcessObject* prev;
    };

    // Processing interface
    void plUnlink();
    void plLinkAfter(ProcessObject*);
    void plLinkBefore(ProcessObject*);
    void plJoin(ProcessObject*);

    U32 mProcessTag;                       // Tag used during sort
    U32 mOrderGUID;                        // UID for keeping order synced (e.g., across network or runs of sim)
    Link mProcessLink;                     // Ordered process queue
};

/// List to keep track of GameBases to process.
class ProcessList
{

public:
    ProcessList();

    void markDirty() { mDirty = true; }
    bool isDirty() { return mDirty; }
    void setDirty(bool dirty) { mDirty = dirty; }

    virtual void addObject(ProcessObject* obj);

    SimTime getLastTime() { return mLastTime; }
    F32 getLastDelta() { return mLastDelta; }
    F32 getLastInterpDelta() { return mLastDelta / F32(TickMs); }
    U32 getTotalTicks() { return mTotalTicks; }
    void dumpToConsole();

    /// @name Advancing Time
    /// The advance time functions return true if a tick was processed.
    ///
    /// These functions go through either gServerProcessList or gClientProcessList and
    /// call each GameBase's processTick().
    /// @{

    bool advanceTime(SimTime timeDelta);
    //bool advanceServerTime(SimTime timeDelta);
    //bool advanceClientTime(SimTime timeDelta);
    //bool advanceSPModeTime(SimTime timeDelta);

    /// @}

protected:

    ProcessObject mHead;

    U32 mCurrentTag;
    bool mDirty;

    U32 mTotalTicks;
    SimTime mLastTick;
    SimTime mLastTime;
    F32 mLastDelta;

    void orderList();
    virtual void advanceObjects();
    virtual void onAdvanceObjects() { advanceObjects(); }
    virtual void onTickObject(ProcessObject*) {}
};

#endif // _PROCESSLIST_H_