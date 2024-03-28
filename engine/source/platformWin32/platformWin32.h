//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _PLATFORMWIN32_H_
#define _PLATFORMWIN32_H_

// Sanity check for UNICODE
#ifdef TORQUE_UNICODE
#  ifndef UNICODE
#     error "ERROR: You must have UNICODE defined in your preprocessor settings (ie, /DUNICODE) if you have TORQUE_UNICODE enabled in torqueConfig.h!"
#  endif
#endif

// define this so that we can use WM_MOUSEWHEEL messages...
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0500
#endif
#include <Windows.h>
#ifndef _PLATFORM_H_
#include "platform/platform.h"
#endif
#ifndef _MRECT_H_
#include "math/mRect.h"
#endif

#if defined(TORQUE_COMPILER_CODEWARRIOR)
#  include <ansi_prefix.win32.h>
#  include <stdio.h>
#  include <string.h>
#else
#  include <stdio.h>
#  include <string.h>
#endif

#if defined(TORQUE_COMPILER_VISUALC) || defined(TORQUE_COMPILER_GCC2)
#define vsnprintf _vsnprintf
#define stricmp _stricmp
#define strnicmp _strnicmp
#define strupr _strupr
#define strlwr _strlwr
#endif

#ifdef _MSC_VER
#pragma warning(disable: 4996) // turn off "deprecation" warnings
#endif

#define NOMINMAX

class Win32WinMgr;
struct GFXVideoMode;

struct Win32PlatState
{
    FILE* log_fp;
    HINSTANCE hinstOpenGL;
    HINSTANCE hinstGLU;
    HINSTANCE hinstOpenAL;
    HWND appWindow;
    HDC appDC;
    HINSTANCE appInstance;
    HGLRC hGLRC;
    DWORD processId;
    WNDCLASSEX wc;
#ifdef UNICODE
    HIMC imeHandle;
#endif

    S32 desktopBitsPixel;
    S32 desktopWidth;
    S32 desktopHeight;
    U32 currentTime;
    bool focused;

    Win32WinMgr* windowManager;
    GFXVideoMode* videoMode;

    Win32PlatState();
};

extern Win32PlatState winState;

extern void setModifierKeys(S32 modKeys);

extern S32(WINAPI* qwglSwapIntervalEXT)(S32 interval);
extern BOOL(WINAPI* qwglGetDeviceGammaRamp3DFX)(HDC, LPVOID);
extern BOOL(WINAPI* qwglSetDeviceGammaRamp3DFX)(HDC, LPVOID);
extern S32(WINAPI* qwglChoosePixelFormat)(HDC, CONST PIXELFORMATDESCRIPTOR*);
extern S32(WINAPI* qwglDescribePixelFormat) (HDC, S32, UINT, LPPIXELFORMATDESCRIPTOR);
extern S32(WINAPI* qwglGetPixelFormat)(HDC);
extern BOOL(WINAPI* qwglSetPixelFormat)(HDC, S32, CONST PIXELFORMATDESCRIPTOR*);
extern BOOL(WINAPI* qwglSwapBuffers)(HDC);
extern BOOL(WINAPI* qwglCopyContext)(HGLRC, HGLRC, UINT);
extern HGLRC(WINAPI* qwglCreateContext)(HDC);
extern HGLRC(WINAPI* qwglCreateLayerContext)(HDC, S32);
extern BOOL(WINAPI* qwglDeleteContext)(HGLRC);
extern HGLRC(WINAPI* qwglGetCurrentContext)(VOID);
extern HDC(WINAPI* qwglGetCurrentDC)(VOID);
extern PROC(WINAPI* qwglGetProcAddress)(LPCSTR);
extern BOOL(WINAPI* qwglMakeCurrent)(HDC, HGLRC);
extern BOOL(WINAPI* qwglShareLists)(HGLRC, HGLRC);
extern BOOL(WINAPI* qwglUseFontBitmaps)(HDC, DWORD, DWORD, DWORD);
extern BOOL(WINAPI* qwglUseFontOutlines)(HDC, DWORD, DWORD, DWORD, FLOAT, FLOAT, S32, LPGLYPHMETRICSFLOAT);
extern BOOL(WINAPI* qwglDescribeLayerPlane)(HDC, S32, S32, UINT, LPLAYERPLANEDESCRIPTOR);
extern S32(WINAPI* qwglSetLayerPaletteEntries)(HDC, S32, S32, S32, CONST COLORREF*);
extern S32(WINAPI* qwglGetLayerPaletteEntries)(HDC, S32, S32, S32, COLORREF*);
extern BOOL(WINAPI* qwglRealizeLayerPalette)(HDC, S32, BOOL);
extern BOOL(WINAPI* qwglSwapLayerBuffers)(HDC, UINT);


#endif //_PLATFORMWIN32_H_
