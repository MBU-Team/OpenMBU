//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------
#include "renderTranslucentMgr.h"
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
    Parent::setupSGData(ri, data);

    data.backBuffTex = NULL;
    data.cubemap = NULL;
    data.lightmap = NULL;
    data.normLightmap = NULL;
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
    //elem.key = HIGH_NUM - U64((gRenderInstManager.getCamPos() - inst->sortPoint).lenSquared() * 2500.0);

    // sort by distance (the multiply is there to prevent us from losing information when converting to a U32)
    F32 camDist = (gRenderInstManager.getCamPos() - inst->sortPoint).len();
    elem.key = *((U32*)&camDist);

    //F32 camDist = HIGH_NUM - (gRenderInstManager.getCamPos() - inst->sortPoint).lenSquared() * 2500.0f;
    //elem.key = *((U32*)&camDist);

    // we want key2 to sort by Material, but if the matInst is null, we can't.
    // in that case, use the "miscTex" for the secondary key

    // TODO: might break under 64 bit
    if (inst->matInst == NULL)
        elem.key2 = static_cast<U32>(reinterpret_cast<size_t>(inst->miscTex));
    else
        elem.key2 = static_cast<U32>(reinterpret_cast<size_t>(inst->matInst->getMaterial()));
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

    //GFX->setCullMode(GFXCullNone);

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

    GFX->setAlphaBlendEnable( true );

    GFX->setZWriteEnable(false);

    //U32 numChanges = 0;
    //U32 numDrawCalls = 0;

    U32 binSize = mElementList.size();
    for (U32 j = 0; j < binSize; j++)
    {
        RenderInst* ri = mElementList[j].inst;
        MatInstance* mat = ri->matInst;

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

            continue;
        }

        // .ifl?
        if (!mat && !ri->particles)
        {
            GFX->setTextureStageColorOp(0, GFXTOPModulate);
            GFX->setTextureStageColorOp(1, GFXTOPDisable);
            GFX->setZWriteEnable( false );

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

            continue;
        }

        setupSGData(ri, sgData);

        GFX->setZWriteEnable(false);
        while (mat->setupPass(sgData))
        {
            if (newPassNeeded(mat, ri))
                break;
            
            // Ew
            //GFX->setPixelShaderConstF(PC_VISIBILITY, &ri->visibility, 1);
            //setupSGData(ri, sgData);
            sgData.matIsInited = true;
            //mat->setLightInfo(sgData);
            mat->setWorldXForm(*ri->worldXform);
            //mat->setObjectXForm(*ri->objXform);
            //mat->setEyePosition(*ri->objXform, gRenderInstManager.getCamPos());
            mat->setBuffers(ri->vertBuff, ri->primBuff);

            // draw it
            GFX->drawPrimitive(ri->primBuffIndex);
        }

    }

    GFX->setZEnable(true);
    GFX->setZWriteEnable(true);
    GFX->setAlphaTestEnable(false);
    GFX->setAlphaBlendEnable(false);

    GFX->popWorldMatrix();

    PROFILE_END();
}
