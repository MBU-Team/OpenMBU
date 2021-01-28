//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------
#ifndef _RENDER_MESH_MGR_H_
#define _RENDER_MESH_MGR_H_

#include "renderElemMgr.h"

//**************************************************************************
// RenderMeshMgr
//**************************************************************************
class RenderMeshMgr : public RenderElemMgr
{
private:
   void setupSGData( RenderInst *ri, SceneGraphData &data );

public:
   virtual void render();
};




#endif
