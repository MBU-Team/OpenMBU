//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "math/mConstants.h"
#include "math/mMathFn.h"
#include "platformWin32/platformWin32.h"
#include "platform/platformVideo.h"
#include "platformWin32/winDirectInput.h"
#include "platformWin32/winDInputDevice.h"
#include "platform/event.h"
#include "console/console.h"
#include "console/consoleTypes.h"
#include "sim/actionMap.h"

#include <xinput.h>

//------------------------------------------------------------------------------
// Static class variables:
bool DInputManager::smKeyboardEnabled = true;
bool DInputManager::smMouseEnabled = false;
bool DInputManager::smJoystickEnabled = false;
bool DInputManager::smXInputEnabled = false;
int DInputManager::smAnalogRange = 0;
F32 DInputManager::smDeadZoneL = XINPUT_DEADZONE_DEFAULT;
F32 DInputManager::smDeadZoneR = XINPUT_DEADZONE_DEFAULT;
F32 DInputManager::smDeadZoneLT = 0;
F32 DInputManager::smDeadZoneRT = 0;

// Type definitions:
typedef HRESULT(WINAPI* FN_DirectInputCreate)(HINSTANCE hinst, DWORD dwVersion, REFIID riidltf, LPVOID* ppvOut, LPUNKNOWN punkOuter);

//------------------------------------------------------------------------------
DInputManager::DInputManager()
{
    mEnabled = false;
    mDInputLib = NULL;
    mDInputInterface = NULL;
    mKeyboardActive = mMouseActive = mJoystickActive = mXInputActive = false;
    mXInputLib = NULL;
}

//------------------------------------------------------------------------------
void DInputManager::init()
{
    Con::addVariable("pref::Input::KeyboardEnabled", TypeBool, &smKeyboardEnabled);
    Con::addVariable("pref::Input::MouseEnabled", TypeBool, &smMouseEnabled);
    Con::addVariable("pref::Input::JoystickEnabled", TypeBool, &smJoystickEnabled);
    Con::addVariable("pref::Input::AnalogRange", TypeS32, &smAnalogRange);
    Con::addVariable("pref::Input::DeadZoneL", TypeF32, &smDeadZoneL);
    Con::addVariable("pref::Input::DeadZoneR", TypeF32, &smDeadZoneR);
    Con::addVariable("pref::Input::DeadZoneLT", TypeF32, &smDeadZoneLT);
    Con::addVariable("pref::Input::DeadZoneRT", TypeF32, &smDeadZoneRT);
}

//------------------------------------------------------------------------------
bool DInputManager::enable()
{
    FN_DirectInputCreate fnDInputCreate;

    disable();

    // Dynamically load the XInput 9 DLL and cache function pointers to the two APIs we use -- jason_cahill
#ifdef LOG_INPUT
    Input::log("Enabling XInput...\n");
#endif
    mXInputLib = LoadLibrary(dT("xinput9_1_0.dll"));
    if (mXInputLib)
    {
        mfnXInputGetState = (FN_XInputGetState)GetProcAddress(mXInputLib, "XInputGetState");
        mfnXInputSetState = (FN_XInputSetState)GetProcAddress(mXInputLib, "XInputSetState");
        if (mfnXInputGetState && mfnXInputSetState)
        {
#ifdef LOG_INPUT
            Input::log("XInput detected.\n");
#endif
            mXInputStateReset = true;
            mXInputDeadZoneOn = true;
            smXInputEnabled = true;
        }
    }
    else
    {
#ifdef LOG_INPUT
        Input::log("XInput was not found.\n");
        mXInputStateReset = false;
        mXInputDeadZoneOn = false;
#endif
    }

#ifdef LOG_INPUT
    Input::log("Enabling DirectInput...\n");
#endif
    mDInputLib = LoadLibrary(dT("DInput8.dll"));
    if (mDInputLib)
    {
        fnDInputCreate = (FN_DirectInputCreate)GetProcAddress(mDInputLib, "DirectInput8Create");
        if (fnDInputCreate)
        {
            bool result = SUCCEEDED(fnDInputCreate(winState.appInstance, DIRECTINPUT_VERSION, IID_IDirectInput8, reinterpret_cast<void**>(&mDInputInterface), NULL));
            if (result)
            {
#ifdef LOG_INPUT
                Input::log("DirectX 8 or greater detected.\n");
#endif
            }

            if (result)
            {
                enumerateDevices();
                mEnabled = true;
                return true;
            }
        }
    }

    disable();

#ifdef LOG_INPUT
    Input::log("Failed to enable DirectInput.\n");
#endif

    return false;
}

//------------------------------------------------------------------------------
void DInputManager::disable()
{
    unacquire(SI_ANY, SI_ANY);

    DInputDevice* dptr;
    iterator ptr = begin();
    while (ptr != end())
    {
        dptr = dynamic_cast<DInputDevice*>(*ptr);
        if (dptr)
        {
            removeObject(dptr);
            //if ( dptr->getManager() )
               //dptr->getManager()->unregisterObject( dptr );
            delete dptr;
            ptr = begin();
        }
        else
            ptr++;
    }

    if (mDInputInterface)
    {
        mDInputInterface->Release();
        mDInputInterface = NULL;
    }

    if (mDInputLib)
    {
        FreeLibrary(mDInputLib);
        mDInputLib = NULL;
    }

    if (mfnXInputGetState)
    {
        mXInputStateReset = true;
        mfnXInputGetState = NULL;
        mfnXInputSetState = NULL;
    }

    if (mXInputLib)
    {
        FreeLibrary(mXInputLib);
        mXInputLib = NULL;
    }

    mEnabled = false;
}

//------------------------------------------------------------------------------
void DInputManager::onDeleteNotify(SimObject* object)
{
    Parent::onDeleteNotify(object);
}

//------------------------------------------------------------------------------
bool DInputManager::onAdd()
{
    if (!Parent::onAdd())
        return false;

    acquire(SI_ANY, SI_ANY);
    return true;
}

//------------------------------------------------------------------------------
void DInputManager::onRemove()
{
    unacquire(SI_ANY, SI_ANY);
    Parent::onRemove();
}

