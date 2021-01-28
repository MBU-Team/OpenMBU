//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _GAMEINTERFACE_H_
#define _GAMEINTERFACE_H_

class FileStream;

class GameInterface
{
   enum JournalMode {
      JournalOff,
      JournalSave,
      JournalPlay,
   };
   JournalMode mJournalMode;
   bool mRunning;
   bool mJournalBreak;
public:
   GameInterface();

   /// @name Platform Interface
   /// The platform calls these functions to control execution of the game.
   /// @{
   virtual int main(int argc, const char **argv);
   virtual void textureKill();
   virtual void textureResurrect();
   virtual void refreshWindow();

   virtual void postEvent(Event &event);
   /// @}

   /// @name Event Handlers
   /// default event behavior with journaling support
   /// default handler forwards events to appropriate routines
   /// @{
   virtual void processEvent(Event *event);

   virtual void processPacketReceiveEvent(PacketReceiveEvent *event);
   virtual void processMouseMoveEvent(MouseMoveEvent *event);
   virtual void processInputEvent(InputEvent *event);
   virtual void processQuitEvent();
   virtual void processTimeEvent(TimeEvent *event);
   virtual void processConsoleEvent(ConsoleEvent *event);
   virtual void processConnectedAcceptEvent(ConnectedAcceptEvent *event);
   virtual void processConnectedReceiveEvent(ConnectedReceiveEvent *event);
   virtual void processConnectedNotifyEvent(ConnectedNotifyEvent *event);
   /// @}

   /// @name Running
   /// @{
   void setRunning(bool running) { mRunning = running; }
   bool isRunning() { return mRunning; }
   /// @}

   /// @name Journaling
   ///
   /// Journaling is used in order to make a "demo" of the actual game.  It logs
   /// all processes that happen throughout code execution (NOT script).  This is
   /// very handy for debugging.  Say an end user finds a crash in the program.
   /// The user can start up the engine with journal recording enabled, reproduce
   /// the crash, then send in the journal file to the development team.  The
   /// development team can then play back the journal file and see exactly what
   /// happened to cause the crash.  This will result in the ability to run the
   /// program through the debugger to easily track what went wrong and easily
   /// create a stack trace.
   ///
   /// Actually enabling journaling may be different in different distributions
   /// if the developers decided to change how it works.  However, by default,
   /// run the program with the "-jSave filename" command argument.  The filename
   /// does not need an extension, and only requires write access.  If the file
   /// does not exist, it will be created.  In order to play back a journal,
   /// use the "-jPlay filename" command argument, and just watch the magic happen.
   /// Examples:
   /// @code
   /// torqueDemo_DEBUG.exe -jSave crash
   /// torqueDemo_DEBUG.exe -jPlay crash
   /// @endcode
   /// @{

   /// If we're doing a journal playback, this function is responsible for reading
   /// events from the journal file and dispatching them.
   void journalProcess();

   /// Start loading journal data from the specified file.
   void loadJournal(const char *fileName);

   /// Start saving journal data to the specified file (must be able to write it).
   void saveJournal(const char *fileName);

   /// Play back the specified journal.
   ///
   /// @param  fileName       Journal file to play back.
   /// @param  journalBreak   Should we break execution after we're done?
   void playJournal(const char *fileName, bool journalBreak = false);

   JournalMode getJournalMode() { return mJournalMode; };

   /// Are we reading back from the journal?
   bool isJournalReading() { return mJournalMode == JournalPlay; }

   /// Are we writing to the journal?
   bool isJournalWriting() { return mJournalMode == JournalSave; }

   void journalRead(U32 *val);                     ///< Read a U32 from the journal.
   void journalWrite(U32 val);                     ///< Write a U32 to the journal.
   void journalRead(U32 size, void *buffer);       ///< Read a block of data from the journal.
   void journalWrite(U32 size, const void *buffer);///< Write a block of data to the journal.

   FileStream *getJournalStream();
   /// @}
};

/// Global game instance.
extern GameInterface *Game;

#endif
