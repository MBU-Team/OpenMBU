//-----------------------------------------------------------------------------
// Torque Game Engine Advanced
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "platform/platform.h"

#include "sfx/dsound/sfxDSBuffer.h"
#include "sfx/sfxProfile.h"
#include "util/safeRelease.h"

#pragma comment( lib, "dxguid.lib" )

SFXDSBuffer* SFXDSBuffer::create(   IDirectSound8 *dsound, 
                                    SFXProfile *profile,
                                    bool useHardware )
{
   AssertFatal( dsound, "SFXDSBuffer::create() - Got null dsound!" );
   AssertFatal( profile, "SFXDSBuffer::create() - Got null profile!" );


   // Return the buffer.
   SFXDSBuffer* buffer = new SFXDSBuffer( dsound,
                                          profile->getResource(),
                                          profile->getDescription()->mIs3D,
                                          useHardware );

   return buffer;
}

SFXDSBuffer::SFXDSBuffer(  IDirectSound8* dsound,
                           const Resource<SFXResource> &resource,
                           bool is3d,
                           bool useHardware )
   :  mDSound( dsound ),
      mResource( resource ),
      mIs3d( is3d ),
      mUseHardware( useHardware ),
      mBuffer( NULL ),
      mDuplicate( false )
{
   AssertFatal( mDSound, "SFXDSBuffer::SFXDSBuffer() - Got null dsound!" );
   AssertFatal( !mResource.isNull(), "SFXDSBuffer::SFXDSBuffer() - Got null resource!" );

   mDSound->AddRef();

   // Duplication is only allowed in hardware
   // so don't init the buffer in that case.
   if ( !mUseHardware )
      _createBuffer( &mBuffer );
}

SFXDSBuffer::~SFXDSBuffer()
{
   mResource = NULL;

   SAFE_RELEASE( mBuffer );
   SAFE_RELEASE( mDSound );
}

bool SFXDSBuffer::_createBuffer( IDirectSoundBuffer8 **buffer8 ) const
{
   // Set up WAV format structure. 
   WAVEFORMATEX wfx;
   dMemset( &wfx, 0, sizeof( WAVEFORMATEX ) ); 
   wfx.wFormatTag = WAVE_FORMAT_PCM; 
   wfx.nChannels = mResource->getChannels();
   wfx.nSamplesPerSec = mResource->getFrequency();
   wfx.wBitsPerSample = mResource->getSampleBits() / wfx.nChannels; 
   wfx.nBlockAlign = mResource->getSampleBits() / 8;
   wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign; 

   // Set up DSBUFFERDESC structure. 
   DSBUFFERDESC dsbdesc; 
   dMemset( &dsbdesc, 0, sizeof( DSBUFFERDESC ) ); 
   dsbdesc.dwSize = sizeof( DSBUFFERDESC ); 
   dsbdesc.dwFlags = 
      ( mIs3d ? DSBCAPS_CTRL3D | DSBCAPS_MUTE3DATMAXDISTANCE : DSBCAPS_CTRLPAN  ) |
      DSBCAPS_CTRLFREQUENCY |
      DSBCAPS_CTRLVOLUME |
      DSBCAPS_GETCURRENTPOSITION2 |
      DSBCAPS_GLOBALFOCUS |
      DSBCAPS_STATIC |
      ( mUseHardware ? DSBCAPS_LOCHARDWARE : DSBCAPS_LOCSOFTWARE );
   dsbdesc.dwBufferBytes = mResource->getSize();
   if ( mIs3d )
      dsbdesc.guid3DAlgorithm = DS3DALG_HRTF_FULL;
   dsbdesc.lpwfxFormat = &wfx;

   // Create the buffer.
   IDirectSoundBuffer *buffer = NULL;
   HRESULT hr = mDSound->CreateSoundBuffer( &dsbdesc, &buffer, NULL );
   if ( FAILED( hr ) || !buffer )
      return false;

   // Grab the version 8 interface.
   hr = buffer->QueryInterface( IID_IDirectSoundBuffer8, (LPVOID*)buffer8 );

   // Release the original interface.
   buffer->Release();

   // If we failed to get the 8 interface... exit.
   if ( FAILED( hr ) || !(*buffer8) )
      return false;

   // Copy the data over and return.
   return _copyData( *buffer8, 
                     0,
                     mResource->getData(),
                     mResource->getSize() );
}

bool SFXDSBuffer::_copyData(  IDirectSoundBuffer8 *buffer8,
                              U32 offset, 
                              const U8 *data,
                              U32 length ) const
{
   // Fill the buffer with the resource data.      
   VOID* lpvWrite;
   DWORD  dwLength;
   VOID* lpvWrite2;
   DWORD  dwLength2;
   HRESULT hr = buffer8->Lock(
         offset,           // Offset at which to start lock.
         length,           // Size of lock; ignored because of flag.
         &lpvWrite,        // Gets address of first part of lock.
         &dwLength,        // Gets size of first part of lock.
         &lpvWrite2,       // Address of wraparound not needed. 
         &dwLength2,       // Size of wraparound not needed.
         0 );
   if ( FAILED( hr ) )
      return false;

   // Copy the first part.
   dMemcpy( lpvWrite, data, dwLength );

   // Do we have a wrap?
   if ( lpvWrite2 )
      dMemcpy( lpvWrite2, data + dwLength, dwLength2 );

   // And finally, unlock.
   hr = buffer8->Unlock(
         lpvWrite,      // Address of lock start.
         dwLength,      // Size of lock.
         lpvWrite2,     // No wraparound portion.
         dwLength2 );   // No wraparound size.

   // Return success code.
   return SUCCEEDED(hr);
}

bool SFXDSBuffer::_duplicateBuffer( IDirectSoundBuffer8 **buffer8 )
{
   AssertFatal( mBuffer, "SFXDSBuffer::_duplicateBuffer() - Duplicate buffer is null!" );

   // If this is the first duplicate then
   // give the caller our original buffer.
   if ( !mDuplicate )
   {
      mDuplicate = true;

      *buffer8 = mBuffer;
      (*buffer8)->AddRef();

      return true;
   }

   IDirectSoundBuffer *buffer1 = NULL;
   HRESULT hr = mDSound->DuplicateSoundBuffer( mBuffer, &buffer1 );
   if ( FAILED( hr ) || !buffer1 )
      return false;

   // Grab the version 8 interface.
   hr = buffer1->QueryInterface( IID_IDirectSoundBuffer8, (LPVOID*)buffer8 );

   // Release the original interface.
   buffer1->Release();

   return SUCCEEDED( hr ) && (*buffer8);
}

bool SFXDSBuffer::createVoice( IDirectSoundBuffer8 **buffer8 )
{
   // If we have a buffer we can try to duplicate.
   if ( mBuffer && _duplicateBuffer( buffer8 ) && *buffer8 )
      return true;

   // We either cannot duplicate or it failed, so
   // go ahead and try to create a new buffer.
   return _createBuffer( buffer8 ) && (*buffer8);
}

void SFXDSBuffer::releaseVoice( IDirectSoundBuffer8 **buffer )
{
   AssertFatal( *buffer, "SFXDSBuffer::releaseVoice() - Got null buffer!" );

   if ( *buffer == mBuffer )
   {
      mDuplicate = false;
      (*buffer)->Stop();
   }

   SAFE_RELEASE( (*buffer) );
}
