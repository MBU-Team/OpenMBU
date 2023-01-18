#ifdef _MSC_VER
#pragma warning(disable: 4996) // turn off "deprecation" warnings
#endif

#include <d3d9.h>
#include <d3dx9math.h>
#include "gfx/D3D/gfxD3DDevice.h"

#include "platform/platform.h"
#include "atlas/runtime/atlasClipMapBlenderCache.h"
#include "core/frameAllocator.h"
#include "materials/shaderData.h"

//-----------------------------------------------------------------------------

AtlasClipMapImageCache_Blender::AtlasClipMapImageCache_Blender()
{
    mCacheOrigin.set(0, 0);
    mLevelOffset = 0;
    mSourceImages.clear();

    mOnePass = NULL;
    mTwoPass[0] = NULL;
    mTwoPass[1] = NULL;
}

AtlasClipMapImageCache_Blender::~AtlasClipMapImageCache_Blender()
{
    SAFE_DELETE(mOpacityTOC);
    SAFE_DELETE(mShadowTOC);
}

void AtlasClipMapImageCache_Blender::registerSourceImage(const char* filename)
{
    mSourceImages.increment();
    mSourceImages.last() = new GFXTexHandle(StringTable->insert(filename), &GFXDefaultStaticDiffuseProfile);
}

/// Note the resource TOC we want to request texture data from.
void AtlasClipMapImageCache_Blender::setTOC(AtlasResourceTexTOC* opacityToc,
    AtlasResourceTexTOC* shadowToc,
    AtlasClipMap* acm)
{
    // Grab our TOCs and their root levels.
    mShadowTOC = new AtlasInstanceTexTOC;
    mShadowTOC->initializeTOC(shadowToc);
    shadowToc->immediateLoad(shadowToc->getStub(0), AtlasTOC::RootLoad);

    mOpacityTOC = new AtlasInstanceTexTOC;
    mOpacityTOC->initializeTOC(opacityToc);
    opacityToc->immediateLoad(opacityToc->getStub(0), AtlasTOC::RootLoad);

    // Assume we are square non-DDS textures and get texture size.
    AssertFatal(shadowToc->getStub(0)->mChunk,
        "AtlasClipMapImageCache_Blender::setTOC - no chunk in root shadow stub!");
    AssertFatal(shadowToc->getStub(0)->mChunk->bitmap,
        "AtlasClipMapImageCache_Blender::setTOC - no bitmap in root shadow chunk!");

    AssertFatal(opacityToc->getStub(0)->mChunk,
        "AtlasClipMapImageCache_Blender::setTOC - no chunk in root shadow stub!");
    AssertFatal(opacityToc->getStub(0)->mChunk->bitmap,
        "AtlasClipMapImageCache_Blender::setTOC - no bitmap in root shadow chunk!");

    mShadowChunkSize = shadowToc->getStub(0)->mChunk->bitmap->getWidth();
    mOpacityChunkSize = opacityToc->getStub(0)->mChunk->bitmap->getWidth();

    // As for the clipmap, we're gonna expand to whatever size it wants, so
    // we don't need to do any adjustment on its end. But we do have to set up
    // the TOC aggregator.
    AtlasInstanceTexTOC* aittocs[2] = { mShadowTOC, mOpacityTOC };
    mAggregateTOCs.initialize(aittocs, 2,
        getBinLog2(acm->mTextureSize / getMin(mShadowChunkSize, mOpacityChunkSize)));

    mCacheRadius = getMin(mShadowChunkSize, mOpacityChunkSize) * 4;
}

