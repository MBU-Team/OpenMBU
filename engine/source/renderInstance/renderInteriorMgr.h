//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------
#ifndef _RENDER_INTERIOR_MGR_H_
#define _RENDER_INTERIOR_MGR_H_

#include "renderElemMgr.h"

//**************************************************************************
// RenderInteriorMgr
//**************************************************************************
class RenderInteriorMgr : public RenderElemMgr
{
private:
    ShaderData* mBlankShader;

    void setupSGData(RenderInst* ri, SceneGraphData& data);
    void renderZpass();

public:
    typedef RenderElemMgr Parent;

    RenderInteriorMgr();
    ~RenderInteriorMgr();

    virtual void render();
    virtual void addElement(RenderInst* inst);
};




#endif
