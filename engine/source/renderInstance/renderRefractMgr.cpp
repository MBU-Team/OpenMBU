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

                if (newPassNeeded(mat, passRI))
                    break;


                setupSGData(passRI, sgData);
                sgData.refractPass = true;
                // sgData.matIsInited = true;
                mat->setLightInfo(sgData);
                mat->setWorldXForm(*passRI->worldXform);
                mat->setObjectXForm(*passRI->objXform);
                mat->setEyePosition(*passRI->objXform, gRenderInstManager.getCamPos());
                mat->setBuffers(passRI->vertBuff, passRI->primBuff);

                // draw it
                GFX->drawPrimitive(passRI->primBuffIndex);
            }

            matListEnd = a;
        }

        // force increment if none happened, otherwise go to end of batch
        j = (j == matListEnd) ? j + 1 : matListEnd;
    }

    GFX->popWorldMatrix();

    PROFILE_END();
}
