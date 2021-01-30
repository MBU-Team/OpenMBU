//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------
#include "renderObjectMgr.h"


//-----------------------------------------------------------------------------
// render objects
//-----------------------------------------------------------------------------
void RenderObjectMgr::render()
{
    for (U32 i = 0; i < mElementList.size(); i++)
    {
        RenderInst* ri = mElementList[i].inst;
        if (ri->type == RenderInstManager::RIT_Shadow)
            ri->obj->renderShadow(ri->state, ri);
        else
            ri->obj->renderObject(ri->state, ri);
    }

}