//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "platform/platform.h"
#include "core/dnet.h"
#include "console/consoleTypes.h"
#include "console/simBase.h"
#include "game/gameConnection.h"
#include "game/shapeBase.h"
#include "core/bitStream.h"
#include "sim/pathManager.h"
#include "game/game.h"
#include "sceneGraph/sceneGraph.h"
#include "game/gameConnectionEvents.h"

bool MoveManager::mDeviceIsKeyboardMouse = false;
bool MoveManager::mAutoCenterCamera = false;
F32 MoveManager::mForwardAction = 0;
F32 MoveManager::mBackwardAction = 0;
F32 MoveManager::mUpAction = 0;
F32 MoveManager::mDownAction = 0;
F32 MoveManager::mLeftAction = 0;
F32 MoveManager::mRightAction = 0;

bool MoveManager::mFreeLook = false;
F32 MoveManager::mPitch = 0;
F32 MoveManager::mYaw = 0;
F32 MoveManager::mRoll = 0;

F32 MoveManager::mPitchUpSpeed = 0;
F32 MoveManager::mPitchDownSpeed = 0;
F32 MoveManager::mYawLeftSpeed = 0;
F32 MoveManager::mYawRightSpeed = 0;
F32 MoveManager::mRollLeftSpeed = 0;
F32 MoveManager::mRollRightSpeed = 0;

F32 MoveManager::mXAxis_L = 0;
F32 MoveManager::mYAxis_L = 0;
F32 MoveManager::mXAxis_R = 0;
F32 MoveManager::mYAxis_R = 0;

U32 MoveManager::mTriggerCount[MaxTriggerKeys] = { 0, };
U32 MoveManager::mPrevTriggerCount[MaxTriggerKeys] = { 0, };

F32 MoveManager::mHorizontalDeadZone = 0.0f;
F32 MoveManager::mVerticalDeadZone = 0.0f;
F32 MoveManager::mCameraAccelSpeed = 0.0f;
F32 MoveManager::mCameraSensitivityHorizontal = 0.0f;
F32 MoveManager::mCameraSensitivityVertical = 0.0f;

#define MAX_MOVE_PACKET_SENDS 4

const Move NullMove =
{
   16,16,16,
   0,0,0,
   0,0,0,   // x,y,z
   0,0,0,   // Yaw, pitch, roll,
   0,0,

   false,

   false,
   false,
   false,

   false, false, false, false,false,false,
   0, 0, 0, 0, 0,
   0.0f, 0.0f, 0.0f, 0.0f, 0.0f
};

void MoveManager::init()
{
    Con::addVariable("mvForwardAction", TypeF32, &mForwardAction);
    Con::addVariable("mvBackwardAction", TypeF32, &mBackwardAction);
    Con::addVariable("mvUpAction", TypeF32, &mUpAction);
    Con::addVariable("mvDownAction", TypeF32, &mDownAction);
    Con::addVariable("mvLeftAction", TypeF32, &mLeftAction);
    Con::addVariable("mvRightAction", TypeF32, &mRightAction);

    Con::addVariable("mvFreeLook", TypeBool, &mFreeLook);
    Con::addVariable("mvDeviceIsKeyboardMouse", TypeBool, &mDeviceIsKeyboardMouse);
    Con::addVariable("mvAutoCenterCamera", TypeBool, &mAutoCenterCamera);
    Con::addVariable("mvPitch", TypeF32, &mPitch);
    Con::addVariable("mvYaw", TypeF32, &mYaw);
    Con::addVariable("mvRoll", TypeF32, &mRoll);
    Con::addVariable("mvPitchUpSpeed", TypeF32, &mPitchUpSpeed);
    Con::addVariable("mvPitchDownSpeed", TypeF32, &mPitchDownSpeed);
    Con::addVariable("mvYawLeftSpeed", TypeF32, &mYawLeftSpeed);
    Con::addVariable("mvYawRightSpeed", TypeF32, &mYawRightSpeed);
    Con::addVariable("mvRollLeftSpeed", TypeF32, &mRollLeftSpeed);
    Con::addVariable("mvRollRightSpeed", TypeF32, &mRollRightSpeed);

    // Dual-analog
    Con::addVariable("mvXAxis_L", TypeF32, &mXAxis_L);
    Con::addVariable("mvYAxis_L", TypeF32, &mYAxis_L);

    Con::addVariable("mvXAxis_R", TypeF32, &mXAxis_R);
    Con::addVariable("mvYAxis_R", TypeF32, &mYAxis_R);

    for (U32 i = 0; i < MaxTriggerKeys; i++)
    {
        char varName[256];
        dSprintf(varName, sizeof(varName), "mvTriggerCount%d", i);
        Con::addVariable(varName, TypeS32, &mTriggerCount[i]);
    }

    Con::addVariable("mvHorizontalDeadZone", TypeF32, &mHorizontalDeadZone);
    Con::addVariable("mvVerticalDeadZone", TypeF32, &mVerticalDeadZone);
    Con::addVariable("mvCameraAccelSpeed", TypeF32, &mCameraAccelSpeed);
    Con::addVariable("mvCameraSensitivityHorizontal", TypeF32, &mCameraSensitivityHorizontal);
    Con::addVariable("mvCameraSensitivityVertical", TypeF32, &mCameraSensitivityVertical);
}

