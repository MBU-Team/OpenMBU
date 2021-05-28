//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "PlatformMacCarb/platformMacCarb.h"
#include "PlatformMacCarb/macCarbConsole.h"
#include "Platform/event.h"
#include "Platform/gameInterface.h"

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#if USE_SIOUX
#include <SIOUX.h>
#else
#if defined(TORQUE_OS_MAC_OSX)
#include <CoreFoundation/CoreFoundation.h>
#else
#include <CoreFoundation.h>
#endif
#endif

#ifdef TORQUE_DEBUG
//#define NO_CONSOLE		1 // to turn off all output.
#define NO_CONSOLE		0
#else
#define NO_CONSOLE		0
#endif

MacConsole *gConsole = NULL;


ConsoleFunction(enableWinConsole, void, 2, 2, "(bool enable)")
{
   argc;
   if (gConsole)
      gConsole->enable(dAtob(argv[1]));
}

void MacConsole::create()
{
#if NO_CONSOLE
   gConsole = NULL;
#else
   gConsole = new MacConsole();
#endif
}

void MacConsole::destroy()
{
   if (gConsole)
      delete gConsole;
   gConsole = NULL;
}

void MacConsole::enable(bool enabled)
{
   if (gConsole == NULL) return;
   
   consoleEnabled = enabled;
   if(consoleEnabled)
   {
//      AllocConsole();
      printf("Console Initialized.\n");
      const char *title = Con::getVariable("Con::WindowTitle");
      if (title && *title)
      {
         unsigned char t2[256];
         str2p(title, t2);
#if USE_SIOUX
         SIOUXSetTitle(t2);
#endif
      }
//      stdOut = GetStdHandle(STD_OUTPUT_HANDLE);
//      stdIn  = GetStdHandle(STD_INPUT_HANDLE);
//      stdErr = GetStdHandle(STD_ERROR_HANDLE);
//
      printf("%s", Con::getVariable("Con::Prompt"));
   }
}

bool MacConsole::isEnabled()
{
   if ( gConsole )
      return gConsole->consoleEnabled;

   return false;
}

static void macConsoleConsumer(ConsoleLogEntry::Level, const char *line)
{
   if (gConsole)
      gConsole->processConsoleLine(line);
}

MacConsole::MacConsole()
{
   for (S32 iIndex = 0; iIndex < MAX_CMDS; iIndex ++)
      rgCmds[iIndex][0] = '\0';

   iCmdIndex = 0;

   consoleEnabled = false;
   
   Con::addConsumer(macConsoleConsumer);

   inpos = 0;

   // !!!!TBD when system is overhauled...
//   lineOutput = true;
   lineOutput = false;

//   Con::addVariable("MacConsoleEnabled", consoleEnableCallback, "false");

#if USE_SIOUX
   // set up the SIOUX stuff here for now.
   SIOUXSettings.standalone = false;
   SIOUXSettings.setupmenus = false;
   SIOUXSettings.initializeTB = false;
   SIOUXSettings.asktosaveonclose = false;
   SIOUXSettings.toppixel = 4 + 24; // why tie to GetMBarHeight(); ??
   SIOUXSettings.leftpixel = 4;
   SIOUXSettings.columns = 120;
   SIOUXSettings.rows = 40;
#else
   // an apple sample showed this as a way to ready a std OSX console
/*
   OSErr err;
   FSRef conAppFSRef;
   err = LSFindApplicationForInfo(kLSUnknownCreator, CFSTR("com.apple.Console"), NULL, &conAppFSRef, NULL);
   if (err == noErr)
   {
       LSOpenFSRef(&conAppFSRef, NULL);
   }
*/
#endif
}

MacConsole::~MacConsole()
{
   Con::removeConsumer(macConsoleConsumer);
}

void MacConsole::printf(const char *s, ...)
{
   int bytes;
   va_list args;
   va_start(args, s);
   vprintf(s, args);
   // XXX See the Windows and LINUX versions for control-char-stripping ideas.
   // Basically we need to stick the output in a buffer so we can operate on
   // it before printing.
}

void MacConsole::processConsoleLine(const char *consoleLine)
{
   if(consoleEnabled)
   {
      inbuf[inpos] = 0;
#if USE_SIOUX
      if (!lineOutput && inpos)
         printf("%c%s\n%s%s", '\r', consoleLine, Con::getVariable("Con::Prompt"), inbuf);
      else
#endif
         printf("%s\n", consoleLine);
   }
}


//--------------------------------------
bool MacConsole::handleEvent(const EventRecord *msg)
{
   if (consoleEnabled)
   {
#if USE_SIOUX
      if (SIOUXHandleOneEvent((EventRecord*)msg))
      {
         if (msg->what==keyDown || msg->what==autoKey)
         {
            U8 ascii = msg->message & charCodeMask;
            U8 keyCode = TranslateOSKeyCode( (msg->message & keyCodeMask) >> 8 );
            process(keyCode, ascii);
         }

         return(true);
      }
#endif
   }
   
   return(false);
}