//------------------------------------------------------------------------------
bool DInputManager::acquire(U8 deviceType, U8 deviceID)
{
    if (deviceType == XInputDeviceType) return true; // If XInput is active, then controllers are automatically acquired -- jason_cahill

    bool anyActive = false;
    DInputDevice* dptr;
    for (iterator ptr = begin(); ptr != end(); ptr++)
    {
        dptr = dynamic_cast<DInputDevice*>(*ptr);
        if (dptr
            && ((deviceType == SI_ANY) || (dptr->getDeviceType() == deviceType))
            && ((deviceID == SI_ANY) || (dptr->getDeviceID() == deviceID)))
        {
            if (dptr->acquire())
                anyActive = true;
        }
    }

    return anyActive;
}

//------------------------------------------------------------------------------
void DInputManager::unacquire(U8 deviceType, U8 deviceID)
{
    if (deviceType == XInputDeviceType) return; // If XInput is active, then controllers are automatically unacquired -- jason_cahill

    DInputDevice* dptr;
    for (iterator ptr = begin(); ptr != end(); ptr++)
    {
        dptr = dynamic_cast<DInputDevice*>(*ptr);
        if (dptr
            && ((deviceType == SI_ANY) || (dptr->getDeviceType() == deviceType))
            && ((deviceID == SI_ANY) || (dptr->getDeviceID() == deviceID)))
            dptr->unacquire();
    }
}

//------------------------------------------------------------------------------
void DInputManager::process()
{
    // Because the XInput APIs manage everything for all four controllers trivially,
    // we don't need the abstraction of a DInputDevice for an unknown number of devices
    // with varying buttons & whistles... nice and easy! -- jason_cahill
    if (isXInputActive())
        processXInput();

    DInputDevice* dptr;
    for (iterator ptr = begin(); ptr != end(); ptr++)
    {
        dptr = dynamic_cast<DInputDevice*>(*ptr);
        if (dptr)
            dptr->process();
    }
}

//------------------------------------------------------------------------------
void DInputManager::enumerateDevices()
{
    if (mDInputInterface)
    {
#ifdef LOG_INPUT
        Input::log("Enumerating input devices...\n");
#endif

        DInputDevice::init();
        DInputDevice::smDInputInterface = mDInputInterface;
        mDInputInterface->EnumDevices(DI8DEVTYPE_KEYBOARD, EnumDevicesProc, this, DIEDFL_ATTACHEDONLY);
        mDInputInterface->EnumDevices(DI8DEVTYPE_MOUSE, EnumDevicesProc, this, DIEDFL_ATTACHEDONLY);
        mDInputInterface->EnumDevices(DI8DEVCLASS_GAMECTRL, EnumDevicesProc, this, DIEDFL_ATTACHEDONLY);
    }
}

//------------------------------------------------------------------------------
BOOL CALLBACK DInputManager::EnumDevicesProc(const DIDEVICEINSTANCE* pddi, LPVOID pvRef)
{
    DInputManager* manager = (DInputManager*)pvRef;
    DInputDevice* newDevice = new DInputDevice(pddi);
    manager->addObject(newDevice);
    if (!newDevice->create())
    {
        manager->removeObject(newDevice);
        delete newDevice;
    }

    return (DIENUM_CONTINUE);
}

//------------------------------------------------------------------------------
bool DInputManager::enableKeyboard()
{
    DInputManager* mgr = dynamic_cast<DInputManager*>(Input::getManager());
    if (!mgr || !mgr->isEnabled())
        return(false);

    if (smKeyboardEnabled && mgr->isKeyboardActive())
        return(true);

    smKeyboardEnabled = true;
    if (Input::isActive())
        smKeyboardEnabled = mgr->activateKeyboard();

    if (smKeyboardEnabled)
    {
        Con::printf("DirectInput keyboard enabled.");
#ifdef LOG_INPUT
        Input::log("Keyboard enabled.\n");
#endif
    }
    else
    {
        Con::warnf("DirectInput keyboard failed to enable!");
#ifdef LOG_INPUT
        Input::log("Keyboard failed to enable!\n");
#endif
    }

    return(smKeyboardEnabled);
}

//------------------------------------------------------------------------------
void DInputManager::disableKeyboard()
{
    DInputManager* mgr = dynamic_cast<DInputManager*>(Input::getManager());
    if (!mgr || !mgr->isEnabled() || !smKeyboardEnabled)
        return;

    mgr->deactivateKeyboard();
    smKeyboardEnabled = false;
    Con::printf("DirectInput keyboard disabled.");
#ifdef LOG_INPUT
    Input::log("Keyboard disabled.\n");
#endif
}

//------------------------------------------------------------------------------
bool DInputManager::isKeyboardEnabled()
{
    return(smKeyboardEnabled);
}

//------------------------------------------------------------------------------
bool DInputManager::activateKeyboard()
{
    if (!mEnabled || !Input::isActive() || !smKeyboardEnabled)
        return(false);

    // Acquire only one keyboard:
    mKeyboardActive = acquire(KeyboardDeviceType, 0);
#ifdef LOG_INPUT
    Input::log(mKeyboardActive ? "Keyboard activated.\n" : "Keyboard failed to activate!\n");
#endif
    return(mKeyboardActive);
}

//------------------------------------------------------------------------------
void DInputManager::deactivateKeyboard()
{
    if (mEnabled && mKeyboardActive)
    {
        unacquire(KeyboardDeviceType, SI_ANY);
        mKeyboardActive = false;
#ifdef LOG_INPUT
        Input::log("Keyboard deactivated.\n");
#endif
    }
}

//------------------------------------------------------------------------------
bool DInputManager::enableMouse()
{
    DInputManager* mgr = dynamic_cast<DInputManager*>(Input::getManager());
    if (!mgr || !mgr->isEnabled())
        return(false);

    if (smMouseEnabled && mgr->isMouseActive())
        return(true);

    smMouseEnabled = true;
    if (Input::isActive())
        smMouseEnabled = mgr->activateMouse();

    if (smMouseEnabled)
    {
        Con::printf("DirectInput mouse enabled.");
#ifdef LOG_INPUT
        Input::log("Mouse enabled.\n");
#endif
    }
    else
    {
        Con::warnf("DirectInput mouse failed to enable!");
#ifdef LOG_INPUT
        Input::log("Mouse failed to enable!\n");
#endif
    }

    return(smMouseEnabled);
}

//------------------------------------------------------------------------------
void DInputManager::disableMouse()
{
    DInputManager* mgr = dynamic_cast<DInputManager*>(Input::getManager());
    if (!mgr || !mgr->isEnabled() || !smMouseEnabled)
        return;

    mgr->deactivateMouse();
    smMouseEnabled = false;
    Con::printf("DirectInput mouse disabled.");
#ifdef LOG_INPUT
    Input::log("Mouse disabled.\n");
#endif
}

