//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------
#include "RenderTranslucentMgr.h"
#include "materials/sceneData.h"
#include "sceneGraph/sceneGraph.h"
#include "materials/matInstance.h"
#include "../../game/shaders/shdrConsts.h"

#define HIGH_NUM ((U32(-1)/2) - 1)

//**************************************************************************
// RenderTranslucentMgr
//**************************************************************************

//-----------------------------------------------------------------------------
// setup scenegraph data
//-----------------------------------------------------------------------------
void RenderTranslucentMgr::setupSGData(RenderInst* ri, SceneGraphData& data)
{
    dMemset(&data, 0, sizeof(SceneGraphData));

    // do this here for the init, but also in setupLights...
    dMemcpy(&data.light, &ri->light, sizeof(ri->light));
    dMemcpy(&data.lightSecondary, &ri->lightSecondary, sizeof(ri->lightSecondary));
    data.dynamicLight = ri->dynamicLight;
    data.dynamicLightSecondary = ri->dynamicLightSecondary;

    data.camPos = gRenderInstManager.getCamPos();
    data.objTrans = *ri->objXform;
    //   data.backBuffTex = *ri->backBuffTex;
    //   data.cubemap = ri->cubemap;

    data.useFog = true;
    data.fogTex = getCurrentClientSceneGraph()->getFogTexture();
    data.fogHeightOffset = getCurrentClientSceneGraph()->getFogHeightOffset();
    data.fogInvHeightRange = getCurrentClientSceneGraph()->getFogInvHeightRange();
    data.visDist = getCurrentClientSceneGraph()->getVisibleDistanceMod();

    data.visibility = ri->visibility;
}

//-----------------------------------------------------------------------------
// add element
//-----------------------------------------------------------------------------
void RenderTranslucentMgr::addElement(RenderInst* inst)
{
    mElementList.increment();
    MainSortElem& elem = mElementList.last();
    elem.inst = inst;
    //   elem.key = elem.key2 = 0;

       // sort by distance
    elem.key = (gRenderInstManager.getCamPos() - inst->sortPoint).lenSquared() * 2500.0;
    elem.key = HIGH_NUM - elem.key;
    elem.key2 = (U32)inst->matInst;
}

