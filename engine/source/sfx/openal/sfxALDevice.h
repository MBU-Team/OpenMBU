//-----------------------------------------------------------------------------
// Torque Game Engine Advanced
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _SFXALDEVICE_H_
#define _SFXALDEVICE_H_

class SFXProvider;

#ifndef _SFXDEVICE_H_
#  include "sfx/sfxDevice.h"
#endif

#ifndef _SFXPROVIDER_H_
#  include "sfx/sfxProvider.h"
#endif

#ifndef _SFXALBUFFER_H_
#  include "sfx/openal/sfxALBuffer.h"
#endif

#ifndef _SFXALVOICE_H_
#  include "sfx/openal/sfxALVoice.h"
#endif

#ifndef _OPENALFNTABLE
#  include "sfx/openal/LoadOAL.h"
#endif

class SFXALDevice : public SFXDevice
{
   typedef SFXDevice Parent;

   public:

      SFXALDevice(   SFXProvider* provider, 
                     const OPENALFNTABLE &openal, 
                     const char* name, 
                     bool useHardware, 
                     S32 maxBuffers );

      virtual ~SFXALDevice();

   protected:

      const StringTableEntry mName;

      SFXALVoiceVector mVoices;

      SFXALBufferVector mBuffers;

      OPENALFNTABLE mOpenAL;

      ALCcontext *mContext;

      ALCdevice *mDevice;

   public:

      const char* getName() const { return mName; }

      //
      SFXBuffer* createBuffer( SFXProfile *profile );

      ///
      SFXVoice* createVoice( SFXBuffer *buffer );

      ///
      void deleteVoice( SFXVoice* buffer );

      U32 getVoiceCount() const { return mVoices.size(); }

      void update( const SFXListener& listener );
};

#endif // _SFXALDEVICE_H_