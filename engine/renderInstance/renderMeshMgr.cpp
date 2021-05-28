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
// setup scenegraph data
//-----------------------------------------------------------------------------
void RenderMeshMgr::setupSGData(RenderInst* ri, SceneGraphData& data)
{
    dMemset(&data, 0, sizeof(SceneGraphData));

    // do this here for the init, but also in setupLights...
    dMemcpy(&data.light, &ri->light, sizeof(ri->light));
    dMemcpy(&data.lightSecondary, &ri->lightSecondary, sizeof(ri->lightSecondary));
    data.dynamicLight = ri->dynamicLight;
    data.dynamicLightSecondary = ri->dynamicLightSecondary;

    data.camPos = gRenderInstManager.getCamPos();
    data.objTrans = *ri->objXform;
    data.backBuffTex = *ri->backBuffTex;
    data.cubemap = ri->cubemap;

    data.useFog = true;
    data.fogTex = getCurrentClientSceneGraph()->getFogTexture();
    data.fogHeightOffset = getCurrentClientSceneGraph()->getFogHeightOffset();
    data.fogInvHeightRange = getCurrentClientSceneGraph()->getFogInvHeightRange();
    data.visDist = getCurrentClientSceneGraph()->getVisibleDistanceMod();

    if (ri->lightmap)
    {
        data.lightmap = *ri->lightmap;
    }
    if (ri->normLightmap)
    {
        data.normLightmap = *ri->normLightmap;
    }

    data.visibility = ri->visibility;
}


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
    GFXVertexBuffer* lastVB = NULL;
    GFXPrimitiveBuffer* lastPB = NULL;
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

            lastVB = NULL;    // no longer valid, null it
            lastPB = NULL;    // no longer valid, null it

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

                if (mat != passRI->matInst)
                {
                    break;
                }

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
                            AssertFatal((passRI->light.mType != LightInfo::Vector), "Not good");
                            AssertFatal((passRI->light.mType != LightInfo::Ambient), "Not good");
                            GFX->setAlphaBlendEnable(true);
                            GFX->setSrcBlend(GFXBlendSrcAlpha);
                            //GFX->setSrcBlend(GFXBlendOne);
                            GFX->setDestBlend(GFXBlendOne);
                            //continue;
                        }
                    }
                    else
                    {
                        GFX->setAlphaBlendEnable(false);
                        GFX->setSrcBlend(GFXBlendOne);
                        GFX->setDestBlend(GFXBlendZero);
                    }
                }

                setupLights(passRI, sgData);


                // fill in shader constants that change per draw
                //-----------------------------------------------
                GFX->setVertexShaderConstF(0, (float*)passRI->worldXform, 4);

                // set object transform
                MatrixF objTrans = *passRI->objXform;
                objTrans.transpose();
                GFX->setVertexShaderConstF(VC_OBJ_TRANS, (float*)&objTrans, 4);
                objTrans.transpose();
                objTrans.inverse();

                // fill in eye data
                Point3F eyePos = gRenderInstManager.getCamPos();
                objTrans.mulP(eyePos);
                GFX->setVertexShaderConstF(VC_EYE_POS, (float*)&eyePos, 1);


                // fill in cubemap data
                if (mat->hasCubemap())
                {
                    Point3F cubeEyePos = gRenderInstManager.getCamPos() - passRI->objXform->getPosition();
                    GFX->setVertexShaderConstF(VC_CUBE_EYE_POS, (float*)&cubeEyePos, 1);

                    MatrixF cubeTrans = *passRI->objXform;
                    cubeTrans.setPosition(Point3F(0.0, 0.0, 0.0));
                    cubeTrans.transpose();
                    GFX->setVertexShaderConstF(VC_CUBE_TRANS, (float*)&cubeTrans, 3);
                }

                // set buffers if changed
                if (lastVB != passRI->vertBuff->getPointer())
                {
                    GFX->setVertexBuffer(*passRI->vertBuff);
                    lastVB = passRI->vertBuff->getPointer();
                }
                if (lastPB != passRI->primBuff->getPointer())
                {
                    GFX->setPrimitiveBuffer(*passRI->primBuff);
                    lastPB = passRI->primBuff->getPointer();
                }

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
