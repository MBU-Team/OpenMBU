//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------
#ifndef _ATLASDEFERREDFILE_H_
#define _ATLASDEFERREDFILE_H_

#include "platform/platform.h"
#include "platform/platformThread.h"
#include "platform/platformMutex.h"
#include "platform/platformSemaphore.h"
#include "atlas/core/atlasDeferredIO.h"
#include "atlas/core/threadSafeQueue.h"
#include "core/stream.h"
#include "platform/platform.h"

/// Basic background IO implementation for Atlas files.
///
/// This class is responsible for executing requests described by
/// AtlasDeferredIO structures.
///
/// It also provides a mechanism for acquiring the file's stream
/// exclusively, and allowing other code to do IO on that stream, without
/// disturbing the deferred IO tasks.
///
/// @ingroup AtlasCore
class AtlasDeferredFile
{
    const char* mFilename;

    void* mStreamMutex;
    Stream* mStream;

    ThreadSafeQueue<AtlasDeferredIO*> mDeferredRequestQueue;
    ThreadSafeQueue<AtlasDeferredIO*> mDeferredResultQueue;

    bool mCanWriteStream;

    void* mLoadingMutex;
    void* mLoadingSemaphore;
    Thread* mDeferredLoaderThread;
    volatile bool mDeferredLoaderThreadActive;

    static void loaderThunk(void* a);
    void loaderThread();

    volatile U32 mTimeSpentWorking;
    volatile U32 mTimeSpentWaiting;
    volatile U32 mLoadOperationsCompleted;

public:
    AtlasDeferredFile();
    ~AtlasDeferredFile();

    /// Initialize access to a specified file, with the appropriate IO mode.
    bool initStream(const char* filename, U32 mode);

    /// Start the deferred IO thread.
    void start();

    /// Stop the deferred IO thread.
    void stop();

    /// Synch with the deferred IO thread and do appropriate callbacks for any
    /// finished IO operations, as described in the AtlasDeferredIO class.
    void sync();

    /// Queue an ADIO.
    void queue(AtlasDeferredIO* io);

    /// Make sure that we have acquired ability to write to stream.
    void ensureStreamWritable();

    /// Get the file stream for exclusive access. Don't forget to call unlockStream()!
    inline Stream* lockStream(bool needWrite = false)
    {
        if (needWrite)
            ensureStreamWritable();

        Mutex::lockMutex(mStreamMutex);
        return mStream;
    }

    /// Unlock the stream after exclusive mode IO is completed.
    inline void unlockStream()
    {
        Mutex::unlockMutex(mStreamMutex);
    }

    /// Do we currently have an active stream?
    inline const bool hasStream() const
    {
        return mStream;
    }

    /// Return true if we currently have an IO activity going on.
    inline bool isLoading()
    {
        bool isIt = false;

        // If there's anything in the semaphore, we're loading.
        if (Semaphore::acquireSemaphore(mLoadingSemaphore, false))
        {
            Semaphore::releaseSemaphore(mLoadingSemaphore);
            isIt = true;
        }

        return isIt;
    }

    void* getLoadingMutex()
    {
        return mLoadingMutex;
    }

    /// Return true if we have any pending or in-process IO operations.
    bool hasPendingIO();

    /// Return the time in ms that the IO thread has spent working.
    const U32 getTimeSpentWorking() const
    {
        return mTimeSpentWorking;
    }

    /// Return the time in ms that the IO thread has spent waiting.
    const U32 getTimeSpentWaiting() const
    {
        return mTimeSpentWaiting;
    }

    /// Return the number of operation the background thread has completed.
    const U32 getLoadOperationsCompleted() const
    {
    }
};

#endif