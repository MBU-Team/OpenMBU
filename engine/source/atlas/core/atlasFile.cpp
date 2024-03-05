//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------
#include "core/resManager.h"
#include "core/fileStream.h"
#include "core/frameAllocator.h"
#include "core/memStream.h"
#include "console/console.h"
#include "console/simBase.h"
#include "util/fourcc.h"
#include "util/safeDelete.h"
#include "atlas/core/atlasFile.h"
#include "atlas/core/atlasBaseTOC.h"
#include "atlas/core/atlasClassFactory.h"
#include "platform/profiler.h"

ResourceInstance* constructAtlasFileResource(Stream& stream)
{
    // Since we do all our IO elsewhere, this just has to let the
    // resmanager do some magic so we only have one instance per file.
    return new AtlasFile();
}

Resource<AtlasFile> AtlasFile::load(const char* filename)
{
    Resource<AtlasFile> af;

    af = ResourceManager->load(filename);

    // If we didn't get something back from the resman or we fail to open it,
    // then clean up and error out.
    if (af.isNull())
    {
        Con::errorf("AtlasFile::load - cannot open atlas file '%s'.", filename);
        return af;
    }

    if (!af->hasStream())
    {
        Con::printf("AtlasFile::load - loading Atlas resource %x with %s", (AtlasFile*)af, filename);
        if (!af->open(filename))
            Con::errorf("AtlasFile::load - cannot open atlas file '%s'.", filename);
    }

    AssertWarn(!af.isNull() && af->getTOCCount(), "AtlasFile::load - loaded empty atlas file!?");

    return af;
}


//------------------------------------------------------------------------

bool AtlasFile::smLogStubLoadStatus = false;

AtlasFile::AtlasFile()
{
    mLastSyncTime = Sim::getCurrentTime();
    mDeserializerThread = NULL;
    mDeserializerThreadActive = false;
    mLastProcessedTOC = 0;

    mDeserializerSemaphore = Semaphore::createSemaphore(0);
}

AtlasFile::~AtlasFile()
{
    // Clear out any pending writes...
    waitForPendingWrites();

    // And stop our threads.
    stopLoaderThreads();

    // Do a little cleanup as well!
    Semaphore::destroySemaphore(mDeserializerSemaphore);
}

//-----------------------------------------------------------------------------


bool AtlasFile::createNew(const char* filename)
{
    // Blank the file if it exists.
    {
        Stream* fs;

        // Wipe it first.
        if (!ResourceManager->openFileForWrite(fs, filename, File::Write))
        {
            Con::errorf("AtlasFile::createNew - failed to open '%s' for wipe! "
                "Are you currently using it in an AtlasInstance2?", filename);
            return false;
        }

        delete fs;
    }

    // Initialize our stream.
    if (!mFile.initStream(filename, FileStream::ReadWrite))
        return false;

    Stream* s = mFile.lockStream(true);

    // Write out headers.

    // Start out with a fourcc. :)
    const U32 ourFourCC = MAKEFOURCC('A', 'T', 'L', 'S');
    s->write(ourFourCC);

    // Write out TOC count.
    U8 tocCount = mTOCs.size();
    s->write(tocCount);

    // Write out our current TOCs. TOCs are fixed size based on their
    // element count, conveniently... so they can sit forever at the start
    // of the file without breaking anything when we change data.
    for (S32 i = 0; i < mTOCs.size(); i++)
    {
        // Encode the type, so we can deserialize...
        const char* tocType = AtlasClassFactory::getTOCName(mTOCs[i]);
        AssertISV(tocType, "AtlasFile::createNew - unknown TOC type!");
        s->writeString(tocType);

        // Write ourselves, as well.
        mTOCs[i]->write(s);
    }

    // Since the TOCs are all initialized with empty stubs, we basically
    // have an empty atlas file at this point, which will be filled out as
    // we go (probably have an import step as next thing from this point).

    // So we're good to go.
    mFile.unlockStream();
    return true;
}

