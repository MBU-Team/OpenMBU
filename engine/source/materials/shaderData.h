//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------
#ifndef _SHADERTDATA_H_
#define _SHADERTDATA_H_

#ifndef _SIMBASE_H_
#include "console/simBase.h"
#endif

#ifndef _GFXSHADER_H_
#include "gfx/gfxShader.h"
#endif

//**************************************************************************
// Shader data
//**************************************************************************
class ShaderData : public SimObject
{
    typedef SimObject Parent;

public:

    //--------------------------------------------------------------
    // Data
    //--------------------------------------------------------------

    bool                    useDevicePixVersion;
    F32                     pixVersion;
    StringTableEntry        DXVertexShaderName;
    StringTableEntry        DXPixelShaderName;

    StringTableEntry        OGLVertexShaderName;
    StringTableEntry        OGLPixelShaderName;

private:
    GFXShader*              shader;
    bool                    shaderInitialized;
    char                    mPath[256];

    //--------------------------------------------------------------
    // Procedures
    //--------------------------------------------------------------
private:

protected:
    bool onAdd();

public:
    ShaderData();

    static void initPersistFields();
    bool initShader();
    bool reloadShader();
    void destroyShader();
    void setPath(const char* path);
    GFXShader* getShader();

    StringTableEntry getVertexShaderPath();
    StringTableEntry getPixelShaderPath();

    DECLARE_CONOBJECT(ShaderData);
};



#endif // SHADERDATA