//------------------------------------------------------------------------------
bool DInputManager::isMouseEnabled()
{
    return(smMouseEnabled);
}

//------------------------------------------------------------------------------
bool DInputManager::activateMouse()
{
    if (!mEnabled || !Input::isActive() || !smMouseEnabled)
        return(false);

    mMouseActive = acquire(MouseDeviceType, SI_ANY);
#ifdef LOG_INPUT
    Input::log(mMouseActive ? "Mouse activated.\n" : "Mouse failed to activate!\n");
#endif
    return(mMouseActive);
}

//------------------------------------------------------------------------------
void DInputManager::deactivateMouse()
{
    if (mEnabled && mMouseActive)
    {
        unacquire(MouseDeviceType, SI_ANY);
        mMouseActive = false;
#ifdef LOG_INPUT
        Input::log("Mouse deactivated.\n");
#endif
    }
}

//------------------------------------------------------------------------------
bool DInputManager::enableJoystick()
{
    DInputManager* mgr = dynamic_cast<DInputManager*>(Input::getManager());
    if (!mgr || !mgr->isEnabled())
        return(false);

    if (smJoystickEnabled && mgr->isJoystickActive())
        return(true);

    smJoystickEnabled = true;
    if (Input::isActive())
        smJoystickEnabled = mgr->activateJoystick();

    if (smJoystickEnabled)
    {
        Con::printf("DirectInput joystick enabled.");
#ifdef LOG_INPUT
        Input::log("Joystick enabled.\n");
#endif
    }
    else
    {
        Con::warnf("DirectInput joystick failed to enable!");
#ifdef LOG_INPUT
        Input::log("Joystick failed to enable!\n");
#endif
    }

    return(smJoystickEnabled);
}

//------------------------------------------------------------------------------
void DInputManager::disableJoystick()
{
    DInputManager* mgr = dynamic_cast<DInputManager*>(Input::getManager());
    if (!mgr || !mgr->isEnabled() || !smJoystickEnabled)
        return;

    mgr->deactivateJoystick();
    smJoystickEnabled = false;
    Con::printf("DirectInput joystick disabled.");
#ifdef LOG_INPUT
    Input::log("Joystick disabled.\n");
#endif
}

//------------------------------------------------------------------------------
bool DInputManager::isJoystickEnabled()
{
    return(smJoystickEnabled);
}

//------------------------------------------------------------------------------
bool DInputManager::activateJoystick()
{
    if (!mEnabled || !Input::isActive() || !smJoystickEnabled)
        return(false);

    mJoystickActive = acquire(JoystickDeviceType, SI_ANY);
#ifdef LOG_INPUT
    Input::log(mJoystickActive ? "Joystick activated.\n" : "Joystick failed to activate!\n");
#endif
    return(mJoystickActive);
}

//------------------------------------------------------------------------------
void DInputManager::deactivateJoystick()
{
    if (mEnabled && mJoystickActive)
    {
        unacquire(JoystickDeviceType, SI_ANY);
        mJoystickActive = false;
#ifdef LOG_INPUT
        Input::log("Joystick deactivated.\n");
#endif
    }
}

//------------------------------------------------------------------------------
const char* DInputManager::getJoystickAxesString(U32 deviceID)
{
    DInputDevice* dptr;
    for (iterator ptr = begin(); ptr != end(); ptr++)
    {
        dptr = dynamic_cast<DInputDevice*>(*ptr);
        if (dptr && (dptr->getDeviceType() == JoystickDeviceType) && (dptr->getDeviceID() == deviceID))
            return(dptr->getJoystickAxesString());
    }

    return("");
}

//------------------------------------------------------------------------------
// Largely, this series of functions is identical to the Joystick versions,
// except that XInput cannot be "activated" or "deactivated". You either have
// the DLL or you don't. Beyond that, you have up to four controllers
// connected at any given time -- jason_cahill
bool DInputManager::enableXInput()
{
    DInputManager* mgr = dynamic_cast<DInputManager*>(Input::getManager());
    if (!mgr || !mgr->isEnabled())
        return(false);

    if (mgr->isXInputActive()) return(true);

    mgr->activateXInput();

    if (smXInputEnabled)
    {
        Con::printf("XInput enabled.");
#ifdef LOG_INPUT
        Input::log("XInput enabled.\n");
#endif
    }
    else
    {
        Con::warnf("XInput failed to enable!");
#ifdef LOG_INPUT
        Input::log("XInput failed to enable!\n");
#endif
    }

    return(smXInputEnabled);
}

//------------------------------------------------------------------------------
void DInputManager::disableXInput()
{
    DInputManager* mgr = dynamic_cast<DInputManager*>(Input::getManager());
    if (!mgr || !mgr->isEnabled())
        return;

    mgr->deactivateXInput();
    Con::printf("XInput disabled.");
#ifdef LOG_INPUT
    Input::log("XInput disabled.\n");
#endif
}

//------------------------------------------------------------------------------
bool DInputManager::isXInputEnabled()
{
    return(smXInputEnabled);
}

//------------------------------------------------------------------------------
bool DInputManager::isXInputConnected(int controllerID)
{
    return(mXInputStateNew[controllerID].bConnected);
}

