//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "platform/platform.h"
#include "platform/event.h"
#include "platform/gameInterface.h"
#include "platformWin32/platformWin32.h"
#include "platformWin32/win32WinMgr.h"
#include "platformWin32/winConsole.h"
#include "platformWin32/winDirectInput.h"
#include "console/console.h"
#include "math/mRandom.h"
#include "core/fileStream.h"
#include "d3d9.h"
#include "gfx/gfxInit.h"
#include "gfx/gfxDevice.h"
#include "core/unicode.h"


extern void createFontInit();
extern void createFontShutdown();
extern void installRedBookDevices();
extern void handleRedBookCallback(U32, U32);

#ifdef UNICODE
static const UTF16* windowClassName = L"Darkstar Window Class";
static UTF16 windowName[256] = L"Darkstar Window";
#else
static const char* windowClassName = "Darkstar Window Class";
static char windowName[256] = "Darkstar Window";
#endif
static bool gWindowCreated = false;

static bool windowNotActive = false;

static MRandomLCG sgPlatRandom;
static bool sgQueueEvents;

// is keyboard input a standard (non-changing) VK keycode
#define dIsStandardVK(c) (((0x08 <= (c)) && ((c) <= 0x12)) || \
                          ((c) == 0x1b) ||                    \
                          ((0x20 <= (c)) && ((c) <= 0x2e)) || \
                          ((0x30 <= (c)) && ((c) <= 0x39)) || \
                          ((0x41 <= (c)) && ((c) <= 0x5a)) || \
                          ((0x70 <= (c)) && ((c) <= 0x7B)))

extern U16  DIK_to_Key(U8 dikCode);

// static helper variables
static bool windowLocked = false;
static bool capsLockDown = false;

static BYTE keyboardState[256];
static S32 modifierKeys = 0;
static bool mouseButtonState[3];
static Point2I lastCursorPos(0, 0);
static HANDLE gMutexHandle = NULL;

static bool sgDoubleByteEnabled = false;

// win32 window
Win32WinMgr* Win32Window = NULL;

// track window states
Win32PlatState winState;

void doSleep(U32 time)
{
    Sleep(time);
}

//--------------------------------------
Win32PlatState::Win32PlatState()
{
    log_fp = NULL;

    hinstOpenGL = NULL;
    hinstGLU = NULL;
    hinstOpenAL = NULL;
    appWindow = NULL;
    appDC = NULL;
    appInstance = NULL;
    hGLRC = NULL;
    processId = NULL;

    desktopBitsPixel = NULL;
    desktopWidth = NULL;
    desktopHeight = NULL;
    currentTime = NULL;
    focused = false;

    windowManager = NULL;

    videoMode = NULL;
}


//--------------------------------------
static const char* getMessageName(S32 msg)
{
    switch (msg)
    {
    case WM_KEYDOWN:
        return "WM_KEYDOWN";
    case WM_KEYUP:
        return "WM_KEYUP";
    case WM_SYSKEYUP:
        return "WM_SYSKEYUP";
    case WM_SYSKEYDOWN:
        return "WM_SYSKEYDOWN";
    default:
        return "Unknown!!";
    }
}

//--------------------------------------
bool Platform::excludeOtherInstances(const char* mutexName)
{
#ifdef UNICODE
    UTF16 b[512];
    convertUTF8toUTF16((UTF8*)mutexName, b, sizeof(b));
    gMutexHandle = CreateMutex(NULL, true, b);
#else
    gMutexHandle = CreateMutex(NULL, true, mutexName);
#endif
    if (!gMutexHandle)
        return false;

    if (GetLastError() == ERROR_ALREADY_EXISTS)
    {
        CloseHandle(gMutexHandle);
        gMutexHandle = NULL;
        return false;
    }
    return true;
}

static void setCursorVisible(bool visible);

//--------------------------------------
void Platform::AlertOK(const char* windowTitle, const char* message)
{
    setCursorVisible(true);
#ifdef UNICODE
    UTF16 m[1024], t[512];
    convertUTF8toUTF16((UTF8*)windowTitle, t, sizeof(t));
    convertUTF8toUTF16((UTF8*)message, m, sizeof(m));
    MessageBox(NULL, m, t, MB_ICONINFORMATION | MB_SETFOREGROUND | MB_TASKMODAL | MB_OK);
#else
    MessageBox(NULL, message, windowTitle, MB_ICONINFORMATION | MB_SETFOREGROUND | MB_TASKMODAL | MB_OK);
#endif
}

//--------------------------------------
bool Platform::AlertOKCancel(const char* windowTitle, const char* message)
{
    setCursorVisible(true);
#ifdef UNICODE
    UTF16 m[1024], t[512];
    convertUTF8toUTF16((UTF8*)windowTitle, t, sizeof(t));
    convertUTF8toUTF16((UTF8*)message, m, sizeof(m));
    return MessageBox(NULL, m, t, MB_ICONINFORMATION | MB_SETFOREGROUND | MB_TASKMODAL | MB_OKCANCEL) == IDOK;
#else
    return MessageBox(NULL, message, windowTitle, MB_ICONINFORMATION | MB_SETFOREGROUND | MB_TASKMODAL | MB_OKCANCEL) == IDOK;
#endif
}

