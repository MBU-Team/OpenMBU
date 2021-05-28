//-----------------------------------------------------------------------------
// Torque Game Engine Advanced
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _SFXPROFILE_H_
#define _SFXPROFILE_H_

#ifndef _CONSOLETYPES_H_
   #include "console/consoleTypes.h"
#endif
#ifndef _SIMDATABLOCK_H_
   #include "console/simBase.h"
#endif
#ifndef _SFXDESCRIPTION_H_
   #include "sfx/sfxDescription.h"
#endif
#ifndef _SFXDEVICE_H_
   #include "sfx/sfxDevice.h"
#endif
#ifndef _SFXRESOURCE_H_
   #include "sfx/sfxResource.h"
#endif
#ifndef _SFXBUFFER_H_
   #include "sfx/sfxBuffer.h"
#endif


class SFXProfile : public SimDataBlock
{
   friend class SFXEmitter; // For access to mFilename

   protected:

      typedef SimDataBlock Parent;

      /// Used on the client side during onAdd.
      S32 mDescriptionId;

      ///
      Resource<SFXResource> mResource;

      ///
      SFXDescription *mDescription;

      /// The sound filename.
      StringTableEntry mFilename;

      /// If true the sound data will be loaded from
      /// disk and possibly cached with the active 
      /// device before the initial call for playback.
      bool mPreload;

      /// The device specific data buffer.
      SafePtr<SFXBuffer> mBuffer;

      /// Called when the buffer needs to be preloaded.
      bool _preloadBuffer();

      /// Callback for device events.
      void _onDeviceEvent( SFXDevice *device, SFXDeviceEventType evt );

   public:

      /// This is only here to make DECLARE_CONOBJECT 
      /// happy.  No one should be creating a profile
      /// via this constructor.
      explicit SFXProfile();

      ///
      SFXProfile( SFXDescription* desc, 
                  StringTableEntry filename = NULL, 
                  bool preload = false );

      /// The destructor.
      virtual ~SFXProfile();

      DECLARE_CONOBJECT( SFXProfile );

      static void initPersistFields();

      bool onAdd();

      void onRemove();

      void packData( BitStream* stream );

      void unpackData( BitStream* stream );

      /// 
      const char* getFilename() const { return mFilename; }

      /// @note This has nothing to do with mPreload.
      /// @see SimDataBlock::preload
      bool preload( bool server, char errorBuffer[256] );

      /// Returns the description object for this sound profile.
      SFXDescription* getDescription() const { return mDescription; }

      /// 
      const Resource<SFXResource>& getResource() const { return mResource; }

      /// Returns the device buffer for this for this 
      /// sound.  It will load it from disk if it has
      /// not been created already.
      SFXBuffer* getBuffer();
};

DECLARE_CONSOLETYPE( SFXProfile );


#endif  // _SFXPROFILE_H_