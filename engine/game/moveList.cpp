//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "game/moveList.h"
#include "game/gameConnection.h"
#include "core/bitStream.h"
#include "game/gameBase.h"

#define MAX_MOVE_PACKET_SENDS 4


MoveList::MoveList()
{
    mLastMoveAck = 0;
    mLastClientMove = 0;
    mFirstMoveIndex = 0;
    mMoveCredit = MaxMoveCount;
    mControlMismatch = false;
    mConnection = NULL;
}

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
        *numMoves = (mMoveList.size() < mMoveCredit) ?
            mMoveList.size() : mMoveCredit;
        *movePtr = mMoveList.begin();

        mMoveCredit -= *numMoves;
        mMoveList.setSize(*numMoves);
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
    AssertFatal(mLastMoveAck == mFirstMoveIndex, "Invalid move index.");
    U32 count = mMoveList.size();

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
}

void MoveList::clientReadMovePacket(BitStream* bstream)
{
#ifdef TORQUE_DEBUG_NET_MOVES
    Con::printf("pre move ack: %i", mLastMoveAck);
#endif

    mLastMoveAck = bstream->readInt(32);

#ifdef TORQUE_DEBUG_NET_MOVES
    Con::printf("post move ack %i, first move %i, last move %i", mLastMoveAck, mFirstMoveIndex, mLastClientMove);
#endif

    if (mLastMoveAck < mFirstMoveIndex)
        mLastMoveAck = mFirstMoveIndex;

    if (mLastMoveAck > mLastClientMove)
        mLastClientMove = mLastMoveAck;
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
}