//--------------------------------------
bool Platform::AlertRetry(const char* windowTitle, const char* message)
{
    setCursorVisible(true);
    return (MessageBoxA(NULL, message, windowTitle, MB_ICONINFORMATION | MB_SETFOREGROUND | MB_TASKMODAL | MB_RETRYCANCEL) == IDRETRY);
}

int Platform::AlertAbortRetryIgnore(const char* windowTitle, const char* message)
{
    setCursorVisible(true);
    int result = MessageBoxA(NULL, message, windowTitle, MB_ICONINFORMATION | MB_SETFOREGROUND | MB_TASKMODAL | MB_ABORTRETRYIGNORE);
    switch (result)
    {
        case IDABORT:
            return ALERT_RESPONSE_ABORT;
        case IDRETRY:
            return ALERT_RESPONSE_RETRY;
        case IDIGNORE:
            return ALERT_RESPONSE_IGNORE;
    }

    return ALERT_RESPONSE_IGNORE;
}

//--------------------------------------
static void InitInput()
{
    dMemset(keyboardState, 0, 256);
    dMemset(mouseButtonState, 0, sizeof(mouseButtonState));
    capsLockDown = (GetKeyState(VK_CAPITAL) & 0x01);
    if (capsLockDown)
    {
        keyboardState[VK_CAPITAL] |= 0x01;
    }
}

//**********************************************************************************************
// * There has got to be a better way of handling the mouse on the window borders than this... *
//**********************************************************************************************
static bool cursorInWindow()
{
    if (Win32Window == NULL)
        return false;

    if (!winState.focused)
        return false;

    if (Win32Window->mBorderless)
        return true;

    RECT cRect;
    GetClientRect(winState.appWindow, &cRect);

    RECT clientRectOriginal = cRect;
    AdjustWindowRectEx(&cRect, Win32Window->mStyle, false, Win32Window->mExStyle);

    RECT windowRect;
    GetWindowRect(winState.appWindow, &windowRect);

    POINT p;
    GetCursorPos(&p);

    p.x -= windowRect.left - cRect.left;
    p.y -= windowRect.top - cRect.top;

    if (p.x <= clientRectOriginal.left || p.x >= clientRectOriginal.right)
        return false;

    if (p.y <= clientRectOriginal.top || p.y >= clientRectOriginal.bottom)
        return false;

    return true;
}

static bool cursorVisibleWanted = true;
static void setCursorVisible(bool visible)
{
    cursorVisibleWanted = visible;
}

static bool IsCursorVisible()
{
    CURSORINFO ci = { sizeof(CURSORINFO) };
    if (GetCursorInfo(&ci))
        return ci.flags & CURSOR_SHOWING;
    return false;
}

static void updateCursorVisibility()
{
    bool show = cursorVisibleWanted || !cursorInWindow();
    if (show != IsCursorVisible())
    {
        //ShowCursor(show);
        int counter = ShowCursor(show);
        if (counter < -1)
        {
            //Con::errorf("Negative counter!! %d", counter);
        }
        if (counter > 0 && !show)
        {
            //Con::errorf("Failed to hide cursor, counter is %d, trying again.", counter);
            while (ShowCursor(false) >= 0);
        }
    }
}
//**********************************************************************************************

//--------------------------------------
static void setMouseClipping()
{
    ClipCursor(NULL);
    if (winState.focused)
    {
        setCursorVisible(false);

        RECT r;
        GetWindowRect(winState.appWindow, &r);

        if (windowLocked)
        {
            POINT p;
            GetCursorPos(&p);
            lastCursorPos.set(p.x - r.left, p.y - r.top);

            ClipCursor(&r);

            S32 centerX = (r.right + r.left) >> 1;
            S32 centerY = (r.bottom + r.top) >> 1;
            SetCursorPos(centerX, centerY);
        }
        else
            SetCursorPos(lastCursorPos.x + r.left, lastCursorPos.y + r.top);
    }
    else
        setCursorVisible(true);
}

//--------------------------------------
static bool sgTaskbarHidden = false;
static HWND sgTaskbar = NULL;

static void hideTheTaskbar()
{
    //    if ( !sgTaskbarHidden )
    //    {
    //       sgTaskbar = FindWindow( "Shell_TrayWnd", NULL );
    //       if ( sgTaskbar )
    //       {
    //          APPBARDATA abData;
    //          dMemset( &abData, 0, sizeof( abData ) );
    //          abData.cbSize = sizeof( abData );
    //          abData.hWnd = sgTaskbar;
    //          SHAppBarMessage( ABM_REMOVE, &abData );
    //          //ShowWindow( sgTaskbar, SW_HIDE );
    //          sgTaskbarHidden = true;
    //       }
    //    }
}

