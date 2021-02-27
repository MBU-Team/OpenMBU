//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "core/dnet.h"
#include "core/bitStream.h"
#include "math/mathUtils.h"
#include "game/gameConnection.h"
#include "game/gameBase.h"
#include "game/gameProcess.h"
#include "platform/profiler.h"
#include "console/consoleTypes.h"

//----------------------------------------------------------------------------
ClientProcessList gClientProcessList;
ServerProcessList gServerProcessList;
SPModeProcessList gSPModeProcessList;

U32 gNetOrderNextId = 0;

#ifdef TORQUE_HIFI
F32 gMaxHiFiVelSq = 100 * 100;
#endif

ConsoleFunction(dumpProcessList, void, 1, 1, "dumpProcessList();")
{
    Con::printf("client process list:");
    getCurrentClientProcessList()->dumpToConsole();
    Con::printf("server process list:");
    gServerProcessList.dumpToConsole();
}

#ifdef TORQUE_HIFI
// Structure used for synchronizing move lists on client/server
struct MoveSync
{
    enum { ActionCount = 4 };

    S32 moveDiff;
    S32 moveDiffSteadyCount;
    S32 moveDiffSameSignCount;

    bool doAction() { return moveDiffSteadyCount >= ActionCount || moveDiffSameSignCount >= 4 * ActionCount; }
    void reset() { moveDiff = 0; moveDiffSteadyCount = 0; moveDiffSameSignCount = 0; }
    void update(S32 diff);
} moveSync;

void MoveSync::update(S32 diff)
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
#endif

namespace
{
    // local work class
    struct GameBaseListNode
    {
        GameBaseListNode()
        {
            mPrev = this;
            mNext = this;
            mObject = NULL;
        }

        GameBaseListNode* mPrev;
        GameBaseListNode* mNext;
        GameBase* mObject;

        void linkBefore(GameBaseListNode* obj)
        {
            // Link this before obj
            mNext = obj;
            mPrev = obj->mPrev;
            obj->mPrev = this;
            mPrev->mNext = this;
        }
    };
} // namespace

//--------------------------------------------------------------------------
// ClientProcessList
//--------------------------------------------------------------------------

ClientProcessList::ClientProcessList()
{
#ifdef TORQUE_HIFI
    mSkipAdvanceObjectsMs = 0;
    mForceHifiReset = false;
    mCatchup = 0;
#endif
}

void ClientProcessList::addObject(ProcessObject* pobj)
{
    GameBase* obj = dynamic_cast<GameBase*>(pobj);
    if (obj == NULL)
        // don't add
        return;

    AssertFatal(obj->isClientObject(), "Adding non-client object to client list");

    if (obj->mNetFlags.test(GameBase::NetOrdered))
    {
        obj->plLinkBefore(&mHead);
        mDirty = true;
    }
    else if (obj->mNetFlags.test(GameBase::TickLast))
    {
        obj->mOrderGUID = 0xFFFFFFFF;
        obj->plLinkBefore(&mHead);
        // not dirty
    }
    else
    {
        obj->plLinkAfter(&mHead);
        // not dirty
    }
}

inline GameBase* ClientProcessList::getGameBase(ProcessObject* obj)
{
    return static_cast<GameBase*>(obj);
}

bool ClientProcessList::doBacklogged(SimTime timeDelta)
{
#ifdef TORQUE_DEBUG   
    static bool backlogged = false;
    static U32 backloggedTime = 0;
#endif

    // See if the control object has pending moves.
    GameConnection* connection = GameConnection::getConnectionToServer();
    if (connection)
    {
        // If the connection to the server is backlogged
        // the simulation is frozen.
        if (connection->mMoveList.isBacklogged())
        {
#ifdef TORQUE_DEBUG   
            if (!backlogged)
            {
                Con::printf("client is backlogged, time is frozen");
                backlogged = true;
            }

            backloggedTime += timeDelta;
#endif
            return true;
        }
    }

#ifdef TORQUE_DEBUG   
    if (backlogged)
    {
        Con::printf("client is no longer backlogged, time is unfrozen (%i ms elapsed)", backloggedTime);
        backlogged = false;
        backloggedTime = 0;
    }
#endif
    return false;
}

