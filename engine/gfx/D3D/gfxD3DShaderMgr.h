//-----------------------------------------------------------------------------
// Torque Shader Engine
// 
// Copyright (c) 2003 GarageGames.Com
//-----------------------------------------------------------------------------

#ifndef _GFXD3DSHADERMGR_H_
#define _GFXD3DSHADERMGR_H_

#include "gfx/gfxShaderMgr.h"

//**************************************************************************
// Shader Manager
//**************************************************************************
class GFXD3DShaderMgr : public GFXShaderMgr
{
    //--------------------------------------------------------------
    // Data
    //--------------------------------------------------------------


    //--------------------------------------------------------------
    // Procedures
    //--------------------------------------------------------------
public:
    virtual ~GFXD3DShaderMgr() {};

    // For custom shader requests
    virtual GFXShader* createShader(char* vertFile, char* pixFile, F32 pixVersion);

    // For procedural shaders - these are created if they don't exist and found if they do.
    virtual GFXShader* getShader(GFXShaderFeatureData& dat,
        GFXVertexFlags vertFlags);


};


#endif

