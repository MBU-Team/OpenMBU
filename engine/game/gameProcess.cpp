//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "core/dnet.h"
#include "core/bitStream.h"
#include "math/mathUtils.h"
#include "game/gameConnection.h"
#include "game/gameBase.h"
#include "game/gameProcess.h"
#include "platform/profiler.h"
#include "console/consoleTypes.h"

//----------------------------------------------------------------------------
ProcessList gClientProcessList(false);
ProcessList gServerProcessList(true);
ProcessList gSPModeProcessList(true);

ConsoleFunction(dumpProcessList, void, 1, 1, "dumpProcessList();")
{
    Con::printf("client process list:");
    getCurrentClientProcessList()->dumpToConsole();
    Con::printf("server process list:");
    gServerProcessList.dumpToConsole();
}