void AtlasClipMapImageCache_Blender::initialize(U32 clipMapSize, U32 clipMapDepth)
{
    mClipMapDepth = clipMapDepth;
    mClipMapSize = clipMapSize;

    mOpacityScratchTexture.set(mClipMapSize, mClipMapSize, GFXFormatR8G8B8A8, &AtlasClipMapTextureProfile, 1);
    mShadowScratchTexture.set(mClipMapSize, mClipMapSize, GFXFormatR8G8B8X8, &AtlasClipMapTextureProfile, 1);

    // Find and init shaders.
    ShaderData* sd = NULL;

    if (!Sim::findObject("AtlasBlender20Shader", sd) || (sd->getShader() == NULL))
    {
        Con::errorf("AtlasClipMapImageCache_Blender::initialize - Couldn't find shader 'TerrBlender20Shader'! Terrain will not blend properly on SM2.0 cards!");
    }
    else
    {
        mOnePass = sd->getShader();
    }

    if (!Sim::findObject("AtlasBlender11AShader", sd) || (sd->getShader() == NULL))
    {
        Con::errorf("AtlasClipMapImageCache_Blender::initialize - Couldn't find shader 'AtlasBlender11AShader'! Terrain will not blend properly on SM1.0 cards!");
    }
    else
    {
        mTwoPass[0] = sd->getShader();
    }

    if (!Sim::findObject("AtlasBlender11BShader", sd) || (sd->getShader() == NULL))
    {
        Con::errorf("AtlasClipMapImageCache_Blender::initialize - Couldn't find shader 'AtlasBlender11BShader'! Terrain will not blend properly on SM1.0 cards!");
    }
    else
    {
        mTwoPass[1] = sd->getShader();
    }
}

bool AtlasClipMapImageCache_Blender::isDataAvailable(U32 inMipLevel, RectI inRegion)
{
    PROFILE_START(AtlasClipMapImageCache_Blender_isDataAvailable);

    // Check all the chunks of all the stubs in the appropriate region...
    const U32 realMipLevel = mAggregateTOCs.getTreeDepth() - inMipLevel;
    RectI r;
    RectI region;
    // Iterate over all of our TOCs...
    for (S32 tocIdx = 0; tocIdx < mAggregateTOCs.mTocCount; tocIdx++)
    {
        // Snag reference to our TOC...
        AtlasInstanceTexTOC* aittoc = mAggregateTOCs.mTOC[tocIdx];

        // If this TOC isn't that deep, then we have to adjust up till we
        // ARE looking at it - we're still using data from it!

        U32 mipLevel = realMipLevel;
        region = inRegion;
        if (mipLevel >= aittoc->getTreeDepth())
        {
            mipLevel = aittoc->getTreeDepth() - 1;

            // Adjust region as well. We can abuse convertToTOCRectWithChunkSize
            // for this.
            S32 scaleFactor = BIT(realMipLevel - mipLevel);
            convertToTOCRectWithChunkSize(scaleFactor, inRegion, region);
        }

        // What are our bounds?
        region.inset(-1, -1); // Add one pixel to extents to make sure we've got data for seams.
        convertToTOCRectWithChunkSize(aittoc->getTextureChunkSize(), region, r);

        // Clamp our update region so we're not walking stuff that is out of range.
        const S32 xStart = mClamp(r.point.x, 0, BIT(mipLevel));
        const S32 xEnd = mClamp(r.point.x + r.extent.x, 0, BIT(mipLevel));

        const S32 yStart = mClamp(r.point.y, 0, BIT(mipLevel));
        const S32 yEnd = mClamp(r.point.y + r.extent.y, 0, BIT(mipLevel));

        // Iterate over relevant chunks and if they're not present, return false.
        for (S32 x = xStart; x < xEnd; x++)
            for (S32 y = yStart; y < yEnd; y++)
                if (!aittoc->getResourceStub(aittoc->getStub(mipLevel, Point2I(x, y)))->mChunk)
                {
                    PROFILE_END();
                    return false;
                }
    }

    PROFILE_END();

    // Hey, we have everything!
    return true;
}