static inline F32 clampFloatWrap(F32 val)
{
    return val - F32(S32(val));
}

static F32 clampFloatClamp(F32 val, U32 bits)
{
    if (val < 0)
        val = 0;
    else if (val > 1)
        val = 1;
    F32 mask = (1 << bits);
    return U32(val * mask) / F32(mask);
}

static inline S32 clampRangeClamp(F32 val)
{
    if (val < -1)
        return 0;
    if (val > 1)
        return 32;
    return (S32)((val + 1) * 16);
}


#define FANG2IANG(x)   ((U32)((S16)((F32(0x10000) / M_2PI) * x)) & 0xFFFF)
#define IANG2FANG(x)   (F32)((M_2PI / F32(0x10000)) * (F32)((S16)x))

void Move::unclamp()
{
    yaw = IANG2FANG(pyaw);
    pitch = IANG2FANG(ppitch);
    roll = IANG2FANG(proll);

    x = (px - 16) / F32(16);
    y = (py - 16) / F32(16);
    z = (pz - 16) / F32(16);

    horizontalDeadZone = IANG2FANG(pHorizontalDeadZone);
    verticalDeadZone = IANG2FANG(pVerticalDeadZone);
    cameraAccelSpeed = IANG2FANG(pCameraAccelSpeed);
    cameraSensitivityHorizontal = IANG2FANG(pCameraSensitivityHorizontal);
    cameraSensitivityVertical = IANG2FANG(pCameraSensitivityVertical);
}

void Move::clamp()
{
    // clamp before FANG2IANG, or else angles can become negative
    const F32 angleClamp = M_PI_F - 0.000001f;
    yaw = mClampF(yaw, -angleClamp, angleClamp);
    pitch = mClampF(pitch, -angleClamp, angleClamp);
    roll = mClampF(roll, -angleClamp, angleClamp);

    // angles are all 16 bit.
    pyaw = FANG2IANG(yaw);
    ppitch = FANG2IANG(pitch);
    proll = FANG2IANG(roll);

    px = clampRangeClamp(x);
    py = clampRangeClamp(y);
    pz = clampRangeClamp(z);

    pHorizontalDeadZone = FANG2IANG(horizontalDeadZone);
    pVerticalDeadZone = FANG2IANG(verticalDeadZone);
    pCameraAccelSpeed = FANG2IANG(cameraAccelSpeed);
    pCameraSensitivityHorizontal = FANG2IANG(cameraSensitivityHorizontal);
    pCameraSensitivityVertical = FANG2IANG(cameraSensitivityVertical);

    unclamp();
}

