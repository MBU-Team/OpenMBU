//-----------------------------------------------------------------------------
// Torque Game Engine Advanced
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "platform/platform.h"

#include "sfx/null/sfxNullDevice.h"
#include "sfx/null/sfxNullBuffer.h"
#include "sfx/sfxListener.h"


SFXNullDevice::SFXNullDevice( SFXProvider* provider, 
                              const char* name, 
                              bool useHardware, 
                              S32 maxBuffers )

   :  SFXDevice( provider, useHardware, maxBuffers ),
      mName( StringTable->insert( name ) )
{
   mMaxBuffers = getMax( maxBuffers, 8 );
}

SFXNullDevice::~SFXNullDevice()
{
   SFXNullBufferVector::iterator iter = mBuffers.begin();
   for ( ; iter != mBuffers.end(); iter++ )
      delete (*iter);

   mBuffers.clear();
}

SFXBuffer* SFXNullDevice::createBuffer( SFXProfile *profile )
{
   AssertFatal( profile, "SFXNullDevice::createBuffer() - Got null profile!" );

   SFXNullBuffer* buffer = new SFXNullBuffer();
   if ( !buffer )
      return NULL;

   mBuffers.push_back( buffer );
   return buffer;
}

SFXVoice* SFXNullDevice::createVoice( SFXBuffer *buffer )
{
   // Don't bother going any further if we've 
   // exceeded the maximum voices.
   if ( mVoices.size() >= mMaxBuffers )
      return NULL;

   AssertFatal( buffer, "SFXNullDevice::createVoice() - Got null buffer!" );

   SFXNullBuffer* nullBuffer = dynamic_cast<SFXNullBuffer*>( buffer );
   AssertFatal( nullBuffer, "SFXNullDevice::createVoice() - Got bad buffer!" );

   SFXNullVoice* voice = new SFXNullVoice();
   if ( !voice )
      return NULL;

   mVoices.push_back( voice );
	return voice;
}

void SFXNullDevice::deleteVoice( SFXVoice* voice )
{
   AssertFatal( voice, "SFXNullDevice::deleteVoice() - Got null voice!" );

   SFXNullVoice* nullVoice = dynamic_cast<SFXNullVoice*>( voice );
   AssertFatal( nullVoice, "SFXNullDevice::deleteVoice() - Got bad voice!" );

	// Find the buffer...
	SFXNullVoiceVector::iterator iter = find( mVoices.begin(), mVoices.end(), nullVoice );
	AssertFatal( iter != mVoices.end(), "SFXNullDevice::deleteVoice() - Got unknown voice!" );

	mVoices.erase( iter );
	delete nullVoice;
}

void SFXNullDevice::update( const SFXListener& listener )
{
}