void AtlasClipMapImageCache_Blender::setInterestCenter(Point2I origin)
{
    PROFILE_START(AtlasClipMapImageCache_Blender_setInterestCenter);

    // Walk the quadtree, topmost unloaded stuff has highest priority.
    // We also need to issue unloads for stuff around the old origin...
    // So let's iterate over everything within the bounds of our radius of BOTH
    // origins. If it's within the originRect, keep it, otherwise cancel it.

    RectI oldOriginTexelRect, newOriginTexelRect;
    RectI oldOriginTOCRect, newOriginTOCRect;
    RectI unionRect;

    // We only ever have to traverse as deep as the deepest TOC that we've
    // aggregated.
    for (S32 i = (mAggregateTOCs.getTreeDepth() - 1); i >= S32(mLevelOffset); i--)
    {
        const U32 shift = mAggregateTOCs.getTreeDepth() - i;

        // Note what texels we'll need for this level.
        oldOriginTexelRect.point = mCacheOrigin;
        oldOriginTexelRect.point.x >>= shift;
        oldOriginTexelRect.point.y >>= shift;
        oldOriginTexelRect.extent.set(0, 0);
        oldOriginTexelRect.inset(-mCacheRadius - 1, -mCacheRadius - 1);

        newOriginTexelRect.point = origin;
        newOriginTexelRect.point.x >>= shift;
        newOriginTexelRect.point.y >>= shift;
        newOriginTexelRect.extent.set(0, 0);
        newOriginTexelRect.inset(-mCacheRadius - 1, -mCacheRadius - 1);

        // And issue load requests to every relevant TOC...
        for (S32 tocIdx = 0; tocIdx < mAggregateTOCs.mTocCount; tocIdx++)
        {
            // Snag reference to our TOC...
            AtlasInstanceTexTOC* aittoc = mAggregateTOCs.mTOC[tocIdx];

            // Skip if this TOC isn't this deep.
            if (i >= aittoc->getTreeDepth())
                continue;

            // Figure our bounds for this TOC traversal.
            convertToTOCRectWithChunkSize(aittoc->getTextureChunkSize(), oldOriginTexelRect, oldOriginTOCRect);
            convertToTOCRectWithChunkSize(aittoc->getTextureChunkSize(), newOriginTexelRect, newOriginTOCRect);
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

                    // Note we weight our loads by depth - the closest to the root
                    // goes first.
                    if (newOriginTOCRect.pointInRect(pos))
                    {
                        aittoc->requestLoad(aittoc->getStub(i, pos), AtlasTOC::NormalPriority,
                            F32(mClipMapDepth - i + 1) / F32(mClipMapDepth + 1));
                    }
                    else
                    {
                        aittoc->cancelLoadRequest(aittoc->getStub(i, pos), AtlasTOC::NormalPriority);
                    }
                }
            }
        }
    }

    // Now we can update the cache origin.
    mCacheOrigin = origin;

    PROFILE_END();
}

// Ugly globals.
MatrixF proj;
RectI viewPort;
bool mustEndScene = false;
S32 pushDepth = 0;

void AtlasClipMapImageCache_Blender::beginRectUpdates(AtlasClipMap::ClipStackEntry& cse)
{
    pushDepth++;
    AssertFatal(pushDepth == 1, "BAD1");

    if (!GFX->canCurrentlyRender())
    {
        mustEndScene = true;
        GFX->beginScene();
    }
    else
    {
        mustEndScene = false;
    }

    proj = GFX->getProjectionMatrix();
    viewPort = GFX->getViewport();
    GFX->pushWorldMatrix();

    // For sanity, let's just purge all our texture states.
    for (S32 i = 0; i < GFX->getNumSamplers(); i++)
        GFX->setTexture(i, NULL);
    GFX->updateStates();

    // Set a render target...
    GFX->pushActiveRenderSurfaces();
    GFX->pushActiveZSurface();
    GFX->setActiveRenderSurface(cse.mTex);
    GFX->setActiveZSurface(NULL);

    // And some render states.
    GFX->setZEnable(false);
    GFX->setZWriteEnable(false);
    GFX->setWorldMatrix(MatrixF(true));
    GFX->setProjectionMatrix(MatrixF(true));
    GFX->disableShaders();

    GFX->setClipRect(RectI(0, 0, mClipMapSize, mClipMapSize));
    GFX->setCullMode(GFXCullNone);

    // And pass it what it needs modelview wise...
    MatrixF proj = GFX->getProjectionMatrix();
    proj.mul(GFX->getWorldMatrix());
    proj.transpose();
    GFX->setVertexShaderConstF(0, (float*)&proj, 4);

    for (S32 i = 0; i < 2; i++)
    {
        GFX->setTexture(i, NULL);
        GFX->setTextureStageMagFilter(i, GFXTextureFilterLinear);
        GFX->setTextureStageMinFilter(i, GFXTextureFilterLinear);
        GFX->setTextureStageMipFilter(i, GFXTextureFilterLinear);
        GFX->setTextureStageAddressModeU(i, GFXAddressClamp);
        GFX->setTextureStageAddressModeV(i, GFXAddressClamp);
    }

    for (S32 i = 0; i < mSourceImages.size(); i++)
    {
        GFX->setTexture(i + 2, *mSourceImages[i]);
        GFX->setTextureStageMagFilter(i + 2, GFXTextureFilterLinear);
        GFX->setTextureStageMinFilter(i + 2, GFXTextureFilterLinear);
        GFX->setTextureStageMipFilter(i + 2, GFXTextureFilterLinear);
        GFX->setTextureStageAddressModeU(i + 2, GFXAddressWrap);
        GFX->setTextureStageAddressModeV(i + 2, GFXAddressWrap);
    }

    // Set blend modes. (we want additive).
    GFX->setAlphaBlendEnable(true);
    GFX->setSrcBlend(GFXBlendOne);
    GFX->setDestBlend(GFXBlendOne);

    GFX->setAlphaTestEnable(false);

    GFX->updateStates();
}