void Move::pack(BitStream* stream, const Move* baseMove)
{
    const Move* pBaseMove = baseMove;
    bool alwaysWriteAll = baseMove != NULL;
    if (!alwaysWriteAll)
        pBaseMove = &NullMove;

    bool somethingDifferent = false;
    bool triggerDifferent = false;
    for (U32 i = 0; i < MaxTriggerKeys; i++)
    {
        if (trigger[i] != pBaseMove->trigger[i])
            triggerDifferent = true;
    }

    if (pyaw != pBaseMove->pyaw || ppitch != pBaseMove->ppitch || proll != pBaseMove->proll ||
        px != pBaseMove->px || py != pBaseMove->py || pz != pBaseMove->pz ||
        deviceIsKeyboardMouse != pBaseMove->deviceIsKeyboardMouse || autoCenterCamera != pBaseMove->autoCenterCamera ||
        freeLook != pBaseMove->freeLook ||
        pHorizontalDeadZone != pBaseMove->pHorizontalDeadZone ||
        pVerticalDeadZone != pBaseMove->pVerticalDeadZone ||
        pCameraAccelSpeed != pBaseMove->pCameraAccelSpeed ||
        pCameraSensitivityHorizontal != pBaseMove->pCameraSensitivityHorizontal ||
        pCameraSensitivityVertical != pBaseMove->pCameraSensitivityVertical ||
        triggerDifferent)
    {
        somethingDifferent = true;
    }

    if (alwaysWriteAll || stream->writeFlag(somethingDifferent))
    {
        if (stream->writeFlag(pyaw != pBaseMove->pyaw))
            stream->writeInt(pyaw, 16);
        if (stream->writeFlag(ppitch != pBaseMove->ppitch))
            stream->writeInt(ppitch, 16);
        if (stream->writeFlag(proll != pBaseMove->proll))
            stream->writeInt(proll, 16);

        if (stream->writeFlag(px != pBaseMove->px))
            stream->writeInt(px, 6);
        if (stream->writeFlag(py != pBaseMove->py))
            stream->writeInt(py, 6);
        if (stream->writeFlag(pz != pBaseMove->pz))
            stream->writeInt(pz, 6);

        stream->writeFlag(freeLook);
        stream->writeFlag(deviceIsKeyboardMouse);
        stream->writeFlag(autoCenterCamera);

        if (stream->writeFlag(triggerDifferent))
        {
            for (U32 i = 0; i < MaxTriggerKeys; i++)
                stream->writeFlag(trigger[i]);
        }

        bool extrasDifferent = false;
        if (pHorizontalDeadZone != pBaseMove->pHorizontalDeadZone ||
            pVerticalDeadZone != pBaseMove->pVerticalDeadZone ||
            pCameraAccelSpeed != pBaseMove->pCameraAccelSpeed ||
            pCameraSensitivityHorizontal != pBaseMove->pCameraSensitivityHorizontal ||
            pCameraSensitivityVertical != pBaseMove->pCameraSensitivityVertical)
        {
            extrasDifferent = true;
        }

        if (alwaysWriteAll || stream->writeFlag(extrasDifferent))
        {
            if (stream->writeFlag(pHorizontalDeadZone != pBaseMove->pHorizontalDeadZone))
                stream->writeInt(pHorizontalDeadZone, 16);
            if (stream->writeFlag(pVerticalDeadZone != pBaseMove->pVerticalDeadZone))
                stream->writeInt(pVerticalDeadZone, 16);
            if (stream->writeFlag(pCameraAccelSpeed != pBaseMove->pCameraAccelSpeed))
                stream->writeInt(pCameraAccelSpeed, 16);
            if (stream->writeFlag(pCameraSensitivityHorizontal != pBaseMove->pCameraSensitivityHorizontal))
                stream->writeInt(pCameraSensitivityHorizontal, 16);
            if (stream->writeFlag(pCameraSensitivityVertical != pBaseMove->pCameraSensitivityVertical))
                stream->writeInt(pCameraSensitivityVertical, 16);
        }
    }
}

void Move::unpack(BitStream* stream, const Move* baseMove)
{
    const Move* pBaseMove = baseMove;
    bool alwaysReadAll = baseMove != NULL;
    if (!alwaysReadAll)
        pBaseMove = &NullMove;

    if (alwaysReadAll || stream->readFlag())
    {
        if (stream->readFlag())
            pyaw = stream->readInt(16);
        else
            pyaw = pBaseMove->pyaw;
        if (stream->readFlag())
            ppitch = stream->readInt(16);
        else
            ppitch = pBaseMove->ppitch;
        if (stream->readFlag())
            proll = stream->readInt(16);
        else
            proll = pBaseMove->proll;

        if (stream->readFlag())
            px = stream->readInt(6);
        else
            px = pBaseMove->px;

        if (stream->readFlag())
            py = stream->readInt(6);
        else
            py = pBaseMove->py;

        if (stream->readFlag())
            pz = stream->readInt(6);
        else
            pz = pBaseMove->pz;

        freeLook = stream->readFlag();
        deviceIsKeyboardMouse = stream->readFlag();
        autoCenterCamera = stream->readFlag();

        bool triggersDiffer = stream->readFlag();
        for (U32 i = 0; i < MaxTriggerKeys; i++)
        {
            if (triggersDiffer)
                trigger[i] = stream->readFlag();
            else
                trigger[i] = pBaseMove->trigger[i];
        }

        if (alwaysReadAll || stream->readFlag())
        {
            if (stream->readFlag())
                pHorizontalDeadZone = stream->readInt(16);
            else
                pHorizontalDeadZone = pBaseMove->pHorizontalDeadZone;

            if (stream->readFlag())
                pVerticalDeadZone = stream->readInt(16);
            else
                pVerticalDeadZone = pBaseMove->pVerticalDeadZone;

            if (stream->readFlag())
                pCameraAccelSpeed = stream->readInt(16);
            else
                pCameraAccelSpeed = pBaseMove->pCameraAccelSpeed;

            if (stream->readFlag())
                pCameraSensitivityHorizontal = stream->readInt(16);
            else
                pCameraSensitivityHorizontal = pBaseMove->pCameraSensitivityHorizontal;

            if (stream->readFlag())
                pCameraSensitivityVertical = stream->readInt(16);
            else
                pCameraSensitivityVertical = pBaseMove->pCameraSensitivityVertical;
        }

        unclamp();
    } else
    {
        dMemcpy(this, pBaseMove, sizeof(Move));
    }
}

