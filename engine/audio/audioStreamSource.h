//--------------------------------------------
// audioStreamSource.h
// header for streaming audio source
//
// Kurtis Seebaldt
//--------------------------------------------

#ifndef _AUDIOSTREAMSOURCE_H_
#define _AUDIOSTREAMSOURCE_H_

#ifndef _PLATFORM_H_
#include "platform/platform.h"
#endif
#ifndef _PLATFORMAUDIO_H_
#include "platform/platformAudio.h"
#endif
#ifndef _PLATFORMAL_H_
#include "platform/platformAL.h"
#endif
#ifndef _AUDIOBUFFER_H_
#include "audio/audioBuffer.h"
#endif
#ifndef _RESMANAGER_H_
#include "core/resManager.h"
#endif

#define NUMBUFFERS 16

class AudioStreamSource
{
	public:
        //need this because subclasses are deleted through base pointer
        virtual ~AudioStreamSource() {}
		virtual bool initStream() = 0;
		virtual bool updateBuffers() = 0;
		virtual void freeStream() = 0;
      virtual F32 getElapsedTime() = 0;
      virtual F32 getTotalTime() = 0;
		//void clear();

		AUDIOHANDLE             mHandle;
		ALuint				    mSource;

		Audio::Description      mDescription;
		AudioSampleEnvironment *mEnvironment;

		Point3F                 mPosition;
		Point3F                 mDirection;
		F32                     mPitch;
		F32                     mScore;
		U32                     mCullTime;

		bool					bFinishedPlaying;
		bool					bIsValid;

#ifdef TORQUE_OS_LINUX
                void checkPosition();
#endif             

	protected:
		const char* mFilename;
};

#endif // _AUDIOSTREAMSOURCE_H_
