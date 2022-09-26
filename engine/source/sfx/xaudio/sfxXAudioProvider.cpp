//-----------------------------------------------------------------------------
// Torque Game Engine Advanced
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

// Note:  This must be defined before platform.h so that
// CoInitializeEx is properly included.
#define _WIN32_DCOM

#include "platform/platform.h"

#include "sfx/xaudio/sfxXAudioDevice.h"
#include "sfx/sfxProvider.h"
#include "util/safeRelease.h"
#include "core/unicode.h"
#include "console/console.h"

#include <xaudio2.h>


class SFXXAudioProvider : public SFXProvider
{
public:

   SFXXAudioProvider();
   virtual ~SFXXAudioProvider();

protected:

   /// Extended SFXDeviceInfo to also store some 
   /// extra XAudio specific data.
   struct XADeviceInfo : SFXDeviceInfo
   {
      UINT32 deviceIndex;

      XAUDIO2_DEVICE_ROLE role;

      WAVEFORMATEXTENSIBLE format;
   };

   /// Helper for creating the XAudio engine.
   static bool _createXAudio( IXAudio2 **xaudio );

public:

   SFXDevice* createDevice( const char* deviceName, bool useHardware, S32 maxBuffers );

};

SFX_INIT_PROVIDER( SFXXAudioProvider );


SFXXAudioProvider::SFXXAudioProvider()
   :  SFXProvider( "XAudio" )
{
   // Create a temp XAudio object for device enumeration.
   IXAudio2 *xAudio = NULL;
   if ( !_createXAudio( &xAudio ) )
   {
      Con::errorf( "SFXXAudioProvider() - XAudio2 failed to load!" );
      return;
   }

   // Add the devices to the info list.
   UINT32 count = 0;
   xAudio->GetDeviceCount( &count );
   for ( UINT32 i = 0; i < count; i++ )
   {
      XAUDIO2_DEVICE_DETAILS details;
      HRESULT hr = xAudio->GetDeviceDetails( i, &details );
      if ( FAILED( hr ) )
         continue;

      // Add a device to the info list.
      XADeviceInfo* info = new XADeviceInfo;
      info->deviceIndex = i;
      dStrncpy( info->driver, "XAudio", sizeof( info->driver ) );
      convertUTF16toUTF8( details.DisplayName, info->name, sizeof( info->name ) );
      info->hasHardware = false;
      info->maxBuffers = 64;
      info->role = details.Role;
      info->format = details.OutputFormat;
      mDeviceInfo.push_back( info );
   }

   // We're done with XAudio for now.
   SAFE_RELEASE( xAudio );

   // If we have no devices... we're done.
   if ( mDeviceInfo.empty() )
   {
      Con::errorf( "SFXXAudioProvider - No valid XAudio2 devices found!" );
      return;
   }

   // If we got this far then we should be able to
   // safely create a device for XAudio.
   regProvider( this );
}

SFXXAudioProvider::~SFXXAudioProvider()
{
}

bool SFXXAudioProvider::_createXAudio( IXAudio2 **xaudio )
{
   // In debug builds enable the debug version 
   // of the XAudio engine.
   #ifdef TORQUE_DEBUG
      #define XAUDIO_FLAGS XAUDIO2_DEBUG_ENGINE
   #else
      #define XAUDIO_FLAGS 0
   #endif

   // This must be called first... it doesn't hurt to 
   // call it more than once.
   CoInitialize( NULL );

   // Try creating the xaudio engine.
   HRESULT hr = XAudio2Create( xaudio, XAUDIO_FLAGS, XAUDIO2_DEFAULT_PROCESSOR );

   return SUCCEEDED( hr ) && (*xaudio);
}

SFXDevice* SFXXAudioProvider::createDevice( const char* deviceName, bool useHardware, S32 maxBuffers )
{
   XADeviceInfo* info = NULL;

   // Look for the device name in the array.
   SFXDeviceInfoVector::iterator iter = mDeviceInfo.begin();
   for ( ; iter != mDeviceInfo.end(); iter++ )
   {
      if ( dStricmp( deviceName, (*iter)->name ) == 0 )
      {
         info = (XADeviceInfo*)*iter;
         break;
      }
   }

   // If we still don't have a desc and the name 
   // is blank then use the first one.
   if ( !info && ( !deviceName || !deviceName[0] ) )
      info = (XADeviceInfo*)mDeviceInfo[0];

   // Do we find one to create?
   if ( info )
   {
      // Create the XAudio object to pass to the device.
      IXAudio2 *xAudio = NULL;
      if ( !_createXAudio( &xAudio ) )
      {
         Con::errorf( "SFXXAudioProvider::createDevice() - XAudio2 failed to load!" );
         return NULL;
      }

      return new SFXXAudioDevice(   this,
                                    deviceName, 
                                    xAudio, 
                                    info->deviceIndex,
                                    info->format.dwChannelMask,
                                    maxBuffers );
   }

   // We didn't find a matching valid device.
   return NULL;
}
