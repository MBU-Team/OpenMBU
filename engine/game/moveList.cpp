//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "game/moveList.h"
#include "game/gameConnection.h"
#include "core/bitStream.h"
#include "game/gameBase.h"
#include "game/gameProcess.h"

#define MAX_MOVE_PACKET_SENDS 4

#ifdef TORQUE_HIFI
const U32 DefaultTargetMoveListSize = 3;
const U32 DefaultMaxMoveSizeList = 5;
const F32 DefaultSmoothMoveAvg = 0.15f;
const F32 DefaultMoveListSizeSlack = 1.0f;
#endif

MoveList::MoveList()
{
    mLastMoveAck = 0;
    mLastClientMove = 0;
    mFirstMoveIndex = 0;
#ifdef TORQUE_HIFI
    mLastSentMove = 0;
    mAvgMoveQueueSize = DefaultTargetMoveListSize;
    mTargetMoveListSize = DefaultTargetMoveListSize;
    mMaxMoveListSize = DefaultMaxMoveSizeList;
    mSmoothMoveAvg = DefaultSmoothMoveAvg;
    mMoveListSizeSlack = DefaultMoveListSizeSlack;
    mTotalServerTicks = ServerTicksUninitialized;
#else
    mMoveCredit = MaxMoveCount;
#endif
    mControlMismatch = false;
    mConnection = NULL;
}

#ifdef TORQUE_HIFI
void MoveList::updateClientServerTickDiff(S32& tickDiff)
{
    if (mLastMoveAck == 0)
        tickDiff = 0;

    // Make adjustments to move list to account for tick mis-matches between client and server.
    if (tickDiff > 0)
    {
        // Server ticked more than client.  Adjust for this by reseting all hifi objects
        // to a later position in the tick cache (see ageTickCache below) and at the same
        // time pulling back some moves we thought we had made (so that time on client
        // doesn't change).
        S32 dropTicks = tickDiff;
        while (dropTicks)
        {
#ifdef TORQUE_DEBUG_NET_MOVES
            Con::printf("dropping move%s", mLastClientMove > mFirstMoveIndex ? "" : " but none there");
#endif
            if (mLastClientMove > mFirstMoveIndex)
                mLastClientMove--;
            else
                tickDiff--;
            dropTicks--;
        }
        AssertFatal(mLastClientMove >= mFirstMoveIndex, "Bad move request");
        AssertFatal(mLastClientMove - mFirstMoveIndex <= mMoveList.size(), "Desynched first and last move.");
    }
    else
    {
        // Client ticked more than server.  Adjust for this by taking extra moves
        // (either adding back moves that were dropped above, or taking new ones).
        for (S32 i = 0; i < -tickDiff; i++)
        {
            if (mMoveList.size() > mLastClientMove - mFirstMoveIndex)
            {
#ifdef TORQUE_DEBUG_NET_MOVES
                Con::printf("add back move");
#endif
                mLastClientMove++;
            }
            else
            {
#ifdef TORQUE_DEBUG_NET_MOVES
                Con::printf("add back move -- create one");
#endif
                collectMove();
                mLastClientMove++;
            }
        }
    }

    // drop moves that are not made yet (because we rolled them back) and not yet sent   
    AssertFatal(mLastSentMove < mFirstMoveIndex || mLastClientMove < mFirstMoveIndex, "MoveList::updateClientServerTickDiff: Negative MoveList length!");
    U32 len;
    if (mLastSentMove < mFirstMoveIndex || mLastClientMove < mFirstMoveIndex)
        len = getMax((S32)(mLastClientMove - mFirstMoveIndex), (S32)(mLastSentMove - mFirstMoveIndex));
    else
        len = getMax(mLastClientMove - mFirstMoveIndex, mLastSentMove - mFirstMoveIndex);
    mMoveList.setSize(len);

#ifdef TORQUE_DEBUG_NET_MOVES
    Con::printf("move list size: %i, last move: %i, last sent: %i", mMoveList.size(), mLastClientMove - mFirstMoveIndex, mLastSentMove - mFirstMoveIndex);
#endif      
}

S32 MoveList::getServerTicks(U32 serverTickNum)
{
    S32 serverTicks = 0;
    if (serverTicksInitialized())
    {
        // handle tick wrapping...
        const S32 MaxTickCount = (1 << TotalTicksBits);
        const S32 HalfMaxTickCount = MaxTickCount >> 1;
        U32 prevTickNum = mTotalServerTicks & TotalTicksMask;
        serverTicks = serverTickNum - prevTickNum;
        if (serverTicks > HalfMaxTickCount)
            serverTicks -= MaxTickCount;
        else if (-serverTicks > HalfMaxTickCount)
            serverTicks += MaxTickCount;
        AssertFatal(serverTicks >= 0, "Server can't tick backwards!!!");
        if (serverTicks < 0)
            serverTicks = 0;
    }
    mTotalServerTicks = serverTickNum;
    return serverTicks;
}
#endif

