#if 0
//-----------------------------------------------------------------------------
// Torque Game Engine Advanced
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "platform/platform.h"
#include "util/safeDelete.h"

#include "sfx/sfxSource.h"
#include "sfx/sfxDevice.h"
#include "sfx/sfxBuffer.h"
#include "sfx/sfxSystem.h"


SFXSource::SFXSource( const SFXProfile* profile )
   :  mStatus( SFXStatusNull ),
      mPitch( 1 ),
      mVelocity( 0, 0, 0 ),
      mTransform( true ),
      mAttenuatedVolume( 0 ),
      mDistToListener( 0 ),
      mModulativeVolume( 1 ),
      mBuffer( NULL ),
      mStatusCallback( NULL )
{
   SFXDescription* desc = profile->getDescription();
   AssertFatal( desc, "Got a null description in the source profile!" );

   mIs3D          = desc->mIs3D;
   mIsLooping     = desc->mIsLooping;
   mVolume        = desc->mVolume;
   mIsStreaming   = desc->mIsStreaming;

   mMinDistance   = desc->mReferenceDistance;
   mMaxDistance   = desc->mMaxDistance;

   mConeInsideAngle     = desc->mConeInsideAngle;
   mConeOutsideAngle    = desc->mConeOutsideAngle;
   mConeOutsideVolume   = desc->mConeOutsideVolume;
   mConeVector          = desc->mConeVector;

   mEnvironmentLevel    = desc->mEnvironmentLevel;
   mChannel             = desc->mChannel;

   // TODO: We're not dealing with null resources!
   mResource = profile->getResource();
}

SFXSource::~SFXSource()
{
   // The buffer should have been freed
   // before we're deleted!
   AssertFatal( !mBuffer, "You forgot the free the buffer!" );
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

bool SFXSource::allocBuffer( SFXDevice* device )
{
   // Free any existing buffer.
   freeBuffer( device );

   // Ask the device for a buffer.
   mBuffer = device->createBuffer(  mIs3D,
                                    mResource->getChannels(),
                                    mResource->getFrequency(),
                                    mResource->getSampleBits(),
                                    mResource->getSize() );
   if ( !mBuffer )
      return false;

   // Fill the buffer.
   if ( !mBuffer->copyData( 0, mResource->getData(), mResource->getSize() ) )
      return false;

   // Set the buffer position.
   const U32 ms = ( Platform::getVirtualMilliseconds() - mStartTime ) % mResource->getLength();
   //if ( ms > SFXSystem::smUpdatePeriod )
   mBuffer->setPosition( mResource->getPosition( ms ) );

   setVolume( mVolume );
   setPitch( mPitch );

   if ( mIs3D )
   {
      setMinMaxDistance( mMinDistance, mMaxDistance );
      setTransform( mTransform );
      setVelocity( mVelocity );
   }

   if ( isPlaying() )
      mBuffer->play( mIsLooping );

   return true;
}

void SFXSource::freeBuffer( SFXDevice* device )
{
   if ( !mBuffer )
      return;

   device->deleteBuffer( mBuffer );
   mBuffer = NULL;
}

void SFXSource::updateVolume( const MatrixF& listener )
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

bool SFXSource::setStatus( SFXStatus status )
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
   if ( isPlaying() )
      return;

   if ( !isPaused() )
      mStartTime = Platform::getVirtualMilliseconds();

   setStatus( SFXStatusPlaying );

   // If we have a buffer start playback, else
   // let the system know we need a buffer.
   if ( mBuffer )
      mBuffer->play( mIsLooping );
   else
      SFX->assignBuffer( this );
}

void SFXSource::stop()
{
   if ( !setStatus( SFXStatusStopped ) )
      return;

   if ( mBuffer )
      mBuffer->stop();
}

SFXStatus SFXSource::update( U32 time )
{
   // If we're stopped or paused then don't do any update.
   if ( !isPlaying() )
      return mStatus;

   // TODO: We need to deal with null resources... we should
   // never get a source if the resource doesn't exist!
   if ( !mResource )
   {
      setStatus( SFXStatusStopped );
      return mStatus;
   }

   // What's the total playback length of the 
   // sample associated to this source?
   const U32 ms = mResource->getLength();

   // Check to see if we've finished playback.
   const U32 elapsed = time - mStartTime;
   if ( elapsed > ms )
   {
      if ( isLooping() )
         mStartTime += ms;
      else
         setStatus( SFXStatusStopped );
   }

   return mStatus;
}

void SFXSource::setVelocity( const VectorF& velocity )
{
   mVelocity = velocity;

   if ( mBuffer )
      mBuffer->setVelocity( velocity );      
}

void SFXSource::setTransform( const MatrixF& transform )
{
   mTransform = transform;

   if ( mBuffer )
      mBuffer->setTransform( mTransform );      
}

void SFXSource::setVolume( F32 volume )
{
   mVolume = mClampF( volume, 0, 1 );

   if ( mBuffer )
      mBuffer->setVolume( mVolume * mModulativeVolume );      
}

void SFXSource::setModulativeVolume( F32 volume )
{
   mModulativeVolume = volume;
   setVolume( mVolume );
}

void SFXSource::setPitch( F32 pitch )
{
   AssertFatal( pitch > 0.0f, "Got bad pitch!" );
   mPitch = pitch;

   if ( mBuffer )
      mBuffer->setPitch( mPitch );
}

void SFXSource::setMinMaxDistance( F32 min, F32 max )
{
   min = getMax( 0.0f, min );
   max = getMax( 0.0f, max );

   mMinDistance = min;
   mMaxDistance = max;
   
   if ( mBuffer )
      mBuffer->setMinMaxDistance( mMinDistance, mMaxDistance );
}

ConsoleMethod( SFXSource, play, void, 2, 2,  "SFXSource.play()\n"
                                             "Starts playback of the source." )
{
   object->play();
}

ConsoleMethod( SFXSource, stop, void, 2, 2,  "SFXSource.stop()\n"
                                             "Endsplayback of the source." )
{
   object->stop();
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
#endif