//-----------------------------------------------------------------------------
// Torque Game Engine Advanced
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _SFXNULLDEVICE_H_
#define _SFXNULLDEVICE_H_

class SFXProvider;

#ifndef _SFXDEVICE_H_
   #include "sfx/sfxDevice.h"
#endif
#ifndef _SFXPROVIDER_H_
   #include "sfx/sfxProvider.h"
#endif
#ifndef _SFXNULLBUFFER_H_
   #include "sfx/null/sfxNullBuffer.h"
#endif
#ifndef _SFXNULLVOICE_H_
   #include "sfx/null/sfxNullVoice.h"
#endif


class SFXNullDevice : public SFXDevice
{
   typedef SFXDevice Parent;

   public:

      SFXNullDevice( SFXProvider* provider, 
                     const char* name, 
                     bool useHardware, 
                     S32 maxBuffers );

      virtual ~SFXNullDevice();

   protected:

      const StringTableEntry mName;

      SFXNullVoiceVector mVoices;

      SFXNullBufferVector mBuffers;

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

#endif // _SFXNULLDEVICE_H_