bool GameConnection::getNextMove(Move& curMove)
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
    curMove.autoCenterCamera = MoveManager::mAutoCenterCamera;

    for (U32 i = 0; i < MaxTriggerKeys; i++)
    {
        curMove.trigger[i] = false;
        if (MoveManager::mTriggerCount[i] & 1)
            curMove.trigger[i] = true;
        else if (!(MoveManager::mPrevTriggerCount[i] & 1) && MoveManager::mPrevTriggerCount[i] != MoveManager::mTriggerCount[i])
            curMove.trigger[i] = true;
        MoveManager::mPrevTriggerCount[i] = MoveManager::mTriggerCount[i];
    }

    curMove.horizontalDeadZone = MoveManager::mHorizontalDeadZone;
    curMove.verticalDeadZone = MoveManager::mVerticalDeadZone;
    curMove.cameraAccelSpeed = MoveManager::mCameraAccelSpeed;
    curMove.cameraSensitivityHorizontal = MoveManager::mCameraSensitivityHorizontal;
    curMove.cameraSensitivityVertical = MoveManager::mCameraSensitivityVertical;

    curMove.clamp();  // clamp for net traffic
    return true;
}

#ifdef MB_CLIENT_PHYSICS_EVERY_FRAME
bool GameConnection::getNextMove2(Move& curMove)
{
    if (mMoveList.size() > MaxMoveQueueSize)
        return false;

    F32 pitchAdd = MoveManager::mPitchUpSpeed - MoveManager::mPitchDownSpeed;
    F32 yawAdd = MoveManager::mYawLeftSpeed - MoveManager::mYawRightSpeed;
    F32 rollAdd = MoveManager::mRollRightSpeed - MoveManager::mRollLeftSpeed;

    curMove.pitch = MoveManager::mPitch + pitchAdd;
    curMove.yaw = MoveManager::mYaw + yawAdd;
    curMove.roll = MoveManager::mRoll + rollAdd;

    //MoveManager::mPitch = 0;
    //MoveManager::mYaw = 0;
    //MoveManager::mRoll = 0;


    curMove.x = MoveManager::mRightAction - MoveManager::mLeftAction + MoveManager::mXAxis_L;
    curMove.y = MoveManager::mForwardAction - MoveManager::mBackwardAction + MoveManager::mYAxis_L;
    curMove.z = MoveManager::mUpAction - MoveManager::mDownAction;

    curMove.freeLook = MoveManager::mFreeLook;
    curMove.deviceIsKeyboardMouse = MoveManager::mDeviceIsKeyboardMouse;
    curMove.autoCenterCamera = MoveManager::mAutoCenterCamera;

    for (U32 i = 0; i < MaxTriggerKeys; i++)
    {
        curMove.trigger[i] = false;
        if (MoveManager::mTriggerCount[i] & 1)
            curMove.trigger[i] = true;
        else if (!(MoveManager::mPrevTriggerCount[i] & 1) && MoveManager::mPrevTriggerCount[i] != MoveManager::mTriggerCount[i])
            curMove.trigger[i] = true;
        //MoveManager::mPrevTriggerCount[i] = MoveManager::mTriggerCount[i];
    }

    curMove.horizontalDeadZone = MoveManager::mHorizontalDeadZone;
    curMove.verticalDeadZone = MoveManager::mVerticalDeadZone;
    curMove.cameraAccelSpeed = MoveManager::mCameraAccelSpeed;
    curMove.cameraSensitivityHorizontal = MoveManager::mCameraSensitivityHorizontal;
    curMove.cameraSensitivityVertical = MoveManager::mCameraSensitivityVertical;

    curMove.clamp();  // clamp for net traffic
    return true;
}
#endif

void GameConnection::pushMove(const Move& mv)
{
    U32 id = mFirstMoveIndex + mMoveList.size();
    U32 sz = mMoveList.size();
    mMoveList.push_back(mv);
    mMoveList[sz].id = id;
    mMoveList[sz].sendCount = 0;
}