static void restoreTheTaskbar()
{
    //    if ( sgTaskbarHidden )
    //    {
    //       APPBARDATA abData;
    //       dMemset( &abData, 0, sizeof( abData ) );
    //       abData.cbSize = sizeof( abData );
    //       abData.hWnd = sgTaskbar;
    //       SHAppBarMessage( ABM_ACTIVATE, &abData );
    //       //ShowWindow( sgTaskbar, SW_SHOW );
    //       sgTaskbarHidden = false;
    //    }
}

//--------------------------------------
void Platform::enableKeyboardTranslation(void)
{
}

//--------------------------------------
void Platform::disableKeyboardTranslation(void)
{
}

//--------------------------------------
void Platform::setWindowLocked(bool locked)
{
    windowLocked = locked;
    setMouseClipping();
}

//--------------------------------------
void Platform::minimizeWindow()
{
    ShowWindow(winState.appWindow, SW_MINIMIZE);
    restoreTheTaskbar();
}


//--------------------------------------
static void processKeyMessage(UINT message, WPARAM wParam, LPARAM lParam)
{
    S32 repeatCount = lParam & 0xffff;
    S32 scanCode = (lParam >> 16) & 0xff;
    S32 nVirtkey = dIsStandardVK(wParam) ? TranslateOSKeyCode(wParam) : DIK_to_Key(scanCode);
    S32 keyCode;
    if (wParam == VK_PROCESSKEY && sgDoubleByteEnabled)
        keyCode = MapVirtualKey(scanCode, 1);   // This is the REAL virtual key...
    else
        keyCode = wParam;

    bool extended = (lParam >> 24) & 1;
    bool repeat = (lParam >> 30) & 1;
    bool make = (message == WM_KEYDOWN || message == WM_SYSKEYDOWN);

    S32 newVirtKey = nVirtkey;
    switch (nVirtkey)
    {
    case KEY_ALT:
        newVirtKey = extended ? KEY_RALT : KEY_LALT;
        break;
    case KEY_CONTROL:
        newVirtKey = extended ? KEY_RCONTROL : KEY_LCONTROL;
        break;
    case KEY_SHIFT:
        newVirtKey = (scanCode == 54) ? KEY_RSHIFT : KEY_LSHIFT;
        break;
    case KEY_RETURN:
        if (extended)
            newVirtKey = KEY_NUMPADENTER;
        break;
    }

    S32 modKey = modifierKeys;

    if (make)
    {
        switch (newVirtKey)
        {
        case KEY_LSHIFT:     modifierKeys |= SI_LSHIFT; modKey = 0; break;
        case KEY_RSHIFT:     modifierKeys |= SI_RSHIFT; modKey = 0; break;
        case KEY_LCONTROL:   modifierKeys |= SI_LCTRL; modKey = 0; break;
        case KEY_RCONTROL:   modifierKeys |= SI_RCTRL; modKey = 0; break;
        case KEY_LALT:       modifierKeys |= SI_LALT; modKey = 0; break;
        case KEY_RALT:       modifierKeys |= SI_RALT; modKey = 0; break;
        }
        if (nVirtkey == KEY_CAPSLOCK)
        {
            capsLockDown = !capsLockDown;
            if (capsLockDown)
                keyboardState[keyCode] |= 0x01;
            else
                keyboardState[keyCode] &= 0xFE;
        }
        keyboardState[keyCode] |= 0x80;
    }
    else
    {
        switch (newVirtKey)
        {
        case KEY_LSHIFT:     modifierKeys &= ~SI_LSHIFT; modKey = 0; break;
        case KEY_RSHIFT:     modifierKeys &= ~SI_RSHIFT; modKey = 0; break;
        case KEY_LCONTROL:   modifierKeys &= ~SI_LCTRL; modKey = 0; break;
        case KEY_RCONTROL:   modifierKeys &= ~SI_RCTRL; modKey = 0; break;
        case KEY_LALT:       modifierKeys &= ~SI_LALT; modKey = 0; break;
        case KEY_RALT:       modifierKeys &= ~SI_RALT; modKey = 0; break;
        }
        keyboardState[keyCode] &= 0x7f;
    }

    U16  ascii[3];
    WORD asciiCode = 0;
    dMemset(&ascii, 0, sizeof(ascii));

    S32 res = ToAscii(keyCode, scanCode, keyboardState, ascii, 0);
    if (res == 2)
    {
        asciiCode = ascii[1];
    }
    else if ((res == 1) || (res < 0))
    {
        asciiCode = ascii[0];
    }

    InputEvent event;

    event.deviceInst = 0;
    event.deviceType = KeyboardDeviceType;
    event.objType = SI_KEY;
    event.objInst = newVirtKey;
    event.action = make ? (repeat ? SI_REPEAT : SI_MAKE) : SI_BREAK;
    event.modifier = modKey;
    event.ascii = asciiCode;
    event.fValue = make ? 1.0 : 0.0;

#ifdef LOG_INPUT
    char keyName[25];
    GetKeyNameText(lParam, keyName, 24);
    if (event.action == SI_MAKE)
        Input::log("EVENT (Win): %s key pressed (Repeat count: %d). MODS:%c%c%c\n", keyName, repeatCount,
            (modKey & SI_SHIFT ? 'S' : '.'),
            (modKey & SI_CTRL ? 'C' : '.'),
            (modKey & SI_ALT ? 'A' : '.'));
    else
        Input::log("EVENT (Win): %s key released.\n", keyName);
#endif

    Game->postEvent(event);
}



