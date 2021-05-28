//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _ATLAS_RUNTIME_ATLASCLIPMAPBLENDERCACHE_H_
#define _ATLAS_RUNTIME_ATLASCLIPMAPBLENDERCACHE_H_

#include "atlas/runtime/atlasInstanceTexTOC.h"
#include "atlas/runtime/atlasClipMap.h"
#include "atlas/runtime/atlasClipMapCache.h"
#include "atlas/core/atlasTOCAggregator.h"

/// A clipmap cache that blends data on the fly from opacity and shadow maps
/// provided by the Atlas file.
class AtlasClipMapImageCache_Blender : public IAtlasClipMapImageCache
{
    struct ColumnNote
    {
        S32 chunkColStart, chunkColEnd, colStart, colEnd;
        AtlasTexChunk* atc;
    };

    /// Helper function to initialize geometry for a blend pass.
    void setupGeometry(
        const RectF& srcRect0,
        const RectF& srcRect1,
        const RectF& masterCoords,
        const RectI& dstRect);

    /// Opacity map.
    AtlasInstanceTexTOC* mOpacityTOC;

    /// Shadow map.
    AtlasInstanceTexTOC* mShadowTOC;

    /// This simplifies requests that should apply to ALL TOCs. The previous
    /// two references are there basically there just so we can manage memory
    /// and get to them when needed (if needed).
    AtlasTOCAggregator<AtlasInstanceTexTOC> mAggregateTOCs;

    /// How many levels off the top of the TOC should we skip in order
    /// to match the size of the clipmap?
    U32 mLevelOffset;

    /// How big is a texture chunk? We assume all textures from the TOC are the
    /// same size.
    U32 mShadowChunkSize, mOpacityChunkSize;

    U32 mClipMapSize, mClipMapDepth;

    /// How many chunks in each direction should we cache?
    U32 mCacheRadius;

    /// Where is our cache centered right now?
    Point2I mCacheOrigin;

    /// Given a texel region, convert to the TOC chunks that contain the
    /// specified data.
    inline void convertToTOCRect(const U32 mipLevel, const RectI& region, RectI& outStubs)
    {
        const F32 chunkSize = getMin(mShadowChunkSize, mOpacityChunkSize);
        outStubs.point.x = mFloor(F32(region.point.x) / chunkSize);
        outStubs.point.y = mFloor(F32(region.point.y) / chunkSize);

        outStubs.extent.x = mCeil(F32(region.point.x + region.extent.x) / chunkSize) - outStubs.point.x;
        outStubs.extent.y = mCeil(F32(region.point.y + region.extent.y) / chunkSize) - outStubs.point.y;
    }

    inline void convertToTOCRectWithChunkSize(const S32 chunkSizeS32, const RectI& region, RectI& outStubs)
    {
        const F32 chunkSize = chunkSizeS32;
        outStubs.point.x = mFloor(F32(region.point.x) / chunkSize);
        outStubs.point.y = mFloor(F32(region.point.y) / chunkSize);

        outStubs.extent.x = mCeil(F32(region.point.x + region.extent.x) / chunkSize) - outStubs.point.x;
        outStubs.extent.y = mCeil(F32(region.point.y + region.extent.y) / chunkSize) - outStubs.point.y;
    }

    Vector<GFXTexHandle*> mSourceImages;

    /// Used for streaming video data to card for blend operations.
    GFXTexHandle mOpacityScratchTexture;
    GFXTexHandle mShadowScratchTexture;

    /// Copy some bits from a TOC into a specified region of memory.
    void blitFromTOC(AtlasInstanceTexTOC* toc, U32 level, RectI region, U8* outPtr, U32 outStride);

    /// SM2.0 shader used for one pass blending.
    GFXShader* mOnePass;

    /// SM1.0 shaders used for two pass blending.
    GFXShader* mTwoPass[2];

public:

    AtlasClipMapImageCache_Blender();
    ~AtlasClipMapImageCache_Blender();

    /// Note the resource TOC we want to request texture data from. We also
    /// pass the (uninitialized) clipmap so that we can set the stack height
    /// appropriately.
    void setTOC(AtlasResourceTexTOC* opacityToc, AtlasResourceTexTOC* shadowToc, AtlasClipMap* acm);

    virtual void initialize(U32 clipMapSize, U32 clipMapDepth);
    virtual void setInterestCenter(Point2I origin);
    virtual bool isDataAvailable(U32 mipLevel, RectI region);

    virtual void beginRectUpdates(AtlasClipMap::ClipStackEntry& cse);
    virtual void doRectUpdate(U32 mipLevel, AtlasClipMap::ClipStackEntry& cse, RectI srcRegion, RectI dstRegion);
    virtual void finishRectUpdates(AtlasClipMap::ClipStackEntry& cse);

    virtual bool isRenderToTargetCache()
    {
        return true;
    }

    void registerSourceImage(const char* filename);
};

#endif