bool AtlasFile::open(const char* filename)
{
    // Initialize our stream.
    if (!mFile.initStream(filename, FileStream::Read))
        return false;

    // Ok, got a stream... let's parse the headers, need some TOCs.
    Stream* s = mFile.lockStream();

    // Header - check the 4cc
    U32 fourCC;
    s->read(&fourCC);

    if (fourCC != MAKEFOURCC('A', 'T', 'L', 'S'))
    {
        // Bad fourcc, what's up here?
        Con::errorf("AtlasFile::open - encountered unexpected fourcc at start of file, aborting!");
        return false;
    }

    // Read in our TOCs.
    U8 tocCount;
    s->read(&tocCount);

    mTOCs.setSize(tocCount);
    for (S32 i = 0; i < mTOCs.size(); i++)
    {
        // Create the right class...
        char buff[256];
        s->readString(buff);

        AtlasTOC* toc = AtlasClassFactory::factoryTOC(buff);

        if (toc)
        {
            // Got it, read it.
            toc->read(s);

            // Register it.
            toc->mFile = this;
            mTOCs[i] = toc;
        }
        else
        {
            // Didn't get it, warn and try to skip.
            Con::warnf("AtlasFile::open - encountered unknown TOC type, attempting to skip...");

            // We assume first U32 of a TOC is its size, so read that and advance.
            U32 size;
            s->read(&size);
            s->setPosition(s->getPosition() + size);
        }
    }

    // we have now loaded our TOCs and are ready to process data requests.
    mFile.unlockStream();
    return true;
}

//-----------------------------------------------------------------------------

bool AtlasFile::optimize(const char* filename)
{
    // Open the optimized output file.

    // Write TOCs (in normal order)

    // Sort TOCs by optimization ID

    // write latest chunks.

    // done.
    return true;
}

//-----------------------------------------------------------------------------

void AtlasFile::registerTOC(AtlasTOC* toc)
{
    AssertFatal(toc->mFile == NULL,
        "AtlasFile::registerTOC - registering a TOC that seems to belong to someone else!");

    toc->mFile = this;
    mTOCs.push_back(toc);
}

//----------------------------------------------------------------------------

void AtlasFile::deserializerThunk(void* d)
{
    ((AtlasFile*)d)->deserializerThread();
}

void AtlasFile::startLoaderThreads()
{
    mFile.start();

    if (!mDeserializerThreadActive)
    {
        mDeserializerThreadActive = true;
        mDeserializerThread = new Thread(deserializerThunk, this);
    }
}

void AtlasFile::stopLoaderThreads()
{
    mFile.stop();

    if (mDeserializerThreadActive)
    {
        AssertFatal(mDeserializerThread, "AtlasFile::stopLoaderThread - don't have a thread!");

        mDeserializerThreadActive = false;

        // Flag the thread so it'll exit.
        Semaphore::releaseSemaphore(mDeserializerSemaphore);

        SAFE_DELETE(mDeserializerThread);
    }
}

