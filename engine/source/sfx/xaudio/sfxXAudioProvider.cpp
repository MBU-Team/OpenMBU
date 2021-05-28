//-----------------------------------------------------------------------------
// Torque Game Engine Advanced
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "platformXbox/platformXbox.h"
#include "platform/platform.h"

#include "sfx/sfxProvider.h"
#include "sfx/xaudio/sfxXAudioDevice.h"

#include <xaudio.h>
//#include <x3daudio.h>

class SFXXAudioProvider : public SFXProvider
{
public:

   SFXXAudioProvider();
   virtual ~SFXXAudioProvider();

protected:
   static bool mXAInitialized; // Static because there is only one XA init call that needs to be made

   void addDeviceDesc( const char* name, const char* desc );

   X3DAUDIO_LISTENER mListener;
   X3DAUDIO_DSP_SETTINGS mDSPSettings;
   X3DAUDIO_HANDLE mX3DInstance;

public:

   SFXDevice* createDevice( const char* deviceName, bool useHardware, S32 maxBuffers );

};

SFX_INIT_PROVIDER( SFXXAudioProvider );

bool SFXXAudioProvider::mXAInitialized = false;

//------------------------------------------------------------------------------

SFXXAudioProvider::SFXXAudioProvider()
   : SFXProvider( "XAudio" )
{
   regProvider( this );

   // HAX!
   SFXDeviceInfo* info = new SFXDeviceInfo;
   dStrncpy( info->name, "SFX XBox360 Device" , sizeof( info->name ) );
   dStrncpy( info->driver, "SFX XAudio Device", sizeof( info->driver ) );
   info->hasHardware = true;
   info->maxBuffers = 8;

   mDeviceInfo.push_back( info );
}

SFXXAudioProvider::~SFXXAudioProvider()
{
   XAudioShutDown();
   mXAInitialized = false;
}

SFXDevice* SFXXAudioProvider::createDevice( const char* deviceName, bool useHardware, S32 maxBuffers )
{
   SFXDeviceInfo* info = NULL;

   // Look for the device name in the array.
   SFXDeviceInfoVector::iterator iter = mDeviceInfo.begin();
   for ( ; iter != mDeviceInfo.end(); iter++ )
   {
      if ( dStricmp( deviceName, (*iter)->name ) == 0 )
      {
         info = (SFXDeviceInfo*)*iter;
         break;
      }
   }

   // If we stil don't have a desc and the name 
   // is blank then use the first one.
   if ( !info && ( !deviceName || !deviceName[0] ) )
      info = (SFXDeviceInfo*)mDeviceInfo[0];

   // Do we find one to create?
   if ( info )
   {
      if( !mXAInitialized )
      {
         // Set up initialize parameters of XAudio Engine
         XAUDIOENGINEINIT EngineInit = { 0 };
         EngineInit.pEffectTable = &XAudioDefaultEffectTable;
         EngineInit.ThreadUsage = XAUDIOTHREADUSAGE_THREAD4 | XAUDIOTHREADUSAGE_THREAD5; // Both threads on core 2

         // Initialize the XAudio Engine
         if( SUCCEEDED( XAudioInitialize( &EngineInit ) ) )
            mXAInitialized = true;
         else
            AssertFatal( false, "XA Init failed. Crap." );
      }
      
      return new SFXXAudioDevice( this, info->name, useHardware, maxBuffers );
   }

   return NULL;
}
