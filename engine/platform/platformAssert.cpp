//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "platform/platformAssert.h"
#include "console/console.h"
#include <stdarg.h>

//-------------------------------------- STATIC Declaration
PlatformAssert *PlatformAssert::platformAssert = NULL;

//--------------------------------------
PlatformAssert::PlatformAssert()
{
   processing = false;
}

//--------------------------------------
PlatformAssert::~PlatformAssert()
{
}

//--------------------------------------
void PlatformAssert::create( PlatformAssert* newAssertClass )
{
   if (!platformAssert)
      platformAssert = newAssertClass ? newAssertClass : new PlatformAssert;
}


//--------------------------------------
void PlatformAssert::destroy()
{
   if (platformAssert)
      delete platformAssert;
   platformAssert = NULL;
}


//--------------------------------------
bool PlatformAssert::displayMessageBox(const char *title, const char *message, bool retry)
{
   if (retry)
      return Platform::AlertRetry(title, message);

   Platform::AlertOK(title, message);
   return false;
}

static char *typeName[] = { "Unknown", "Fatal-ISV", "Fatal", "Warning" };

//--------------------------------------

bool PlatformAssert::process(Type         assertType,
                             const char  *filename,
                             U32          lineNumber,
                             const char  *message)
{
   processing = true;

   // always dump to the Assert to the Console
   if (Con::isActive())
   {
      if (assertType == Warning)
	      Con::warnf(ConsoleLogEntry::Assert, "%s: (%s @ %ld) %s", typeName[assertType], filename, lineNumber, message);
      else
	      Con::errorf(ConsoleLogEntry::Assert, "%s: (%s @ %ld) %s", typeName[assertType], filename, lineNumber, message);
   }

   // if not a WARNING pop-up a dialog box
   if (assertType != Warning)
   {
      // used for processing navGraphs (an assert won't botch the whole build)
      if(Con::getBoolVariable("$FP::DisableAsserts", false) == true)
         Platform::forceShutdown(1);

      char buffer[2048];
      dSprintf(buffer, 2048, "%s: (%s @ %ld)", typeName[assertType], filename, lineNumber);

#ifdef TORQUE_DEBUG
      // In debug versions, allow a retry even for ISVs...
      bool retry = displayMessageBox(buffer, message, true);
#else
      bool retry = displayMessageBox(buffer, message, ((assertType == Fatal) ? true : false) );
#endif
      if (retry)
         Platform::debugBreak();
      else
         Platform::forceShutdown(1);
   }
   processing = false;

   return true;
}

bool PlatformAssert::processingAssert()
{
   return platformAssert ? platformAssert->processing : false;
}

//--------------------------------------
bool PlatformAssert::processAssert(Type        assertType,
                                   const char  *filename,
                                   U32         lineNumber,
                                   const char  *message)
{
   if (platformAssert)
      return platformAssert->process(assertType, filename, lineNumber, message);
   else // when platAssert NULL (during _start/_exit) try direct output...
      dPrintf("\n%s: (%s @ %ld) %s\n", typeName[assertType], filename, lineNumber, message);

   // this could also be platform-specific: OutputDebugString on PC, DebugStr on Mac.
   // Will raw printfs do the job?  In the worst case, it's a break-pointable line of code.
   // would have preferred Con but due to race conditions, it might not be around...
   // Con::errorf(ConsoleLogEntry::Assert, "%s: (%s @ %ld) %s", typeName[assertType], filename, lineNumber, message);

   return true;
}

//--------------------------------------
const char* avar(const char *message, ...)
{
   static char buffer[4096];
   va_list args;
   va_start(args, message);
   dVsprintf(buffer, sizeof(buffer), message, args);
   return( buffer );
}
