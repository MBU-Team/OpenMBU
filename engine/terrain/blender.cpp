//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "terrain/blender.h"
#include "gfx/primBuilder.h"
#include "gfx/gFont.h"
#include "gfx/gfxCanon.h"
#include "gfx/gfxTextureProfile.h"
#include "gfx/gfxTransformSaver.h"
#include "gui/core/guiTypes.h"
#include "console/consoleTypes.h"

GFXTexHandle Blender::mMipBuffer = NULL;
S32          Blender::mTexSize = 256;

bool gIsBlenderEnabled = false;

GFX_ImplementTextureProfile(TerrainBlenderOpacityMapProfile,
    GFXTextureProfile::DiffuseMap,
    GFXTextureProfile::PreserveSize | GFXTextureProfile::NoMipmap | GFXTextureProfile::KeepBitmap,
    GFXTextureProfile::None);

GFX_ImplementTextureProfile(TerrainBlenderTextureProfile,
    GFXTextureProfile::DiffuseMap,
    GFXTextureProfile::RenderTarget | GFXTextureProfile::PreserveSize,
    GFXTextureProfile::None);

Blender::Blender(TerrainBlock* owner)
    : mOwner(owner), mSources(), mOpacity()
{
    mSources.setSize(16);
    mOpacity.setSize(4);

    mMipBuffer = NULL;

    Blender::mTexSize = Con::getIntVariable("pref::Terrain::textureSize",
        128 >> GFX->mTextureManager->getBitmapScalePower(&TerrainBlenderTextureProfile));
}

Blender::~Blender()
{
    // Clean stuff up.
    mSources.setSize(0);
#ifdef BLENDER_DEBUG_TEXTURES
    f = NULL;
#endif

    mMipBuffer = NULL;

    for (U32 i = 0; i < mOpacity.size(); i++)
        mOpacity[i] = NULL;
}

void Blender::zombify()
{
    mMipBuffer = NULL;
}

void Blender::resurrect()
{
}

void Blender::updateOpacity()
{
    // Go through and "cook" all our opacity data into a single combined
    // set of opacity maps. We can fit 4 maps/texture, so we maintain an
    // array of textures.

    // Get rid of old maps
    mOpacity.setSize(mSources.size() / 4);

    // Build new maps
    for (U32 rangeStart = 0; rangeStart < mSources.size(); rangeStart += 4)
    {
        // If we have a null on source #rangeStart, we are done.
        if (!(mSources[rangeStart + 0].alpha)) break;

        GBitmap* tempMap = new GBitmap(TerrainBlock::BlockSize, TerrainBlock::BlockSize, 0, GFXFormatR8G8B8A8);
        U8* tmpBits = (U8*)tempMap->getWritableBits();

        // Copy everything relevant into the texture...
        for (U32 x = 0; x < TerrainBlock::BlockSize * TerrainBlock::BlockSize; x++)
        {
            // We assume always the first, not always the second...
            // Rewrite me - BJG
            tmpBits[0] = mSources[rangeStart + 0].alpha[x];
            if (mSources[rangeStart + 1].alpha && mSources.size() > rangeStart + 1)
            {
                tmpBits[1] = mSources[rangeStart + 1].alpha[x];
                if (mSources[rangeStart + 2].alpha && mSources.size() > rangeStart + 2)
                {
                    tmpBits[2] = mSources[rangeStart + 2].alpha[x];
                    if (mSources[rangeStart + 3].alpha && mSources.size() > rangeStart + 3)
                    {
                        tmpBits[3] = mSources[rangeStart + 3].alpha[x];
                    }
                    else
                        tmpBits[3] = 0;
                }
                else
                {
                    tmpBits[2] = 0;
                    tmpBits[3] = 0;
                }
            }
            else
            {
                tmpBits[1] = 0;
                tmpBits[2] = 0;
                tmpBits[3] = 0;
            }

            // Move to next chunk of data...
            tmpBits += 4;
        }

        // Make a texture and store it...
        mOpacity[rangeStart / 4].set(tempMap, &TerrainBlenderOpacityMapProfile, true);
    }
}

void Blender::addSourceTexture(U32 materialIdx, GFXTexHandle bmp, U8* alphas)
{
    // Get a new SourceTexture
    SourceTexture s;

    // Put the alphas into a texture
    s.alpha = alphas;

    // Assign other values
    s.source = bmp;

    // Store it in the appropriate slot
    mSources[materialIdx] = s;
}

