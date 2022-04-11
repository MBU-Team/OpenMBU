//-----------------------------------------------------------------------------
// Torque Game Engine Advanced
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "gfx/gfxShaderMgr.h"
#include "gfx/gfxShader.h"

//****************************************************************************
// GFX Shader Manager
//****************************************************************************


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------
GFXShaderMgr::GFXShaderMgr()
{
}

//----------------------------------------------------------------------------
// Shutdown
//----------------------------------------------------------------------------
void GFXShaderMgr::shutdown()
{

    // delete custom shaders
    for( U32 i=0; i<mCustShaders.size(); i++ )
    {
        if( mCustShaders[i] )
        {
            delete mCustShaders[i];
            mCustShaders[i] = NULL;
        }
    }
    flushProceduralShaders();
}

//----------------------------------------------------------------------------
// Destroy shader - JM
//----------------------------------------------------------------------------
void GFXShaderMgr::destroyShader(GFXShader* shader)
{
    if( !shader || mCustShaders.empty() ) return;

    for( U32 i = 0; i<mCustShaders.size(); i++ )
    {
        if( mCustShaders[i] == shader )
        {
            mCustShaders.erase(i);
            break;
        }
    }

    delete shader;
}

void GFXShaderMgr::flushProceduralShaders()
{
    for (ShaderMap::Iterator i = mProcShaders.begin(); i != mProcShaders.end(); i++)
    {
        GFXShader* shader = i->value;
        delete shader;
    }
    mProcShaders.clear();
}