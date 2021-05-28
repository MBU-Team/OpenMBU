//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------
#include "core/frameAllocator.h"
#include "atlas/core/atlasDeferredFile.h"
#include "platform/profiler.h"
#include "util/safeDelete.h"
#include "console/console.h"
#include "core/fileStream.h"

AtlasDeferredFile::AtlasDeferredFile()
{
    mFilename = NULL;
    mStream = NULL;
    mCanWriteStream = false;

    mStreamMutex = Mutex::createMutex();
    mLoadingMutex = Mutex::createMutex();
    mLoadingSemaphore = Semaphore::createSemaphore(0);

    mDeferredLoaderThread = NULL;
    mDeferredLoaderThreadActive = false;

    mTimeSpentWaiting = mTimeSpentWorking = mLoadOperationsCompleted = 0;
}

AtlasDeferredFile::~AtlasDeferredFile()
{
    stop();

    Mutex::destroyMutex(mStreamMutex);
    Mutex::destroyMutex(mLoadingMutex);
    Semaphore::destroySemaphore(mLoadingSemaphore);

    if (mStream)
    {
        // This is a little hacky...
        AssertFatal(dynamic_cast<FileStream*>(mStream),
            "AtlasDeferredFile::~AtlasDeferredFile - trying to clean up a non-FileStream!");
        ((FileStream*)mStream)->flush();
        ((FileStream*)mStream)->close();
        delete mStream;
    }
}

bool AtlasDeferredFile::initStream(const char* filename, U32 mode)
{
    // Don't forgot to lock.
    lockStream();

    // Kill old file if any.
    SAFE_DELETE(mStream);

    // Open the file.

    // For now bypass resmanager, we need read/write.
    mFilename = filename; //ResourceManager->getPathOf(filename);

    if (!mFilename)
    {
        Con::errorf("AtlasFile::initStream - filename '%s' is invalid!", mFilename);
        unlockStream();
        return false;
    }

    FileStream* fs = new FileStream();
    if (!fs->open(mFilename, (FileStream::AccessMode)mode))
    {
        Con::errorf("AtlasDeferredFile::initSream - failed to open '%s'! aborting...", filename);

        SAFE_DELETE(fs);
        mFilename = NULL;
        unlockStream();
        return false;
    }

    mStream = fs;
    unlockStream();
    return true;
}

//-----------------------------------------------------------------------------

void AtlasDeferredFile::loaderThunk(void* a)
{
    ((AtlasDeferredFile*)a)->loaderThread();
}

void AtlasDeferredFile::loaderThread()
{
    while (true)
    {
        AtlasDeferredIO* adio = NULL;

        U32 waitStartTime = Platform::getRealMilliseconds();

        Semaphore::acquireSemaphore(mLoadingSemaphore, true);

        mTimeSpentWaiting += Platform::getRealMilliseconds() - waitStartTime;

        // May have been waken to quit.
        if (!mDeferredLoaderThreadActive)
            return;

        // Otherwise, do our processing...
        Mutex::lockMutex(mLoadingMutex);

        if (mDeferredRequestQueue.dequeue(adio))
        {
            U32 workStartTime = Platform::getRealMilliseconds();

            Mutex::lockMutex(mStreamMutex);

            // Got something, let's do the load.
            adio->doAction(mStream);

            Mutex::unlockMutex(mStreamMutex);

            AssertFatal(adio->data, "AtlasDeferredFile::loaderThread - bad ADIO data.");

            // Complete, or queue for completion.
            if (adio->flags.test(AtlasDeferredIO::CompleteOnSync))
                mDeferredResultQueue.queue(adio);
            else
                adio->complete();

            mTimeSpentWorking += Platform::getRealMilliseconds() - workStartTime;
        }

        Mutex::unlockMutex(mLoadingMutex);

    }
}

void AtlasDeferredFile::queue(AtlasDeferredIO* adio)
{
    AssertISV(adio->length, "AtlasDeferredFile::queue - zero length!");

    if (adio->isImmediate())
    {
        adio->doAction(lockStream(adio->isWrite()));
        unlockStream();

        if (adio->flags.test(AtlasDeferredIO::CompleteOnSync))
            mDeferredResultQueue.queue(adio);
        else
            adio->complete();
    }
    else
    {
        mDeferredRequestQueue.queue(adio);
        Semaphore::releaseSemaphore(mLoadingSemaphore);
    }
}

//-----------------------------------------------------------------------------
bool AtlasDeferredFile::hasPendingIO()
{
    PROFILE_START(AtlasDeferredFile_hasPendingIO);
    if (isLoading())
    {
        PROFILE_END();
        return true;
    }

    const Vector<AtlasDeferredIO*>& tmp = mDeferredRequestQueue.lockVector();
    const U32 count = tmp.size();
    mDeferredRequestQueue.unlockVector();

    PROFILE_END();
    return count != 0;
}

void AtlasDeferredFile::start()
{
    AssertISV(mStream, "AtlasDeferredFile::start - no stream!");

    if (!mDeferredLoaderThreadActive)
    {
        AssertFatal(!mDeferredLoaderThread, "AtlasDeferredFile::start - already had a thread.");
        mDeferredLoaderThreadActive = true;
        mDeferredLoaderThread = new Thread(loaderThunk, this, true);
    }
}

void AtlasDeferredFile::sync()
{
    // Iterate over our deferred results and call complete on them,
    // then clear the queue.

    PROFILE_START(AtlasDeferredFile_sync_lock);
    // Copy results to a temp buffer to minimize lock time.
    FrameAllocatorMarker copyFam;

    Vector<AtlasDeferredIO*>& resQ = mDeferredResultQueue.lockVector();
    U32 resCount = resQ.size();
    AtlasDeferredIO** resData = (AtlasDeferredIO**)copyFam.alloc(resCount * sizeof(AtlasDeferredIO*));
    dMemcpy(resData, resQ.address(), resCount * sizeof(AtlasDeferredIO*));
    resQ.clear();
    mDeferredResultQueue.unlockVector();

    PROFILE_END();

    PROFILE_START(AtlasDeferredFile_sync_process);
    // Now process results.
    for (S32 i = 0; i < resCount; i++)
        resData[i]->complete();
    PROFILE_END();
}

void AtlasDeferredFile::stop()
{
    if (mDeferredLoaderThreadActive)
    {
        AssertFatal(mDeferredLoaderThread, "AtlasDeferredFile::stop - no thread.");

        // Note we want to stop.
        mDeferredLoaderThreadActive = false;

        // Poke the thread so it'll stop.
        Semaphore::releaseSemaphore(mLoadingSemaphore);

        // And clean up.
        SAFE_DELETE(mDeferredLoaderThread);
    }
}

void AtlasDeferredFile::ensureStreamWritable()
{
    if (mCanWriteStream)
        return;

    Con::printf("AtlasDeferredFile::ensureStreamWritable - switching file into writable mode... This may cause a crash if you have other copies of Torque accessing it!");

    // Reinit the stream if we can't write.
    initStream(mFilename, FileStream::ReadWrite);
}