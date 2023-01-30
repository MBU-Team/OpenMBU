//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------
#include "renderMeshMgr.h"

#include "gfx/gfxTransformSaver.h"
#include "materials/sceneData.h"
#include "sceneGraph/sceneGraph.h"
#include "materials/matInstance.h"
#include "../../game/shaders/shdrConsts.h"


//**************************************************************************
// RenderMeshMgr
//**************************************************************************

//-----------------------------------------------------------------------------
// render
//-----------------------------------------------------------------------------
void RenderMeshMgr::render()
{
    // Early out if nothing to draw.
    if (!mElementList.size())
        return;

    PROFILE_START(RenderMeshMgrRender);

    // Automagically save & restore our viewport and transforms.
    GFXTransformSaver saver;

    // set render states
    if (getCurrentClientSceneGraph()->isReflectPass())
    {
        GFX->setCullMode(GFXCullCW);
    }
    else
    {
        GFX->setCullMode(GFXCullCCW);
    }

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

        setupSGData(ri, sgData);
        MatInstance* mat = ri->matInst;

        // .ifl?
        if (!mat && !ri->particles)
        {
            GFX->setTextureStageColorOp(0, GFXTOPModulate);
            GFX->setTextureStageColorOp(1, GFXTOPDisable);

            GFX->pushWorldMatrix();
            GFX->setWorldMatrix(*ri->worldXform);

            GFX->setTexture(0, ri->miscTex);
            GFX->setPrimitiveBuffer(*ri->primBuff);
            GFX->setVertexBuffer(*ri->vertBuff);
            GFX->disableShaders();
            GFX->setupGenericShaders(GFXDevice::GSModColorTexture);
            GFX->drawPrimitive(ri->primBuffIndex);

            GFX->popWorldMatrix();

            continue;
        }

        if (!mat)
        {
            mat = gRenderInstManager.getWarningMat();
        }

        bool firstmatpass = true;
        while (mat && mat->setupPass(sgData))
        {
            if (newPassNeeded(mat, ri))
                break;
            
            // no dynamics if glowing...
            RenderPassData *passdata = mat->getPass(mat->getCurPass());
            if(passdata && passdata->glow && ri->dynamicLight)
                continue;
            
            // don't break the material multipass rendering...
            if (firstmatpass)
            {
                if (ri->primitiveFirstPass)
                {
                    bool& firstpass = *ri->primitiveFirstPass;
                    if (firstpass)
                    {
                        GFX->setAlphaBlendEnable(false);
                        GFX->setSrcBlend(GFXBlendOne);
                        GFX->setDestBlend(GFXBlendZero);
                        firstpass = false;
                    }
                    else
                    {
                        AssertFatal((ri->light.mType != LightInfo::Vector), "Not good");
                        AssertFatal((ri->light.mType != LightInfo::Ambient), "Not good");
                        mat->setLightingBlendFunc();
                    }
                }
                else
                {
                    GFX->setAlphaBlendEnable(false);
                    GFX->setSrcBlend(GFXBlendOne);
                    GFX->setDestBlend(GFXBlendZero);
                }
            }
            
            mat->setWorldXForm(*ri->worldXform);
            //mat->setObjectXForm(*ri->objXform);
            //setupSGData(ri, sgData);
            //sgData.matIsInited = true;
            //mat->setLightInfo(sgData);
            //mat->setEyePosition(*ri->objXform, gRenderInstManager.getCamPos());
            mat->setBuffers(ri->vertBuff, ri->primBuff);
            
            // draw it
            GFX->drawPrimitive(ri->primBuffIndex);

            firstmatpass = false;
        }
    }

    GFX->setLightingEnable(false);

    PROFILE_END();
}
