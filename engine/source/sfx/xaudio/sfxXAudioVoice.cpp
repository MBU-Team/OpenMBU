//-----------------------------------------------------------------------------
// Torque Game Engine Advanced
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "platform/platform.h"
#include "sfx/xaudio/sfxXAudioVoice.h"

#include "sfx/xaudio/sfxXAudioDevice.h"
#include "sfx/xaudio/sfxXAudioBuffer.h"
#include "util/safeDelete.h"

#include "core/realComp.h"


SFXXAudioVoice* SFXXAudioVoice::create(   IXAudio2 *xaudio,
                                          bool is3D,
                                          SFXXAudioBuffer *buffer )
{
   AssertFatal( xaudio, "SFXXAudioVoice::create() - Got null XAudio!" );
   AssertFatal( buffer, "SFXXAudioVoice::create() - Got null buffer!" );

   // Create the voice object first as it also the callback object.
   SFXXAudioVoice *voice = new SFXXAudioVoice();

   // Get the buffer format.
   WAVEFORMATEX wfx;
   buffer->getFormat( &wfx );

   // We don't support multi-channel 3d sounds!
   if ( is3D && wfx.nChannels > 1 )
      return NULL;

   // Create the voice.
   IXAudio2SourceVoice *xaVoice;
   HRESULT hr = xaudio->CreateSourceVoice(   &xaVoice, 
                                             (WAVEFORMATEX*)&wfx,
                                             0, 
                                             XAUDIO2_DEFAULT_FREQ_RATIO, 
                                             voice, 
                                             NULL, 
                                             NULL );

   if ( FAILED( hr ) || !voice )
   {
      delete voice;
      return NULL;
   }

   voice->mIs3D = is3D;
   voice->mEmitter.ChannelCount = wfx.nChannels;
   voice->mBuffer = buffer;
   voice->mVoice = xaVoice;

   return voice;
}

SFXXAudioVoice::SFXXAudioVoice()
   :  mDevice( NULL ),
      mVoice( NULL ),
      mBuffer( NULL ),
      mStatus( SFXStatusNull ),
      mIs3D( false ),
      mPitch( 1.0f ),
      mStopEvent( CreateEvent( NULL, FALSE, FALSE, NULL ) )
{
   dMemset( &mEmitter, 0, sizeof( mEmitter ) );
   mEmitter.DopplerScaler = 1.0f;
}

SFXXAudioVoice::~SFXXAudioVoice()
{
   if ( mEmitter.pVolumeCurve )
   {
      SAFE_DELETE_ARRAY( mEmitter.pVolumeCurve->pPoints );
      SAFE_DELETE( mEmitter.pVolumeCurve );
   }

   SAFE_DELETE( mEmitter.pCone );

   mVoice->DestroyVoice();
   CloseHandle( mStopEvent );
}

void SFXXAudioVoice::setPosition( U32 pos )
{
   // TODO!
}

void SFXXAudioVoice::setMinMaxDistance( F32 min, F32 max )
{
   // Set the overall volume curve scale.
   mEmitter.CurveDistanceScaler = max;

   // The curve uses normalized distances, so 
   // figure out the normalized min distance.
   F32 normMin = 0.0f;
   if ( min > 0.0f )
      normMin = min / max;

   // Have we setup the curve yet?
   if ( !mEmitter.pVolumeCurve )      
   {
      mEmitter.pVolumeCurve = new X3DAUDIO_DISTANCE_CURVE;

      // We use 6 points for our volume curve.
      mEmitter.pVolumeCurve->pPoints = new X3DAUDIO_DISTANCE_CURVE_POINT[6];
      mEmitter.pVolumeCurve->PointCount = 6;

      // The first and last points are known 
      // and will not change.
      mEmitter.pVolumeCurve->pPoints[0].Distance = 0.0f;
      mEmitter.pVolumeCurve->pPoints[0].DSPSetting = 1.0f;
      mEmitter.pVolumeCurve->pPoints[5].Distance = 1.0f;
      mEmitter.pVolumeCurve->pPoints[5].DSPSetting = 0.0f;
   }

   // Set the second point of the curve.
   mEmitter.pVolumeCurve->pPoints[1].Distance = normMin;
   mEmitter.pVolumeCurve->pPoints[1].DSPSetting = 1.0f;

   // The next three points are calculated to 
   // give the sound a rough logarithmic falloff.
   F32 distStep = ( 1.0f - normMin ) / 4.0f;
   for ( U32 i=0; i < 3; i++ )
   {
      U32 index = 2 + i;
      F32 dist = normMin + ( distStep * (F32)( i + 1 ) );

      mEmitter.pVolumeCurve->pPoints[index].Distance = dist;
      mEmitter.pVolumeCurve->pPoints[index].DSPSetting = 1.0f - log10( dist * 10.0f );
   }
}

