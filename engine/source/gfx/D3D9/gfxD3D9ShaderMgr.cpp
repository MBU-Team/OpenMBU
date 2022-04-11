//-----------------------------------------------------------------------------
// Torque Game Engine Advanced
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "gfx/D3D9/platformD3D.h"
#include "gfx/D3D9/gfxD3D9Shader.h"
#include "gfx/D3D9/gfxD3D9ShaderMgr.h"
#include "gfx/gfxDevice.h"

#ifdef TORQUE_SHADERGEN
#  include "shaderGen/shaderGen.h"
#endif

//----------------------------------------------------------------------------
// Create Shader - for custom shader requests
//----------------------------------------------------------------------------
GFXShader *GFXD3D9ShaderMgr::createShader( const char *vertFile, const char *pixFile, F32 pixVersion )
{
   GFXD3D9Shader *newShader = new GFXD3D9Shader;
   newShader->registerResourceWithDevice(GFX);
   newShader->init( vertFile, pixFile, pixVersion );
   
   if( newShader )
      mCustShaders.push_back( newShader );
   
   return newShader;
}

//----------------------------------------------------------------------------
// Get shader
//----------------------------------------------------------------------------
GFXShader *GFXD3D9ShaderMgr::getShader( GFXShaderFeatureData &dat, GFXVertexFlags flags )
{               
#ifdef TORQUE_SHADERGEN
   U32 idx = dat.codify();

   // return shader if exists
   if( mProcShaders[idx] )
      return mProcShaders[idx];

   // if not, then create it
   char vertFile[256];
   char pixFile[256];
   F32  pixVersion;

   gShaderGen.generateShader( dat, vertFile, pixFile, &pixVersion, flags );

   GFXD3D9Shader * shader = new GFXD3D9Shader;
   shader->registerResourceWithDevice(GFX);
   shader->init( vertFile, pixFile, pixVersion );
   mProcShaders[idx] = shader;

   return shader;
#else
   return NULL;
#endif
}