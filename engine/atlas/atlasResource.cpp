//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
// Some portions of Atlas code derived from Thatcher Ulrich's original
// ChunkLOD implementation.
//-----------------------------------------------------------------------------

#include "atlas/atlasResource.h"
#include "console/consoleTypes.h"
#include "sceneGraph/sceneGraph.h"
#include "sceneGraph/sceneState.h"
#include "materials/sceneData.h"
#include "platform/profiler.h"
#include "util/frustrumCuller.h"
#include "atlas/atlasCollision.h"
#include "atlas/atlasInstance.h"
#include "game/game.h"
#include "gfx/primBuilder.h"

// TODO - remove dependency on me.
#include "game/shapeBase.h"

AtlasResource* AtlasResource::smCurChunkResource = NULL;
Point3F           AtlasResource::smCurCamPos;
Point3F           AtlasResource::smCurObjCamPos;
SceneGraphData* AtlasResource::smCurSGD = NULL;
MatrixF           AtlasResource::smInvObjTrans;
Point2I           AtlasResource::smBoundChunkPos(0, 0);
U32               AtlasResource::smBoundChunkLevel = -1;

ResourceInstance* constructAtlasChunkFile(Stream& stream)
{
    AtlasResource* cr = new AtlasResource();

    // Resource manager is a bit goofy, we don't get to keep our 
    // stream! So we load the header info here and do paging off
    // of a separate stream the owning object helps us create...
    if (!cr->load(stream))
    {
        delete cr;
        return NULL;
    }

    return cr;
}

//------------------------------------------------------------------------------

AtlasResource::AtlasResource()
{
    // Most everything is set up in the load call.
    mOwnerCount = 0;

    mLoader = NULL;
    mColGridSize = -1;
    mColTreeDepth = -1;
    mInitialized = false;
    mPreloading = false;
    mStream = NULL;
    mTQTFile = NULL;

    mRunLoaderThread = false;
    mLoadMutex = NULL;
    mLoadMutex = NULL;
}

AtlasResource::~AtlasResource()
{
    // Make sure our thread is dead! This also cleans up other thread
    // related resources.
    stopLoader();

    for (S32 i = 0; i < MAX_LOADER_THREAD_REQUESTS; i++)
    {
        if (mLT_GeomRequest[i])
            mTLRAlloc.free((ThreadLoadRequest*)mLT_GeomRequest[i]);

        if (mLT_GeomRetire[i])
            mTLRAlloc.free((ThreadLoadRequest*)mLT_GeomRetire[i]);

        if (mLT_TexRequest[i])
            mTLRAlloc.free((ThreadLoadRequest*)mLT_TexRequest[i]);

        if (mLT_TexRetire[i])
            mTLRAlloc.free((ThreadLoadRequest*)mLT_TexRetire[i]);
    }

    while (mGeomRequest.size())
    {
        mTLRAlloc.free(mGeomRequest.last());
        mGeomRequest.decrement();
    }

    while (mTexRequest.size())
    {
        mTLRAlloc.free(mTexRequest.last());
        mTexRequest.decrement();
    }

    // Clean up our other allocations.
    SAFE_DELETE(mTracer);
    SAFE_DELETE(mStream);
    SAFE_DELETE(mTQTFile);
    SAFE_DELETE_ARRAY(mChunks);
    SAFE_DELETE_ARRAY(mChunkTable);
    mTexFreeList.clear();
}

void AtlasResource::purge()
{
    Con::printf("AtlasResource::purge - purging!");
    AtlasResource::setCurrentResource(this);

    for (S32 i = 0; i < mEntriesAllocated; i++)
    {
        // Forcibly unload every bit of data we have.
        mChunks[i].purge();
    }

    AtlasResource::setCurrentResource(NULL);
}

