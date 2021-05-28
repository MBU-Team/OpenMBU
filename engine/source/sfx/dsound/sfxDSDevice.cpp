//-----------------------------------------------------------------------------
// Torque Game Engine Advanced
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "platform/platform.h"
#include "sfx/dsound/sfxDSDevice.h"

#include "sfx/dsound/sfxDSBuffer.h"
#include "sfx/dsound/sfxDSVoice.h"
#include "sfx/sfxListener.h"
#include "platformWin32/platformWin32.h"
#include "util/safeRelease.h"


SFXDSDevice::SFXDSDevice( SFXProvider* provider, DSoundFNTable *dsFnTbl, GUID* guid, const char* name, bool useHardware, S32 maxBuffers )
   :  SFXDevice( provider, useHardware, maxBuffers ),
      mName( StringTable->insert( name ) ),
      mDSound( NULL ),
      mPrimaryBuffer( NULL ),
      mListener( NULL ),
      mDSoundTbl( dsFnTbl )
{
   HRESULT hr = mDSoundTbl->DirectSoundCreate8( guid, &mDSound, NULL );   
   if ( FAILED( hr ) || !mDSound )
   {
      // TODO: Deal with it!
      return;
   }

   hr = mDSound->SetCooperativeLevel( winState.appWindow, DSSCL_PRIORITY );
   if ( FAILED( hr ) )
   {
      // TODO: Deal with it!
      return;
   }

   // Get the primary buffer.
   DSBUFFERDESC dsbd;   
   dMemset( &dsbd, 0, sizeof( DSBUFFERDESC ) ); 
   dsbd.dwSize = sizeof( DSBUFFERDESC );
   dsbd.dwFlags = DSBCAPS_CTRL3D | DSBCAPS_PRIMARYBUFFER;
   hr = mDSound->CreateSoundBuffer( &dsbd, &mPrimaryBuffer, NULL );
   if ( FAILED( hr ) )
   {
      // TODO: Deal with it!
      return;
   }

   // Set the format and bitrate on the primary buffer.
   S32 frequency = Con::getIntVariable( "$pref::SFX::frequency", 44100 );
   S32 bitrate = Con::getIntVariable( "$pref::SFX::bitrate", 32 );

   WAVEFORMATEX wfx;
   dMemset( &wfx, 0, sizeof( WAVEFORMATEX ) ); 
   wfx.wFormatTag = WAVE_FORMAT_PCM; 
   wfx.nChannels = 2; 
   wfx.nSamplesPerSec = frequency;
   wfx.wBitsPerSample = bitrate; 
   wfx.nBlockAlign = ( wfx.nChannels * wfx.wBitsPerSample ) / 8;
   wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign; 
   hr = mPrimaryBuffer->SetFormat( &wfx );
   if ( FAILED( hr ) )
   {
      // TODO: Deal with it!
      return;
   }

   // Grab the 3d listener.
   hr = mPrimaryBuffer->QueryInterface( IID_IDirectSound3DListener8, (LPVOID*)&mListener );
   if ( FAILED( hr ) )
   {
      // TODO: Deal with it!
      return;
   }

   // Initialize some settings.
   F32 doppler    = Con::getIntVariable( "$directSound::dopplerFactor", 0.75f );
   F32 distance   = Con::getIntVariable( "$directSound::distanceFactor", 1.0f );
   F32 rolloff    = Con::getIntVariable( "$directSound::rolloffFactor", 1.0f );
   mListener->SetDopplerFactor( doppler, DS3D_DEFERRED );
   mListener->SetDistanceFactor( distance, DS3D_DEFERRED );
   mListener->SetRolloffFactor( rolloff, DS3D_DEFERRED );
   mListener->CommitDeferredSettings();

   mCaps.dwSize = sizeof( DSCAPS );
   mDSound->GetCaps( &mCaps );

   // If the device reports no hardware buffers then
   // we have no choice but to disable hardware.
   if ( mCaps.dwMaxHw3DAllBuffers == 0 )
      mUseHardware = false;

   // If mMaxBuffers is negative then use the caps
   // to decide on a good maximum value... or set 8.
   if ( mMaxBuffers < 0 )
      mMaxBuffers = getMax( mCaps.dwMaxHw3DAllBuffers, 8 );
}

SFXDSDevice::~SFXDSDevice()
{
   // First cleanup the buffers... then voices.

   SFXDSBufferVector::iterator buffer = mBuffers.begin();
   for ( ; buffer != mBuffers.end(); buffer++ )
      delete (*buffer);
   mBuffers.clear();

   SFXDSVoiceVector::iterator voice = mVoices.begin();
   for ( ; voice != mVoices.end(); voice++ )
      delete (*voice);
   mVoices.clear();

   // And release our resources.
   SAFE_RELEASE( mListener );
   SAFE_RELEASE( mPrimaryBuffer );
   SAFE_RELEASE( mDSound );
}

SFXBuffer* SFXDSDevice::createBuffer( SFXProfile *profile )
{
   AssertFatal( profile, "SFXDSDevice::createBuffer() - Got null profile!" );

   SFXDSBuffer* buffer = SFXDSBuffer::create(   mDSound, 
                                                profile, 
                                                mUseHardware );
   if ( !buffer )
      return NULL;

   mBuffers.push_back( buffer );
   return buffer;
}

SFXVoice* SFXDSDevice::createVoice( SFXBuffer *buffer )
{
   // Don't bother going any further if we've 
   // exceeded the maximum voices.
   if ( mVoices.size() >= mMaxBuffers )
      return NULL;

   AssertFatal( buffer, "SFXDSDevice::createVoice() - Got null buffer!" );

   SFXDSBuffer* dsBuffer = dynamic_cast<SFXDSBuffer*>( buffer );
   AssertFatal( dsBuffer, "SFXDSDevice::createVoice() - Got bad buffer!" );

   SFXDSVoice* voice = SFXDSVoice::create( dsBuffer );
   if ( !voice )
      return NULL;

   mVoices.push_back( voice );
	return voice;
}

void SFXDSDevice::deleteVoice( SFXVoice* voice )
{
   AssertFatal( voice, "SFXDSDevice::deleteVoice() - Got null voice!" );

   SFXDSVoice* dsVoice = dynamic_cast<SFXDSVoice*>( voice );
   AssertFatal( dsVoice, "SFXDSDevice::deleteVoice() - Got bad voice!" );

	// Find the buffer...
	SFXDSVoiceVector::iterator iter = find( mVoices.begin(), mVoices.end(), dsVoice );
	AssertFatal( iter != mVoices.end(), "SFXDSDevice::deleteVoice() - Got unknown voice!" );

	mVoices.erase( iter );
	delete dsVoice;
}

void SFXDSDevice::update( const SFXListener& listener )
{
   // Get the transform from the listener.
   const MatrixF& transform = listener.getTransform();
   Point3F pos, dir, up;
   transform.getColumn( 3, &pos );
   transform.getColumn( 1, &dir );
   transform.getColumn( 2, &up );

   // And the velocity...
   const VectorF& velocity = listener.getVelocity();

   // CodeReview This is a symptom fix, should be undone. BJG - 3/25/07
   if(!mListener)
      return;

   // Finally, set it all to DSound!
   mListener->SetPosition( pos.x, pos.z, pos.y, DS3D_DEFERRED );
   mListener->SetOrientation( dir.x, dir.z, dir.y, up.x, up.z, up.y, DS3D_DEFERRED );
   mListener->SetVelocity( velocity.x, velocity.z, velocity.y, DS3D_DEFERRED );
   
   // Apply the deferred settings that changed between updates.
   mListener->CommitDeferredSettings();
}

