//-----------------------------------------------------------------------------
// Torque Game Engine Advanced
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _SFXWAVRESOURCE_H_
#define _SFXWAVRESOURCE_H_


#ifndef _SFXRESOURCE_H_
#include "sfx/sfxResource.h"
#endif


/// The concrete sound resource for loading 
/// PCM Wave audio data.
class SFXWavResource : public SFXResource
{
   protected:

      /// The constructor is protected. 
      ///
      /// @see SFXResource::load()
      /// @see ResManager::load()
      ///
      SFXWavResource();

      /// The destructor.
      virtual ~SFXWavResource();

      /// This does the real work of loading 
      /// the data from the stream.
      bool load( Stream& stream );

   public:

      /// The creation function registered with the
      /// resource manager.
      ///
      /// @see ResManager::registerExtension()
      ///
      static ResourceInstance* create( Stream &stream );

};


#endif  // _SFXWAVRESOURCE_H_
