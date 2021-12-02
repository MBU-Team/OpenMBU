//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "atlas/runtime/atlasClipMap.h"
#include "atlas/runtime/atlasClipMapCache.h"
#include "materials/shaderData.h"

GFX_ImplementTextureProfile(AtlasClipMapTextureProfile,
    GFXTextureProfile::DiffuseMap,
    GFXTextureProfile::Static | GFXTextureProfile::PreserveSize,
    GFXTextureProfile::None);

GFX_ImplementTextureProfile(AtlasClipMapTextureRTProfile,
    GFXTextureProfile::DiffuseMap,
    GFXTextureProfile::Static | GFXTextureProfile::PreserveSize | GFXTextureProfile::RenderTarget,
    GFXTextureProfile::None);

void AtlasClipMap::ClipStackEntry::initDebugTexture(U32 level)
{
    static U8 colors[7][3] =
    {
       {0xFF, 0x00, 0x00},
       {0xFF, 0xFF, 0x00},
       {0x00, 0xFF, 0x00},
       {0x00, 0xFF, 0xFF},
       {0x00, 0x00, 0xFF},
       {0xFF, 0x00, 0xFF},
       {0xFF, 0xFF, 0xFF},
    };

    GBitmap* debugBmp = new GBitmap(4, 4);

    ColorI debugClr(colors[level][0], colors[level][1], colors[level][2]);
    ColorI edgeClr(colors[level][2], colors[level][1], colors[level][0]);

    for (S32 i = 0; i < 4; i++)
        for (S32 j = 0; j < 4; j++)
        {
            bool edge = i == 0 || j == 0 || i == 3 || j == 3;
            debugBmp->setColor(i, j, edge ? edgeClr : debugClr);
        }

    mDebugTex.set(debugBmp, &GFXDefaultStaticDiffuseProfile, true);
}

//-----------------------------------------------------------------------------

AtlasClipMap::AtlasClipMap()
{
    // Start with some modest requirements.
    mClipMapSize = 512;

    mMaxTexelUploadPerRecenter = mClipMapSize * mClipMapSize;

    // Debug builds run slower so scale this down a bit - maybe to 50%
#ifdef TORQUE_DEBUG
    mMaxTexelUploadPerRecenter /= 2;
#endif

    mClipStackDepth = 0;

    // Use an 8k texture.
    mTextureSize = BIT(13);

    mImageCache = NULL;

    mTotalUpdates = mTexelsUpdated = 0;

    // Set shaders to NULL initially.
    for (S32 i = 0; i < 4; i++)
        mShaders[i] = 0;

    mTexCallbackHandle = -1;
}

AtlasClipMap::~AtlasClipMap()
{
    if (mTexCallbackHandle != -1)
        GFX->unregisterTexCallback(mTexCallbackHandle);
}

void AtlasClipMap::texCB(GFXTexCallbackCode code, void* userData)
{
    AtlasClipMap* acm = (AtlasClipMap*)userData;
    AssertFatal(acm, "AtlasClipMap::texCB - got null userData");

    if (code == GFXZombify)
    {
        // Deallocate our textures.
        for (S32 i = 0; i < acm->mLevels.size(); i++)
            acm->mLevels[i].mTex = NULL;
        return;
    }

    if (code == GFXResurrect)
    {
        // Refill our levels.
        U32 time = Platform::getRealMilliseconds();

        // Reallocate our textures.
        GFXTextureProfile* texProfile = acm->mImageCache->isRenderToTargetCache() ?
            &AtlasClipMapTextureRTProfile : &AtlasClipMapTextureProfile;
        for (S32 i = 0; i < acm->mLevels.size(); i++)
            acm->mLevels[i].mTex.set(acm->mClipMapSize, acm->mClipMapSize,
                GFXFormatR8G8B8X8, texProfile, 0);

        acm->fillWithTextureData();
        U32 postTime = Platform::getRealMilliseconds();
        Con::printf("AtlasClipMap::texCB - took %d ms to fill clipmap after zombification.", postTime - time);

        return;
    }
}

