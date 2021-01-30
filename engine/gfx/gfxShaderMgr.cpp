#include "gfxShaderMgr.h"

//****************************************************************************
// GFX Shader Manager
//****************************************************************************


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------
GFXShaderMgr::GFXShaderMgr()
{
    U32 maxShaders = 1 << GFXShaderFeatureData::NumFeatures;

    mProcShaders.setSize(maxShaders);
    for (U32 i = 0; i < mProcShaders.size(); i++)
    {
        mProcShaders[i] = NULL;
    }

}

//----------------------------------------------------------------------------
// Shutdown
//----------------------------------------------------------------------------
void GFXShaderMgr::shutdown()
{

    // delete custom shaders
    for (U32 i = 0; i < mCustShaders.size(); i++)
    {
        if (mCustShaders[i])
        {
            delete mCustShaders[i];
            mCustShaders[i] = NULL;
        }
    }

    // delete procedural shaders
    for (U32 i = 0; i < mProcShaders.size(); i++)
    {
        if (mProcShaders[i])
        {
            delete mProcShaders[i];
            mProcShaders[i] = NULL;
        }
    }

}

//----------------------------------------------------------------------------
// Destroy shader - JM
//----------------------------------------------------------------------------
void GFXShaderMgr::destroyShader(GFXShader* shader)
{
    if (!shader || mCustShaders.empty()) return;

    for (U32 i = 0; i < mCustShaders.size(); i++)
    {
        if (mCustShaders[i] == shader)
        {
            mCustShaders.erase(i);
            break;
        }
    }

    delete shader;
}