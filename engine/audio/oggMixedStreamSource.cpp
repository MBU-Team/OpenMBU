//--------------------------------------
//
// This class is basically a buffer that is filled from
// the ogg stream the theoraplayer has open
//
//--------------------------------------

#include "audio/oggMixedStreamSource.h"

OggMixedStreamSource::OggMixedStreamSource(const char *filename)
{
   bIsValid = false;
   bBuffersAllocated = false;
   for(int i = 0; i < BUFFERCNT; i++)
   {
      mBufferList[i] = 0;
      m_fBufferInUse[i] = false;
   }


   mHandle = NULL_AUDIOHANDLE;
   mSource = NULL;

   mFilename = filename;
   mPosition = Point3F(0.f,0.f,0.f);

   dMemset(&mDescription, 0, sizeof(Audio::Description));
   mEnvironment = 0;
   mPosition.set(0.f,0.f,0.f);
   mDirection.set(0.f,1.f,0.f);
   mPitch = 1.f;
   mScore = 0.f;
   mCullTime = 0;

   bFinishedPlaying = false;
   bIsValid = false;
   bBuffersAllocated = false;
}

OggMixedStreamSource::~OggMixedStreamSource()
{
   if(bIsValid)
      freeStream();
}

bool OggMixedStreamSource::initStream()
{
   alSourceStop(mSource);
   alSourcei(mSource, AL_BUFFER, 0);

   // Clear Error Code
   alGetError();

   alGenBuffers(BUFFERCNT, mBufferList);
   if (alGetError() != AL_NO_ERROR)
      return false;

   bBuffersAllocated = true;

   alSourcei(mSource, AL_LOOPING, AL_FALSE);

   bIsValid = true;

   return true;
}

bool OggMixedStreamSource::updateBuffers()
{
   // buffers are updated from theora player
   return true;
}

void OggMixedStreamSource::freeStream()
{
   // free the al buffers
   if(bBuffersAllocated)
   {
      alSourceStop(mSource);
      alSourcei(mSource, AL_BUFFER, 0);
      alDeleteBuffers(BUFFERCNT, mBufferList);

      alGetError();

      for(int i = 0; i < BUFFERCNT; i++)
      {
         mBufferList[i] = 0;
         m_fBufferInUse[i] = false;
      }

      bBuffersAllocated = false;
   }
}

ALuint OggMixedStreamSource::GetAvailableBuffer()
{
   if(!bBuffersAllocated)
      return 0;

   // test for unused buffers
   for(int i = 0; i < BUFFERCNT; i++)
   {
      if(!m_fBufferInUse[i])
      {
         m_fBufferInUse[i] = true;
         return mBufferList[i];
      }
   }

   alGetError();

   // test for processed buffers
   ALint         processed;
   alGetSourcei(mSource, AL_BUFFERS_PROCESSED, &processed);

   if(!processed)
      return 0; // no available buffers

   ALuint BufferID;
   alSourceUnqueueBuffers(mSource, 1, &BufferID);

   if (alGetError() != AL_NO_ERROR)
      return 0; // something went wrong..

   return BufferID;
}

bool OggMixedStreamSource::QueueBuffer(ALuint BufferID)
{
   alSourceQueueBuffers(mSource, 1, &BufferID);
   if (alGetError() != AL_NO_ERROR)
      return false;
   return true;
}