void AtlasClipMap::initClipStack()
{
    mLevels.clear();

    // What texture profile are we going to be using?
    AssertFatal(mImageCache, "AtlasClipMap::initClipStack - must have image cache by this point.");
    GFXTextureProfile* texProfile = mImageCache->isRenderToTargetCache() ? &AtlasClipMapTextureRTProfile : &AtlasClipMapTextureProfile;

    // Figure out how many clipstack textures we'll have.
    mClipStackDepth = getBinLog2(mTextureSize) - getBinLog2(mClipMapSize) + 1;
    mLevels.setSize(mClipStackDepth);

    // Print a little report on our allocation.
    Con::printf("Allocating a %d px clipmap for a %dpx source texture.", mClipMapSize, mTextureSize);
    Con::printf("   - %d base clipstack entries, + 1 cap.", mClipStackDepth - 1);

    U32 baseTexSize = (mClipMapSize * mClipMapSize * 4);
    Con::printf("   - Using approximately %fMB of texture memory.",
        (F32(baseTexSize * mClipStackDepth) * 1.33) / (1024.0 * 1024.0));

    // First do our base textures - they are not mipped.
    for (S32 i = 0; i < mClipStackDepth; i++)
    {
        mLevels[i].mScale = BIT(mClipStackDepth - (1 + i));
        mLevels[i].mTex.set(mClipMapSize, mClipMapSize, GFXFormatR8G8B8X8, texProfile, 0); //, getBinLog2(mClipMapSize));
    }

    // Some stuff can get skipped once we're set up.
    if (mTexCallbackHandle != -1)
        return;

    // Don't forget to allocate our debug textures...
    for (S32 i = 0; i < mClipStackDepth; i++)
        mLevels[i].initDebugTexture(i);

    // Do shader lookup for 2,3,4 level shaders.
    for (S32 i = 2; i < 5; i++)
    {
        ShaderData* shader = NULL;
        const char* tmpBuff = avar("AtlasShader%d", i);
        if (!Sim::findObject(StringTable->insert(tmpBuff), shader))
        {
            Con::errorf("AtlasClipMap::initClipStack - could not find shader '%s'!", tmpBuff);
            continue;
        }

        mShaders[i - 1] = shader->getShader();
    }

    // Grab a callback from the texture manager to deal with zombification.
    GFX->registerTexCallback(texCB, this, mTexCallbackHandle);

    // Ok, everything is ready to go.
}

