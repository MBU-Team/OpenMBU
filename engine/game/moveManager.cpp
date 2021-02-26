//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "core/bitStream.h"
#include "console/consoleTypes.h"
#include "math/mConstants.h"
#include "game/moveManager.h"

bool MoveManager::mDeviceIsKeyboardMouse = false;
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

#define MAX_MOVE_PACKET_SENDS 4

const Move NullMove =
{
   16,16,16,
   0,0,0,
   0,0,0,   // x,y,z
   0,0,0,   // Yaw, pitch, roll,
   0,0,

   false,
   false,false,false,false,false,false
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
}

void Move::clamp()
{
    // angles are all 16 bit.
    pyaw = FANG2IANG(yaw);
    ppitch = FANG2IANG(pitch);
    proll = FANG2IANG(roll);

    px = clampRangeClamp(x);
    py = clampRangeClamp(y);
    pz = clampRangeClamp(z);
    unclamp();
}

void Move::pack(BitStream* stream, const Move* basemove)
{
    bool alwaysWriteAll = basemove != NULL;
    if (!basemove)
        basemove = &NullMove;

    S32 i;
    bool triggerDifferent = false;
    for (i = 0; i < MaxTriggerKeys; i++)
        if (trigger[i] != basemove->trigger[i])
            triggerDifferent = true;
    bool somethingDifferent = (pyaw != basemove->pyaw) ||
        (ppitch != basemove->ppitch) ||
        (proll != basemove->proll) ||
        (px != basemove->px) ||
        (py != basemove->py) ||
        (pz != basemove->pz) ||
        (deviceIsKeyboardMouse != basemove->deviceIsKeyboardMouse) ||
        (freeLook != basemove->freeLook) ||
        triggerDifferent;

    if (alwaysWriteAll || stream->writeFlag(somethingDifferent))
    {
        if (stream->writeFlag(pyaw != basemove->pyaw))
            stream->writeInt(pyaw, 16);
        if (stream->writeFlag(ppitch != basemove->ppitch))
            stream->writeInt(ppitch, 16);
        if (stream->writeFlag(proll != basemove->proll))
            stream->writeInt(proll, 16);

        if (stream->writeFlag(px != basemove->px))
            stream->writeInt(px, 6);
        if (stream->writeFlag(py != basemove->py))
            stream->writeInt(py, 6);
        if (stream->writeFlag(pz != basemove->pz))
            stream->writeInt(pz, 6);
        stream->writeFlag(freeLook);
        stream->writeFlag(deviceIsKeyboardMouse);

        if (stream->writeFlag(triggerDifferent))
            for (i = 0; i < MaxTriggerKeys; i++)
                stream->writeFlag(trigger[i]);
    }
}

void Move::unpack(BitStream* stream, const Move* basemove)
{
    bool alwaysReadAll = basemove != NULL;
    if (!basemove)
        basemove = &NullMove;

    if (alwaysReadAll || stream->readFlag())
    {
        pyaw = stream->readFlag() ? stream->readInt(16) : basemove->pyaw;
        ppitch = stream->readFlag() ? stream->readInt(16) : basemove->ppitch;
        proll = stream->readFlag() ? stream->readInt(16) : basemove->proll;

        px = stream->readFlag() ? stream->readInt(6) : basemove->px;
        py = stream->readFlag() ? stream->readInt(6) : basemove->py;
        pz = stream->readFlag() ? stream->readInt(6) : basemove->pz;
        freeLook = stream->readFlag();
        deviceIsKeyboardMouse = stream->readFlag();

        bool triggersDiffer = stream->readFlag();
        for (S32 i = 0; i < MaxTriggerKeys; i++)
            trigger[i] = triggersDiffer ? stream->readFlag() : basemove->trigger[i];
        unclamp();
    }
    else
        *this = *basemove;
}
