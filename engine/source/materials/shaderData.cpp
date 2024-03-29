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
    addField("DXPixelShaderFile", TypeString, Offset(DXPixelShaderName, ShaderData));

    addField("OGLVertexShaderFile", TypeString, Offset(OGLVertexShaderName, ShaderData));
    addField("OGLPixelShaderFile", TypeString, Offset(OGLPixelShaderName, ShaderData));

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

StringTableEntry ShaderData::getVertexShaderPath()
{
    if (DXVertexShaderName[0] != '.')
        return DXVertexShaderName;
    char fullFilename[128];
    dStrncpy(fullFilename, mPath, dStrlen(mPath) + 1);
    dStrcat(fullFilename, DXVertexShaderName);

    return StringTable->insert(fullFilename);
}

StringTableEntry ShaderData::getPixelShaderPath()
{
    if (DXPixelShaderName[0] != '.')
        return DXPixelShaderName;
    char fullFilename[128];
    dStrncpy(fullFilename, mPath, dStrlen(mPath) + 1);
    dStrcat(fullFilename, DXPixelShaderName);

    return StringTable->insert(fullFilename);
}

bool ShaderData::onAdd()
{
    bool ret = Parent::onAdd();
    // save the current script path for texture lookup later
    const char* curScriptFile = Con::getVariable("Con::File");  // current script file - local materials.cs
    const char* path = dStrrchr(curScriptFile, '/');
    if (path != NULL) {
        U32 fileStrLen = path - curScriptFile + 1;
        dStrncpy(mPath, curScriptFile, fileStrLen);
        mPath[fileStrLen] = '\0';
    }
    else
        mPath[0] = '\0';
    return ret;
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
            char fullFilename1[128];
            char fullFilename2[128];
            const char* vtexShaderPath = DXVertexShaderName;
            const char* pixelShaderPath = DXPixelShaderName;

            if (vtexShaderPath != NULL && vtexShaderPath[0] == '.')
            {
                dStrncpy(fullFilename1, mPath, dStrlen(mPath) + 1);
                dStrcat(fullFilename1, vtexShaderPath);
                vtexShaderPath = fullFilename1;
            }

            if (pixelShaderPath != NULL && pixelShaderPath[0] == '.')
            {
                dStrncpy(fullFilename2, mPath, dStrlen(mPath) + 1);
                dStrcat(fullFilename2, pixelShaderPath);
                pixelShaderPath = fullFilename2;
            }

            shader = GFX->createShader( (char*)vtexShaderPath,
                                        (char*)pixelShaderPath,
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

void ShaderData::setPath(const char* path)
{
    dStrncpy(mPath, path, 256);
}

//--------------------------------------------------------------------------
// Reload shader
//--------------------------------------------------------------------------
ConsoleMethod(ShaderData, reload, bool, 2, 2, "Rebuilds both the vertex and pixel shaders.")
{
    return object->reloadShader();
}
