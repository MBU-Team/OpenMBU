//--------------------------------------------
// audioStreamSource.h
// header for streaming audio source
//
// Kurtis Seebaldt
//--------------------------------------------

#ifndef _AUDIOSTREAMSOURCEFACTORY_H_
#define _AUDIOSTREAMSOURCEFACTORY_H_

#ifndef _PLATFORM_H_
#include "platform/platform.h"
#endif
#ifndef _PLATFORMAUDIO_H_
#include "platform/platformAudio.h"
#endif
#ifndef _PLATFORMAL_H_
#include "platform/platformAL.h"
#endif
#ifndef _AUDIOSTREAMSOURCE_H_
#include "audio/audioStreamSource.h"
#endif

class AudioStreamSourceFactory
{
	public:
		static AudioStreamSource* getNewInstance(const char* filename);
};  

#endif // _AUDIOSTREAMSOURCEFACTORY_H_
