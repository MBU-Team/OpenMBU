//-----------------------------------------------------------------------------
// Torque Game Engine Advanced
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------
#include "platform/platform.h"

#include "sfx/sfxProvider.h"
#include "sfx/openal/sfxALDevice.h"
#include "sfx/openal/aldlist.h"
#include "console/console.h"

class SFXALProvider : public SFXProvider
{
public:

   SFXALProvider();
   virtual ~SFXALProvider();

protected:
   OPENALFNTABLE mOpenAL;
   ALDeviceList *mALDL;

   struct ALDeviceInfo : SFXDeviceInfo
   {
      
   };

public:
   SFXDevice *createDevice( const char* deviceName, bool useHardware, S32 maxBuffers );

};

SFX_INIT_PROVIDER( SFXALProvider );


SFXALProvider::SFXALProvider()
: SFXProvider( "OpenAL" )
{
   if( LoadOAL10Library( NULL, &mOpenAL ) != AL_TRUE )
   {
      Con::printf( "SFXALProvider - OpenAL not available." );
      return;
   }
   mALDL = new ALDeviceList( mOpenAL );

   // Did we get any devices?
   if ( mALDL->GetNumDevices() < 1 )
   {
      Con::printf( "SFXALProvider - No valid devices found!" );
      return;
   }

   // Cool, loop through them, and caps em
   const char *deviceFormat = "OpenAL v%d.%d %s";

   char temp[256];
   for( int i = 0; i < mALDL->GetNumDevices(); i++ )
   {
      ALDeviceInfo* info = new ALDeviceInfo;
      
      dStrncpy( info->name, mALDL->GetDeviceName(i), sizeof( info->name ) );

      int major, minor, eax = 0;

      mALDL->GetDeviceVersion( i, &major, &minor );

      // Apologies for the blatent enum hack -patw
      for( int j = SFXALEAX2; j < SFXALEAXRAM; j++ )
         eax += (int)mALDL->IsExtensionSupported( i, (SFXALCaps)j );

      if( eax > 0 )
      {
         eax += 2; // EAX support starts at 2.0
         dSprintf( temp, sizeof( temp ), "[EAX %d.0] %s", eax, ( mALDL->IsExtensionSupported( i, SFXALEAXRAM ) ? "EAX-RAM" : "" ) );
      }
      else
         dStrcpy( temp, "" );

      dSprintf( info->driver, sizeof( info->driver ), deviceFormat, major, minor, temp );
      info->hasHardware = eax > 0;
      info->maxBuffers = mALDL->GetMaxNumSources( i );

      mDeviceInfo.push_back( info );
   }

   regProvider( this );
}

SFXALProvider::~SFXALProvider()
{
   UnloadOAL10Library();
   delete mALDL;
}

SFXDevice *SFXALProvider::createDevice( const char* deviceName, bool useHardware, S32 maxBuffers )
{
   ALDeviceInfo *info = NULL;

   // Look for the device name in the array.
   SFXDeviceInfoVector::iterator iter = mDeviceInfo.begin();
   for ( ; iter != mDeviceInfo.end(); iter++ )
   {
      if ( dStricmp( deviceName, (*iter)->name ) == 0 )
      {
         info = (ALDeviceInfo *)*iter;
         break;
      }
   }

   // If we stil don't have a desc and the name 
   // is blank then use the first one.
   if ( !info && ( !deviceName || !deviceName[0] ) )
      info = (ALDeviceInfo *)mDeviceInfo[0];

   // Do we find one to create?
   if ( info )
      return new SFXALDevice( this, mOpenAL, info->name, useHardware, maxBuffers );

   return NULL;
}