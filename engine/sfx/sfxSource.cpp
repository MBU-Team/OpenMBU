//-----------------------------------------------------------------------------
// Torque Game Engine Advanced
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "platform/platform.h"
#include "sfx/sfxSource.h"

#include "sfx/sfxDevice.h"
#include "sfx/sfxVoice.h"
#include "sfx/sfxSystem.h"
#include "util/safeDelete.h"


SFXSource::SFXSource()
   : mVoice( NULL )
{
   // NOTE: This should never be used directly 
   // and is only here to satisfy satisfy the
   // construction needs of IMPLEMENT_CONOBJECT.
}

SFXSource::SFXSource( const SFXProfile *profile )
   :  mStatus( SFXStatusNull ),
      mPitch( 1 ),
      mVelocity( 0, 0, 0 ),
      mTransform( true ),
      mAttenuatedVolume( 0 ),
      mDistToListener( 0 ),
      mModulativeVolume( 1 ),
      mVoice( NULL ),
      mPlayTime( 0 ),
      mPauseTime( 0 ),
      mIsStreaming( false ),

      // This sucks... but it works.
      mProfile( const_cast<SFXProfile*>( profile ) ),

      mStatusCallback( NULL )
{
   const SFXDescription* desc = mProfile->getDescription();

   mIs3D = desc->mIs3D;
   mIsLooping = desc->mIsLooping;
   mVolume = desc->mVolume;
   mMinDistance = desc->mReferenceDistance;
   mMaxDistance = desc->mMaxDistance;
   mConeInsideAngle = desc->mConeInsideAngle;
   mConeOutsideAngle = desc->mConeOutsideAngle;
   mConeOutsideVolume = desc->mConeOutsideVolume;
   mConeVector = desc->mConeVector;
   mChannel = desc->mChannel;
   mIsStreaming = desc->mIsStreaming;
}

SFXSource* SFXSource::_create( const SFXProfile *profile )
{
   AssertFatal( profile, "SFXSource::_create() - Got a null profile!" );

   SFXDescription* desc = profile->getDescription();
   if ( !desc )
   {
      Con::errorf( "SFXSource::_create() - Profile has null description!" );
      return NULL;
   }

   //const Resource<SFXResource> &resource = profile->getResource();
   SFXSource *source = NULL;

   //if ( desc->mIsStreaming )
   //{
   //   // TODO: This is where we would build a streaming
   //   // source object.

   //   Con::errorf( "SFXSource::_create() - Stream sources are unsupported!" );
   //   return NULL;
   //}
   //else
   //{
      /*
      // Grab the buffer.
      SFXBuffer *buffer = profile->getBuffer();
      if ( !buffer )
      {
         Con::errorf( "SFXSource::_create() - The sound buffer could not be created!" );
         return NULL;
      }
      */

      /*
      TODO: Move to SFXBuffer creation!

      // Check for the very common error of trying to
      // play a stereo sound via a 3D source.
      if (  desc->mIs3D &&
            resource->getChannels() != 1 )
      {
         Con::errorf( "SFXSource::_create() - A mono sound file is required for 3D sources!" );
         return NULL;
      }
      */

      source = new SFXSource( profile );
   //}

   // The source is a console object... register it.
   source->registerObject();

   // All sources are added to 
   // the global source set.
   Sim::getSFXSourceSet()->addObject( source );

   return source;
}

SFXSource::~SFXSource()
{
   // The voice should have been freed
   // before we're deleted!
   AssertFatal( !mVoice, "SFXSource::~SFXSource() - The voice was not freed!" );
}

IMPLEMENT_CONOBJECT(SFXSource);

void SFXSource::initPersistFields()
{
   // We don't want any fields as they complicate updating
   // the state of the source.  If we could easily set dirty
   // bits on changes to fields then it would be worth it,
   // so for now just use the console methods.
}

bool SFXSource::processArguments( S32 argc, const char **argv )
{
   // We don't want to allow a source to be constructed
   // via the script... force use of the SFXSystem.
   Con::errorf( ConsoleLogEntry::Script, "Use sfxCreateSource, sfxPlay, or sfxPlayOnce!" );
   return false;
}

