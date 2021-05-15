//-----------------------------------------------------------------------------
// Torque Game Engine Advanced
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "platform/platform.h"
#include "sfx/openal/sfxALBuffer.h"

#include "sfx/sfxProfile.h"


SFXALBuffer* SFXALBuffer::create(   const OPENALFNTABLE &oalft, 
                                    SFXProfile *profile,
                                    bool useHardware )
{
   AssertFatal( profile, "SFXALBuffer::create() - Got null profile!" );

   SFXALBuffer *buffer = new SFXALBuffer( oalft,
                                          profile->getResource(),
                                          profile->getDescription()->mIs3D,
                                          useHardware );

   return buffer;
}

SFXALBuffer::SFXALBuffer(  const OPENALFNTABLE &oalft, 
                           const Resource<SFXResource> &resource,
                           bool is3d,
                           bool useHardware )
   :  mOpenAL( oalft ),
      mResource( resource ),
      mIs3d( is3d ),
      mUseHardware( useHardware )
{
}

SFXALBuffer::~SFXALBuffer()
{
}

bool SFXALBuffer::createVoice(   ALuint *bufferName,
                                 ALuint *sourceName,
                                 ALenum *bufferFormat ) const
{
   mOpenAL.alGenBuffers( 1, bufferName );
   mOpenAL.alGenSources( 1, sourceName );

   AssertFatal( mOpenAL.alIsBuffer( *bufferName ), "AL Buffer Sanity Check Failed!" ); \
   AssertFatal( mOpenAL.alIsSource( *sourceName ), "AL Source Sanity Check Failed!" );

   // Stereo 16 == 32 bits per sample, 16 per channel
   if( mResource->getChannels() > 1 )
      *bufferFormat = mResource->getSampleBits() == 32 ? AL_FORMAT_STEREO16 : AL_FORMAT_STEREO8;
   else
      *bufferFormat = mResource->getSampleBits() == 16 ? AL_FORMAT_MONO16 : AL_FORMAT_MONO8;

   // Is this 3d?
   mOpenAL.alSourcei( *sourceName, AL_SOURCE_RELATIVE, ( mIs3d ? AL_FALSE : AL_TRUE ) );

   AssertFatal( mOpenAL.alIsBuffer( *bufferName ), "AL Buffer Sanity Check Failed!" ); \
   AssertFatal( mOpenAL.alIsSource( *sourceName ), "AL Source Sanity Check Failed!" );

   mOpenAL.alGetError();
   mOpenAL.alBufferData(   *bufferName, 
                           *bufferFormat, 
                           mResource->getData(), 
                           mResource->getSize(), 
                           mResource->getFrequency() );

   U32 err = mOpenAL.alGetError();

   mOpenAL.alSourcei( *sourceName, AL_BUFFER, *bufferName );

   return ( err == AL_NO_ERROR );
}
