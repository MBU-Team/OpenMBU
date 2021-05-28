//-----------------------------------------------------------------------------
// Torque Game Engine Advanced
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _SFXRESOURCE_H_
#define _SFXRESOURCE_H_


#ifndef _RESMANAGER_H_
#include "core/resManager.h"
#endif


/// The various types of sound data that may be
/// returned from SFXResource::getData().
enum SFXFormat
{
   SFX_FORMAT_MONO8,       //< A mono 8bit format.
   SFX_FORMAT_MONO16,      //< A mono 16bit format.
   SFX_FORMAT_STEREO8,     //< A stereo 8bit format.
   SFX_FORMAT_STEREO16,    //< A stereo 16bit format.

   /// The data is in a provider/device specific 
   // proprietary format.  See the overloaded resource
   // class for the format description.
   SFX_FORMAT_PROPRIETARY, 
};


/// This is the base class for all sound file resources including
/// streamed sound files.
///
/// The first step occurs at ResourceManager::load() time at which
/// only the header information, the format, size frequency, and 
/// looping flag, are loaded from the sound file.  This provides 
/// just the nessasary information to simulate sound playback for
/// sounds playing just out of the users hearing range.
/// 
/// The second step loads the actual sound data or begins filling
/// the stream buffer.  This is triggered by a call to ????.
/// SFXProfile, for example, does this when mPreload is enabled.
///
class SFXResource : public ResourceInstance
{
   protected:

      /// The constructor is protected. 
      /// @see SFXResource::load()
      SFXResource();

      /// The destructor.
      virtual ~SFXResource();

      /// The format of the sample data.
      SFXFormat mFormat;

      /// The loaded sample data.
      U8* mData;

      /// The length of the mData array in bytes.
      U32 mSize;

      /// The number of samples per second.
      U32 mFrequency;

      /// The length of the sample in milliseconds.
      U32 mLength;

   public:

      /// This is a helper function used by SFXProfile for load
      /// a sound resource.  It takes care of trying different 
      /// types for extension-less filenames.
      ///
      /// @param filename The sound file path with or without extension.
      ///
      static Resource<SFXResource> load( const char* filename );

      /// A helper function which returns true if the 
      /// sound resource exists.
      ///
      /// @param filename The sound file path with or without extension.
      ///
      static bool exists( const char* filename );

      /// Returns the sample data array.
      const U8* getData() const { return mData; }

      /// The length of the data buffer in bytes.
      U32 getSize() const { return mSize; }

      /// Returns the total playback time milliseconds.
      U32 getLength() const { return mLength; }

      /// Returns the byte offset into the sample 
      /// buffer from the time in milliseconds.
      ///
      /// @param ms The time in milliseconds.
      ///
      U32 getPosition( U32 ms ) const;

      /// Returns the number of bytes per sample based on
      /// the format ( a sample includes all channels ).
      U32 getSampleBytes() const;

      /// Returns the number of bits per sample ( a 
      /// sample includes all channels ).
      U32 getSampleBits() const { return getSampleBytes() * 8; }

      /// Returns the number of seperate audio signals in
      /// the sound.  Typically 1 for mono or 2 for stereo.
      U32 getChannels() const;

      /// Returns the number of samples per second.
      U32 getFrequency() const { return mFrequency; }
};


#endif  // _SFXRESOURCE_H_
