//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------
#include "renderGlowMgr.h"
#include "materials/sceneData.h"
#include "sceneGraph/sceneGraph.h"
#include "materials/matInstance.h"
#include "../../game/shaders/shdrConsts.h"


//**************************************************************************
// RenderGlowMgr
//**************************************************************************

//-----------------------------------------------------------------------------
// setup scenegraph data
//-----------------------------------------------------------------------------
void RenderGlowMgr::setupSGData(RenderInst* ri, SceneGraphData& data)
{
    Parent::setupSGData(ri, data);
    data.glowPass = true;
}


//-----------------------------------------------------------------------------
// render
//-----------------------------------------------------------------------------
void RenderGlowMgr::render()
{
#ifdef _XBOX
    return; // HACK -patw
#endif

   // Early out if nothing to draw.
    if (mElementList.empty())
        return;

    // CodeReview - This is pretty hackish. - AlexS 4/19/07
    if (GFX->getPixelShaderVersion() < 0.001f)
        return;

    GlowBuffer* glowBuffer = getCurrentClientSceneGraph()->getGlowBuff();
    if (!glowBuffer || glowBuffer->isDisabled() || mElementList.empty()) return;

    PROFILE_START(RenderGlowMgrRender);

    RectI vp = GFX->getViewport();

    GFX->pushActiveRenderTarget();
    glowBuffer->setAsRenderTarget();

    GFX->pushWorldMatrix();

    // set up register combiners
    GFX->setTextureStageAlphaOp(0, GFXTOPModulate);
    GFX->setTextureStageAlphaOp(1, GFXTOPDisable);
    GFX->setTextureStageAlphaArg1(0, GFXTATexture);
    GFX->setTextureStageAlphaArg2(0, GFXTADiffuse);

    GFX->setTextureStageColorOp(0, GFXTOPModulate);
    GFX->setTextureStageColorOp(1, GFXTOPDisable);

    // turn on alpha test
    GFX->setAlphaTestEnable(true);
    GFX->setAlphaRef(1);
    GFX->setAlphaFunc(GFXCmpGreaterEqual);

    GFX->setAlphaBlendEnable(true);

    // set render states
    GFX->setCullMode(GFXCullCCW);
    GFX->setZWriteEnable(false);

    for (U32 i = 0; i < TEXTURE_STAGE_COUNT; i++)
    {
        GFX->setTextureStageAddressModeU(i, GFXAddressWrap);
        GFX->setTextureStageAddressModeV(i, GFXAddressWrap);

        GFX->setTextureStageMagFilter(i, GFXTextureFilterLinear);
        GFX->setTextureStageMinFilter(i, GFXTextureFilterLinear);
        GFX->setTextureStageMipFilter(i, GFXTextureFilterLinear);
    }

    // init loop data
    SceneGraphData sgData;
    U32 binSize = mElementList.size();

    for (U32 j = 0; j < binSize; j++)
    {
        RenderInst* ri = mElementList[j].inst;

        // temp fix - these shouldn't submit glow ri's...
        if (ri->dynamicLight)
        {
            continue;
        }

        setupSGData(ri, sgData);
        MatInstance* mat = ri->matInst;
        if (!mat)
        {
            mat = gRenderInstManager.getWarningMat();
        }

        while (mat->setupPass(sgData))
        {
            RenderInst* passRI = mElementList[j].inst;

            if (newPassNeeded(mat, passRI))
                break;

            mat->setWorldXForm(*passRI->worldXform);
            //mat->setObjectXForm(*passRI->objXform);
            //mat->setEyePosition(*passRI->objXform, gRenderInstManager.getCamPos());
            mat->setBuffers(passRI->vertBuff, passRI->primBuff);

            if (passRI->primBuff)
                GFX->drawPrimitive(passRI->primBuffIndex); // draw it
        }
    }

    // restore render states, copy to screen
    GFX->setZWriteEnable(true);
    GFX->popActiveRenderTarget();
    glowBuffer->copyToScreen(vp);

    GFX->popWorldMatrix();

    PROFILE_END();
}
