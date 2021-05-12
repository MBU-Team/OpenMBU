//-----------------------------------------------------------------------------
// Torque Game Engine Advanced
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _SFXDSBUFFER_H_
#define _SFXDSBUFFER_H_

#ifndef _SFXBUFFER_H_
   #include "sfx/sfxBuffer.h"
#endif
#ifndef _SFXRESOURCE_H_
   #include "sfx/sfxResource.h"
#endif

#include <dsound.h>

class SFXProfile;


class SFXDSBuffer : public SFXBuffer
{
   friend class SFXDSDevice;

   protected:

      ///
      Resource<SFXResource> mResource;

      ///
      bool mIs3d;

      ///
      bool mUseHardware;

      IDirectSound8 *mDSound;

      /// The bufffer used when duplication is allowed.
      IDirectSoundBuffer8 *mBuffer;
    
      /// We set this to true when the original buffer has
      /// been handed out and duplicates need to be made.
      bool mDuplicate;

      ///
      SFXDSBuffer(   IDirectSound8 *dsound,
                     const Resource<SFXResource> &resource,
                     bool is3d,
                     bool useHardware );

      virtual ~SFXDSBuffer();

      ///
      bool _createBuffer( IDirectSoundBuffer8 **buffer8 ) const;

      ///
      bool _duplicateBuffer( IDirectSoundBuffer8 **buffer8 );

      ///
      bool _copyData(   IDirectSoundBuffer8 *buffer8,
                        U32 offset, 
                        const U8 *data, 
                        U32 length ) const;

   public:

      ///
      static SFXDSBuffer* create(   IDirectSound8* dsound, 
                                    SFXProfile *profile,
                                    bool useHardware );

      //
      bool createVoice( IDirectSoundBuffer8 **buffer );

      //
      void releaseVoice( IDirectSoundBuffer8 **buffer );

};

/// A vector of SFXDSBuffer pointers.
typedef Vector<SFXDSBuffer*> SFXDSBufferVector;

#endif // _SFXDSBUFFER_H_