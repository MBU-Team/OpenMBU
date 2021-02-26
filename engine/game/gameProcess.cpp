//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "core/dnet.h"
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

ConsoleFunction(dumpProcessList, void, 1, 1, "dumpProcessList();")
{
    Con::printf("client process list:");
    getCurrentClientProcessList()->dumpToConsole();
    Con::printf("server process list:");
    gServerProcessList.dumpToConsole();
}

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
}

void ClientProcessList::addObject(ProcessObject* pobj)
{
    GameBase* obj = dynamic_cast<GameBase*>(pobj);
    if (obj == NULL)
        // don't add
        return;

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

    if (doBacklogged(timeDelta))
        return false;

    bool ret = Parent::advanceTime(timeDelta);
    ProcessObject* obj = NULL;

    AssertFatal(mLastDelta >= 0.0f && mLastDelta <= 1.0f, "Doh!  That would be bad.");

    obj = mHead.mProcessLink.next;
    while (obj != &mHead)
    {
        GameBase* gb = getGameBase(obj);
        if (gb->mProcessTick)
            gb->interpolateTick(mLastDelta);

        obj = obj->mProcessLink.next;
    }

    // Inform objects of total elapsed delta so they can advance
    // client side animations.
    F32 dt = F32(timeDelta) / 1000;

    obj = mHead.mProcessLink.next;
    while (obj != &mHead)
    {
        GameBase* gb = getGameBase(obj);
        gb->advanceTime(dt);

        obj = obj->mProcessLink.next;
    }

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

        connection->mMoveList.collectMove();
        advanceObjects();
    }
    else
        advanceObjects();
}

void ClientProcessList::onTickObject(ProcessObject* pobj)
{
    SimObjectPtr<GameBase> obj = getGameBase(pobj);

    // Each object is either advanced a single tick, or if it's
    // being controlled by a client, ticked once for each pending move.
    Move* movePtr;
    U32 numMoves;
    GameConnection* con = obj->getControllingClient();
    if (con && con->getControlObject() == obj)
    {
        con->mMoveList.getMoveList(&movePtr, &numMoves);
        if (numMoves)
        {
            // Note: should only have a single move at this point
            AssertFatal(numMoves == 1, "ClientProccessList::onTickObject: more than one move in queue");

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
    }
    else if (obj->mProcessTick)
        obj->processTick(0);
}

void ClientProcessList::advanceObjects()
{
#ifdef TORQUE_DEBUG_NET_MOVES
    Con::printf("Advance client time...");
#endif

    Parent::advanceObjects();

#ifdef TORQUE_DEBUG_NET_MOVES
    Con::printf("---------");
#endif
}

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
    GameConnection* con = obj->getControllingClient();

    if (con && con->getControlObject() == obj)
    {
        Move* movePtr;
        U32 m, numMoves;
        con->mMoveList.getMoveList(&movePtr, &numMoves);

        for (m = 0; m < numMoves && con && con->getControlObject() == obj; m++, movePtr++)
        {
#ifdef TORQUE_DEBUG_NET_MOVES
            U32 sum = Move::ChecksumMask & obj->getPacketDataChecksum(obj->getControllingClient());
#endif

            obj->processTick(movePtr);

            if (con && con->getControlObject() == obj)
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
        }
        con->mMoveList.clearMoves(m);
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

    // Credit all the connections with the elapsed ticks.
    SimGroup* g = Sim::getClientGroup();
    for (SimGroup::iterator i = g->begin(); i != g->end(); i++)
        if (GameConnection* t = dynamic_cast<GameConnection*>(*i))
            t->mMoveList.incMoveCredit(1);

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
    GameConnection* con = obj->getControllingClient();

    if (con && con->getControlObject() == obj)
    {
        Move* movePtr;
        U32 m, numMoves;
        con->mMoveList.getMoveList(&movePtr, &numMoves);

        for (m = 0; m < numMoves && con && con->getControlObject() == obj; m++, movePtr++)
        {
            obj->processTick(movePtr);

            if (con && con->getControlObject() == obj)
            {
                U32 newsum = Move::ChecksumMask & obj->getPacketDataChecksum(obj->getControllingClient());

                // check move checksum
                if (movePtr->checksum != newsum)
                    movePtr->checksum = Move::ChecksumMismatch;
            }
        }
        con->mMoveList.clearMoves(m);
    }
    else if (obj->mProcessTick)
        obj->processTick(0);
}

bool SPModeProcessList::advanceTime(SimTime timeDelta)
{
    //PROFILE_SCOPE(AdvanceSPModeTime);

    bool ret = Parent::advanceTime(timeDelta);
    ProcessObject* obj = NULL;

    AssertFatal(mLastDelta >= 0.0f && mLastDelta <= 1.0f, "Doh!  That would be bad.");

    // Inform objects of total elapsed delta so they can advance
    // client side animations.
    F32 dt = F32(timeDelta) / 1000;

    obj = mHead.mProcessLink.next;
    while (obj != &mHead)
    {
        GameBase* gb = getGameBase(obj);
        gb->advanceTime(dt);

        obj = obj->mProcessLink.next;
    }

    return ret;
}