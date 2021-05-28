#include "atlas/runtime/atlasClipMapUniqueCache.h"
#include "platform/platform.h"
#include "core/frameAllocator.h"

AtlasClipMapImageCache_TextureTOC::AtlasClipMapImageCache_TextureTOC()
{
    mCacheOrigin.set(0, 0);
}

AtlasClipMapImageCache_TextureTOC::~AtlasClipMapImageCache_TextureTOC()
{
    SAFE_DELETE(mAitToc);
}

/// Note the resource TOC we want to request texture data from.
void AtlasClipMapImageCache_TextureTOC::setTOC(AtlasResourceTexTOC* arttoc, AtlasClipMap* acm)
{
    mAitToc = new AtlasInstanceTexTOC;
    mAitToc->initializeTOC(arttoc);
    arttoc->immediateLoad(arttoc->getStub(0), AtlasTOC::RootLoad);

    // Assume we are square non-DDS textures and get texture size.
    AssertFatal(arttoc->getStub(0)->mChunk,
        "AtlasClipMapImageCache_TextureTOC::setTOC - no chunk in root stub!");
    AssertFatal(arttoc->getStub(0)->mChunk->bitmap,
        "AtlasClipMapImageCache_TextureTOC::setTOC - no bitmap in root chunk!");

    mTOCTextureSize = arttoc->getStub(0)->mChunk->bitmap->getWidth();

    // Now, make sure that for the desired size of clipmap we have the right
    // number of levels and have the appropriate level offset ie when dealing
    // with a clipmap wider than the top of the TOC.

    // Go down till clipmap matches size of level...
    const U32 clipMapSize = acm->mClipMapSize;
    U32 offset = 0;
    bool foundOne = false;
    for (S32 i = 0; i < arttoc->getTreeDepth(); i++)
    {
        const U32 tocLevelSize = BIT(i) * mTOCTextureSize;

        // If the TOC is smaller than the clipmap at this level, skip it.
        if (tocLevelSize < clipMapSize)
            continue;

        offset = i;
        foundOne = true;
        break;
    }

    mLevelOffset = offset;

    AssertISV(foundOne, "AtlasClipMapImageCache_TextureTOC::setTOC - "
        "clipmap size is bigger than all levels of the TOC! Set smaller clipmap size or bigger texture TOC!");

    // Also, update the clipmap's notion of how big a texture it's dealing with.
    acm->mTextureSize = BIT(arttoc->getTreeDepth() - 1) * mTOCTextureSize;
}

void AtlasClipMapImageCache_TextureTOC::initialize(U32 clipMapSize, U32 clipMapDepth)
{
    mClipMapSize = clipMapSize;
    mClipMapDepth = clipMapDepth;

    mCacheRadius = mCeil((mCeil(F32(mClipMapSize) / F32(mTOCTextureSize)) + 2.0) / 2.0) * mTOCTextureSize;
}

bool AtlasClipMapImageCache_TextureTOC::isDataAvailable(U32 inMipLevel, RectI region)
{
    // Check all the chunks of all the stubs in the appropriate region...
    const U32 mipLevel = mAitToc->getTreeDepth() - inMipLevel - 1;
    RectI r;
    convertToTOCRect(inMipLevel, region, r);

    const S32 xStart = mClamp(r.point.x, 0, BIT(mipLevel));
    const S32 xEnd = mClamp(r.point.x + r.extent.x, 0, BIT(mipLevel));

    const S32 yStart = mClamp(r.point.y, 0, BIT(mipLevel));
    const S32 yEnd = mClamp(r.point.y + r.extent.y, 0, BIT(mipLevel));

    //bool haveData = true;

    for (S32 x = xStart; x < xEnd; x++)
    {
        for (S32 y = yStart; y < yEnd; y++)
        {
            AtlasResourceTexStub* arts = mAitToc->getResourceStub(mAitToc->getStub(mipLevel, Point2I(x, y)));

            if (!arts->hasResource())
            {
                /*Con::printf("   - data unavailable for level %d - stub (%d,%d@%d) p=%f ref=%d",
                   inMipLevel, x,y,mipLevel,
                   arts->mRequests.calculateCumulativePriority(), arts->mRequests.getRefCount());*/

                   //haveData = false; 
                return false;
            }

        }
    }

    return true; //haveData;
}

