//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "platformWin32/platformWin32.h"
#include "time.h"

void Platform::sleep(U32 ms)
{
   Sleep(ms);
}

//--------------------------------------
void Platform::getLocalTime(LocalTime &lt)
{
   struct tm *systime;
   time_t long_time;

   time( &long_time );                // Get time as long integer.
   systime = localtime( &long_time ); // Convert to local time.

   lt.sec      = systime->tm_sec;
   lt.min      = systime->tm_min;
   lt.hour     = systime->tm_hour;
   lt.month    = systime->tm_mon;
   lt.monthday = systime->tm_mday;
   lt.weekday  = systime->tm_wday;
   lt.year     = systime->tm_year;
   lt.yearday  = systime->tm_yday;
   lt.isdst    = systime->tm_isdst;
}

U32 Platform::getTime()
{
   time_t long_time;
   time( &long_time );
   return long_time;
}

void Platform::fileToLocalTime(const FileTime & ft, LocalTime * lt)
{
   if(!lt)
      return;

   dMemset(lt, 0, sizeof(LocalTime));

   FILETIME winFileTime;
   winFileTime.dwLowDateTime = ft.v1;
   winFileTime.dwHighDateTime = ft.v2;

   SYSTEMTIME winSystemTime;

   // convert the filetime to local time
   FILETIME convertedFileTime;
   if(::FileTimeToLocalFileTime(&winFileTime, &convertedFileTime))
   {
      // get the time into system time struct
      if(::FileTimeToSystemTime((const FILETIME *)&convertedFileTime, &winSystemTime))
      {
         SYSTEMTIME * time = &winSystemTime;

         // fill it in...
         lt->sec = time->wSecond;
         lt->min = time->wMinute;
         lt->hour = time->wHour;
         lt->month = time->wMonth;
         lt->monthday = time->wDay;
         lt->weekday = time->wDayOfWeek;
         lt->year = (time->wYear < 1900) ? 1900 : (time->wYear - 1900);

         // not calculated
         lt->yearday = 0;
         lt->isdst = false;
      }
   }
}

U32 Platform::getRealMilliseconds()
{
   return GetTickCount();
}

U32 Platform::getVirtualMilliseconds()
{
   return winState.currentTime;
}

void Platform::advanceTime(U32 delta)
{
   winState.currentTime += delta;
}