static S32 mouseX = 0xFFFFFFFF;
static S32 mouseY = 0xFFFFFFFF;

//--------------------------------------
static void CheckCursorPos()
{
    if (windowLocked && winState.focused)
    {
        POINT mousePos;
        GetCursorPos(&mousePos);
        RECT r;

        GetWindowRect(winState.appWindow, &r);

        S32 centerX = (r.right + r.left) >> 1;
        S32 centerY = (r.bottom + r.top) >> 1;

        if (mousePos.x != centerX)
        {
            InputEvent event;

            event.deviceInst = 0;
            event.deviceType = MouseDeviceType;
            event.objType = SI_XAXIS;
            event.objInst = 0;
            event.action = SI_MOVE;
            event.modifier = modifierKeys;
            event.ascii = 0;
            event.fValue = (mousePos.x - centerX);
            Game->postEvent(event);
        }
        if (mousePos.y != centerY)
        {
            InputEvent event;

            event.deviceInst = 0;
            event.deviceType = MouseDeviceType;
            event.objType = SI_YAXIS;
            event.objInst = 0;
            event.action = SI_MOVE;
            event.modifier = modifierKeys;
            event.ascii = 0;
            event.fValue = (mousePos.y - centerY);
            Game->postEvent(event);
        }
        SetCursorPos(centerX, centerY);
    }
}

//--------------------------------------
static void mouseButtonEvent(S32 action, S32 objInst)
{
    CheckCursorPos();
    if (!windowLocked)
    {
        if (action == SI_MAKE)
            SetCapture(winState.appWindow);
        else
            ReleaseCapture();
    }

    U32 buttonId = objInst - KEY_BUTTON0;
    if (buttonId < 3)
        mouseButtonState[buttonId] = (action == SI_MAKE) ? true : false;

    InputEvent event;

    event.deviceInst = 0;
    event.deviceType = MouseDeviceType;
    event.objType = SI_BUTTON;
    event.objInst = objInst;
    event.action = action;
    event.modifier = modifierKeys;
    event.ascii = 0;
    event.fValue = action == SI_MAKE ? 1.0 : 0.0;

#ifdef LOG_INPUT
    if (action == SI_MAKE)
        Input::log("EVENT (Win): mouse button%d pressed. MODS:%c%c%c\n", buttonId, (modifierKeys & SI_SHIFT ? 'S' : '.'), (modifierKeys & SI_CTRL ? 'C' : '.'), (modifierKeys & SI_ALT ? 'A' : '.'));
    else
        Input::log("EVENT (Win): mouse button%d released.\n", buttonId);
#endif
    Game->postEvent(event);
}

//--------------------------------------
static void mouseWheelEvent(S32 delta)
{
    static S32 _delta = 0;

    _delta += delta;
    if (abs(delta) >= WHEEL_DELTA)
    {
        _delta = 0;
        InputEvent event;

        event.deviceInst = 0;
        event.deviceType = MouseDeviceType;
        event.objType = SI_ZAXIS;
        event.objInst = 0;
        event.action = SI_MOVE;
        event.modifier = modifierKeys;
        event.ascii = 0;
        event.fValue = delta;

#ifdef LOG_INPUT
        Input::log("EVENT (Win): mouse wheel moved. delta = %d\n", delta);
#endif

        Game->postEvent(event);
    }
}

//--------------------------------------
static void updateWindowFocus(HWND hWnd, bool newFocus, bool setMouseClip)
{
    // Ensure the window is actually focused right now
    bool currentFocus = GetForegroundWindow() == hWnd;
    if (winState.focused == newFocus || currentFocus != newFocus)
        return;

    winState.focused = newFocus;
    Con::printf("Window focused: %d", winState.focused);

    if (winState.focused)
    {
        Input::activate();
    }
    else
    {
        DInputManager* mgr = dynamic_cast<DInputManager*>(Input::getManager());
        if (!mgr || !mgr->isMouseActive())
        {
            // Deactivate all the mouse triggers:
            for (U32 i = 0; i < 3; i++)
            {
                if (mouseButtonState[i])
                    mouseButtonEvent(SI_BREAK, KEY_BUTTON0 + i);
            }
        }
        Input::deactivate();
    }

    if (windowLocked && setMouseClip)
        setMouseClipping();
}

struct WinMessage
{
    UINT message;
    WPARAM wParam;
    LPARAM lParam;

    WinMessage() {}
    WinMessage(UINT m, WPARAM w, LPARAM l) : message(m), wParam(w), lParam(l) {}
};

Vector<WinMessage> sgWinMessages;


#ifndef PBT_APMQUERYSUSPEND
#define PBT_APMQUERYSUSPEND             0x0000
#endif

