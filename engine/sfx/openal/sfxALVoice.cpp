//-----------------------------------------------------------------------------
// Torque Game Engine Advanced
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "platform/platform.h"
#include "sfx/openal/sfxALVoice.h"

#include "sfx/openal/sfxALBuffer.h"
#include "sfx/openal/sfxALDevice.h"

#ifdef TORQUE_DEBUG
#  define AL_SANITY_CHECK() \
   AssertFatal( mOpenAL.alIsBuffer( mBufferName ), "AL Buffer Sanity Check Failed!" ); \
   AssertFatal( mOpenAL.alIsSource( mSourceName ), "AL Source Sanity Check Failed!" );
#else
#  define AL_SANITY_CHECK() 
#endif

SFXALVoice* SFXALVoice::create( SFXALBuffer *buffer )
{
   AssertFatal( buffer, "SFXALVoice::create() - Got null buffer!" );
 
   ALuint bufferName;
   ALuint sourceName;
   ALenum bufferFormat;

   if ( !buffer->createVoice( &bufferName,
                              &sourceName,
                              &bufferFormat ) )
      return NULL;

   SFXALVoice *voice = new SFXALVoice( buffer->mOpenAL,
                                       buffer,
                                       bufferName,
                                       sourceName );



   // Is this 3d?
  // Okay, this looks odd, but bear with me for a moment.  AL_SOURCE_RELATIVE does NOT indicate
  // whether or not the volume of the sound should change depending on the position of the listener.
  // OpenAL assumes that the volume will ALWAYS depend on the position of the listener.  What AL_SOURCE_RELATIVE
  // does do is dictate if the position of THIS SOURCE is relative to the listener.  If AL_SOURCE_RELATIVE is AL_TRUE
  // and the source's position is 0, 0, 0, then the source is directly on top of the listener at all times, which is what
  // we want for non-3d sounds.
   voice->mOpenAL.alSourcei(sourceName, AL_SOURCE_RELATIVE, (buffer->mIs3d ? AL_FALSE : AL_TRUE));

   return voice;
}

SFXALVoice::SFXALVoice( const OPENALFNTABLE &oalft,
                        SFXALBuffer *buffer, 
                        ALuint bufferName,
                        ALuint sourceName )

   :  mOpenAL( oalft ),
      mIsPlaying( false ), 
      mBufferName( bufferName ), 
      mSourceName( sourceName )
{
   AL_SANITY_CHECK();
}

SFXALVoice::~SFXALVoice()
{
   mOpenAL.alDeleteSources( 1, &mSourceName );
   mOpenAL.alDeleteBuffers( 1, &mBufferName );
}

void SFXALVoice::setPosition( U32 pos )
{
   AL_SANITY_CHECK();
   mOpenAL.alSourcei( mSourceName, AL_BYTE_OFFSET, pos );
}

void SFXALVoice::setMinMaxDistance( F32 min, F32 max )
{
   AL_SANITY_CHECK();

   mOpenAL.alSourcef( mSourceName, AL_REFERENCE_DISTANCE, min );
   mOpenAL.alSourcef( mSourceName, AL_MAX_DISTANCE, max );
}

SFXStatus SFXALVoice::getStatus() const
{
   AL_SANITY_CHECK();

   ALint state;
   mOpenAL.alGetSourcei( mSourceName, AL_SOURCE_STATE, &state );
   
   if ( state == AL_PLAYING )
      return SFXStatusPlaying;
   
   if ( state == AL_PAUSED )
      return SFXStatusPaused;

   return SFXStatusStopped;
}

void SFXALVoice::play( bool looping )
{
   AL_SANITY_CHECK();

   mOpenAL.alSourceStop( mSourceName );
   mOpenAL.alSourcei( mSourceName, AL_LOOPING, ( looping ? AL_TRUE : AL_FALSE ) );
   mOpenAL.alSourcePlay( mSourceName );

   // Test this
   ALint state;
   mOpenAL.alGetSourcei( mSourceName, AL_SOURCE_STATE, &state );
}

void SFXALVoice::pause()
{
   AL_SANITY_CHECK();

   mOpenAL.alSourcePause( mSourceName );
}

void SFXALVoice::stop()
{
   AL_SANITY_CHECK();

   mOpenAL.alSourceStop( mSourceName );
}

void SFXALVoice::setVelocity( const VectorF& velocity )
{
   AL_SANITY_CHECK();

   mOpenAL.alSourcefv( mSourceName, AL_VELOCITY, (F32 *)velocity );
}

void SFXALVoice::setTransform( const MatrixF& transform )
{
   AL_SANITY_CHECK();

   Point3F pos, dir;
   transform.getColumn( 3, &pos );
   transform.getColumn( 1, &dir );
   mOpenAL.alSourcefv( mSourceName, AL_POSITION, (F32 *)pos );
   mOpenAL.alSourcefv( mSourceName, AL_DIRECTION, (F32 *)dir );
}

void SFXALVoice::setVolume( F32 volume )
{
   AL_SANITY_CHECK();

   mOpenAL.alSourcef( mSourceName, AL_GAIN, volume );
}

void SFXALVoice::setPitch( F32 pitch )
{ 
   AL_SANITY_CHECK();

   mOpenAL.alSourcef( mSourceName, AL_PITCH, pitch );
}

