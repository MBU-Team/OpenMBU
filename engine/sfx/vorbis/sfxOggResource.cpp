//-----------------------------------------------------------------------------
// Torque Game Engine Advanced
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "platform/platform.h"

#include "sfxOggResource.h"
#include "vorbisStream.h"


ResourceInstance* SFXOggResource::create( Stream &stream )
{
   SFXOggResource* res = new SFXOggResource;
   if ( res->load( stream ) )
      return res;

   delete res;
   return NULL;
}


SFXOggResource::SFXOggResource( )
   : SFXResource()
{
}


SFXOggResource::~SFXOggResource()
{
}


bool SFXOggResource::load( Stream& stream )
{
   OggVorbisFile vf;
   vorbis_info *vi;

   if ( vf.ov_open( &stream, NULL, 0 ) < 0 )
      return false;

   //Read Vorbis File Info
   vi = vf.ov_info(-1);
   mFrequency = vi->rate;

   U32 samples = (U32)vf.ov_pcm_total( -1 );

   if ( vi->channels == 1 ) 
   {
      mFormat = SFX_FORMAT_MONO16;
      mSize = 2 * samples;
   }
   else
   {
      mFormat = SFX_FORMAT_STEREO16;
      mSize = 4 * samples;
   }

   #ifdef TORQUE_BIG_ENDIAN
      bool endian = true;
   #else
      bool endian = false;
   #endif

   mData = new U8[ mSize ];
   S32 current_section = 0;
   read( &vf, mData, mSize, endian, &current_section );

   vf.ov_clear();

   // Calculate the sample length being careful
   // to avoid overflow on the division.
   mLength = ( (U64)samples*(U64)1000 ) / (U64)mFrequency;

   return true;
}

S32 SFXOggResource::read( OggVorbisFile* vf, U8* buffer, U32 length, bool bigendianp, S32* bitstream )
{
   const U32 CHUNKSIZE = 4096;

   U32 bytesRead = 0;
   U32 offset = 0;
   U32 bytesToRead = 0;

   while( offset < length )
   {
      if ( ( length - offset ) < CHUNKSIZE )
         bytesToRead = length - offset;
      else
         bytesToRead = CHUNKSIZE;

      bytesRead = vf->ov_read( (char*)buffer, bytesToRead, bigendianp, bitstream );
      if ( bytesRead <= 0 )
         break;

      offset += bytesRead;
      buffer += bytesRead;
   }

   return offset;
}