//--------------------------------------
static LRESULT PASCAL WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_POWERBROADCAST:
    {
        if (wParam == PBT_APMQUERYSUSPEND)
            return BROADCAST_QUERY_DENY;
        return true;
    }
    case WM_SYSCOMMAND:
        switch (wParam)
        {
        case SC_SCREENSAVE:
        case SC_MONITORPOWER:
            if (GetForegroundWindow() == winState.appWindow)
            {
                return 0;
            }
            break;
        }
        break;
    case WM_ACTIVATE:
        setCursorVisible(false);
        if (LOWORD(wParam) != WA_INACTIVE)
        {
            Game->refreshWindow();

            if (Win32Window)
            {
                if (HIWORD(wParam) != 0)
                {
                    ShowWindow(winState.appWindow, SW_RESTORE);
                    GFX->activate();
                }
            }
        }
        updateWindowFocus(hWnd, LOWORD(wParam) != WA_INACTIVE, true);
        break;
    case WM_SETFOCUS:
        updateWindowFocus(hWnd, true, true);
        break;
    case WM_KILLFOCUS:
        updateWindowFocus(hWnd, false, true);
        break;
    case WM_NCACTIVATE:
        updateWindowFocus(hWnd, wParam, false);
        break;

    case WM_MOVE:
        Game->refreshWindow();
        break;

    case MM_MCINOTIFY:
        handleRedBookCallback(wParam, lParam);
        break;

        //case WM_DESTROY:
    case WM_CLOSE:
        delete Win32Window;
        Win32Window = NULL;

        PostQuitMessage(0);
        break;
    default:
    {
        if (sgQueueEvents)
        {
            WinMessage msg(message, wParam, lParam);
            sgWinMessages.push_front(msg);
        }
    }
    }

    return DefWindowProc(hWnd, message, wParam, lParam);
}

//--------------------------------------
static void OurDispatchMessages()
{
    WinMessage msg(0, 0, 0);
    UINT message;
    WPARAM wParam;
    LPARAM lParam;

    DInputManager* mgr = dynamic_cast<DInputManager*>(Input::getManager());

    while (sgWinMessages.size())
    {
        msg = sgWinMessages.front();
        sgWinMessages.pop_front();
        message = msg.message;
        wParam = msg.wParam;
        lParam = msg.lParam;

        if (!mgr || !mgr->isMouseActive())
        {
            switch (message)
            {
            case WM_MOUSEMOVE:
                if (!windowLocked)
                {
                    MouseMoveEvent event;
                    S32 xPos = LOWORD(lParam);
                    S32 yPos = HIWORD(lParam);

                    if (Win32Window->mBorderless)
                    {
                        F32 intendedWidth = Win32Window->mIntendedWidth;
                        F32 intendedHeight = Win32Window->mIntendedHeight;
                        F32 actualWidth = Win32Window->mActualWidth;
                        F32 actualHeight = Win32Window->mActualHeight;

                        xPos = (S32)((F32)xPos * intendedWidth / actualWidth);
                        yPos = (S32)((F32)yPos * intendedHeight / actualHeight);
                    }


                    event.xPos = xPos;  // horizontal position of cursor
                    event.yPos = yPos;  // vertical position of cursor
                    event.modifier = modifierKeys;

#ifdef LOG_INPUT
#ifdef LOG_MOUSEMOVE
                    Input::log("EVENT (Win): mouse move to (%d, %d).\n", event.xPos, event.yPos);
#endif
#endif
                    Game->postEvent(event);
                }
                break;
            case WM_LBUTTONDOWN:
                mouseButtonEvent(SI_MAKE, KEY_BUTTON0);
                break;
            case WM_MBUTTONDOWN:
                mouseButtonEvent(SI_MAKE, KEY_BUTTON2);
                break;
            case WM_RBUTTONDOWN:
                mouseButtonEvent(SI_MAKE, KEY_BUTTON1);
                break;
            case WM_LBUTTONUP:
                mouseButtonEvent(SI_BREAK, KEY_BUTTON0);
                break;
            case WM_MBUTTONUP:
                mouseButtonEvent(SI_BREAK, KEY_BUTTON2);
                break;
            case WM_RBUTTONUP:
                mouseButtonEvent(SI_BREAK, KEY_BUTTON1);
                break;
            case WM_MOUSEWHEEL:
                mouseWheelEvent((S16)HIWORD(wParam));
                break;
            }
        }

        if (!mgr || !mgr->isKeyboardActive())
        {
            switch (message)
            {
            case WM_KEYUP:
            case WM_SYSKEYUP:
            case WM_KEYDOWN:
            case WM_SYSKEYDOWN:
                processKeyMessage(message, wParam, lParam);
                break;
            }
        }
    }
}

//--------------------------------------
static bool ProcessMessages()
{
    MSG msg;

    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
    {
        if (msg.message == WM_QUIT)
            return false;

        TranslateMessage(&msg);
        DispatchMessage(&msg);
        OurDispatchMessages();
    }

    return true;
}

