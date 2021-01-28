//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _AUDIOBUFFER_H_
#define _AUDIOBUFFER_H_

#ifndef _PLATFORM_H_
#include "platform/platform.h"
#endif
#ifndef _PLATFORMAL_H_
#include "platform/platformAL.h"
#endif
#ifndef _RESMANAGER_H_
#include "core/resManager.h"
#endif

// MLH - don't need oggbvorbis in tools
#ifndef TORQUE_NO_OGGVORBIS
#include "audio/vorbisStream.h"
#endif

//--------------------------------------------------------------------------

class AudioBuffer: public ResourceInstance
{
   friend class AudioThread;

private:
   StringTableEntry  mFilename;
   bool              mLoading;
   ALuint            malBuffer;

   bool readRIFFchunk(Stream &s, const char *seekLabel, U32 *size);
   bool readWAV(ResourceObject *obj);

#ifndef TORQUE_NO_OGGVORBIS
   bool readOgg(ResourceObject *obj);
   long oggRead(OggVorbisFile* vf, char *buffer,int length,
		    int bigendianp,int *bitstream);
#endif

public:
   AudioBuffer(StringTableEntry filename);
   ~AudioBuffer();
   ALuint getALBuffer();
   bool isLoading() {return(mLoading);}

   static Resource<AudioBuffer> find(const char *filename);
   static ResourceInstance* construct(Stream& stream);

};


#endif  // _H_AUDIOBUFFER_