void AtlasClipMapImageCache_Blender::setupGeometry(
    const RectF& srcRect0,
    const RectF& srcRect1,
    const RectF& masterCoords,
    const RectI& dstRect)
{
    GFXVertexBufferHandle<GFXVertexPTTT> verts(GFX, 4, GFXBufferTypeVolatile);

    const F32 masterLeft = F32(masterCoords.point.y);
    const F32 masterRight = F32(masterCoords.point.y + masterCoords.extent.y);
    const F32 masterTop = F32(masterCoords.point.x);
    const F32 masterBottom = F32(masterCoords.point.x + masterCoords.extent.x);

    const F32 tex0Left = F32(srcRect0.point.y) / F32(mClipMapSize);
    const F32 tex0Right = F32(srcRect0.point.y + srcRect0.extent.y) / F32(mClipMapSize);
    const F32 tex0Top = F32(srcRect0.point.x) / F32(mClipMapSize);
    const F32 tex0Bottom = F32(srcRect0.point.x + srcRect0.extent.x) / F32(mClipMapSize);

    const F32 tex1Left = F32(srcRect1.point.y) / F32(mClipMapSize);
    const F32 tex1Right = F32(srcRect1.point.y + srcRect1.extent.y) / F32(mClipMapSize);
    const F32 tex1Top = F32(srcRect1.point.x) / F32(mClipMapSize);
    const F32 tex1Bottom = F32(srcRect1.point.x + srcRect1.extent.x) / F32(mClipMapSize);

    const F32 screenLeft = dstRect.point.y;
    const F32 screenRight = dstRect.point.y + dstRect.extent.y;
    const F32 screenTop = dstRect.point.x;
    const F32 screenBottom = dstRect.point.x + dstRect.extent.x;

    const F32 fillConv = GFX->getFillConventionOffset();

    verts.lock();

    verts[0].point.set(screenLeft - fillConv, screenTop - fillConv, 0.f);
    verts[0].texCoord1.set(tex0Left, tex0Top);
    verts[0].texCoord2.set(tex1Left, tex1Top);
    verts[0].texCoord3.set(masterLeft, masterTop);

    verts[1].point.set(screenRight - fillConv, screenTop - fillConv, 0.f);
    verts[1].texCoord1.set(tex0Right, tex0Top);
    verts[1].texCoord2.set(tex1Right, tex1Top);
    verts[1].texCoord3.set(masterRight, masterTop);

    verts[2].point.set(screenLeft - fillConv, screenBottom - fillConv, 0.f);
    verts[2].texCoord1.set(tex0Left, tex0Bottom);
    verts[2].texCoord2.set(tex1Left, tex1Bottom);
    verts[2].texCoord3.set(masterLeft, masterBottom);

    verts[3].point.set(screenRight - fillConv, screenBottom - fillConv, 0.f);
    verts[3].texCoord1.set(tex0Right, tex0Bottom);
    verts[3].texCoord2.set(tex1Right, tex1Bottom);
    verts[3].texCoord3.set(masterRight, masterBottom);

    verts.unlock();

    GFX->setVertexBuffer(verts);
}

