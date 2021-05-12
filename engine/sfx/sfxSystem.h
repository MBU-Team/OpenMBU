//-----------------------------------------------------------------------------
// Torque Game Engine Advanced
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _SFXSYSTEM_H_
#define _SFXSYSTEM_H_


#ifndef _SFXSOURCE_H_
   #include "sfx/sfxSource.h"
#endif
#ifndef _SFXPROFILE_H_
   #include "sfx/sfxProfile.h"
#endif
#ifndef _SFXDEVICE_H_
   #include "sfx/sfxDevice.h"
#endif
#ifndef _SFXLISTENER_H_
   #include "sfx/sfxListener.h"
#endif
#ifndef _SIGNAL_H_
   #include "util/tSignal.h"
#endif


/// This class provides access to the sound system.
///
/// There are a few script preferences that are used by
/// the sound providers.
///
///   $pref::SFX::frequency - This is the playback frequency
///   for the primary sound buffer used for mixing.  Although
///   most providers will reformat on the fly, for best quality
///   and performance match your sound files to this setting.
///
///   $pref::SFX::bitrate - This is the playback bitrate for
///   the primary sound buffer used for mixing.  Although most
///   providers will reformat on the fly, for best quality
///   and performance match your sound files to this setting.
///
class SFXSystem
{
   friend class SFXSource;    // for _onRemoveSource.
   friend class SFXProfile;   // for _createBuffer.

   public:
   
      /// The number of volume channels available in the system.
      enum { NumChannels = 32 };

   protected:

      /// The one and only instance of the SFXSystem.
      static SFXSystem* smSingleton;

      /// The protected constructor.
      ///
      /// @see SFXSystem::init()
      ///
      SFXSystem();

      /// The non-virtual destructor.  You shouldn't
      /// ever need to overload this class.
      ~SFXSystem();

      /// The current output sound device initialized
      /// and ready to play back.
      SFXDevice* mDevice;

      /// This contains all the sources currently created
      /// in the system.  This includes all the play once 
      /// sources below as well.
      SFXSourceVector mSources;
      
      /// This is used to keep track of play once sources
      /// that must be released when they stop playing.
      SFXSourceVector mPlayOnceSources;

      /// The position and orientation of the listener.
      SFXListener mListener;

      /// The last time the system got an update.
      U32 mLastTime;

      /// The channel volume which controls the volume of
      /// all sources assigned to that channel.
      F32 mChannelVolume[NumChannels];

      /// The overall volume for all sounds in the system.
      F32 mMasterVolume;

      /// Stats reported back to the console
      /// for tracking performance.
      S32 mStatNumSources;
      S32 mStatNumPlaying;
      S32 mStatNumCulled;
      S32 mStatNumVoices;

      /// Called to reprioritize and reassign buffers as
      /// sources change state, volumes are adjusted, and 
      /// the listener moves around.
      ///
      /// @param msTime The new system time in milliseconds.
      ///
      /// @see SFXSource::_update()
      ///
      void _updateSources( const U32 msTime );

      /// This called to reprioritize and reassign
      /// voices to sources.
      void _assignVoices();

      /// This is called from the source to 
      /// properly clean itself up.
      ///
      /// @see SFXSource::onRemove()
      ///
      void _onRemoveSource( SFXSource* source );

      /// Called from SFXProfile to create a device specific
      /// sound buffer used in conjunction with a voice in playback.
      SFXBuffer*  _createBuffer( SFXProfile* profile );

   public:

      /// Returns the one an only instance of the SFXSystem 
      /// unless it hasn't been initialized or its been disabled
      /// in your build.
      ///
      /// For convienence you can use the SFX-> macro as well.
      ///
      /// @see SFXSystem::init()
      /// @see SFX
      ///
      static SFXSystem* getSingleton() { return smSingleton; }

      /// This is called from initialization to prepare the
      /// sound system singleton.  This also includes registering
      /// common resource types and initializing available sound
      /// providers.
      static void init();

      /// This is called after Sim::shutdown() in shutdownLibraries()
      /// to free the sound system singlton.  After this the SFX
      /// singleton is null and any call to it will crash.
      static void destroy();