void AtlasClipMap::calculateClipMapLevels(const F32 near, const F32 far,
    const RectF& texBounds,
    S32& outStartLevel, S32& outEndLevel)
{
    PROFILE_START(AtlasClipMap_calculateClipMapLevels);

    // We also have to deal w/ the available data. So let's figure out if our
    // desired TCs are in the loaded textureset.

    // Adjust the specified TC range into a texel range.
    RectF tcR = texBounds;
    tcR.point = Point2F(tcR.point.y * F32(mTextureSize), tcR.point.x * F32(mTextureSize));
    tcR.extent = tcR.extent * F32(mTextureSize);
    tcR.inset(1, 1);

    // Put some safe defaults in for starters.
    outEndLevel = mClipStackDepth - 1;
    outStartLevel = getMax(outEndLevel - 3, S32(0));

    // Now iterate over every clipstack entry and find the smallest that contains
    // the relevant TCs.
    S32 minLevelOverlap = mClipStackDepth + 1;
    S32 maxLevelOverlap = -1;

    for (S32 i = mClipStackDepth - 1; i >= 0; i--)
    {
        // Find region for entry at this level.
        RectF r;
        r.point = Point2F(
            F32(BIT(i)) * mLevels[i].mToroidalOffset.x,
            F32(BIT(i)) * mLevels[i].mToroidalOffset.y);
        r.extent.set(F32(BIT(i) * mClipMapSize), F32(BIT(i) * mClipMapSize));

        // Is our tex region fully contained?
        if (r.contains(tcR))
        {
            // If we're fully contained, then this is our new max.
            maxLevelOverlap = i;
            minLevelOverlap = i;
            continue;
        }

        // Or else maybe we've got overlap?
        if (r.overlaps(tcR))
        {
            // Make sure that texels at this level are going to be visible (at
            // closest point).
            if (near > 100.f)
            {
                F32 texelSize = (far - near) / ((tcR.len_x() + tcR.len_y()) / 2.f);

                F32 size = GFX->projectRadius(near, texelSize);

                // If it's less than 1px tall, we can skip this level...
                if (size < 0.9f)
                    continue;
            }

            // If we're overlapping then this is our new min...
            minLevelOverlap = getMin(minLevelOverlap, i);
            continue;
        }
    }

    // Given our level range, do a best fit. We ALWAYS have to have
    // enough for the minimum detail, so we fit that constraint then
    // do our best to give additional detail on top of that.

    outEndLevel = mClamp(maxLevelOverlap, 1, mClipStackDepth - 1);
    outStartLevel = mClamp(minLevelOverlap, outEndLevel - 3, outEndLevel - 1);

    // Make sure we're not exceeding our max delta.
    const S32 delta = outEndLevel - outStartLevel;
    AssertFatal(delta >= 1 && delta <= 4,
        "AtlasClipMap::calculateClipMapLevels - range in levels outside of 2..4 range!");

    PROFILE_END();
}

void AtlasClipMap::fillWithTestPattern()
{
    AssertISV(false, "AtlasClipMap::fillWithTestPattern - assumes bitmaps, which "
        "are no longer present, switch to lock() semantics if you need this!");

    // Table of random colors.
    U8 colorTbl[16][3] =
    {
       { 0xFF, 0x00, 0x0F },
       { 0xFF, 0x00, 0xA0 },
       { 0xFF, 0x00, 0xFF },
       { 0x00, 0xA0, 0x00 },
       { 0x00, 0xA0, 0xAF },
       { 0x00, 0xA0, 0xF0 },
       { 0xA0, 0xFF, 0xA0 },
       { 0x00, 0xF0, 0xA0 },
       { 0x00, 0xF0, 0xFF },
       { 0xA0, 0x00, 0x00 },
       { 0xA0, 0x00, 0xAF },
       { 0xA0, 0x00, 0xF0 },
       { 0xA0, 0xF0, 0x0F },
       { 0xA0, 0xF0, 0xA0 },
       { 0xA0, 0xF0, 0xFF },
       { 0x00, 0xFF, 0x00 },
    };

    // Lock each layer of each texture and write a test pattern in.

    // Base levels first.
    for (S32 i = 0; i < mClipStackDepth; i++)
    {
        GFXTexHandle& gth = mLevels[i].mTex;
        U8* bmpData = (U8*)gth.getBitmap()->getWritableBits(0);

        for (S32 x = 0; x < gth.getHeight(); x++)
        {
            for (S32 y = 0; y < gth.getWidth(); y++)
            {
                S32 xFlag = x & 4;
                S32 yFlag = y & 4;

                U32 offset = (x * gth.getWidth() + y) * 4;
                if (xFlag ^ yFlag)
                {
                    // Set bright.
                    bmpData[offset + 0] = colorTbl[i][0];
                    bmpData[offset + 1] = colorTbl[i][1];
                    bmpData[offset + 2] = colorTbl[i][2];
                    bmpData[offset + 3] = 0xFF;
                }
                else
                {
                    // Set dim.
                    bmpData[offset + 0] = colorTbl[i][0] / 3;
                    bmpData[offset + 1] = colorTbl[i][1] / 3;
                    bmpData[offset + 2] = colorTbl[i][2] / 3;
                    bmpData[offset + 3] = 0xFF;
                }
            }
        }

        if (i == mClipStackDepth - 1)
            gth.getBitmap()->extrudeMipLevels();
        else
        {
            // Write black/translucent in the higher levels.
            GBitmap* gb = gth.getBitmap();
            for (S32 j = 1; j < gb->numMipLevels; j++)
            {
                U8* b = gb->getWritableBits(j);
                dMemset(b, 0, 4 * gb->getWidth(j) * gb->getHeight(j));
            }
        }

        gth.refresh();
    }
}

