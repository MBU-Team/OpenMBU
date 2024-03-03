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
    enum
    {
        ChecksumBits = 0x10,
        ChecksumMask = 0xFFFF,
        ChecksumMismatch = 0xFFFFFFFF,
    };

    // packed storage rep, set in clamp
    S32 px, py, pz;
    U32 pyaw, ppitch, proll;
    F32 x, y, z;          // float -1 to 1
    F32 yaw, pitch, roll; // 0-2PI
    U32 id;               // sync'd between server & client - debugging tool.
    U32 sendCount;
    U32 checksum;

    bool deviceIsKeyboardMouse;
    bool autoCenterCamera;
    bool freeLook;
    bool trigger[MaxTriggerKeys];

    // We may want to do this differently in the future, but for now let's do it as part of Move.
    U32 pHorizontalDeadZone, pVerticalDeadZone, pCameraAccelSpeed, pCameraSensitivityHorizontal, pCameraSensitivityVertical;
    F32 horizontalDeadZone;
    F32 verticalDeadZone;
    F32 cameraAccelSpeed;
    F32 cameraSensitivityHorizontal;
    F32 cameraSensitivityVertical;

    void pack(BitStream* stream, const Move* baseMove = NULL);
    void unpack(BitStream* stream, const Move* baseMove = NULL);
    void clamp();
    void unclamp();
};

extern const Move NullMove;

class MoveManager
{
public:
    static bool mDeviceIsKeyboardMouse;
    static bool mAutoCenterCamera;
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

    // We may want to do this differently in the future, but for now let's do it as part of Move.
    static F32 mHorizontalDeadZone;
    static F32 mVerticalDeadZone;
    static F32 mCameraAccelSpeed;
    static F32 mCameraSensitivityHorizontal;
    static F32 mCameraSensitivityVertical;

    static void init();
};

#endif
