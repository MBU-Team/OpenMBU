//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "platformWin32/platformWin32.h"
#include "platform/platformSemaphore.h"
#include "platform/profiler.h"

void* Semaphore::createSemaphore(U32 initialCount)
{
    HANDLE* semaphore = new HANDLE;
    *semaphore = CreateSemaphore(0, initialCount, S32_MAX, 0);
    return(semaphore);
}

void Semaphore::destroySemaphore(void* semaphore)
{
    AssertFatal(semaphore, "Semaphore::destroySemaphore: invalid semaphore");
    CloseHandle(*(HANDLE*)semaphore);
    delete semaphore;
}

bool Semaphore::acquireSemaphore(void* semaphore, bool block)
{
    AssertFatal(semaphore, "Semaphore::acquireSemaphore: invalid semaphore");
    PROFILE_START(Semaphore_acquireSemaphore);
    if (block)
    {
        WaitForSingleObject(*(HANDLE*)semaphore, INFINITE);
        PROFILE_END();
        return(true);
    }
    else
    {
        DWORD result = WaitForSingleObject(*(HANDLE*)semaphore, 0);
        PROFILE_END();
        return(result == WAIT_OBJECT_0);
    }
}

void Semaphore::releaseSemaphore(void* semaphore)
{
    AssertFatal(semaphore, "Semaphore::releaseSemaphore: invalid semaphore");
    PROFILE_START(Semaphore_releaseSemaphore);
    ReleaseSemaphore(*(HANDLE*)semaphore, 1, 0);
    PROFILE_END();

}
