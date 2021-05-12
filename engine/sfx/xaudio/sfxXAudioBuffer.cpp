//-----------------------------------------------------------------------------
// Torque Game Engine Advanced
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "sfx/xaudio/sfxXAudioBuffer.h"
#include "sfx/xaudio/sfxXAudioDevice.h"
#include "util/safeDelete.h"
#include <xaudio.h>
#include "core/frameAllocator.h"

#define SFX_MAX_PITCH_SHIFT 4

SFXXAudioBuffer::SFXXAudioBuffer( bool is3d, U32 channels, U32 frequency, U32 bitsPerSample, U32 dataSize ) :
   mSourceVoice( NULL ), mDataSize( dataSize ), mAlignedData( NULL )
{
   // Source Voice Config
   XAUDIOSOURCEVOICEINIT SourceVoiceInit = { 0 };

   switch( bitsPerSample )
   {
      case 8:
         SourceVoiceInit.Format.SampleType = XAUDIOSAMPLETYPE_8BITPCM;
         break;
      case 16:
         SourceVoiceInit.Format.SampleType = XAUDIOSAMPLETYPE_16BITPCM;
         break;
      default:
         AssertFatal( false, "Unknown Sample Type" );
   }

   SourceVoiceInit.Format.ChannelCount = (XAUDIOCHANNEL)channels;
   SourceVoiceInit.Format.SampleRate = (XAUDIOSAMPLERATE)frequency;
   SourceVoiceInit.MaxPacketCount = 1;
   SourceVoiceInit.MaxPitchShift = SFX_MAX_PITCH_SHIFT;

   XAudioCreateSourceVoice( &SourceVoiceInit, &mSourceVoice );

   // See biscuit below -patw
   mAlignedData = XPhysicalAlloc( dataSize, MAXULONG_PTR, 2048, PAGE_READWRITE );
}

SFXXAudioBuffer::~SFXXAudioBuffer()
{
   SAFE_RELEASE( mSourceVoice );
   XPhysicalFree( mAlignedData );
}

bool SFXXAudioBuffer::copyData( U32 offset, const U8* data, U32 length )
{
   AssertFatal( length == mDataSize, "Data size mismatch" );

   // So this is data from somewhere else, from the XDK docs:
   // "XMA data needs to be allocated in physical memory and aligned on 2048 byte boundaries."
   // So what we are going to do is allocate a chunk of memory and then copy the
   // data into it, so the data can be killed by whatever called this.
   AssertFatal( mAlignedData != NULL, "BLAAAAAAAAARRRRRRRRRRRRRRRRGGGGGGGGGGGGGH!" );
   XMemCpy( mAlignedData, data, length );

   mPacket.pBuffer = (U8 *)mAlignedData + offset;
   mPacket.BufferSize = length;
   mPacket.LoopStart = offset;
   mPacket.LoopLength = length - offset;

   return true;
}

void SFXXAudioBuffer::setPosition( U32 pos )
{
// TODO: 3d Implementation
}

void SFXXAudioBuffer::setMinMaxDistance( F32 min, F32 max )
{
// TODO: 3d Implementation
}

bool SFXXAudioBuffer::isPlaying() const
{
   FrameTemp<XAUDIOSOURCESTATE> sourceState;
   mSourceVoice->GetVoiceState( sourceState );

   return *sourceState == XAUDIOSOURCESTATE_STARTED;
}

void SFXXAudioBuffer::play( bool looping )
{
   // Set up loop stuff
   mPacket.LoopCount = ( looping ? XAUDIOLOOPCOUNT_INFINITE : 0 );
   mPacket.pContext = NULL;

   mSourceVoice->SubmitPacket( &mPacket, ( looping ? 0 : XAUDIOSUBMITPACKET_DISCONTINUITY ) );
   mSourceVoice->Start( 0 );
}

void SFXXAudioBuffer::stop()
{
   mSourceVoice->Stop( 0 );
}

void SFXXAudioBuffer::setVelocity( const VectorF& velocity )
{
   // TODO: 3d Implementation
}

void SFXXAudioBuffer::setTransform( const MatrixF& transform )
{
// TODO: 3d Implementation
}

void SFXXAudioBuffer::setVolume( F32 volume )
{
#ifdef TORQUE_DEBUG
   if( volume > XAUDIOVOLUME_MAX )
      Con::warnf( "XAudio Warning! Call to setVolume over max volume, clipping could occur." );
#endif
   mSourceVoice->SetVolume( (XAUDIOVOLUME)volume );
}

void SFXXAudioBuffer::setPitch( F32 pitch )
{
   // XAudio wants pitch shift in octaves, so start w/ 0
   pitch -= 1.f; 
#ifdef TORQUE_DEBUG
   if( pitch > SFX_MAX_PITCH_SHIFT )
      Con::warnf( "XAudio Warning! Increase max pitch shift to accomodate the value %f", pitch );
#endif

   mSourceVoice->SetPitch( (XAUDIOPITCH)pitch );
}