AtlasResourceEntry* AtlasResource::allocChunk(U32 lod, AtlasResourceEntry* parent)
{
    AssertFatal(mEntriesAllocated < mEntryCount,
        "AtlasResource::allocChunk - too many chunks!");
    AtlasResourceEntry* res = &mChunks[mEntriesAllocated];
    mEntriesAllocated++;

    res->mLod = lod;
    res->mParent = parent;

    return res;
}

bool AtlasResource::load(Stream& sx)
{
    // Make sure the texture free list is empty.
    mTexFreeList.clear();

    Stream* s = &sx;

    // Load file header...
    U32 tag;
    s->read(&tag); // BJGTODO - check the tag...

    S16 tmp16;
    s->read(&tmp16); // Version
    s->read(&tmp16); mTreeDepth = tmp16;
    s->read(&mError_LODMax);
    s->read(&mVerticalScale);
    s->read(&mBaseChunkSize);
    s->read(&mEntryCount);
    s->read(&mColTreeDepth);
    mColGridSize = BIT(mColTreeDepth - 1);

    // Create chunk lookup table.
    mChunkTable = new AtlasResourceEntry * [mEntryCount];
    dMemset(mChunkTable, 0, sizeof(mChunkTable[0]) * mEntryCount);

    // Prepare the chunk tree table.
    mEntriesAllocated = 0;
    mChunks = new AtlasResourceEntry[mEntryCount];
    for (S32 i = 0; i < mEntryCount; i++)
        constructInPlace(&mChunks[i]);

    setCurrentResource(this);

    // Now, load & initialize the tree...
    mEntriesAllocated++;
    mChunks[0].mLod = 0;
    mChunks[0].mParent = NULL;
    mChunks[0].read(s, mTreeDepth - 1);
    mChunks[0].lookupNeighbors();

    // Initialize the tracer.
    mTracer = new AtlasResourceTracer(this);

    // Clean up!
    setCurrentResource(NULL);

    return true;
}

bool AtlasResource::initialize(const char* chunkFile, const char* tqtFile)
{
    // First, initialize the TQT. If NULL we don't bother, which is ok as we're
    // a server object most likely.
    if (tqtFile)
    {
        SAFE_DELETE(mTQTFile);
        mTQTFile = new AtlasTQTFile(tqtFile);
    }

    // Open the chunk file.
    if (mStream)
        ResourceManager->closeStream(mStream);

    if (!(mStream = ResourceManager->openStream(chunkFile)))
    {
        Con::errorf("Atlas::load - error opening '%s' for read!", chunkFile);
        return false;
    }

    mInitialized = true;

    return true;
}

bool AtlasResource::castRay(const Point3F& start, const Point3F& end, RayInfo* info, bool emptyCollide)
{
    AssertISV(mTracer, "AtlasResource::castRay - no tracer available!");

    // We get the ray in object space, but we need to scale it be over 0..1 so
    // that the QuadTreeTracer will work nicely with it.

    F32 invSize = 1.f / (mBaseChunkSize * F32(BIT(mTreeDepth - 1)));

    Point3F fixedStart = start;
    fixedStart.x *= invSize;
    fixedStart.y *= invSize;

    Point3F fixedEnd = end;
    fixedEnd.x *= invSize;
    fixedEnd.y *= invSize;

    return mTracer->castRay(fixedStart, fixedEnd, info);
}

//------------------------------------------------------------------------

/// This exists solely to call loaderThread in the separate thread on the
/// appropriate object.
static void loaderThunk(void* arg)
{
    AtlasResource* obj = (AtlasResource*)arg;
    obj->loaderThread();
}

void AtlasResource::startLoader()
{
    AssertISV(mInitialized, "AtlasResource::startLoader - not initialized!");

    // Only one!
    if (mRunLoaderThread)
        return;

    // Clean up the buffers.
    for (S32 i = 0; i < MAX_LOADER_THREAD_REQUESTS; i++)
    {
        mLT_GeomRetire[i] = NULL;
        mLT_GeomRequest[i] = NULL;
        mLT_TexRetire[i] = NULL;
        mLT_TexRequest[i] = NULL;
    }

    mRunLoaderThread = true;
    mLoadMutex = Mutex::createMutex();
    mLoader = new Thread(loaderThunk, this, true);
}