void AtlasClipMapImageCache_Blender::doRectUpdate(U32 mipLevel,
    AtlasClipMap::ClipStackEntry& cse,
    RectI srcRegion, RectI dstRegion)
{
    AssertFatal(pushDepth == 1, "BAD3");

    // We're getting a request in texels on the specified level...

    // So we don't adjust anything unless we are virtualizing data - ie, 
    // scaling an opacity or shadow map up to cover a level we don't have
    // native unscaled data for.

    // Thus, opacityScale is 0 unless the adjusted level is past the extent
    // of opacity data, in which case scaling will occur.

    // We need to take the size of this level, divide by chunk size, take
    // log2 to get the tree depth we want for this map. This allows us
    // to figure out what level to use based on resolution, thus being
    // free of tree depth restrictions.
    S32 levelSize = mClipMapSize * cse.mScale;
    S32 requiredOpacityDepth = getFastBinLog2(U32(levelSize / mOpacityTOC->getTextureChunkSize()));
    S32 opacityScale = 0;

    // Figure out what downscale (if any) we're working with...
    RectI scaledOpacitySrc = srcRegion;
    if (requiredOpacityDepth >= mOpacityTOC->getTreeDepth())
    {
        // We have to calculate the factor by which data will be scaled,
        // and we also have to convert our request into the appropriate
        // level.

        // Figure the delta we're adjusting new...
        S32 newLevel = mOpacityTOC->getTreeDepth() - 1;
        opacityScale = requiredOpacityDepth - newLevel;
        requiredOpacityDepth = newLevel;

        // Adjust stuff...
        U32 scaledLevelSize = levelSize >> opacityScale;
        convertToTOCRectWithChunkSize(1 << opacityScale, srcRegion, scaledOpacitySrc);

        // Now, if possible we want a 1tx border around the region we're using.
        // This allows us to avoid nasty seam issues.

        // Nudge out left...
        if (scaledOpacitySrc.point.x > 0)
        {
            scaledOpacitySrc.point.x--;
            scaledOpacitySrc.extent.x++;
        }

        // Nudge out right..
        if ((scaledOpacitySrc.extent.x + scaledOpacitySrc.point.x) < (scaledLevelSize - 1))
            scaledOpacitySrc.extent.x++;

        // Nudge top...
        if (scaledOpacitySrc.point.y > 0)
        {
            scaledOpacitySrc.point.y--;
            scaledOpacitySrc.extent.y++;
        }

        // Nudge bottom...
        if ((scaledOpacitySrc.extent.y + scaledOpacitySrc.point.y) < (scaledLevelSize - 1))
            scaledOpacitySrc.extent.y++;
    }

    // Now process the shadow map...
    S32 requiredShadowDepth = getFastBinLog2(U32(levelSize / mShadowTOC->getTextureChunkSize()));
    S32 shadowScale = 0;

    // Figure out what downscale (if any) we're working with...
    RectI scaledShadowSrc = srcRegion;
    if (requiredShadowDepth >= mShadowTOC->getTreeDepth())
    {
        // We have to calculate the factor by which data will be scaled,
        // and we also have to convert our request into the appropriate
        // level.

        // Figure the delta we're adjusting new...
        S32 newLevel = mShadowTOC->getTreeDepth() - 1;
        shadowScale = requiredShadowDepth - newLevel;
        requiredShadowDepth = newLevel;

        // Adjust stuff...
        U32 scaledLevelSize = levelSize >> shadowScale;
        convertToTOCRectWithChunkSize(1 << shadowScale, srcRegion, scaledShadowSrc);

        // Now, if possible we want a 1tx border around the region we're using.
        // This allows us to avoid nasty seam issues.

        // Nudge out left...
        if (scaledShadowSrc.point.x > 0)
        {
            scaledShadowSrc.point.x--;
            scaledShadowSrc.extent.x++;
        }

        // Nudge out right..
        if ((scaledShadowSrc.extent.x + scaledShadowSrc.point.x) < (scaledLevelSize - 1))
            scaledShadowSrc.extent.x++;

        // Nudge top...
        if (scaledShadowSrc.point.y > 0)
        {
            scaledShadowSrc.point.y--;
            scaledShadowSrc.extent.y++;
        }

        // Nudge bottom...
        if ((scaledShadowSrc.extent.y + scaledShadowSrc.point.y) < (scaledLevelSize - 1))
            scaledShadowSrc.extent.y++;
    }

    // Upload to a rect in the scratch texture.
    RectI localOpacityRect = scaledOpacitySrc;
    localOpacityRect.point.set(0, 0);
    localOpacityRect.extent.x = mClamp(localOpacityRect.extent.x, 1, mClipMapSize - 1) + 1;
    localOpacityRect.extent.y = mClamp(localOpacityRect.extent.y, 1, mClipMapSize - 1) + 1;

    RectI localShadowRect = scaledShadowSrc;
    localShadowRect.point.set(0, 0);
    localShadowRect.extent.x = mClamp(localShadowRect.extent.x, 1, mClipMapSize - 1) + 1;
    localShadowRect.extent.y = mClamp(localShadowRect.extent.y, 1, mClipMapSize - 1) + 1;

    GFXLockedRect* glr = mOpacityScratchTexture.lock(0, &localOpacityRect);
    blitFromTOC(mOpacityTOC, requiredOpacityDepth, scaledOpacitySrc, glr->bits, glr->pitch);
    mOpacityScratchTexture.unlock();

    glr = mShadowScratchTexture.lock(0, &localShadowRect);
    blitFromTOC(mShadowTOC, requiredShadowDepth, scaledShadowSrc, glr->bits, glr->pitch);
    mShadowScratchTexture.unlock();

    // We have to figure our source sampling to subtexel accuracy so we get nice
    // seams...
    F32 opacityScaleF32 = (1 << opacityScale);
    RectF accurateOpacityLocal;
    accurateOpacityLocal.point.x = F32(srcRegion.point.x) / opacityScaleF32 - F32(scaledOpacitySrc.point.x);
    accurateOpacityLocal.point.y = F32(srcRegion.point.y) / opacityScaleF32 - F32(scaledOpacitySrc.point.y);
    accurateOpacityLocal.extent.x = F32(srcRegion.extent.x) / opacityScaleF32;
    accurateOpacityLocal.extent.y = F32(srcRegion.extent.y) / opacityScaleF32;

    F32 shadowScaleF32 = (1 << shadowScale);
    RectF accurateShadowLocal;
    accurateShadowLocal.point.x = F32(srcRegion.point.x) / shadowScaleF32 - F32(scaledShadowSrc.point.x);
    accurateShadowLocal.point.y = F32(srcRegion.point.y) / shadowScaleF32 - F32(scaledShadowSrc.point.y);
    accurateShadowLocal.extent.x = F32(srcRegion.extent.x) / shadowScaleF32;
    accurateShadowLocal.extent.y = F32(srcRegion.extent.y) / shadowScaleF32;

    RectF masterRect;
    masterRect.point.x = F32(srcRegion.point.x) / F32(levelSize);
    masterRect.point.y = F32(srcRegion.point.y) / F32(levelSize);
    masterRect.extent.x = F32(srcRegion.extent.x) / F32(levelSize);
    masterRect.extent.y = F32(srcRegion.extent.y) / F32(levelSize);

    // Set up our geometry.
    setupGeometry(
        accurateOpacityLocal,
        accurateShadowLocal,
        masterRect,
        dstRegion);

    // And draw our blended quad...
    GFX->setTexture(0, mOpacityScratchTexture);
    GFX->setTexture(1, mShadowScratchTexture);

    for (S32 i = 0; i < 2; i++)
    {
        GFX->setTextureStageMagFilter(i, GFXTextureFilterPoint);
        GFX->setTextureStageMinFilter(i, GFXTextureFilterPoint);
        GFX->setTextureStageMipFilter(i, GFXTextureFilterPoint);

        // Work around an issue w/ state caching on device loss.
        GFX->updateStates();

        GFX->setTextureStageMagFilter(i, GFXTextureFilterLinear);
        GFX->setTextureStageMinFilter(i, GFXTextureFilterLinear);
        GFX->setTextureStageMipFilter(i, GFXTextureFilterLinear);
        GFX->setTextureStageAddressModeU(i, GFXAddressClamp);
        GFX->setTextureStageAddressModeV(i, GFXAddressClamp);
    }

    if (GFX->getPixelShaderVersion() < 2.0)
    {
        // Do two pass shader w/ additive blending.
        GFX->setShader(mTwoPass[0]);
        GFX->setAlphaBlendEnable(false);
        GFX->setTexture(2, (0 < mSourceImages.size()) ? *mSourceImages[0] : NULL);
        GFX->setTexture(3, (1 < mSourceImages.size()) ? *mSourceImages[1] : NULL);
        GFX->drawPrimitive(GFXTriangleStrip, 0, 2);

        GFX->setShader(mTwoPass[1]);
        GFX->setAlphaBlendEnable(true);
        GFX->setTexture(2, (2 < mSourceImages.size()) ? *mSourceImages[2] : NULL);
        GFX->setTexture(3, (3 < mSourceImages.size()) ? *mSourceImages[3] : NULL);
        GFX->drawPrimitive(GFXTriangleStrip, 0, 2);
    }
    else
    {
        // Do one pass.
        GFX->setShader(mOnePass);
        GFX->setAlphaBlendEnable(false);
        GFX->drawPrimitive(GFXTriangleStrip, 0, 2);
    }

    GFX->setVertexBuffer(NULL);
}