U32 GameConnection::getMoveList(Move** movePtr, U32* numMoves)
{
    if (isConnectionToServer())
    {
        // give back moves starting at the last client move...

        AssertFatal(mLastClientMove >= mFirstMoveIndex, "Bad move request");
        AssertFatal(mLastClientMove - mFirstMoveIndex <= mMoveList.size(), "Desynched first and last move.");
        *numMoves = mMoveList.size() - mLastClientMove + mFirstMoveIndex;
        *movePtr = mMoveList.address() + mLastClientMove - mFirstMoveIndex;
    }
    else
    {
        // On the server we keep our own move list.
        *numMoves = mMoveList.size();
        mAvgMoveQueueSize *= (1.0f-mSmoothMoveAvg);
        mAvgMoveQueueSize += mSmoothMoveAvg * F32(*numMoves);

    #ifdef TORQUE_DEBUG_NET_MOVES
        Con::printf("moves remaining: %i, running avg: %f",*numMoves,mAvgMoveQueueSize);
    #endif

        if (mAvgMoveQueueSize<mTargetMoveListSize-mMoveListSizeSlack && *numMoves<mTargetMoveListSize && *numMoves)
        {
            *numMoves=0;
            mAvgMoveQueueSize = (F32)getMax(U32(mAvgMoveQueueSize + mMoveListSizeSlack + 0.5f),*numMoves);

    #ifdef TORQUE_DEBUG_NET_MOVES
            Con::printf("too few moves on server, padding with null move");
    #endif
        }
        if (*numMoves)
            *numMoves=1;

        if ( mMoveList.size()>mMaxMoveListSize || (mAvgMoveQueueSize>mTargetMoveListSize+mMoveListSizeSlack && mMoveList.size()>mTargetMoveListSize) )
        {
            U32 drop = mMoveList.size()-mTargetMoveListSize;
            clearMoves(drop);
            mAvgMoveQueueSize = (F32)mTargetMoveListSize;

    #ifdef TORQUE_DEBUG_NET_MOVES
            Con::printf("too many moves on server, dropping moves (%i)",drop);
    #endif
        }

        *movePtr = mMoveList.begin();
    }

    return *numMoves;
}

void GameConnection::resetClientMoves()
{
    mLastClientMove = mFirstMoveIndex;
}

void GameConnection::collectMove(U32 time)
{
    Move mv;
    if (!isPlayingBack() && getNextMove(mv))
    {
        mv.checksum = Move::ChecksumMismatch;
        pushMove(mv);
        recordBlock(BlockTypeMove, sizeof(Move), &mv);
    }
}

void GameConnection::clearMoves(U32 count)
{
    if (isConnectionToServer()) {
        mLastClientMove += count;
        AssertFatal(mLastClientMove >= mFirstMoveIndex, "Bad move request");
        AssertFatal(mLastClientMove - mFirstMoveIndex <= mMoveList.size(), "Desynched first and last move.");
    }
    else {
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

void GameConnection::incLastSentMove()
{
    if (mMoveList.size() > mLastSentMove - mFirstMoveIndex)
        mLastSentMove++;
}

bool GameConnection::areMovesPending()
{
    return isConnectionToServer() ?
        mMoveList.size() - mLastClientMove + mFirstMoveIndex :
        mMoveList.size();
}

bool GameConnection::isBacklogged()
{
    // If there are no pending moves and the input queue is full,
    // then the connection to the server must be clogged.
    if (!isConnectionToServer())
        return false;
    return mLastClientMove - mFirstMoveIndex == mMoveList.size() &&
        mMoveList.size() >= MaxMoveCount;
}


void GameConnection::moveWritePacket(BitStream* bstream)
{
    Move* move;
    U32 count;
    AssertFatal(mLastMoveAck == mFirstMoveIndex, "Invalid move index.");
    
    if (mLastSentMove < mFirstMoveIndex)
        mLastSentMove = mFirstMoveIndex;

    count = mLastSentMove - mFirstMoveIndex;
    move = mMoveList.address();

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
    Move* baseMove = NULL;
    for (int i = 0; i < count; i++)
    {
        move[offset + i].sendCount++;
        move[offset + i].pack(bstream, baseMove);
        bstream->writeInt(move[offset + i].checksum, Move::ChecksumBits);

        baseMove = &move[offset + i];
    }
}

void GameConnection::moveReadPacket(BitStream* bstream)
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
        for (S32 i = 0; i < skip; i++)
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