bool ClientProcessList::advanceTime(SimTime timeDelta)
{
    //PROFILE_SCOPE(AdvanceClientTime);

#ifdef TORQUE_HIFI
    if (mSkipAdvanceObjectsMs && timeDelta > mSkipAdvanceObjectsMs)
    {
        timeDelta -= mSkipAdvanceObjectsMs;
        advanceTime(mSkipAdvanceObjectsMs);
        AssertFatal(!mSkipAdvanceObjectsMs, "Doh!");
    }
#endif

    if (doBacklogged(timeDelta))
        return false;

#ifdef TORQUE_HIFI
    // remember interpolation value because we might need to set it back
    F32 oldLastDelta = mLastDelta;
#endif

    bool ret = Parent::advanceTime(timeDelta);

#ifdef TORQUE_HIFI
    if (!mSkipAdvanceObjectsMs)
    {
#endif

        AssertFatal(mLastDelta >= 0.0f && mLastDelta <= 1.0f, "Doh!  That would be bad.");

        for (ProcessObject* obj = mHead.mProcessLink.next; obj != &mHead; obj = obj->mProcessLink.next)
        {
            GameBase* gb = getGameBase(obj);
            if (gb->mProcessTick)
                gb->interpolateTick(mLastDelta);
        }

        // Inform objects of total elapsed delta so they can advance
        // client side animations.
        F32 dt = F32(timeDelta) / 1000;
        for (ProcessObject* obj = mHead.mProcessLink.next; obj != &mHead; obj = obj->mProcessLink.next)
        {
            GameBase* gb = getGameBase(obj);
            gb->advanceTime(dt);
        }

#ifdef TORQUE_HIFI
    }
    else
    {
        mSkipAdvanceObjectsMs -= timeDelta;
        mLastDelta = oldLastDelta;
    }
#endif

    return ret;
}

//----------------------------------------------------------------------------
void ClientProcessList::onAdvanceObjects()
{
    GameConnection* connection = GameConnection::getConnectionToServer();
    if (connection)
    {
        // process any demo blocks that are NOT moves, and exactly one move
        // we advance time in the demo stream by a move inserted on
        // each tick.  So before doing the tick processing we advance
        // the demo stream until a move is ready
        if (connection->isPlayingBack())
        {
            U32 blockType;
            do
            {
                blockType = connection->getNextBlockType();
                bool res = connection->processNextBlock();
                // if there are no more blocks, exit out of this function,
                // as no more client time needs to process right now - we'll
                // get it all on the next advanceClientTime()
                if (!res)
                    return;
            } while (blockType != GameConnection::BlockTypeMove);
        }

#ifdef TORQUE_HIFI
        if (!mSkipAdvanceObjectsMs)
        {
#endif

            connection->mMoveList.collectMove();
            advanceObjects();

#ifdef TORQUE_HIFI
        }

        connection->mMoveList.onAdvanceObjects();
#endif
    }
#ifndef TORQUE_HIFI
    else
        advanceObjects();
#endif
}

void ClientProcessList::onTickObject(ProcessObject* pobj)
{
    SimObjectPtr<GameBase> obj = getGameBase(pobj);
    AssertFatal(obj->isClientObject(), "Server object on client process list");

    // Each object is either advanced a single tick, or if it's
    // being controlled by a client, ticked once for each pending move.
    Move* movePtr;
    U32 numMoves;
    GameConnection* con = obj->getControllingClient();
    if (con && con->getControlObject() == obj && con->mMoveList.getMoveList(&movePtr, &numMoves))
    {
#ifndef TORQUE_HIFI
        // Note: should only have a single move at this point
        AssertFatal(numMoves == 1, "ClientProccessList::onTickObject: more than one move in queue");
#endif

#ifdef TORQUE_DEBUG_NET_MOVES
        U32 sum = Move::ChecksumMask & obj->getPacketDataChecksum(obj->getControllingClient());
#endif

        obj->processTick(movePtr);

        if (bool(obj) && obj->getControllingClient())
        {
            U32 newsum = Move::ChecksumMask & obj->getPacketDataChecksum(obj->getControllingClient());

            // set checksum if not set or check against stored value if set
            movePtr->checksum = newsum;

#ifdef TORQUE_DEBUG_NET_MOVES
            Con::printf("move checksum: %i, (start %i), (move %f %f %f)",
                movePtr->checksum, sum, movePtr->yaw, movePtr->y, movePtr->z);
#endif
        }
        con->mMoveList.clearMoves(1);
    }
    else if (obj->mProcessTick)
        obj->processTick(0);

#ifdef TORQUE_HIFI
    if (bool(obj) && (obj->getType() & GameBaseHiFiObjectType))
    {
        GameConnection* serverConnection = GameConnection::getConnectionToServer();
        TickCacheEntry* tce = obj->getTickCache().addCacheEntry();
        BitStream bs(tce->packetData, TickCacheEntry::MaxPacketSize);
        obj->writePacketData(serverConnection, &bs);

        Point3F vel = obj->getVelocity();
        F32 velSq = mDot(vel, vel);
        gMaxHiFiVelSq = getMax(gMaxHiFiVelSq, velSq);
    }
#endif
}

