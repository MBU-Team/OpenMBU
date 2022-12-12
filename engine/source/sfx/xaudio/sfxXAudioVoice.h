//-----------------------------------------------------------------------------
// Torque Game Engine Advanced
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _SFXXAUDIOVOICE_H_
#define _SFXXAUDIOVOICE_H_

#ifndef _SFXVOICE_H_
   #include "sfx/sfxVoice.h"
#endif

#include <xaudio2.h>
#include <x3daudio.h>

class SFXXAudioBuffer;


class SFXXAudioVoice : public SFXVoice, public IXAudio2VoiceCallback
{
   friend class SFXXAudioDevice;

   protected:

      /// This constructor does not create a valid voice.
      /// @see SFXXAudioVoice::create
      SFXXAudioVoice();

      /// The device that created us.
      SFXXAudioDevice *mDevice;

      /// The XAudio voice.
      IXAudio2SourceVoice *mVoice;

      /// The buffer we're currently playing.
      SFXXAudioBuffer *mBuffer;

      /// Used to know what sounds need 
      /// positional updates.
      bool mIs3D;

      /// Since 3D sounds are pitch shifted for doppler
      /// effect we need to track the users base pitch.
      F32 mPitch;

      /// This is how we track our status.
      mutable SFXStatus mStatus;

      /// This event is tripped by the XAudio thread callback
      /// when a source finishes playback.
      HANDLE mStopEvent;

      /// The cached X3DAudio emitter data.
      X3DAUDIO_EMITTER mEmitter;

      // IXAudio2VoiceCallback
      void STDMETHODCALLTYPE OnStreamEnd();
      void STDMETHODCALLTYPE OnVoiceProcessingPassEnd() {}
      void STDMETHODCALLTYPE OnVoiceProcessingPassStart(UINT32 BytesRequired) {}
      void STDMETHODCALLTYPE OnBufferEnd( void *bufferContext ) {}
      void STDMETHODCALLTYPE OnBufferStart( void *bufferContext ) {}
      void STDMETHODCALLTYPE OnLoopEnd( void *bufferContext ) {}
      void STDMETHODCALLTYPE OnVoiceError( void * bufferContext, HRESULT error ) {}

   public:

      ///
      static SFXXAudioVoice* create(   IXAudio2 *xaudio,
                                       bool is3D,
                                       SFXXAudioBuffer *buffer );

      ///
      virtual ~SFXXAudioVoice();

      // SFXVoice
      void setPosition( U32 pos );
      void setMinMaxDistance( F32 min, F32 max );
      void play( bool looping );
      void pause();
      void stop();
      SFXStatus getStatus() const;
      void setVelocity( const VectorF& velocity );
      void setTransform( const MatrixF& transform );
      void setVolume( F32 volume );
      void setPitch( F32 pitch );
      void setCone( F32 innerAngle, F32 outerAngle, F32 outerVolume );

      /// Is this a 3D positional voice.
      bool is3D() const { return mIs3D; }

      ///
      const X3DAUDIO_EMITTER& getEmitter() const { return mEmitter; }
};


/// A vector of SFXXAudioVoice pointers.
typedef Vector<SFXXAudioVoice*> SFXXAudioVoiceVector;

#endif // _SFXXAUDIOVOICE_H_