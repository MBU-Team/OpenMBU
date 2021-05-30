//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------
#ifndef _ATLASGEOMFILE_H_
#define _ATLASGEOMFILE_H_

#include "platform/platform.h"
#include "platform/platformThread.h"
#include "platform/platformMutex.h"
#include "platform/platformSemaphore.h"
#include "util/safeDelete.h"
#include "core/stream.h"
#include "core/tVector.h"
#include "core/resManager.h"
#include "math/mMath.h"
#include "atlas/core/atlasChunk.h"
#include "atlas/core/atlasDeferredIO.h"
#include "atlas/core/atlasDeferredFile.h"

/// @defgroup Atlas Atlas Terrain System
///
/// Atlas is a flexible framework for paged terrain.

/// @defgroup AtlasCore Atlas Core
/// 
/// Core Atlas classes and functionality. The basic resource paging 
/// implementation lives here, including chunks, TOCs, and the AtlasFile.
///
/// Actual functionality for editing, resource, and instance areas of Atlas
/// lives in the appropriate directories.
/// @ingroup Atlas

///
class AtlasFile;
class AtlasTOC;
class AtlasChunk;

/// Used to track a load request made upon an AtlasFile.
///
/// This structure contains pointers to all the information needed to
/// accomplish a background load through an AtlasFile.
///
/// @ingroup AtlasCore
struct AtlasReadNote
{
    AtlasReadNote()
    {
        toc = NULL;
        adio = NULL;
        chunk = NULL;
    }

    ~AtlasReadNote()
    {
        toc = (AtlasTOC*)(-1);
        chunk = (AtlasChunk*)(-1);
        SAFE_DELETE(adio);
    }

    /// The TOC this request is made upon.
    AtlasTOC* toc;

    /// Index to the stub we're loading the chunk for.
    ///
    /// An index is used to allow type independence.
    U32 stubIdx;

    /// Reference to the loaded chunk; populated at chunk read time.
    AtlasChunk* chunk;

    /// The ADIO describing the raw file data from which the chunk will
    /// be read.
    AtlasDeferredIO* adio;
};

/// Represents a single .atlas file; provides an interface for performing
/// threaded reads and writes against that file, as well as tracking 
/// the TOCs in that file.
///
/// It also deals with getting the resource system to play nice with Atlas.
///
/// @ingroup AtlasCore
class AtlasFile : public ResourceInstance
{
    typedef ResourceInstance Parent;
    friend class AtlasTOC;

protected:

    /// We actually manage the raw file IO with a subclass.
    ///
    /// @see AtlasDeferredFile for more information.
    AtlasDeferredFile mFile;

    /// List of the TOCs which are present in this file, in order.
    Vector<AtlasTOC*> mTOCs;

    /// Queue of notes that have had processing started on them by the IO 
    /// thread.
    ThreadSafeQueue<AtlasReadNote*> mLoadingNotes;

    /// Queue of notes that are awaiting processing by the IO thread.
    ThreadSafeQueue<AtlasReadNote*> mPendingLoadQueue;

    /// Queue of notes that are done with IO and awaiting deserialization.
    ThreadSafeQueue<AtlasReadNote*> mPendingDeserializeQueue;

    /// Queue of notes that are ready for processing by the main thread.
    ThreadSafeQueue<AtlasReadNote*> mPendingProcessQueue;

    /// Helper function to make sure we've got another note in flight.
    void enqueueNextPendingLoad(bool waitTillNext = true);

    /// The deserialization thread. This thread is responsible for performing
    /// the appropriate deserialization on raw data once it is read from disk,
    /// and queuing it for instatement.
    ///
    /// @see deserializerThread
    Thread* mDeserializerThread;

    /// Control flag for the deserialization thread.
    volatile bool mDeserializerThreadActive;

    void* mDeserializerSemaphore;

    /// Track time of last sync() call.
    U32 mLastSyncTime;

    void deserializerThread();
    static void deserializerThunk(void* d);

    U32 mLastProcessedTOC;

public:

    /// Do all loads through THIS method, not the ResourceManager.
    ///
    /// We rely on the ResourceManager to track active Atlas instances and 
    /// load/unload the AtlasFile as needed, but in order to properly bind
    /// the AtlasFile instance to the actual file, we have to bypass some
    /// parts what the ResourceManager does.
    static Resource<AtlasFile> load(const char* filename);

    AtlasFile();
    ~AtlasFile();

