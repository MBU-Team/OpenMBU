//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _WINDINPUTDEVICE_H_
#define _WINDINPUTDEVICE_H_

#ifndef _PLATFORMWIN32_H_
#include "platformWin32/platformWin32.h"
#endif
#ifndef _PLATFORMINPUT_H_
#include "platform/platformInput.h"
#endif
#ifndef _EVENT_H_
#include "platform/event.h"
#endif

#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>


class DInputDevice : public InputDevice
{
public:
    static LPDIRECTINPUT8 smDInputInterface;

protected:
    enum Constants
    {
        QUEUED_BUFFER_SIZE = 128,

        SIZEOF_BUTTON = 1,                  // size of an object's data in bytes
        SIZEOF_KEY = 1,
        SIZEOF_AXIS = 4,
        SIZEOF_POV = 4,
    };

    static U8   smKeyboardCount;
    static U8   smMouseCount;
    static U8   smJoystickCount;
    static U8   smUnknownCount;

    static U8   smModifierKeys;
    static bool smKeyStates[256];
    static bool smInitialized;

    //--------------------------------------
    LPDIRECTINPUTDEVICE8 mDevice;
    DIDEVICEINSTANCE     mDeviceInstance;
    DIDEVCAPS            mDeviceCaps;
    U8                   mDeviceType;
    U8                   mDeviceID;

    bool                 mAcquired;
    bool                 mNeedSync;

    //- Force Feedback ----------- jason_cahill -
    LPDIRECTINPUTEFFECT  mForceFeedbackEffect;         // Holds our DirectInput FF Effect
    DWORD                mNumForceFeedbackAxes;        // # axes (we only support 0, 1, or 2
    DWORD                mForceFeedbackAxes[2];        // Force Feedback axes offsets into DIOBJECTFORMAT

    //--------------------------------------
    DIDEVICEOBJECTINSTANCE* mObjInstance;
    DIOBJECTDATAFORMAT* mObjFormat;
    ObjInfo* mObjInfo;
    U8* mObjBuffer1;    // polled device input buffers
    U8* mObjBuffer2;
    U8* mPrevObjBuffer; // points to buffer 1 or 2

    U32 mObjBufferSize;                     // size of objBuffer*
    U32 mObjCount;                          // number of objects on this device
    U32 mObjEnumCount;                      // used during enumeration ONLY
    U32 mObjBufferOfs;                      // used during enumeration ONLY

    static BOOL CALLBACK EnumObjectsProc(const DIDEVICEOBJECTINSTANCE* doi, LPVOID pvRef);

    bool enumerateObjects();
    bool processAsync();
    bool processImmediate();
    bool processKeyEvent(InputEvent& event);

    void syncKeyboardState();

    DWORD findObjInstance(DWORD offset);
    bool  buildEvent(DWORD offset, S32 newData, S32 oldData);

public:
    DInputDevice(const DIDEVICEINSTANCE* deviceInst);
    ~DInputDevice();

    static void init();
    static void resetModifierKeys();

    bool create();
    void destroy();

    bool acquire();
    bool unacquire();

    bool isAcquired();
    bool isPolled();

    U8 getDeviceType();
    U8 getDeviceID();

    const char* getName();
    const char* getProductName();

    // Constant Effect Force Feedback --------- jason_cahill -
    void rumble(float x, float y);

    // Console interface functions:
    const char* getJoystickAxesString();
    static bool joystickDetected();
    //

    bool process();
};

//------------------------------------------------------------------------------
inline bool DInputDevice::isAcquired()
{
    return mAcquired;
}

//------------------------------------------------------------------------------
inline bool DInputDevice::isPolled()
{
    //return true;
    return (mDeviceCaps.dwFlags & DIDC_POLLEDDEVICE);
}

//------------------------------------------------------------------------------
inline U8 DInputDevice::getDeviceType()
{
    return mDeviceType;
}

//------------------------------------------------------------------------------
inline U8 DInputDevice::getDeviceID()
{
    return mDeviceID;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
U16 DIK_to_Key(U8 dikCode);
U8  Key_to_DIK(U16 keyCode);
#endif // _H_WINDINPUTDEVICE_
