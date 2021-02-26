//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "core/dnet.h"
#include "game/gameConnection.h"
#include "game/gameBase.h"
#include "game/shapeBase.h"

#include "sim/processList.h"
#include "platform/profiler.h"
#include "console/consoleTypes.h"

//-----------------------------------------------------------------------------

void ProcessObject::plUnlink()
{
    mProcessLink.next->mProcessLink.prev = mProcessLink.prev;
    mProcessLink.prev->mProcessLink.next = mProcessLink.next;
    mProcessLink.next = mProcessLink.prev = this;
}

void ProcessObject::plLinkAfter(ProcessObject* obj)
{
    AssertFatal(mProcessLink.next == this && mProcessLink.prev == this, "ProcessObject::plLinkAfter: must be unlinked before calling this");
#ifdef TORQUE_DEBUG
    ProcessObject* test1 = obj;
    ProcessObject* test2 = obj->mProcessLink.next;
    ProcessObject* test3 = obj->mProcessLink.prev;
    ProcessObject* test4 = this;
#endif

    // Link this after obj
    mProcessLink.next = obj->mProcessLink.next;
    mProcessLink.prev = obj;
    obj->mProcessLink.next = this;
    mProcessLink.next->mProcessLink.prev = this;

#ifdef TORQUE_DEBUG
    AssertFatal(test1->mProcessLink.next->mProcessLink.prev == test1 && test1->mProcessLink.prev->mProcessLink.next == test1, "Doh!");
    AssertFatal(test2->mProcessLink.next->mProcessLink.prev == test2 && test2->mProcessLink.prev->mProcessLink.next == test2, "Doh!");
    AssertFatal(test3->mProcessLink.next->mProcessLink.prev == test3 && test3->mProcessLink.prev->mProcessLink.next == test3, "Doh!");
    AssertFatal(test4->mProcessLink.next->mProcessLink.prev == test4 && test4->mProcessLink.prev->mProcessLink.next == test4, "Doh!");
#endif
}

void ProcessObject::plLinkBefore(ProcessObject* obj)
{
    AssertFatal(mProcessLink.next == this && mProcessLink.prev == this, "ProcessObject::plLinkBefore: must be unlinked before calling this");
#ifdef TORQUE_DEBUG
    ProcessObject* test1 = obj;
    ProcessObject* test2 = obj->mProcessLink.next;
    ProcessObject* test3 = obj->mProcessLink.prev;
    ProcessObject* test4 = this;
#endif

    // Link this before obj
    mProcessLink.next = obj;
    mProcessLink.prev = obj->mProcessLink.prev;
    obj->mProcessLink.prev = this;
    mProcessLink.prev->mProcessLink.next = this;

#ifdef TORQUE_DEBUG
    AssertFatal(test1->mProcessLink.next->mProcessLink.prev == test1 && test1->mProcessLink.prev->mProcessLink.next == test1, "Doh!");
    AssertFatal(test2->mProcessLink.next->mProcessLink.prev == test2 && test2->mProcessLink.prev->mProcessLink.next == test2, "Doh!");
    AssertFatal(test3->mProcessLink.next->mProcessLink.prev == test3 && test3->mProcessLink.prev->mProcessLink.next == test3, "Doh!");
    AssertFatal(test4->mProcessLink.next->mProcessLink.prev == test4 && test4->mProcessLink.prev->mProcessLink.next == test4, "Doh!");
#endif
}

void ProcessObject::plJoin(ProcessObject* head)
{
    ProcessObject* tail1 = head->mProcessLink.prev;
    ProcessObject* tail2 = mProcessLink.prev;
    tail1->mProcessLink.next = this;
    mProcessLink.prev = tail1;
    tail2->mProcessLink.next = head;
    head->mProcessLink.prev = tail2;
}

//-----------------------------------------------------------------------------

ProcessList::ProcessList()
{
    mCurrentTag = 0;
    mDirty = false;

    mTotalTicks = 0;
    mLastTick = 0;
    mLastTime = 0;
    mLastDelta = 0.0f;
}

