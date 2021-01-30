//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
// Some portions of Atlas code derived from Thatcher Ulrich's original
// ChunkLOD implementation.
//-----------------------------------------------------------------------------

#ifndef _ATLASGEOMFILE_H_
#define _ATLASGEOMFILE_H_

#include "platform/platformThread.h"
#include "platform/platformMutex.h"
#include "sim/sceneObject.h"
#include "core/stream.h"
#include "core/dataChunker.h"
#include "gfx/gfxDevice.h"
#include "gfx/gfxStructs.h"
#include "materials/matInstance.h"
#include "atlas/atlasTQTFile.h"
#include "atlas/atlasResource.h"
#include "atlas/atlasResourceInfo.h"
#include "core/resManager.h"
#include "util/quadTreeTracer.h"


#define MAX_LOADER_THREAD_REQUESTS 3

class AtlasResourceTracer;
class AtlasResourceEntry;
class AtlasInstance;
class AtlasInstanceEntry;
extern ResourceInstance* constructAtlasChunkFile(Stream& stream);

/// AtlasResource is used as a shared repository for a given Atlas
/// geometry and texture set. It stores all the common information -
/// quadtree connectivity, bounds, geometry, etc - needed for all the instances
/// of that dataset.
///
/// The hairy bit here is paged resources. The goal here is to keep the union
/// of all paged resources in memory. This is accomplished by a cached
/// reference tracking system. Whenever an instance needs something paged in,
/// it requests that from the AtlasResource. The request is added to a linked
/// list for each chunk (if it is not already loaded). Prioritization is done
/// by taking the maximum of the recorded averages. We assume that we won't have
/// two instances of the same Atlas overlapping in space - this is frankly
/// silly and is not representative of the usual case.
///
/// We remap the request/delete semantics to map to reference count
/// manipulations.
class AtlasResource : public ResourceInstance
{
    typedef ResourceInstance Parent;

public:
    typedef long FileOffset;

    enum DeleteReason
    {
        BadDeleteReason,

        DeleteUpdateLod,
        DeleteWarmupUnimportant,
        DeleteWarmupImportant,

        DeleteTextureNoGeom,
        DeleteTextureLevelTooHigh,

        PurgeReason
    };

    enum LoadReason
    {
        BadLoadReason,

        CanSplitChildPreload,
        WarmUpPreload,

        ServerPreload,
        UpdateTextureLoad,
    };

    /// @name AtlasData_Statics Statics
    /// Rather than pass a reference to ourselves into every Entry method, we
    /// simply store it in a global for easy access. We also do this for some
    /// other key data.
    ///
    /// @see getCurrentChunkData()
    ///
    /// @{

    ///
    static AtlasResource* smCurChunkResource;
    static Point3F          smCurCamPos;
    static Point3F          smCurObjCamPos;
    static SceneGraphData* smCurSGD;
    static MatrixF          smInvObjTrans;

    static Point2I          smBoundChunkPos;
    static U32              smBoundChunkLevel;

    /// @}

    // Chunk allocation...
    AtlasResourceEntry** mChunkTable;
    AtlasResourceEntry* allocChunk(U32 lod, AtlasResourceEntry* parent);
    AtlasResourceEntry* mChunks;
    U32 mEntriesAllocated;
    U32 mEntryCount;

    /// The resource manager doesn't let us keep the stream so we have to 
    /// get our filename passed to us a bit later on.  Yay!
    bool mInitialized;

    // Tree and scale info...
    U32 mTreeDepth;
    F32 mVerticalScale;
    F32 mBaseChunkSize;

    // Default LOD info...
    F32 mError_LODMax;
    F32 mDistance_LODMax;
    F32 mTextureDistance_LODMax;

    // Collision properties.
    U32 mColGridSize;
    U32 mColTreeDepth;

    // File handles.
    Stream* mStream;
    AtlasTQTFile* mTQTFile;

    /// Load actual geometry info for a chunk.
    AtlasResourceInfo* loadChunkGeom(U32 idx);

    Thread* mLoader;
    volatile bool mRunLoaderThread;
    void* mLoadMutex;

    /// Turns off some internal integrity checks so that we can force-load
    /// server-side info and the like.
    bool     mPreloading;

    void acquireLoadMutex();
    void releaseLoadMutex();

    struct ThreadLoadRequest
    {
        LoadReason              mReason;
        F32                     mPriority;
        AtlasResourceEntry* mChunk;