void AtlasFile::syncThreads()
{
    // Compare current time with last, don't update if it's less than 10ms or so.
    const U32 nextTime = Platform::getRealMilliseconds();
    if ((nextTime - mLastSyncTime) < 10)
        return;


    PROFILE_START(AtlasFile_syncThreads);
    //Con::printf("----------- sync ----------------");

    mFile.sync();

    // Do a sync callback for our TOCs
    PROFILE_START(AtlasFile_syncThreads_callback);
    F32 dt = F32(nextTime - mLastSyncTime) / 1000.f;
    for (S32 i = 0; i < mTOCs.size(); i++)
        mTOCs[i]->syncCallback(dt);
    mLastSyncTime = nextTime;
    PROFILE_END();

    // First, let's dequeue all our in-flight loads so we don't queue the same
    // thing twice. To make sure we don't get diddled, we also have to lock
    // the pending queue.

    // We'd better be sure we unlock these. Order is important to prevent
    // deadlocks.
    PROFILE_START(AtlasFile_syncThreads_notes);
    Vector<AtlasReadNote*>& pendingQ = mPendingLoadQueue.lockVector();
    Vector<AtlasReadNote*>& inProcessQ = mLoadingNotes.lockVector();

    // Filter out loads in process.
    for (S32 i = 0; i < inProcessQ.size(); i++)
    {
        const AtlasReadNote* arn = inProcessQ[i];
        arn->toc->cbOnChunkReadStarted(arn->stubIdx);
    }

    /*if(inProcessQ.size())
       Con::printf("    %d are in-process", inProcessQ.size()); */

    inProcessQ.clear();

    // Update our load queue.
    Vector<AtlasReadNote*>* updateList = new Vector<AtlasReadNote*>[mTOCs.size()];
    U32 remaining = 0;

    const U32 numToFetch = 5;

    for (S32 i = 0; i < mTOCs.size(); i++)
    {
        updateList[i].reserve(numToFetch);
        mTOCs[i]->recalculateUpdates(updateList[i], numToFetch);
        remaining += updateList[i].size();
    }

    /*if(remaining)
       Con::printf("    %d in queue", remaining);*/

       // Round-robin add them to the master queue.
    {
        // Wipe pending ARNs... this is kinda gross.
        for (S32 i = 0; i < pendingQ.size(); i++)
        {
            SAFE_DELETE(pendingQ[i]);
        }
        pendingQ.clear();

        // And fill it up again, round robin.
        U32 originalRemaining = remaining;
        pendingQ.reserve(remaining);
        while (remaining)
        {
            for (S32 i = 0; i < mTOCs.size(); i++)
            {
                if (updateList[i].size())
                {
                    pendingQ.push_back(updateList[i].first());
                    updateList[i].pop_front();
                    remaining--;
                }
            }
        }

        AssertFatal(pendingQ.size() == originalRemaining,
            "AtlasFile::syncThreads - unexpected pending queue size after population.");
    }

    // Remember what I said about locking order before?
    mLoadingNotes.unlockVector();
    mPendingLoadQueue.unlockVector();

    PROFILE_END();

    // Clean up our temporary queues.
    delete[] updateList;

    PROFILE_START(AtlasFile_syncThreads_instatement);

    // Pass stuff to the TOCs for instatement.
    {
        FrameAllocatorMarker famAlloc;

        // Copy it out, then wipe the master, as instatement may take a while.
        Vector<AtlasReadNote*>& lockedProcessQ = mPendingProcessQueue.lockVector();
        Vector<AtlasReadNote*>& lockedInProcessQ = mLoadingNotes.lockVector();

        //  Copy our pending data to a buffer so we can work on it.
        U32 pendingCount = lockedProcessQ.size();
        AtlasReadNote** pendingData = (AtlasReadNote**)famAlloc.alloc(sizeof(AtlasReadNote*) * pendingCount);
        dMemcpy(pendingData, lockedProcessQ.address(), sizeof(AtlasReadNote*) * pendingCount);
        lockedProcessQ.clear();

        mPendingProcessQueue.unlockVector();

        // And do the instating, removing from the loadNoteQueue as needed.
        for (S32 i = 0; i < pendingCount; i++)
        {
            PROFILE_START(AtlasFile_syncThreads_instatement_callback);
            pendingData[i]->toc->cbOnChunkReadComplete(pendingData[i]->stubIdx, pendingData[i]->chunk);
            PROFILE_END();

            // Remove the ARN from the in process queue, so we won't puke
            // trying to mark it as pending later on.
            for (S32 j = 0; j < lockedInProcessQ.size(); j++)
            {
                if (lockedInProcessQ[j] == pendingData[i])
                {
                    lockedInProcessQ.erase_fast(j);
                    break;
                }
            }

            // Get rid of the ARN.
            delete pendingData[i];
        }

        mLoadingNotes.unlockVector();
    }

    PROFILE_END();

    // Make sure we're loading.
    enqueueNextPendingLoad(false);

    PROFILE_END();
}

//-----------------------------------------------------------------------------

void AtlasFile::deserializerThread()
{
    while (true)
    {
        // Block on a semaphore - on wake we're either quitting or have work.
        Semaphore::acquireSemaphore(mDeserializerSemaphore, true);

        // Time to quit!
        if (!mDeserializerThreadActive)
            return;

        // Otherwise, we have work, let's get on with it.
        AtlasReadNote* arn;

        // If we didn't get anything, stop and try again.
        if (!mPendingDeserializeQueue.dequeue(arn))
        {
            Con::warnf("AtlasFile::deserializerThread - odd, nothing in pending "
                "deserialize queue, was I woken for nothing? Sknxx...");
            continue;
        }

        // Otherwise, we've some work to do!
        AssertFatal((volatile void*)arn->adio->data, "AtlasFile::deserializerThread - no data in ADIO! (1)");
        AssertFatal((void*)arn->adio->data, "AtlasFile::deserializerThread - no data in ADIO! (2)");

        // Ok, we've got a request to process... let's do it!
        AssertFatal(arn->adio->data, "AtlasFile::deserializerThread - no data in ADIO!");
        MemStream ms(arn->adio->length, arn->adio->data, true, false);
        arn->chunk = arn->toc->constructNewChunk();
        AtlasChunk::readFromStream(arn->chunk, &ms);

        // We can free our read data at this point, since it's been converted
        // into the chunk.
        SAFE_DELETE(arn->adio->data);

        // Note to console we loaded something, if desired.
        if (smLogStubLoadStatus)
            arn->toc->dumpStubLoadStatus(arn->stubIdx);

        // And let's queue it up to be instated.
        mPendingProcessQueue.queue(arn);

        // We're probably going to be processing something, so let's enqueue
        // the next load, if any.
        enqueueNextPendingLoad(false);

        // And sleep for just a sec so we don't saturate the CPU.
        Platform::sleep(1);
    }
}

//-----------------------------------------------------------------------------

