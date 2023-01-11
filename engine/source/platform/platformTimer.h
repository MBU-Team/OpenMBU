//-----------------------------------------------------------------------------
// Torque Game Engine Advanced
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _PLATFORM_PLATFORMTIMER_H_
#define _PLATFORM_PLATFORMTIMER_H_

#include "platform/platform.h"
#include "util/journal/journaledSignal.h"

/// Platform-specific timer class.
///
/// This exists primarily as support for the TimeManager, but may be useful
/// elsewhere.
class PlatformTimer
{
protected:
   PlatformTimer();
public:
   virtual ~PlatformTimer();
   
   /// Get the number of MS that have elapsed since creation or the last
   /// reset call.
   virtual const S32 getElapsedMs()=0;
   
   /// Reset elapsed ms back to zero.
   virtual void reset()=0;
   
   /// Create a new PlatformTimer.
   static PlatformTimer *create();
};

/// Utility class to fire journalled time-delta events at regular intervals.
///
/// Most games and simulations need the ability to update their state based on
/// a time-delta. However, tracking time accurately and sending out well-conditioned
/// events (for instance, allowing no events with delta=0) tends to be platform
/// specific. This class provides an abstraction around this platform mojo.
///
/// In addition, a well behaved application may want to alter how frequently
/// it processes time advancement depending on its execution state. For instance,
/// a game running in the background can significantly reduce CPU usage
/// by only updating every 100ms, instead of trying to maintain a 1ms update
/// update rate.
class TimeManager
{
   PlatformTimer *mTimer;
   S32 mForegroundThreshold, mBackgroundThreshold;
   bool mBackground;
   
   void _updateTime();

public:

   TimeManagerEvent timeEvent;
   
   TimeManager();   
   ~TimeManager();
   
   void setForegroundThreshold(const S32 msInterval);
   const S32 getForegroundThreshold() const;
   
   void setBackgroundThreshold(const S32 msInterval);
   const S32 getBackgroundThreshold() const;

   void setBackground(const bool isBackground) { mBackground = isBackground; };
   const bool getBackground() const { return mBackground; };

};

#endif