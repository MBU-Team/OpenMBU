//-----------------------------------------------------------------------------
// Torque Game Engine Advanced
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _SFXALBUFFER_H_
#define _SFXALBUFFER_H_

#ifndef _SFXBUFFER_H_
   #include "sfx/sfxBuffer.h"
#endif
#ifndef _SFXRESOURCE_H_
   #include "sfx/sfxResource.h"
#endif
#ifndef _OPENALFNTABLE
   #include "sfx/openal/LoadOAL.h"
#endif


class SFXProfile;


class SFXALBuffer : public SFXBuffer
{
   friend class SFXALDevice;
   friend class SFXALVoice;

   protected:

      SFXALBuffer(   const OPENALFNTABLE &oalft, 
                     const Resource<SFXResource> &resource,
                     bool is3d,
                     bool useHardware );

      ///
      Resource<SFXResource> mResource;

      ///
      bool mIs3d;

      ///
      bool mUseHardware;

      const OPENALFNTABLE &mOpenAL;

   public:

      static SFXALBuffer* create(   const OPENALFNTABLE &oalft, 
                                    SFXProfile *profile,
                                    bool useHardware );

      virtual ~SFXALBuffer();

      ///
      bool createVoice( ALuint *bufferName,
                        ALuint *sourceName,
                        ALenum *bufferFormat ) const;

};


/// A vector of SFXALBuffer pointers.
typedef Vector<SFXALBuffer*> SFXALBufferVector;

#endif // _SFXALBUFFER_H_