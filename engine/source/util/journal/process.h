//-----------------------------------------------------------------------------
// Torque Game Engine Advanced
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _UTIL_JOURNAL_PROCESS_H_
#define _UTIL_JOURNAL_PROCESS_H_

#include "platform/platform.h"
#include "core/tVector.h"
#include "util/delegate.h"
#include "util/tSignal.h"


#define PROCESS_FIRST_ORDER 0.0f
#define PROCESS_NET_ORDER 0.35f
#define PROCESS_INPUT_ORDER 0.4f
#define PROCESS_DEFAULT_ORDER 0.5f
#define PROCESS_TIME_ORDER 0.75f
#define PROCESS_RENDER_ORDER 0.8f
#define PROCESS_LAST_ORDER 1.0f

class StandardMainLoop;


/// Event generation signal.
///
/// Objects that generate events need to register a callback with
/// this signal and should only generate events from within the callback.
///
/// This signal is triggered from the ProcessEvents() method.
class Process
{
public:
   /// Trigger the ProcessSignal and replay journal events.
   ///
   /// The ProcessSignal is triggered during which all events are generated,
   /// journaled, and delivered using the EventSignal classes. Event producers should
   /// only generate events from within the function they register with ProcessSignal.
   /// ProcessSignal is also triggered during event playback, though all new events are
   /// thrown away so as not to interfere with journal playback.
   /// This function returns false if Process::requestShutdown() has been called, otherwise it
   /// returns true.
   ///
   /// NOTE: This should only be called from main loops - it should really be private,
   ///  but we need to sort out how to handle the unit test cases
   static bool processEvents();

   /// Ask the processEvents() function to shutdown.
   static void requestShutdown();


   template <class T>
   static void notifyInit(T func, F32 order = PROCESS_DEFAULT_ORDER) 
   {
      get()._signalInit.notify(func,order);
   }


   template <class T>
   static void notifyCommandLine(T func, F32 order = PROCESS_DEFAULT_ORDER) 
   {
      get()._signalCommandLine.notify(func,order);
   }


   template <class T>
   static void notify(T func, F32 order = PROCESS_DEFAULT_ORDER) 
   {
      get()._signalProcess.notify(func,order);
   }

   template <class T,class U>
   static void notify(T obj,U func, F32 order = PROCESS_DEFAULT_ORDER) 
   {
      get()._signalProcess.notify(obj,func,order);
   }

   template <class T>
   static void remove(T func) 
   {
      get()._signalProcess.remove(func);
   }

   template <class T,class U>
   static void remove(T obj,U func) 
   {
      get()._signalProcess.remove(obj,func);
   }


   template <class T>
   static void notifyShutdown(T func, F32 order = PROCESS_DEFAULT_ORDER) 
   {
      get()._signalShutdown.notify(func,order);
   }
   /// Trigger the registered init functions
   static void init();

private:

   /// Trigger the registered command line handling functions
   static void handleCommandLine(S32 argc, const char **argv);

   /// Trigger the registered shutdown functions
   static void shutdown();

   /// Private constructor
   Process();

   /// Access method will construct the singleton as necessary
   static Process  &get();


   static Process *_theOneProcess;  ///< the one instance of the Process class

   Signal<>                   _signalInit;
   Signal<S32, const char **> _signalCommandLine;
   Signal<>                   _signalProcess;
   Signal<>                   _signalShutdown;

   bool _RequestShutdown;
};

/// Register an initialization function
///   To use this, declare a static var like this:
///      static ProcessRegisterInit sgInit(InitFoo);
class ProcessRegisterInit
{
public:
   template <class T>
   ProcessRegisterInit( T func, F32 order = PROCESS_DEFAULT_ORDER )
   {
      Process::notifyInit( func, order );
   }
};

/// Register a command line handling function
///   To use this, declare a static var like this:
///      static ProcessRegisterCommandLine sgCommandLine(HandleCommandLine);
class ProcessRegisterCommandLine
{
public:
   template <class T>
   ProcessRegisterCommandLine( T func, F32 order = PROCESS_DEFAULT_ORDER )
   {
      Process::notifyCommandLine( func, order );
   }
};

/// Register a processing function
///   To use this, declare a static var like this:
///      static ProcessRegisterProcessing sgProcess(ProcessFoo);
class ProcessRegisterProcessing
{
public:
   template <class T>
   ProcessRegisterProcessing( T func, F32 order = PROCESS_DEFAULT_ORDER )
   {
      Process::notify( func, order );
   }
};

/// Register a shutdown function
///   To use this, declare a static var like this:
///      static ProcessRegisterShutdown sgShutdown(ShutdownFoo);
class ProcessRegisterShutdown
{
public:
   template <class T>
   ProcessRegisterShutdown( T func, F32 order = PROCESS_DEFAULT_ORDER )
   {
      Process::notifyShutdown( func, order );
   }
};
///@}

#endif