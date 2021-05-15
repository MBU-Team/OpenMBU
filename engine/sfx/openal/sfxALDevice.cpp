//-----------------------------------------------------------------------------
// Torque Game Engine Advanced
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "platform/platform.h"

#include "sfx/openal/sfxALDevice.h"
#include "sfx/openal/sfxALBuffer.h"
#include "sfx/sfxListener.h"


SFXALDevice::SFXALDevice(  SFXProvider* provider, 
                           const OPENALFNTABLE &openal, 
                           const char* name, 
                           bool useHardware, 
                           S32 maxBuffers )

   :  SFXDevice( provider, useHardware, maxBuffers ),
      mName( StringTable->insert( name ) ), 
      mOpenAL( openal ), 
      mDevice( NULL ), 
      mContext( NULL )
{
   mMaxBuffers = getMax( maxBuffers, 8 );

   // TODO: The OpenAL device doesn't set the primary buffer
   // $pref::SFX::frequency or $pref::SFX::bitrate!

   mDevice = mOpenAL.alcOpenDevice( name );
   mOpenAL.alcGetError( mDevice );
   if( mDevice ) 
   {
      mContext = mOpenAL.alcCreateContext( mDevice, NULL );

      if( mContext ) 
         mOpenAL.alcMakeContextCurrent( mContext );

      U32 err = mOpenAL.alcGetError( mDevice );
      
      if( err != ALC_NO_ERROR )
         Con::errorf( "SFXALDevice - Initialization Error: %s", mOpenAL.alcGetString( mDevice, err ) );
   }

   AssertFatal( mDevice != NULL && mContext != NULL, "Failed to create OpenAL device and/or context!" );
}


SFXALDevice::~SFXALDevice()
{
   SFXALVoiceVector::iterator voice = mVoices.begin();
   for ( ; voice != mVoices.end(); voice++ )
      delete (*voice);
   mVoices.clear();

   SFXALBufferVector::iterator buffer = mBuffers.begin();
   for ( ; buffer != mBuffers.end(); buffer++ )
      delete (*buffer);
   mBuffers.clear();

   mOpenAL.alcMakeContextCurrent( NULL );
	mOpenAL.alcDestroyContext( mContext );
	mOpenAL.alcCloseDevice( mDevice );
}

SFXBuffer* SFXALDevice::createBuffer( SFXProfile *profile )
{
   AssertFatal( profile, "SFXALDevice::createBuffer() - Got null profile!" );

   SFXALBuffer* buffer = SFXALBuffer::create(   mOpenAL, 
                                                profile, 
                                                mUseHardware );
   if ( !buffer )
      return NULL;

   mBuffers.push_back( buffer );
   return buffer;
}

SFXVoice* SFXALDevice::createVoice( SFXBuffer *buffer )
{
   // Don't bother going any further if we've 
   // exceeded the maximum voices.
   if ( mVoices.size() >= mMaxBuffers )
      return NULL;

   AssertFatal( buffer, "SFXALDevice::createVoice() - Got null buffer!" );

   SFXALBuffer* alBuffer = dynamic_cast<SFXALBuffer*>( buffer );
   AssertFatal( alBuffer, "SFXALDevice::createVoice() - Got bad buffer!" );

   SFXALVoice* voice = SFXALVoice::create( alBuffer );
   if ( !voice )
      return NULL;

   mVoices.push_back( voice );
	return voice;
}

void SFXALDevice::deleteVoice( SFXVoice* voice )
{
   AssertFatal( voice, "SFXALDevice::deleteVoice() - Got null voice!" );

   SFXALVoice* alVoice = dynamic_cast<SFXALVoice*>( voice );
   AssertFatal( alVoice, "SFXALDevice::deleteVoice() - Got bad voice!" );

	// Find the buffer...
	SFXALVoiceVector::iterator iter = find( mVoices.begin(), mVoices.end(), alVoice );
	AssertFatal( iter != mVoices.end(), "SFXALDevice::deleteVoice() - Got unknown voice!" );

	mVoices.erase( iter );
	delete alVoice;
}

void SFXALDevice::update( const SFXListener& listener )
{
   const MatrixF& transform = listener.getTransform();
   Point3F pos, tupple[2];
   transform.getColumn( 3, &pos );
   transform.getColumn( 1, &tupple[0] );
   transform.getColumn( 2, &tupple[1] );

   const VectorF& velocity = listener.getVelocity();

   mOpenAL.alListenerfv( AL_POSITION, (F32 *)pos );
   mOpenAL.alListenerfv( AL_VELOCITY, (F32 *)velocity );
   mOpenAL.alListenerfv( AL_ORIENTATION, (F32 *)&tupple[0] );
   
   // In OpenAL if you have sources that you don't want to have a doppler effect,
   // you have to set the source's velocity values to the same as the listener.
   for (S32 i = 0; i < mVoices.size(); i++)
   {
      if (mVoices[i]->is3D())
         mVoices[i]->setVelocity(velocity);
   }

}