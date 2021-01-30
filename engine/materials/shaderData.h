//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------
#ifndef _SHADERTDATA_H_
#define _SHADERTDATA_H_

#ifndef _GAMEBASE_H_
#include "game/gameBase.h"
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
    GFXShader* shader;

    F32                     pixVersion;
    StringTableEntry        DXVertexShaderName;
    StringTableEntry        DXPixelShaderName;

    StringTableEntry        OGLVertexShaderName;
    StringTableEntry        OGLPixelShaderName;


    //--------------------------------------------------------------
    // Procedures
    //--------------------------------------------------------------
private:

public:
    ShaderData();

    static void initPersistFields();
    bool onAdd();
    bool initShader();
    bool reloadShader();
    void destroyShader();

    DECLARE_CONOBJECT(ShaderData);
};



#endif // SHADERDATA





