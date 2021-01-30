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
    void setupSGData(RenderInst* ri, SceneGraphData& data);

public:
    virtual void render();
    virtual void addElement(RenderInst* inst);
};




#endif