void MoveList::markControlDirty()
{
    mLastClientMove = mLastMoveAck;

#ifdef TORQUE_HIFI
    // save state for future update
    GameBase* obj = mConnection->getControlObject();
    AssertFatal(obj, "ClientProcessList::markControlDirty: no control object");
    obj->setGhostUpdated(true);
    obj->getTickCache().beginCacheList();
    TickCacheEntry* tce = obj->getTickCache().incCacheList();
    BitStream bs(tce->packetData, TickCacheEntry::MaxPacketSize);
    obj->writePacketData(mConnection, &bs);
#endif
}

#ifdef TORQUE_HIFI
void MoveList::resetMoveList()
{
    mMoveList.clear();
    mLastMoveAck = 0;
    mLastClientMove = 0;
    mFirstMoveIndex = 0;
    mLastSentMove = 0;
}
#endif

bool MoveList::getNextMove(Move& curMove)
{
    if (mMoveList.size() > MaxMoveQueueSize)
        return false;

    F32 pitchAdd = MoveManager::mPitchUpSpeed - MoveManager::mPitchDownSpeed;
    F32 yawAdd = MoveManager::mYawLeftSpeed - MoveManager::mYawRightSpeed;
    F32 rollAdd = MoveManager::mRollRightSpeed - MoveManager::mRollLeftSpeed;

    curMove.pitch = MoveManager::mPitch + pitchAdd;
    curMove.yaw = MoveManager::mYaw + yawAdd;
    curMove.roll = MoveManager::mRoll + rollAdd;

    MoveManager::mPitch = 0;
    MoveManager::mYaw = 0;
    MoveManager::mRoll = 0;

    curMove.x = MoveManager::mRightAction - MoveManager::mLeftAction + MoveManager::mXAxis_L;
    curMove.y = MoveManager::mForwardAction - MoveManager::mBackwardAction + MoveManager::mYAxis_L;
    curMove.z = MoveManager::mUpAction - MoveManager::mDownAction;

    curMove.freeLook = MoveManager::mFreeLook;
    curMove.deviceIsKeyboardMouse = MoveManager::mDeviceIsKeyboardMouse;

    for (U32 i = 0; i < MaxTriggerKeys; i++)
    {
        curMove.trigger[i] = false;
        if (MoveManager::mTriggerCount[i] & 1)
            curMove.trigger[i] = true;
        else if (!(MoveManager::mPrevTriggerCount[i] & 1) && MoveManager::mPrevTriggerCount[i] != MoveManager::mTriggerCount[i])
            curMove.trigger[i] = true;
        MoveManager::mPrevTriggerCount[i] = MoveManager::mTriggerCount[i];
    }

    if (mConnection->getControlObject())
        mConnection->getControlObject()->preprocessMove(&curMove);

    curMove.clamp();  // clamp for net traffic
    return true;
}

void MoveList::pushMove(const Move& mv)
{
    U32 id = mFirstMoveIndex + mMoveList.size();
    U32 sz = mMoveList.size();
    mMoveList.push_back(mv);
    mMoveList[sz].id = id;
    mMoveList[sz].sendCount = 0;
}