        union {
            AtlasResourceInfo* mGeom;
            DDSFile* mDDS;
        };
    };

    struct DeleteRequest
    {
        DeleteRequest()
        {
            mReason = BadDeleteReason;
            mItem = NULL;
        }

        DeleteRequest(AtlasResourceEntry* item, AtlasInstance* who, DeleteReason reason)
            : mReason(reason), mItem(item), mInstance(who)
        {
        }

        DeleteReason         mReason;
        AtlasResourceEntry* mItem;
        AtlasInstance* mInstance;
    };

    FreeListChunker<ThreadLoadRequest> mTLRAlloc;

    /// We keep track of the last frame we sync'ed on to avoid multiple syncs per
    /// frame.
    U32 mLastSyncFrame;

    // The general list of requested geometry data.
    Vector<ThreadLoadRequest*> mGeomRequest;
    Vector<ThreadLoadRequest*> mTexRequest;

    /// @name Thread Queues
    ///
    /// These buffers hold things for the thread to immediately process. Request
    /// feeds the thread, and retire returns loaded resources.
    ///
    /// @{

    ///
    volatile ThreadLoadRequest* mLT_GeomRequest[MAX_LOADER_THREAD_REQUESTS];
    volatile ThreadLoadRequest* mLT_GeomRetire[MAX_LOADER_THREAD_REQUESTS];
    volatile ThreadLoadRequest* mLT_TexRequest[MAX_LOADER_THREAD_REQUESTS];
    volatile ThreadLoadRequest* mLT_TexRetire[MAX_LOADER_THREAD_REQUESTS];

    /// @}

    void startLoader();
    bool processThreadGeomRequests();
    bool processThreadTexRequests();
    void loaderThread();
    void stopLoader();

    Vector<GFXTexHandle> mTexFreeList;

    AtlasResourceTracer* mTracer;

    /// Set this AtlasResource to use the specified chunk and TQT files.
    bool load(Stream& s);
    bool initialize(const char* chuFile, const char* tqtFile = NULL);

    /// Forces all the info to become resident in memory that the server
    /// will need for serving collision and the like.
    void loadServerInfo(AtlasInstance* who);

    AtlasResource();
    ~AtlasResource();

    /// Get a pointer to the currently active ChunkData. Mostly used internally.
    static AtlasResource* getCurrentResource()
    {
        return smCurChunkResource;
    }

    /// set a pointer to the currently active ChunkData. Mostly used internally.
    static void setCurrentResource(AtlasResource* data)
    {
        smCurChunkResource = data;
    }

    /// @group requests Thread Interface
    ///
    /// Since load/unload is multithreaded, these methods allow us to mark
    /// what we need loaded or unloaded. All the details of talking to the thread
    /// are hidden from us.
    ///
    /// @{

    ///
    void requestGeomLoad(AtlasResourceEntry* ce,
        AtlasInstance* who, F32 urgency, LoadReason reason);
    void requestTexLoad(AtlasResourceEntry* ce,
        AtlasInstance* who, F32 urgency, LoadReason reason);

    /// Called to fetch data from the background loader, as well as feed it new
    /// requests. When in synchronous loading (see AtlasInstance::smSynchronousLoad),
    /// this is also where loading occurs.
    bool sync();

    /// @}


    /// Cast a ray. (Done in AtlasResource space).
    bool castRay(const Point3F& start, const Point3F& end, RayInfo* info, bool emptyCollide);

    /// This is a unified internal collision interface for the terrain.
    ///
    /// We end up recursing by bounding box down into the chunks, it makes more
    /// sense to have one set of functions that take different terminal actions
    /// rather than having to live with two sets of code. Atlas::buildConvex
    /// and Atlas::buildPolyList methods simply wrap this, and it fills in
    /// what is appropriate. If you were superclever you'd go and do your poly
    /// and convex queries simultaneously.
    ///
    /// localMat is used to transform everything from objectspace to worldspace.
    bool buildCollisionInfo(const Box3F& box, Convex* c, AbstractPolyList* poly);

    /// Forcibly unload any loaded resources.
    void purge();

    /// Sit and spin until everything is loaded. This version just clears out
    /// the load queues.
    void precache();

    /// Return a pointer to a given entry in the tree.
    AtlasResourceEntry* getResourceEntry(U32 level, Point2I pos)
    {
        return mChunkTable[QuadTreeTracer::getNodeIndex(level, pos)];
    }

    S32 mOwnerCount;

    void incOwnership();
    void decOwnership();
};

#endif