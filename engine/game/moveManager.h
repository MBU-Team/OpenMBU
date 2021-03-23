//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _MOVEMANAGER_H_
#define _MOVEMANAGER_H_

enum MoveConstants {
    MaxTriggerKeys = 6,
    MaxMoveQueueSize = 45,
};

class BitStream;

struct Move
{
    // packed storage rep, set in clamp
    S32 px, py, pz;
    U32 pyaw, ppitch, proll;
    F32 x, y, z;          // float -1 to 1
    F32 yaw, pitch, roll; // 0-2PI
    U32 id;               // sync'd between server & client - debugging tool.
    U32 sendCount;

    bool deviceIsKeyboardMouse;
    bool freeLook;
    bool trigger[MaxTriggerKeys];

    void pack(BitStream* stream);
    void unpack(BitStream* stream);
    void clamp();
    void unclamp();
};

extern const Move NullMove;

class MoveManager
{
public:
    static bool mDeviceIsKeyboardMouse;
    static F32 mForwardAction;
    static F32 mBackwardAction;
    static F32 mUpAction;
    static F32 mDownAction;
    static F32 mLeftAction;
    static F32 mRightAction;

    static bool mFreeLook;
    static F32 mPitch;
    static F32 mYaw;
    static F32 mRoll;

    static F32 mPitchUpSpeed;
    static F32 mPitchDownSpeed;
    static F32 mYawLeftSpeed;
    static F32 mYawRightSpeed;
    static F32 mRollLeftSpeed;
    static F32 mRollRightSpeed;
    static F32 mXAxis_L;
    static F32 mYAxis_L;
    static F32 mXAxis_R;
    static F32 mYAxis_R;

    static U32 mTriggerCount[MaxTriggerKeys];
    static U32 mPrevTriggerCount[MaxTriggerKeys];

    static void init();
};

#endif