void AtlasResource::stopLoader()
{
    if (mLoader && mRunLoaderThread)
    {
        // Tell it to die and wait...
        mRunLoaderThread = false;
        delete mLoader;
        mLoader = NULL;

        // All done!
        Mutex::destroyMutex(mLoadMutex);
        mLoadMutex = NULL;

        // One last sync to catch in-fligh results.
        sync();
    }
}

S32 FN_CDECL cmpTLR(const void* p1, const void* p2)
{
    const AtlasResource::ThreadLoadRequest* r1 = *((const AtlasResource::ThreadLoadRequest**)p1);
    const AtlasResource::ThreadLoadRequest* r2 = *((const AtlasResource::ThreadLoadRequest**)p2);

    return r2->mPriority - r1->mPriority;
}

bool AtlasResource::sync()
{
    // Skip if we've already updated this frame and we're not in a preload mode.
    if (ShapeBase::sLastRenderFrame == mLastSyncFrame && !mPreloading)
        return false;
    mLastSyncFrame = ShapeBase::sLastRenderFrame;


    // Sort the list of requests.
    PROFILE_START(AtlasResource_sync__Sort);
    dQsort(mGeomRequest.address(), mGeomRequest.size(), sizeof(ThreadLoadRequest*), cmpTLR);
    dQsort(mTexRequest.address(), mTexRequest.size(), sizeof(ThreadLoadRequest*), cmpTLR);
    PROFILE_END();

    // Get a lock so we can manipulate the thread's request buffers...
    acquireLoadMutex();

    // Suck everything from the retire queues.
    for (S32 i = 0; i < MAX_LOADER_THREAD_REQUESTS; i++)
    {
        volatile ThreadLoadRequest* geomR = mLT_GeomRetire[i];
        volatile ThreadLoadRequest* texR = mLT_TexRetire[i];

        // Skip if nothing there.
        if (geomR)
        {
            PROFILE_START(AtlasResource_sync__GeomInstate);

            if (!mPreloading && geomR->mChunk->mParent
                && !geomR->mChunk->mParent->mGeom)
            {
                // Drat!  Our parent data was unloaded, while we were
                // being loaded.  Only thing to do is discard the newly loaded
                // data, to avoid breaking an invariant.
                // (No big deal; this situation is rare.)

                Con::printf("  - Force-discarding chunk (%d, %d) @ %d",
                    geomR->mChunk->mPos.x, geomR->mChunk->mPos.y, geomR->mChunk->mLevel);
                SAFE_DELETE(geomR->mGeom);
            }
            else
            {
                // Otherwise, instate it.
                geomR->mGeom->prepare();
                SAFE_DELETE(geomR->mChunk->mGeom);
                geomR->mChunk->mGeom = geomR->mGeom;
            }

            // And clear the retire buffer.
            mLT_GeomRetire[i] = NULL;

            // And free the structure.
            mTLRAlloc.free((ThreadLoadRequest*)geomR);

            PROFILE_END();
        }

        // Skip if nothing there.
        if (texR)
        {
            PROFILE_START(AtlasResource_sync__TexInstate);

            if (texR->mChunk->mParent
                && texR->mChunk->mParent->mTex.isNull())
            {
                PROFILE_START(AtlasResource_sync__TexInstate_Discard);

                // Drat!  Our parent texture was unloaded, while we were
                // being loaded.  Only thing to do is to discard the
                // newly loaded image, to avoid breaking the invariant.
                // (No big deal; this situation is rare.)
                Con::printf("  - Force-discarding tile (%d, %d) @ %d",
                    texR->mChunk->mPos.x, texR->mChunk->mPos.y, texR->mChunk->mLevel);

                SAFE_DELETE(texR->mDDS);

                PROFILE_END();
            }
            else
            {
                if (mTexFreeList.size())
                {
                    PROFILE_START(AtlasResource_sync__TexInstate_Refresh);

                    // If the chunk already has a texture, stick it on the free list.
                    if (texR->mChunk->mTex.isValid())
                        mTexFreeList.push_back(texR->mChunk->mTex);

                    // Get a tex from the free list and instate.
                    texR->mChunk->mTex = mTexFreeList.last();
                    mTexFreeList.pop_back();

                    // A bit of a hack to get it to refresh the texture.
                    texR->mChunk->mTex->mDDS = texR->mDDS;
                    texR->mChunk->mTex.refresh();
                    texR->mChunk->mTex->mDDS = NULL;

                    PROFILE_END();
                }
                else
                {
                    // Create the new texture.
                    texR->mChunk->mTex.set(texR->mDDS, &GFXDefaultStaticDiffuseProfile, false);
                }

                // Blast the DDS...
                SAFE_DELETE(texR->mDDS);

                // Also, do a bit of a hack to get the texture loaded NOW rather
                // than later... This prevents hitching when we pan.
                GFX->setTexture(0, texR->mChunk->mTex);
                PrimBuild::begin(GFXTriangleList, 3);
                PrimBuild::vertex2f(0, 0);
                PrimBuild::vertex2f(0, 1);
                PrimBuild::vertex2f(1, 0);
                PrimBuild::end();
                GFX->setTexture(0, NULL);
            }

            // And clear the retire buffer.
            mLT_TexRetire[i] = NULL;

            // And free the structure.
            mTLRAlloc.free((ThreadLoadRequest*)texR);

            PROFILE_END();
        }
    }

    PROFILE_START(AtlasResource_sync__Queue);

    // And queue up some new stuff. Don't worry about overwriting old pointers.
    for (S32 i = 0; i < MAX_LOADER_THREAD_REQUESTS; i++)
    {
        if (mGeomRequest.size())
        {
            // Get an item from the vector.
            volatile ThreadLoadRequest* p = NULL;

            // Get new ones till we find one that isn't already loaded.
            while (!p && mGeomRequest.size())
            {
                p = mGeomRequest.last();
                mGeomRequest.pop_back();

                // If it has geometry, discard it.
                if (p->mChunk->hasResidentGeometryData())
                {
                    mTLRAlloc.free((ThreadLoadRequest*)p);

                    // And mark it null, so we try again.
                    p = NULL;
                }
            }

            // Ok, Stuff it in.
            mLT_GeomRequest[i] = p;
        }

        if (mTexRequest.size())
        {
            // Get an item from the vector.
            volatile ThreadLoadRequest* p = NULL;

            // Get new ones till we find one that isn't already loaded.
            while (!p && mTexRequest.size())
            {
                p = mTexRequest.last();
                mTexRequest.pop_back();

                // If it already has a texture, discard it.
                if (p->mChunk->hasResidentTextureData())
                {
                    mTLRAlloc.free((ThreadLoadRequest*)p);

                    // And mark it null, so we try again.
                    p = NULL;
                }
            }

            // Ok, Stuff it in.
            mLT_TexRequest[i] = p;
        }
    }

    PROFILE_END();

    // All done!
    releaseLoadMutex();

    bool didWork = false;

    // If we're in synchronous mode, first off make sure the thread is dead, then
    // do our loading here.
    if (AtlasInstance::smSynchronousLoad)
    {
        if (mRunLoaderThread)
            stopLoader();

        // Do our loading.
        PROFILE_START(AtlasResource_sync_Process);
        didWork |= processThreadGeomRequests();
        didWork |= processThreadTexRequests();
        PROFILE_END();
    }
    else
    {
        // Deal with someone turning the flag off - in this case we have to 
        // kick the loader back on.
        if (!mRunLoaderThread)
            startLoader();
    }

    // Also, prune the free list - try to keep enough on hand to service
    // immediate needs.
    while (mTexFreeList.size() > 2 * MAX_LOADER_THREAD_REQUESTS)
    {
        PROFILE_START(AtlasData_texFree);
        mTexFreeList.pop_back();
        PROFILE_END();
    }

    return didWork;
}

