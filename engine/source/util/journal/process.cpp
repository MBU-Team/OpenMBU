//-----------------------------------------------------------------------------
// Torque Game Engine Advanced
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "util/journal/process.h"
#include "util/journal/journal.h"


Process   *Process::_theOneProcess = NULL;

//-----------------------------------------------------------------------------

void  Process::requestShutdown()
{
   Process::get()._RequestShutdown = true;
}

//-----------------------------------------------------------------------------

Process::Process()
:  _RequestShutdown( false )
{
}

Process  &Process::get()
{
   // NOTE that this function is not thread-safe
   //    To make it thread safe, use the double-checked locking mechanism for singleton objects

   if ( _theOneProcess == NULL )
      _theOneProcess = new Process;

   return *_theOneProcess;
}

void  Process::init()
{
   Process::get()._signalInit.trigger();
}

void  Process::handleCommandLine(S32 argc, const char **argv)
{
   Process::get()._signalCommandLine.trigger(argc, argv);
}

bool  Process::processEvents()
{
   // Process all the devices. We need to call these even during journal
   // playback to ensure that the OS event queues are serviced.
   Process::get()._signalProcess.trigger();

   if (!Process::get()._RequestShutdown) 
   {
      if (Journal::IsPlaying())
         return Journal::PlayNext();
      return true;
   }

   // Reset the Quit flag so the function can be called again.
   Process::get()._RequestShutdown = false;
   return false;
}

void  Process::shutdown()
{
   Process::get()._signalShutdown.trigger();
}

