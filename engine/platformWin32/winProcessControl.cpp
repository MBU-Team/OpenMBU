//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "platformWin32/platformWin32.h"

void Platform::postQuitMessage(const U32 in_quitVal)
{
    PostQuitMessage(in_quitVal);
}

void Platform::debugBreak()
{
    //DebugBreak();

    // Using this one as it allows us to see the call stack
    __debugbreak();
}

void Platform::forceShutdown(S32 returnValue)
{
    ExitProcess(returnValue);
}
