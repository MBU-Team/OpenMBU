//-----------------------------------------------------------------------------
// Torque Game Engine Advanced
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "platform/platform.h"


#include "sfx/sfxResource.h"
#include "sfx/sfxWavResource.h"
#ifndef TORQUE_NO_OGGVORBIS
   #include "sfx/vorbis/sfxOggResource.h"
#endif


// TODO: This class should support threaded loading of data in 
// the background.  Here is how it could work... 
//
// The header data is always read in the foreground thread when
// the resource is created.
//
// We add a "mThreadedLoad" option to the SFXProfile which 
// triggers the threaded load.
//
// When the data is requested the mThreadedLoad option is
// passed.  
//
// ?????


Resource<SFXResource> SFXResource::load( const char* filename )
{
   // Sound resources have a load on demand feature that
   // isn't directly supported by the resource manager.
   // To put loading under our control we add the resource
   // to the system ourselves giving it just the filename
   // used for loading later.

   // First see if the resource has been cached or can 
   // load directly without us futzing with the extension.
   Resource<SFXResource> buffer = ResourceManager->load( filename );
   if ( (bool)buffer )
      return buffer;

   // TODO: Fix the resource manager to deal with loading
   // resources by extensionless name and type using a 
   // prioritized list of file extensions to test with. This
   // would help here and in the texture loading system.

   // We need to try to load this with a specific file
   // extension.  The resource only does this for WAV
   // and optionally OGG types.
   //
   // First see if the file is a wav and exists.
   char temp[256];
   dStrncpy( temp, filename, sizeof( temp ) );
   dStrncat( temp, ".wav", sizeof( temp ) );
   Stream* stream = ResourceManager->openStream( temp );
   if ( stream )
   {
      ResourceManager->add( filename, SFXWavResource::create( *stream ) );
      ResourceManager->closeStream( stream );
      return ResourceManager->load( filename );
   }
   else
   {
      #ifndef TORQUE_NO_OGGVORBIS

         // Now try it as an ogg.
         dStrncpy( temp, filename, sizeof( temp ) );
         dStrncat( temp, ".ogg", sizeof( temp ) );
         stream = ResourceManager->openStream( temp );
         if ( stream )
         {
            ResourceManager->add( filename, SFXOggResource::create( *stream ) );
            ResourceManager->closeStream( stream );
            return ResourceManager->load( filename );
         }

      #endif
   }

   return NULL;
}


bool SFXResource::exists( const char* filename )
{
   // First check to see if the resource manager can find it.
   if ( ResourceManager->getPathOf( filename ) )
      return true;

   // Else try our extensions.
   char temp[256];
   dStrncpy( temp, filename, sizeof( temp ) );
   dStrncat( temp, ".wav", sizeof( temp ) );
   if ( ResourceManager->getPathOf( temp ) )
      return true;

   #ifndef TORQUE_NO_OGGVORBIS

      dStrncpy( temp, filename, sizeof( temp ) );
      dStrncat( temp, ".ogg", sizeof( temp ) );
      if ( ResourceManager->getPathOf( temp ) )
         return true;

   #endif

   return false;
}


SFXResource::SFXResource()
   :  mFormat( SFX_FORMAT_MONO16 ),
      mData( NULL ),
      mSize( 0 ),
      mFrequency( 22050 ),
      mLength( 0 )
{
}


SFXResource::~SFXResource()
{
   delete [] mData;
}

U32 SFXResource::getChannels() const
{
   switch( mFormat )
   {
      case SFX_FORMAT_MONO8:
      case SFX_FORMAT_MONO16:
         return 1;

      case SFX_FORMAT_STEREO8:
      case SFX_FORMAT_STEREO16:
         return 2;
   };

   return 0;
}

U32 SFXResource::getSampleBytes() const
{
   switch( mFormat )
   {
      case SFX_FORMAT_MONO8:
         return 1;

      case SFX_FORMAT_STEREO8:
      case SFX_FORMAT_MONO16:
         return 2;

      case SFX_FORMAT_STEREO16:
         return 4;
   };

   return 0;
}

U32 SFXResource::getPosition( U32 ms ) const
{
   if ( ms > mLength )
      ms = mLength;

   U32 bytes = ( ( ms * mFrequency ) * getSampleBytes() ) / 1000;
   return bytes;
}