void AtlasResource::acquireLoadMutex()
{
    PROFILE_START(AtlasResource_acquireLoadMutex);
    if (mLoadMutex)
        Mutex::lockMutex(mLoadMutex);
    PROFILE_END();
}

void AtlasResource::releaseLoadMutex()
{
    PROFILE_START(AtlasResource_releaseLoadMutex);
    if (mLoadMutex)
        Mutex::unlockMutex(mLoadMutex);
    PROFILE_END();
}


bool AtlasResource::processThreadGeomRequests()
{
    // Only return true if we did something.
    bool didSomething = false;

    acquireLoadMutex();

    //  Find the first item...
    volatile ThreadLoadRequest* r = NULL;
    S32 i;
    for (i = 0; i < MAX_LOADER_THREAD_REQUESTS; i++)
    {
        if (mLT_GeomRequest[i] && !mLT_GeomRequest[i]->mChunk->mGeom)
        {
            r = mLT_GeomRequest[i];
            break;
        }
    }

    // If nothing, exit out.
    if (!r)
    {
        releaseLoadMutex();
        return false;
    }

    // If so, remove it from the list and start processing...
    mLT_GeomRequest[i] = NULL;

    releaseLoadMutex();

    char* reason;
    switch (r->mReason)
    {
    case CanSplitChildPreload:  reason = "CanSplit"; break;
    case WarmUpPreload:         reason = "Warm Up";  break;

    default: reason = "unknown!"; break;
    }

    //Con::printf("  + Loading chunk (%d, %d) @ %d, %s", r->mChunk->mPos.x, r->mChunk->mPos.y, r->mChunk->mLevel, reason);
    r->mGeom = loadChunkGeom(r->mChunk->mDataFilePosition);

    // Now add it to the retire.
    bool gotHit = false;
    while (!gotHit && (mRunLoaderThread || AtlasInstance::smSynchronousLoad))
    {
        acquireLoadMutex();

        for (i = 0; i < MAX_LOADER_THREAD_REQUESTS; i++)
        {
            if (!mLT_GeomRetire[i])
            {
                mLT_GeomRetire[i] = r;
                gotHit = true;
                break;
            }
        }

        releaseLoadMutex();

        if (!gotHit)
            Platform::sleep(1);

    }

    if (!gotHit)
    {
        Con::warnf("AtlasResource::processThreadGeomRequests - had to eat a geom!");
        SAFE_DELETE(r->mGeom); // We have to eat the geometry, joy.
    }

    return true;
}