void AtlasClipMap::clipAgainstGrid(const S32 gridSpacing, const RectI& rect, S32* outCount, RectI* outBuffer)
{
    // Check against X grids...
    const S32 startX = rect.point.x;
    const S32 endX = rect.point.x + rect.extent.x;

    const S32 gridMask = ~(gridSpacing - 1);
    const S32 startGridX = startX & gridMask;
    const S32 endGridX = endX & gridMask;

    RectI buffer[2];
    S32 rectCount = 0;

    // Check X...
    if (startGridX != endGridX && endX - endGridX > 0)
    {
        // We have a clip! Split against the grid multiple and store.
        rectCount = 2;
        buffer[0].point.x = startX;
        buffer[0].point.y = rect.point.y;
        buffer[0].extent.x = endGridX - startX;
        buffer[0].extent.y = rect.extent.y;

        buffer[1].point.x = endGridX;
        buffer[1].point.y = rect.point.y;
        buffer[1].extent.x = endX - endGridX;
        buffer[1].extent.y = rect.extent.y;
    }
    else
    {
        // Copy it in.
        rectCount = 1;
        buffer[0] = rect;
    }

    // Now, check Y for the one or two rects we have from above.
    *outCount = 0;
    for (S32 i = 0; i < rectCount; i++)
    {
        // Figure our extent and grid information.
        const S32 startY = buffer[i].point.y;
        const S32 endY = buffer[i].point.y + rect.extent.y;
        const S32 startGridY = startY & gridMask;
        const S32 endGridY = endY & gridMask;

        if (startGridY != endGridY && endY - endGridY > 0)
        {
            // We have a clip! Split against the grid multiple and store.
            RectI* buffA = outBuffer + *outCount;
            RectI* buffB = buffA + 1;
            (*outCount) += 2;

            buffA->point.x = buffer[i].point.x;
            buffA->point.y = startY;
            buffA->extent.x = buffer[i].extent.x;
            buffA->extent.y = endGridY - startY;

            buffB->point.x = buffer[i].point.x;
            buffB->point.y = endGridY;
            buffB->extent.x = buffer[i].extent.x;
            buffB->extent.y = endY - endGridY;
        }
        else
        {
            // Copy it in.
            outBuffer[*outCount] = buffer[i];
            (*outCount)++;
        }
    }
}

