//-----------------------------------------------------------------------------
// Torque Game Engine Advanced
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "platform/platform.h"
#include "sfx/fmod/sfxFMODBuffer.h"

#include "sfx/sfxProfile.h"
#include "sfx/fmod/sfxFMODDevice.h"


SFXFMODBuffer* SFXFMODBuffer::create( SFXProfile *profile )
{
   if ( !profile )
      return NULL;

   const SFXDescription *desc = profile->getDescription();
   const Resource<SFXResource> &res = profile->getResource();

   FMOD_SOUND *sound = NULL;

   // create new Sound
   FMOD_MODE fMode = FMOD_SOFTWARE | ( desc->mIs3D ? FMOD_3D : FMOD_2D );

   //if ( desc->mIsStreaming )
      //fMode |= FMOD_CREATESTREAM;

   FMOD_CREATESOUNDEXINFO* pCreatesoundexinfo = NULL;
   FMOD_CREATESOUNDEXINFO  createsoundexinfo;
   const char *nameOrData = NULL;

   // if not stub resource, set up sound info structure
   //if (!resource->isStub())
   {
      //AssertFatal(!resource->isStreaming(), "This case not tested and probably doesn't work");

      fMode |= FMOD_OPENUSER; // this tells fmod we are supplying the data directly

      U32 channels = res->getChannels();
      U32 frequency = res->getFrequency();
      U32 bitsPerSample = res->getSampleBits();
      U32 dataSize = res->getSize();

      FMOD_SOUND_FORMAT sfxFmt = FMOD_SOUND_FORMAT_NONE;
      switch(bitsPerSample)
      {
      case 8:
         sfxFmt = FMOD_SOUND_FORMAT_PCM8;
         break;
      case 16:
         sfxFmt = FMOD_SOUND_FORMAT_PCM16;
         break;
      case 24:
         sfxFmt = FMOD_SOUND_FORMAT_PCM24;
         break;
      case 32:
         sfxFmt = FMOD_SOUND_FORMAT_PCM32;
         break;
      default:
         AssertISV(false, "SFXFMODDevice::createBuffer - unsupported bits-per-sample (what format is it in, 15bit PCM?)");
         break;
      }

      dMemset(&createsoundexinfo, 0, sizeof(FMOD_CREATESOUNDEXINFO));
      createsoundexinfo.cbsize            = sizeof(FMOD_CREATESOUNDEXINFO); /* required. */
      createsoundexinfo.decodebuffersize  = frequency; /* Chunk size of stream update in samples.  This will be the amount of data passed to the user callback. */
      createsoundexinfo.length            = dataSize; /* Length of PCM data in bytes of whole sound (for Sound::getLength) */
      createsoundexinfo.numchannels       = channels; /* Number of channels in the sound. */
      createsoundexinfo.defaultfrequency  = frequency; /* Default playback rate of sound. */
      createsoundexinfo.format            = sfxFmt;  /* Data format of sound. */
      createsoundexinfo.pcmreadcallback   = NULL; /* User callback for reading. */
      createsoundexinfo.pcmsetposcallback = NULL; /* User callback for seeking. */
      pCreatesoundexinfo = &createsoundexinfo;
   }
   /*
   else
   {
      // stub resource, set nameOrData to filename
      nameOrData = resource->mSourceResource->getFullPath();
   }
   */

   FModAssert( SFXFMODDevice::smFunc->FMOD_System_CreateSound(
      SFXFMODDevice::smSystem, 
      nameOrData, 
      fMode, 
      pCreatesoundexinfo, 
      &sound ), "SFXFMODDevice::createBuffer - Failed to alloc sound!");

   // Allocate the actual buffer, and note it in our list.
   SFXFMODBuffer *buffer = new SFXFMODBuffer( res, sound );
   buffer->_fillBuffer();

   return buffer;
}

SFXFMODBuffer::SFXFMODBuffer( const Resource<SFXResource> &resource,
                              FMOD_SOUND *sound )
   :  mResource( resource ),
      mSound( sound )
{
   SFXFMODDevice::smFunc->FMOD_Sound_GetMode( mSound, &mMode );
}

SFXFMODBuffer::~SFXFMODBuffer()
{
   FModAssert( SFXFMODDevice::smFunc->FMOD_Sound_Release( mSound ), 
      "SFXFMODBuffer::~SFXFMODBuffer - Failed to release a sound!" );

   mSound = NULL;
   mResource = NULL;
}

void SFXFMODBuffer::_fillBuffer()
{
   _copyData(  0, 
               mResource->getData(), 
               mResource->getSize() );     
}

bool SFXFMODBuffer::_copyData( U32 offset, const U8* data, U32 length )
{
   AssertFatal( data != NULL && length > 0, "Must have data!" );

   // Fill the buffer with the resource data.      
   void* lpvWrite;
   U32  dwLength;
   void* lpvWrite2;
   U32  dwLength2;
   int res = SFXFMODDevice::smFunc->FMOD_Sound_Lock(
      mSound,
      offset,           // Offset at which to start lock.
      length,           // Size of lock; ignored because of flag.
      &lpvWrite,        // Gets address of first part of lock.
      &lpvWrite2,       // Address of wraparound not needed. 
      &dwLength,        // Gets size of first part of lock.
      &dwLength2       // Size of wraparound not needed.
      );

   if ( res != FMOD_OK )
   {
      // You can remove this if it gets spammy. However since we can
      // safely fail in this case it doesn't seem right to assert...
      // at the same time it can be very annoying not to know why 
      // an upload fails!
      Con::errorf("SFXFMODBuffer::_copyData - failed to lock a sound buffer! (%d)", this);
      return false;
   }

   // Copy the first part.
   dMemcpy( lpvWrite, data, dwLength );

   // Do we have a wrap?
   if ( lpvWrite2 )
      dMemcpy( lpvWrite2, data + dwLength, dwLength2 );

   // And finally, unlock.
   FModAssert( SFXFMODDevice::smFunc->FMOD_Sound_Unlock(
      mSound,
      lpvWrite,      // Address of lock start.
      lpvWrite2,     // No wraparound portion.
      dwLength,      // Size of lock.
      dwLength2 ),   // No wraparound size.
      "Failed to unlock sound buffer!" );

   return true;
}
