//-----------------------------------------------------------------------------
// Torque Game Engine Advanced
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _SFXALVOICE_H_
#define _SFXALVOICE_H_

#ifndef _SFXVOICE_H_
   #include "sfx/sfxVoice.h"
#endif

#ifndef _OPENALFNTABLE
#  include "sfx/openal/LoadOAL.h"
#endif

class SFXALBuffer;


class SFXALVoice : public SFXVoice
{
   friend class SFXALDevice;

   protected:

      SFXALVoice( const OPENALFNTABLE &oalft,
                  SFXALBuffer *buffer, 
                  ALuint bufferName,
                  ALuint sourceName );

      bool mIsPlaying;

      ALuint mBufferName;

      ALuint mSourceName;

      /// Buggy OAL jumps around when pausing.  Save playback cursor here.
      F32 mResumeAtSampleOffset;

      bool mIs3D;

      const OPENALFNTABLE &mOpenAL;

   public:

      static SFXALVoice* create( SFXALBuffer *buffer );

      virtual ~SFXALVoice();

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

      bool is3D() { return mIs3D; }
};


/// A vector of SFXALVoice pointers.
typedef Vector<SFXALVoice*> SFXALVoiceVector;

#endif // _SFXALVOICE_H_