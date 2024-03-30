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
    if (dStrlen(DXVertexShaderName) > 1 && DXVertexShaderName[0] == '.' && (DXVertexShaderName[1] == '/' || DXVertexShaderName[1] == '\\'))
    {
        char fullFilename[128];
        dStrncpy(fullFilename, mPath, dStrlen(mPath) + 1);
        dStrcat(fullFilename, DXVertexShaderName + 2);

        return StringTable->insert(fullFilename);
    }

    return DXVertexShaderName;
}

StringTableEntry ShaderData::getPixelShaderPath()
{
    if (dStrlen(DXPixelShaderName) > 1 && DXPixelShaderName[0] == '.' && (DXPixelShaderName[1] == '/' || DXPixelShaderName[1] == '\\'))
    {
        char fullFilename[128];
        dStrncpy(fullFilename, mPath, dStrlen(mPath) + 1);
        dStrcat(fullFilename, DXPixelShaderName + 2);

        return StringTable->insert(fullFilename);
    }

    return DXPixelShaderName;
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

            if (vtexShaderPath != NULL && dStrlen(vtexShaderPath) > 1 && vtexShaderPath[0] == '.' && (vtexShaderPath[1] == '/' || vtexShaderPath[1] == '\\'))
            {
                dStrncpy(fullFilename1, mPath, dStrlen(mPath) + 1);
                dStrcat(fullFilename1, vtexShaderPath + 2);
                vtexShaderPath = fullFilename1;
            }

            if (pixelShaderPath != NULL && dStrlen(pixelShaderPath) > 1 && pixelShaderPath[0] == '.' && (pixelShaderPath[1] == '/' || pixelShaderPath[1] == '\\'))
            {
                dStrncpy(fullFilename2, mPath, dStrlen(mPath) + 1);
                dStrcat(fullFilename2, pixelShaderPath + 2);
                pixelShaderPath = fullFilename2;
            }

            bool valid = true;
            if (vtexShaderPath != NULL && !ResourceManager->find(vtexShaderPath))
            {
                Con::errorf("ShaderData::initShader - Could not find vertex shader file: %s", vtexShaderPath);
                valid = false;
            }

            if (pixelShaderPath != NULL && !ResourceManager->find(pixelShaderPath))
            {
                Con::errorf("ShaderData::initShader - Could not find pixel shader file: %s", pixelShaderPath);
                valid = false;
            }

            if (!valid)
                return false;

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
