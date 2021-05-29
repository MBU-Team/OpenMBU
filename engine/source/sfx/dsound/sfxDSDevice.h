//-----------------------------------------------------------------------------
// Torque Game Engine Advanced
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _SFXDSDEVICE_H_
#define _SFXDSDEVICE_H_


#ifndef _SFXDEVICE_H_
   #include "sfx/sfxDevice.h"
#endif
#ifndef _SFXDSVOICE_H_
   #include "sfx/dsound/sfxDSVoice.h"
#endif
#ifndef OS_DLIBRARY_H
   #include "platform/platformDlibrary.h"
#endif

#include <dsound.h>

// Typedefs
#define DS_FUNCTION(fn_name, fn_return, fn_args) \
   typedef fn_return (WINAPI *DSFNPTR##fn_name ) fn_args ;
#include "sfx/dsound/dsFunctions.h"
#undef DS_FUNCTION

// Function table
struct DSoundFNTable
{
   DSoundFNTable() : isLoaded( false ) {};
   bool isLoaded;
   DLibraryRef dllRef;

#define DS_FUNCTION(fn_name, fn_return, fn_args) \
   DSFNPTR##fn_name fn_name;
#include "sfx/dsound/dsFunctions.h"
#undef DS_FUNCTION
};


/// Helper for asserting on dsound HRESULTS.
inline void DSAssert( HRESULT hr, const char *info )
{
   #ifdef TORQUE_DEBUG

      if( FAILED( hr ) )
      {
         char buf[256];
         dSprintf( buf, 256, "Error code: %x\n%s", hr, info );
         AssertFatal( false, buf );
      }

   #endif
}


/// The DirectSound device implementation exposes a couple
/// of settings to script that you should be aware of:
///
///   $DirectSound::dopplerFactor - This controls the scale of
///   the doppler effect.  Valid factors are 0.0 to 10.0 and it
///   defaults to 0.75.
///
///   $DirectSound::distanceFactor - This sets the unit conversion
///   for
///
///   $DirectSound::rolloffFactor - ;
///
///
class SFXDSDevice : public SFXDevice
{
   typedef SFXDevice Parent;

   public:

      //explicit SFXDSDevice();

      SFXDSDevice(   SFXProvider* provider,
                     DSoundFNTable *dsFnTbl,
                     GUID* guid,
                     const char* name,
                     bool useHardware,
                     S32 maxBuffers );

      virtual ~SFXDSDevice();

   protected:

      const StringTableEntry mName;

      IDirectSound8 *mDSound;

      IDirectSound3DListener8 *mListener;

      IDirectSoundBuffer *mPrimaryBuffer;

      DSoundFNTable *mDSoundTbl;

      DSCAPS mCaps;

      SFXDSVoiceVector mVoices;

      SFXDSBufferVector mBuffers;

   public:

      const char* getName() const { return mName; }

      //
      SFXBuffer* createBuffer( SFXProfile *profile );

      ///
      SFXVoice* createVoice( SFXBuffer *buffer );

      ///
      void deleteVoice( SFXVoice* buffer );

      U32 getVoiceCount() const { return mVoices.size(); }

      void update( const SFXListener& listener );
};

#endif // _SFXDSDEVICE_H_