void AtlasClipMap::calculateModuloDeltaBounds(const RectI& oldData, const RectI& newData,
    RectI* outRects, S32* outRectCount)
{
    // Sanity checking.
    AssertFatal(oldData.point.x >= 0 && oldData.point.y >= 0 && oldData.isValidRect(),
        "AtlasClipMap::calculateModuloDeltaBounds - negative oldData origin or bad rect!");

    AssertFatal(newData.point.x >= 0 && newData.point.y >= 0 && newData.isValidRect(),
        "AtlasClipMap::calculateModuloDeltaBounds - negative newData origin or bad rect!");

    AssertFatal(newData.extent == oldData.extent,
        "AtlasClipMap::calculateModuloDeltaBounts - mismatching extents, can only work with matching extents!");

    // Easiest case - if they're the same then do nothing.
    if (oldData.point == newData.point)
    {
        *outRectCount = 0;
        return;
    }

    // Easy case - if there's no overlap then it's all new!
    if (!oldData.overlaps(newData))
    {
        // Clip out to return buffer, and we're done.
        clipAgainstGrid(mClipMapSize, newData, outRectCount, outRects);
        return;
    }

    // Calculate some useful values for both X and Y. Delta is used a lot
    // in determining bounds, and the boundary values are important for
    // determining where to start copying new data in.
    const S32 xDelta = newData.point.x - oldData.point.x;
    const S32 yDelta = newData.point.y - oldData.point.y;

    const S32 xBoundary = (oldData.point.x + oldData.extent.x) % mClipMapSize;
    const S32 yBoundary = (oldData.point.y + oldData.extent.y) % mClipMapSize;

    AssertFatal(xBoundary % mClipMapSize == oldData.point.x % mClipMapSize,
        "AtlasClipMap::calculateModuleDeltaBounds - we assume that left and "
        "right of the dataset are identical (ie, it's periodical on size of clipmap!) (x)");

    AssertFatal(yBoundary % mClipMapSize == oldData.point.y % mClipMapSize,
        "AtlasClipMap::calculateModuleDeltaBounds - we assume that left and "
        "right of the dataset are identical (ie, it's periodical on size of clipmap!) (y)");

    // Now, let's build up our rects. We have one rect if we are moving
    // on the X or Y axis, two if both. We dealt with the no-move case
    // previously.
    if (xDelta == 0)
    {
        // Moving on Y! So generate and store clipped results.
        RectI yRect;

        if (yDelta < 0)
        {
            // We need to generate the box from right of old to right of new.
            yRect.point = newData.point;
            yRect.extent.x = mClipMapSize;
            yRect.extent.y = -yDelta;
        }
        else
        {
            // We need to generate the box from left of old to left of new.
            yRect.point.x = newData.point.x; // Doesn't matter which rect we get this from.
            yRect.point.y = (oldData.point.y + oldData.extent.y);
            yRect.extent.x = mClipMapSize;
            yRect.extent.y = yDelta;
        }

        // Clip out to return buffer, and we're done.
        clipAgainstGrid(mClipMapSize, yRect, outRectCount, outRects);

        return;
    }
    else if (yDelta == 0)
    {
        // Moving on X! So generate and store clipped results.
        RectI xRect;

        if (xDelta < 0)
        {
            // We need to generate the box from right of old to right of new.
            xRect.point = newData.point;
            xRect.extent.x = -xDelta;
            xRect.extent.y = mClipMapSize;
        }
        else
        {
            // We need to generate the box from left of old to left of new.
            xRect.point.x = (oldData.point.x + oldData.extent.x);
            xRect.point.y = newData.point.y; // Doesn't matter which rect we get this from.
            xRect.extent.x = xDelta;
            xRect.extent.y = mClipMapSize;
        }

        // Clip out to return buffer, and we're done.
        clipAgainstGrid(mClipMapSize, xRect, outRectCount, outRects);

        return;
    }
    else
    {
        // Both! We have an L shape. So let's do the bulk of it in one rect,
        // and the remainder in the other. We'll choose X as the dominant axis.
        //
        // a-----b---------c   going from e to a.
        // |     |         |
        // |     |         |
        // d-----e---------f   So the dominant rect is abgh and the passive
        // |     |         |   rect is bcef. Obviously depending on delta we
        // |     |         |   have to switch things around a bit.
        // |     |         |          y+ ^
        // |     |         |             |  
        // g-----h---------i   x+->      |

        RectI xRect, yRect;

        if (xDelta < 0)
        {
            // Case in the diagram.
            xRect.point = newData.point;
            xRect.extent.x = -xDelta;
            xRect.extent.y = mClipMapSize;

            // Set up what of yRect we know, too.
            yRect.point.x = xRect.point.x + xRect.extent.x;
            yRect.extent.x = mClipMapSize - mAbs(xDelta);
        }
        else
        {
            // Opposite of case in diagram!
            xRect.point.x = oldData.point.x + oldData.extent.x;
            xRect.point.y = newData.point.y;
            xRect.extent.x = xDelta;
            xRect.extent.y = mClipMapSize;

            // Set up what of yRect we know,  too.
            yRect.point.x = (xRect.point.x + xRect.extent.x) - mClipMapSize;
            yRect.extent.x = mClipMapSize - xRect.extent.x;
        }

        if (yDelta < 0)
        {
            // Case in the diagram.
            yRect.point.y = newData.point.y;
            yRect.extent.y = -yDelta;
        }
        else
        {
            // Opposite of case in diagram!
            yRect.point.y = oldData.point.y + oldData.extent.y;
            yRect.extent.y = yDelta;
        }

        // Make sure we don't overlap.
        AssertFatal(!yRect.overlaps(xRect),
            "AtlasClipMap::calculateModuloDeltaBounds - have overlap in result rects!");

        // Ok, now run them through the clipper.
        S32 firstCount;
        clipAgainstGrid(mClipMapSize, xRect, &firstCount, outRects);
        clipAgainstGrid(mClipMapSize, yRect, outRectCount, outRects + firstCount);
        *outRectCount += firstCount;

        // All done!
        return;
    }

    // We really should never get here. All branches of above if return!
    AssertISV(false, "AtlasClipMap::calculateModuloDeltaBounds - should not be here.");
    return;
}

