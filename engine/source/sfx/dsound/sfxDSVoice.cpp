//-----------------------------------------------------------------------------
// Torque Game Engine Advanced
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "platform/platform.h"
#include "sfx/dsound/sfxDSVoice.h"

#include "sfx/dsound/sfxDSDevice.h"
#include "util/safeRelease.h"


SFXDSVoice* SFXDSVoice::create( SFXDSBuffer *buffer )
{
   AssertFatal( buffer, "SFXDSVoice::create() - Got null buffer!" );

   IDirectSoundBuffer8 *dsBuffer8 = NULL;
   if ( !buffer->createVoice( &dsBuffer8 ) || !dsBuffer8 )
      return NULL;

   // Now try to grab a 3D interface... if we don't
   // get one its probably because its not a 3d sound.
   IDirectSound3DBuffer8* dsBuffer3d8 = NULL;
   dsBuffer8->QueryInterface( IID_IDirectSound3DBuffer8, (LPVOID*)&dsBuffer3d8 );

   // There is a known issue to work around.  From the DX docs...
   //
   //   There is a known issue with volume levels of duplicated buffers. The
   //   duplicated buffer will play at full volume unless you change the volume
   //   to a different value than the original buffer's volume setting. If the 
   //   volume stays the same (even if you explicitly set the same volume in 
   //   the duplicated buffer with a IDirectSoundBuffer8::SetVolume call), the
   //   buffer will play at full volume regardless. To work around this problem,
   //   immediately set the volume of the duplicated buffer to something slightly
   //   different than what it was, even if you change it one millibel. The volume
   //   may then be immediately set back again to the original desired value.
   //
   LONG volume;
   dsBuffer8->GetVolume( &volume );
   dsBuffer8->SetVolume( volume == DSBVOLUME_MAX ? 
                           DSBVOLUME_MAX - 1 : DSBVOLUME_MAX );
   dsBuffer8->SetVolume( DSBVOLUME_MAX );

   // Create the voice and return!
   SFXDSVoice* voice = new SFXDSVoice( buffer,
                                       dsBuffer8,
                                       dsBuffer3d8 );

   return voice;
}

SFXDSVoice::SFXDSVoice( SFXDSBuffer *buffer,
                        IDirectSoundBuffer8 *dsBuffer, 
                        IDirectSound3DBuffer8 *dsBuffer3d )
   :  mBuffer( buffer ),
      mDSBuffer( dsBuffer ),
      mDSBuffer3D( dsBuffer3d ),
      mFrequency( 0 ),
      mPaused( false )
{
   AssertFatal( mBuffer, "SFXDSVoice::SFXDSVoice() - SFXDSBuffer is null!" );
   AssertFatal( mDSBuffer, "SFXDSVoice::SFXDSVoice() - Dsound buffer is null!" );

   mDSBuffer->GetFrequency( (LPDWORD)&mFrequency );
}

SFXDSVoice::~SFXDSVoice()
{
   setPitch(1.0f);
   SAFE_RELEASE( mDSBuffer3D );
   mBuffer->releaseVoice( &mDSBuffer );

   mBuffer = NULL;
}

void SFXDSVoice::setPosition( U32 pos )
{
   mDSBuffer->SetCurrentPosition( pos );
}

void SFXDSVoice::setMinMaxDistance( F32 min, F32 max )
{
   if ( !mDSBuffer3D )
      return;

   mDSBuffer3D->SetMinDistance( min, DS3D_DEFERRED );
   mDSBuffer3D->SetMaxDistance( max, DS3D_DEFERRED );
}

SFXStatus SFXDSVoice::getStatus() const
{
   if ( mPaused )
      return SFXStatusPaused;

   DWORD status = 0;
   mDSBuffer->GetStatus( &status );

   if ( status & DSBSTATUS_PLAYING )
      return SFXStatusPlaying;

   return SFXStatusStopped;
}

void SFXDSVoice::play( bool looping )
{
   DSAssert( mDSBuffer->Play( 0, 0, looping ? DSBPLAY_LOOPING : 0 ), 
      "SFXDSVoice::play() - Playback failed!" );
   mPaused = false;
}

void SFXDSVoice::pause()
{
   DSAssert( mDSBuffer->Stop(), "SFXDSVoice::pause - stop failed!" );
   mPaused = true;
}

void SFXDSVoice::stop()
{
   DSAssert( mDSBuffer->Stop(), "SFXDSVoice::stop - stop failed!" );
   mDSBuffer->SetCurrentPosition( 0 );
   mPaused = false;
}

void SFXDSVoice::setVelocity( const VectorF& velocity )
{
   if ( !mDSBuffer3D )
      return;

   DSAssert( mDSBuffer3D->SetVelocity( velocity.x, velocity.z, velocity.y, DS3D_DEFERRED ), 
      "SFXDSVoice::setVelocity - couldn't update buffer!" );
}

void SFXDSVoice::setTransform( const MatrixF& transform )
{
   if ( !mDSBuffer3D )
      return;

   Point3F pos, dir;
   transform.getColumn( 3, &pos );
   transform.getColumn( 1, &dir );
   DSAssert( mDSBuffer3D->SetPosition( pos.x, pos.z, pos.y, DS3D_DEFERRED ), 
      "SFXDSVoice::setTransform - couldn't set position of the buffer." );

   // Need to hook in directional sound sources someday? -- TODO
   //hr = mBuffer3d->SetConeOrientation( dir.x, dir.z, dir.y, DS3D_DEFERRED );
}

void SFXDSVoice::setVolume( F32 volume )
{
   // We must convert the linear volume into logrithmic for dsound.
   LONG logVolume;
   if ( volume <= 0.0f )
      logVolume = DSBVOLUME_MIN;
   else
   {
      logVolume = -2000.0 * mLog( 1.0f / volume );
      logVolume = mClamp( logVolume, DSBVOLUME_MIN, DSBVOLUME_MAX );
   }

   HRESULT hr = mDSBuffer->SetVolume( logVolume );
   DSAssert( hr, "SFXDSVoice::setVolume - couldn't set volume!" );
}

void SFXDSVoice::setPitch( F32 pitch )
{ 
   F32 frequency = mFloor( mClampF( mFrequency * pitch, DSBFREQUENCY_MIN, DSBFREQUENCY_MAX ) );

   DSAssert( mDSBuffer->SetFrequency( (U32)frequency ), 
      "SFXDSVoice::setPitch - couldn't set playback frequency.");
}
