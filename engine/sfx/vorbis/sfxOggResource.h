//-----------------------------------------------------------------------------
// Torque Game Engine Advanced
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _SFXOGGRESOURCE_H_
#define _SFXOGGRESOURCE_H_


#ifndef _SFXRESOURCE_H_
#include "sfx/sfxResource.h"
#endif

class OggVorbisFile;


/// The concrete sound resource for loading 
/// Ogg Vorbis audio data.
class SFXOggResource : public SFXResource
{
   protected:

      /// The constructor is protected. 
      ///
      /// @see SFXResource::load()
      /// @see ResManager::load()
      ///
      SFXOggResource();

      /// The destructor.
      virtual ~SFXOggResource();

      /// This does the real work of loading the 
      /// data from the stream.
      bool load( Stream& stream );

      /// Helper function reads one buffer length of data.
      static S32 read( OggVorbisFile* vf, U8* buffer, U32 length, bool bigendianp, S32* bitstream );

   public:

      /// The creation function registered with the
      /// resource manager.
      ///
      /// @see ResManager::registerExtension
      ///
      static ResourceInstance* create( Stream &stream );

};


#endif  // _SFXOGGRESOURCE_H_