      /// This is only public so that it can be called by 
      /// the game update loop.  It updates the current 
      /// device and all sources.
      void _update();

      /// This initializes a new device.
      ///
      /// @param providerName    The name of the provider.
      /// @param deviceName      The name of the provider device.
      /// @param useHardware     Toggles the use of hardware processing when available.
      /// @param maxBuffers      The maximum buffers for this device to use or -1 
      ///                        for the device to pick its own reasonable default.
      /// @param changeDevice    Allows this to change the current device to a new one
      /// @return Returns true if the device was created.
      ///
      bool createDevice(   const char* providerName, 
                           const char* deviceName, 
                           bool useHardware,
                           S32 maxBuffers,
                           bool changeDevice = false);


      /// Returns the current device information or NULL if no
      /// device is present.  The information string is in the
      /// following format:
      /// 
      /// Provider Name\tDevice Name\tUse Hardware\tMax Buffers
      ///
      const char* getDeviceInfoString();

      /// This destroys the current device.  All sources loose their
      /// playback buffers, but otherwise continue to function.
      void deleteDevice();

      /// Returns true if a device is allocated.
      bool hasDevice() const { return mDevice != NULL; }

      /// Used to create new sound sources from a sound profile.  The
      /// returned source is in a stopped state and ready for playback.
      /// Use the SFX_DELETE macro to free the source when your done.
      ///
      /// @param profile   The sound profile for the created source.
      /// @param transform The optional transform if creating a 3D source.
      /// @param velocity  The optional doppler velocity if creating a 3D source.
      ///
      /// @return The sound source or NULL if an error occured.
      ///
      SFXSource*  createSource(  const SFXProfile* profile, 
                                 const MatrixF* transform = NULL, 
                                 const VectorF* velocity = NULL );

      /// Creates a source which when it finishes playing will auto delete
      /// itself.  Be aware that the returned SFXSource pointer should only
      /// be used for error checking or immediate setting changes.  It may
      /// be deleted as soon as the next system tick.
      ///
      /// @param profile   The sound profile for the created source.
      /// @param transform The optional transform if creating a 3D source.
      /// @param velocity  The optional doppler velocity if creating a 3D source.
      ///
      /// @return The sound source or NULL if an error occured.
      ///
      SFXSource* playOnce( const SFXProfile *profile, 
                           const MatrixF *transform = NULL,
                           const VectorF *velocity = NULL );

      /// Returns the one and only listener object.
      SFXListener& getListener() { return mListener; }

      /// Stops all the sounds in a particular channel or across 
      /// all channels if the channel is -1.
      ///
      /// @param channel The channel index less than NumChannels or -1.
      ///
      /// @see NumChannels
      ///
      void stopAll( S32 channel = -1 );

      /// Returns the volume for the specified sound channel.
      ///
      /// @param channel The channel index less than NumChannels.
      ///
      /// @return The channel volume.
      ///
      /// @see NumChannels
      ///
      F32 getChannelVolume( U32 channel ) const;

      /// Sets the volume on the specified sound channel, changing
      /// the volume on all sources in that channel.
      ///
      /// @param channel The channel index less than NumChannels.
      /// @param volume The channel volume to set.
      ///
      /// @see NumChannels
      ///
      void setChannelVolume( U32 channel, F32 volume );

      /// Returns the system master volume level.
      ///
      /// @return The channel volume.
      ///
      F32 getMasterVolume() const { return mMasterVolume; }

      /// Sets the master volume level, changing the
      /// volume of all sources.
      ///
      /// @param volume The channel volume to set.
      ///
      void setMasterVolume( F32 volume );
};


/// Less verbose macro for accessing the SFX singleton.  This
/// should be the prefered method for accessing the system.
///
/// @see SFXSystem
/// @see SFXSystem::getSingleton()
///
#define SFX SFXSystem::getSingleton()


/// A simple macro to automate the deletion of a source.
///
/// @see SFXSource
///
#undef  SFX_DELETE
#define SFX_DELETE( source )  \
   if ( source )              \
   {                          \
      source->deleteObject(); \
      source = NULL;          \
   }                          \


#endif // _SFXSYSTEM_H_