//--------------------------------------
void Platform::process()
{
    updateCursorVisibility();

    DInputManager* mgr = dynamic_cast<DInputManager*>(Input::getManager());
    if (!mgr || !mgr->isMouseActive())
    {
        CheckCursorPos();
    }
    else
    {
        // re-center mouse position
        RECT r;
        GetWindowRect(winState.appWindow, &r);
        S32 centerX = (r.right + r.left) >> 1;
        S32 centerY = (r.bottom + r.top) >> 1;
        SetCursorPos(centerX, centerY);
    }

    WindowsConsole->process();

    if (!ProcessMessages())
    {
        // generate a quit event
        Event quitEvent;
        quitEvent.type = QuitEventType;

        Game->postEvent(quitEvent);
    }
    // if there's no window, we sleep 1, otherwise we sleep 0
    if (!Game->isJournalReading())
        Sleep(Win32Window != NULL ? 0 : 1); // give others some process time if necessary...

    Input::process();
}

extern U32 calculateCRC(void* buffer, S32 len, U32 crcVal);

#if defined(TORQUE_DEBUG) || defined(INTERNAL_RELEASE)
static U32 stubCRC = 0;
#else
static U32 stubCRC = 0xEA63F56C;
#endif

//--------------------------------------
static void GetDesktopState()
{
    HWND hDesktop = GetDesktopWindow();
    HDC hDeskDC = GetDC(hDesktop);
    winState.desktopBitsPixel = GetDeviceCaps(hDeskDC, BITSPIXEL);
    winState.desktopWidth = GetDeviceCaps(hDeskDC, HORZRES);
    winState.desktopHeight = GetDeviceCaps(hDeskDC, VERTRES);
    ReleaseDC(hDesktop, hDeskDC);
}

//--------------------------------------
//
// This function exists so DirectInput can communicate with the Windows mouse
// in windowed mode.
//
//--------------------------------------
void setModifierKeys(S32 modKeys)
{
    modifierKeys = modKeys;
}


//--------------------------------------
void Platform::setWindowSize(U32 newWidth, U32 newHeight, bool fullScreen, bool borderless)
{
    Win32Window->resizeWindow(newWidth, newHeight, fullScreen, borderless);
}


//--------------------------------------
ConsoleFunction(getDesktopResolution, const char*, 1, 1, "getDesktopResolution()")
{
    argc; argv;
    char buffer[256];
    dSprintf(buffer, sizeof(buffer), "%d %d %d", winState.desktopWidth, winState.desktopHeight, winState.desktopBitsPixel);
    char* returnString = Con::getReturnBuffer(dStrlen(buffer) + 1);
    dStrcpy(returnString, buffer);
    return(returnString);
}

//--------------------------------------
void Platform::init()
{
    //SetProcessAffinityMask( GetCurrentProcess(), 1 );

    Con::printf("Initializing platform...");
    // Set the platform variable for the scripts
    Con::setVariable("$platform", "windows");

    WinConsole::create();
    //why exactly is this here?
    if (!WinConsole::isEnabled())
        Input::init();

    InitInput();   // in case DirectInput falls through
    GetDesktopState();
    installRedBookDevices();

    Con::printf("Creating GFXDevice...");
    GFXDevice::create();

    sgDoubleByteEnabled = GetSystemMetrics(SM_DBCSENABLED);
    sgQueueEvents = true;
    Con::printf("Done");
}

//--------------------------------------
void Platform::shutdown()
{
    sgQueueEvents = false;

    if (gMutexHandle)
        CloseHandle(gMutexHandle);
    setWindowLocked(false);

    Input::destroy();
    WinConsole::destroy();

    GFXDevice::destroy();
}


class WinTimer
{
private:
    U32 mTickCountCurrent;
    U32 mTickCountNext;
    S64 mPerfCountCurrent;
    S64 mPerfCountNext;
    S64 mFrequency;
    F64 mPerfCountRemainderCurrent;
    F64 mPerfCountRemainderNext;
    bool mUsingPerfCounter;
public:
    WinTimer()
    {
        mPerfCountRemainderCurrent = 0.0f;
        mUsingPerfCounter = QueryPerformanceFrequency((LARGE_INTEGER*)&mFrequency);
        if (mUsingPerfCounter)
            mUsingPerfCounter = QueryPerformanceCounter((LARGE_INTEGER*)&mPerfCountCurrent);
        if (!mUsingPerfCounter)
            mTickCountCurrent = GetTickCount();
    }
    U32 getElapsedMS()
    {
        if (mUsingPerfCounter)
        {
            QueryPerformanceCounter((LARGE_INTEGER*)&mPerfCountNext);
            F64 elapsedF64 = (1000.0 * F64(mPerfCountNext - mPerfCountCurrent) / F64(mFrequency));
            elapsedF64 += mPerfCountRemainderCurrent;
            U32 elapsed = mFloor(elapsedF64);
            mPerfCountRemainderNext = elapsedF64 - F64(elapsed);
            return elapsed;
        }
        else
        {
            mTickCountNext = GetTickCount();
            return mTickCountNext - mTickCountCurrent;
        }
    }
    void advance()
    {
        mTickCountCurrent = mTickCountNext;
        mPerfCountCurrent = mPerfCountNext;
        mPerfCountRemainderCurrent = mPerfCountRemainderNext;
    }
};

