//-----------------------------------------------------------------------------
// Torque Game Engine Advanced
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "platform/platform.h"
#include "sfx/sfxListener.h"

#include "sfx/sfxSource.h"
#include "platform/profiler.h"


SFXListener::SFXListener()
   :  mTransform( true ),
      mVelocity( 0, 0, 0 )
{
}

SFXListener::~SFXListener()
{
}

// TODO: Maybe use the channel as a key for priority?
// Let the user define a priority value for each channel
// in script.  We assign it in the system init and use
// it when doleing out hardware handles.

S32 QSORT_CALLBACK SFXListener::sourceCompare( const void* item1, const void* item2 )
{
   const SFXSource* source1 = *((SFXSource**)item1);
   const SFXSource* source2 = *((SFXSource**)item2);

	// Sounds that are playing are always sorted 
	// closer than non-playing sounds.
	if ( !source1->isPlaying() )
		return 1;
	if ( !source2->isPlaying() )
		return -1;

   // The sources with louder attenuated 
   // volume are higher priority.
	const F32 volume1 = source1->getAttenuatedVolume();
	const F32 volume2 = source2->getAttenuatedVolume();
   if ( volume1 < volume2 )
      return 1;
   if ( volume1 > volume2 )
      return -1;

   // If we got this far then the source that was 
   // played last has the higher priority.
   if ( source1->mPlayTime > source2->mPlayTime )
      return -1;
   if ( source1->mPlayTime < source2->mPlayTime )
      return 1;

   // These are sorted the same!
   return 0;
}

void SFXListener::sortSources( SFXSourceVector& sources )
{
   PROFILE_SCOPE( SFXListener_SortSources );

   // First have the sources update the attenuated
   // volume for each source.
   SFXSourceVector::iterator iter = sources.begin();  
   for ( ; iter != sources.end(); iter++ )
      (*iter)->_updateVolume( mTransform );

   // Now sort the source vector by the attenuated 
   // volume and channel priorities.  This leaves us
   // with the loudest and highest priority sounds 
   // at the front of the vector.
   dQsort( (void *)sources.address(), sources.size(), sizeof(SFXSource*), sourceCompare );
}