//--------------------------------------
// on the mac, we call this after the console has consumed a keyboard event.
void MacConsole::process(U8 keyCode, U8 ascii)
{
   // for now, minimal processing.
   
   if (consoleEnabled)
   {
      char outbuf[512];
      S32 outpos = 0;
     
      switch (ascii)
      {
         // If no ASCII char, check if it's a handled virtual key
         case 28:
         case 29:
         case 30:
         case 31:
            switch (keyCode)
            {
               // UP ARROW
               case KEY_UP:
               {
                  // Go to the previous command in the cyclic array
                  if ((-- iCmdIndex) < 0)
                     iCmdIndex = MAX_CMDS - 1;

                  // If this command isn't empty ...
                  if (rgCmds[iCmdIndex][0] != '\0')
                  {
                     // Obliterate current displayed text
                     for (S32 i = outpos = 0; i < inpos; i ++)
                     {
                        outbuf[outpos ++] = '\b';
                        outbuf[outpos ++] = ' ';
                        outbuf[outpos ++] = '\b';
                     }

                     // Copy command into command and display buffers
                     for (inpos = 0; inpos < (S32)strlen(rgCmds[iCmdIndex]); inpos ++, outpos ++)
                     {
                        outbuf[outpos] = rgCmds[iCmdIndex][inpos];
                        inbuf [inpos ] = rgCmds[iCmdIndex][inpos];
                     }
                  }
                  // If previous is empty, stay on current command
                  else if ((++ iCmdIndex) >= MAX_CMDS)
                  {
                     iCmdIndex = 0;
                  }
                  
                  break;
               }

               // DOWN ARROW
               case KEY_DOWN:
               {
                  // Go to the next command in the command array, if
                  // it isn't empty
                  if (rgCmds[iCmdIndex][0] != '\0' && (++ iCmdIndex) >= MAX_CMDS)
                      iCmdIndex = 0;

                  // Obliterate current displayed text
                  for (S32 i = outpos = 0; i < inpos; i ++)
                  {
                     outbuf[outpos ++] = '\b';
                     outbuf[outpos ++] = ' ';
                     outbuf[outpos ++] = '\b';
                  }

                  // Copy command into command and display buffers
                  for (inpos = 0; inpos < (S32)strlen(rgCmds[iCmdIndex]); inpos ++, outpos ++)
                  {
                     outbuf[outpos] = rgCmds[iCmdIndex][inpos];
                     inbuf [inpos ] = rgCmds[iCmdIndex][inpos];
                  }
                  break;
               }

               // LEFT ARROW
               case KEY_LEFT:
                  break;

               // RIGHT ARROW
               case KEY_RIGHT:
                  break;

               default :
                  break;
            }
            break;

         // backspace???
         case '\b':
            if(inpos > 0)
            {
               outbuf[outpos++] = '\b';
               outbuf[outpos++] = ' ';
               outbuf[outpos++] = '\b';
               inpos--;
            }
            break;

         case '\t':
            // In the output buffer, we're going to have to erase the current line (in case
            // we're cycling through various completions) and write out the whole input
            // buffer, so (inpos * 3) + complen <= 512.  Should be OK.  The input buffer is
            // also 512 chars long so that constraint will also be fine for the input buffer.
            {
               // Erase the current line.
               U32 i;
               for (i = 0; i < inpos; i++) {
                  outbuf[outpos++] = '\b';
                  outbuf[outpos++] = ' ';
                  outbuf[outpos++] = '\b';
               }
               // Modify the input buffer with the completion.
               U32 maxlen = 512 - (inpos * 3);
               if (GetCurrentKeyModifiers() & shiftKeyBit) {
                  inpos = Con::tabComplete(inbuf, inpos, maxlen, false);
               }
               else {
                  inpos = Con::tabComplete(inbuf, inpos, maxlen, true);
               }

               // Copy the input buffer to the output.
               for (i = 0; i < inpos; i++) {
                  outbuf[outpos++] = inbuf[i];
               }
            }
            break;

         // some kind of CR/LF
         case '\n':
         case '\r':
//          outbuf[outpos++] = '\r';
            outbuf[outpos++] = '\n';

            inbuf[inpos] = 0;
            outbuf[outpos] = 0;
            
            // for moment, don't echo final input, as SIOUX seems to be...
//            printf("%s", inbuf);

            S32 eventSize;
            eventSize = ConsoleEventHeaderSize;
                
            dStrcpy(postEvent.data, inbuf);
            postEvent.size = eventSize + dStrlen(inbuf) + 1;
            Game->postEvent(postEvent);

            // If we've gone off the end of our array, wrap
            // back to the beginning
            if (iCmdIndex >= MAX_CMDS)
                iCmdIndex %= MAX_CMDS;

            // Put the new command into the array
            strcpy(rgCmds[iCmdIndex ++], inbuf);

            printf("%s", Con::getVariable("Con::Prompt"));
            inpos = outpos = 0;
            break;

         default:
            inbuf[inpos++] = ascii;
            outbuf[outpos++] = ascii;
            break;
      }

      if (outpos)
      {
         outbuf[outpos] = 0;
         printf("%s", outbuf);
      }
   }
}