void AtlasClipMapImageCache_TextureTOC::setInterestCenter(Point2I origin)
{
    // Walk the quadtree, topmost unloaded stuff has highest priority.
    // We also need to issue unloads for stuff around the old origin...
    // So let's iterate over everything within the bounds of our radius of BOTH
    // origins. If it's within the originRect, keep it, otherwise cancel it.

    RectI oldOriginTexelRect, newOriginTexelRect;
    RectI oldOriginTOCRect, newOriginTOCRect;
    RectI unionRect;

    for (S32 i = mAitToc->getTreeDepth() - 1; i >= S32(mLevelOffset); i--)
    {
        const U32 shift = mAitToc->getTreeDepth() - i - 1;

        oldOriginTexelRect.point = mCacheOrigin;
        oldOriginTexelRect.point.x >>= shift;
        oldOriginTexelRect.point.y >>= shift;
        oldOriginTexelRect.extent.set(0, 0);
        oldOriginTexelRect.inset(-mCacheRadius, -mCacheRadius);

        newOriginTexelRect.point = origin;
        newOriginTexelRect.point.x >>= shift;
        newOriginTexelRect.point.y >>= shift;
        newOriginTexelRect.extent.set(0, 0);
        newOriginTexelRect.inset(-mCacheRadius, -mCacheRadius);

        convertToTOCRect(i, oldOriginTexelRect, oldOriginTOCRect);
        convertToTOCRect(i, newOriginTexelRect, newOriginTOCRect);

        unionRect = oldOriginTOCRect;
        unionRect.unionRects(newOriginTOCRect);

        // Clamp our update region so we're not walking stuff that is out of range.
        const S32 xStart = mClamp(unionRect.point.x, 0, BIT(i));
        const S32 xEnd = mClamp(unionRect.point.x + unionRect.extent.x, 0, BIT(i));

        const S32 yStart = mClamp(unionRect.point.y, 0, BIT(i));
        const S32 yEnd = mClamp(unionRect.point.y + unionRect.extent.y, 0, BIT(i));

        for (S32 x = xStart; x < xEnd; x++)
        {
            for (S32 y = yStart; y < yEnd; y++)
            {
                const Point2I pos(x, y);
                AtlasInstanceTexStub* aits = mAitToc->getStub(i, pos);

                // Note we weight our loads by depth - the closest to the root
                // goes first.
                if (newOriginTOCRect.pointInRect(pos))
                    mAitToc->requestLoad(aits, AtlasTOC::NormalPriority,
                        F32(mClipMapDepth - i + 1) / F32(mClipMapDepth + 1));
                else
                {
                    mAitToc->cancelLoadRequest(aits, AtlasTOC::NormalPriority);
                }
            }
        }
    }

    // Now we can update the cache origin.
    mCacheOrigin = origin;
}

void AtlasClipMapImageCache_TextureTOC::beginRectUpdates(AtlasClipMap::ClipStackEntry& cse)
{
    // Nothing to do here...
}

void AtlasClipMapImageCache_TextureTOC::doRectUpdate(U32 mipLevel, AtlasClipMap::ClipStackEntry& cse, RectI srcRegion, RectI dstRegion)
{
    // Get a lock and call our blitter.
    GFXLockedRect* lockRect = cse.mTex->lock(0, &dstRegion);
    bitblt(mipLevel, cse, srcRegion, lockRect->bits, lockRect->pitch);
    cse.mTex.unlock();
}

void AtlasClipMapImageCache_TextureTOC::finishRectUpdates(AtlasClipMap::ClipStackEntry& cse)
{
    // Nothing to do here either.
}


struct ColumnNote
{
    S32 chunkColStart, chunkColEnd, colStart, colEnd;
    AtlasTexChunk* atc;
};

