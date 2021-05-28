//-----------------------------------------------------------------------------
// Torque Game Engine Advanced
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _SFXDSVOICE_H_
#define _SFXDSVOICE_H_

#ifndef _SFXVOICE_H_
   #include "sfx/sfxVoice.h"
#endif
#ifndef _SFXDSBUFFER_H_
   #include "sfx/dsound/sfxDSBuffer.h"
#endif

#include <dsound.h>


class SFXDSVoice : public SFXVoice
{
   protected:

      SFXDSVoice( SFXDSBuffer *buffer,
                  IDirectSoundBuffer8 *dsBuffer, 
                  IDirectSound3DBuffer8 *dsBuffer3d );

      SFXDSBuffer *mBuffer;

      IDirectSoundBuffer8 *mDSBuffer;

      IDirectSound3DBuffer8 *mDSBuffer3D;

      U32 mFrequency;

      bool mPaused;

   public:

      ///
      static SFXDSVoice* create( SFXDSBuffer *buffer );

      ///
      virtual ~SFXDSVoice();

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
};


/// A vector of SFXDSVoice pointers.
typedef Vector<SFXDSVoice*> SFXDSVoiceVector;

#endif // _SFXDSBUFFER_H_