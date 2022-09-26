//-----------------------------------------------------------------------------
// Torque Game Engine Advanced
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _SFXXAUDIOBUFFER_H_
#define _SFXXAUDIOBUFFER_H_

#ifndef _SFXBUFFER_H_
   #include "sfx/sfxBuffer.h"
#endif
#ifndef _SFXRESOURCE_H_
   #include "sfx/sfxResource.h"
#endif

#include <xaudio2.h>

class SFXProfile;


class SFXXAudioBuffer : public SFXBuffer
{
   friend class SFXXAudioDevice;

   protected:

      ///
      Resource<SFXResource> mResource;

      /// The buffer struct used by the XAudio voices.
      XAUDIO2_BUFFER mBuffer;

      bool mIs3d;

      ///
      SFXXAudioBuffer( const Resource<SFXResource> &resource, bool is3d );
      virtual ~SFXXAudioBuffer();

   public:

      ///
      static SFXXAudioBuffer* create( SFXProfile *profile );

      ///
      void getFormat( WAVEFORMATEX *wft ) const;
      
      ///
      const XAUDIO2_BUFFER& getData() const { return mBuffer; }
};


/// A vector of SFXXAudioBuffer pointers.
typedef Vector<SFXXAudioBuffer*> SFXXAudioBufferVector;

#endif // _SFXXAUDIOBUFFER_H_