//-----------------------------------------------------------------------------
// Torque Game Engine Advanced
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _SFXXAUDIODEVICE_H_
#define _SFXXAUDIODEVICE_H_

class SFXProvider;

#ifndef _SFXDEVICE_H_
#  include "sfx/sfxDevice.h"
#endif

#ifndef _SFXPROVIDER_H_
#  include "sfx/sfxProvider.h"
#endif

#ifndef _SFXXAUDIOBUFFER_H_
#  include "sfx/xaudio/sfxXAudioBuffer.h"
#endif

//------------------------------------------------------------------------------

#ifndef SAFE_RELEASE
#define SAFE_RELEASE(a) if( (a) != NULL ) (a)->Release(); (a) = NULL;
#endif


#define SFX_XAUDIO_CHANNELCOUNT 6

#if _XDK_VER < 2638
// Speaker channel masks
const DWORD SPEAKER_FRONT_LEFT      = 0x01;
const DWORD SPEAKER_FRONT_RIGHT     = 0x02;
const DWORD SPEAKER_FRONT_CENTER    = 0x04;
const DWORD SPEAKER_LOW_FREQUENCY   = 0x08;
const DWORD SPEAKER_BACK_LEFT       = 0x10;
const DWORD SPEAKER_BACK_RIGHT      = 0x20;
#endif

extern XAUDIOREVERBI3DL2SETTINGS g_pReverbFxParams [];
extern const XAUDIOFXINIT reverbEffectInit;
extern const XAUDIOFXINIT* g_pSourceVoiceEffectsInit[];
extern const XAUDIOVOICEFXCHAIN g_SourceEffectsChain;
extern FLOAT g_EmitterAzimuths[];

//------------------------------------------------------------------------------

class SFXXAudioDevice : public SFXDevice
{
   typedef SFXDevice Parent;

   public:
      SFXXAudioDevice( SFXProvider* provider, const char* name, bool useHardware, S32 maxBuffers );
      virtual ~SFXXAudioDevice();

   protected:
      const StringTableEntry mName;
      SFXXAudioBufferVector mBuffers;

   public:

      const char* getName() const { return mName; }

      // TODO: add a duplicateBuffer which uses DuplicateSoundBuffer!

      SFXBuffer* createBuffer( bool is3d, U32 channels, U32 frequency, U32 bitsPerSample, U32 dataSize );

      void deleteBuffer( SFXBuffer* buffer );

      S32 getBufferCount() const { return mBuffers.size(); }

      void update( const SFXListener& listener );
};

#endif // _SFXXAUDIODEVICE_H_