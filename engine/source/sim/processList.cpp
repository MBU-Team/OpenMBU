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
#include "core/bitStream.h"
#include "game/gameProcess.h"
#include "math/mathUtils.h"
#include "game/tickCache.h"

//----------------------------------------------------------------------------

bool ProcessList::mDebugControlSync = false;
U32 gNetOrderNextId = 0;
F32 gMaxHiFiVelSq = 100 * 100;

//--------------------------------------------------------------------------

struct MoveSync
{
    enum
    {
        ActionCount = 4,
    };

    S32 moveDiff = 0;
    S32 moveDiffSteadyCount = 0;
    S32 moveDiffSameSignCount = 0;

    bool doAction() const
    {
        return moveDiffSteadyCount >= 4 || moveDiffSameSignCount >= 16;
    }

    void reset()
    {
        moveDiff = 0;
        moveDiffSteadyCount = 0;
        moveDiffSameSignCount = 0;
    }

    void update(S32 diff)
    {
        if (diff && diff == moveDiff)
        {
            moveDiffSteadyCount++;
            moveDiffSameSignCount++;
        }
        else if (diff * moveDiff > 0)
        {
            moveDiffSteadyCount = 0;
            moveDiffSameSignCount++;
        }
        else
            reset();
        moveDiff = diff;
    }
};

MoveSync moveSync;

//--------------------------------------------------------------------------


ProcessObject::ProcessObject()
{
    mAfterObject = NULL;
    mProcessTag = 0;
    mProcessLink.prev = this;
    mProcessLink.next = this;
    mNetOrderId = 0;
}

void ProcessObject::plUnlink()
{
    mProcessLink.next->mProcessLink.prev = mProcessLink.prev;
    mProcessLink.prev->mProcessLink.next = mProcessLink.next;
    mProcessLink.next = mProcessLink.prev = this;
}

void ProcessObject::plLinkAfter(ProcessObject* obj)
{
    // Link this after obj
    mProcessLink.next = obj->mProcessLink.next;
    mProcessLink.prev = obj;
    obj->mProcessLink.next = this;
    mProcessLink.next->mProcessLink.prev = this;
}