int DInputManager::getXInputState(int controllerID, int property, bool current)
{
    int retVal;

    switch (property)
    {
    case XI_THUMBLX:        (current) ? retVal = mXInputStateNew[controllerID].state.Gamepad.sThumbLX : retVal = mXInputStateOld[controllerID].state.Gamepad.sThumbLX; return retVal;
    case XI_THUMBLY:        (current) ? retVal = mXInputStateNew[controllerID].state.Gamepad.sThumbLY : retVal = mXInputStateOld[controllerID].state.Gamepad.sThumbLY; return retVal;
    case XI_THUMBRX:        (current) ? retVal = mXInputStateNew[controllerID].state.Gamepad.sThumbRX : retVal = mXInputStateOld[controllerID].state.Gamepad.sThumbRX; return retVal;
    case XI_THUMBRY:        (current) ? retVal = mXInputStateNew[controllerID].state.Gamepad.sThumbRY : retVal = mXInputStateOld[controllerID].state.Gamepad.sThumbRY; return retVal;
    case XI_LEFT_TRIGGER:   (current) ? retVal = mXInputStateNew[controllerID].state.Gamepad.bLeftTrigger : retVal = mXInputStateOld[controllerID].state.Gamepad.bLeftTrigger; return retVal;
    case XI_RIGHT_TRIGGER:  (current) ? retVal = mXInputStateNew[controllerID].state.Gamepad.bRightTrigger : retVal = mXInputStateOld[controllerID].state.Gamepad.bRightTrigger; return retVal;
    case XI_DPAD_UP:        (current) ? retVal = ((mXInputStateNew[controllerID].state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_UP) != 0) : retVal = ((mXInputStateOld[controllerID].state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_UP) != 0); return retVal;
    case XI_DPAD_DOWN:      (current) ? retVal = ((mXInputStateNew[controllerID].state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_DOWN) != 0) : retVal = ((mXInputStateOld[controllerID].state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_DOWN) != 0); return retVal;
    case XI_DPAD_LEFT:      (current) ? retVal = ((mXInputStateNew[controllerID].state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_LEFT) != 0) : retVal = ((mXInputStateOld[controllerID].state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_LEFT) != 0); return retVal;
    case XI_DPAD_RIGHT:     (current) ? retVal = ((mXInputStateNew[controllerID].state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT) != 0) : retVal = ((mXInputStateOld[controllerID].state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT) != 0); return retVal;
    case XI_START:          (current) ? retVal = ((mXInputStateNew[controllerID].state.Gamepad.wButtons & XINPUT_GAMEPAD_START) != 0) : retVal = ((mXInputStateOld[controllerID].state.Gamepad.wButtons & XINPUT_GAMEPAD_START) != 0); return retVal;
    case XI_BACK:           (current) ? retVal = ((mXInputStateNew[controllerID].state.Gamepad.wButtons & XINPUT_GAMEPAD_BACK) != 0) : retVal = ((mXInputStateOld[controllerID].state.Gamepad.wButtons & XINPUT_GAMEPAD_BACK) != 0); return retVal;
    case XI_LEFT_THUMB:     (current) ? retVal = ((mXInputStateNew[controllerID].state.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_THUMB) != 0) : retVal = ((mXInputStateOld[controllerID].state.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_THUMB) != 0); return retVal;
    case XI_RIGHT_THUMB:    (current) ? retVal = ((mXInputStateNew[controllerID].state.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_THUMB) != 0) : retVal = ((mXInputStateOld[controllerID].state.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_THUMB) != 0); return retVal;
    case XI_LEFT_SHOULDER:  (current) ? retVal = ((mXInputStateNew[controllerID].state.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER) != 0) : retVal = ((mXInputStateOld[controllerID].state.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER) != 0); return retVal;
    case XI_RIGHT_SHOULDER: (current) ? retVal = ((mXInputStateNew[controllerID].state.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER) != 0) : retVal = ((mXInputStateOld[controllerID].state.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER) != 0); return retVal;
    case XI_A:              (current) ? retVal = ((mXInputStateNew[controllerID].state.Gamepad.wButtons & XINPUT_GAMEPAD_A) != 0) : retVal = ((mXInputStateOld[controllerID].state.Gamepad.wButtons & XINPUT_GAMEPAD_A) != 0); return retVal;
    case XI_B:              (current) ? retVal = ((mXInputStateNew[controllerID].state.Gamepad.wButtons & XINPUT_GAMEPAD_B) != 0) : retVal = ((mXInputStateOld[controllerID].state.Gamepad.wButtons & XINPUT_GAMEPAD_B) != 0); return retVal;
    case XI_X:              (current) ? retVal = ((mXInputStateNew[controllerID].state.Gamepad.wButtons & XINPUT_GAMEPAD_X) != 0) : retVal = ((mXInputStateOld[controllerID].state.Gamepad.wButtons & XINPUT_GAMEPAD_X) != 0); return retVal;
    case XI_Y:              (current) ? retVal = ((mXInputStateNew[controllerID].state.Gamepad.wButtons & XINPUT_GAMEPAD_Y) != 0) : retVal = ((mXInputStateOld[controllerID].state.Gamepad.wButtons & XINPUT_GAMEPAD_Y) != 0); return retVal;
    }

    return -1;
}

//------------------------------------------------------------------------------
bool DInputManager::activateXInput()
{
    if (!mEnabled)
        return(false);

    mXInputActive = acquire(XInputDeviceType, SI_ANY);
#ifdef LOG_INPUT
    Input::log(mXInputActive ? "XInput activated.\n" : "XInput failed to activate!\n");
#endif
    return(mXInputActive);
}

//------------------------------------------------------------------------------
void DInputManager::deactivateXInput()
{
    if (mEnabled && mXInputActive)
    {
        unacquire(XInputDeviceType, SI_ANY);
        mXInputActive = false;
#ifdef LOG_INPUT
        Input::log("XInput deactivated.\n");
#endif
    }
}

//------------------------------------------------------------------------------
ConsoleFunction(activateKeyboard, bool, 1, 1, "activateKeyboard()")
{
    argc; argv;
    DInputManager* mgr = dynamic_cast<DInputManager*>(Input::getManager());
    if (mgr)
        return(mgr->activateKeyboard());

    return(false);
}

//------------------------------------------------------------------------------
ConsoleFunction(deactivateKeyboard, void, 1, 1, "deactivateKeyboard()")
{
    argc; argv;
    DInputManager* mgr = dynamic_cast<DInputManager*>(Input::getManager());
    if (mgr)
        mgr->deactivateKeyboard();
}

//------------------------------------------------------------------------------
ConsoleFunction(enableMouse, bool, 1, 1, "enableMouse()")
{
    argc; argv;
    return(DInputManager::enableMouse());
}

//------------------------------------------------------------------------------
ConsoleFunction(disableMouse, void, 1, 1, "disableMouse()")
{
    argc; argv;
    DInputManager::disableMouse();
}

//------------------------------------------------------------------------------
ConsoleFunction(enableJoystick, bool, 1, 1, "enableJoystick()")
{
    argc; argv;
    return(DInputManager::enableJoystick());
}

//------------------------------------------------------------------------------
ConsoleFunction(disableJoystick, void, 1, 1, "disableJoystick()")
{
    argc; argv;
    DInputManager::disableJoystick();
}

//------------------------------------------------------------------------------
ConsoleFunction(isJoystickEnabled, bool, 1, 1, "()")
{
    argc; argv;
    return DInputManager::isJoystickEnabled();
}

