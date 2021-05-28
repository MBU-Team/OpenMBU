//-----------------------------------------------------------------------------
// Torque Shader Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------
#ifndef _SCREENSHOT_H_
#define _SCREENSHOT_H_

#include "gfx/gfxDevice.h"

//**************************************************************************
/*!
   This class will eventually support various capabilities such as panoramics,
   high rez captures, and cubemap captures.

   Right now it just captures standard screenshots, but it does support
   captures from multisample back buffers, so antialiased captures will work.
*/
//**************************************************************************

class ScreenShot
{
    GFXTexHandle   mRenderSurface;
    U32            mSurfWidth;
    U32            mSurfHeight;

    static void texManagerCallback(GFXTexCallbackCode code, void* userData);
    void setupSurface(U32 width, U32 height);

public:

    bool     mPending;        // necessary to synch capture before backbuffer flips - see GuiCanvas

#ifdef UNICODE
    UTF16 mFilename[512];
#else
    char mFilename[256];
#endif


    ScreenShot();

    /// captures the back buffer
    virtual void captureStandard() {}

    // captures a custom size - test
    virtual void captureCustom() {}


};

extern ScreenShot* gScreenShot;

#endif  // _SCREENSHOT_H_
