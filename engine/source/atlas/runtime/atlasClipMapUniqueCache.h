//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _ATLAS_RUNTIME_ATLASCLIPMAPUNIQUECACHE_H_
#define _ATLAS_RUNTIME_ATLASCLIPMAPUNIQUECACHE_H_

#include "atlas/runtime/atlasInstanceTexTOC.h"
#include "atlas/runtime/atlasClipMap.h"
#include "atlas/runtime/atlasClipMapCache.h"

/// A clipmap cache that fetches its data from an AtlasResourceTexTOC.
class AtlasClipMapImageCache_TextureTOC : public IAtlasClipMapImageCache
{
    /// The texture TOC we're going to be grabbing data from.
    AtlasInstanceTexTOC* mAitToc;

    /// How many levels off the top of the TOC should we skip in order
    /// to match the size of the clipmap?
    U32 mLevelOffset;

    /// How big is a texture chunk? We assume all textures from the TOC are the
    /// same size.
    U32 mTOCTextureSize;

    U32 mClipMapSize, mClipMapDepth;

    /// How many chunks in each direction should we cache?
    U32 mCacheRadius;

    /// Where is our cache centered right now?
    Point2I mCacheOrigin;

    /// Given a texel region, convert to the TOC chunks that contain the
    /// specified data.
    inline void convertToTOCRect(const U32 mipLevel, const RectI& region, RectI& outStubs)
    {
        const F32 chunkSize = mTOCTextureSize;
        outStubs.point.x = mFloor(F32(region.point.x) / chunkSize);
        outStubs.point.y = mFloor(F32(region.point.y) / chunkSize);

        outStubs.extent.x = mCeil(F32(region.point.x + region.extent.x) / chunkSize) - outStubs.point.x;
        outStubs.extent.y = mCeil(F32(region.point.y + region.extent.y) / chunkSize) - outStubs.point.y;
    }

    void bitblt(U32 mipLevel, const AtlasClipMap::ClipStackEntry& cse,
        RectI srcRegion, U8* bits, U32 pitch);

public:
    AtlasClipMapImageCache_TextureTOC();
    ~AtlasClipMapImageCache_TextureTOC();

    /// Note the resource TOC we want to request texture data from. We also
    /// pass the (uninitialized) clipmap so that we can set the stack height
    /// appropriately.
    void setTOC(AtlasResourceTexTOC* arttoc, AtlasClipMap* clipMap);

    virtual void initialize(U32 clipMapSize, U32 clipMapDepth);
    virtual void setInterestCenter(Point2I origin);
    virtual bool isDataAvailable(U32 mipLevel, RectI region);

    virtual void beginRectUpdates(AtlasClipMap::ClipStackEntry& cse);
    virtual void doRectUpdate(U32 mipLevel, AtlasClipMap::ClipStackEntry& cse, RectI srcRegion, RectI dstRegion);
    virtual void finishRectUpdates(AtlasClipMap::ClipStackEntry& cse);

    virtual bool isRenderToTargetCache()
    {
        return false;
    }
};

#endif