//------------------------------------------------------------------------------
// Although I said above that you couldn't change the "activation" of XInput,
// you can enable and disable it. It gets enabled by default if you have the
// DLL. You would want to disable it if you have 360 controllers and want to
// read them as joysticks... why you'd want to do that is beyond me -- jason_cahill
ConsoleFunction(enableXInput, bool, 1, 1, "enableXInput()")
{
    argc; argv;
    return(DInputManager::enableXInput());
}

ConsoleFunction(enableGamepad, bool, 1, 1, "enableGamepad()")
{
    argc; argv;
    return(DInputManager::enableXInput());
}

//------------------------------------------------------------------------------
ConsoleFunction(disableXInput, void, 1, 1, "disableXInput()")
{
    argc; argv;
    DInputManager::disableXInput();
}

ConsoleFunction(getGamepadName, const char*, 1, 1, "getGamepadName()")
{
    argc; argv;
    return "XInput";
}

//------------------------------------------------------------------------------
// This function requests a full "refresh" of events for all controllers the
// next time we go through the input processing loop. This is useful to call
// at the beginning of your game code after your actionMap is set up to hook
// all of the appropriate events -- jason_cahill
ConsoleFunction(resetXInput, void, 1, 1, "resetXInput()")
{
    argc; argv;
    DInputManager* mgr = dynamic_cast<DInputManager*>(Input::getManager());
    if (mgr && mgr->isEnabled()) mgr->resetXInput();
}

//------------------------------------------------------------------------------
ConsoleFunction(isXInputConnected, bool, 2, 2, "( int controllerID )")
{
    argc; argv;
    DInputManager* mgr = dynamic_cast<DInputManager*>(Input::getManager());
    if (mgr && mgr->isEnabled()) return mgr->isXInputConnected(atoi(argv[1]));
    return false;
}

//------------------------------------------------------------------------------
ConsoleFunction(getXInputState, int, 3, 4, "( int controllerID, string property, bool current )")
{
    argc; argv;
    DInputManager* mgr = dynamic_cast<DInputManager*>(Input::getManager());

    if (!mgr || !mgr->isEnabled()) return -1;
    if (!(stricmp(argv[2], "XI_THUMBLX"))) return mgr->getXInputState(atoi(argv[1]), XI_THUMBLX, (atoi(argv[3]) == 1));
    if (!(stricmp(argv[2], "XI_THUMBLY"))) return mgr->getXInputState(atoi(argv[1]), XI_THUMBLY, (atoi(argv[3]) == 1));
    if (!(stricmp(argv[2], "XI_THUMBRX"))) return mgr->getXInputState(atoi(argv[1]), XI_THUMBRX, (atoi(argv[3]) == 1));
    if (!(stricmp(argv[2], "XI_THUMBRY"))) return mgr->getXInputState(atoi(argv[1]), XI_THUMBRY, (atoi(argv[3]) == 1));
    if (!(stricmp(argv[2], "XI_LEFT_TRIGGER"))) return mgr->getXInputState(atoi(argv[1]), XI_LEFT_TRIGGER, (atoi(argv[3]) == 1));
    if (!(stricmp(argv[2], "XI_RIGHT_TRIGGER"))) return mgr->getXInputState(atoi(argv[1]), XI_RIGHT_TRIGGER, (atoi(argv[3]) == 1));
    if (!(stricmp(argv[2], "XI_DPAD_UP"))) return mgr->getXInputState(atoi(argv[1]), XI_DPAD_UP, (atoi(argv[3]) == 1));
    if (!(stricmp(argv[2], "XI_DPAD_DOWN"))) return mgr->getXInputState(atoi(argv[1]), XI_DPAD_DOWN, (atoi(argv[3]) == 1));
    if (!(stricmp(argv[2], "XI_DPAD_LEFT"))) return mgr->getXInputState(atoi(argv[1]), XI_DPAD_LEFT, (atoi(argv[3]) == 1));
    if (!(stricmp(argv[2], "XI_DPAD_RIGHT"))) return mgr->getXInputState(atoi(argv[1]), XI_DPAD_RIGHT, (atoi(argv[3]) == 1));
    if (!(stricmp(argv[2], "XI_START"))) return mgr->getXInputState(atoi(argv[1]), XI_START, (atoi(argv[3]) == 1));
    if (!(stricmp(argv[2], "XI_BACK"))) return mgr->getXInputState(atoi(argv[1]), XI_BACK, (atoi(argv[3]) == 1));
    if (!(stricmp(argv[2], "XI_LEFT_THUMB"))) return mgr->getXInputState(atoi(argv[1]), XI_LEFT_THUMB, (atoi(argv[3]) == 1));
    if (!(stricmp(argv[2], "XI_RIGHT_THUMB"))) return mgr->getXInputState(atoi(argv[1]), XI_RIGHT_THUMB, (atoi(argv[3]) == 1));
    if (!(stricmp(argv[2], "XI_LEFT_SHOULDER"))) return mgr->getXInputState(atoi(argv[1]), XI_LEFT_SHOULDER, (atoi(argv[3]) == 1));
    if (!(stricmp(argv[2], "XI_RIGHT_SHOULDER"))) return mgr->getXInputState(atoi(argv[1]), XI_RIGHT_SHOULDER, (atoi(argv[3]) == 1));
    if (!(stricmp(argv[2], "XI_A"))) return mgr->getXInputState(atoi(argv[1]), XI_A, (atoi(argv[3]) == 1));
    if (!(stricmp(argv[2], "XI_B"))) return mgr->getXInputState(atoi(argv[1]), XI_B, (atoi(argv[3]) == 1));
    if (!(stricmp(argv[2], "XI_X"))) return mgr->getXInputState(atoi(argv[1]), XI_X, (atoi(argv[3]) == 1));
    if (!(stricmp(argv[2], "XI_Y"))) return mgr->getXInputState(atoi(argv[1]), XI_Y, (atoi(argv[3]) == 1));

    return -1;
}

