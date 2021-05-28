//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _PLATFORMMUTEX_H_
#define _PLATFORMMUTEX_H_

#include "platform/platformAssert.h"

// TODO: change this to a proper class.

struct Mutex
{
    static void* createMutex(void);
    static void destroyMutex(void*);
    static bool lockMutex(void* mutex, bool block = true);
    static void unlockMutex(void*);
};

/// Helper for simplifying mutex locking code.
///
/// This class will automatically unlock a mutex that you've
/// locked through it, saving you from managing a lot of complex
/// exit cases. For instance:
///
/// @code
/// MutexHandle handle;
/// handle.lock(myMutex);
///
/// if(error1)
///   return; // Auto-unlocked by handle if we leave here - normally would
///           // leave the mutex locked, causing much pain later.
///
/// handle.unlock();
/// @endcode
class MutexHandle
{
private:
    void* mMutexPtr;

public:
    MutexHandle()
        : mMutexPtr(NULL)
    {
    }

    ~MutexHandle()
    {
        if (mMutexPtr)
            unlock();
    }

    bool lock(void* mutex, bool blocking = true)
    {
        AssertFatal(!mMutexPtr, "MutexHandle::lock - shouldn't be locking things twice!");

        bool ret = Mutex::lockMutex(mutex, blocking);

        if (ret)
        {
            // We succeeded, do book-keeping.
            mMutexPtr = mutex;
        }

        return ret;
    }

    void unlock()
    {
        AssertFatal(mMutexPtr, "MutexHandle::unlock - didn't have a mutex to unlock!");

        Mutex::unlockMutex(mMutexPtr);
        mMutexPtr = NULL;
    }

};

#endif
