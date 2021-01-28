//-----------------------------------------------------------------------------
// Torque Shader Engine
// 
// Copyright (c) 2003 GarageGames.Com
//-----------------------------------------------------------------------------

#ifndef _GFXSHADER_H_
#define _GFXSHADER_H_

#include "platform/types.h"

//**************************************************************************
// Shader
//**************************************************************************
class GFXShader
{
public:
   U32 mVertexFlags;
   F32 mPixVersion;

   virtual ~GFXShader(){};

   virtual void process(){};
};

#endif // GFXSHADER
