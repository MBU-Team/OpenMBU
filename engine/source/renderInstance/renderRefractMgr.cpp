//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------
#include "renderRefractMgr.h"
#include "materials/sceneData.h"
#include "sceneGraph/sceneGraph.h"
#include "materials/matInstance.h"
#include "../../game/shaders/shdrConsts.h"


//**************************************************************************
// RenderRefractMgr
//**************************************************************************

//-----------------------------------------------------------------------------
// render
//-----------------------------------------------------------------------------
void RenderRefractMgr::render()
{
    // Early out if nothing to draw.
    if (!mElementList.size())
        return;

    PROFILE_START(RenderRefractMgrRender);

    GFX->pushWorldMatrix();

    if (getCurrentClientSceneGraph()->isReflectPass())
        GFX->setCullMode(GFXCullCW);
    else
        GFX->setCullMode(GFXCullCCW);

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
    SceneGraphData sgData;
    U32 binSize = mElementList.size();

    for (U32 j = 0; j < binSize; j++)
    {
        RenderInst* ri = mElementList[j].inst;

        setupSGData(ri, sgData);
        sgData.refractPass = true;
        MatInstance* mat = ri->matInst;

        U32 matListEnd = j;


        while (mat->setupPass(sgData))
        {
            if (newPassNeeded(mat, ri))
                break;

            //sgData.refractPass = true;
            //setupSGData(passRI, sgData);

            // sgData.matIsInited = true;
            mat->setLightInfo(sgData);
            mat->setWorldXForm(*ri->worldXform);
            mat->setObjectXForm(*ri->objXform);
            mat->setEyePosition(*ri->objXform, gRenderInstManager.getCamPos());
            mat->setBuffers(ri->vertBuff,ri->primBuff);

            // draw it
            GFX->drawPrimitive(ri->primBuffIndex);
        }
    }

    GFX->popWorldMatrix();

    PROFILE_END();
}
