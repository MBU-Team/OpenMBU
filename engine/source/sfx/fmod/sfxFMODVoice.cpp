//-----------------------------------------------------------------------------
// Torque Game Engine Advanced
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "platform/platform.h"
#include "sfx/fmod/sfxFMODVoice.h"

#include "sfx/fmod/sfxFMODBuffer.h"
#include "sfx/fmod/sfxFMODDevice.h"


SFXFMODVoice* SFXFMODVoice::create( SFXFMODDevice *device,
                                    SFXFMODBuffer *buffer )
{
   AssertFatal( device, "SFXFMODVoice::create() - Got null device!" );
   AssertFatal( buffer, "SFXFMODVoice::create() - Got null buffer!" );

   SFXFMODVoice* voice = new SFXFMODVoice( device, buffer );

   /*
   // A voice should have a channel assigned 
   // for its entire lifetime.
   if ( !voice->_assignChannel() )
   {
      delete voice;
      return NULL;
   }
   */

   return voice;
}

SFXFMODVoice::SFXFMODVoice(   SFXFMODDevice *device, 
                              SFXFMODBuffer *buffer )
   :  mDevice( device ),
      mBuffer( buffer ),
      mChannel( NULL )
{
   AssertFatal( device, "SFXFMODVoice::SFXFMODVoice() - No device assigned!" );
   AssertFatal( buffer, "SFXFMODVoice::SFXFMODVoice() - No buffer assigned!" );
   AssertFatal( mBuffer->mSound != NULL, "SFXFMODVoice::SFXFMODVoice() - No sound assigned!" );

   // Assign a channel when the voice is created.
   _assignChannel();
}

SFXFMODVoice::~SFXFMODVoice()
{
	stop();
}

bool SFXFMODVoice::_assignChannel()
{
   AssertFatal( mBuffer->mSound != NULL, "SFXFMODVoice::_assignChannel() - No sound assigned!" );

   // we start playing it now in the paused state, so that we can immediately set attributes that
   // depend on having a channel (position, volume, etc).  According to the FMod docs
   // it is ok to do this.
   return SFXFMODDevice::smFunc->FMOD_System_PlaySound(
      SFXFMODDevice::smSystem, 
      FMOD_CHANNEL_FREE, 
      mBuffer->mSound, 
      true, 
      &mChannel ) == FMOD_OK;
}

void SFXFMODVoice::setPosition( U32 pos )
{
	if ( !mChannel )
		return;

	// Position is in bytes.
	//FModAssert(SFXFMODDevice::smFunc->FMOD_Channel_SetPosition(mChannel, pos, FMOD_TIMEUNIT_PCMBYTES), "SFXFMODBuffer::setPosition - Failed to set position in buffer!");
	SFXFMODDevice::smFunc->FMOD_Channel_SetPosition(mChannel, pos, FMOD_TIMEUNIT_PCMBYTES);
}

void SFXFMODVoice::setMinMaxDistance( F32 min, F32 max )
{
	if ( !mChannel || !( mBuffer->mMode & FMOD_3D ) )
		return;

   FModAssert(SFXFMODDevice::smFunc->FMOD_Channel_Set3DMinMaxDistance(mChannel, min, max), "SFXFMODBuffer::setMinMaxDistance - Failed to set min/max distance!");
	//FModAssert(SFXFMODDevice::smFunc->FMOD_Sound_Set3DMinMaxDistance(mSound, min, max), "SFXFMODBuffer::setMinMaxDistance - Failed to set min/max distance!");
}

SFXStatus SFXFMODVoice::getStatus() const
{
   // No channel... then we're stopped!
   if ( !mChannel )
      return SFXStatusStopped;

	FMOD_BOOL isTrue = false;
   SFXFMODDevice::smFunc->FMOD_Channel_GetPaused( mChannel, &isTrue );
   if ( isTrue )
      return SFXStatusPaused;

   SFXFMODDevice::smFunc->FMOD_Channel_IsPlaying( mChannel, &isTrue );
   if ( isTrue )
      return SFXStatusPlaying;

   return SFXStatusStopped;
}