U32 MoveList::getMoveList(Move** movePtr, U32* numMoves)
{
    if (mConnection->isConnectionToServer())
    {
        // give back moves starting at the last client move...

        AssertFatal(mLastClientMove >= mFirstMoveIndex, "Bad move request");
        AssertFatal(mLastClientMove - mFirstMoveIndex <= mMoveList.size(), "Desynched first and last move.");
        *numMoves = mMoveList.size() - mLastClientMove + mFirstMoveIndex;
        *movePtr = mMoveList.address() + mLastClientMove - mFirstMoveIndex;
    }
    else
    {
#ifdef TORQUE_HIFI
        // On the server we keep our own move list.
        * numMoves = mMoveList.size();
        mAvgMoveQueueSize *= (1.0f - mSmoothMoveAvg);
        mAvgMoveQueueSize += mSmoothMoveAvg * F32(*numMoves);

#ifdef TORQUE_DEBUG_NET_MOVES
        Con::printf("moves remaining: %i, running avg: %f", *numMoves, mAvgMoveQueueSize);
#endif

        if (mAvgMoveQueueSize < mTargetMoveListSize - mMoveListSizeSlack && *numMoves < mTargetMoveListSize && *numMoves)
        {
            *numMoves = 0;
            mAvgMoveQueueSize = (F32)getMax(U32(mAvgMoveQueueSize + mMoveListSizeSlack + 0.5f), *numMoves);

#ifdef TORQUE_DEBUG_NET_MOVES
            Con::printf("too few moves on server, padding with null move");
#endif
        }
        if (*numMoves)
            *numMoves = 1;

        if (mMoveList.size() > mMaxMoveListSize || (mAvgMoveQueueSize > mTargetMoveListSize + mMoveListSizeSlack && mMoveList.size() > mTargetMoveListSize))
        {
            U32 drop = mMoveList.size() - mTargetMoveListSize;
            clearMoves(drop);
            mAvgMoveQueueSize = (F32)mTargetMoveListSize;

#ifdef TORQUE_DEBUG_NET_MOVES
            Con::printf("too many moves on server, dropping moves (%i)", drop);
#endif
        }

        *movePtr = mMoveList.begin();
#else
        *numMoves = (mMoveList.size() < mMoveCredit) ? mMoveList.size() : mMoveCredit;
        *movePtr = mMoveList.begin();

        mMoveCredit -= *numMoves;
        mMoveList.setSize(*numMoves);
#endif
    }

    return *numMoves;
}

void MoveList::collectMove()
{
    Move mv;
    if (!mConnection->isPlayingBack() && getNextMove(mv))
    {
        mv.checksum = Move::ChecksumMismatch;
        pushMove(mv);
        mConnection->recordBlock(GameConnection::BlockTypeMove, sizeof(Move), &mv);
    }
}

void MoveList::clearMoves(U32 count)
{
    if (mConnection->isConnectionToServer())
    {
        mLastClientMove += count;
        AssertFatal(mLastClientMove >= mFirstMoveIndex, "Bad move request");
        AssertFatal(mLastClientMove - mFirstMoveIndex <= mMoveList.size(), "Desynched first and last move.");
    }
    else
    {
        AssertFatal(count <= mMoveList.size(), "GameConnection: Clearing too many moves");
        for (S32 i = 0; i < count; i++)
            if (mMoveList[i].checksum == Move::ChecksumMismatch)
                mControlMismatch = true;
            else
                mControlMismatch = false;
        if (count == mMoveList.size())
            mMoveList.clear();
        else
            while (count--)
                mMoveList.pop_front();
    }
}

#ifndef TORQUE_HIFI
void MoveList::incMoveCredit(U32 ticks)
{
    AssertFatal(!mConnection->isConnectionToServer(), "Cannot inc move credit on the client.");
    // Game tick increment
    mMoveCredit += ticks;
    if (mMoveCredit > MaxMoveCount)
        mMoveCredit = MaxMoveCount;

    // Clear pending moves for the elapsed time if there
    // is no control object.
    if (mConnection->getControlObject() == NULL)
        mMoveList.clear();
}
#endif

bool MoveList::areMovesPending()
{
    return mConnection->isConnectionToServer() ?
        mMoveList.size() - mLastClientMove + mFirstMoveIndex :
        mMoveList.size();
}

bool MoveList::isBacklogged()
{
    // If there are no pending moves and the input queue is full,
    // then the connection to the server must be clogged.
    if (!mConnection->isConnectionToServer())
        return false;
    return mLastClientMove - mFirstMoveIndex == mMoveList.size() &&
        mMoveList.size() >= MaxMoveCount;
}


void MoveList::clientWriteMovePacket(BitStream* bstream)
{
#ifdef TORQUE_HIFI
    if (!serverTicksInitialized())
        resetMoveList();
#endif

    AssertFatal(mLastMoveAck == mFirstMoveIndex, "Invalid move index.");
#ifdef TORQUE_HIFI
    // enforce limit on number of moves sent
    if (mLastSentMove < mFirstMoveIndex)
        mLastSentMove = mFirstMoveIndex;
    U32 count = mLastSentMove - mFirstMoveIndex;
#else
    U32 count = mMoveList.size();
#endif

    Move* move = mMoveList.address();
    U32 start = mLastMoveAck;
    U32 offset;
    for (offset = 0; offset < count; offset++)
        if (move[offset].sendCount < MAX_MOVE_PACKET_SENDS)
            break;
    if (offset == count && count != 0)
        offset--;

    start += offset;
    count -= offset;

    if (count > MaxMoveCount)
        count = MaxMoveCount;
    bstream->writeInt(start, 32);
    bstream->writeInt(count, MoveCountBits);
    Move* prevMove = NULL;
    for (int i = 0; i < count; i++)
    {
        move[offset + i].sendCount++;
        move[offset + i].pack(bstream, prevMove);
        bstream->writeInt(move[offset + i].checksum, Move::ChecksumBits);
        prevMove = &move[offset + i];
    }
}