void AtlasClipMapImageCache_Blender::finishRectUpdates(AtlasClipMap::ClipStackEntry& cse)
{
    pushDepth--;
    AssertFatal(pushDepth == 0, "BAD2");

    // And restore the render target.
    GFX->popActiveRenderSurfaces();
    GFX->popActiveZSurface();

    // Extrude mips...
    ((GFXD3DTextureObject*)cse.mTex.getPointer())->get2DTex()->GenerateMipSubLevels();

    // Reset texture stages.
    // For sanity, let's just purge all our texture states.
    for (S32 i = 0; i < GFX->getNumSamplers(); i++)
        GFX->setTexture(i, NULL);
    GFX->updateStates();

    // And clear render states.
    GFX->setZEnable(true);
    GFX->setZWriteEnable(true);
    GFX->popWorldMatrix();
    GFX->setProjectionMatrix(proj);
    GFX->setViewport(viewPort);

    GFX->setAlphaBlendEnable(false);
    GFX->setAlphaTestEnable(false);

    //  Reset the shader modelview as well...
    MatrixF proj = GFX->getProjectionMatrix();
    proj.mul(GFX->getWorldMatrix());
    proj.transpose();
    GFX->setVertexShaderConstF(0, (float*)&proj, 4);

    // Purge state before we end the scene - just in case.
    GFX->updateStates();

    if (mustEndScene)
        GFX->endScene();
}

