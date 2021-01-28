//-----------------------------------------------------------------------------
// Torque Shader Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _H_MORIANGAME_
#define _H_MORIANGAME_

#ifndef _GAMEINTERFACE_H_
#include "platform/gameInterface.h"
#endif

class MorianGame : public GameInterface
{
  public:
   S32 main(S32 argc, const char **argv);
};

#endif  // _H_MORIANGAME_
