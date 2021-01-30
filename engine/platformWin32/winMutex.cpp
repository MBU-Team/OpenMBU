//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "platformWin32/platformWin32.h"
#include "platform/platformMutex.h"
#include "platform/profiler.h"

void* Mutex::createMutex()
{
    CRITICAL_SECTION* mutex = new CRITICAL_SECTION;
    InitializeCriticalSection(mutex);
    return((void*)mutex);
}

void Mutex::destroyMutex(void* mutex)
{
    AssertFatal(mutex, "Mutex::destroyMutex: invalid mutex");
    DeleteCriticalSection((CRITICAL_SECTION*)mutex);
    delete mutex;
}

bool Mutex::lockMutex(void* mutex, bool block)
{
    AssertFatal(mutex, "Mutex::lockMutex: invalid mutex");
    PROFILE_START(Mutex_lockMutex);

    if (block)
    {
        EnterCriticalSection((CRITICAL_SECTION*)mutex);
        PROFILE_END();
        return true;
    }
    else
    {
        bool ret = TryEnterCriticalSection((CRITICAL_SECTION*)mutex);
        PROFILE_END();
        return ret;
    }
}

void Mutex::unlockMutex(void* mutex)
{
    AssertFatal(mutex, "Mutex::unlockMutex: invalid mutex");

    PROFILE_START(Mutex_unlockMutex);
    LeaveCriticalSection((CRITICAL_SECTION*)mutex);
    PROFILE_END();
}