//-----------------------------------------------------------------------------
// render
//-----------------------------------------------------------------------------
void RenderTranslucentMgr::render()
{
    // Early out if nothing to draw.
    if (mElementList.empty())
        return;

    PROFILE_START(RenderTranslucentMgrRender);

    GFX->pushWorldMatrix();

    SceneGraphData sgData;

    GFX->setCullMode(GFXCullNone);

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


    GFX->setZWriteEnable(false);

    GFXVertexBuffer* lastVB = NULL;
    GFXPrimitiveBuffer* lastPB = NULL;

    U32 numChanges = 0;
    U32 numDrawCalls = 0;

    U32 binSize = mElementList.size();
    for (U32 j = 0; j < binSize; )
    {
        RenderInst* ri = mElementList[j].inst;
        MatInstance* mat = ri->matInst;

        U32 matListEnd = j;

        // set culling
        if (!mat || mat->getMaterial()->doubleSided)
        {
            GFX->setCullMode(GFXCullNone);
        }
        else
        {
            GFX->setCullMode(GFXCullCCW);
        }

        // render these separately...
        if (ri->type == RenderInstManager::RIT_ObjectTranslucent)
        {
            ri->obj->renderObject(ri->state, ri);

            GFX->setCullMode(GFXCullNone);
            GFX->setTextureStageAlphaOp(0, GFXTOPModulate);
            GFX->setTextureStageAlphaOp(1, GFXTOPDisable);
            GFX->setTextureStageAlphaArg1(0, GFXTATexture);
            GFX->setTextureStageAlphaArg2(0, GFXTADiffuse);
            GFX->setTextureStageColorOp(0, GFXTOPModulate);
            GFX->setTextureStageColorOp(1, GFXTOPDisable);
            GFX->setAlphaTestEnable(true);
            GFX->setAlphaRef(1);
            GFX->setAlphaFunc(GFXCmpGreaterEqual);
            GFX->setZWriteEnable(false);
            lastVB = NULL;
            lastPB = NULL;
            j++;
            continue;
        }

        // handle particles
        if (ri->particles)
        {
            GFX->setCullMode(GFXCullNone);
            GFX->setTextureStageColorOp(0, GFXTOPModulate);
            GFX->setTextureStageColorOp(1, GFXTOPDisable);
            GFX->setZWriteEnable(false);

            GFX->setAlphaBlendEnable(true);
            GFX->setSrcBlend(GFXBlendSrcAlpha);

            if (ri->transFlags)
                GFX->setDestBlend(GFXBlendInvSrcAlpha);
            else
                GFX->setDestBlend(GFXBlendOne);

            GFX->pushWorldMatrix();
            GFX->setWorldMatrix(*ri->worldXform);


            GFX->setTexture(0, ri->miscTex);
            GFX->setPrimitiveBuffer(*ri->primBuff);
            GFX->setVertexBuffer(*ri->vertBuff);
            GFX->disableShaders();
            GFX->setupGenericShaders(GFXDevice::GSModColorTexture);
            GFX->drawIndexedPrimitive(GFXTriangleList, 0, ri->primBuffIndex * 4, 0, ri->primBuffIndex * 2);

            GFX->popWorldMatrix();

            lastVB = NULL;    // no longer valid, null it
            lastPB = NULL;    // no longer valid, null it

            j++;
            continue;
        }

        // .ifl?
        if (!mat && !ri->particles)
        {
            GFX->setTextureStageColorOp(0, GFXTOPModulate);
            GFX->setTextureStageColorOp(1, GFXTOPDisable);

            if (ri->translucent)
            {
                GFX->setAlphaBlendEnable(true);
                GFX->setSrcBlend(GFXBlendSrcAlpha);

                if (ri->transFlags)
                    GFX->setDestBlend(GFXBlendInvSrcAlpha);
                else
                    GFX->setDestBlend(GFXBlendOne);
            }

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

        setupSGData(ri, sgData);

        GFX->setZWriteEnable(false);
        bool firstmatpass = true;
        while (mat->setupPass(sgData))
        {
            ++numChanges;

            U32 a;
            for (a = j; a < binSize; a++)
            {
                RenderInst* passRI = mElementList[a].inst;
                
                // if new matInst is null or different, break
                if (newPassNeeded(mat, passRI))
                    break;

                // z sorting and stuff is not working in this mgr...
                // lighting on hold until this gets fixed...
                // don't break the material multipass rendering...
                if (firstmatpass)
                {
                    if (passRI->primitiveFirstPass)
                    {
                        bool& firstpass = *passRI->primitiveFirstPass;
                        if (!firstpass)
                        {
                            GFX->setAlphaBlendEnable(true);
                            GFX->setSrcBlend(GFXBlendOne);
                            GFX->setDestBlend(GFXBlendOne);
                        }
                        firstpass = false;
                    }
                }

                setupSGData(passRI, sgData);

                setupLights(passRI, sgData);


                GFX->setVertexShaderConstF(0, (float*)passRI->worldXform, 4);

                MatrixF objTrans = *passRI->objXform;
                objTrans.transpose();
                GFX->setVertexShaderConstF(VC_OBJ_TRANS, (float*)&objTrans, 4);
                objTrans.transpose();
                objTrans.inverse();

                Point3F eyePos = gRenderInstManager.getCamPos();
                objTrans.mulP(eyePos);
                GFX->setVertexShaderConstF(VC_EYE_POS, (float*)&eyePos, 1);

                //Point3F lightDir = passRI->lightDir;
                //objTrans.mulV( lightDir );
                //GFX->setVertexShaderConstF( VC_LIGHT_DIR1, (float*)&lightDir, 1 );


                // fill in cubemap data
                //-------------------------
                if (mat->hasCubemap())
                {
                    Point3F cubeEyePos = gRenderInstManager.getCamPos() - passRI->objXform->getPosition();
                    GFX->setVertexShaderConstF(VC_CUBE_EYE_POS, (float*)&cubeEyePos, 1);

                    MatrixF cubeTrans = *passRI->objXform;
                    cubeTrans.setPosition(Point3F(0.0, 0.0, 0.0));
                    cubeTrans.transpose();
                    GFX->setVertexShaderConstF(VC_CUBE_TRANS, (float*)&cubeTrans, 3);
                }

                // set buffers
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

                ++numDrawCalls;
            }

            matListEnd = a;
            firstmatpass = false;
        }

        // force increment if none happened, otherwise go to end of batch
        j = (j == matListEnd) ? j + 1 : matListEnd;

    }

    GFX->setZEnable(true);
    GFX->setZWriteEnable(true);
    GFX->setAlphaTestEnable(false);
    GFX->setAlphaBlendEnable(false);

    GFX->popWorldMatrix();

    PROFILE_END();
}
