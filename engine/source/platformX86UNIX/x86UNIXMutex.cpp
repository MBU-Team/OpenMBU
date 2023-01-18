//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------



#include "console/console.h"
#include "platformX86UNIX/platformX86UNIX.h"
#include "platform/platformMutex.h"
#include "platformX86UNIX/x86UNIXMutex.h"

#include <pthread.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

void * Mutex::createMutex()
{
   pthread_mutex_t *mutex;

   mutex = new pthread_mutex_t;

   pthread_mutexattr_t attr;

   pthread_mutexattr_init(&attr);
   pthread_mutexattr_settype(&attr,PTHREAD_MUTEX_RECURSIVE);
   pthread_mutex_init(mutex, &attr);

   return((void*)mutex);
}

void Mutex::destroyMutex(void * mutex)
{
   pthread_mutex_t *pt_mutex = reinterpret_cast<pthread_mutex_t*>(mutex);
   AssertFatal(pt_mutex, "Mutex::destroyMutex: invalid mutex");
   pthread_mutex_destroy(pt_mutex);
   delete pt_mutex;
}

bool Mutex::lockMutex(void * mutex, bool block)
{
   pthread_mutex_t *pt_mutex = reinterpret_cast<pthread_mutex_t*>(mutex);
   AssertFatal(pt_mutex, "Mutex::lockMutex: invalid mutex");
   if (block)
   {
       return pthread_mutex_lock(pt_mutex) == 0;
   }
   else
   {
       return pthread_mutex_trylock(pt_mutex) == 0;
   }
}

void Mutex::unlockMutex(void * mutex)
{
   pthread_mutex_t *pt_mutex = reinterpret_cast<pthread_mutex_t*>(mutex);
   AssertFatal(pt_mutex, "Mutex::unlockMutex: invalid mutex");
   pthread_mutex_unlock(pt_mutex);
}

ProcessMutex::ProcessMutex()
{
   mFD = -1;
}

ProcessMutex::~ProcessMutex()
{
   release();
}

bool ProcessMutex::acquire(const char *mutexName)
{
   if (mFD != -1)
      return false;

   dSprintf(
      mLockFileName, LockFileNameSize,
      "/tmp/%s.lock", mutexName);

   mFD = open(mLockFileName,
      O_CREAT | O_RDWR,
      S_IRUSR | S_IRGRP | S_IROTH |
      S_IWUSR | S_IWGRP | S_IWOTH);

   if (mFD == -1)
   {
      Con::printf("Couldn't open file: %s", mLockFileName);
      return false;
   }

   struct flock lock;
   //lock.l_type = F_RDLCK;
   lock.l_type = F_WRLCK;
   lock.l_whence = SEEK_SET;
   lock.l_start = 0;
   lock.l_len = 0;
   lock.l_pid = getpid();

   if (fcntl(mFD, F_SETLK, &lock) == -1)
   {
      Con::printf("Couldn't lock file: %s, %s",
         mLockFileName, strerror(errno));
      if (fcntl(mFD, F_GETLK, &lock) != -1)
         Con::printf("Lock owned by pid: %d", lock.l_pid);
      Con::printf("Remove the file if lock is stale");
      close(mFD);
      mFD = -1;
      return false;
   }
   return true;
}

void ProcessMutex::release()
{
   if (mFD != -1)
   {
      close(mFD);
      mFD = -1;
      unlink(mLockFileName);
   }
}

ConsoleFunction(debug_testx86unixmutex, void, 1, 1, "debug_testx86unixmutex()")
{
   void* mutex = Mutex::createMutex();
   AssertFatal(mutex, "couldn't create mutex");
   Con::printf("created mutex");
   Mutex::lockMutex(mutex);
   Con::printf("locked mutex");
   Mutex::unlockMutex(mutex);
   Con::printf("unlocked mutex");
   Mutex::destroyMutex(mutex);
   Con::printf("destroyed mutex");
}

