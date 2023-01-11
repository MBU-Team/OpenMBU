//-----------------------------------------------------------------------------
// Torque Game Engine Advanced
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

// Grab the win32 headers so we can access QPC
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include "platform/platformTimer.h"
#include "math/mMath.h"

class Win32Timer : public PlatformTimer
{
private:
   U32 mTickCountCurrent;
   U32 mTickCountNext;
   S64 mPerfCountCurrent;
   S64 mPerfCountNext;
   S64 mFrequency;
   F64 mPerfCountRemainderCurrent;
   F64 mPerfCountRemainderNext;
   bool mUsingPerfCounter;
public:

   Win32Timer()
   {
      mPerfCountRemainderCurrent = 0.0f;

      // Attempt to use QPC for high res timing, otherwise fallback to GTC.
      mUsingPerfCounter = QueryPerformanceFrequency((LARGE_INTEGER *) &mFrequency);
      if(mUsingPerfCounter)
         mUsingPerfCounter = QueryPerformanceCounter((LARGE_INTEGER *) &mPerfCountCurrent);
      if(!mUsingPerfCounter)
         mTickCountCurrent = GetTickCount();
   }

   const S32 getElapsedMs()
   {
      if(mUsingPerfCounter)
      {
         // Use QPC, update remainders so we don't leak time, and return the elapsed time.
         QueryPerformanceCounter( (LARGE_INTEGER *) &mPerfCountNext);
         F64 elapsedF64 = (1000.0 * F64(mPerfCountNext - mPerfCountCurrent) / F64(mFrequency));
         elapsedF64 += mPerfCountRemainderCurrent;
         U32 elapsed = (U32)mFloor(elapsedF64);
         mPerfCountRemainderNext = elapsedF64 - F64(elapsed);

         return elapsed;
      }
      else
      {
         // Do something naive with GTC.
         mTickCountNext = GetTickCount();
         return mTickCountNext - mTickCountCurrent;
      }
   }

   void reset()
   {
      // Do some simple copying to reset the timer to 0.
      mTickCountCurrent = mTickCountNext;
      mPerfCountCurrent = mPerfCountNext;
      mPerfCountRemainderCurrent = mPerfCountRemainderNext;
   }
};

PlatformTimer *PlatformTimer::create()
{
   return new Win32Timer();
}
