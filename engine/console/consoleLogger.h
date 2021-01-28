//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "console/simBase.h"
#include "console/console.h"
#include "core/fileStream.h"

#ifndef _CONSOLE_LOGGER_H_
#define _CONSOLE_LOGGER_H_

/// A class designed to be used as a console consumer and log
/// the data it receives to a file.
class ConsoleLogger : public SimObject
{
   typedef SimObject Parent;

   private:
      bool mLogging;                   ///< True if it is currently consuming and logging
      FileStream mStream;              ///< File stream this object writes to
      static bool smInitialized;                ///< This is for use with the default constructor
      bool mAppend;                    ///< If false, it will clear the file before logging to it.
      StringTableEntry mFilename;      ///< The file name to log to.

      /// List of active ConsoleLoggers to send log messages to
      static Vector<ConsoleLogger *> mActiveLoggers;

      /// The log function called by the consumer callback
      /// @param   consoleLine   Line of text to log
      void log( const char *consoleLine );

      /// Utility function, sets up the object (for script interface) returns true if successful
      bool init();

   public:

      // @name Public console variables
      /// @{
      ConsoleLogEntry::Level mLevel;   ///< The level of log messages to log
      /// @}

      DECLARE_CONOBJECT( ConsoleLogger );

      static void initPersistFields();

      /// Console constructor
      ///
      /// @code
      /// // Example script constructor usage.
      /// %obj = new ConsoleLogger( objName, logFileName, [append = false] );
      /// @endcode
      bool processArguments( S32 argc, const char **argv );

      /// Default constructor, make sure to initalize
      ConsoleLogger();

      /// Constructor
      /// @param   fileName   File name to log to
      /// @param   append     If false, it will clear the file, then start logging, else it will append
      ConsoleLogger( const char *fileName, bool append = false );

      /// Destructor
      ~ConsoleLogger();

      /// Attach to the console and begin logging
      ///
      /// Returns true if the action is successful
      bool attach();

      /// Detach from the console and stop logging
      ///
      /// Returns true if the action is successful
      bool detach();

      /// Sets the level of console messages to log.
      ///
      /// @param   level   Log level. Only items of the specified level or
      ///                  lower are logged.
      /// @see ConsoleLogEntry::Level
      void setLogLevel( ConsoleLogEntry::Level level );

      /// Returns the level of console messages to log
      ConsoleLogEntry::Level getLogLevel() const;

      /// The callback for the console consumer
      ///
      /// @note This is a global callback, not executed per-instance.
      /// @see Con::addConsumer
      static void logCallback( ConsoleLogEntry::Level level, const char *consoleLine );
};

#endif
