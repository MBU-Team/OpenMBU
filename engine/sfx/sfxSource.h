//-----------------------------------------------------------------------------
// Torque Game Engine Advanced
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _SFXSOURCE_H_
#define _SFXSOURCE_H_

#ifndef _SFXDEVICE_H_
   #include "sfx/sfxDevice.h"
#endif
#ifndef _SFXVOICE_H_
   #include "sfx/sfxVoice.h"
#endif
#ifndef _SIMBASE_H_
   #include "console/simBase.h"
#endif
#ifndef _MPOINT_H_
   #include "math/mPoint.h"
#endif
#ifndef _MMATH_H_
   #include "math/mMatrix.h"
#endif
#ifndef _TVECTOR_H_
   #include "core/tVector.h"
#endif

class SFXProfile;
class SFXDescription;
class SFXBuffer;
class SFXDevice;


/// A source is a scriptable controller for all 
/// aspects of sound playback.
class SFXSource : public SimObject
{
   friend class SFXSystem;
   friend class SFXListener;

   typedef SimObject Parent;
   
   protected:

      /// Used by SFXSystem to create sources.
      static SFXSource* _create( const SFXProfile *profile );

      /// Internal constructor used for sources.
      SFXSource( const SFXProfile *profile );

      /// Only SFXSystem should be deleting us!
      virtual ~SFXSource();

      /// This is called only from the device to allow
      /// the source to update it's attenuated volume.
      void _updateVolume( const MatrixF& listener );

      /// Called by the device so that the source can 
      /// update itself and any attached buffer.
      SFXStatus _updateStatus();

      /// The last updated playback status of the source.
      mutable SFXStatus mStatus;

      /// The time that playback started.
      U32 mPlayTime;

      /// The playback time when we paused or zero.
      U32 mPauseTime;

      /// The profile used to create this source
      SimObjectPtr<SFXProfile> mProfile;

      /// The device specific voice which is used during
      /// playback.  By making it a SafePtr it will NULL
      /// automatically when the device is deleted.
      SafePtr<SFXVoice> mVoice;

      /// This is the volume of a source with respect
      /// to the last listener position.  It is used
      /// for culling sounds.
      F32 mAttenuatedVolume;

      /// The distance of this source to the last 
      /// listener position.
      F32 mDistToListener;

      /// The desired sound volume.
      F32 mVolume;

      /// This 
      F32 mModulativeVolume;

      /// The sound pitch scalar.
      F32 mPitch;

      /// The transform if this is a 3d source.
      MatrixF mTransform;

      /// The last set velocity.
      VectorF mVelocity;

      bool mIs3D;

      bool mIsLooping;

      bool mIsStreaming;

      F32 mMinDistance;

      F32 mMaxDistance;

      U32 mConeInsideAngle;

      U32 mConeOutsideAngle;

      F32 mConeOutsideVolume;

      Point3F mConeVector;

      U32 mChannel;

      StringTableEntry mStatusCallback;

      /// We overload this to disable creation of 
      /// a source via script 'new'.
      bool processArguments( S32 argc, const char **argv );

      /// Used internally for setting the sound status.
      bool _setStatus( SFXStatus status );

      /// This is called from SFXSystem for setting
      /// the volume scalar generated from the master
      /// and channel volumes.
      void _setModulativeVolume( F32 volume );

      /// Create a new voice for this source.
      bool _allocVoice( SFXDevice* device );

      /// Release the voice if the source has one.
      void _freeVoice( SFXDevice* device );

      /// Overloaed from SimObject to do cleanup.
      void onRemove();

   public:

      DECLARE_CONOBJECT(SFXSource);

      /// The default constructor is *only* here to satisfy the
      /// construction needs of IMPLEMENT_CONOBJECT.  It does not
      /// create a valid source!
      explicit SFXSource();

      /// Also needed by IMPLEMENT_CONOBJECT, but we don't use it.
      static void initPersistFields();

      /// This is normally called from the system to 
      /// detect if this source has been assigned a
      /// voice for playback.
      bool hasVoice() const { return mVoice != NULL; }

      /// Starts the sound from the current playback position.
      void play();

      /// Stops playback and resets the playback position.
      void stop();

      /// Pauses the sound playback.
      void pause();

      /// Sets the position and orientation for a 3d buffer.
      void setTransform( const MatrixF& transform );

      /// Sets the position for a 3d buffer.
      void setPosition(const Point3F& position);

      /// Sets the velocity for a 3d buffer.
      void setVelocity( const VectorF& velocity );

      /// Sets the minimum and maximum distances for 3d falloff.
      void setMinMaxDistance( F32 min, F32 max );

      /// Sets the source volume which will still be
      /// scaled by the master and channel volumes.
      void setVolume( F32 volume );

      /// Sets the source pitch scale.
      void setPitch( F32 pitch );

      /// Returns the last set velocity.
      const VectorF& getVelocity() const { return mVelocity; }

      /// Returns the last set transform.
      const MatrixF& getTransform() const { return mTransform; }

      /// Returns the source volume.
      F32 getVolume() const { return mVolume; }

      /// Returns the volume with respect to the master 
      /// and channel volumes and the listener.
      F32 getAttenuatedVolume() const { return mAttenuatedVolume; }

      /// Returns the source pitch scale.
      F32 getPitch() const { return mPitch; }

      /// Returns the last known status without checking
      /// the voice or doing the virtual calculation.
      SFXStatus getLastStatus() const { return mStatus; }

      /// Returns the sound status.
      SFXStatus getStatus() const { return const_cast<SFXSource*>( this )->_updateStatus(); }

      /// Returns true if the source is playing.
      bool isPlaying() const { return getStatus() == SFXStatusPlaying; }

      /// Returns true if the source is stopped.
      bool isStopped() const { return getStatus() == SFXStatusStopped; }

      /// Returns true if the source has been paused.
      bool isPaused() const { return getStatus() == SFXStatusPaused; }

      /// Returns true if this is a 3D source.
      bool is3d() const { return mIs3D; }

      /// Returns true if this is a looping source.
      bool isLooping() const { return mIsLooping; }

      /// Returns the volume channel this source is assigned to.
      U32 getChannel() const { return mChannel; }

      /// Returns the last distance to the listener.
      F32 getDistToListener() const { return mDistToListener; }
};


/// A vector of SFXSource pointers.
typedef Vector<SFXSource*> SFXSourceVector;


#endif // _SFXSOURCE_H_