void AtlasClipMapImageCache_TextureTOC::bitblt(U32 mipLevel,
    const AtlasClipMap::ClipStackEntry& cse,
    RectI srcRegion, U8* bits, U32 pitch)
{

    /*Con::printf("           + copying data from rect (%d,%d ext=%d,%d)",
       srcRegion.point.x, srcRegion.point.y,
       srcRegion.extent.x, srcRegion.extent.y); */

       // We want to write everything out in linear order, in order to have
       // (hopefully) have good memory characteristics, esp. if we're writing onto
       // the bus.
       //
       // So we need to walk across the relevant sections of the texture chunks,
       // so our IO looks something like this:
       //      Reads                    Writes
       //   Across Chunks             To Clip Map
       // |----| |----| |----|      |----------------|
       // |  12| |3456| |78  |      |    12345678    |
       // |  12| |3456| |78  |      |    12345678    |
       // |----| |----| |----|      |    12345678    |
       //                           |                |
       // |----| |----| |----|      |----------------|
       // |  12| |3456| |78  |
       // |    | |    | |    |
       // |----| |----| |----|
       //
       // Neat! Naturally we need to determine this efficiently as we're in a 
       // texture lock when we do it. (Ick!)

    mipLevel = mAitToc->getTreeDepth() - mipLevel - 1;

    RectI srcTOCs;
    convertToTOCRect(mipLevel, srcRegion, srcTOCs);

    // Make sure we CAN bitblt this... must be all 
    const RectI safeTOCRegion(0, 0, BIT(mipLevel), BIT(mipLevel));
    if (!safeTOCRegion.contains(srcTOCs))
        return;

    U32 texelsCopied = 0;

    const S32 left = mClamp(srcTOCs.point.x, 0, BIT(mipLevel));
    const S32 top = mClamp(srcTOCs.point.y, 0, BIT(mipLevel));
    const S32 right = mClamp(srcTOCs.point.x + srcTOCs.extent.x, 0, BIT(mipLevel));
    const S32 bottom = mClamp(srcTOCs.point.y + srcTOCs.extent.y, 0, BIT(mipLevel));

    // Memoize column info - will save us a TON of work if we have a vertical
    // stripe. (With 512px clipmap, if we move one pixel "against the grain"
    // it will let us save 512 times calculating this stuff, if we're
    // copying from one tile...)

    FrameTemp<ColumnNote> cnotes(bottom - top);

    for (S32 i = 0; i < bottom - top; i++)
    {
        const S32 y = i + top;

        // Now we're walking along the columns, so figure a min/max column
        // for this chunk's row.
        cnotes[i].chunkColStart = S32(y * mTOCTextureSize);
        cnotes[i].chunkColEnd = S32((y + 1) * mTOCTextureSize);

        AssertFatal(srcRegion.point.y < cnotes[i].chunkColEnd,
            "AtlasClipMapImageCache_TextureTOC::bitblit - considering chunk outside source region (end)!");

        AssertFatal(srcRegion.point.y + srcRegion.extent.y >= cnotes[i].chunkColStart,
            "AtlasClipMapImageCache_TextureTOC::bitblit - considering chunk outside source region (start)!");

        cnotes[i].colStart = mClamp(srcRegion.point.y,
            cnotes[i].chunkColStart, cnotes[i].chunkColEnd);
        cnotes[i].colEnd = mClamp(srcRegion.point.y + srcRegion.extent.y,
            cnotes[i].chunkColStart, cnotes[i].chunkColEnd);

        AssertFatal(cnotes[i].chunkColStart < cnotes[i].chunkColEnd,
            "AtlasClipMapImageCache_TextureTOC::bitblit - invalid cnotes[i].chunkCol start and end.");

        AssertFatal(cnotes[i].colStart < cnotes[i].colEnd,
            "AtlasClipMapImageCache_TextureTOC::bitblit - invalid cnotes[i].col start and end.");
    }

    for (S32 x = left; x < right; x++)
    {
        // So, in reference to above diagram, we're walking across some
        // chunks that contain image data we care about. For each row of
        // pixels we need to copy we have to walk across the chunks in this row.

        // Let's figure what rows those are, then we can do some more copying.
        const S32 chunkRowStart = S32(x * mTOCTextureSize);
        const S32 chunkRowEnd = S32((x + 1) * mTOCTextureSize);

        const S32 rowStart = getMax(chunkRowStart, srcRegion.point.x);
        const S32 rowEnd = getMin(chunkRowEnd, srcRegion.point.x + srcRegion.extent.x);

        // Update chunk bitmap pointers.
        for (S32 i = 0; i < (bottom - top); i++)
        {
            const S32 y = i + top;
            cnotes[i].atc = mAitToc->getResourceStub(mAitToc->getStub(mipLevel, Point2I(x, y)))->mChunk;
            AssertWarn(cnotes[i].atc, avar("AtlasClipMapImageCache_TextureTOC::bitblt - missing chunk (%d,%d@%d)!", x, y, mipLevel));
        }

        for (S32 row = rowStart; row < rowEnd; row++)
        {
            for (S32 y = top; y < bottom; y++)
            {
                const S32 i = y - top;

                // Now correct into chunkspace and copy some image data out.
                U8* outBits = bits +
                    (row - srcRegion.point.x) * pitch +
                    4 * (cnotes[i].colStart - srcRegion.point.y);
                texelsCopied += (cnotes[i].colEnd - cnotes[i].colStart);

                if (!cnotes[i].atc)
                {
                    // Do some magenta!
                    for (S32 j = 0; j < cnotes[i].colEnd - cnotes[i].colStart; j++)
                    {
                        outBits[j * 4 + 0] = 0xFF;
                        outBits[j * 4 + 1] = 0x00;
                        outBits[j * 4 + 2] = 0xFF;
                        outBits[j * 4 + 3] = 0x00;
                    }

                    continue;
                }

                const U8* bmpBits = cnotes[i].atc->bitmap->getAddress(
                    cnotes[i].colStart - cnotes[i].chunkColStart,
                    row - chunkRowStart);


                if (cnotes[i].atc->bitmap->bytesPerPixel == 4)
                {
                    dMemcpy(outBits, bmpBits,
                        4 * (cnotes[i].colEnd - cnotes[i].colStart));
                }
                else if (cnotes[i].atc->bitmap->bytesPerPixel == 3)
                {
                    // Do a fast little copy
                    const U8* curIn = bmpBits;
                    const U8* endIn = (bmpBits + (cnotes[i].colEnd - cnotes[i].colStart) * 3);
                    U32* curOut = (U32*)outBits;

                    while (curIn < endIn)
                    {
                        *curOut = (*(U32*)curIn) & 0x00FFFFFF;
                        curIn += 3;
                        curOut++;
                    }
                }
                else
                {
                    for (S32 z = 0; z < (cnotes[i].colEnd - cnotes[i].colStart) * 2; z++)
                    {
                        outBits[z * 2 + 0] = row % 137;
                        outBits[z * 2 + 1] = (cnotes[i].colStart + z) % 137;
                    }
                }
            }
        }
    }

    AssertFatal(texelsCopied == (srcRegion.extent.x * srcRegion.extent.y),
        "AtlasClipMapImageCache_TextureTOC - Copied fewer texels than expected!");
}