bool SFXSource::_allocVoice( SFXDevice* device )
{
   // We shouldn't have any existing voice!
   AssertFatal( !mVoice, "SFXSource::_allocVoice() - Already had a voice!" );

   // If the profile has been deleted then we
   // cannot get the buffer.
   if ( !mProfile )
      return false;

   // Grab the buffer from the profile.
   SFXBuffer *buffer = mProfile->getBuffer();
   if ( !buffer )
      return false;

   // Ask the device for a voice based on this buffer.
   mVoice = device->createVoice( buffer );
   if ( !mVoice )
      return false;

   setVolume( mVolume );
   setPitch( mPitch );
   setMinMaxDistance( mMinDistance, mMaxDistance );
   setTransform( mTransform );
   setVelocity( mVelocity );

   if ( getLastStatus() == SFXStatusPaused )
      mPlayTime += Platform::getVirtualMilliseconds() - mPauseTime;

   // TODO: This is kinda messy... maybe we should
   // add time based position methods and let the
   // voice and buffer coordinate this internally.
   const Resource<SFXResource> &resoutce = mProfile->getResource();
   U32 playbackMs = Platform::getVirtualMilliseconds() - mPlayTime;
   playbackMs %= resoutce->getLength();
   U32 pos = resoutce->getPosition( playbackMs );
   mVoice->setPosition( pos );

   if ( getLastStatus() == SFXStatusPlaying )
      mVoice->play( mIsLooping );

   return true;
}

void SFXSource::onRemove()
{
   // Remove it from the set.
   Sim::getSFXSourceSet()->removeObject( this );

   // Let the system know.
   SFX->_onRemoveSource( this );

   Parent::onRemove();
}

void SFXSource::_freeVoice( SFXDevice* device )
{
   if ( !mVoice )
      return;

   // Let the device delete it.
   device->deleteVoice( mVoice );

   // The safe ptr should have been nulled
   // when the device deleted the voice.
   AssertFatal( !mVoice, "SFXSource::_freeVoice() - The smartptr was not nulled!" );
}

void SFXSource::_updateVolume( const MatrixF& listener )
{
   const F32 volume = mVolume * mModulativeVolume;

   if ( !mIs3D ) 
   {
      mDistToListener = 0.0f;
      mAttenuatedVolume = volume; 
      return;
	}

   Point3F pos, lpos;
   mTransform.getColumn( 3, &pos );
   listener.getColumn( 3, &lpos );

   mDistToListener = ( pos - lpos ).len();

   // If we're outside the maximum distance then force
   // the attenuated volume to zero.
   if ( mDistToListener > mMaxDistance )
   {
      mAttenuatedVolume = 0;
      return;
   }

   // This should never be zero or negative!
	AssertFatal( mMinDistance > 0.0f, "Can't have a negative or zero min dist!" );

   // The volume of a sound to the listener is:
	//
	//	1 / ( d / m ) * v
	//
	// where d is the distance to the listener, m is
	// the minimum sound distance, and v is the 
	// un-attenuated volume.

   // Calculate ( d / m ) and set the un-attenuated volumes.
	const F32 distanceOverMinimum = mDistToListener / mMinDistance;
	
   // If ( d / m ) is greater than one then the volume is attenuated.
	if ( distanceOverMinimum > 1.0f )
		mAttenuatedVolume = ( 1.0f / distanceOverMinimum ) * volume;
   else
      mAttenuatedVolume = volume;
}

bool SFXSource::_setStatus( SFXStatus status )
{
   if ( mStatus == status )
      return false;

   mStatus = status;

   // Convert the status to a string.
   const char* statusString;
   switch ( mStatus )
   {
      case SFXStatusPlaying:
         statusString = "playing";
         break;

      case SFXStatusStopped:
         statusString = "stopped";
         break;

      case SFXStatusPaused:
         statusString = "paused";
         break;

      case SFXStatusNull:
      default:
         statusString = "null";
   };

   // Do the callback if we have it.
   if ( mStatusCallback && mStatusCallback[0] )
      Con::executef( 2, mStatusCallback, statusString );
   else if ( getNamespace() )
      Con::executef( this, 2, "onStatus", statusString );

   return true;
}

void SFXSource::play()
{
   // Update our status once.
   _updateStatus();

   if ( mStatus == SFXStatusPlaying )
      return;

   if ( mStatus != SFXStatusPaused )
      mPlayTime = Platform::getVirtualMilliseconds();

   _setStatus( SFXStatusPlaying );

   if ( mVoice )
      mVoice->play( mIsLooping );
   else
   {
      // To ensure the fastest possible reaction 
      // to this playback let the system reassign
      // voices immediately.
      SFX->_assignVoices();
   }
}

void SFXSource::stop()
{
   _setStatus( SFXStatusStopped );

   if ( mVoice )
      mVoice->stop();
}

void SFXSource::pause()
{
   if ( !_setStatus( SFXStatusPaused ) )
      return;

   if ( mVoice )
      mVoice->pause();
}

