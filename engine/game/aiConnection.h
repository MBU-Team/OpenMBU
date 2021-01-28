//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _AICONNECTION_H_
#define _AICONNECTION_H_

#ifndef _GAMECONNECTION_H_
#include "game/gameConnection.h"
#endif
#ifndef _MOVEMANAGER_H_
#include "game/moveManager.h"
#endif

//-----------------------------------------------------------------------------

class AIConnection : public GameConnection
{
   typedef GameConnection Parent;

protected:
   Move mMove;

public:
   AIConnection();
   DECLARE_CONOBJECT( AIConnection );

   // Interface
   const Move& getMove();
   void setMove(Move *m);

   // GameConnection overrides
   void clearMoves(U32 n);
   virtual void getMoveList(Move **,U32 *numMoves);
};


#endif
