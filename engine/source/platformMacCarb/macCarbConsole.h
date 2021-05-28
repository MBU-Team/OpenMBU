//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _MACCARBCONSOLE_H_
#define _MACCARBCONSOLE_H_

#define MAX_CMDS 10
#ifndef _CONSOLE_H_
#include "console/console.h"
#endif
#ifndef _EVENT_H_
#include "Platform/event.h"
#endif

// this is where we determine whether to try and use raw STDIO, or whether
// we try to base off of an external console library.  SIOUX in MWERKS case.
#if __MWERKS__
#ifndef __MACH__
#define USE_SIOUX		1
#endif
#endif


class MacConsole
{
   bool consoleEnabled;

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
   static void create();
   static void destroy();
   static bool isEnabled();

   MacConsole();
   ~MacConsole();
   void enable(bool);

   void processConsoleLine(const char *consoleLine);

   bool handleEvent(const EventRecord *msg);
   void process(U8 keyCode, U8 ascii);
};

extern MacConsole *gConsole;

#endif
