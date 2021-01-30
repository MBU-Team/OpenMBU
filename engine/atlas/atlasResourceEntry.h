//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
// Some portions of Atlas code derived from Thatcher Ulrich's original
// ChunkLOD implementation.
//-----------------------------------------------------------------------------

#ifndef _ATLASRESOURCEENTRY_H_
#define _ATLASRESOURCEENTRY_H_

#include "gfx/gfxTextureHandle.h"
#include "atlas/atlasResource.h"

class AtlasResource;
class AtlasInstance;
class AtlasResourceInfo;

/// Represents the header info for a given chunk in an Atlas terrain.
class AtlasResourceEntry
{
public:
    enum LoadType
    {
        LoadTypeGeometry,
        LoadTypeTexture,
        LoadTypeCount,
    };

private:
    /// Simply so that if we need to change how we track file offsets we can
    /// do so easily.
    typedef long FileOffset;

    /// Actually perform the unload for this entry's paged data.
    void doUnload(LoadType type);

    /// Interface for allocating and tracking load requests.
    struct LoadRequest
    {
        LoadType       mType;
        AtlasResource::LoadReason     mReason;
        AtlasInstance* mRequester;
        F32            mPriority;
        LoadRequest* mNext;
    } *mRequestListHead;

    static FreeListChunker<LoadRequest> smLoadRequestChunker;

public:

    static LoadRequest* allocRequest();
    static void freeRequest(LoadRequest* lr);

public:

    /// How many things are referencing our resources? If this is >0, then we're
    /// either loaded or trying to load. We have one per resource type.
    U32 mRefCount[LoadTypeCount];

    /// Debugging aid, we add heat when we load/unload, and it "cools off" 
    /// over time.
    F32 mHeat;

    AtlasResourceEntry()
    {
        for (S32 i = 0; i < LoadTypeCount; i++)
            mRefCount[i] = 0;

        mHeat = 0;
        mGeom = NULL;
        mChildren[0];
        mRequestListHead = NULL;
    }

    ~AtlasResourceEntry()
    {
        // Put all our LoadRequests back in the pool.
        LoadRequest* walk = mRequestListHead;
        while (walk)
        {
            LoadRequest* t = walk;
            walk = walk->mNext;
            freeRequest(t);
        }

        // Anything else...
        mTex = NULL;
        SAFE_DELETE(mGeom);
    }

    AtlasResourceEntry* mChildren[4];
    AtlasResourceEntry* mParent;

    union
    {
        S32                 mLabel;
        AtlasResourceEntry* mEntry;
    } mNeighbor[4];

    /// Base LOD value - used in setup of real LOD values.
    U16 mLod;

    /// Our label.
    S32 mLabel;

    /// Fast z-bound info.
    S16 mMin, mMax;
    Box3F   mBounds;  ///< Precalculated bounding box.
    Point2I mPos;     ///< Where are we (in our level) on the quadtree?
    U8      mLevel;   ///< What level in the quadtree are we?  4 ** 256 = 1.34078079 × 10^154 max nodes in the tree.

    /// Offset of our pageable data in the chunk file.
    FileOffset mDataFilePosition;

    /// Geometry/collision info.
    AtlasResourceInfo* mGeom;

    /// Texture handle.
    GFXTexHandle mTex;

    /// Helper function to read the entry data from a file.
    void read(Stream* s, U32 recursion_count);

    /// Resolve pointers to neighbors, important for maintaining LOD cohesion.
    void lookupNeighbors();

    const bool  hasResidentGeometryData() const { return mGeom != NULL; }
    const bool  hasResidentTextureData()  const { return mTex.isValid(); }
    const bool  hasResidentAnyData()      const
    {
        return hasResidentGeometryData() || hasResidentTextureData();
    }

    const bool  hasChildren() const { return mChildren[0] != NULL; }
    const U32   getLevel() const { return mLevel; }

    /// Indicate need for this to load its data.
    void requestLoad(LoadType type, AtlasInstance* who, F32 priority, AtlasResource::LoadReason why);

    /// Indicate that a given client instance is done using this resource.
    void decRef(LoadType type, AtlasInstance* who);

    /// Indicate that a client instance is going to start using this resource.
    ///
    /// This does not cause the entry to load anything, but does prevent it from
    /// unloading until all references are cleared up.
    void incRef(LoadType type);

    /// Blast our data, which forces everything to get paged back in.
    void purge();

    /// Get the load priority for the specified type of resource. We basically
    /// take the max of all specified requests.
    F32 getLoadPriority(LoadType type);
};

#endif