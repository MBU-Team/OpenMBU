//-----------------------------------------------------------------------------
// Torque Game Engine Advanced
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "platformXbox/platformXbox.h"
#include "sfx/xaudio/sfxXAudioDevice.h"
#include "sfx/xaudio/sfxXAudioBuffer.h"
#include "sfx/sfxListener.h"


SFXXAudioDevice::SFXXAudioDevice( SFXProvider* provider, const char* name, bool useHardware, S32 maxBuffers )
   :  SFXDevice( provider, useHardware, maxBuffers ),
      mName( StringTable->insert( name ) )
{
   mMaxBuffers = getMax( maxBuffers, 8 );
}


SFXXAudioDevice::~SFXXAudioDevice()
{
   SFXXAudioBufferVector::iterator iter = mBuffers.begin();
   for ( ; iter != mBuffers.end(); iter++ )
      delete (*iter);

   mBuffers.clear();
}


SFXBuffer* SFXXAudioDevice::createBuffer( bool is3d, U32 channels, U32 frequency, U32 bitsPerSample, U32 dataSize )
{
   // Don't bother going any further if we've exceeded
   // the maximum buffers and we're in software mode.
   if ( mBuffers.size() >= mMaxBuffers )
      return NULL;

   // Return the buffer.
   SFXXAudioBuffer* buffer = new SFXXAudioBuffer( is3d, channels, frequency, bitsPerSample, dataSize );
   mBuffers.push_back( buffer );
   return buffer;
}

void SFXXAudioDevice::deleteBuffer( SFXBuffer* buffer )
{
   SFXXAudioBufferVector::iterator iter = find( mBuffers.begin(), mBuffers.end(), buffer );
   AssertFatal( iter != mBuffers.end(), "Got unknown buffer!" );

   mBuffers.erase( iter );
   delete buffer;
}

void SFXXAudioDevice::update( const SFXListener& listener )
{

}