void AtlasClipMap::recenter(Point2F center)
{
    PROFILE_START(AtlasClipMap_recenter);

    AssertFatal(isPow2(mClipMapSize),
        "AtlasClipMap::recenter - require pow2 clipmap size!");

    // Ok, we're going to do toroidal updates on each entry of the clipstack
    // (except for the cap, which covers the whole texture), based on this
    // new center point.
    {
        // Calculate the new texel at most detailed level.
        Point2F texelCenterF = center * F32(mClipMapSize) * mLevels[0].mScale;
        Point2I texelCenter(mFloor(texelCenterF.y), mFloor(texelCenterF.x));

        // Update interest region.
        mImageCache->setInterestCenter(texelCenter);
    }

    // Note how many we were at so we can cut off at the right time.
    S32 lastTexelsUpdated = mTexelsUpdated;

    // For each texture...
    for (S32 i = mClipStackDepth - 2; i >= 0; i--)
    {
        ClipStackEntry& cse = mLevels[i];

        // Calculate new center point for this texture.
        Point2F texelCenterF = center * F32(mClipMapSize) * cse.mScale;

        const S32 texelMin = mClipMapSize / 2;
        const S32 texelMax = S32(F32(mClipMapSize) * cse.mScale) - texelMin;

        Point2I texelTopLeft(
            mClamp(S32(mFloor(texelCenterF.y)), texelMin, texelMax) - texelMin,
            mClamp(S32(mFloor(texelCenterF.x)), texelMin, texelMax) - texelMin);

        // This + current toroid offset tells us what regions have to be blasted.
        RectI oldData(cse.mToroidalOffset, Point2I(mClipMapSize, mClipMapSize));
        RectI newData(texelTopLeft, Point2I(mClipMapSize, mClipMapSize));

        // Make sure we have available data...
        if (!mImageCache->isDataAvailable(i, newData))
            continue;

        // Alright, determine the set of data we actually need to upload.
        RectI buffer[8];
        S32   rectCount = 0;

        calculateModuloDeltaBounds(oldData, newData, buffer, &rectCount);
        AssertFatal(rectCount < 8, "AtlasClipMap::recenter - got too many rects back!");

        cse.mClipCenter = center;
        cse.mToroidalOffset = texelTopLeft;

        /*     if(rectCount)
                Con::printf("    issuing %d updates to clipmap level %d (offset=%dx%d)",
                               rectCount, i, texelTopLeft.x, texelTopLeft.y); */

        if (rectCount)
        {
            mImageCache->beginRectUpdates(cse);
            //Con::errorf("layer %x, %d updates", &cse,  rectCount);

            // And GO!
            for (S32 j = 0; j < rectCount; j++)
            {
                PROFILE_START(AtlasClipMap_recenter_upload);

                AssertFatal(buffer[j].isValidRect(), "AtlasClipMap::recenter - got invalid rect!");

                // Note the rect, so we can then wrap and let the image cache do its thing.
                RectI srcRegion = buffer[j];
                buffer[j].point.x = srcRegion.point.x % mClipMapSize;
                buffer[j].point.y = srcRegion.point.y % mClipMapSize;

                AssertFatal(newData.contains(srcRegion),
                    "AtlasClipMap::recenter - got update buffer outside of expected new data bounds.");

                mTotalUpdates++;
                mTexelsUpdated += srcRegion.extent.x * srcRegion.extent.y;

                mImageCache->doRectUpdate(i, cse, srcRegion, buffer[j]);

                PROFILE_END();
            }

            mImageCache->finishRectUpdates(cse);
        }

        // Check if we've overrun our budget.
        if ((mTexelsUpdated - lastTexelsUpdated) > mMaxTexelUploadPerRecenter)
        {
            //Con::warnf("AtlasClipMap::recenter - exceeded budget for this frame, deferring till next frame.");
            break;
        }

    }

    PROFILE_END();
}

