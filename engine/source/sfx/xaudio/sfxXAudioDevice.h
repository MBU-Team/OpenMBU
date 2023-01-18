//-----------------------------------------------------------------------------
// Torque Game Engine Advanced
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _SFXXAUDIODEVICE_H_
#define _SFXXAUDIODEVICE_H_

class SFXProvider;

#ifndef _SFXDEVICE_H_
#include "sfx/sfxDevice.h"
#endif

#ifndef _SFXPROVIDER_H_
#include "sfx/sfxProvider.h"
#endif

#ifndef _SFXXAUDIOVOICE_H_
#include "sfx/xaudio/sfxXAudioVoice.h"
#endif

#ifndef _SFXXAUDIOBUFFER_H_
#include "sfx/xaudio/sfxXAudioBuffer.h"
#endif

#include <xaudio2.h>
#include <x3daudio.h>


class SFXXAudioDevice : public SFXDevice
{
   typedef SFXDevice Parent;

   protected:

      const StringTableEntry mName;

      /// The XAudio engine interface passed 
      /// on creation from the provider.
      IXAudio2 *mXAudio;

      /// The X3DAudio instance.
      X3DAUDIO_HANDLE mX3DAudio;

      /// The one and only mastering voice.
      IXAudio2MasteringVoice* mMasterVoice;

      /// The details of the master voice.
      XAUDIO2_VOICE_DETAILS mMasterVoiceDetails;

      /// The one listener.
      X3DAUDIO_LISTENER mListener;

      /// All the currently allocated voices.
      SFXXAudioVoiceVector mVoices;

      /// All the current audio buffers.
      SFXXAudioBufferVector mBuffers;

   public:

      SFXXAudioDevice(  SFXProvider* provider, 
                        const char* name,
                        IXAudio2 *xaudio,
                        U32 deviceIndex,
                        U32 speakerChannelMask,
                        U32 maxBuffers );

      virtual ~SFXXAudioDevice();

      // SFXDevice
      const char* getName() const { return mName; }
      SFXBuffer* createBuffer( SFXProfile *profile );
      SFXVoice* createVoice( SFXBuffer *buffer );
      void deleteVoice( SFXVoice* buffer );
      U32 getVoiceCount() const { return mVoices.size(); }
      void update( const SFXListener& listener );

      /// Called from the voice when its about to start playback.
      void _setOutputMatrix( SFXXAudioVoice *voice );
};

#endif // _SFXXAUDIODEVICE_H_