SFXStatus SFXSource::_updateStatus()
{
   // If we have a voice... it has full
   // control over the status.
   if ( mVoice )
      return ( mStatus = mVoice->getStatus() );   

   // If we're not in a playing state or we're a looping
   // sound then we don't need to calculate the status.
   if ( mIsLooping || mStatus != SFXStatusPlaying )
      return mStatus;

   // If we're playing and don't have a voice we
   // need to decide if the sound is done playing
   // to ensure proper virtualization of the sound.

   if ( !mProfile || !mProfile->getResource() )
   {
      _setStatus( SFXStatusStopped );
      return mStatus;
   }

   const SFXResource* res = mProfile->getResource();

   // What's the total playback length of the 
   // sample associated to this source?
   const U32 msLength = res->getLength();

   // TODO: We need to know if a source is running on
   // simulation time or real time... its different!
   const U32 time = Platform::getVirtualMilliseconds();

   // Check to see if we've finished playback.
   const U32 elapsed = time - mPlayTime;
   if ( elapsed > msLength )
      _setStatus( SFXStatusStopped );

   return mStatus;
}

void SFXSource::setVelocity( const VectorF& velocity )
{
   mVelocity = velocity;

   if ( mVoice )
      mVoice->setVelocity( velocity );      
}

void SFXSource::setPosition(const Point3F& position)
{
    mTransform.setPosition(position);
    setTransform(mTransform);
}

void SFXSource::setTransform( const MatrixF& transform )
{
   mTransform = transform;

   if ( mVoice )
      mVoice->setTransform( mTransform );      
}

void SFXSource::setVolume( F32 volume )
{
   mVolume = mClampF( volume, 0, 1 );

   if ( mVoice )
      mVoice->setVolume( mVolume * mModulativeVolume );      
}

void SFXSource::_setModulativeVolume( F32 volume )
{
   mModulativeVolume = volume;
   setVolume( mVolume );
}

void SFXSource::setPitch( F32 pitch )
{
   AssertFatal( pitch > 0.0f, "Got bad pitch!" );
   mPitch = pitch;

   if ( mVoice )
      mVoice->setPitch( mPitch );
}

void SFXSource::setMinMaxDistance( F32 min, F32 max )
{
   min = getMax( 0.0f, min );
   max = getMax( 0.0f, max );

   mMinDistance = min;
   mMaxDistance = max;

   if ( mVoice )
      mVoice->setMinMaxDistance( mMinDistance, mMaxDistance );
}

ConsoleMethod( SFXSource, play, void, 2, 2,  "SFXSource.play()\n"
                                             "Starts playback of the source." )
{
   object->play();
}

ConsoleMethod( SFXSource, stop, void, 2, 2,  "SFXSource.stop()\n"
                                             "Ends playback of the source." )
{
   object->stop();
}

ConsoleMethod( SFXSource, pause, void, 2, 2,  "SFXSource.pause()\n"
                                                "Pauses playback of the source." )
{
   object->pause();
}

ConsoleMethod( SFXSource, isPlaying, bool, 2, 2,  "SFXSource.isPlaying()\n"
                                                  "Returns true if the source is playing or false if not." )
{
   return object->isPlaying();
}

ConsoleMethod( SFXSource, isPaused, bool, 2, 2,  "SFXSource.isPaused()\n"
                                                  "Returns true if the source is paused or false if not." )
{
   return object->isPaused();
}

ConsoleMethod( SFXSource, setVolume, void, 3, 3,  "SFXSource.setVolume( F32 volume )\n"
                                                  "Sets the playback volume of the source." )
{
   object->setVolume( dAtof( argv[2] ) );
}

ConsoleMethod( SFXSource, setPitch, void, 3, 3,  "SFXSource.setPitch( F32 pitch )\n"
                                                  "Scales the pitch of the source." )
{
   object->setPitch( dAtof( argv[2] ) );
}

ConsoleMethod( SFXSource, setCallback, void, 3, 3,  "SFXSource.setCallback( string callbackFunction )\n"
                                                  "Sets the  the pitch of the source." )
{
   //object->setCallback( argv[2] );
}

ConsoleMethod( SFXSource, getChannel, S32, 2, 2,   "SFXSource.getChannel()\n"
                                                   "Returns the volume channel." )
{
   return object->getChannel();
}

ConsoleMethod( SFXSource, setTransform, void, 3, 3,  "SFXSource.setTransform( xform )\n" )
{
   Point3F pos;
   dSscanf( argv[2], "%g %g %g", &pos.x, &pos.y, &pos.z );
   MatrixF mat = object->getTransform();
   mat.setPosition( pos );
   object->setTransform( mat );
}
