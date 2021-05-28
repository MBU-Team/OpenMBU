//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _TERRTEXTURE_H_
#define _TERRTEXTURE_H_

// #define TERR_TEXTURE_DEBUG

#include "gfx/gfxTextureHandle.h"
#include "materials/matInstance.h"
#include "terrain/terrRender.h"
#include "terrain/terrBatch.h"

struct EmitChunk;

struct AllocatedTexture {
    // Spatial
    U32 level;
    S32 x, y;

    // Render
    EmitChunk* emitList;
    GFXTexHandle handle;

    // For frame/free lists.
    AllocatedTexture* next;
    AllocatedTexture* previous;

    // Cache
    AllocatedTexture* nextLink;

    // Static interface
private:
    static AllocatedTexture* mTextureGrid[AllocatedTextureCount];
    static AllocatedTexture** mTextureGridPtr[5];

    static inline AllocatedTexture* getGrid(U32 level, U32 x, U32 y)
    {
        return mTextureGridPtr[level - 2][x + (y << (8 - level))];
    }

    static inline void setGrid(U32 level, U32 x, U32 y, AllocatedTexture* newEntry)
    {
        mTextureGridPtr[level - 2][x + (y << (8 - level))] = newEntry;
    }

public:
    static void init();
    static AllocatedTexture* alloc(Point2I pos, U32 level);
    static void freeTex(AllocatedTexture* tex);
    static void trimFree();
    static void moveToFrame(AllocatedTexture* tex);
    static void moveToFree(AllocatedTexture* tex);
    static void trimFreeList();
    static void flushCache();
    static void flushCacheRect(RectI bRect);


#ifdef TERR_TEXTURE_DEBUG
    static U32 mAllocTexCount;
#endif

    AllocatedTexture()
    {
#ifdef TERR_TEXTURE_DEBUG
        mAllocTexCount++;
        Con::printf("++ allocTexMake %d %x", mAllocTexCount, this);
#endif
        nextLink = next = previous = NULL;
        emitList = NULL;
        handle = NULL;
    }

    AllocatedTexture(U32 aLevel, S32 aX, S32 aY)
    {
#ifdef TERR_TEXTURE_DEBUG
        mAllocTexCount++;
        Con::printf("++ allocTexMake %d %x (%d, %d)@%d", mAllocTexCount, this, aX, aY, aLevel);
#endif
        nextLink = next = previous = NULL;
        emitList = NULL;
        handle = NULL;

        level = aLevel;
        x = aX;
        y = aY;
    }

    ~AllocatedTexture()
    {
#ifdef TERR_TEXTURE_DEBUG
        mAllocTexCount--;
        Con::printf("-- allocTexKill %d %x", mAllocTexCount, this);
#endif
        handle = NULL;
    }

    inline void safeUnlink()
    {
        if (!(next && previous)) return;

        next->previous = previous;
        previous->next = next;
        next = previous = NULL;
    }

    inline void unlink()
    {
        AssertFatal(next && previous, "Invalid unlink.");
        next->previous = previous;
        previous->next = next;
        next = previous = NULL;
    }
    inline void linkAfter(AllocatedTexture* t)
    {
        AssertFatal(next == NULL && previous == NULL, "Cannot link a non-null next & prev");

        next = t->next;
        previous = t;
        t->next->previous = this;
        t->next = this;
    }
};


namespace TerrTexture
{

    extern S32 mTextureMinSquareSize;
    extern bool mEnableTerrainDetails;

    extern AllocatedTexture* mTextureFrameWalk;

    extern AllocatedTexture mTextureFrameListHead;
    extern AllocatedTexture mTextureFrameListTail;

    extern AllocatedTexture mTextureFreeListHead;
    extern AllocatedTexture mTextureFreeListTail;

    extern AllocatedTexture* mCurrentTexture;

    extern U32 mTextureSlopSize;
    extern Vector<GFXTexHandle> mTextureFreeList;

    extern S32 mDynamicTextureCount;
    extern S32 mTextureSpaceUsed;
    extern S32 mLevelZeroCount;
    extern S32 mFullMipCount;
    extern S32 mStaticTextureCount;
    extern S32 mUnusedTextureCount;
    extern S32 mStaticTSU;
    extern U32 mNewGenTextureCount;


    void init();
    void shutdown();
    void textureRecurse(SquareStackNode* stack);
    void startFrame();
    AllocatedTexture* getNextFrameTexture();
    void buildBlendMap(AllocatedTexture* tex);
};

#endif

