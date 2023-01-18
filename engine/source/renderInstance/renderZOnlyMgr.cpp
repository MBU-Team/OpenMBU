//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------
#include "renderZOnlyMgr.h"
#include "materials/sceneData.h"
#include "sceneGraph/sceneGraph.h"
#include "materials/matInstance.h"
#include "../../game/shaders/shdrConsts.h"


//**************************************************************************
// RenderZOnlyMgr
//**************************************************************************
RenderZOnlyMgr::RenderZOnlyMgr()
{
    mBlankShader = NULL;
}


//-----------------------------------------------------------------------------
// render
//-----------------------------------------------------------------------------
void RenderZOnlyMgr::render()
{
    if (!this->mBlankShader)
        Sim::findObject<ShaderData>("BlankShader", mBlankShader);

    if (!mBlankShader || !mBlankShader->getShader())
        return;

    ColorI clearColor = ColorI(0x00, 0x00, 0x00, 0xFF); // 0xFF000000

    GFX->clear(GFXClearZBuffer | GFXClearStencil, clearColor, 1.0f, 0);
    mBlankShader->getShader()->process();

    GFX->enableColorWrites(false, false, false, false);
    GFX->setZWriteEnable(true);
    GFX->setZEnable(true);
    GFX->setZFunc(GFXCmpLessEqual);
    
    GFXPrimitiveBuffer* lastPB = NULL;

    for (S32 i = 0; i < mElementList.size(); i++)
    {
        RenderInst* inst = mElementList[i].inst;
        GFX->setVertexShaderConstF(0, (float*)inst->worldXform, 4);
        GFX->setVertexBuffer(*inst->vertBuff);
        GFX->setPrimitiveBuffer(*inst->primBuff);

        if (inst->primBuff)
            GFX->drawPrimitive(inst->primBuffIndex);
    }

    GFX->disableShaders();
    GFX->enableColorWrites(true, true, true, true);
    GFX->updateStates();
}
