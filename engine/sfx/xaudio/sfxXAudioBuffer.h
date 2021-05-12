//-----------------------------------------------------------------------------
// Torque Game Engine Advanced
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _SFXXAUDIOBUFFER_H_
#define _SFXXAUDIOBUFFER_H_

#include "platformXbox/platformXbox.h"
#include <xaudio.h>
#include <x3daudio.h>

#ifndef _SFXBUFFER_H_
   #include "sfx/sfxBuffer.h"
#endif

class SFXSource;

class SFXXAudioBuffer : public SFXBuffer
{
   friend class SFXXAudioDevice;

   protected:
      SFXXAudioBuffer( bool is3d, U32 channels, U32 frequency, U32 bitsPerSample, U32 dataSize );
      IXAudioSourceVoice *mSourceVoice;
      XAUDIOPACKET mPacket;

      U32 mDataSize;
      void *mAlignedData;

   public:

      virtual ~SFXXAudioBuffer();

      bool copyData( U32 offset, const U8* data, U32 length );

      void setPosition( U32 pos );

      void setMinMaxDistance( F32 min, F32 max );

      bool isPlaying() const;

      void play( bool looping );
      void stop();

      void setVelocity( const VectorF& velocity );
      void setTransform( const MatrixF& transform );
      void setVolume( F32 volume );
      void setPitch( F32 pitch );
};


/// A vector of SFXXAudioBuffer pointers.
typedef Vector<SFXXAudioBuffer*> SFXXAudioBufferVector;

#endif // _SFXXAUDIOBUFFER_H_