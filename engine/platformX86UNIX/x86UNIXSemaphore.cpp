//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------



#include "platformX86UNIX/platformX86UNIX.h"
#include "platform/platformSemaphore.h"
#include <fcntl.h>

#if defined(__linux__)
#include <semaphore.h>
#elif defined(__OpenBSD__)
#include <sys/sem.h>
#endif

void * Semaphore::createSemaphore(U32 initialCount)
{
#if defined(__linux__)
   sem_t *semaphore;
   /* hell, I want an elite semaphore, OK?! - rjp */
   semaphore = sem_open("/tmp/eliteQueue.31337", O_CREAT, 0664, initialCount);
   return(semaphore);
#elif defined(__OpenBSD__)
   key_t mykey;
   int semaphore;
   semaphore = semget(mykey, initialCount, IPC_CREAT | 0660);
   return(&semaphore);
#endif
}

void Semaphore::destroySemaphore(void * semaphore)
{
   AssertFatal(semaphore, "Semaphore::destroySemaphore: invalid semaphore");
#if defined(__linux__)
   sem_close((sem_t *)semaphore);
#elif defined(__OpenBSD__)
   semctl((*(int *)semaphore), 0, IPC_RMID, 0);
#endif
}

bool Semaphore::acquireSemaphore(void * semaphore, bool block)
{
   AssertFatal(semaphore, "Semaphore::acquireSemaphore: invalid semaphore");
   if(block)
   {
      /* must wait */
#if defined(__linux__)
      sem_wait((sem_t *)semaphore);
      return(true);
#elif defined(__OpenBSD__)
      struct sembuf sem_lock = { 0, -1, IPC_NOWAIT };
      if (semop(*(int *)semaphore, &sem_lock, 1) == -1)
         return(false);
      else
         return(true);
#endif
   }
   else
   {
      /* try to wait */
#if defined(__linux__)
      U32 result = sem_trywait((sem_t *)semaphore);
      return(result == 0);
#elif defined(__OpenBSD__)
      struct sembuf sem_lock = { 0, -1 };
      U32 result = semop(*(int *)semaphore, &sem_lock, 1);
      return(result == 0);
#endif
   }
}

void Semaphore::releaseSemaphore(void * semaphore)
{
   AssertFatal(semaphore, "Semaphore::releaseSemaphore: invalid semaphore");
#if defined(__linux__)
   sem_unlink("/tmp/eliteQueue.31337");
#elif defined(__OpenBSD__)
   struct sembuf sem_unlock = { 0, 1, IPC_NOWAIT};
   semop(*(int *)semaphore, &sem_unlock, 1);
#endif
}