void AtlasClipMapImageCache_Blender::blitFromTOC(AtlasInstanceTexTOC* toc,
    U32 inMipLevel,
    RectI srcRegion, U8* bits, U32 pitch)
{
    // We want to write everything out in linear order, in order to have
    // (hopefully) have good memory characteristics, esp. if we're writing over
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

    S32 mipLevel = inMipLevel; //toc->getTreeDepth() - inMipLevel - 1;

    const S32 tocTextureSize = toc->getTextureChunkSize();

    RectI srcTOCs;
    convertToTOCRectWithChunkSize(tocTextureSize, srcRegion, srcTOCs);

    // Make sure we CAN bitblt this... must be all in range.
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
        cnotes[i].chunkColStart = S32(y * tocTextureSize);
        cnotes[i].chunkColEnd = S32((y + 1) * tocTextureSize);

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
        const S32 chunkRowStart = S32(x * tocTextureSize);
        const S32 chunkRowEnd = S32((x + 1) * tocTextureSize);

        const S32 rowStart = getMax(chunkRowStart, srcRegion.point.x);
        const S32 rowEnd = getMin(chunkRowEnd, srcRegion.point.x + srcRegion.extent.x);

        // Update chunk bitmap pointers.
        for (S32 i = 0; i < (bottom - top); i++)
        {
            const S32 y = i + top;
            cnotes[i].atc = toc->getResourceStub(toc->getStub(mipLevel, Point2I(x, y)))->mChunk;
            AssertFatal(cnotes[i].atc, avar("AtlasClipMapImageCache_Blender::blitFromTOC - missing chunk (%d,%d@%d)!", x, y, mipLevel));
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
                    continue;

                const U8* bmpBits = cnotes[i].atc->bitmap->getAddress(
                    cnotes[i].colStart - cnotes[i].chunkColStart,
                    row - chunkRowStart);

                if (cnotes[i].atc->bitmap->bytesPerPixel == 4)
                {
                    dMemcpy(outBits, bmpBits, (cnotes[i].colEnd - cnotes[i].colStart) << 2);
                }
                else if (cnotes[i].atc->bitmap->bytesPerPixel == 3)
                {
                    // Try to do a fast copy...
                    const U8* curPtr = bmpBits;
                    const U8* curEndPtr = bmpBits + (cnotes[i].colEnd - cnotes[i].colStart) * 3;
                    U8* outPtr = outBits;
                    const U8* outEndPtr = outBits + ((cnotes[i].colEnd - cnotes[i].colStart) << 2);

                    while (outPtr < outEndPtr)
                    {
                        *((U32*)outPtr) = *((U32*)curPtr) & 0xFFFFFF;

                        outPtr += 4;
                        curPtr += 3;
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