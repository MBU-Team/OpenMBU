//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------



#ifndef _X86UNIXSTDCONSOLE_H_
#define _X86UNIXSTDCONSOLE_H_

#define MAX_CMDS 10
#ifndef _CONSOLE_H_
#include "console/console.h"
#endif
#ifndef _EVENT_H_
#include "platform/event.h"
#endif

#include <termios.h>

class StdConsole
{
   bool stdConsoleEnabled;
   // true if we're running in the background
   bool inBackground;

   int stdOut;
   int stdIn;
   int stdErr;
   ConsoleEvent postEvent;
   char inbuf[512];
   S32  inpos;
   bool lineOutput;
   char curTabComplete[512];
   S32  tabCompleteStart;
   char rgCmds[MAX_CMDS][512];
   S32  iCmdIndex;

   // this holds the original terminal state
   // before we messed with it
   struct termios *originalTermState;

   void printf(const char *s, ...);

public:
   StdConsole();
   virtual ~StdConsole();
   void process();
   void enable(bool);
   void processConsoleLine(const char *consoleLine);
   static void create();
   static void destroy();
   static bool isEnabled();
   void resetTerminal();
};

extern StdConsole *stdConsole;

#endif
