//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "game/aiConnection.h"

IMPLEMENT_CONOBJECT( AIConnection );


//-----------------------------------------------------------------------------

AIConnection::AIConnection() {
   mAIControlled = true;
   mMove = NullMove;
}


//-----------------------------------------------------------------------------

void AIConnection::clearMoves( U32 )
{
   // Clear the pending move list. This connection generates moves
   // on the fly, so there are never any pending moves.
}

void AIConnection::setMove(Move* m)
{
   mMove = *m;
}

const Move& AIConnection::getMove()
{
   return mMove;
}

/// Retrive the pending moves
/**
 * The GameConnection base class queues moves for delivery to the
 * controll object.  This function is normally used to retrieve the
 * queued moves recieved from the client.  The AI connection does not
 * have a connected client and simply generates moves on-the-fly
 * base on it's current state.
 */
void AIConnection::getMoveList( Move **lngMove, U32 *numMoves )
{
   *numMoves = 1;
   *lngMove = &mMove;
}


//-----------------------------------------------------------------------------
// Console functions & methods
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------

static inline F32 moveClamp(F32 v)
{
   // Support function to convert/clamp the input into a move rotation
   // which only allows 0 -> M_2PI.
   F32 a = mClampF(v,-M_PI,M_PI);
   return (a < 0)? a + M_2PI: a;
}


//-----------------------------------------------------------------------------
/// Construct and connect an AI connection object
ConsoleFunction(aiConnect, S32 , 2, 20, "(...)"
                "Make a new AIConnection, and pass arguments to the onConnect script callback.")
{
   // Create the connection
   AIConnection *aiConnection = new AIConnection();
   aiConnection->registerObject();

   // Add the connection to the client group
   SimGroup *g = Sim::getClientGroup();
   g->addObject( aiConnection );

   // Prep the arguments for the console exec...
   // Make sure and leav args[1] empty.
   const char* args[21];
   args[0] = "onConnect";
   for (S32 i = 1; i < argc; i++)
      args[i + 1] = argv[i];

   // Execute the connect console function, this is the same
   // onConnect function invoked for normal client connections
   Con::execute(aiConnection, argc + 1, args);
   return aiConnection->getId();
}


//-----------------------------------------------------------------------------
ConsoleMethod(AIConnection,setMove,void,4, 4,"(string field, float value)"
              "Set a field on the current move.\n\n"
              "@param   field One of {'x','y','z','yaw','pitch','roll'}\n"
              "@param   value Value to set field to.")
{
   Move move = object->getMove();

   // Ok, a little slow for now, but this is just an example..
   if (!dStricmp(argv[2],"x"))
      move.x = mClampF(dAtof(argv[3]),-1,1);
      else
   if (!dStricmp(argv[2],"y"))
      move.y = mClampF(dAtof(argv[3]),-1,1);
      else
   if (!dStricmp(argv[2],"z"))
      move.z = mClampF(dAtof(argv[3]),-1,1);
      else
   if (!dStricmp(argv[2],"yaw"))
      move.yaw = moveClamp(dAtof(argv[3]));
      else
   if (!dStricmp(argv[2],"pitch"))
      move.pitch = moveClamp(dAtof(argv[3]));
      else
   if (!dStricmp(argv[2],"roll"))
      move.roll = moveClamp(dAtof(argv[3]));

   //
   object->setMove(&move);
}

ConsoleMethod(AIConnection,getMove,F32,3, 3,"(string field)"
              "Get the given field of a move.\n\n"
              "@param field One of {'x','y','z','yaw','pitch','roll'}\n"
              "@returns The requested field on the current move.")
{
   const Move& move = object->getMove();
   if (!dStricmp(argv[2],"x"))
      return move.x;
   if (!dStricmp(argv[2],"y"))
      return move.y;
   if (!dStricmp(argv[2],"z"))
      return move.z;
   if (!dStricmp(argv[2],"yaw"))
      return move.yaw;
   if (!dStricmp(argv[2],"pitch"))
      return move.pitch;
   if (!dStricmp(argv[2],"roll"))
      return move.roll;
   return 0;
}


ConsoleMethod(AIConnection,setFreeLook,void,3, 3,"(bool isFreeLook)"
              "Enable/disable freelook on the current move.")
{
   Move move = object->getMove();
   move.freeLook = dAtob(argv[2]);
   object->setMove(&move);
}

ConsoleMethod(AIConnection,getFreeLook,bool,2, 2,"getFreeLook()"
              "Is freelook on for the current move?")
{
   return object->getMove().freeLook;
}


//-----------------------------------------------------------------------------

ConsoleMethod(AIConnection,setTrigger,void,4, 4,"(int trigger, bool set)"
              "Set a trigger.")
{
   S32 idx = dAtoi(argv[2]);
   if (idx >= 0 && idx < MaxTriggerKeys)  {
      Move move = object->getMove();
      move.trigger[idx] = dAtob(argv[3]);
      object->setMove(&move);
   }
}

ConsoleMethod(AIConnection,getTrigger,bool,4, 4,"(int trigger)"
              "Is the given trigger set?")
{
   S32 idx = dAtoi(argv[2]);
   if (idx >= 0 && idx < MaxTriggerKeys)
      return object->getMove().trigger[idx];
   return false;
}


//-----------------------------------------------------------------------------

ConsoleMethod(AIConnection,getAddress,const char*,2, 2,"")
{
   // Override the netConnection method to return to indicate
   // this is an ai connection.
   return "ai:local";
}