void ProcessObject::plLinkBefore(ProcessObject* obj)
{
    // Link this before obj
    mProcessLink.next = obj;
    mProcessLink.prev = obj->mProcessLink.prev;
    obj->mProcessLink.prev = this;
    mProcessLink.prev->mProcessLink.next = this;
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

//--------------------------------------------------------------------------

ProcessList::ProcessList(bool isServer)
{
    mDirty = false;
    mCurrentTag = 0;
    mLastTick = 0;
    mLastTime = 0;
    mTotalTicks = 0;
    mLastDelta = 0.0f;
    mSkipAdvanceObjectsMs = 0;
    mForceHifiReset = false;
    mIsServer = isServer;

    //   Con::addVariable("debugControlSync",TypeBool, &mDebugControlSync);
}

GameBase* ProcessList::getGameBase(ProcessObject* obj)
{
    return static_cast<GameBase*>(obj);
}

void ProcessList::addObject(GameBase* obj) {

    if (obj->mNetFlags.test(GameBase::NetOrdered))
    {
        if (!gNetOrderNextId)
            ++gNetOrderNextId;

        if (obj->isServerObject())
            obj->mNetOrderId = gNetOrderNextId++;

        obj->plLinkBefore(&mHead);

        markDirty();
    } else
    {
        if (obj->mNetFlags.test(GameBase::TickLast))
        {
            obj->mNetOrderId = -1;
            obj->plLinkBefore(&mHead);
        } else
        {
            obj->plLinkAfter(&mHead);
        }
    }
}

//----------------------------------------------------------------------------

void ProcessList::orderList()
{
    // GameBase tags are intialized to 0, so current tag
    // should never be 0.
    if (!++mCurrentTag)
        mCurrentTag++;

    // Install a temporary head node
    ProcessObject list;
    list.plLinkBefore(mHead.mProcessLink.next);
    mHead.plUnlink();

    ProcessObject* next = list.mProcessLink.next;
    if (next != &list)
    {
        do
        {
            if (next->mNetOrderId)
            {
                for (ProcessObject* obj = next->mProcessLink.next; obj != &list; obj = obj->mProcessLink.next)
                {
                    if (obj->mNetOrderId < next->mNetOrderId)
                    {
                        ProcessObject* before = next->mProcessLink.prev;
                        ProcessObject* after = obj->mProcessLink.next;
                        next->plUnlink();
                        obj->plUnlink();
                        next->plLinkBefore(after);
                        obj->plLinkAfter(before);

                        ProcessObject* temp = obj;
                        obj = next;
                        next = temp;
                    }
                }
            }
            next = next->mProcessLink.next;

        } while (next != &list);

        // Reverse topological sort into the orignal head node
        while (list.mProcessLink.next != &list) {
            ProcessObject* ptr = list.mProcessLink.next;
            ptr->mProcessTag = mCurrentTag;
            ptr->plUnlink();
            if (ptr->mAfterObject) {
                // Build chain "stack" of dependant objects and patch
                // it to the end of the current list.
                while (bool(ptr->mAfterObject) &&
                    ptr->mAfterObject->mProcessTag != mCurrentTag) {
                    ptr->mAfterObject->mProcessTag = mCurrentTag;
                    ptr->mAfterObject->plUnlink();
                    ptr->mAfterObject->plLinkBefore(ptr);
                    ptr = ptr->mAfterObject;
                }
                ptr->plJoin(&mHead);
            }
            else
                ptr->plLinkBefore(&mHead);
        }
    }
    mDirty = false;
}


//----------------------------------------------------------------------------

bool ProcessList::advanceServerTime(SimTime timeDelta)
{
    PROFILE_START(AdvanceServerTime);

    if (mDirty) orderList();

    SimTime targetTime = mLastTime + timeDelta;
    SimTime targetTick = targetTime - (targetTime & 31);

    bool ret = mLastTick != targetTick;

    // Advance all the objects
    for (; mLastTick != targetTick; mLastTick += TickMs)
        advanceObjects();

    mLastTime = targetTime;
    PROFILE_END();
    return ret;
}


//----------------------------------------------------------------------------

#ifdef MB_CLIENT_PHYSICS_EVERY_FRAME
extern Move gFirstMove;
extern Move gNextMove;
#endif

bool ProcessList::advanceClientTime(SimTime timeDelta)
{
    PROFILE_START(AdvanceClientTime);

    if (mSkipAdvanceObjectsMs && timeDelta > mSkipAdvanceObjectsMs)
    {
        timeDelta -= mSkipAdvanceObjectsMs;
        advanceClientTime(mSkipAdvanceObjectsMs);
        AssertFatal(!mSkipAdvanceObjectsMs, "Doh!");
    }

    if (mDirty) orderList();

#ifdef MB_CLIENT_PHYSICS_EVERY_FRAME
    Move firstmove = gFirstMove;
    Move nextmove = gNextMove;
#endif

    SimTime targetTime = mLastTime + timeDelta;
    SimTime targetTick = targetTime - (targetTime & 0x1F);
    SimTime tickCount = (targetTick - mLastTick) >> TickShift;

    // See if the control object has pending moves.
    //GameBase* control = 0;
    GameConnection* connection = GameConnection::getConnectionToServer();
    if (connection && connection->isBacklogged())
    {
        PROFILE_END();
        return false;
    }

    if (tickCount && mLastTick != targetTick)
    {
        while (true)
        {
            if (connection)
            {
                if (connection->isPlayingBack())
                {
                    while (true)
                    {
                        U32 nextBlockType = connection->getNextBlockType();
                        if (!connection->processNextBlock())
                        {
                            PROFILE_END();
                            return true;
                        }
                        if (nextBlockType == GameConnection::BlockTypeMove)
                            break;
                    }
                }

                if (!mSkipAdvanceObjectsMs)
                {
                    connection->collectMove(mLastTick);
                    advanceObjects();
                }
                connection->incLastSentMove();
            }

            mLastTick += TickMs;
            if (mLastTick == targetTick)
                break;
        }
    }

    if (!mSkipAdvanceObjectsMs)
    {
        mLastDelta = (float)(-(targetTime + 1) & 0x1F) * 0.03125f;
        AssertFatal(mLastDelta >= 0.0f && mLastDelta <= 1.0f, "Doh!  That would be bad.");
        for (ProcessObject* pobj = mHead.mProcessLink.next; pobj != &mHead; pobj = pobj->mProcessLink.next)
        {
            GameBase* gb = getGameBase(pobj);
            if (gb->mProcessTick)
                gb->interpolateTick(mLastDelta);
        }

        F32 dt = F32(timeDelta) / 1000;
        for (ProcessObject* pobj = mHead.mProcessLink.next; pobj != &mHead; pobj = pobj->mProcessLink.next)
        {
            GameBase* gb = getGameBase(pobj);
            gb->advanceTime(dt);
        }

#ifdef MB_CLIENT_PHYSICS_EVERY_FRAME
        for (ProcessObject* pobj = mHead.mProcessLink.next; pobj != &mHead; pobj = pobj->mProcessLink.next)
        {
            GameBase* gb = getGameBase(pobj);
            gb->processPhysicsTick(&firstmove, dt);
        }
#endif
    }
    else {
        mSkipAdvanceObjectsMs -= timeDelta;
    }

    mLastTime = targetTime;
    PROFILE_END();
    return tickCount != 0;
}

//----------------------------------------------------------------------------

bool ProcessList::advanceSPModeTime(SimTime timeDelta)
{
    PROFILE_START(AdvanceSPModeTime);

    if (mDirty) orderList();

    SimTime targetTime = mLastTime + timeDelta;
    SimTime targetTick = targetTime - (targetTime % TickMs);
    SimTime tickCount = (targetTick - mLastTick) >> TickShift;

    bool ret = mLastTick != targetTick;

    GameConnection* con = GameConnection::getConnectionToServer();

    // Advance all the objects
    for (; mLastTick != targetTick; mLastTick += TickMs)
    {
        if (con)
            con->collectMove(mLastTick);
        advanceObjects();
        if (con)
            con->incLastSentMove();
    }

    mLastDelta = ((float)(-(targetTime + 1) % TickMs)) * 0.03125f;
    AssertFatal(mLastDelta >= 0.0f && mLastDelta <= 1.0f, "Doh!  That would be bad.");

    for (ProcessObject* obj = mHead.mProcessLink.next; obj != &mHead;
        obj = obj->mProcessLink.next)
    {
        GameBase* gb = getGameBase(obj);
        if (gb->mProcessTick)
            gb->interpolateTick(mLastDelta);
    }

    F32 dt = F32(timeDelta) / 1000;
    for (ProcessObject* obj = mHead.mProcessLink.next; obj != &mHead;
        obj = obj->mProcessLink.next)
    {
        GameBase* gb = getGameBase(obj);
        gb->advanceTime(dt);
    }

    mLastTime = targetTime;
    PROFILE_END();
    return ret;
}

//----------------------------------------------------------------------------

void ProcessList::dumpToConsole()
{
    for (ProcessObject* pobj = mHead.mProcessLink.next; pobj != &mHead; pobj = pobj->mProcessLink.next)
    {
        SimObject* obj = dynamic_cast<SimObject*>(getGameBase(pobj));
        if (obj)
            Con::printf("id %i, order guid %i, type %s", obj->getId(), 0, obj->getClassName());
        else
            Con::printf("---unknown object type, order guid %i", 0);
    }
}


//----------------------------------------------------------------------------

void ProcessList::advanceObjects()
{
    PROFILE_START(AdvanceObjects);

    if (!mIsServer)
        gMaxHiFiVelSq = 0.0f;

    // A little link list shuffling is done here to avoid problems
    // with objects being deleted from within the process method.
    ProcessObject list;
    list.plLinkBefore(mHead.mProcessLink.next);
    mHead.plUnlink();
    while (list.mProcessLink.next != &list)
    {
        SimObjectPtr<GameBase> obj = getGameBase(list.mProcessLink.next);

        obj->plUnlink();
        obj->plLinkBefore(&mHead);

        // Each object is either advanced a single tick, or if it's
        // being controlled by a client, ticked once for each pending move.
        GameConnection* con = obj->getControllingClient();

        bool processed = false;

        if (con && con->getControlObject() == obj) {
            Move* movePtr;
            U32 numMoves;

            if (con->getMoveList(&movePtr, &numMoves))
            {
#ifdef TORQUE_DEBUG_NET_MOVES
                U32 sum = Move::ChecksumMask & obj->getPacketDataChecksum(con);
#endif

                obj->processTick(movePtr);
                if (!obj.isNull() && obj->getControllingClient())
                {
                    U32 newsum = Move::ChecksumMask & obj->getPacketDataChecksum(con);
                    if (obj->isGhost() || gSPMode)
                        movePtr->checksum = newsum;
                    else if (movePtr->checksum != newsum)
                    {
                        movePtr->checksum = Move::ChecksumMismatch;
#ifdef TORQUE_DEBUG_NET_MOVES
                        if (!obj->mIsAiControlled)
                            Con::printf("move %i checksum disagree: %i != %i, (start %i), (move %f %f %f)",
                                movePtr->id, movePtr->checksum, newsum, sum, movePtr->yaw, movePtr->y, movePtr->z);
#endif
                    } else
                    {
#ifdef TORQUE_DEBUG_NET_MOVES
                        Con::printf("move %i checksum agree: %i == %i, (start %i), (move %f %f %f)",
                            movePtr->id, movePtr->checksum, newsum, sum, movePtr->yaw, movePtr->y, movePtr->z);
#endif
                    }

                }
                con->clearMoves(1);
                processed = true;
            }
        }

        if (obj->mProcessTick && !processed)
        {
            obj->processTick(NULL);
        }
        
        if (!obj.isNull() && (obj->isGhost() || gSPMode) && (obj->getType() & GameBaseHiFiObjectType) != 0)
        {
            GameConnection* serverCon = GameConnection::getConnectionToServer();

            TickCacheEntry* entry = obj->addTickCacheEntry();

            BitStream bs(entry->packetData, TickCacheEntry::MaxPacketSize);
            obj->writePacketData(serverCon, &bs);

            Point3F velocity = obj->getVelocity();
            F32 velSq = mDot(velocity, velocity);
            gMaxHiFiVelSq = getMax(gMaxHiFiVelSq, velSq);
        }
    }

    if (mIsServer)
    {
        SimGroup* group = Sim::gClientGroup;
        for (S32 i = 0; i < group->size(); i++)
        {
            SimObject* obj = (*group)[i];
            GameConnection* con = static_cast<GameConnection*>(obj);
            if (!con->getControlObject() && !con->getMoves().empty())
            {
                con->clearMoves(1);
            }
        }
    } else if (!GameConnection::getConnectionToServer()->getControlObject())
    {
        GameConnection::getConnectionToServer()->clearMoves(1);
    }

    mTotalTicks++;

    PROFILE_END();
}

void ProcessList::ageTickCache(S32 numToAge, S32 len)
{
    for (ProcessObject* i = mHead.mProcessLink.next; i != &mHead; i = i->mProcessLink.next)
    {
        GameBase* gb = getGameBase(i);
        if ((gb->getTypeMask() & GameBaseHiFiObjectType) != 0)
            gb->ageTickCache(numToAge, len);
    }
}

void ProcessList::updateMoveSync(S32 moveDiff)
{
    moveSync.update(moveDiff);
    if (moveSync.doAction() && moveDiff < 0)
    {
        getCurrentClientProcessList()->skipAdvanceObjects(TickMs * -moveDiff);
        moveSync.reset();
    }
}

void ProcessList::clientCatchup(GameConnection* connection, S32 catchup)
{
#ifdef TORQUE_DEBUG_NET_MOVES
    Con::printf("client catching up... (%i)%s", catchup, mForceHifiReset ? " reset" : "");
#endif

    const F32 maxVel = mSqrt(gMaxHiFiVelSq) * 1.25f;
    F32 dt = F32(catchup + 1) * TickSec;
    Point3F bigDelta(maxVel * dt, maxVel * dt, maxVel * dt);

    // walk through all process objects looking for ones which were updated
    // -- during first pass merely collect neighbors which need to be reset and updated in unison
    ProcessObject* pobj;
    if (catchup && !mForceHifiReset)
    {
        for (pobj = mHead.mProcessLink.next; pobj != &mHead; pobj = pobj->mProcessLink.next)
        {
            GameBase* obj = getGameBase(pobj);
            static SimpleQueryList nearby;
            nearby.mList.clear();
            // check for nearby objects which need to be reset and then caught up
            // note the funky loop logic -- first time through obj is us, then
            // we start iterating through nearby list (to look for objects nearby
            // the nearby objects), which is why index starts at -1
            // [objects nearby the nearby objects also get added to the nearby list]
            for (S32 i = -1; obj; obj = ++i < nearby.mList.size() ? (GameBase*)nearby.mList[i] : NULL)
            {
                if (obj->isGhostUpdated() && (obj->getType() & GameBaseHiFiObjectType) && !obj->mNetFlags.test(GameBase::NetNearbyAdded))
                {
                    Point3F start = obj->getWorldSphere().center;
                    Point3F end = start + 1.1f * dt * obj->getVelocity();
                    F32 rad = 1.5f * obj->getWorldSphere().radius;

                    // find nearby items not updated but are hi fi, mark them as updated (and restore old loc)
                    // check to see if added items have neighbors that need updating
                    Box3F box;
                    Point3F rads(rad, rad, rad);
                    box.min = box.max = start;
                    box.min -= bigDelta + rads;
                    box.max += bigDelta + rads;

#ifdef MB_ULTRA
                    // CodeReview - this is left in for MBU, but also so we can deal with the issue later.
                    // add marble blast hack so hifi networking can see hidden objects
                    // (since hidden is under control of hifi networking)
                    gForceNotHidden = true;
#endif

                    S32 j = nearby.mList.size();
                    getCurrentClientContainer()->findObjects(box, GameBaseHiFiObjectType, SimpleQueryList::insertionCallback, &nearby);

#ifdef MB_ULTRA
                    // CodeReview - this is left in for MBU, but also so we can deal with the issue later.
                    // disable above hack
                    gForceNotHidden = false;
#endif

                    // drop anyone not heading toward us or already checked
                    for (; j < nearby.mList.size(); j++)
                    {
                        GameBase* obj2 = (GameBase*)nearby.mList[j];
                        // if both passive, these guys don't interact with each other
                        bool passive = obj->mNetFlags.test(GameBase::HiFiPassive) && obj2->mNetFlags.test(GameBase::HiFiPassive);
                        if (!obj2->isGhostUpdated() && !passive)
                        {
                            // compare swept spheres of obj and obj2
                            // if collide, reset obj2, setGhostUpdated(true), and continue
                            Point3F end2 = obj2->getWorldSphere().center;
                            Point3F start2 = end2 - 1.1f * dt * obj2->getVelocity();
                            F32 rad2 = 1.5f * obj->getWorldSphere().radius;
                            if (MathUtils::capsuleCapsuleOverlap(start, end, rad, start2, end2, rad2))
                            {
                                // better add obj2
                                obj2->beginTickCacheList();
                                TickCacheEntry* tce = obj2->incTickCacheList(true);
                                BitStream bs(tce->packetData, TickCacheEntry::MaxPacketSize);
                                obj2->readPacketData(connection, &bs);
                                obj2->setGhostUpdated(true);

                                // continue so we later add the neighbors too
                                continue;
                            }

                        }

                        // didn't pass above test...so don't add it or nearby objects
                        nearby.mList[j] = nearby.mList.last();
                        nearby.mList.decrement();
                        j--;
                    }
                    obj->mNetFlags.set(GameBase::NetNearbyAdded);
                }
            }
        }
    }

    // save water mark -- for game base list
    FrameAllocatorMarker mark;

    // build ordered list of client objects which need to be caught up
    ProcessObject list;
    for (pobj = mHead.mProcessLink.next; pobj != &mHead; pobj = pobj->mProcessLink.next)
    {
        GameBase* obj = getGameBase(pobj);

        if (obj->isGhostUpdated() && (obj->getType() & GameBaseHiFiObjectType))
        {
            // construct process object and add it to the list
            // hold pointer to our object in mAfterObject
            ProcessObject* po = (ProcessObject*)FrameAllocator::alloc(sizeof(ProcessObject));
            constructInPlace(po);
            po->mAfterObject = obj;
            po->plLinkBefore(&list);

            // begin iterating through tick list (skip first tick since that is the state we've been reset to)
            obj->beginTickCacheList();
            obj->incTickCacheList(true);
        }
        else if (mForceHifiReset && (obj->getType() & GameBaseHiFiObjectType))
        {
            // add all hifi objects
            obj->beginTickCacheList();
            TickCacheEntry* tce = obj->incTickCacheList(true);
            BitStream bs(tce->packetData, TickCacheEntry::MaxPacketSize);
            obj->readPacketData(connection, &bs);
            obj->setGhostUpdated(true);

            // construct process object and add it to the list
            // hold pointer to our object in mAfterObject
            ProcessObject* po = (ProcessObject*)FrameAllocator::alloc(sizeof(ProcessObject));
            constructInPlace(po);
            po->mAfterObject = obj;
            po->plLinkBefore(&list);
        }
        else if (obj == connection->getControlObject() && obj->isGhostUpdated())
        {
            // construct process object and add it to the list
            // hold pointer to our object in mAfterObject
            // .. but this is not a hi fi object, so don't mess with tick cache
            ProcessObject* po = (ProcessObject*)FrameAllocator::alloc(sizeof(ProcessObject));
            constructInPlace(po);
            po->mAfterObject = obj;
            po->plLinkBefore(&list);
        }
        else if (obj->isGhostUpdated())
        {
            // not hifi but we were updated, so perform net smooth now
            obj->computeNetSmooth(mLastDelta);
        }

        // clear out work flags
        obj->mNetFlags.clear(GameBase::NetNearbyAdded);
        obj->setGhostUpdated(false);
    }

    // run through all the moves in the move list so we can play them with our control object
    Move* movePtr;
    U32 numMoves;

    connection->resetClientMoves();
    connection->getMoveList(&movePtr, &numMoves);
    AssertFatal(catchup <= numMoves, "doh");

    // tick catchup time
    for (U32 m = 0; m < catchup; m++)
    {
        for (ProcessObject* walk = list.mProcessLink.next; walk != &list; walk = walk->mProcessLink.next)
        {
            // note that we get object from after object not getGameBase function
            // this is because we are an on the fly linked list which uses mAfterObject
            // rather than the linked list embedded in GameBase (clean this up?)
            GameBase* obj = walk->mAfterObject;

            // it's possible for a non-hifi object to get in here, but
            // only if it is a control object...make sure we don't do any
            // of the tick cache stuff if we are not hifi.
            bool hifi = obj->getType() & GameBaseHiFiObjectType;
            TickCacheEntry* tce = hifi ? obj->incTickCacheList(true) : NULL;

            // tick object
            if (obj == connection->getControlObject())
            {
#ifdef TORQUE_DEBUG_NET_MOVES
                U32 sum = Move::ChecksumMask & obj->getPacketDataChecksum(obj->getControllingClient());
#endif

                obj->processTick(movePtr);

                // set checksum if not set or check against stored value if set
                movePtr->checksum = Move::ChecksumMask & obj->getPacketDataChecksum(obj->getControllingClient());

#ifdef TORQUE_DEBUG_NET_MOVES
                Con::printf("move checksum: %i, (start %i), (move %f %f %f)",
                    movePtr->checksum, sum, movePtr->yaw, movePtr->y, movePtr->z);
#endif

                movePtr++;
            }
            else
            {
                AssertFatal(tce && hifi, "Should not get in here unless a hi fi object!!!");
                obj->processTick(tce->move);
            }

            if (hifi)
            {
                BitStream bs(tce->packetData, TickCacheEntry::MaxPacketSize);
                obj->writePacketData(connection, &bs);
            }
        }
        if (connection->getControlObject() == NULL)
            movePtr++;
    }
    connection->clearMoves(catchup);

    // Handle network error smoothing here...but only for control object
    GameBase* control = connection->getControlObject();
    if (control && !control->isNewGhost())
    {
        control->computeNetSmooth(mLastDelta);
        control->setNewGhost(false);
    }

    if (moveSync.doAction() && moveSync.moveDiff > 0)
    {
        S32 moveDiff = moveSync.moveDiff;
#ifdef TORQUE_DEBUG_NET_MOVES
        Con::printf("client timewarping to catchup %i moves", moveDiff);
#endif
        while (moveDiff--)
            advanceObjects();
        moveSync.reset();
    }

#ifdef TORQUE_DEBUG_NET_MOVES
    Con::printf("---------");
#endif
}
