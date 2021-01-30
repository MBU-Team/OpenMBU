//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------


#include "terrain/terrData.h"
#include "math/mMath.h"
#include "console/console.h"
#include "console/consoleTypes.h"
#include "gfx/gBitmap.h"
#include "terrain/terrRender.h"
#include "sceneGraph/sceneState.h"
#include "terrain/waterBlock.h"
#include "terrain/blender.h"
#include "core/frameAllocator.h"
#include "sceneGraph/sceneGraph.h"
#include "sceneGraph/sgUtil.h"
#include "platform/profiler.h"
#include "gfx/gfxDevice.h"
#include "materials/matInstance.h"
#include "terrain/sky.h"
#include "terrain/terrTexture.h"

#ifdef TERR_TEXTURE_DEBUG
U32 AllocatedTexture::mAllocTexCount = 0;
#endif

S32 TerrTexture::mTextureMinSquareSize;

bool TerrTexture::mEnableTerrainDetails = true;

AllocatedTexture* TerrTexture::mTextureFrameWalk;
AllocatedTexture TerrTexture::mTextureFrameListHead;
AllocatedTexture TerrTexture::mTextureFrameListTail;
AllocatedTexture* TerrTexture::mCurrentTexture = NULL;

AllocatedTexture TerrTexture::mTextureFreeListHead;
AllocatedTexture TerrTexture::mTextureFreeListTail;

AllocatedTexture* AllocatedTexture::mTextureGrid[AllocatedTextureCount];
AllocatedTexture** AllocatedTexture::mTextureGridPtr[5];

Vector<GFXTexHandle> TerrTexture::mTextureFreeList(__FILE__, __LINE__);
bool sgTextureFreeListPrimed = false;
U32  sgFreeListPrimeCount = 1;

U32 TerrTexture::mTextureSlopSize = 100;

S32 TerrTexture::mDynamicTextureCount;
S32 TerrTexture::mTextureSpaceUsed;
S32 TerrTexture::mLevelZeroCount;
S32 TerrTexture::mFullMipCount;
S32 TerrTexture::mStaticTextureCount;
S32 TerrTexture::mUnusedTextureCount;
S32 TerrTexture::mStaticTSU;
U32 TerrTexture::mNewGenTextureCount;

//---------------------------------------------------------------
// Texture allocation / free list management
//---------------------------------------------------------------

void AllocatedTexture::init()
{
    S32 i;
    for (i = 0; i < AllocatedTextureCount; i++)
        mTextureGrid[i] = NULL;

    mTextureGridPtr[0] = mTextureGrid;
    mTextureGridPtr[1] = mTextureGrid + 4096;
    mTextureGridPtr[2] = mTextureGrid + 4096 + 1024;
    mTextureGridPtr[3] = mTextureGrid + 4096 + 1024 + 256;
    mTextureGridPtr[4] = mTextureGrid + 4096 + 1024 + 256 + 64;
}

void AllocatedTexture::flushCache()
{
    // Free our unused texture handles
    AllocatedTexture* tex;
    while ((tex = TerrTexture::mTextureFreeListHead.next) != &TerrTexture::mTextureFreeListTail)
    {
        tex->unlink();
        freeTex(tex);
    }

    // Clear the grid...
    for (S32 i = 0; i < AllocatedTextureCount; i++)
        mTextureGrid[i] = 0;

    for (S32 i = 0; i < TerrTexture::mTextureFreeList.size(); i++)
        TerrTexture::mTextureFreeList[i] = NULL;
    TerrTexture::mTextureFreeList.clear();

}

void AllocatedTexture::flushCacheRect(RectI bRect)
{
    bRect.inset(-1, -1);
    for (S32 i = 0; i < AllocatedTextureCount; i++)
    {
        // See what's here...
        AllocatedTexture* tex = mTextureGrid[i];
        if (!tex)
            continue;

        // Got something, so see if it intersects with the invalidated area...
        RectI texRect(tex->x, tex->y, 1 << tex->level, 1 << tex->level);
        if (texRect.intersect(bRect))
        {
            // It did, so let's free it...
            tex->safeUnlink();
            freeTex(tex);
        }
    }
}