void ClientProcessList::advanceObjects()
{
#ifdef TORQUE_DEBUG_NET_MOVES
    Con::printf("Advance client time...");
#endif

#ifdef TORQUE_HIFI
    // client re-computes this each time objects are advanced
    gMaxHiFiVelSq = 0;
#endif
    Parent::advanceObjects();

#ifdef TORQUE_HIFI
    // We need to consume a move on the connections whether 
    // there is a control object to consume the move or not,
    // otherwise client and server can get out of sync move-wise
    // during startup.  If there is a control object, we cleared
    // a move above.  Handle case where no control object here.
    // Note that we might consume an extra move here and there when
    // we had a control object in above loop but lost it during tick.
    // That is no big deal so we don't bother trying to carefully
    // track it.
    if (GameConnection::getConnectionToServer()->getControlObject() == NULL)
        GameConnection::getConnectionToServer()->mMoveList.clearMoves(1);
#endif

#ifdef TORQUE_DEBUG_NET_MOVES
    Con::printf("---------");
#endif
}

#ifdef TORQUE_HIFI
void ClientProcessList::ageTickCache(S32 numToAge, S32 len)
{
    for (ProcessObject* pobj = mHead.mProcessLink.next; pobj != &mHead; pobj = pobj->mProcessLink.next)
    {
        GameBase* obj = getGameBase(pobj);
        if (obj->getType() & GameBaseHiFiObjectType)
            obj->getTickCache().ageCache(numToAge, len);
    }
}

void ClientProcessList::updateMoveSync(S32 moveDiff)
{
    moveSync.update(moveDiff);
    if (moveSync.doAction() && moveDiff < 0)
    {
        skipAdvanceObjects(TickMs * -moveDiff);
        moveSync.reset();
    }
}
#endif

