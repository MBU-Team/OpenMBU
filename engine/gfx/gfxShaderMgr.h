//-----------------------------------------------------------------------------
// Torque Shader Engine
// 
// Copyright (c) 2003 GarageGames.Com
//-----------------------------------------------------------------------------

#ifndef _GFXSHADERMGR_H_
#define _GFXSHADERMGR_H_

#include "gfx/GFXDevice.h"
#include "core/tVector.h"

//**************************************************************************
// Shader Manager
//**************************************************************************
class GFXShaderMgr
{
    //--------------------------------------------------------------
    // Data
    //--------------------------------------------------------------
    enum Constants
    {
    };

protected:
    Vector <GFXShader*> mCustShaders;
    Vector <GFXShader*> mProcShaders;


    //--------------------------------------------------------------
    // Procedures
    //--------------------------------------------------------------
public:
    GFXShaderMgr();
    virtual ~GFXShaderMgr() {};

    // For custom shader requests
    virtual GFXShader* createShader(char* vertFile, char* pixFile, F32 pixVersion) = 0;

    // For procedural shaders - these are created if they don't exist and found if they do.
    virtual GFXShader* getShader(GFXShaderFeatureData& dat,
        GFXVertexFlags) = 0;
    // destroy
    virtual void destroyShader(GFXShader* shader);


    virtual void shutdown();

};


#endif
