//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------
#ifndef _RENDER_ZONLY_MGR_H_
#define _RENDER_ZONLY_MGR_H_

#include "renderElemMgr.h"

//**************************************************************************
// RenderGlowMgr
//**************************************************************************
class RenderZOnlyMgr : public RenderElemMgr
{
private:
    ShaderData* mBlankShader;

public:
    RenderZOnlyMgr();

    virtual void render();
};




#endif
