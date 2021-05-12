//-----------------------------------------------------------------------------
// Torque Game Engine Advanced
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "platform/platform.h"

#include "sfx/sfxDevice.h"


SFXDeviceEventSignal SFXDevice::smEventSignal;


SFXDevice::SFXDevice( SFXProvider* provider, bool useHardware, S32 maxBuffers )
   :  mProvider( provider ),
      mUseHardware( useHardware ),
      mMaxBuffers( maxBuffers )
{
   AssertFatal( provider, "We must have a provider pointer on device creation!" );
}
