//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _MOVELIST_H_
#define _MOVELIST_H_

#ifndef _MOVEMANAGER_H_
#include "game/moveManager.h"
#endif
#ifndef _TVECTOR_H_
#include "core/tVector.h"
#endif

class BitStream;
class ResizeBitStream;
class NetObject;
class GameConnection;

class MoveList
{
    enum PrivateConstants
    {
        MoveCountBits = 5,
        /// MaxMoveCount should not exceed the MoveManager's
        /// own maximum (MaxMoveQueueSize)
        MaxMoveCount = 30,
    };

public:

    MoveList();

#ifdef TORQUE_HIFI
    void init() { mTotalServerTicks = ServerTicksUninitialized; }
#else
    void init() {}
#endif

    void setConnection(GameConnection* connection) { mConnection = connection; }

    /// @name Move Packets
    /// Write/read move data to the packet.
    /// @{

    ///
    void ghostReadExtra(NetObject*, BitStream*, bool newGhost);
    void ghostWriteExtra(NetObject*, BitStream*) {}
    void ghostPreRead(NetObject*, bool newGhost);

    void clientWriteMovePacket(BitStream* bstream);
    void clientReadMovePacket(BitStream*);
    void serverWriteMovePacket(BitStream*);
    void serverReadMovePacket(BitStream* bstream);
    /// @}

    void writeDemoStartBlock(ResizeBitStream* stream);
    void readDemoStartBlock(BitStream* stream);

public:
#ifdef TORQUE_HIFI
    // but not members of base class
    void           resetClientMoves() { mLastClientMove = mFirstMoveIndex; }
    void           resetCatchup() { mLastClientMove = mLastMoveAck; }
#endif

public:

    void collectMove();
    void pushMove(const Move& mv);

    virtual U32 getMoveList(Move**, U32* numMoves);
    virtual bool areMovesPending();
    virtual void clearMoves(U32 count);

    void markControlDirty();
    bool isMismatch() { return mControlMismatch; }
    bool isBacklogged();

#ifdef TORQUE_HIFI
    void onAdvanceObjects() { if (mMoveList.size() > mLastSentMove - mFirstMoveIndex) mLastSentMove++; }
#endif

#ifndef TORQUE_HIFI
    void incMoveCredit(U32 ticks);
#endif

protected:
    bool getNextMove(Move& curMove);

#ifdef TORQUE_HIFI
    void resetMoveList();
#endif

protected:

#ifdef TORQUE_HIFI
    S32            getServerTicks(U32 serverTickNum);
    void           updateClientServerTickDiff(S32& tickDiff);
#endif

    U32 mLastMoveAck;
    U32 mLastClientMove;
    U32 mFirstMoveIndex;
#ifdef TORQUE_HIFI
    U32 mLastSentMove;
    F32 mAvgMoveQueueSize;
#else
    U32 mMoveCredit;
#endif
    bool mControlMismatch;

#ifdef TORQUE_HIFI
    // server side move list management
    U32 mTargetMoveListSize;       // Target size of move buffer on server
    U32 mMaxMoveListSize;          // Max size move buffer allowed to grow to
    F32 mSmoothMoveAvg;            // Smoothing parameter for move list size running average
    F32 mMoveListSizeSlack;         // Amount above/below target size move list running average allowed to diverge

    // client side tracking of server ticks
    enum { TotalTicksBits = 10, TotalTicksMask = (1 << TotalTicksBits) - 1, ServerTicksUninitialized = 0xFFFFFFFF };
    U32 mTotalServerTicks;

    bool serverTicksInitialized() { return mTotalServerTicks != ServerTicksUninitialized; }
#endif

    GameConnection* mConnection;

    Vector<Move>     mMoveList;
};

#endif // _MOVELIST_H_
