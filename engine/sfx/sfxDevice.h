//-----------------------------------------------------------------------------
// Torque Game Engine Advanced
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _SFXDEVICE_H_
#define _SFXDEVICE_H_

#ifndef _UTIL_SIGNAL_H_
   #include "util/tSignal.h"
#endif


class SFXProvider;
class SFXListener;
class SFXBuffer;
class SFXVoice;
class SFXProfile;
class SFXDevice;


///
enum SFXDeviceEventType
{
   ///
   SFXDeviceEvent_Create,

   ///
   SFXDeviceEvent_Destroy,
};

///
typedef Signal<SFXDevice*,SFXDeviceEventType> SFXDeviceEventSignal;


class SFXDevice
{
   protected:

      SFXDevice( SFXProvider* provider, bool useHardware, S32 maxBuffers );

      /// The provider which created this device.
      SFXProvider* mProvider;

      /// Should the device try to use hardware processing.
      bool mUseHardware;

      /// The maximum playback buffers this device will use.
      S32 mMaxBuffers;

      /// 
      static SFXDeviceEventSignal smEventSignal;

   public:

      ///
      virtual ~SFXDevice() { }

      ///
      static SFXDeviceEventSignal& getEventSignal() { return smEventSignal; }
  
      /// Returns the provider which created this device.
      SFXProvider* getProvider() const { return mProvider; }

      /// Is the device set to use hardware processing.
      bool getUseHardware() const { return mUseHardware; }

      /// The maximum number of playback buffers this device will use.
      S32 getMaxBuffers() const { return mMaxBuffers; }

      /// Returns the name of this device.
      virtual const char* getName() const = 0;

      /// Tries to create a new sound buffer.  If creation fails
      /// freeing another buffer will usually allow a new one to 
      /// be created.
      ///
      /// @param is3d            True if a 3D sound buffer is desired.
      /// @param channels        The number of sound channels... typically 1 or 2.
      /// @param frequency       The number of samples per second ( a sample includes
      ///                        all channels ).
      /// @param bitsPerSample   The number of bits per sample ( a sample includes all
      ///                        channels ).
      /// @param dataSize        The total size of the buffer in bytes.
      ///
      /// @return Returns a new buffer or NULL if one cannot be created.
      ///
      virtual SFXBuffer* createBuffer( SFXProfile *profile ) = 0;

      ///
      virtual SFXVoice* createVoice( SFXBuffer* buffer ) = 0;

      ///
      virtual void deleteVoice( SFXVoice* buffer ) = 0;

      ///
      virtual U32 getVoiceCount() const = 0;

      /// Called from SFXSystem to do any updates the device may need to make.
      virtual void update( const SFXListener& listener ) = 0;
};


#endif // _SFXDEVICE_H_