//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _APP_GAMEPROCESS_H_
#define _APP_GAMEPROCESS_H_

#include "platform/platform.h"
#include "sim/processList.h"

#include "game/game.h"

class GameBase;
class GameConnection;
struct Move;

extern ProcessList gClientProcessList;
extern ProcessList gServerProcessList;
extern ProcessList gSPModeProcessList;

inline ProcessList* getCurrentServerProcessList()
{
    return gSPMode ? &gSPModeProcessList : &gServerProcessList;
}

inline ProcessList* getCurrentClientProcessList()
{
    return gSPMode ? &gSPModeProcessList : &gClientProcessList;
}

#endif