bool AtlasClipMap::fillWithTextureData()
{
    // Note our interest for the cache.
    {
        // Calculate the new texel at most detailed level.
        Point2F texelCenterF = mLevels[0].mClipCenter * F32(mClipMapSize) * mLevels[0].mScale;
        Point2I texelCenter(mFloor(texelCenterF.y), mFloor(texelCenterF.x));

        // Update interest region.
        mImageCache->setInterestCenter(texelCenter);
    }

    // First generate our desired rects for each level.
    FrameTemp<RectI> desiredData(mClipStackDepth);
    for (S32 i = 0; i < mClipStackDepth; i++)
    {
        ClipStackEntry& cse = mLevels[i];

        // Calculate new center point for this texture.
        Point2F texelCenterF = cse.mClipCenter * F32(mClipMapSize) * cse.mScale;

        const S32 texelMin = mClipMapSize / 2;
        const S32 texelMax = S32(F32(mClipMapSize) * cse.mScale) - texelMin;

        Point2I texelTopLeft(
            mClamp(S32(mFloor(texelCenterF.y)), texelMin, texelMax) - texelMin,
            mClamp(S32(mFloor(texelCenterF.x)), texelMin, texelMax) - texelMin);

        desiredData[i] = RectI(texelTopLeft, Point2I(mClipMapSize, mClipMapSize));

        // Make sure we have available data...
        if (!mImageCache->isDataAvailable(i, desiredData[i]))
            return false;
    }

    // Upload all the textures...
    for (S32 i = 0; i < mClipStackDepth; i++)
    {
        ClipStackEntry& cse = mLevels[i];

        RectI buffer[8];
        S32   rectCount = 0;

        clipAgainstGrid(mClipMapSize, desiredData[i], &rectCount, buffer);
        AssertFatal(rectCount < 8, "AtlasClipMap::fillWithTextureData - got too many rects back!");

        if (rectCount)
        {
            mImageCache->beginRectUpdates(cse);

            for (S32 j = 0; j < rectCount; j++)
            {
                RectI srcRegion = buffer[j];
                buffer[j].point.x = srcRegion.point.x % mClipMapSize;
                buffer[j].point.y = srcRegion.point.y % mClipMapSize;

                mImageCache->doRectUpdate(i, cse, srcRegion, buffer[j]);
            }

            mImageCache->finishRectUpdates(cse);
        }

        cse.mToroidalOffset = desiredData[i].point;
    }

    // Success!
    return true;
}

void AtlasClipMap::setCache(IAtlasClipMapImageCache* cache)
{
    cache->initialize(mClipMapSize, mClipStackDepth);
    mImageCache = cache;
}