#ifdef TORQUE_HIFI
void ClientProcessList::clientCatchup(GameConnection* connection)
{
#ifdef TORQUE_DEBUG_NET_MOVES
    Con::printf("client catching up... (%i)%s", mCatchup, mForceHifiReset ? " reset" : "");
#endif

    if (connection->getControlObject() && connection->getControlObject()->isGhostUpdated())
        // if control object is reset, make sure moves are reset too
        connection->mMoveList.resetCatchup();

    const F32 maxVel = mSqrt(gMaxHiFiVelSq) * 1.25f;
    F32 dt = F32(mCatchup + 1) * TickSec;
    Point3F bigDelta(maxVel * dt, maxVel * dt, maxVel * dt);

    // walk through all process objects looking for ones which were updated
    // -- during first pass merely collect neighbors which need to be reset and updated in unison
    ProcessObject* pobj;
    if (mCatchup && !mForceHifiReset)
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

                    // CodeReview - this is left in for MBU, but also so we can deal with the issue later.
                    // add marble blast hack so hifi networking can see hidden objects
                    // (since hidden is under control of hifi networking)
                    gForceNotHidden = true;

                    S32 j = nearby.mList.size();
                    gClientContainer.findObjects(box, GameBaseHiFiObjectType, SimpleQueryList::insertionCallback, &nearby);

                    // CodeReview - this is left in for MBU, but also so we can deal with the issue later.
                    // disable above hack
                    gForceNotHidden = false;

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
                                obj2->getTickCache().beginCacheList();
                                TickCacheEntry* tce = obj2->getTickCache().incCacheList();
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
    GameBaseListNode list;
    for (pobj = mHead.mProcessLink.next; pobj != &mHead; pobj = pobj->mProcessLink.next)
    {
        GameBase* obj = getGameBase(pobj);

        if (obj->isGhostUpdated() && (obj->getType() & GameBaseHiFiObjectType))
        {
            // construct process object and add it to the list
            // hold pointer to our object in mAfterObject
            GameBaseListNode* po = (GameBaseListNode*)FrameAllocator::alloc(sizeof(GameBaseListNode));
            po->mObject = obj;
            po->linkBefore(&list);

            // begin iterating through tick list (skip first tick since that is the state we've been reset to)
            obj->getTickCache().beginCacheList();
            obj->getTickCache().incCacheList();
        }
        else if (mForceHifiReset && (obj->getType() & GameBaseHiFiObjectType))
        {
            // add all hifi objects
            obj->getTickCache().beginCacheList();
            TickCacheEntry* tce = obj->getTickCache().incCacheList();
            BitStream bs(tce->packetData, TickCacheEntry::MaxPacketSize);
            obj->readPacketData(connection, &bs);
            obj->setGhostUpdated(true);

            // construct process object and add it to the list
            // hold pointer to our object in mAfterObject
            GameBaseListNode* po = (GameBaseListNode*)FrameAllocator::alloc(sizeof(GameBaseListNode));
            po->mObject = obj;
            po->linkBefore(&list);
        }
        else if (obj == connection->getControlObject() && obj->isGhostUpdated())
        {
            // construct process object and add it to the list
            // hold pointer to our object in mAfterObject
            // .. but this is not a hi fi object, so don't mess with tick cache
            GameBaseListNode* po = (GameBaseListNode*)FrameAllocator::alloc(sizeof(GameBaseListNode));
            po->mObject = obj;
            po->linkBefore(&list);
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
    connection->mMoveList.resetClientMoves();
    connection->mMoveList.getMoveList(&movePtr, &numMoves);
    AssertFatal(mCatchup <= numMoves, "doh");

    // tick catchup time
    for (U32 m = 0; m < mCatchup; m++)
    {
        for (GameBaseListNode* walk = list.mNext; walk != &list; walk = walk->mNext)
        {
            // note that we get object from after object not getGameBase function
            // this is because we are an on the fly linked list which uses mAfterObject
            // rather than the linked list embedded in GameBase (clean this up?)
            GameBase* obj = walk->mObject;

            // it's possible for a non-hifi object to get in here, but
            // only if it is a control object...make sure we don't do any
            // of the tick cache stuff if we are not hifi.
            bool hifi = obj->getType() & GameBaseHiFiObjectType;
            TickCacheEntry* tce = hifi ? obj->getTickCache().incCacheList() : NULL;

            // tick object
            if (obj == connection->getControlObject())
            {
                obj->processTick(movePtr);
                movePtr->checksum = obj->getPacketDataChecksum(connection);
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
    connection->mMoveList.clearMoves(mCatchup);

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

    // all caught up
    mCatchup = 0;
}
#else
void ClientProcessList::clientCatchup(GameConnection* connection)
{
    SimObjectPtr<GameBase> control = connection->getControlObject();
    if (control)
    {
        Move* movePtr;
        U32 numMoves;
        U32 m = 0;
        connection->mMoveList.getMoveList(&movePtr, &numMoves);

#ifdef TORQUE_DEBUG_NET_MOVES
        Con::printf("client catching up... (%i)%s", numMoves, mForceHifiReset ? " reset" : "");
#endif

        for (m = 0; m < numMoves && control; m++)
            control->processTick(movePtr++);
        connection->mMoveList.clearMoves(m);
    }

#ifdef TORQUE_DEBUG_NET_MOVES
    Con::printf("---------");
#endif
}
#endif

//--------------------------------------------------------------------------
// ServerProcessList
//--------------------------------------------------------------------------

ServerProcessList::ServerProcessList()
{
}

void ServerProcessList::addObject(ProcessObject* pobj)
{
    GameBase* obj = dynamic_cast<GameBase*>(pobj);
    if (obj == NULL)
        // don't add
        return;
    AssertFatal(obj->isServerObject(), "Adding client object to server list");

    if (obj->mNetFlags.test(GameBase::NetOrdered))
    {
        if ((gNetOrderNextId & 0xFFFF) == 0)
            // don't let it be zero
            gNetOrderNextId++;
        obj->mOrderGUID = (gNetOrderNextId++) & 0xFFFF; // 16 bits should be enough
        obj->plLinkBefore(&mHead);
        mDirty = true;
    }
    else if (obj->mNetFlags.test(GameBase::TickLast))
    {
        obj->mOrderGUID = 0xFFFFFFFF;
        obj->plLinkBefore(&mHead);
        // not dirty
    }
    else
    {
        obj->plLinkAfter(&mHead);
        // not dirty
    }
}

inline GameBase* ServerProcessList::getGameBase(ProcessObject* obj)
{
    return static_cast<GameBase*>(obj);
}

void ServerProcessList::onTickObject(ProcessObject* pobj)
{
    SimObjectPtr<GameBase> obj = getGameBase(pobj);
    AssertFatal(obj->isServerObject(), "Client object on server process list");

    // Each object is either advanced a single tick, or if it's
    // being controlled by a client, ticked once for each pending move.
    Move* movePtr;
    U32 numMoves;
    GameConnection* con = obj->getControllingClient();
    if (con && con->getControlObject() == obj && con->mMoveList.getMoveList(&movePtr, &numMoves))
    {
#ifdef TORQUE_DEBUG_NET_MOVES
        U32 sum = Move::ChecksumMask & obj->getPacketDataChecksum(obj->getControllingClient());
#endif

#ifndef TORQUE_HIFI
        U32 m;
        for (m = 0; m < numMoves && con && con->getControlObject() == obj; m++, movePtr++)
        {
#endif
            obj->processTick(movePtr);

#ifdef TORQUE_HIFI
            if (bool(obj) && obj->getControllingClient())
#else
            if (con && con->getControlObject() == obj)
#endif
            {
                U32 newsum = Move::ChecksumMask & obj->getPacketDataChecksum(obj->getControllingClient());

                // check move checksum
                if (movePtr->checksum != newsum)
                {
#ifdef TORQUE_DEBUG_NET_MOVES
                    if (!obj->mIsAiControlled)
                        Con::printf("move %i checksum disagree: %i != %i, (start %i), (move %f %f %f)",
                            movePtr->id, movePtr->checksum, newsum, sum, movePtr->yaw, movePtr->y, movePtr->z);
#endif

                    movePtr->checksum = Move::ChecksumMismatch;
                }
                else
                {
#ifdef TORQUE_DEBUG_NET_MOVES
                    Con::printf("move %i checksum agree: %i == %i, (start %i), (move %f %f %f)",
                        movePtr->id, movePtr->checksum, newsum, sum, movePtr->yaw, movePtr->y, movePtr->z);
#endif
                }
            }
#ifndef TORQUE_HIFI
        }
        con->mMoveList.clearMoves(m);
#endif
    }
    else if (obj->mProcessTick)
        obj->processTick(0);
}

void ServerProcessList::advanceObjects()
{
#ifdef TORQUE_DEBUG_NET_MOVES
    Con::printf("Advance server time...");
#endif

    Parent::advanceObjects();

#ifdef TORQUE_HIFI
    // We need to consume a move on the connections whether 
       // there is a control object to consume the move or not,
       // otherwise client and server can get out of sync move-wise
       // during startup.  
    for (S32 i = 0; i < Sim::getClientGroup()->size(); i++)
    {
        GameConnection* con = (GameConnection*)(*Sim::getClientGroup())[i];
        if (con->mMoveList.areMovesPending())
            con->mMoveList.clearMoves(1);
    }
#else
    // Credit all the connections with the elapsed ticks.
    SimGroup* g = Sim::getClientGroup();
    for (SimGroup::iterator i = g->begin(); i != g->end(); i++)
        if (GameConnection* t = dynamic_cast<GameConnection*>(*i))
            t->mMoveList.incMoveCredit(1);
#endif

#ifdef TORQUE_DEBUG_NET_MOVES
    Con::printf("---------");
#endif
}

//--------------------------------------------------------------------------
// SPModeProcessList
//--------------------------------------------------------------------------

SPModeProcessList::SPModeProcessList()
{
}

inline GameBase* SPModeProcessList::getGameBase(ProcessObject* obj)
{
    return static_cast<GameBase*>(obj);
}

void SPModeProcessList::onTickObject(ProcessObject* pobj)
{
    SimObjectPtr<GameBase> obj = getGameBase(pobj);
    AssertFatal(obj->isServerObject(), "Client object on server process list");

    // Each object is either advanced a single tick, or if it's
    // being controlled by a client, ticked once for each pending move.
    Move* movePtr;
    U32 numMoves;
    GameConnection* con = obj->getControllingClient();
    if (con && con->getControlObject() == obj && con->mMoveList.getMoveList(&movePtr, &numMoves))
    {
#ifdef TORQUE_DEBUG_NET_MOVES
        U32 sum = Move::ChecksumMask & obj->getPacketDataChecksum(obj->getControllingClient());
#endif

#ifndef TORQUE_HIFI
        U32 m;
        for (m = 0; m < numMoves && con && con->getControlObject() == obj; m++, movePtr++)
        {
#endif
            obj->processTick(movePtr);

#ifdef TORQUE_HIFI
            if (bool(obj) && obj->getControllingClient())
#else
            if (con && con->getControlObject() == obj)
#endif
            {
                U32 newsum = Move::ChecksumMask & obj->getPacketDataChecksum(obj->getControllingClient());

                // check move checksum
                if (movePtr->checksum != newsum)
                {
#ifdef TORQUE_DEBUG_NET_MOVES
                    if (!obj->mIsAiControlled)
                        Con::printf("move %i checksum disagree: %i != %i, (start %i), (move %f %f %f)",
                            movePtr->id, movePtr->checksum, newsum, sum, movePtr->yaw, movePtr->y, movePtr->z);
#endif

                    movePtr->checksum = Move::ChecksumMismatch;
                }
                else
                {
#ifdef TORQUE_DEBUG_NET_MOVES
                    Con::printf("move %i checksum agree: %i == %i, (start %i), (move %f %f %f)",
                        movePtr->id, movePtr->checksum, newsum, sum, movePtr->yaw, movePtr->y, movePtr->z);
#endif
                }
            }
#ifndef TORQUE_HIFI
        }
        con->mMoveList.clearMoves(m);
#endif
    }
    else if (obj->mProcessTick)
        obj->processTick(0);
}

bool SPModeProcessList::advanceTime(SimTime timeDelta)
{
    //PROFILE_SCOPE(AdvanceSPModeTime);

#ifdef TORQUE_HIFI
    if (mSkipAdvanceObjectsMs && timeDelta > mSkipAdvanceObjectsMs)
    {
        timeDelta -= mSkipAdvanceObjectsMs;
        advanceTime(mSkipAdvanceObjectsMs);
        AssertFatal(!mSkipAdvanceObjectsMs, "Doh!");
    }
#endif

    if (doBacklogged(timeDelta))
        return false;

#ifdef TORQUE_HIFI
    // remember interpolation value because we might need to set it back
    F32 oldLastDelta = mLastDelta;
#endif

    bool ret = Parent::advanceTime(timeDelta);

#ifdef TORQUE_HIFI
    if (!mSkipAdvanceObjectsMs)
    {
#endif

        AssertFatal(mLastDelta >= 0.0f && mLastDelta <= 1.0f, "Doh!  That would be bad.");

        // Inform objects of total elapsed delta so they can advance
        // client side animations.
        F32 dt = F32(timeDelta) / 1000;
        for (ProcessObject* obj = mHead.mProcessLink.next; obj != &mHead; obj = obj->mProcessLink.next)
        {
            GameBase* gb = getGameBase(obj);
            gb->advanceTime(dt);
        }

#ifdef TORQUE_HIFI
    }
    else
    {
        mSkipAdvanceObjectsMs -= timeDelta;
        mLastDelta = oldLastDelta;
    }
#endif

    return ret;
}