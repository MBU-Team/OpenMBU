//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------



#include "platform/platformThread.h"
#include "platformX86UNIX/platformX86UNIX.h"
#include "platform/platformSemaphore.h"
#include <pthread.h>

//--------------------------------------------------------------------------
struct x86UNIXThreadData
{
   ThreadRunFunction       mRunFunc;
   void*                     mRunArg;
   Thread *                mThread;
   void *                  mSemaphore;

   x86UNIXThreadData()
   {
      mRunFunc    = 0;
      mRunArg     = 0;
      mThread     = 0;
      mSemaphore  = 0;
   };
};

//--------------------------------------------------------------------------
Thread::Thread(ThreadRunFunction func, void* arg, bool start_thread)
{
   x86UNIXThreadData * threadData = new x86UNIXThreadData();
   threadData->mRunFunc = func;
   threadData->mRunArg = arg;
   threadData->mThread = this;
   threadData->mSemaphore = Semaphore::createSemaphore();

   mData = reinterpret_cast<void*>(threadData);
   if (start_thread)
      start();
}

Thread::~Thread()
{
   join();

   x86UNIXThreadData * threadData = reinterpret_cast<x86UNIXThreadData*>(mData);
   Semaphore::destroySemaphore(threadData->mSemaphore);
   delete threadData;
}

static void *ThreadRunHandler(void * arg)
{
   x86UNIXThreadData * threadData = reinterpret_cast<x86UNIXThreadData*>(arg);

   threadData->mThread->run(threadData->mRunArg);
   Semaphore::releaseSemaphore(threadData->mSemaphore);
   return NULL;
}

void Thread::start()
{
   if(isAlive())
      return;

   x86UNIXThreadData * threadData = reinterpret_cast<x86UNIXThreadData*>(mData);
   Semaphore::acquireSemaphore(threadData->mSemaphore);

   pthread_t threadID;
   pthread_create(&threadID, NULL, ThreadRunHandler, mData);
}

bool Thread::join()
{
   if(!isAlive())
      return(false);

   x86UNIXThreadData * threadData = reinterpret_cast<x86UNIXThreadData*>(mData);
   return(Semaphore::acquireSemaphore(threadData->mSemaphore));
}

void Thread::run(void* arg)
{
   x86UNIXThreadData * threadData = reinterpret_cast<x86UNIXThreadData*>(mData);
   if(threadData->mRunFunc)
      threadData->mRunFunc(arg);
}

bool Thread::isAlive()
{
   x86UNIXThreadData * threadData = reinterpret_cast<x86UNIXThreadData*>(mData);

   bool signal = Semaphore::acquireSemaphore(threadData->mSemaphore, false);
   if(signal)
      Semaphore::releaseSemaphore(threadData->mSemaphore);
   return(!signal);
}

U32 Thread::getCurrentThreadId()
{
    return (U32)pthread_self();
}
