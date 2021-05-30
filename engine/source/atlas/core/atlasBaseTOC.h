//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _ATLASBASETOC_H_
#define _ATLASBASETOC_H_

#include "math/mMath.h"
#include "util/safeDelete.h"
#include "atlas/core/atlasChunk.h"
#include "atlas/core/atlasFile.h"

class AtlasFile;
class AtlasGeomChunk;
class AtlasTexChunk;

/// Base class for all Atlas stubs.
///
/// @ingroup AtlasCore
class AtlasBaseStub
{
public:
    /// Level in the tree (from top - 0 == largest square).
    U32 level;

    /// Position in that level (range is 0..BIT(level)).
    Point2I pos;
};

/// Base class for all Atlas Table Of Contents classes.
///
/// Atlas works off of the notion of a file, containing multiple TOCs.
///
/// For various type safety reasons, although TOCs are templated, we have
/// to have a base class which we can access w/o any type knowledge for stubs
/// or chunks. This, AtlasTOC provides several useful generic members, as well
/// as some helper functions, and many callbacks which are important to the 
/// threaded loading process.
///
/// Many of these functions are only really relevant for AtlasResourceTOCs, but
/// get carried over to AtlasInstanceTOCs to avoid having yet another interface.
///
/// @ingroup AtlasCore
class AtlasTOC
{
    friend class AtlasFile;

protected:
    /// Depth of our tree.
    U32 mTreeDepth;

    /// Owning file.
    AtlasFile* mFile;

    /// Safety; are we allowing people to use us in a way that implies we're
    /// a quadtree? (This is mostly here so people don't try to diddle a
    /// config TOC the wrong way.)
    bool mIsQuadtree;

public:

    AtlasTOC()
    {
        mIsQuadtree = false;
        mFile = NULL;
        mTreeDepth = -1;
    }

    virtual ~AtlasTOC() {};

    /// Size of a quadtree of depth.
    static inline const U32 getNodeCount(const U32 depth)
    {
        return 0x55555555 & ((1 << depth * 2) - 1);
    }

    /// Index of a node at given position in a quadtree.
    static inline const U32 getNodeIndex(const U32 level, const Point2I pos)
    {
        //AssertFatal(level < mTreeDepth, "QuadTreeTracer::getNodeIndex - out of range level!)
        AssertFatal(pos.x < BIT(level) && pos.x >= 0, "AtlasTOC::getNodeIndex - out of range x for level!");
        AssertFatal(pos.y < BIT(level) && pos.y >= 0, "AtlasTOC::getNodeIndex - out of range y for level!");

        const U32 base = getNodeCount(level);
        return base + (pos.x << level) + pos.y;
    }

    /// Serialize the TOC to a stream. This is mostly useful for
    /// ResourceTOCs.
    virtual bool write(Stream* s) { return false; }

    /// Serialize the TOC from a stream. This is mostly useful for
    /// ResourceTOCs.
    virtual bool read(Stream* s) { return false; }

    /// (Re)build all the tiles that contain the specified region, which is
    /// specified in leaf level tiles.
    virtual void generate(RectI invalidationRegion) {}

    virtual void cbOnChunkWriteComplete(dsize_t key, AtlasDeferredIO* adio) {};
    virtual void cbOnStubWriteComplete(dsize_t key, AtlasDeferredIO* adio) {};
    virtual void cbPostBackToAtlasFile(dsize_t key, AtlasDeferredIO* adio) {};

    virtual void cbOnChunkReadComplete(dsize_t stubIdx, AtlasChunk* chunk) {};
    virtual void cbOnChunkReadStarted(dsize_t stubIdx) {};
    virtual void cbPopulateChunkReadNote(AtlasReadNote* arn) {};

    /// Used to dump the load request status of the specified stub.
    virtual void dumpStubLoadStatus(U32 stubIdx) {};

    /// This gets called back from AtlasFile::syncThreads, with time in seconds
    /// since last call. It's useful for updating things like heat.
    virtual void syncCallback(const F32 deltaTime) {};

    /// Construct a new chunk that is of the appropriate type for this TOC,
    /// and set the owning TOC to ourselves.
    virtual AtlasChunk* constructNewChunk() { return NULL; };

    /// Take our internal priority list, and fill the provided list with a 
    /// pri-sorted set of elements describing what are our highest priorities
    /// at the moment.
    virtual void recalculateUpdates(Vector<AtlasReadNote*>& list, U32 maxCount) {};

    /// For tracking purposes we allow most load/unload methods to specify a
    /// reason. This also lets us filter and prioritize based on context, if
    /// needed.
    enum LoadReasons
    {
        RootLoad = 1, ///< Issued by a new object needing to kick off loading.
        LowPriority,
        NormalPriority,
        WarmUp,
        AttemptSplitChildren,
        EditorLoad, ///< This is a load for the purposes of some editor task.
        ConfigTOCLoad,
        BlenderLoad,
        InstanceShutdown, ///< Unload only, used in the AtlasInstanceTOC destructor.
        CollisionPreload, ///< Loading required data for leaf collision.
    };

    inline const U32 getTreeDepth() const
    {
        return mTreeDepth;
    }

    inline AtlasFile* getAtlasFile() const
    {
        return mFile;
    }
};

/// Templated base class for TOCs.
///
/// This provides all the stub/chunk type specific behavior for an Atlas
/// TOC.
///
/// @ingroup AtlasCore
template<class StubType>
class AtlasBaseTOC : public AtlasTOC
{
public:
    /// Bring the chunk type in as a local typedef to save indirection.
    typedef typename StubType::ChunkType ChunkType;

protected:

    /// Helper function to allocate the right number of stubs for a quadtree
    /// of the specified depth, as well as construct them, set their positions
    /// and so forth.
    void helpInitializeTOC(U32 depth)
    {
        AssertFatal(mIsQuadtree, "AtlasBaseToC<StubType>::helpInitializeTOC - treating non-quadtree TOC as a quadtree!");
        mTreeDepth = depth;
        mStubCount = getNodeCount(depth);
        mStubs = new StubType[mStubCount];

        // Populate level/pos information.
        for (S32 i = 0; i < mTreeDepth; i++)
        {
            for (S32 x = 0; x < BIT(i); x++)
            {
                for (S32 y = 0; y < BIT(i); y++)
                {
                    StubType* s = getStub(i, Point2I(x, y));
                    s->level = i;
                    s->pos.set(x, y);
                }
            }
        }
    }

    StubType* mStubs;
    U32 mStubCount;

public:

    AtlasBaseTOC()
    {
        mFile = NULL;

        mStubs = NULL;
        mStubCount = 0;
    }

    ~AtlasBaseTOC()
    {
        SAFE_DELETE_ARRAY(mStubs);
    }

    /// Get the stub for a given position in the quadtree.
    inline StubType* getStub(const U32 level, const Point2I pos)
    {
        AssertFatal(mIsQuadtree, "AtlasBaseTOC<StubType>::helpInitializeTOC - treating non-quadtree TOC as a quadtree!");

        const U32 nodeID = getNodeIndex(level, pos);
        AssertFatal(nodeID < mStubCount, "AtlasBaseTOC::getStub - index out of bounds!");
        return mStubs + nodeID;
    }

    inline U32 getStubCount()
    {
        return mStubCount;
    }

    inline StubType* getStub(U32 idx)
    {
        return mStubs + idx;
    }
};


#endif