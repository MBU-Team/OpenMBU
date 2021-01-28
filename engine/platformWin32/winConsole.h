//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _WINCONSOLE_H_
#define _WINCONSOLE_H_

#define MAX_CMDS 10
#ifndef _CONSOLE_H_
#include "console/console.h"
#endif
#ifndef _EVENT_H_
#include "platform/event.h"
#endif

class WinConsole
{
   bool winConsoleEnabled;

   HANDLE stdOut;
   HANDLE stdIn;
   HANDLE stdErr;
   ConsoleEvent postEvent;
   char inbuf[512];
   S32  inpos;
   bool lineOutput;
   char curTabComplete[512];
   S32  tabCompleteStart;
   char rgCmds[MAX_CMDS][512];
   S32  iCmdIndex;

   void printf(const char *s, ...);

public:
   WinConsole();
   ~WinConsole();

   void process();
   void enable(bool);
   void processConsoleLine(const char *consoleLine);
   static void create();
   static void destroy();
   static bool isEnabled();
};

extern WinConsole *WindowsConsole;

#endif
