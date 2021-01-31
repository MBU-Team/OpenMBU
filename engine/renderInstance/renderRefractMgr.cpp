//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------
#include "renderRefractMgr.h"
#include "materials/sceneData.h"
#include "sceneGraph/sceneGraph.h"
#include "materials/matInstance.h"
#include "../../example/shaders/shdrConsts.h"


//**************************************************************************
// RenderRefractMgr
//**************************************************************************

//-----------------------------------------------------------------------------
// setup scenegraph data
//-----------------------------------------------------------------------------
void RenderRefractMgr::setupSGData(RenderInst* ri, SceneGraphData& data)
{
    dMemset(&data, 0, sizeof(SceneGraphData));

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

}


//-----------------------------------------------------------------------------
// render
//-----------------------------------------------------------------------------
void RenderRefractMgr::render()
{

    GFX->pushWorldMatrix();

    if (getCurrentClientSceneGraph()->isReflectPass())
    {
        GFX->setCullMode(GFXCullCW);
    }
    else
    {
        GFX->setCullMode(GFXCullCCW);
    }


    //-------------------------------------
    // copy sfx backbuffer
    //-------------------------------------
    // set ortho projection matrix
    MatrixF proj = GFX->getProjectionMatrix();
    MatrixF newMat(true);
    GFX->setProjectionMatrix(newMat);
    GFX->pushWorldMatrix();
    GFX->setWorldMatrix(newMat);

    GFX->copyBBToSfxBuff();

    // restore projection matrix
    GFX->setProjectionMatrix(proj);
    GFX->popWorldMatrix();
    //-------------------------------------

    // init loop data
    GFXVertexBuffer* lastVB = NULL;
    GFXPrimitiveBuffer* lastPB = NULL;
    SceneGraphData sgData;
    U32 binSize = mElementList.size();

    for (U32 j = 0; j < binSize; )
    {
        RenderInst* ri = mElementList[j].inst;

        setupSGData(ri, sgData);
        sgData.refractPass = true;
        MatInstance* mat = ri->matInst;

        U32 matListEnd = j;


        while (mat->setupPass(sgData))
        {
            U32 a;
            for (a = j; a < binSize; a++)
            {
                RenderInst* passRI = mElementList[a].inst;

                if (mat != passRI->matInst)
                {
                    break;
                }

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

                Point3F lightDir = passRI->light.mDirection;
                objTrans.mulV( lightDir );
                GFX->setVertexShaderConstF( VC_LIGHT_DIR1, (float*)&lightDir, 1 );


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

        }

        // force increment if none happened, otherwise go to end of batch
        j = (j == matListEnd) ? j + 1 : matListEnd;

    }


    GFX->popWorldMatrix();
}
