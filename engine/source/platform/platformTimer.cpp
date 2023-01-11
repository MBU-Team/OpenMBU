//-----------------------------------------------------------------------------
// Torque Game Engine Advanced
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "platform/platformTimer.h"
#include "util/journal/process.h"

void TimeManager::_updateTime()
{
   // Calculate & filter time delta since last event.

   // How long since last update?
   S32 delta = mTimer->getElapsedMs();

   // Now - we want to try to sleep until the time threshold will hit.
   S32 msTillThresh = (mBackground ? mBackgroundThreshold : mForegroundThreshold) - delta;

   if(msTillThresh > 0)
   {
      // There's some time to go, so let's sleep.
      Platform::sleep( msTillThresh );
   }

   // Ok - let's grab the new elapsed and send that out.
   S32 finalDelta = mTimer->getElapsedMs();
   mTimer->reset();

   timeEvent.trigger(finalDelta);
}

TimeManager::TimeManager()
{
   mBackground = false;
   mTimer = PlatformTimer::create();
   Process::notify(this, &TimeManager::_updateTime, PROCESS_TIME_ORDER);
   
   mForegroundThreshold = 5;
   mBackgroundThreshold = 10;
}

TimeManager::~TimeManager()
{
   Process::remove(this, &TimeManager::_updateTime);
   delete mTimer;
}

void TimeManager::setForegroundThreshold(const S32 msInterval)
{
   AssertFatal(msInterval > 0, "TimeManager::setForegroundThreshold - should have at least 1 ms between time events to avoid math problems!");
   mForegroundThreshold = msInterval;
}

const S32 TimeManager::getForegroundThreshold() const
{
   return mForegroundThreshold;
}

void TimeManager::setBackgroundThreshold(const S32 msInterval)
{
   AssertFatal(msInterval > 0, "TimeManager::setBackgroundThreshold - should have at least 1 ms between time events to avoid math problems!");
   mBackgroundThreshold = msInterval;
}

const S32 TimeManager::getBackgroundThreshold() const
{
   return mBackgroundThreshold;
}

//----------------------------------------------------------------------------------

class DefaultPlatformTimer : public PlatformTimer
{
   S32 mLastTime, mNextTime;
   
public:
   DefaultPlatformTimer()
   {
      mLastTime = mNextTime = Platform::getRealMilliseconds();
   }
   
   const S32 getElapsedMs()
   {
      mNextTime = Platform::getRealMilliseconds();
      return (mNextTime - mLastTime);
   }
   
   void reset()
   {
      mLastTime = mNextTime;
   }
};

//----------------------------------------------------------------------------------

#pragma message("Mac/Lunix will need to implement this or get unresolved externals.")
#pragma message(" It was previously defined here with a Win32 ifdef which goes against")
#pragma message(" how torque implements its platform agnostic systems - JDD")
//PlatformTimer *PlatformTimer::create()
//{
//   return new DefaultPlatformTimer();
//}

PlatformTimer::PlatformTimer()
{
}

PlatformTimer::~PlatformTimer()
{
}

