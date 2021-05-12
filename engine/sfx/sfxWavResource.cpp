//-----------------------------------------------------------------------------
// Torque Game Engine Advanced
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "platform/platform.h"
#include "sfx/sfxWavResource.h"


ResourceInstance* SFXWavResource::create( Stream &stream )
{
   SFXWavResource* res = new SFXWavResource;
   if ( res->load( stream ) )
      return res;

   delete res;
   return NULL;
}


SFXWavResource::SFXWavResource( )
   : SFXResource()
{
}


SFXWavResource::~SFXWavResource()
{
}


/// WAV File-header
struct WAVFileHdr
{
   U8  id[4];
   U32 size;
   U8  type[4];
};

//// WAV Fmt-header
struct WAVFmtHdr
{
   U16 format;
   U16 channels;
   U32 samplesPerSec;
   U32 bytesPerSec;
   U16 blockAlign;
   U16 bitsPerSample;
};

/// WAV FmtEx-header
struct WAVFmtExHdr
{
   U16 size;
   U16 samplesPerBlock;
};

/// WAV Smpl-header
struct WAVSmplHdr
{
   U32 manufacturer;
   U32 product;
   U32 samplePeriod;
   U32 note;
   U32 fineTune;
   U32 SMPTEFormat;
   U32 SMPTEOffest;
   U32 loops;
   U32 samplerData;

   struct
   {
      U32 identifier;
      U32 type;
      U32 start;
      U32 end;
      U32 fraction;
      U32 count;
   } loop[1];
};

/// WAV Chunk-header
struct WAVChunkHdr
{
   U8  id[4];
   U32 size;
};


bool SFXWavResource::load( Stream& stream )
{
   WAVFileHdr fileHdr;
   stream.read( 4, &fileHdr.id[0] );
   stream.read( &fileHdr.size );
   stream.read( 4, &fileHdr.type[0] );

   fileHdr.size=((fileHdr.size+1)&~1)-4;

   WAVChunkHdr chunkHdr;
   stream.read( 4, &chunkHdr.id[0] );
   stream.read( &chunkHdr.size );

   // Unread chunk data rounded up to nearest WORD.
   S32 chunkRemaining = chunkHdr.size + ( chunkHdr.size & 1 );

   WAVFmtHdr   fmtHdr;
   WAVFmtExHdr fmtExHdr;
   WAVSmplHdr  smplHdr;

   dMemset(&fmtHdr, 0, sizeof(fmtHdr));

   while ((fileHdr.size!=0) && (stream.getStatus() != Stream::EOS))
   {
      // WAV format header chunk.
      if ( !dStrncmp( (const char*)chunkHdr.id, "fmt ", 4 ) )
      {
         stream.read(&fmtHdr.format);
         stream.read(&fmtHdr.channels);
         stream.read(&fmtHdr.samplesPerSec);
         stream.read(&fmtHdr.bytesPerSec);
         stream.read(&fmtHdr.blockAlign);
         stream.read(&fmtHdr.bitsPerSample);

         if ( fmtHdr.format == 0x0001 )
         {
            mFormat = ( fmtHdr.channels==1 ?
                        ( fmtHdr.bitsPerSample == 8 ? SFX_FORMAT_MONO8 : SFX_FORMAT_MONO16 ) :
                        ( fmtHdr.bitsPerSample == 8 ? SFX_FORMAT_STEREO8 : SFX_FORMAT_STEREO16 ) );

            mFrequency = fmtHdr.samplesPerSec;
            chunkRemaining -= sizeof( WAVFmtHdr );
         }
         else
         {
            stream.read(sizeof(WAVFmtExHdr), &fmtExHdr);
            chunkRemaining -= sizeof(WAVFmtExHdr);
         }
      }

      // WAV data chunk
      else if (!dStrncmp((const char*)chunkHdr.id,"data",4))
      {
         if (fmtHdr.format==0x0001)
         {
            mSize = chunkHdr.size;
            mData = new U8[ mSize ];
            if ( mData )
            {
               stream.read( mSize, mData );
               
               #ifdef TORQUE_BIG_ENDIAN

                  // need to endian-flip the 16-bit data.
                  if (fmtHdr.bitsPerSample==16) // !!!TBD we don't handle stereo, so may be RL flipped.
                  {
                     U16 *ds = (U16*)mData;
                     U16 *de = (U16*)(mData+mSize);
                     while (ds<de)
                     {
                        *ds = convertLEndianToHost(*ds);
                        ds++;
                     }
                  }

               #endif

               chunkRemaining -= chunkHdr.size;
            }
         }
         else if (fmtHdr.format==0x0011)
         {
            //IMA ADPCM
         }
         else if (fmtHdr.format==0x0055)
         {
            //MP3 WAVE
         }
      }

      // WAV sample header
      else if (!dStrncmp((const char*)chunkHdr.id,"smpl",4))
      {
         // this struct read is NOT endian safe but it is ok because
         // we are only testing the loops field against ZERO
         stream.read(sizeof(WAVSmplHdr), &smplHdr);

         // This has never been hooked up and its usefulness is
         // dubious.  Do we really want the audio file overriding
         // the SFXDescription setting?    
         //mLooping = ( smplHdr.loops ? true : false );

         chunkRemaining -= sizeof(WAVSmplHdr);
      }

      // either we have unread chunk data or we found an unknown chunk type
      // loop and read up to 1K bytes at a time until we have
      // read to the end of this chunk
      char buffer[1024];
      AssertFatal(chunkRemaining >= 0, "AudioBuffer::readWAV: remaining chunk data should never be less than zero.");
      while (chunkRemaining > 0)
      {
         S32 readSize = getMin(1024, chunkRemaining);
         stream.read(readSize, buffer);
         chunkRemaining -= readSize;
      }

      fileHdr.size-=(((chunkHdr.size+1)&~1)+8);

      // read next chunk header...
      stream.read(4, &chunkHdr.id[0]);
      stream.read(&chunkHdr.size);
      // unread chunk data rounded up to nearest WORD
      chunkRemaining = chunkHdr.size + (chunkHdr.size&1);
   }

   // Calculate the sample length being careful
   // to avoid overflow on the division.
   const U32 samples = (U64)mSize / (U64)getSampleBytes();
   mLength = ( (U64)samples*(U64)1000 ) / (U64)mFrequency;

   return true;
}
