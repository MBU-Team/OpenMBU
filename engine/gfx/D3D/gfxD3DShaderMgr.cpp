#include "d3dx9.h"

#include "gfxD3DShaderMgr.h"
#include "gfxD3DShader.h"
#include "shaderGen/shaderGen.h"

//****************************************************************************
// GFX D3D Shader Manager
//****************************************************************************


//----------------------------------------------------------------------------
// Create Shader - for custom shader requests
//----------------------------------------------------------------------------
GFXShader* GFXD3DShaderMgr::createShader(char* vertFile,
    char* pixFile,
    F32 pixVersion)
{
    GFXD3DShader* newShader = new GFXD3DShader;
    newShader->init(vertFile, pixFile, pixVersion);

    if (newShader)
    {
        mCustShaders.push_back(newShader);
    }

    return newShader;
}

//----------------------------------------------------------------------------
// Get shader
//----------------------------------------------------------------------------
GFXShader* GFXD3DShaderMgr::getShader(GFXShaderFeatureData& dat,
    GFXVertexFlags flags)
{
    U32 idx = dat.codify();

    // return shader if exists
    if (mProcShaders[idx])
    {
        return mProcShaders[idx];
    }

    // if not, then create it
    char vertFile[256];
    char pixFile[256];
    F32  pixVersion;

    gShaderGen.generateShader(dat, vertFile, pixFile, &pixVersion, flags);

    GFXD3DShader* shader = new GFXD3DShader;
    shader->init(vertFile, pixFile, pixVersion);
    mProcShaders[idx] = shader;

    return shader;

}

