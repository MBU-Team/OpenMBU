//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "platform/platform.h"
#include "platform/event.h"
#include "platform/gameInterface.h"
#include "core/fileStream.h"
#include "console/console.h"

GameInterface *Game = NULL;

GameInterface::GameInterface()
{
   AssertFatal(Game == NULL, "ERROR: Multiple games declared.");
   Game = this;
   mJournalMode = JournalOff;
   mRunning = true;
}

int GameInterface::main(int, const char**)
{
   return(0);
}

void GameInterface::textureKill()
{

}

void GameInterface::textureResurrect()
{

}

void GameInterface::refreshWindow()
{

}

static U32 sReentrantCount = 0;

void GameInterface::processEvent(Event *event)
{
   if(!mRunning)
      return;

   if(PlatformAssert::processingAssert()) // ignore any events if an assert dialog is up.
      return;

#ifdef TORQUE_DEBUG
   sReentrantCount++;
   AssertFatal(sReentrantCount == 1, "Error! ProcessEvent is NOT reentrant.");
#endif

   switch(event->type)
   {
      case PacketReceiveEventType:
         processPacketReceiveEvent((PacketReceiveEvent *) event);
         break;
      case MouseMoveEventType:
         processMouseMoveEvent((MouseMoveEvent *) event);
         break;
      case InputEventType:
         processInputEvent((InputEvent *) event);
         break;
      case QuitEventType:
         processQuitEvent();
         break;
      case TimeEventType:
         processTimeEvent((TimeEvent *) event);
         break;
      case ConsoleEventType:
         processConsoleEvent((ConsoleEvent *) event);
         break;
      case ConnectedAcceptEventType:
         processConnectedAcceptEvent( (ConnectedAcceptEvent *) event );
         break;
      case ConnectedReceiveEventType:
         processConnectedReceiveEvent( (ConnectedReceiveEvent *) event );
         break;
      case ConnectedNotifyEventType:
         processConnectedNotifyEvent( (ConnectedNotifyEvent *) event );
         break;
   }

#ifdef TORQUE_DEBUG
   sReentrantCount--;
#endif

}


void GameInterface::processPacketReceiveEvent(PacketReceiveEvent*)
{

}

void GameInterface::processMouseMoveEvent(MouseMoveEvent*)
{

}

void GameInterface::processInputEvent(InputEvent*)
{

}

void GameInterface::processQuitEvent()
{
}

void GameInterface::processTimeEvent(TimeEvent*)
{
}

void GameInterface::processConsoleEvent(ConsoleEvent*)
{

}

void GameInterface::processConnectedAcceptEvent(ConnectedAcceptEvent*)
{

}

void GameInterface::processConnectedReceiveEvent(ConnectedReceiveEvent*)
{

}

void GameInterface::processConnectedNotifyEvent(ConnectedNotifyEvent*)
{

}

struct ReadEvent : public Event
{
   U8 data[3072];
};

FileStream gJournalStream;

void GameInterface::postEvent(Event &event)
{
   if(mJournalMode == JournalPlay && event.type != QuitEventType)
      return;
   if(mJournalMode == JournalSave)
   {
      gJournalStream.write(event.size, &event);
      gJournalStream.flush();
   }
   processEvent(&event);
}

void GameInterface::journalProcess()
{
   if(mJournalMode == JournalPlay)
   {
      ReadEvent journalReadEvent;
// used to be:
//      if(gJournalStream.read(&journalReadEvent.type))
//        if(gJournalStream.read(&journalReadEvent.size))
// for proper non-endian stream handling, the read-ins should match the write-out by using bytestreams read:
      if(gJournalStream.read(sizeof(Event), &journalReadEvent))
      {
         if(gJournalStream.read(journalReadEvent.size - sizeof(Event), &journalReadEvent.data))
         {
            if(gJournalStream.getPosition() == gJournalStream.getStreamSize() && mJournalBreak)
               Platform::debugBreak();
            processEvent(&journalReadEvent);
            return;
         }
      }
      // JournalBreak is used for debugging, so halt all game
      // events if we get this far.
      if(mJournalBreak)
         mRunning = false;
      else
         mJournalMode = JournalOff;
   }
}

void GameInterface::saveJournal(const char *fileName)
{
   mJournalMode = JournalSave;
   gJournalStream.open(fileName, FileStream::Write);
}

void GameInterface::playJournal(const char *fileName,bool journalBreak)
{
   mJournalMode = JournalPlay;
   mJournalBreak = journalBreak;
   gJournalStream.open(fileName, FileStream::Read);
}

FileStream *GameInterface::getJournalStream()
{
   return &gJournalStream;
}

void GameInterface::journalRead(U32 *val)
{
   gJournalStream.read(val);
}

void GameInterface::journalWrite(U32 val)
{
   gJournalStream.write(val);
}

void GameInterface::journalRead(U32 size, void *buffer)
{
   gJournalStream.read(size, buffer);
}

void GameInterface::journalWrite(U32 size, const void *buffer)
{
   gJournalStream.write(size, buffer);
}