//------------------------------------------------------------------------------
ConsoleFunction(echoInputState, void, 1, 1, "echoInputState()")
{
    argc; argv;
    DInputManager* mgr = dynamic_cast<DInputManager*>(Input::getManager());
    if (mgr && mgr->isEnabled())
    {
        Con::printf("DirectInput is enabled %s.", Input::isActive() ? "and active" : "but inactive");
        Con::printf("- Keyboard is %sabled and %sactive.",
            mgr->isKeyboardEnabled() ? "en" : "dis",
            mgr->isKeyboardActive() ? "" : "in");
        Con::printf("- Mouse is %sabled and %sactive.",
            mgr->isMouseEnabled() ? "en" : "dis",
            mgr->isMouseActive() ? "" : "in");
        Con::printf("- Joystick is %sabled and %sactive.",
            mgr->isJoystickEnabled() ? "en" : "dis",
            mgr->isJoystickActive() ? "" : "in");
    }
    else
        Con::printf("DirectInput is not enabled.");
}

//- Console Function: rumble --------------------------------- jason_cahill -
ConsoleFunction(rumble, void, 4, 4, "(string device, float xRumble, float yRumble) Rumbles the specified controller with constant force feedback in the (x, y) direction.\nValid inputs for xRumble/yRumble are [0 - 1].")
{
    DInputManager* mgr = dynamic_cast<DInputManager*>(Input::getManager());
    if (mgr && mgr->isEnabled())
    {
        mgr->rumble(argv[1], dAtof(argv[2]), dAtof(argv[3]));
    }
    else
    {
        Con::printf("DirectInput/XInput is not enabled.");
    }
}

//- This function starts the rumble for joysticks and xinput devices ---------- jason_cahill -
bool DInputManager::rumble(const char* pDeviceName, float x, float y)
{
    // Determine the device
    U32 deviceType;
    U32 deviceInst;

    // Find the requested device
    if (!ActionMap::getDeviceTypeAndInstance(pDeviceName, deviceType, deviceInst))
    {
        Con::printf("DInputManager::rumble: unknown device: %s", pDeviceName);
        return false;
    }

    // clamp (x, y) to the range of [0 ... 1] each
    x = getMax(0.f, getMin(1.f, x));
    y = getMax(0.f, getMin(1.f, y));

    switch (deviceType)
    {
    case JoystickDeviceType:
        // Find the device and shake it!
        DInputDevice* dptr;
        for (iterator ptr = begin(); ptr != end(); ptr++)
        {
            dptr = dynamic_cast<DInputDevice*>(*ptr);
            if (dptr)
            {
                if (deviceType == dptr->getDeviceType() && deviceInst == dptr->getDeviceID())
                {
                    dptr->rumble(x, y);
                    return true;
                }
            }
        }

        // We should never get here... something's really messed up
        Con::errorf("DInputManager::rumbleJoystick - Couldn't find device to rumble! This should never happen!\n");
        return false;

    case XInputDeviceType:
        XINPUT_VIBRATION vibration;
        vibration.wLeftMotorSpeed = static_cast<int>(x * 65535);
        vibration.wRightMotorSpeed = static_cast<int>(y * 65535);
        return (mfnXInputSetState(deviceInst, &vibration) == ERROR_SUCCESS);

    default:
        Con::printf("DInputManager::rumble - only supports joystick and xinput devices");
        return false;
    }
}

// A workhorse to build an XInput event and post it -- jason_cahill
inline void DInputManager::buildXInputEvent(U32 deviceInst, U16 objType, U16 objInst, U8 action, float fValue)
{
    InputEvent newEvent;

    newEvent.deviceType = XInputDeviceType;
    newEvent.deviceInst = deviceInst;
    newEvent.objType = objType;
    newEvent.action = action;
    newEvent.fValue = fValue;
    newEvent.objInst = objInst;
    Game->postEvent(newEvent);
}

// The next three functions: fireXInputConnectEvent, fireXInputMoveEvent, and fireXInputButtonEvent
// determine whether a "delta" has occurred between the last captured controller state and the
// currently captured controller state and only if so, do we fire an event. The shortcutter
// "mXInputStateReset" is the exception and is true whenever DirectInput gets reset (because 
// the user ALT-TABBED away, for example). That means that after every context switch,
// you will get a full set of updates on the "true" state of the controller. -- jason_cahill
inline void DInputManager::fireXInputConnectEvent(int controllerID, bool condition, bool connected)
{
    if (mXInputStateReset || condition)
    {
#ifdef LOG_INPUT
        Input::log("EVENT (XInput): xinput%d CONNECT %s\n", controllerID, connected ? "make" : "break");
#endif
        buildXInputEvent(controllerID, XI_CONNECT, XI_CONNECT, connected ? SI_MAKE : SI_BREAK, 0);
    }
}

inline void DInputManager::fireXInputMoveEvent(int controllerID, bool condition, int objType, int objInst, float fValue)
{
    if (mXInputStateReset || condition)
    {
#ifdef LOG_INPUT
        char* objName;
        switch (objType)
        {
        case XI_THUMBLX:       objName = "THUMBLX"; break;
        case XI_THUMBLY:       objName = "THUMBLY"; break;
        case XI_THUMBRX:       objName = "THUMBRX"; break;
        case XI_THUMBRY:       objName = "THUMBRY"; break;
        case XI_LEFT_TRIGGER:  objName = "LEFT_TRIGGER"; break;
        case XI_RIGHT_TRIGGER: objName = "RIGHT_TRIGGER"; break;
        default:               objName = "UNKNOWN"; break;
        }

        Input::log("EVENT (XInput): xinput%d %s MOVE %.1f.\n", controllerID, objName, fValue);
#endif
        buildXInputEvent(controllerID, objInst, objInst, SI_MOVE, fValue);
    }
}

inline void DInputManager::fireXInputButtonEvent(int controllerID, bool forceFire, int button, int objType, int objInst)
{
    if (mXInputStateReset || forceFire || ((mXInputStateNew[controllerID].state.Gamepad.wButtons & button) != (mXInputStateOld[controllerID].state.Gamepad.wButtons & button)))
    {
#ifdef LOG_INPUT
        char* objName;
        switch (objType)
        {
        case XI_DPAD_UP:        objName = "DPAD_UP"; break;
        case XI_DPAD_DOWN:      objName = "DPAD_DOWN"; break;
        case XI_DPAD_LEFT:      objName = "DPAD_LEFT"; break;
        case XI_DPAD_RIGHT:     objName = "DPAD_RIGHT"; break;
        case XI_START:          objName = "START"; break;
        case XI_BACK:           objName = "BACK"; break;
        case XI_LEFT_THUMB:     objName = "LEFT_THUMB"; break;
        case XI_RIGHT_THUMB:    objName = "RIGHT_THUMB"; break;
        case XI_LEFT_SHOULDER:  objName = "LEFT_SHOULDER"; break;
        case XI_RIGHT_SHOULDER: objName = "RIGHT_SHOULDER"; break;
        case XI_A:              objName = "A"; break;
        case XI_B:              objName = "B"; break;
        case XI_X:              objName = "X"; break;
        case XI_Y:              objName = "Y"; break;
        default:                objName = "UNKNOWN"; break;
        }

        Input::log("EVENT (XInput): xinput%d %s %s\n", controllerID, objName, ((mXInputStateNew[controllerID].state.Gamepad.wButtons & button) != 0) ? "make" : "break");
#endif
        int action = ((mXInputStateNew[controllerID].state.Gamepad.wButtons & button) != 0) ? SI_MAKE : SI_BREAK;
        buildXInputEvent(controllerID, objType, objInst, action, action == 1);
    }
}

