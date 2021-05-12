//-----------------------------------------------------------------------------
// Torque Game Engine Advanced
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "platform/platform.h"

#include "sfx/sfxProvider.h"
#include "sfx/null/sfxNullDevice.h"


class SFXNullProvider : public SFXProvider
{
public:

   SFXNullProvider();
   virtual ~SFXNullProvider();

protected:
   void addDeviceDesc( const char* name, const char* desc );

public:

   SFXDevice* createDevice( const char* deviceName, bool useHardware, S32 maxBuffers );

};

SFX_INIT_PROVIDER( SFXNullProvider );


SFXNullProvider::SFXNullProvider()
   : SFXProvider( "Null" )
{
   regProvider( this );
   addDeviceDesc( "SFX Null Device", "SFX baseline device" );
}

SFXNullProvider::~SFXNullProvider()
{
}


void SFXNullProvider::addDeviceDesc( const char* name, const char* desc )
{
   SFXDeviceInfo* info = new SFXDeviceInfo;
   dStrncpy( info->name, desc, sizeof( info->name ) );
   dStrncpy( info->driver, name, sizeof( info->driver ) );
   info->hasHardware = false;
   info->maxBuffers = 8;

   mDeviceInfo.push_back( info );
}

SFXDevice* SFXNullProvider::createDevice( const char* deviceName, bool useHardware, S32 maxBuffers )
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
      return new SFXNullDevice( this, info->name, useHardware, maxBuffers );

   return NULL;
}
