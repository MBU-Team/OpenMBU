//-----------------------------------------------------------------------------
// Torque Game Engine Advanced
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "platform/platform.h"
#include "sfx/xaudio/sfxXAudioBuffer.h"

#include "sfx/sfxProfile.h"


SFXXAudioBuffer* SFXXAudioBuffer::create( SFXProfile *profile )
{
   AssertFatal( profile,  "SFXXAudioBuffer::create - Got null profile!" );

   const Resource<SFXResource> &resource = profile->getResource();
   if ( resource.isNull() )
      return NULL;

   SFXXAudioBuffer *buffer = new SFXXAudioBuffer( resource, profile->getDescription()->mIs3D );
   return buffer;
}


SFXXAudioBuffer::SFXXAudioBuffer( const Resource<SFXResource> &resource, bool is3d )
   : mResource( resource ), mIs3d(is3d)
{
   // Prepare the XAudio buffer struct.
   dMemset( &mBuffer, 0, sizeof( mBuffer ) );
   mBuffer.pAudioData = resource->getData();
   mBuffer.Flags = XAUDIO2_END_OF_STREAM;
   mBuffer.AudioBytes = resource->getSize();
}

SFXXAudioBuffer::~SFXXAudioBuffer()
{
}

void SFXXAudioBuffer::getFormat( WAVEFORMATEX *wfx ) const
{
   dMemset( wfx, 0, sizeof( WAVEFORMATEX ) ); 
   wfx->wFormatTag = WAVE_FORMAT_PCM; 
   wfx->nChannels = mResource->getChannels();
   wfx->nSamplesPerSec = mResource->getFrequency();
   wfx->wBitsPerSample = mResource->getSampleBits() / wfx->nChannels; 
   wfx->nBlockAlign = mResource->getSampleBits() / 8;
   wfx->nAvgBytesPerSec = wfx->nSamplesPerSec * wfx->nBlockAlign; 
}