AllocatedTexture* AllocatedTexture::alloc(Point2I pos, U32 level)
{
    PROFILE_START(AllocatedTexture_alloc);

    // Which block are we currently in?
    S32 blockX = TerrainRender::mBlockOffset.x + TerrainRender::mTerrainOffset.x;
    S32 blockY = TerrainRender::mBlockOffset.y + TerrainRender::mTerrainOffset.y;

    // Which square are we talking about?
    S32 x = pos.x + blockX;
    S32 y = pos.y + blockY;

    // Mask to the block...
    S32 px = (x & TerrainBlock::BlockMask) >> level;
    S32 py = (y & TerrainBlock::BlockMask) >> level;

    // We only deal with levels from 2 to 6.
    AssertFatal(level >= 2 && level <= 6, "Invalid level");

    // Fetch the specified entry from the texture grid
    AllocatedTexture* cur = getGrid(level, px, py);

    if (cur)
    {
        // Got something...

        // Iterate through it and see if we can find a match...
        while (cur && (cur->x != x || cur->y != y))
            cur = cur->nextLink;
    }

    // Ok, we thought we had something but didn't, or didn't have something.
    // So we need to allocate and stuff it in the grid...
    if (!cur)
    {
        // Create it...
        cur = new AllocatedTexture(level, x, y);

        // Store it in the grid
        cur->nextLink = getGrid(level, px, py);
        setGrid(level, px, py, cur);
    }

    // Now we have something, so set it up for us...
    cur->safeUnlink();
    cur->linkAfter(&TerrTexture::mTextureFrameListHead);
    TerrTexture::mCurrentTexture = cur;

    // Make sure to allocate a texture NOW, not later...
    if (cur->handle.isNull())
        TerrTexture::buildBlendMap(cur);

    PROFILE_END();

    // All done!
    return cur;
}

void AllocatedTexture::trimFreeList()
{
    PROFILE_START(AllocatedTexture_trimFreeList);

    // Get rid of everything after the first N entries in the free list.

    // Skip the first N...
    U32 slop = 0;
    AllocatedTexture* fwalk = TerrTexture::mTextureFreeListHead.next;
    while (fwalk != &TerrTexture::mTextureFreeListTail && slop < TerrTexture::mTextureSlopSize)
    {
        fwalk = fwalk->next;
        slop++;
    }

    // Delete everything after the first N...
    while (fwalk != &TerrTexture::mTextureFreeListTail)
    {
        AllocatedTexture* next = fwalk->next;

        // Remove it from the list
        fwalk->unlink();

        // Delete it...
        freeTex(fwalk);

        // And move on to the next one
        fwalk = next;
    }

    PROFILE_END();
}

void AllocatedTexture::freeTex(AllocatedTexture* tex)
{
    PROFILE_START(AllocatedTexture_freeTex);

    // Return the texture handle to the free list...
    if (tex->handle)
    {
        TerrTexture::mTextureFreeList.increment();
        constructInPlace(&TerrTexture::mTextureFreeList.last());
        TerrTexture::mTextureFreeList.last() = tex->handle;
    }
    tex->handle = NULL;

    // Mask to the block...
    S32 px = (tex->x & TerrainBlock::BlockMask) >> tex->level;
    S32 py = (tex->y & TerrainBlock::BlockMask) >> tex->level;

    // We only deal with levels from 2 to 6.
    AssertFatal(tex->level >= 2 && tex->level <= 6, "Invalid level");

    AllocatedTexture* walk = getGrid(tex->level, px, py);

    bool didDelete = false;
    if (walk == tex)
    {
        setGrid(tex->level, px, py, tex->nextLink);
        didDelete = true;
    }
    else
        while (walk)
        {
            if (walk->nextLink == tex)
            {
                // If we find ourselves, patch stuff up and bail.
                walk->nextLink = tex->nextLink;
                didDelete = true;
                break;
            }

            walk = walk->nextLink;
        }

    if (!didDelete) Con::errorf("Didn't find ourselves in the cache!");

    // And now deallocate the memory...
    delete tex;

    PROFILE_END();
}

//---------------------------------------------------------------

