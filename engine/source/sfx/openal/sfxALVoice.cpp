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

   return voice;
}

SFXALVoice::SFXALVoice( const OPENALFNTABLE &oalft,
                        SFXALBuffer *buffer, 
                        ALuint bufferName,
                        ALuint sourceName )

   :  mOpenAL( oalft ),
      mResumeAtSampleOffset(-1.0f),
      mIsPlaying( false ), 
      mBufferName( bufferName ), 
      mSourceName( sourceName ),
      mIs3D(buffer->mIs3d)
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
   mOpenAL.alSourcei( mSourceName, AL_SAMPLE_OFFSET, pos );
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

   // Adjust play cursor for buggy OAL when resuming playback.  Do this after alSourcePlay
   // as it is the play function that will cause the cursor to jump.

   if (mResumeAtSampleOffset != -1.0f)
   {
       mOpenAL.alSourcef(mSourceName, AL_SAMPLE_OFFSET, mResumeAtSampleOffset);
       mResumeAtSampleOffset = -1.0f;
   }

   // Test this
   ALint state;
   mOpenAL.alGetSourcei( mSourceName, AL_SOURCE_STATE, &state );
}

void SFXALVoice::pause()
{
   AL_SANITY_CHECK();

   mOpenAL.alSourcePause( mSourceName );

   //WORKAROUND: Another workaround for the buggy OAL.  Resuming playback of a paused source will cause the 
   // play cursor to jump.  Save the cursor so we can manually move it into position in _play().  Sigh.

   mOpenAL.alGetSourcef(mSourceName, AL_SAMPLE_OFFSET, &mResumeAtSampleOffset);
}

void SFXALVoice::stop()
{
   AL_SANITY_CHECK();

   mOpenAL.alSourceStop( mSourceName );
   
   mResumeAtSampleOffset = -1.0f;
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