void SFXFMODVoice::play( bool looping )
{
   S32 loopMode = looping ? -1 : 0;

   if ( !mChannel )
      _assignChannel();

   // set channel props and unpause the sound
   FMOD_MODE mode = mDevice->get3dRollOffMode();
   mode |= (looping ? FMOD_LOOP_NORMAL : FMOD_LOOP_OFF);

   FModAssert(SFXFMODDevice::smFunc->FMOD_Channel_SetMode(mChannel, mode), "SFXFMODBuffer::play - failed to set mode on sound.");
   FModAssert(SFXFMODDevice::smFunc->FMOD_Channel_SetLoopCount(mChannel, loopMode), "SFXFMODBuffer::play - Failed to set looping!");

   FModAssert(
      (SFXFMODDevice::smFunc->FMOD_Channel_SetPaused(mChannel, false)), 
      "SFXFMODBuffer::play - Failed to unpause sound!");
}

void SFXFMODVoice::stop()
{
	if ( !mChannel )
		return;

	// Failing on this is OK as it can happen if the sound has already 
	// stopped and in other non-critical cases. Note the commented version
	// with assert, in case you want to re-enable it:
	//FModAssert(SFXFMODDevice::smFunc->FMOD_Channel_Stop(mChannel), "SFXFMODBuffer::stop - failed to stop channel!");
	SFXFMODDevice::smFunc->FMOD_Channel_Stop(mChannel);
	mChannel = NULL;
}

void SFXFMODVoice::pause()
{
	if ( !mChannel )
		return;

	SFXFMODDevice::smFunc->FMOD_Channel_SetPaused( mChannel, true );
}

void SFXFMODVoice::setVelocity( const VectorF& velocity )
{
	if ( !mChannel || !( mBuffer->mMode & FMOD_3D ) )
		return;

	// Note we have to do a handedness swap; see the listener update code
	// in SFXFMODDevice for details.
	FMOD_VECTOR vec;
	vec.x = velocity.x;
	vec.y = velocity.z;
	vec.z = velocity.y;

	//FModAssert(SFXFMODDevice::smFunc->FMOD_Channel_Set3DAttributes(mChannel, NULL, &vec), "SFXFMODBuffer::setVelocity - failed to set channel 3d velocity!");
	SFXFMODDevice::smFunc->FMOD_Channel_Set3DAttributes(mChannel, NULL, &vec);
}

void SFXFMODVoice::setTransform( const MatrixF& transform )
{
	if ( !mChannel || !( mBuffer->mMode & FMOD_3D ) )
		return;

	// Note we have to do a handedness swap; see the listener update code
	// in SFXFMODDevice for details.
	Point3F pos = transform.getPosition();

	FMOD_VECTOR vec;
	vec.x = pos.x;
	vec.y = pos.z;
	vec.z = pos.y;

	// This can fail safe, so don't assert if it fails.
	//FModAssert(SFXFMODDevice::smFunc->FMOD_Channel_Set3DAttributes(mChannel, &vec, NULL), "SFXFMODBuffer::setTransform - failed to set channel 3d transform!");
	SFXFMODDevice::smFunc->FMOD_Channel_Set3DAttributes(mChannel, &vec, NULL);
}

void SFXFMODVoice::setVolume( F32 volume )
{
	if ( !mChannel )
		return;

	// This can fail safe, so don't assert if it fails.
	//FModAssert(SFXFMODDevice::smFunc->FMOD_Channel_SetVolume(mChannel, volume), "SFXFMODBuffer::setVolume - failed to set channel volume!");
	SFXFMODDevice::smFunc->FMOD_Channel_SetVolume(mChannel, volume);
}

void SFXFMODVoice::setPitch( F32 pitch )
{
	if ( !mChannel )
		return;

   // if we do not know the frequency, we cannot change the pitch
   F32 frequency = mBuffer->mResource->getFrequency();
   if ( frequency == 0 )
      return;

	// Scale the original frequency by the pitch factor.
	//FModAssert(SFXFMODDevice::smFunc->FMOD_Channel_SetFrequency(mChannel, mFrequency * pitch), "SFXFMODBuffer::setPitch - failed to set channel frequency!");
	SFXFMODDevice::smFunc->FMOD_Channel_SetFrequency(mChannel, frequency * pitch);
}
