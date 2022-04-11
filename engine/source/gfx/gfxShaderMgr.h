//-----------------------------------------------------------------------------
// Torque Game Engine Advanced
//
// Copyright (c) 2003 GarageGames.Com
//-----------------------------------------------------------------------------

#ifndef _GFXSHADERMGR_H_
#define _GFXSHADERMGR_H_

#ifndef _GFXSTRUCTS_H_
#include "gfx/gfxStructs.h"
#endif

#ifndef _TVECTOR_H_
#include "core/tVector.h"
#endif

#ifndef CORE_TDICTIONARY_H
#include "core/tDictionary.h"
#endif

class GFXShader;

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
    typedef Map<U32, GFXShader*> ShaderMap;
    ShaderMap mProcShaders;
    //--------------------------------------------------------------
    // Procedures
    //--------------------------------------------------------------
public:
    GFXShaderMgr();
    virtual ~GFXShaderMgr(){};

    // For custom shader requests
    virtual GFXShader * createShader( const char *vertFile, const char *pixFile, F32 pixVersion ) = 0;

    // For procedural shaders - these are created if they don't exist and found if they do.
    virtual GFXShader * getShader( GFXShaderFeatureData &dat,
                                   GFXVertexFlags ) = 0;
    // destroy
    virtual void destroyShader( GFXShader *shader );

    virtual void shutdown();

    // This will delete all of the procedural shaders that we have.  Used to regenerate shaders when
    // the ShaderFeatures have changed (due to lighting system change, or new plugin)
    virtual void flushProceduralShaders();

};


#endif