void SFXXAudioVoice::OnStreamEnd()
{
   // Warning:  This is being called within the XAudio 
   // thread, so be sure your thread safe!

   // Just signal the simulation thread that the
   // sound had finished playback.
   SetEvent( mStopEvent );
}

SFXStatus SFXXAudioVoice::getStatus() const
{
   // First check the event... if its signaled
   // then the voice has finished playback.
   if ( WaitForSingleObject( mStopEvent, 0 ) == WAIT_OBJECT_0 )
      mStatus = SFXStatusStopped;

   return mStatus;
}

void SFXXAudioVoice::play( bool looping )
{
   if ( getStatus() != SFXStatusPaused )
   {
      // If we're not paused then we're 
      // starting from the begining.

      XAUDIO2_BUFFER buffer = mBuffer->getData();      

      if ( looping )
         buffer.LoopCount = XAUDIO2_LOOP_INFINITE;

      mVoice->SubmitSourceBuffer( &buffer );
   }

   // Give the device a chance to calculate our positional
   // audio settings before we start playback... this is 
   // importaint else we get glitches.
   if ( mIs3D )
      mDevice->_setOutputMatrix( this );

   // Start playback.
   mStatus = SFXStatusPlaying;
   ResetEvent( mStopEvent );
   mVoice->Start( 0 );
}

void SFXXAudioVoice::pause()
{
   mStatus = SFXStatusPaused;
   mVoice->Stop( 0 );
}

void SFXXAudioVoice::stop()
{
   mStatus = SFXStatusStopped;
   mVoice->Stop( 0 );
}

void SFXXAudioVoice::setVelocity( const VectorF& velocity )
{
   mEmitter.Velocity.x = velocity.x;
   mEmitter.Velocity.y = velocity.y;

   // XAudio and Torque use opposite handedness, so
   // flip the z coord to account for that.
   mEmitter.Velocity.z = -velocity.z;
}

void SFXXAudioVoice::setTransform( const MatrixF& transform )
{
   transform.getColumn( 3, (Point3F*)&mEmitter.Position );
   transform.getColumn( 1, (Point3F*)&mEmitter.OrientFront );
   transform.getColumn( 2, (Point3F*)&mEmitter.OrientTop );

   // XAudio and Torque use opposite handedness, so
   // flip the z coord to account for that.
   mEmitter.Position.z     *= -1.0f;
   mEmitter.OrientFront.z  *= -1.0f;
   mEmitter.OrientTop.z    *= -1.0f;

   // Need to hook in directional sound sources someday? -- TODO
}

void SFXXAudioVoice::setVolume( F32 volume )
{
   mVoice->SetVolume( volume );
}

void SFXXAudioVoice::setPitch( F32 pitch )
{
   mPitch = mClampF( pitch, XAUDIO2_MIN_FREQ_RATIO, XAUDIO2_DEFAULT_FREQ_RATIO );
   mVoice->SetFrequencyRatio( mPitch );
}

void SFXXAudioVoice::setCone( F32 innerAngle, F32 outerAngle, F32 outerVolume )
{
   // If the cone is set to 360 then the
   // cone is null and doesn't need to be
   // set on the voice.
   if ( isEqual( innerAngle, M_2PI_F ) )
   {
      SAFE_DELETE( mEmitter.pCone );
      return;
   }

   if ( !mEmitter.pCone )
   {
      mEmitter.pCone = new X3DAUDIO_CONE;

      // The inner volume is always 1... the overall
      // volume is what scales it.
      mEmitter.pCone->InnerVolume = 1.0f; 

      // We don't use these yet.
      mEmitter.pCone->InnerLPF = 0.0f; 
      mEmitter.pCone->OuterLPF = 0.0f; 
      mEmitter.pCone->InnerReverb = 0.0f; 
      mEmitter.pCone->OuterReverb = 0.0f; 
   }

   mEmitter.pCone->InnerAngle = innerAngle;
   mEmitter.pCone->OuterAngle = outerAngle;
   mEmitter.pCone->OuterVolume = outerVolume;
}