// This function does all of the dirty work associated with reporting the state of all 4 XInput controllers -- jason_cahill
void DInputManager::processXInput(void)
{
    F32 deadZoneL = smDeadZoneL * FLOAT(0x7FFF);
    F32 deadZoneR = smDeadZoneR * FLOAT(0x7FFF);
    F32 deadZoneLT = smDeadZoneLT * FLOAT(0xFF);
    F32 deadZoneRT = smDeadZoneRT * FLOAT(0xFF);

    if (mfnXInputGetState)
    {
        for (int i = 0; i < 4; i++)
        {
            mXInputStateOld[i] = mXInputStateNew[i];
            mXInputStateNew[i].bConnected = (mfnXInputGetState(i, &mXInputStateNew[i].state) == ERROR_SUCCESS);

            // trim the controller's thumbsticks to zero if they are within the deadzone
            if (mXInputDeadZoneOn)
            {
                // Zero value if thumbsticks are within the dead zone 
                if ((mXInputStateNew[i].state.Gamepad.sThumbLX < deadZoneL && mXInputStateNew[i].state.Gamepad.sThumbLX > -deadZoneL) &&
                    (mXInputStateNew[i].state.Gamepad.sThumbLY < deadZoneL && mXInputStateNew[i].state.Gamepad.sThumbLY > -deadZoneL))
                {
                    mXInputStateNew[i].state.Gamepad.sThumbLX = 0;
                    mXInputStateNew[i].state.Gamepad.sThumbLY = 0;
                }

                if ((mXInputStateNew[i].state.Gamepad.sThumbRX < deadZoneR && mXInputStateNew[i].state.Gamepad.sThumbRX > -deadZoneR) &&
                    (mXInputStateNew[i].state.Gamepad.sThumbRY < deadZoneR && mXInputStateNew[i].state.Gamepad.sThumbRY > -deadZoneR))
                {
                    mXInputStateNew[i].state.Gamepad.sThumbRX = 0;
                    mXInputStateNew[i].state.Gamepad.sThumbRY = 0;
                }

                // Triggers
                if ((mXInputStateNew[i].state.Gamepad.bLeftTrigger < deadZoneLT && mXInputStateNew[i].state.Gamepad.bLeftTrigger > -deadZoneLT))
                {
                    mXInputStateNew[i].state.Gamepad.bLeftTrigger = 0;
                }

                if ((mXInputStateNew[i].state.Gamepad.bRightTrigger < deadZoneRT && mXInputStateNew[i].state.Gamepad.bRightTrigger > -deadZoneRT))
                {
                    mXInputStateNew[i].state.Gamepad.bRightTrigger = 0;
                }
            }

            // don't go beyond 32767 (if it was 32768)
            mXInputStateNew[i].state.Gamepad.sThumbLX = getMax(mXInputStateNew[i].state.Gamepad.sThumbLX, (short) -32767);
            mXInputStateNew[i].state.Gamepad.sThumbLY = getMax(mXInputStateNew[i].state.Gamepad.sThumbLY, (short) -32767);
            mXInputStateNew[i].state.Gamepad.sThumbRX = getMax(mXInputStateNew[i].state.Gamepad.sThumbRX, (short) -32767);
            mXInputStateNew[i].state.Gamepad.sThumbRY = getMax(mXInputStateNew[i].state.Gamepad.sThumbRY, (short) -32767);

            float lx, ly, ang, scale, hypot;
            switch (smAnalogRange)
            {
                case 0:
                    // Normal range
                    break;
                case 1:
                    // Expand the analog range so an unmodded controller acts as a modded one
                    lx = (mXInputStateNew[i].state.Gamepad.sThumbLX / 32767.0f);
                    ly = (mXInputStateNew[i].state.Gamepad.sThumbLY / 32767.0f);

                    scale = M_SQRT2_F;
                    lx *= scale;
                    ly *= scale;

                    lx = mClampF(lx, -1.0f, 1.0f);
                    ly = mClampF(ly, -1.0f, 1.0f);

                    mXInputStateNew[i].state.Gamepad.sThumbLX = 32767 * lx;
                    mXInputStateNew[i].state.Gamepad.sThumbLY = 32767 * ly;
                    break;
                case 2:
                    // 8-way
                    if (mXInputStateNew[i].state.Gamepad.sThumbLX || mXInputStateNew[i].state.Gamepad.sThumbLY)
                    {
                        lx = (mXInputStateNew[i].state.Gamepad.sThumbLX / 32767.0f);
                        ly = (mXInputStateNew[i].state.Gamepad.sThumbLY / 32767.0f);

                        ang = mAtan(ly, lx);

                        // Is there a nicer way to do this?
                        if      (ang < M_PI_F * (-7.0f / 8.0f)) { lx = -1.0f; ly =  0.0f; }
                        else if (ang < M_PI_F * (-5.0f / 8.0f)) { lx = -1.0f; ly = -1.0f; }
                        else if (ang < M_PI_F * (-3.0f / 8.0f)) { lx =  0.0f; ly = -1.0f; }
                        else if (ang < M_PI_F * (-1.0f / 8.0f)) { lx =  1.0f; ly = -1.0f; }
                        else if (ang < M_PI_F * ( 1.0f / 8.0f)) { lx =  1.0f; ly =  0.0f; }
                        else if (ang < M_PI_F * ( 3.0f / 8.0f)) { lx =  1.0f; ly =  1.0f; }
                        else if (ang < M_PI_F * ( 5.0f / 8.0f)) { lx =  0.0f; ly =  1.0f; }
                        else if (ang < M_PI_F * ( 7.0f / 8.0f)) { lx = -1.0f; ly =  1.0f; }
                        else    /*ang > 7/8 pi*/                { lx = -1.0f; ly =  0.0f; }
                    
                        mXInputStateNew[i].state.Gamepad.sThumbLX = 32767 * lx;
                        mXInputStateNew[i].state.Gamepad.sThumbLY = 32767 * ly;
                    }
                    break;
                case 3:
                    // Enforce Unmodded
                    lx = (mXInputStateNew[i].state.Gamepad.sThumbLX / 32767.0f);
                    ly = (mXInputStateNew[i].state.Gamepad.sThumbLY / 32767.0f);

                    hypot = mSqrt(lx * lx + ly * ly);
                    if (hypot > 1.0f)
                    {
                        ang = mAtan(ly, lx);
                        lx = mCos(ang);
                        ly = mSin(ang);

                        mXInputStateNew[i].state.Gamepad.sThumbLX = 32767 * lx;
                        mXInputStateNew[i].state.Gamepad.sThumbLY = 32767 * ly;
                    }
                    break;
                default:
                    break;
            }

            // this controller was connected or disconnected
            bool bJustConnected = ((mXInputStateOld[i].bConnected != mXInputStateNew[i].bConnected) && (mXInputStateNew[i].bConnected));
            fireXInputConnectEvent(i, (mXInputStateOld[i].bConnected != mXInputStateNew[i].bConnected), mXInputStateNew[i].bConnected);

            // If this controller is disconnected, stop reporting events for it
            if (!mXInputStateNew[i].bConnected)
                continue;

            // == LEFT THUMBSTICK ==
            fireXInputMoveEvent(i, (bJustConnected) || (mXInputStateNew[i].state.Gamepad.sThumbLX != mXInputStateOld[i].state.Gamepad.sThumbLX), SI_MOVE, XI_THUMBLX, (mXInputStateNew[i].state.Gamepad.sThumbLX / 32767.0f));
            fireXInputMoveEvent(i, (bJustConnected) || (mXInputStateNew[i].state.Gamepad.sThumbLY != mXInputStateOld[i].state.Gamepad.sThumbLY), SI_MOVE, XI_THUMBLY, (mXInputStateNew[i].state.Gamepad.sThumbLY / -32767.0f));

            // == RIGHT THUMBSTICK ==
            fireXInputMoveEvent(i, (bJustConnected) || (mXInputStateNew[i].state.Gamepad.sThumbRX != mXInputStateOld[i].state.Gamepad.sThumbRX), SI_MOVE, XI_THUMBRX, (mXInputStateNew[i].state.Gamepad.sThumbRX / 32767.0f));
            fireXInputMoveEvent(i, (bJustConnected) || (mXInputStateNew[i].state.Gamepad.sThumbRY != mXInputStateOld[i].state.Gamepad.sThumbRY), SI_MOVE, XI_THUMBRY, (mXInputStateNew[i].state.Gamepad.sThumbRY / -32767.0f));

            // == LEFT & RIGHT REAR TRIGGERS ==
            fireXInputMoveEvent(i, (bJustConnected) || (mXInputStateNew[i].state.Gamepad.bLeftTrigger != mXInputStateOld[i].state.Gamepad.bLeftTrigger), SI_MOVE, XI_LEFT_TRIGGER, (mXInputStateNew[i].state.Gamepad.bLeftTrigger / 255.0f));
            fireXInputMoveEvent(i, (bJustConnected) || (mXInputStateNew[i].state.Gamepad.bRightTrigger != mXInputStateOld[i].state.Gamepad.bRightTrigger), SI_MOVE, XI_RIGHT_TRIGGER, (mXInputStateNew[i].state.Gamepad.bRightTrigger / 255.0f));

            // == BUTTONS: DPAD ==
            fireXInputButtonEvent(i, bJustConnected, XINPUT_GAMEPAD_DPAD_UP, SI_BUTTON, XI_DPAD_UP);
            fireXInputButtonEvent(i, bJustConnected, XINPUT_GAMEPAD_DPAD_DOWN, SI_BUTTON, XI_DPAD_DOWN);
            fireXInputButtonEvent(i, bJustConnected, XINPUT_GAMEPAD_DPAD_LEFT, SI_BUTTON, XI_DPAD_LEFT);
            fireXInputButtonEvent(i, bJustConnected, XINPUT_GAMEPAD_DPAD_RIGHT, SI_BUTTON, XI_DPAD_RIGHT);

            // == BUTTONS: START & BACK ==
            fireXInputButtonEvent(i, bJustConnected, XINPUT_GAMEPAD_START, SI_BUTTON, XI_START);
            fireXInputButtonEvent(i, bJustConnected, XINPUT_GAMEPAD_BACK, SI_BUTTON, XI_BACK);

            // == BUTTONS: LEFT AND RIGHT THUMBSTICK ==
            fireXInputButtonEvent(i, bJustConnected, XINPUT_GAMEPAD_LEFT_THUMB, SI_BUTTON, XI_LEFT_THUMB);
            fireXInputButtonEvent(i, bJustConnected, XINPUT_GAMEPAD_RIGHT_THUMB, SI_BUTTON, XI_RIGHT_THUMB);

            // == BUTTONS: LEFT AND RIGHT SHOULDERS (formerly WHITE and BLACK on Xbox 1) ==
            fireXInputButtonEvent(i, bJustConnected, XINPUT_GAMEPAD_LEFT_SHOULDER, SI_BUTTON, XI_LEFT_SHOULDER);
            fireXInputButtonEvent(i, bJustConnected, XINPUT_GAMEPAD_RIGHT_SHOULDER, SI_BUTTON, XI_RIGHT_SHOULDER);

            // == BUTTONS: A, B, X, and Y ==
            fireXInputButtonEvent(i, bJustConnected, XINPUT_GAMEPAD_A, SI_BUTTON, XI_A);
            fireXInputButtonEvent(i, bJustConnected, XINPUT_GAMEPAD_B, SI_BUTTON, XI_B);
            fireXInputButtonEvent(i, bJustConnected, XINPUT_GAMEPAD_X, SI_BUTTON, XI_X);
            fireXInputButtonEvent(i, bJustConnected, XINPUT_GAMEPAD_Y, SI_BUTTON, XI_Y);
        }

        if (mXInputStateReset) mXInputStateReset = false;
    }
}