bool AtlasResource::processThreadTexRequests()
{
    // Only return true if we did something.
    bool didSomething = false;

    acquireLoadMutex();

    //  Find the first item...
    volatile ThreadLoadRequest* r = NULL;
    S32 i;
    for (i = 0; i < MAX_LOADER_THREAD_REQUESTS; i++)
    {
        if (mLT_TexRequest[i] && mLT_TexRequest[i]->mChunk->mTex.isNull())
        {
            r = mLT_TexRequest[i];
            break;
        }
    }

    // If nothing, exit out.
    if (!r)
    {
        releaseLoadMutex();

        return false;
    }

    // If so, remove it from the list and start processing...
    mLT_TexRequest[i] = NULL;

    releaseLoadMutex();

    // Load it!
    char* reason;
    switch (r->mReason)
    {
    case UpdateTextureLoad: reason = "LOD too low"; break;
    default: reason = "unknown"; break;
    }

    //Con::printf("  + loading tile (%d, %d) @ %d, %s",  r->mChunk->mPos.x,  r->mChunk->mPos.y,  r->mChunk->mLevel, reason);
    PROFILE_START(AtlasResource_process_loadDDS);
    r->mDDS = mTQTFile->loadDDS(r->mChunk->mLevel, r->mChunk->mPos);
    PROFILE_END();

    // Now add it to the retire.
    bool gotHit = false;

    while (!gotHit && (mRunLoaderThread || AtlasInstance::smSynchronousLoad))
    {
        acquireLoadMutex();

        for (i = 0; i < MAX_LOADER_THREAD_REQUESTS; i++)
        {
            if (!mLT_TexRetire[i])
            {
                mLT_TexRetire[i] = r;
                gotHit = true;
                break;
            }
        }

        releaseLoadMutex();

        if (!gotHit)
            Platform::sleep(1);
    }

    if (!gotHit)
    {
        Con::warnf("AtlasResource::processThreadGeomRequests - had to eat a tex!");
        SAFE_DELETE(r->mDDS); // We have to eat the texture, joy.
    }

    return true;
}