static WinTimer gTimer;

extern bool LinkConsoleFunctions;

//--------------------------------------
static S32 run(S32 argc, const char** argv)
{

    // Console hack to ensure consolefunctions get linked in
    LinkConsoleFunctions = true;

    createFontInit();

    S32 ret = Game->main(argc, argv);
    createFontShutdown();
    return ret;
}

/// Reference to the render target allocated on this window.
static GFXWindowTargetRef mTarget = NULL;

GFXWindowTarget * Platform::getWindowGFXTarget()
{
    return mTarget;
}

//--------------------------------------
void Platform::initWindow(const Point2I& initialSize, const char* name)
{
    Con::printf("Video Init:");

    //find our adapters
    Vector<GFXAdapter*> adapters;
    GFXInit::enumerateAdapters();
    GFXInit::getAdapters(&adapters);

    //loop through and tell the user what kind of adapters we found
    if (!adapters.size())
        Con::printf("Could not find a display adapter");
    else
    {
        for (int k = 0; k < adapters.size(); k++)
        {
            if (adapters[k]->mType == Direct3D9)
                Con::printf("Direct 3D device found");
            else if (adapters[k]->mType == OpenGL)
                Con::printf("OpenGL device found");
            else
                Con::printf("Unknown device found");
        }
    }

    Con::printf("");

    const char* renderer = Con::getVariable("$pref::Video::displayDevice");
    AssertFatal(dStrcmp(renderer, "OpenGL") == 0 || dStrcmp(renderer, "D3D") == 0, "Invalid renderer found");

    if (dStrcmp(renderer, "D3D") == 0)
        GFXInit::createDevice(adapters[0]);
    else
        GFXInit::createDevice(adapters[1]);

    //create the window OF PROPER SIZE (loaded from prefs)
    //should save each part of the video mode to prefs, then
    //on load, find the closest one to that (most likely the exact one)
    GFXVideoMode vm;

    int fullscreenType = Con::getIntVariable("$pref::Video::fullScreen", 0);

    U32 w = 0, h = 0, d = 0;

    if (fullscreenType > 0)
        dSscanf(Con::getVariable("$pref::Video::resolution"), "%d %d %d", &w, &h, &d);
    else
    {
        dSscanf(Con::getVariable("$pref::Video::windowedRes"), "%d %d", &w, &h);
        if (w == 0 || h == 0)
            dSscanf(Con::getVariable("$pref::Video::resolution"), "%d %d %d", &w, &h, &d);
        else
            d = 32;
    }

    if (w == 0 || h == 0 || d == 0)
    {
        w = 640;
        h = 480;
        d = 32;
    }

    int antialiasLevel = Con::getIntVariable("$pref::Video::FSAALevel", 0);

    vm.resolution.x = w;
    vm.resolution.y = h;
    vm.bitDepth = d;
    vm.fullScreen = fullscreenType == 1;
    vm.borderless = fullscreenType == 2;
    vm.refreshRate = 60; //HACK
    vm.antialiasLevel = antialiasLevel;

    winState.videoMode = new GFXVideoMode(vm);

    //TODO find a better way to handle Win32WinMgr...
    // create the window
    Win32Window = new Win32WinMgr(GFX->getDeviceIndex(), WindowProc);
    Win32Window->createWindow(name, 0, 0, vm.resolution.x, vm.resolution.y, vm.refreshRate, vm.fullScreen, vm.borderless);

    // YaHOOOOOO multiple windows!
    const Vector<GFXDevice*>* deviceVector = GFX->getDeviceVector();

    for (U32 i = 0; i < deviceVector->size(); i++)
    {
        GFX->setActiveDevice(i);
        //GFX->init(vm);
    }

    winState.processId = GetCurrentProcessId();

    mTarget = GFX->allocWindowTarget();//Win32Window);
    if(mTarget.isValid())
        mTarget->resetMode();
}

//--------------------------------------
S32 main(S32 argc, const char** argv)
{
    winState.appInstance = GetModuleHandleA(argv[0]);
    return run(argc, argv);
}

//--------------------------------------
S32 PASCAL WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR lpszCmdLine, S32)
{
    Vector<char*> argv;
    char moduleName[256];
    GetModuleFileNameA(NULL, moduleName, sizeof(moduleName));
    argv.push_back(moduleName);

    for (const char* word, *ptr = lpszCmdLine; *ptr; ) {
        // Eat white space
        for (; dIsspace(*ptr) && *ptr; ptr++)
            ;
        // Pick out the next word
        for (word = ptr; !dIsspace(*ptr) && *ptr; ptr++)
            ;
        // Add the word to the argument list.
        if (*word) {
            int len = ptr - word;
            char* arg = (char*)dMalloc(len + 1);
            dStrncpy(arg, word, len);
            arg[len] = 0;
            argv.push_back(arg);
        }
    }

    winState.appInstance = hInstance;

    S32 retVal = run(argv.size(), (const char**)argv.address());

    for (U32 j = 1; j < argv.size(); j++)
        dFree(argv[j]);

    return retVal;
}

