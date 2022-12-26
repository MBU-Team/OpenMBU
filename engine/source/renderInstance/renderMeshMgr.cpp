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

    for (U32 j = 0; j < binSize; )
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

            j++;
            continue;
        }

        if (!mat)
        {
            mat = gRenderInstManager.getWarningMat();
        }
        U32 matListEnd = j;

        bool firstmatpass = true;
        while (mat && mat->setupPass(sgData))
        {
            U32 a;
            for (a = j; a < binSize; a++)
            {
                RenderInst* passRI = mElementList[a].inst;

                if (newPassNeeded(mat, passRI))
                    break;

                // no dynamics if glowing...
                RenderPassData *passdata = mat->getPass(mat->getCurPass());
                if(passdata && passdata->glow && passRI->dynamicLight)
                    continue;

                // don't break the material multipass rendering...
                if (firstmatpass)
                {
                    if (passRI->primitiveFirstPass)
                    {
                        bool& firstpass = *passRI->primitiveFirstPass;
                        if (firstpass)
                        {
                            GFX->setAlphaBlendEnable(false);
                            GFX->setSrcBlend(GFXBlendOne);
                            GFX->setDestBlend(GFXBlendZero);
                            firstpass = false;
                        }
                        else
                        {
                            //AssertFatal((passRI->light.mType != LightInfo::Vector), "Not good");
                            //AssertFatal((passRI->light.mType != LightInfo::Ambient), "Not good");
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

                mat->setWorldXForm(*passRI->worldXform);
                mat->setObjectXForm(*passRI->objXform);
                setupSGData(passRI, sgData);
                sgData.matIsInited = true;
                mat->setLightInfo(sgData);
                mat->setEyePosition(*passRI->objXform, gRenderInstManager.getCamPos());
                mat->setBuffers(passRI->vertBuff, passRI->primBuff);

                // draw it
                GFX->drawPrimitive(passRI->primBuffIndex);
            }

            matListEnd = a;
            firstmatpass = false;
        }

        // force increment if none happened, otherwise go to end of batch
        j = (j == matListEnd) ? j + 1 : matListEnd;
    }

    GFX->setLightingEnable(false);

    PROFILE_END();
}