void AtlasResource::loaderThread()
{
    U32 count = 0;

    while (mRunLoaderThread)
    {
        bool didWork = false;

        didWork |= processThreadGeomRequests();
        didWork |= processThreadTexRequests();

        count++;

        // Sleep unless there was work done.
        if (didWork)
            Platform::sleep(1);
        else
            Platform::sleep(10);
    }

    // It's ok to print here as we're in a known state.
    Con::printf("AtlasResource::loaderThread - %d iterations before death.", count);
}

void AtlasResource::requestGeomLoad(AtlasResourceEntry* ce, AtlasInstance* who, F32 priority, LoadReason reason)
{
    PROFILE_START(AtlasResource_requestGeomLoad);

    // Don't load if the parent isn't loaded... Skip this if AtlasInstance is 
    // NULL, as that indicates we're doing some sort of specialized internal
    // manipulation.
    if (who && ce->mParent && ce->mParent->mGeom == NULL)
    {
        PROFILE_END();
        return;
    }

    // Make sure we're only requesting once.
    for (S32 i = 0; i < mGeomRequest.size(); i++)
        if (mGeomRequest[i]->mChunk == ce)
        {
            // Update our priority...
            ce->requestLoad(AtlasResourceEntry::LoadTypeGeometry, who, priority, reason);
            mGeomRequest[i]->mPriority = ce->getLoadPriority(AtlasResourceEntry::LoadTypeGeometry);

            PROFILE_END();
            return;
        }

    ThreadLoadRequest* tlr = mTLRAlloc.alloc();

    tlr->mPriority = priority;
    tlr->mChunk = ce;
    tlr->mGeom = NULL;
    tlr->mReason = reason;

    mGeomRequest.push_back(tlr);
    ce->mHeat += 1.f;

    PROFILE_END();
}

void AtlasResource::requestTexLoad(AtlasResourceEntry* ce, AtlasInstance* who, F32 priority, LoadReason reason)
{
    PROFILE_START(AtlasResource_requestTexLoad);

    AssertFatal(ce, "AtlasResource::requestTexLoad - NULL chunk!");
    AssertFatal(ce->mTex.isNull(),
        "AtlasResource::requestTexLoad - Already had a texture loaded!");
    AssertFatal(mTQTFile->getTreeDepth() >= ce->mLevel,
        "AtlasResource::requestTexLoad - Too deep CE level.");

    // Don't load if the parent isn't loaded.
    if (ce->mParent && ce->mParent->mTex.isNull())
    {
        PROFILE_END();
        return;
    }

    // Make sure we're only requesting once.
    for (S32 i = 0; i < mTexRequest.size(); i++)
    {
        if (mTexRequest[i]->mChunk == ce)
        {
            // Update our priority...
            ce->requestLoad(AtlasResourceEntry::LoadTypeTexture, who, priority, reason);
            mTexRequest[i]->mPriority = ce->getLoadPriority(AtlasResourceEntry::LoadTypeTexture);
            PROFILE_END();
            return;
        }
    }

    ThreadLoadRequest* tlr = mTLRAlloc.alloc();

    tlr->mPriority = priority;
    tlr->mChunk = ce;
    tlr->mDDS = NULL;
    tlr->mReason = reason;

    mTexRequest.push_back(tlr);

    ce->mHeat += 1.f;

    PROFILE_END();
}


