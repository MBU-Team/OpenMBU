//--------------------------------------
// audioStreamSource.cc
// implementation of streaming audio source
//
// Kurtis Seebaldt
//--------------------------------------

#include "audio/audioStreamSourceFactory.h"

#include "audio/wavStreamSource.h"
#ifndef TORQUE_NO_OGGVORBIS
#include "audio/vorbisStreamSource.h"
#include "audio/oggMixedStreamSource.h"
#endif

AudioStreamSource* AudioStreamSourceFactory::getNewInstance(const char *filename)
{
#ifndef TORQUE_NO_OGGVORBIS

	if(!dStricmp(filename, "oggMixedStream"))
		return new OggMixedStreamSource(filename);

	S32 len = dStrlen(filename);

	// Check filename extension and guess filetype from that

	if(len > 3 && !dStricmp(filename + len - 4, ".wav"))
		return new WavStreamSource(filename);
	if(len > 3 && !dStricmp(filename + len - 4, ".ogg"))
		return new VorbisStreamSource(filename);
#endif
	return NULL;
}
