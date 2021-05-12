//-----------------------------------------------------------------------------
// Torque Game Engine Advanced
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _SFXFMODVOICE_H_
#define _SFXFMODVOICE_H_

#ifndef _SFXDEVICE_H_
   #include "sfx/sfxDevice.h"
#endif
#ifndef _SFXVOICE_H_
   #include "sfx/sfxVoice.h"
#endif

#include "sfx/sfxResource.h"

#include "fmod.h"

class SFXSource;
class SFXFMODBuffer;
class SFXFMODDevice;


class SFXFMODVoice : public SFXVoice
{
   protected:

      SFXFMODDevice *mDevice;

      SFXFMODBuffer *mBuffer;

      FMOD_CHANNEL *mChannel;

      ///
	   SFXFMODVoice(  SFXFMODDevice *device, 
                     SFXFMODBuffer *buffer );

      // copy data into buffer
      bool _copyData(U32 offset, const U8* data, U32 length );

      // prep for playback
      bool _assignChannel();

   public:

      ///
      static SFXFMODVoice* create(  SFXFMODDevice *device, 
                                    SFXFMODBuffer *buffer );

      ///
      virtual ~SFXFMODVoice();

      // copy data into buffer and prep for playback
      //bool copyData( U32 offset, const U8* data, U32 length );

      void setPosition( U32 pos );

      void setMinMaxDistance( F32 min, F32 max );

      bool isPlaying() const;

      void play( bool looping );

      void stop();

      void pause();

      SFXStatus getStatus() const;

      void setVelocity( const VectorF& velocity );

      void setTransform( const MatrixF& transform );

      void setVolume( F32 volume );

      void setPitch( F32 pitch );

};


/// A vector of SFXFMODVoice pointers.
typedef Vector<SFXFMODVoice*> SFXFMODVoiceVector;

#endif // _SFXFMODBUFFER_H_