//------------------------------------------------------------------------

AtlasResourceInfo* AtlasResource::loadChunkGeom(U32 idx)
{
    // Ok, load this bad boy.
    mStream->setPosition(idx);
    AtlasResourceInfo* r = new AtlasResourceInfo;
    r->read(mStream, this);
    return r;
}

//------------------------------------------------------------------------

void AtlasResource::loadServerInfo(AtlasInstance* who)
{
    // Load all the leaves so we get our collision info...

    // Iterate over leaves and mark 'em as needed.
    for (S32 i = 0; i < BIT(mTreeDepth - 1); i++)
    {
        for (S32 j = 0; j < BIT(mTreeDepth - 1); j++)
        {
            Point2I pos(i, j);
            AtlasResourceEntry* cre = mChunkTable[QuadTreeTracer::getNodeIndex(mTreeDepth - 1, pos)];
            AssertFatal(!cre->hasChildren(), "AtlasResource::loadServerInfo - got non-leaf node!");

            requestGeomLoad(cre, NULL, 1.f, AtlasResource::ServerPreload);
            cre->incRef(AtlasResourceEntry::LoadTypeGeometry);
        }
    }

    // And spin till they're all loaded. We can't have people falling through the floor.
    precache();

    // Now make sure we have all the info we need.
    for (S32 i = 0; i < BIT(mTreeDepth - 1); i++)
    {
        for (S32 j = 0; j < BIT(mTreeDepth - 1); j++)
        {
            Point2I pos(i, j);
            AtlasResourceEntry* cre = mChunkTable[QuadTreeTracer::getNodeIndex(mTreeDepth - 1, pos)];
            AssertFatal(!cre->hasChildren(), "AtlasResource::loadServerInfo - got non-leaf node!");

            AssertFatal(cre->mGeom, "AtlasResource::loadServerInfo - Failed to load a chunk!");
            AssertFatal(cre->mGeom && cre->mGeom->mColTree,
                "AtlasResource::loadServerInfo - loaded a non-colliding chunk!");
        }
    }

}

void AtlasResource::precache()
{
    mPreloading = true;
    bool saveload = AtlasInstance::smSynchronousLoad;
    AtlasInstance::smSynchronousLoad = true;

    // No matter what, this is probably good.
    stopLoader();

    // Ok, sit and spin!
    bool didWork = true;
    U32 count = 0;

    while (didWork)
    {
        didWork = false;

        for (S32 i = 0; i < getMax(1, MAX_LOADER_THREAD_REQUESTS - 1); i++)
        {
            didWork |= processThreadGeomRequests();
            didWork |= processThreadTexRequests();
        }

        // Make sure we don't skip due to being on same frame...
        mLastSyncFrame = -1;
        didWork |= sync();

        count++;
    }

    mPreloading = false;
    AtlasInstance::smSynchronousLoad = saveload;

    Con::printf("AtlasInstance(%s) - Completed precache in %d cycles!", " - ", count);

    // Is it appropriate to start the thread up again?
    if (!AtlasInstance::smSynchronousLoad)
        startLoader();
}

//------------------------------------------------------------------------

void AtlasResource::incOwnership()
{
    if (mInitialized && mOwnerCount == 0)
        startLoader();

    mOwnerCount++;
}

void AtlasResource::decOwnership()
{
    mOwnerCount--;

    if (mOwnerCount == 0)
    {
        // Time to clean up - purge all our resources and kill the thread.
        stopLoader();
        purge();
    }
}