void renderPass(F32 a, F32 b, F32 c, F32 d, F32 copyOffset)
{
    PrimBuild::color4f(1, 1, 1, 1);
    PrimBuild::begin(GFXTriangleFan, 4);
    PrimBuild::texCoord2f(a, d);
    PrimBuild::vertex3f(-1.0 - copyOffset, -1.0 + copyOffset, 0.0);

    PrimBuild::texCoord2f(a, b);
    PrimBuild::vertex3f(-1.0 - copyOffset, 1.0 + copyOffset, 0.0);

    PrimBuild::texCoord2f(c, b);
    PrimBuild::vertex3f(1.0 - copyOffset, 1.0 + copyOffset, 0.0);

    PrimBuild::texCoord2f(c, d);
    PrimBuild::vertex3f(1.0 - copyOffset, -1.0 + copyOffset, 0.0);
    PrimBuild::end();
}

GFXTexHandle Blender::blend(S32 x, S32 y, S32 level, GFXTexHandle lightmap, GFXTexHandle oldSurface)
{
    //GFX_Canonizer("Blender::blend", __FILE__, __LINE__);

    // Automagically save & restore our viewport and transforms.
    GFXTransformSaver saver;

    // Figure out sizes
    U32 texSize = mTexSize;

    F32 copyOffset = 1 / (F32)texSize;

    // Set up our buffer if needed
    if (mMipBuffer.isNull() || mMipBuffer->getWidth() != texSize)
    {
        mMipBuffer.set(texSize, texSize, GFXFormatR8G8B8X8, &TerrainBlenderTextureProfile);
    }

    // Allocate a new texture to render to, if we need it...
    GFXTexHandle renderSurface;

    if (oldSurface.isNull() || oldSurface->getWidth() != texSize)
        renderSurface.set(texSize, texSize, GFXFormatR8G8B8X8, &TerrainBlenderTextureProfile, 6);
    else
        renderSurface = oldSurface;

    // Figure out texture co-ordinates
    F32 a, b, c, d;

    // Find starting points on terrain
    F32 nx, ny;
    nx = x % TerrainBlock::BlockSize;
    ny = y % TerrainBlock::BlockSize;

    // Figure out the size of the block we're rendering...
    F32 blockSpan = 1 << level;

    // Top left
    a = nx / (F32)TerrainBlock::BlockSize;
    b = ny / (F32)TerrainBlock::BlockSize;

    // Bottom right (with a weird and random offset)
    c = (nx + blockSpan) / (F32)TerrainBlock::BlockSize;
    d = (ny + blockSpan) / (F32)TerrainBlock::BlockSize;

    // Initialize transforms
    MatrixF identity(true);

    GFX->setProjectionMatrix(identity);
    GFX->setWorldMatrix(identity);


    // Disable Z and stencil
    GFX->setBaseRenderState(); // Down with the red menace -- BJG
    GFX->setStencilEnable(false);
    GFX->setZEnable(false);
    GFX->setZWriteEnable(false);

    // Prepare material

    // for PS2.0 path
    MatInstance passMat("TerrainBlenderPS20Material");

    // for PS1.1 path
    MatInstance passMatA("TerrainBlenderPS11AMaterial");
    MatInstance passMatB("TerrainBlenderPS11BMaterial");

    bool isPS20 = (GFX->getPixelShaderVersion() >= 2.0);

    SceneGraphData sgData;
    sgData.objTrans.identity();

    GFX->pushActiveRenderSurfaces();
    {
        GFX->setActiveRenderSurface(renderSurface);
        GFX->clear(GFXClearTarget, ColorI(0, 0, 0), 0.0f, 0);

        // Render the opacity map stuff
        for (U32 i = 0; i < 6; i++)
        {
            GFX->setTextureStageColorOp(i, GFXTOPModulate);
            GFX->setTextureStageAddressModeU(i, GFXAddressWrap);
            GFX->setTextureStageAddressModeV(i, GFXAddressWrap);
            GFX->setTextureStageMipFilter(i, GFXTextureFilterLinear);
            GFX->setTextureStageMinFilter(i, GFXTextureFilterLinear);
            GFX->setTextureStageMagFilter(i, GFXTextureFilterLinear);
        }


        for (U32 i = 0; i < mSources.size(); i += 4)
        {
            // Break if there's nothing to draw
            if (mSources[i + 0].source.isNull() &&
                mSources[i + 1].source.isNull() &&
                mSources[i + 2].source.isNull() &&
                mSources[i + 3].source.isNull())
                break;

            // Load up the various things we're blending...
            // If we're taking the old path then do it two at a time.

            if (isPS20)
            {
                passMat.init(sgData, (GFXVertexFlags)getGFXVertFlags((GFXVertexPCT*)NULL));

                // Load up the opacity map...
                GFX->setTexture(0, mOpacity[i / 4]);

                GFX->setTexture(1, mSources[i + 0].source);
                GFX->setTexture(2, mSources[i + 1].source);
                GFX->setTexture(3, mSources[i + 2].source);
                GFX->setTexture(4, mSources[i + 3].source);

                // Load up lightmap...
                GFX->setTexture(5, lightmap);

                while (passMat.setupPass(sgData))
                {
                    // Enable blending (cuz the material will blast our settings)...
                    GFX->setAlphaBlendEnable(true);
                    GFX->setSrcBlend(GFXBlendOne);
                    GFX->setDestBlend(GFXBlendOne);

                    // Render a pass
                    renderPass(a, b, c, d, copyOffset);
                }
            }
            else
            {
                passMatA.init(sgData, (GFXVertexFlags)getGFXVertFlags((GFXVertexPCT*)NULL));

                // Load up the opacity map...
                GFX->setTexture(0, mOpacity[i / 4]);

                GFX->setTexture(1, mSources[i + 0].source);
                GFX->setTexture(2, mSources[i + 1].source);

                // Load up lightmap...
                GFX->setTexture(3, lightmap);

                while (passMatA.setupPass(sgData))
                {
                    // Enable blending (cuz the material will blast our settings)...
                    GFX->setAlphaBlendEnable(true);
                    GFX->setSrcBlend(GFXBlendOne);
                    GFX->setDestBlend(GFXBlendOne);

                    // Render a pass
                    renderPass(a, b, c, d, copyOffset);
                }

                passMatB.init(sgData, (GFXVertexFlags)getGFXVertFlags((GFXVertexPCT*)NULL));

                // Load up the opacity map...
                GFX->setTexture(0, mOpacity[i / 4]);

                GFX->setTexture(1, mSources[i + 2].source);
                GFX->setTexture(2, mSources[i + 3].source);

                // Load up lightmap...
                GFX->setTexture(3, lightmap);

                while (passMatB.setupPass(sgData))
                {
                    // Enable blending (cuz the material will blast our settings)...
                    GFX->setAlphaBlendEnable(true);
                    GFX->setSrcBlend(GFXBlendOne);
                    GFX->setDestBlend(GFXBlendOne);

                    // Render a pass
                    renderPass(a, b, c, d, copyOffset);
                }

            }

        }
    }

    // Now we gotta make us some mips.

    // (yee hah!)
    GFX->setTextureStageColorOp(0, GFXTOPModulate);
    GFX->setTextureStageColorOp(1, GFXTOPDisable);
    GFX->setAlphaBlendEnable(false);
    GFX->disableShaders();

    GFX->setTextureStageAddressModeU(0, GFXAddressClamp);
    GFX->setTextureStageAddressModeV(0, GFXAddressClamp);
    GFX->setTexture(0, renderSurface);

    // Copy into the mip buffer
    GFX->setActiveRenderSurface(mMipBuffer);
    {
        GFX->clear(GFXClearTarget, ColorI(0, 255, 0), 0.0f, 0);
        renderPass(0, 0, 1, 1, copyOffset);
    }


    // Now do some mips
    GFX->setTexture(0, mMipBuffer);

    for (U32 i = 1; i < renderSurface->mMipLevels; i++)
    {
        GFX->setActiveRenderSurface(renderSurface, 0, i);
        {
            GFX->clear(GFXClearTarget, ColorI(20 * i, 0, 0), 0.0f, 0);
            renderPass(0, 0, 1, 1, 1 / (texSize >> (i - 1)));
        }

    }

    GFX->popActiveRenderSurfaces();

    // Restore states
    GFX->setStencilEnable(false);
    GFX->setZEnable(true);
    GFX->setZWriteEnable(true);
    GFX->setBaseRenderState();
    GFX->setTextureStageAddressModeU(0, GFXAddressWrap);
    GFX->setTextureStageAddressModeV(0, GFXAddressWrap);

    // Return the surface
    return renderSurface;
}