void AtlasFile::enqueueNextPendingLoad(bool waitTillNext)
{
    PROFILE_START(AtlasFile_enqueueNextPendingLoad);

    // We can early out if we aren't forcing a wait and it's loading.
    if (!waitTillNext && mFile.hasPendingIO())
    {
        PROFILE_END();
        return;
    }

    // We acquire a lock while posting, so we don't have to add any extra
    // checks to get our wait behavior.

    // This doesn't seem to be needed.
    //Mutex::lockMutex(mFile.getLoadingMutex());

    AtlasReadNote* arn = NULL;

    // Fetch a new item.
    {
        Vector<AtlasReadNote*>& pendingQ = mPendingLoadQueue.lockVector();

        // Save so we know when we've tried all possibilities.
        U32 originalTOC = mLastProcessedTOC;

        const U32 peekSize = getMin(mTOCs.size(), pendingQ.size());

        do
        {
            U32 localPeekSize = peekSize;

            // Scan for a hit on our current favored TOC.
            for (S32 i = 0; i < pendingQ.size() && i < localPeekSize; i++)
            {
                // If it's NULL, make us look a little longer.
                if (!pendingQ[i])
                {
                    localPeekSize++;
                    continue;
                }

                if (pendingQ[i]->toc == mTOCs[mLastProcessedTOC])
                {
                    // Got a hit, snag it and exit out.
                    arn = pendingQ[i];

                    // We just mark w/ null as we'll be regenerating it soon enough.
                    pendingQ[i] = NULL;

                    // And skip out.
                    break;
                }
            }

            // Advanced to next TOC...
            mLastProcessedTOC = (mLastProcessedTOC + 1) % mTOCs.size();
        } while (!arn && mLastProcessedTOC != originalTOC); // If we found a hit, or we wrapped around, stop.

        mPendingLoadQueue.unlockVector();
    }

    if (arn)
    {
        PROFILE_START(AtlasFile_enqueueNextPendingLoad_arn);
        // Note that we've started the load on this, to squelch further
        // load attempts.
        mLoadingNotes.queue(arn);

        // We have to construct our ADIO at this point.
        arn->toc->cbPopulateChunkReadNote(arn);

        // And queue the ADIO.
        mFile.queue(arn->adio);
        PROFILE_END();
    }

    //Mutex::unlockMutex(mFile.getLoadingMutex());

    PROFILE_END();
}

//-----------------------------------------------------------------------------

void AtlasFile::waitForPendingWrites()
{
    Con::printf("AtlasFile::waitForPendingWrites - Waiting for pending output to finish.");

    while (getDeferredFile().hasPendingIO())
    {
        Con::printf("AtlasFile (%x) - Waiting for write IO to finish...", this);
        Platform::sleep(32);
    }

    Con::printf("AtlasFile::waitForPendingWrites - Done!");
}

void AtlasFile::precache()
{
    U32 pendingCount = 1;
    while (pendingCount)
    {
        syncThreads();

        Platform::sleep(32);

        // Check for pending data; if there is some just skip to a synch.
        pendingCount = mPendingDeserializeQueue.lockVector().size();
        mPendingDeserializeQueue.unlockVector();

        if (pendingCount)
            continue;

        pendingCount = mPendingLoadQueue.lockVector().size();
        mPendingLoadQueue.unlockVector();

        if (pendingCount)
            continue;

        pendingCount = mPendingProcessQueue.lockVector().size();
        mPendingProcessQueue.unlockVector();
    }
}

void AtlasFile::dumpLoadQueue()
{
    Con::printf("AtlasFile ---- Load Queue Report --------------------------");

    // Generate a vector with our load requests, this code is based on what's in
    // sync() so make sure we stay matched up with that!

    // Populate our load queue...
    Vector<AtlasReadNote*>* updateList = new Vector<AtlasReadNote*>[mTOCs.size()];
    U32 remaining = 0;

    for (S32 i = 0; i < mTOCs.size(); i++)
    {
        mTOCs[i]->recalculateUpdates(updateList[i], 100000);
        Con::printf("   %d in a %s", updateList[i].size(), AtlasClassFactory::getTOCName(mTOCs[i]));

        remaining += updateList[i].size();
    }

    Con::printf("   %d total items.", remaining);

    // Simulate our round-robin loading strategy, but dump to console.
    while (remaining)
    {
        for (S32 i = 0; i < mTOCs.size(); i++)
        {
            // Skip if empty...
            if (!updateList[i].size())
                continue;

            // Something here, dump it.
            AtlasReadNote* arn = updateList[i].first();
            updateList[i].pop_front();

            // Let the TOC report on it.
            arn->toc->dumpStubLoadStatus(arn->stubIdx);
            remaining--;
        }
    }

}