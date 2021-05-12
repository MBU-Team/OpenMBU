//-----------------------------------------------------------------------------
// Torque Game Engine Advanced
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "platform/platform.h"
#include "sfx/sfxProvider.h"

#include "sfx/dsound/sfxDSDevice.h"
#include "util/safeRelease.h"
#include "console/console.h"


class SFXDSProvider : public SFXProvider
{
public:

   SFXDSProvider();
   virtual ~SFXDSProvider();

protected:
   DSoundFNTable mDSound;

   struct DSDeviceInfo : SFXDeviceInfo
   {
      GUID* guid;
      DSCAPS caps;
   };

   static BOOL CALLBACK dsEnumProc( 
      LPGUID lpGUID, 
      LPCSTR lpszDesc, 
      LPCSTR lpszDrvName, 
      LPVOID lpContext );

   void addDeviceDesc( GUID* guid, const char* name, const char* desc );

public:

   SFXDevice* createDevice( const char* deviceName, bool useHardware, S32 maxBuffers );

};

SFX_INIT_PROVIDER( SFXDSProvider );

//------------------------------------------------------------------------------
// Helper

bool dsBindFunction( DLibrary *dll, void *&fnAddress, const char *name )
{
   fnAddress = dll->bind( name );

   if (!fnAddress)
      Con::warnf( "DSound Loader: DLL bind failed for %s", name );

   return fnAddress != 0;
}

//------------------------------------------------------------------------------


SFXDSProvider::SFXDSProvider()
   : SFXProvider( "DirectSound" )
{
   // Grab the functions we'll want from the dsound DLL.
   mDSound.dllRef = OsLoadLibrary( "dsound.dll" );
   mDSound.isLoaded = true;
#define DS_FUNCTION(fn_name, fn_return, fn_args) \
   mDSound.isLoaded &= dsBindFunction(mDSound.dllRef, *(void**)&mDSound.fn_name, #fn_name);
#include "sfx/dsound/dsFunctions.h"
#undef DS_FUNCTION

   AssertISV( mDSound.isLoaded, "DirectSound failed to load." );

   // All we need to do to init is enumerate the 
   // devices... if this fails then don't register
   // the provider as it's broken in some way.
   if ( FAILED( mDSound.DirectSoundEnumerateA( dsEnumProc, (VOID*)this ) ) )
   {
      Con::errorf( "SFXDSProvider - Device enumeration failed!" );
      return;
   }

   // Did we get any devices?
   if ( mDeviceInfo.empty() )
   {
      Con::errorf( "SFXDSProvider - No valid devices found!" );
      return;
   }

   // Wow, we made it - register the provider.
   regProvider( this );
}

SFXDSProvider::~SFXDSProvider()
{

}


BOOL CALLBACK SFXDSProvider::dsEnumProc( 
   LPGUID lpGUID, 
   LPCSTR lpszDesc, 
   LPCSTR lpszDrvName, 
   LPVOID lpContext )
{
   SFXDSProvider* provider = (SFXDSProvider*)lpContext;
   provider->addDeviceDesc( lpGUID, lpszDrvName, lpszDesc );
   return TRUE;
}

void SFXDSProvider::addDeviceDesc( GUID* guid, const char* name, const char* desc )
{
   // Create a dummy device to get the caps.
   IDirectSound8* dsound;
   HRESULT hr = mDSound.DirectSoundCreate8( guid, &dsound, NULL );   
   if ( FAILED( hr ) || !dsound )
      return;

   // Init the caps structure and have the device fill it out.
   DSCAPS caps;
   dMemset( &caps, 0, sizeof( caps ) ); 
   caps.dwSize = sizeof( caps );
   hr = dsound->GetCaps( &caps );

   // Clean up and handle errors.
   SAFE_RELEASE( dsound );

   if ( FAILED( hr ) )
      return;

   // Now, record the desc info into our own internal list.
   DSDeviceInfo* info = new DSDeviceInfo;
   dStrncpy( info->name, desc, sizeof( info->name ) );
   dStrncpy( info->driver, name, sizeof( info->driver ) );
   info->hasHardware = caps.dwMaxHw3DAllBuffers > 0;
   info->maxBuffers = caps.dwMaxHw3DAllBuffers;
   info->guid = guid;
   info->caps = caps;

   mDeviceInfo.push_back( info );
}

SFXDevice* SFXDSProvider::createDevice( const char* deviceName, bool useHardware, S32 maxBuffers )
{
   DSDeviceInfo* info = NULL;

   // Look for the device name in the array.
   SFXDeviceInfoVector::iterator iter = mDeviceInfo.begin();
   for ( ; iter != mDeviceInfo.end(); iter++ )
   {
      if ( dStricmp( deviceName, (*iter)->name ) == 0 )
      {
         info = (DSDeviceInfo*)*iter;
         break;
      }
   }

   // If we still don't have a desc and the name 
   // is blank then use the first one.
   if ( !info && ( !deviceName || !deviceName[0] ) )
      info = (DSDeviceInfo*)mDeviceInfo[0];

   // Do we find one to create?
   if ( info )
      return new SFXDSDevice( this, 
                              &mDSound,
                              info->guid,
                              info->name, 
                              useHardware, 
                              maxBuffers );

   // Whoops - fell through! (Couldn't create a matching device.)
   return NULL;
}
