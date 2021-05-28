//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _THREADSAFEQUEUE_H_
#define _THREADSAFEQUEUE_H_

#include "platform/platform.h"
#include "platform/platformMutex.h"
#include "core/tVector.h"

/// A simple thread-safe queue.
///
/// @ingroup AtlasCore
template<class T>
class ThreadSafeQueue
{
    void* mMutex;
    Vector<T> mQueue;
    volatile bool mLocked;

public:

    ThreadSafeQueue()
    {
        mLocked = false;
        mMutex = Mutex::createMutex();
    }

    ~ThreadSafeQueue()
    {
        Mutex::destroyMutex(mMutex);
        mMutex = NULL;
    }

    /// Add new item to end of queue.
    void queue(const T& data)
    {
        Mutex::lockMutex(mMutex);
        mLocked = true;

        mQueue.push_back(data);

        mLocked = false;
        Mutex::unlockMutex(mMutex);
    }

    /// Dequeue an item from the front of the queue.
    bool dequeue(T& res)
    {
        bool ret = false;

        Mutex::lockMutex(mMutex);
        mLocked = true;
        if (mQueue.size())
        {
            res = mQueue.first();
            mQueue.pop_front();
            ret = true;
        }
        mLocked = false;
        Mutex::unlockMutex(mMutex);

        return ret;
    }

    /// Remove all items from the queue.
    void clear()
    {
        Mutex::lockMutex(mMutex);
        mLocked = true;

        mQueue.clear();

        mLocked = false;
        Mutex::unlockMutex(mMutex);
    }

    /// Sort everything in the queue using the provided qsort-compatible
    /// function.
    void sort(int(*compareFunc)(const T* a, const T* b))
    {
        Mutex::lockMutex(mMutex);
        mLocked = true;

        dQsort(mQueue.address(), mQueue.size(), sizeof(T), compareFunc);

        mLocked = false;
        Mutex::unlockMutex(mMutex);
    }

    /// Lock the vector for direct access.
    Vector<T>& lockVector()
    {
        Mutex::lockMutex(mMutex);
        AssertFatal(!mLocked, "ThreadSafeQueue<T>::lockVector - already locked!");
        mLocked = true;

        return mQueue;
    }

    /// Unlock the vector once we're done with it.
    void unlockVector()
    {
        AssertFatal(mLocked, "ThreadSafeQueue<T>::unlockVector - not locked!");
        mLocked = false;
        Mutex::unlockMutex(mMutex);
    }
};

#endif