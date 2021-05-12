//-----------------------------------------------------------------------------
// Torque Game Engine Advanced
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "precompiled.h"

#include "sfx/sfxProvider.h"
#include "sfx/fmod/sfxFMODDevice.h"
#include "util/safeRelease.h"

#include <stdio.h>

class SFXFMODProvider : public SFXProvider
{
public:

   SFXFMODProvider();
   virtual ~SFXFMODProvider();

protected:
   FModFNTable mFMod;

   struct FModDeviceInfo : SFXDeviceInfo
   {
   };

public:

   SFXDevice* createDevice( const char* deviceName, bool useHardware, S32 maxBuffers );
};

SFX_INIT_PROVIDER( SFXFMODProvider );

//------------------------------------------------------------------------------
// Helper

bool fmodBindFunction( DLibrary *dll, void *&fnAddress, const char* name )
{
   fnAddress = dll->bind( name );

   if (!fnAddress)
      Con::warnf( "FMod Loader: DLL bind failed for %s", name );

   return fnAddress != 0;
}

//------------------------------------------------------------------------------


SFXFMODProvider::SFXFMODProvider()
   : SFXProvider( "FMod" )
{
//   return;
   // Statically linked, always loaded.
   mFMod.dllRef = OsLoadLibrary("libfmodex.dylib");
   if(!mFMod.dllRef)
   {
      ::fprintf(stderr, "Failed to load libfmodex.dylib, you will not have sound!");
      return;
   }
   
   mFMod.isLoaded = true;
#define FMOD_FUNCTION(fn_name, fn_args) \
   mFMod.isLoaded &= fmodBindFunction(mFMod.dllRef, *(void**)&mFMod.fn_name, #fn_name);
#include "sfx/fmod/fmodFunctions.mac.h"
#undef FMOD_FUNCTION

   if(mFMod.isLoaded == false)
   {
      Con::errorf("SFXFMODProvider - Could not load the FMod.dll!");
      return;
   }

   // Allocate the FMod system.
   FMOD_RESULT res = mFMod.FMOD_System_Create(&SFXFMODDevice::smSystem);

   if(res != FMOD_OK)
   {
      // Failed - deal with it!
      Con::errorf("SFXFMODProvider - could not create the FMod system.");
      return;
   }

   // Check that the version is OK.
   unsigned int version;
   res = mFMod.FMOD_System_GetVersion(SFXFMODDevice::smSystem, &version);
   FModAssert(res, "SFXFMODProvider - Failed to get fmod version!");

   if(version < FMOD_VERSION)
   {
      Con::errorf("SFXFMODProvider - FMod version in DLL is too old; can't continue.");
      return;
   }

   // Now, enumerate our devices.
   int numDrivers;
   res = mFMod.FMOD_System_GetNumDrivers(SFXFMODDevice::smSystem, &numDrivers);
   FModAssert(res, "SFXFMODProvider - Failed to get driver count.");

   char nameBuff[256];

   for(S32 i=0; i<numDrivers; i++)
   {
      res = mFMod.FMOD_System_GetDriverInfo(SFXFMODDevice::smSystem, i, nameBuff, 256, NULL);
      FModAssert(res, "SFXFMODProvider - Failed to get driver name!");

      // Great - got something - so add it to the list of options.
      FModDeviceInfo *fmodInfo = new FModDeviceInfo();
      dStrncpy(fmodInfo->name, nameBuff, 256);
      fmodInfo->hasHardware = true;
      fmodInfo->maxBuffers = 32;
      fmodInfo->driver[0] = 0;

      mDeviceInfo.push_back(fmodInfo);
   }

   // Did we get any devices?
   if ( mDeviceInfo.empty() )
   {
      Con::errorf( "SFXFMODProvider - No valid devices found!" );
      return;
   }

   // Wow, we made it - register the provider.
   regProvider( this );
}

SFXFMODProvider::~SFXFMODProvider()
{

}


SFXDevice* SFXFMODProvider::createDevice( const char* deviceName, bool useHardware, S32 maxBuffers )
{
   FModDeviceInfo* info = NULL;

   // Look for the device name in the array.
   SFXDeviceInfoVector::iterator iter = mDeviceInfo.begin();
   for ( ; iter != mDeviceInfo.end(); iter++ )
   {
      if ( dStricmp( deviceName, (*iter)->name ) == 0 )
      {
         info = (FModDeviceInfo*)*iter;
         break;
      }
   }

   // If we still don't have a desc and the name 
   // is blank then use the first one.
   if ( !info && ( !deviceName || !deviceName[0] ) )
      info = (FModDeviceInfo*)mDeviceInfo[0];

   // Do we find one to create?
   if ( info )
      return new SFXFMODDevice(this, &mFMod, 0, info->name );

   // Whoops - fell through! (Couldn't create a matching device.)
   return NULL;
}
