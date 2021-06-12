//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------
#ifndef _RENDER_TRANSLUCENT_MGR_H_
#define _RENDER_TRANSLUCENT_MGR_H_

#include "renderElemMgr.h"

//**************************************************************************
// RenderTranslucentMgr
//**************************************************************************
class RenderTranslucentMgr : public RenderElemMgr
{
private:
    void setupSGData(RenderInst* ri, SceneGraphData& data);
    void addElement(RenderInst* inst);

public:
    typedef RenderElemMgr Parent;

    virtual void render();
};




#endif
