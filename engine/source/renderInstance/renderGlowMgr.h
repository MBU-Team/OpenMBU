//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------
#ifndef _RENDER_GLOW_MGR_H_
#define _RENDER_GLOW_MGR_H_

#include "renderElemMgr.h"

//**************************************************************************
// RenderGlowMgr
//**************************************************************************
class RenderGlowMgr : public RenderElemMgr
{
private:
    void setupSGData(RenderInst* ri, SceneGraphData& data);

public:
    typedef RenderElemMgr Parent;
    virtual void render();
};




#endif