void MoveList::serverReadMovePacket(BitStream* bstream)
{
    // Server side packet read.
    U32 start = bstream->readInt(32);
    U32 count = bstream->readInt(MoveCountBits);

    Move* prevMove = NULL;
    Move prevMoveHolder;

    // Skip forward (must be starting up), or over the moves
    // we already have.
    int skip = mLastMoveAck - start;
    if (skip < 0)
    {
        mLastMoveAck = start;
    }
    else
    {
        if (skip > count)
            skip = count;
        for (int i = 0; i < skip; i++)
        {
            prevMoveHolder.unpack(bstream, prevMove);
            prevMoveHolder.checksum = bstream->readInt(Move::ChecksumBits);
            prevMove = &prevMoveHolder;
            S32 idx = mMoveList.size() - skip + i;
            if (idx >= 0)
            {
#ifdef TORQUE_DEBUG_NET_MOVES
                if (mMoveList[idx].checksum != prevMoveHolder.checksum)
                    Con::printf("updated checksum on move %i from %i to %i", mMoveList[idx].id, mMoveList[idx].checksum, prevMoveHolder.checksum);
#endif
                mMoveList[idx].checksum = prevMoveHolder.checksum;
            }
        }
        start += skip;
        count = count - skip;
    }

    // Put the rest on the move list.
    int index = mMoveList.size();
    mMoveList.increment(count);
    while (index < mMoveList.size())
    {
        mMoveList[index].unpack(bstream, prevMove);
        mMoveList[index].checksum = bstream->readInt(Move::ChecksumBits);
        prevMove = &mMoveList[index];
        mMoveList[index].id = start++;
        index++;
    }

    mLastMoveAck += count;

#ifdef TORQUE_HIFI
    mLastMoveAck += count;

    if (mMoveList.size() > mMaxMoveListSize)
    {
        U32 drop = mMoveList.size() - mTargetMoveListSize;
        clearMoves(drop);
        mAvgMoveQueueSize = (F32)mTargetMoveListSize;

#ifdef TORQUE_DEBUG_NET_MOVES
        Con::printf("too many moves on server, dropping moves (%i)", drop);
#endif
    }
#endif
}

void MoveList::writeDemoStartBlock(ResizeBitStream* stream)
{
    stream->write(mLastMoveAck);
    stream->write(mLastClientMove);
    stream->write(mFirstMoveIndex);

    stream->write(U32(mMoveList.size()));
    for (U32 j = 0; j < mMoveList.size(); j++)
        mMoveList[j].pack(stream);
}

void MoveList::readDemoStartBlock(BitStream* stream)
{
    stream->read(&mLastMoveAck);
    stream->read(&mLastClientMove);
    stream->read(&mFirstMoveIndex);

    U32 size;
    Move mv;
    stream->read(&size);
    mMoveList.clear();
    while (size--)
    {
        mv.unpack(stream);
        pushMove(mv);
    }
}

void MoveList::serverWriteMovePacket(BitStream* bstream)
{
#ifdef TORQUE_DEBUG_NET_MOVES
    Con::printf("ack %i minus %i", mLastMoveAck, mMoveList.size());
#endif

    // acknowledge only those moves that have been ticked
    bstream->writeInt(mLastMoveAck - mMoveList.size(), 32);

#ifdef TORQUE_HIFI
    // send over the current tick count on the server...
    bstream->writeInt(gServerProcessList.getTotalTicks() & TotalTicksMask, TotalTicksBits);
#endif
}

