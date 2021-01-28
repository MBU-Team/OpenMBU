//--------------------------------------------
// wavStreamSource.h
// header for streaming audio source for WAV
//--------------------------------------------

#ifndef _WAVSTREAMSOURCE_H_
#define _WAVSTREAMSOURCE_H_

#ifndef _AUDIOSTREAMSOURCE_H_
#include "audio/audioStreamSource.h"
#endif

class WavStreamSource: public AudioStreamSource
{
	public:
		WavStreamSource(const char *filename);
		virtual ~WavStreamSource();

		virtual bool initStream();
		virtual bool updateBuffers();
		virtual void freeStream();
      virtual F32 getElapsedTime();
      virtual F32 getTotalTime();

	private: 
		ALuint				    mBufferList[NUMBUFFERS];
		S32						mNumBuffers;
		S32						mBufferSize;
		Stream				   *stream;

		bool					bReady;
		bool					bFinished;

		ALenum  format;
		ALsizei size;
		ALsizei freq;

		ALuint			DataSize;
		ALuint			DataLeft;
		ALuint			dataStart;
		ALuint			buffersinqueue;

		bool			bBuffersAllocated;

		void clear();
		void resetStream();
};  

#endif // _AUDIOSTREAMSOURCE_H_
