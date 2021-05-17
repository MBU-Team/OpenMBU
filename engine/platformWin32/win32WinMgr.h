//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _WIN32WINMGR_H_
#define _WIN32WINMGR_H_

// Win32 version
class Win32WinMgr
{
public:
    Win32WinMgr(U32 uniqueIDNum, WNDPROC winProc);
    ~Win32WinMgr();

    void createWindow(const char* windowTitle, const U32 x, const U32 y, const U32 width, const U32 height, const U32 frequency, bool fullscreen, bool borderless);
    void destroyWindow();

    RectI getCenteredWindowRect(const U32 width, const U32 height, bool fullscreen, bool borderless);

    void resizeWindow(const U32 width, const U32 height, bool fullscreen, bool borderless);

    DWORD mStyle;
    DWORD mExStyle;

    DWORD mIntendedWidth;
    DWORD mIntendedHeight;
    DWORD mActualWidth;
    DWORD mActualHeight;
    bool mBorderless;
};

#endif // _WIN32WINMGR_H_