void ProcessList::addObject(ProcessObject* obj) {
    obj->plLinkAfter(&mHead);
}


//----------------------------------------------------------------------------

void ProcessList::orderList()
{
    // ProcessObject tags are initialized to 0, so current tag should never be 0.
    if (++mCurrentTag == 0)
        mCurrentTag++;

    // Install a temporary head node
    ProcessObject list;
    list.plLinkBefore(mHead.mProcessLink.next);
    mHead.plUnlink();

    // start out by (bubble) sorting list by GUID
    for (ProcessObject* cur = list.mProcessLink.next; cur != &list; cur = cur->mProcessLink.next)
    {
        if (cur->mOrderGUID == 0)
            // special case -- can be no lower, so accept as lowest (this is also
            // a common value since it is what non ordered objects have)
            continue;

        for (ProcessObject* walk = cur->mProcessLink.next; walk != &list; walk = walk->mProcessLink.next)
        {
            if (walk->mOrderGUID < cur->mOrderGUID)
            {
                // swap walk and cur -- need to be careful because walk might be just after cur
                // so insert after item before cur and before item after walk
                ProcessObject* before = cur->mProcessLink.prev;
                ProcessObject* after = walk->mProcessLink.next;
                cur->plUnlink();
                walk->plUnlink();
                cur->plLinkBefore(after);
                walk->plLinkAfter(before);
                ProcessObject* swap = walk;
                walk = cur;
                cur = swap;
            }
        }
    }

    // Reverse topological sort into the original head node
    while (list.mProcessLink.next != &list)
    {
        ProcessObject* ptr = list.mProcessLink.next;
        ProcessObject* afterObject = ptr->getAfterObject();
        ptr->mProcessTag = mCurrentTag;
        ptr->plUnlink();
        if (afterObject)
        {
            // Build chain "stack" of dependent objects and patch
            // it to the end of the current list.
            while (afterObject && afterObject->mProcessTag != mCurrentTag)
            {
                afterObject->mProcessTag = mCurrentTag;
                afterObject->plUnlink();
                afterObject->plLinkBefore(ptr);
                ptr = afterObject;
                afterObject = ptr->getAfterObject();
            }
            ptr->plJoin(&mHead);
        }
        else
            ptr->plLinkBefore(&mHead);
    }
    mDirty = false;
}

void ProcessList::dumpToConsole()
{
    for (ProcessObject* pobj = mHead.mProcessLink.next; pobj != &mHead; pobj = pobj->mProcessLink.next)
    {
        SimObject* obj = dynamic_cast<SimObject*>(pobj);
        if (obj)
            Con::printf("id %i, order guid %i, type %s", obj->getId(), pobj->mOrderGUID, obj->getClassName());
        else
            Con::printf("---unknown object type, order guid %i", pobj->mOrderGUID);
    }
}


//----------------------------------------------------------------------------

bool ProcessList::advanceTime(SimTime timeDelta)
{
    PROFILE_START(AdvanceTime);

    if (mDirty)
        orderList();

    SimTime targetTime = mLastTime + timeDelta;
    SimTime targetTick = targetTime - (targetTime % TickMs);
    bool ret = mLastTick != targetTick;

    // Advance all the objects
    for (; mLastTick != targetTick; mLastTick += TickMs)
        onAdvanceObjects();

    mLastTime = targetTime;
    mLastDelta = ((TickMs - ((targetTime + 1) % TickMs)) % TickMs) / F32(TickMs);

    PROFILE_END();
    return ret;
}

//----------------------------------------------------------------------------

void ProcessList::advanceObjects()
{
    PROFILE_START(AdvanceObjects);

    // A little link list shuffling is done here to avoid problems
    // with objects being deleted from within the process method.
    ProcessObject list;
    list.plLinkBefore(mHead.mProcessLink.next);
    mHead.plUnlink();
    for (ProcessObject* pobj = list.mProcessLink.next; pobj != &list; pobj = list.mProcessLink.next)
    {
        pobj->plUnlink();
        pobj->plLinkBefore(&mHead);

        onTickObject(pobj);
    }

    mTotalTicks++;

    PROFILE_END();
}