extern U32 gFixedFramerate;

//--------------------------------------
void TimeManager::process()
{
    TimeEvent event;
    event.elapsedTime = gTimer.getElapsedMS();

//#ifndef TORQUE_NVPERFHUD
//    if (event.elapsedTime > 2)
//#endif
    if (gFixedFramerate == 0 || event.elapsedTime > (1000 / gFixedFramerate))
    {
        gTimer.advance();
        Game->postEvent(event);
    }
}

F32 Platform::getRandom()
{
    return sgPlatRandom.randF();
}

////--------------------------------------
/// Spawn the default Operating System web browser with a URL
/// @param webAddress URL to pass to browser
/// @return true if browser successfully spawned
bool Platform::openWebBrowser(const char* webAddress)
{
    static bool sHaveKey = false;
    static char sWebKey[512];

    if (!sHaveKey)
    {
        HKEY regKey;
        DWORD size = sizeof(sWebKey);

        if (RegOpenKeyEx(HKEY_CLASSES_ROOT, dT("\\http\\shell\\open\\command"), 0, KEY_QUERY_VALUE, &regKey) != ERROR_SUCCESS)
        {
            Con::errorf(ConsoleLogEntry::General, "Failed to open the HKCR\\http registry key!!!");
            return(false);
        }

        if (RegQueryValueEx(regKey, dT(""), NULL, NULL, (unsigned char*)sWebKey, &size) != ERROR_SUCCESS)
        {
            Con::errorf(ConsoleLogEntry::General, "Failed to query the open command registry key!!!");
            return(false);
        }

        RegCloseKey(regKey);
        sHaveKey = true;

        char* p = dStrstr(sWebKey, (char*)"\"%1\"");
        if (p) *p = 0;
    }

    STARTUPINFO si;
    dMemset(&si, 0, sizeof(si));
    si.cb = sizeof(si);

    char buf[1024];
    dSprintf(buf, sizeof(buf), "%s %s", (const char*)sWebKey, webAddress);

#ifdef UNICODE
    UTF16 b[1024];
    convertUTF8toUTF16((UTF8*)buf, b, sizeof(b));
#endif

    Con::errorf(ConsoleLogEntry::General, "Web browser command = %s **", buf);

    PROCESS_INFORMATION pi;
    dMemset(&pi, 0, sizeof(pi));
    CreateProcess(NULL,
#ifdef UNICODE
        b,
#else
        buf,
#endif
        NULL,
        NULL,
        false,
        CREATE_NEW_CONSOLE | CREATE_NEW_PROCESS_GROUP,
        NULL,
        NULL,
        &si,
        &pi);

    return(true);
}

//--------------------------------------
// Login password routines:
//--------------------------------------
#ifdef UNICODE
static const UTF16* TorqueRegKey = dT("SOFTWARE\\GarageGames\\Torque");
#else
static const char* TorqueRegKey = "SOFTWARE\\GarageGames\\Torque";
#endif

const char* Platform::getLoginPassword()
{
    HKEY regKey;
    char* returnString = NULL;
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, TorqueRegKey, 0, KEY_QUERY_VALUE, &regKey) == ERROR_SUCCESS)
    {
        U8 buf[32];
        DWORD size = sizeof(buf);
        if (RegQueryValueEx(regKey, dT("LoginPassword"), NULL, NULL, buf, &size) == ERROR_SUCCESS)
        {
            returnString = Con::getReturnBuffer(size + 1);
            dStrcpy(returnString, (const char*)buf);
        }

        RegCloseKey(regKey);
    }

    if (returnString)
        return(returnString);
    else
        return("");
}

//--------------------------------------
bool Platform::setLoginPassword(const char* password)
{
    HKEY regKey;
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, TorqueRegKey, 0, KEY_WRITE, &regKey) == ERROR_SUCCESS)
    {
        if (RegSetValueEx(regKey, dT("LoginPassword"), 0, REG_SZ, (const U8*)password, dStrlen(password) + 1) != ERROR_SUCCESS)
            Con::errorf(ConsoleLogEntry::General, "setLoginPassword - Failed to set the subkey value!");

        RegCloseKey(regKey);
        return(true);
    }
    else
        Con::errorf(ConsoleLogEntry::General, "setLoginPassword - Failed to open the Torque registry key!");

    return(false);
}

//--------------------------------------
// Silly Korean registry key checker:
//--------------------------------------
ConsoleFunction(isKoreanBuild, bool, 1, 1, "isKoreanBuild()")
{
    argc; argv;
    HKEY regKey;
    bool result = false;
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, TorqueRegKey, 0, KEY_QUERY_VALUE, &regKey) == ERROR_SUCCESS)
    {
        DWORD val;
        DWORD size = sizeof(val);
        if (RegQueryValueEx(regKey, dT("Korean"), NULL, NULL, (U8*)&val, &size) == ERROR_SUCCESS)
            result = (val > 0);

        RegCloseKey(regKey);
    }

    return(result);
}
