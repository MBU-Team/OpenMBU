//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------
#include "shaderData.h"
#include "console/consoleTypes.h"
#include "gfx/gfxDevice.h"
#include "core/bitStream.h"
#include "gfx/gfxShaderMgr.h"

IMPLEMENT_CONOBJECT(ShaderData);

//****************************************************************************
// Shader Data
//****************************************************************************


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------
ShaderData::ShaderData()
{
    DXVertexShaderName = NULL;
    DXPixelShaderName = NULL;

    OGLVertexShaderName = NULL;
    OGLPixelShaderName = NULL;

    useDevicePixVersion = false;
    pixVersion = 1.0;
    shader = NULL;
    shaderInitialized = false;
}

//--------------------------------------------------------------------------
// Init fields
//--------------------------------------------------------------------------
void ShaderData::initPersistFields()
{
    Parent::initPersistFields();

    addField("DXVertexShaderFile", TypeString, Offset(DXVertexShaderName, ShaderData));
    addField("DXPixelShaderFile", TypeFilename, Offset(DXPixelShaderName, ShaderData));

    addField("OGLVertexShaderFile", TypeString, Offset(OGLVertexShaderName, ShaderData));
    addField("OGLPixelShaderFile", TypeFilename, Offset(OGLPixelShaderName, ShaderData));

    addField("useDevicePixVersion",  TypeBool, Offset(useDevicePixVersion,   ShaderData));
    addField("pixVersion", TypeF32, Offset(pixVersion, ShaderData));
}

//--------------------------------------------------------------------------
// getShader
//--------------------------------------------------------------------------
GFXShader* ShaderData::getShader()
{
    if( !shaderInitialized )
    {
        initShader();
        shaderInitialized = true;
    }

    return shader;
}

//--------------------------------------------------------------------------
// Init shader
//--------------------------------------------------------------------------
bool ShaderData::initShader()
{
    if( shader ) return true;

    F32 pixver = pixVersion;
    if(useDevicePixVersion)
        pixver = getMax(pixver, GFX->getPixelShaderVersion());

    // get shader type from GFX layer
    switch( GFX->getAdapterType() )
    {
        //case Direct3D9_360:
        case Direct3D9:
        {
            shader = GFX->createShader( (char*)DXVertexShaderName,
                                        (char*)DXPixelShaderName,
                                        pixver );
            break;
        }

        default:
            return false;
    }

    return true;
}

//--------------------------------------------------------------------------
// Reload shader - JM
//--------------------------------------------------------------------------
bool ShaderData::reloadShader()
{
    if (!shader) return false;

    destroyShader();
    return initShader();
}

//--------------------------------------------------------------------------
// Destroy shader - JM
//--------------------------------------------------------------------------
void ShaderData::destroyShader()
{
    if (!shader) return;

    GFX->destroyShader(shader);
    shader = NULL;

    return;
}

//--------------------------------------------------------------------------
// Reload shader
//--------------------------------------------------------------------------
ConsoleMethod(ShaderData, reload, bool, 2, 2, "Rebuilds both the vertex and pixel shaders.")
{
    return object->reloadShader();
}