void TerrTexture::init()
{
    mTextureMinSquareSize = 0;

    mTextureFrameListHead.next = &mTextureFrameListTail;
    mTextureFrameListHead.previous = NULL;
    mTextureFrameListTail.next = NULL;
    mTextureFrameListTail.previous = &mTextureFrameListHead;

    mTextureFreeListHead.next = &mTextureFreeListTail;
    mTextureFreeListHead.previous = NULL;
    mTextureFreeListTail.next = NULL;
    mTextureFreeListTail.previous = &mTextureFreeListHead;

    AllocatedTexture::init();

    Con::addVariable("TerrTexture::Texture::dynamicTextureCount", TypeS32, &mDynamicTextureCount);
    Con::addVariable("TerrTexture::Texture::textureSpaceUsed", TypeS32, &mTextureSpaceUsed);
    Con::addVariable("TerrTexture::Texture::unusedTextureCount", TypeS32, &mUnusedTextureCount);
    Con::addVariable("TerrTexture::Texture::staticTextureCount", TypeS32, &mStaticTextureCount);
    Con::addVariable("TerrTexture::Texture::levelZeroCount", TypeS32, &mLevelZeroCount);
    Con::addVariable("TerrTexture::Texture::fullMipCount", TypeS32, &mFullMipCount);

    Con::addVariable("pref::Terrain::texDetail", TypeS32, &mTextureMinSquareSize);
    Con::addVariable("pref::Terrain::enableDetails", TypeBool, &mEnableTerrainDetails);
    Con::addVariable("pref::Terrain::textureCacheSize", TypeS32, &mTextureSlopSize);
}

void TerrTexture::shutdown()
{
    AllocatedTexture::flushCache();
}

void TerrTexture::startFrame()
{
    mNewGenTextureCount = 0;
    mDynamicTextureCount = 0;
    mTextureSpaceUsed = 0;
    mUnusedTextureCount = 0;
    mStaticTextureCount = 0;
    mLevelZeroCount = 0;
    mFullMipCount = 0;
    mStaticTSU = 0;
}

AllocatedTexture* TerrTexture::getNextFrameTexture()
{
    PROFILE_START(TerrTexture_getNextFrameTex);

    // Set up our walk variable...
    if (!mTextureFrameWalk)
    {
        // First call this frame, so let's set things up...
        mTextureFrameWalk = mTextureFreeListHead.next;
    }
    else
    {
        // Subsequent calls mean we have to clean up previous stuff
        mTextureFrameWalk->emitList = NULL;
    }

    // Move to the next item
    mTextureFrameWalk = mTextureFrameListHead.next;

    // Are we done?
    if (mTextureFrameWalk == &mTextureFrameListTail)
    {
        mTextureFrameWalk = NULL;
        PROFILE_END();
        return NULL;
    }

    // Ok, we have something... put it in the free list
    // and see about returning it...
    mTextureFrameWalk->unlink();
    mTextureFrameWalk->linkAfter(&mTextureFreeListHead);

    // Statistics
    mStaticTextureCount++;

    // set up texture generation parameters, one texture rep for the square...
    F32 invLevel = 1 / F32(TerrainRender::mSquareSize << mTextureFrameWalk->level);
    TerrBatch::setTexGen(
        Point4F(invLevel, 0, 0, -(mTextureFrameWalk->x >> mTextureFrameWalk->level)),
        Point4F(0, invLevel, 0, -(mTextureFrameWalk->y >> mTextureFrameWalk->level))
    );

    PROFILE_END();
    return mTextureFrameWalk;
}

//---------------------------------------------------------------
void TerrTexture::buildBlendMap(AllocatedTexture* tex)
{

    PROFILE_START(TerrainTextureBuildBlendMap);

    // The goal here is to produce a blended texture for the terrain code
    // to use on the relevant square. We do this on-card with a render-to-texture.
    // On fast cards this will go VERY quickly. On bad cards it shouldn't be much
    // worse than the old, MMX-optimized version (or the C implementation).

    mDynamicTextureCount++;

    // Try to reuse a texture
    if (mTextureFreeList.size())
    {
        //      Con::printf("  - got from a list that's %d long", mTextureFreeList.size());
        tex->handle = mTextureFreeList.last();
        mTextureFreeList.last() = NULL;
        mTextureFreeList.decrement();
    }

    tex->handle = TerrainRender::mCurrentBlock->mBlender->blend(
        tex->x, tex->y, tex->level,
        TerrainRender::mCurrentBlock->getLightMapTexture(),
        tex->handle);

    PROFILE_END();

}
