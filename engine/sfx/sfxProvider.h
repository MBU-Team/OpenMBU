//-----------------------------------------------------------------------------
// Torque Game Engine Advanced
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _SFXPROVIDER_H_
#define _SFXPROVIDER_H_

#ifndef _TVECTOR_H_
   #include "core/tVector.h"
#endif


class SFXDevice;


/// This macro is used to init the provider.
#define SFX_INIT_PROVIDER( providerClass )      \
   static providerClass g##providerClass##Instance;


struct SFXDeviceInfo
{
   char  driver[256];
   char  name[256];
   bool  hasHardware;
   S32   maxBuffers;
};

typedef Vector<SFXDeviceInfo*> SFXDeviceInfoVector;

class SFXProvider
{
   friend class SFXSystem;

   private:

      /// The head of the linked list of avalible providers.
      static SFXProvider* smProviders;

      /// The next provider in the linked list of available providers. 
      SFXProvider* mNextProvider;

      /// The provider name which is passed by the concrete provider
      /// class to the SFXProvider constructor.
      const char* mName;

   protected:

      /// The array of avaIlable devices from this provider.  The
      /// concrete provider class will fill this on construction.
      SFXDeviceInfoVector  mDeviceInfo;

      /// This adds the provider to the provider list.  It should be called
      /// from the concrete provider constructor if the provider was properly
      /// initialized and is available for device enumeration and creation.
      static void regProvider( SFXProvider* provider );

      SFXProvider( const char* name );
      ~SFXProvider();

      /// This is called from SFXSystem to create a new device.  Must be implemented
      /// by all contrete provider classes.
      ///
      /// @param deviceName      The case sensitive name of the device or NULL to create the 
      //                         default device.
      /// @param useHardware     Toggles the use of hardware processing when available.
      /// @param maxBuffers      The maximum buffers for this device to use or -1 
      ///                        for the device to pick a reasonable default for that device.
      ///
      /// @return Returns the created device or NULL for failure.
      ///
      virtual SFXDevice* createDevice( const char* deviceName, bool useHardware, S32 maxBuffers ) = 0;

   public:

      /// Returns a specific provider by searching the provider list
      /// for the first provider with the case sensitive name.
      static SFXProvider* findProvider( const char* providerName );

      /// Returns the first provider in the provider list.  Use 
      /// getNextProvider() to iterate over list.
      static SFXProvider* getFirstProvider() { return smProviders; }

      /// Returns the next provider in the provider list or NULL
      /// when the end of the list is reached.
      SFXProvider* getNextProvider() const { return mNextProvider; }

      /// The case sensitive name of this provider.
      const char* getName() const { return mName; }

      /// Returns a read only vector with device information for
      /// all creatable devices available from this provider.
      const SFXDeviceInfoVector& getDeviceInfo() const { return mDeviceInfo; }
};


#endif // _SFXPROVIDER_H_