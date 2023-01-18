//-----------------------------------------------------------------------------
// Torque Game Engine Advanced
// 
// Copyright (c) 2003 GarageGames.Com
//-----------------------------------------------------------------------------

#ifndef _GFXD3D9SHADERMGR_H_
#define _GFXD3D9SHADERMGR_H_

#include "gfx/gfxShaderMgr.h"

//**************************************************************************
// Shader Manager
//**************************************************************************
class GFXD3D9ShaderMgr : public GFXShaderMgr
{
public:
   virtual ~GFXD3D9ShaderMgr(){};

   // For custom shader requests
   virtual GFXShader *createShader( const char *vertFile, const char *pixFile, F32 pixVersion );

   // For procedural shaders - these are created if they don't exist and found if they do.
   virtual GFXShader *getShader( GFXShaderFeatureData &dat, GFXVertexFlags vertFlags );
};

#endif