    /// Add a TOC to the file. This causes undefined (bad) results if you run
    /// it on a pre-existing file, as it can't insert new data into an active
    /// Atlas file. (It would have to move all the chunks down, which would
    /// be both tremendously slow and also difficult to orchestrate with
    /// loading threads active.)
    void registerTOC(AtlasTOC*);

    /// Get TOC by slot. This is typed so you always get the right sort of
    /// TOC. Slot order is important, and reflects the order the TOCs are
    /// written.
    ///
    /// Basically, the slot-th toc of a given type is the slot-th TOC of that
    /// type which was written to the file. So a file might have a slot 0
    /// geometry TOC, and then texture TOCs in slots 0 and 1 (for opacity
    /// maps and a lightmap). (In this scenario, their write order might be
    /// opacity map, geometry, lightmap - but we only consider TOCs of the
    /// same type when identifying slot order.)
    template<class T> inline bool getTocBySlot(U32 slot, T*& toc);

    /// Open file, read in TOCs, and get ready for threads to be started.
    bool open(const char* filename);

    /// Create a new Atlas file with the currently loaded TOCs.
    ///
    /// Atlas file construction works off of this pattern:
    ///    1. Allocate a new AtlasFile.
    ///    2. Create all TOCs.
    ///    3. Call registerTOC on them.
    ///    4. createNew().
    bool createNew(const char* filename);

    /// Optimize to given path. Optimization entails writing the file from
    /// scratch, respecting the optimizeOrder of the stubs. This has two
    /// results. First, you get rid of any old chunk data. Second, it lets
    /// you optimize chunk order to reflect file IO patterns.
    ///
    /// @todo Currently unimplemented.
    bool optimize(const char* filename);

    void startLoaderThreads();
    void stopLoaderThreads();

    /// Once loader threads have been started, make sure to call syncThreads
    /// every so often. This synchronizes with the AtlasDeferredFile, with the
    /// various TOCs in this file, and does final loading steps.
    ///
    /// But as it happens you DON'T have to call this if all you're doing is
    /// writing.
    ///
    /// You want to call this about once a frame in a runtime situation.
    void syncThreads();

    /// Get direct access to the AtlasDeferredFile, useful for directly
    /// manipulating the Stream.
    inline AtlasDeferredFile& getDeferredFile()
    {
        return mFile;
    }

    /// Does our AtlasDeferredFile have a stream associated with it? Mostly
    /// used for sanity checking.
    inline const bool hasStream() const
    {
        return mFile.hasStream();
    }

    /// Internal callback from AtlasResourceTOC<>.
    void queuePendingDeserialize(AtlasReadNote* arn)
    {
        AssertFatal(arn->adio->data, "AtlasFile::queuePendingDeserialize - no adio data!");

        // Add to queue.
        mPendingDeserializeQueue.queue(arn);

        // Release the semaphore so our flag will get scheduled.
        Semaphore::releaseSemaphore(mDeserializerSemaphore);
    }

    /// Helper function - after you've done a bunch of writes, BEFORE you
    /// destroy the AtlasFile, make sure to call this so everything gets
    /// flushed to disk. Otherwise you're likely to kill any in-flight writes.
    ///
    /// The Atlas file should stay uncorrupted (unless you manage to kill it
    /// in the middle of a write, which is fairly unlikely since all writes
    /// are atomic OS IO calls), but not calling this guy when you're done
    /// writing to an Atlas file is a good way to have missing data.
    void waitForPendingWrites();

    /// Spin until all of our load queues are empty and we've processed all
    /// pending items.
    void precache();

    /// Dump our current load priorities to the console, in the order we'll
    /// try to load them.
    void dumpLoadQueue();

    const U32 getTOCCount() const
    {
        return mTOCs.size();
    }

    static bool smLogStubLoadStatus;
};

template<class T>
bool AtlasFile::getTocBySlot(U32 slot, T*& toc)
{
    // Go down the list and the slot-th toc that casts properly.
    U32 curSlot = 0;
    for (S32 i = 0; i < mTOCs.size(); i++)
    {
        T* ptr = NULL;

        if (ptr = dynamic_cast<T*>(mTOCs[i]))
        {
            if (curSlot == slot)
            {
                toc = ptr;
                return true;
            }
            curSlot++;
        }
    }

    return false;
}

/// @ingroup AtlasCore
extern ResourceInstance* constructAtlasFileResource(Stream& s);

#endif