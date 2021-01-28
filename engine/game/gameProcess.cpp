//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "core/dnet.h"
#include "game/gameConnection.h"
#include "game/gameBase.h"
#include "game/shapeBase.h"
#include "platform/profiler.h"
#include "console/consoleTypes.h"

//----------------------------------------------------------------------------

ProcessList gClientProcessList(false);
ProcessList gServerProcessList(true);

bool ProcessList::mDebugControlSync = false;

ProcessList::ProcessList(bool isServer)
{
   mDirty = false;
   mCurrentTag = 0;
   mLastTick = 0;
   mLastTime = 0;
   mLastDelta = 0;
   mIsServer = isServer;
//   Con::addVariable("debugControlSync",TypeBool, &mDebugControlSync);
}


//----------------------------------------------------------------------------

void ProcessList::orderList()
{
   // GameBase tags are intialized to 0, so current tag
   // should never be 0.
   if (!++mCurrentTag)
      mCurrentTag++;

   // Install a temporary head node
   GameBase list;
   list.plLinkBefore(head.mProcessLink.next);
   head.plUnlink();

   // Reverse topological sort into the orignal head node
   while (list.mProcessLink.next != &list) {
      GameBase* ptr = list.mProcessLink.next;
      ptr->mProcessTag = mCurrentTag;
      ptr->plUnlink();
      if (ptr->mAfterObject) {
         // Build chain "stack" of dependant objects and patch
         // it to the end of the current list.
         while (bool(ptr->mAfterObject) &&
               ptr->mAfterObject->mProcessTag != mCurrentTag) {
            ptr->mAfterObject->mProcessTag = mCurrentTag;
            ptr->mAfterObject->plUnlink();
            ptr->mAfterObject->plLinkBefore(ptr);
            ptr = ptr->mAfterObject;
         }
         ptr->plJoin(&head);
      }
      else
         ptr->plLinkBefore(&head);
   }
   mDirty = false;
}


//----------------------------------------------------------------------------

bool ProcessList::advanceServerTime(SimTime timeDelta)
{
   PROFILE_START(AdvanceServerTime);

   if (mDirty) orderList();

   SimTime targetTime = mLastTime + timeDelta;
   SimTime targetTick = targetTime & ~TickMask;
   SimTime tickCount = (targetTick - mLastTick) >> TickShift;

   bool ret = mLastTick != targetTick;
   // Advance all the objects
   for (; mLastTick != targetTick; mLastTick += TickMs)
      advanceObjects();

   // Credit all the connections with the elapsed ticks.
   SimGroup *g = Sim::getClientGroup();
   for (SimGroup::iterator i = g->begin(); i != g->end(); i++)
      if (GameConnection *t = dynamic_cast<GameConnection *>(*i))
         t->incMoveCredit(tickCount);

   mLastTime = targetTime;
   PROFILE_END();
   return ret;
}


//----------------------------------------------------------------------------

bool ProcessList::advanceClientTime(SimTime timeDelta)
{
   PROFILE_START(AdvanceClientTime);

   if (mDirty) orderList();

   SimTime targetTime = mLastTime + timeDelta;
   SimTime targetTick = (targetTime + TickMask) & ~TickMask;
   SimTime tickCount = (targetTick - mLastTick) >> TickShift;

   // See if the control object has pending moves.
   GameBase* control = 0;
   GameConnection* connection = GameConnection::getConnectionToServer();
   if (connection)
   {
      // If the connection to the server is backlogged
      // the simulation is frozen.
      if (connection->isBacklogged()) {
         mLastTime = targetTime;
         mLastTick = targetTick;
         PROFILE_END();
         return false;
      }
      if (connection->areMovesPending())
         control = connection->getControlObject();
   }

   // If we are going to tick, or have moves pending for the
   // control object, we need to reset everyone back to their
   // last full tick pos.
   if (mLastDelta && (tickCount || control))
      for (GameBase* obj = head.mProcessLink.next; obj != &head;
            obj = obj->mProcessLink.next)
         if (obj->mProcessTick)
            obj->interpolateTick(0);

   // Produce new moves and advance all the objects
   if (tickCount)
   {
      for (; mLastTick != targetTick; mLastTick += TickMs)
      {
         if(connection)
         {
            // process any demo blocks that are NOT moves, and exactly one move
            // we advance time in the demo stream by a move inserted on
            // each tick.  So before doing the tick processing we advance
            // the demo stream until a move is ready
            if(connection->isPlayingBack())
            {
               U32 blockType;
               do
               {
                  blockType = connection->getNextBlockType();
                  bool res = connection->processNextBlock();
                  // if there are no more blocks, exit out of this function,
                  // as no more client time needs to process right now - we'll
                  // get it all on the next advanceClientTime()
                  if(!res)
                     return true;
               } while(blockType != GameConnection::BlockTypeMove);
            }
            connection->collectMove(mLastTick);
         }
         advanceObjects();
      }
   }
   else
      if (control)
      {
         // Sync up the control object with the latest client moves.
         Move* movePtr;
         U32 m = 0, numMoves;
         connection->getMoveList(&movePtr, &numMoves);
         while (m < numMoves)
         {
            control->processTick(&movePtr[m++]);
         }
         connection->clearMoves(m);
      }

   mLastDelta = (TickMs - (targetTime & TickMask)) & TickMask;
   F32 dt = mLastDelta / F32(TickMs);
   for (GameBase* obj = head.mProcessLink.next; obj != &head;
         obj = obj->mProcessLink.next)
      if (obj->mProcessTick)
         obj->interpolateTick(dt);

   // Inform objects of total elapsed delta so they can advance
   // client side animations.
   dt = F32(timeDelta) / 1000;
   for (GameBase* obj = head.mProcessLink.next; obj != &head;
         obj = obj->mProcessLink.next)
      obj->advanceTime(dt);

   mLastTime = targetTime;
   PROFILE_END();
   return tickCount != 0;
}


//----------------------------------------------------------------------------

void ProcessList::advanceObjects()
{
   PROFILE_START(AdvanceObjects);

   // A little link list shuffling is done here to avoid problems
   // with objects being deleted from within the process method.
   GameBase list;
   GameBase* obj;
   list.plLinkBefore(head.mProcessLink.next);
   head.plUnlink();
   while ((obj = list.mProcessLink.next) != &list) {
      obj->plUnlink();
      obj->plLinkBefore(&head);

      // Each object is either advanced a single tick, or if it's
      // being controlled by a client, ticked once for each pending move.
      if (obj->mTypeMask & GameBaseObjectType) {

         GameBase *pGB = static_cast<GameBase *>(obj);
         GameConnection* con = pGB->getControllingClient();

         if (con && con->getControlObject() == pGB) {
            Move* movePtr;
            U32 m, numMoves;

            con->getMoveList(&movePtr, &numMoves);

            for (m = 0; m < numMoves && pGB->getControllingClient() == con; )
               obj->processTick(&movePtr[m++]);

            con->clearMoves(m);

            continue;
         }
      }
      if (obj->mProcessTick)
         obj->processTick(0);
   }
   PROFILE_END();
}
