//-----------------------------------------------------------------------------
// Torque Game Engine Advanced
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _SFXFMODBUFFER_H_
#define _SFXFMODBUFFER_H_

#ifndef _SFXBUFFER_H_
   #include "sfx/sfxBuffer.h"
#endif
#ifndef _SFXRESOURCE_H_
   #include "sfx/sfxResource.h"
#endif
#ifndef _FMOD_H
   #include "fmod.h"
#endif

class SFXProfile;


class SFXFMODBuffer : public SFXBuffer
{
   friend class SFXFMODDevice;
   friend class SFXFMODVoice;

   protected:

      Resource<SFXResource> mResource;

      FMOD_SOUND *mSound;	   
      FMOD_MODE mMode;

      // 
      SFXFMODBuffer( const Resource<SFXResource> &resource,
                     FMOD_SOUND *sound );

      // copy data into buffer
      bool _copyData( U32 offset, const U8* data, U32 length );

      /// Copy the data to the buffer.
      void _fillBuffer();

      virtual ~SFXFMODBuffer();

   public:

      ///
      static SFXFMODBuffer* create( SFXProfile *profile );

};


/// A vector of SFXDSBuffer pointers.
typedef Vector<SFXFMODBuffer*> SFXFMODBufferVector;

#endif // _SFXFMODBUFFER_H_