void MoveList::clientReadMovePacket(BitStream* bstream)
{
#ifdef TORQUE_HIFI
    if (!serverTicksInitialized())
        resetMoveList();
#endif

#ifdef TORQUE_DEBUG_NET_MOVES
    Con::printf("pre move ack: %i", mLastMoveAck);
#endif

    mLastMoveAck = bstream->readInt(32);

#ifdef TORQUE_DEBUG_NET_MOVES
    Con::printf("post move ack %i, first move %i, last move %i", mLastMoveAck, mFirstMoveIndex, mLastClientMove);
#endif

#ifdef TORQUE_HIFI
    // This is how many times we've ticked since last ack -- before adjustments below
    S32 ourTicks = mLastMoveAck - mFirstMoveIndex;
#endif

    if (mLastMoveAck < mFirstMoveIndex)
        mLastMoveAck = mFirstMoveIndex;

    if (mLastMoveAck > mLastClientMove)
    {
#ifdef TORQUE_HIFI
        ourTicks -= mLastMoveAck - mLastClientMove;
#endif
        mLastClientMove = mLastMoveAck;
    }
    while (mFirstMoveIndex < mLastMoveAck)
    {
        if (mMoveList.size())
        {
            mMoveList.pop_front();
            mFirstMoveIndex++;
        }
        else
        {
            AssertWarn(1, "Popping off too many moves!");
            mFirstMoveIndex = mLastMoveAck;
        }
    }

#ifdef TORQUE_HIFI
    // get server ticks using total number of ticks on server to date...
    U32 serverTickNum = bstream->readInt(TotalTicksBits);
    S32 serverTicks = getServerTicks(serverTickNum);
    S32 tickDiff = serverTicks - ourTicks;

#ifdef TORQUE_DEBUG_NET_MOVES
    Con::printf("server ticks: %i, client ticks: %i, diff: %i%s", serverTicks, ourTicks, tickDiff, !tickDiff ? "" : " (ticks mis-match)");
#endif


    // Apply the first (of two) client-side synchronization mechanisms.  Key is that
    // we need to both synchronize client/server move streams (so first move in list is made
    // at same "time" on both client and server) and maintain the "time" at which the most
    // recent move was made on the server.  In both cases, "time" is the number of ticks
    // it took to get to that move.
    updateClientServerTickDiff(tickDiff);

    // Apply the second (and final) client-side synchronization mechanism.  The tickDiff adjustments above 
    // make sure time is preserved on client.  But that assumes that a future (or previous) update will adjust
    // time in the other direction, so that we don't get too far behind or ahead of the server.  The updateMoveSync
    // mechanism tracks us over time to make sure we eventually return to be in sync, and makes adjustments
    // if we don't after a certain time period (number of updates).  Unlike the tickDiff mechanism, when
    // the updateMoveSync acts time is not preserved on the client.
    gClientProcessList.updateMoveSync(mLastSentMove - mLastClientMove);

    // set catchup parameters...
    U32 totalCatchup = mLastClientMove - mFirstMoveIndex;

    gClientProcessList.ageTickCache(ourTicks + (tickDiff > 0 ? tickDiff : 0), totalCatchup + 1);
    gClientProcessList.forceHifiReset(tickDiff != 0);
    gClientProcessList.setCatchup(totalCatchup);
#endif
}

void MoveList::ghostPreRead(NetObject* nobj, bool newGhost)
{
#ifdef TORQUE_HIFI
    if ((nobj->getType() & GameBaseHiFiObjectType) && !newGhost)
    {
        AssertFatal(dynamic_cast<GameBase*>(nobj), "Should be a gamebase");
        GameBase* obj = static_cast<GameBase*>(nobj);

        // set next cache entry to start
        obj->getTickCache().beginCacheList();

        // reset to old state because we are about to unpack (and then tick forward)
        TickCacheEntry* tce = obj->getTickCache().incCacheList(false);
        if (tce)
        {
            BitStream bs(tce->packetData, TickCacheEntry::MaxPacketSize);
            obj->readPacketData(mConnection, &bs);
        }
    }
#endif
}

void MoveList::ghostReadExtra(NetObject* nobj, BitStream* bstream, bool newGhost)
{
#ifdef TORQUE_HIFI
    // Receive additional per ghost information.
    // Get pending moves for ghosts that have them and add the moves to
    // the tick cache.
    if (nobj->getType() & GameBaseHiFiObjectType)
    {
        AssertFatal(dynamic_cast<GameBase*>(nobj), "Should be a gamebase");
        GameBase* obj = static_cast<GameBase*>(nobj);

        // mark ghost so that it updates correctly
        obj->setGhostUpdated(true);
        obj->setNewGhost(newGhost);

        // set next cache entry to start
        obj->getTickCache().beginCacheList();

        // save state for future update
        TickCacheEntry* tce = obj->getTickCache().incCacheList();
        BitStream bs(tce->packetData, TickCacheEntry::MaxPacketSize);
        obj->writePacketData(mConnection, &bs);
    }
#endif
}