//-----------------------------------------------------------------------------
// Torque Shader Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------
#include "screenshot.h"
#include "sceneGraph/sceneGraph.h"
#include "core/fileStream.h"
#include "gui/core/guiCanvas.h"
#include "core/unicode.h"

// This must be initialized by the device
ScreenShot* gScreenShot = NULL;


//**************************************************************************
// Console function
//**************************************************************************
ConsoleFunction(screenShot, void, 3, 3, "(string file, string format)"
    "Take a screenshot.\n\n"
    "@param format One of JPEG or PNG.")
{
    if (!gScreenShot)
    {
        Con::errorf("Screenshot module not initialized by device");
    }

    gScreenShot->mPending = true;
#ifdef UNICODE
    convertUTF8toUTF16((UTF8*)argv[1], gScreenShot->mFilename, sizeof(gScreenShot->mFilename));
#else
    dStrcpy(gScreenShot->mFilename, argv[1]);
#endif
}

//**************************************************************************
// ScreenShot class
//**************************************************************************
ScreenShot::ScreenShot()
{
    mSurfWidth = 0;
    mSurfHeight = 0;
    mPending = false;
}


