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

    void init() {}

    void setConnection(GameConnection* connection) { mConnection = connection; }

    /// @name Move Packets
    /// Write/read move data to the packet.
    /// @{

    ///
    void ghostReadExtra(NetObject*, BitStream*, bool newGhost) {};
    void ghostWriteExtra(NetObject*, BitStream*) {}
    void ghostPreRead(NetObject*, bool newGhost) {}

    void clientWriteMovePacket(BitStream* bstream);
    void clientReadMovePacket(BitStream*);

    void serverWriteMovePacket(BitStream*);
    void serverReadMovePacket(BitStream* bstream);

    void writeDemoStartBlock(ResizeBitStream* stream);
    void readDemoStartBlock(BitStream* stream);
    /// @}

public:

    void collectMove();
    void pushMove(const Move& mv);
    U32 getMoveList(Move**, U32* numMoves);
    void clearMoves(U32 count);

    void markControlDirty() { mLastClientMove = mLastMoveAck; }
    bool isMismatch() { return mControlMismatch; }

    bool isBacklogged();
    void incMoveCredit(U32 ticks);

protected:
    bool getNextMove(Move& curMove);
    bool areMovesPending();

protected:

    U32 mLastMoveAck;
    U32 mLastClientMove;
    U32 mFirstMoveIndex;
    U32 mMoveCredit;
    bool mControlMismatch;

    GameConnection* mConnection;

    Vector<Move>     mMoveList;
};

#endif // _MOVELIST_H_
