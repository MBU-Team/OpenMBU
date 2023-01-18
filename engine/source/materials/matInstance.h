//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------
#ifndef _MATINSTANCE_H_
#define _MATINSTANCE_H_

#ifndef _MATERIAL_H_
#include "materials/material.h"
#endif

#include "miscShdrDat.h"
#include "sceneData.h"
#include "processedMaterial.h"

class GFXShader;
class GFXCubemap;
class ShaderFeature;
class MatInstanceHook;

//**************************************************************************
// Material Instance
//**************************************************************************
class MatInstance
{
public:
    static void reInitInstances();
private:
    static Vector<MatInstance*> mMatInstList;

    void construct();

private:
    Material* mMaterial;
    ProcessedMaterial* mProcessedMaterial;
    SceneGraphData    mSGData;
    S32               mCurPass;
    GFXVertexFlags    mVertFlags;
    U32               mMaxStages;
    S32               mCullMode;
    bool              mHasGlow;
    U32               mSortWeight;

    bool filterGlowPasses( SceneGraphData &sgData );
    bool filterRefractPasses( SceneGraphData &sgData );
    virtual void processMaterial();
public:
    // Used to attach information to a MatInstance.
    MatInstanceHook* mLightingHook;

#ifdef TORQUE_DEBUG
    char mMatName[64];
    static void dumpMatInsts();
#endif

    // Compares this MatInstance to mat
    virtual S32 compare(MatInstance* mat);

    /// Create a material instance by reference to a Material.
    MatInstance( Material &mat );
    /// Create a material instance by name.
    MatInstance( const char * matName );

    virtual ~MatInstance();

    bool setupPass( SceneGraphData &sgData );

    void setObjectXForm(MatrixF xform)
    {
        mProcessedMaterial->setObjectXForm(xform, getCurPass());
    }

    void setWorldXForm(MatrixF xform)
    {
        mProcessedMaterial->setWorldXForm(xform);
    }

    void setLightInfo(const SceneGraphData& sgData)
    {
        mProcessedMaterial->setLightInfo(sgData, getCurPass());
    }

    void setEyePosition(MatrixF objTrans, Point3F position)
    {
        mProcessedMaterial->setEyePosition(objTrans, position, getCurPass());
    }
    void setBuffers(GFXVertexBufferHandleBase* vertBuffer, GFXPrimitiveBufferHandle* primBuffer)
    {
        mProcessedMaterial->setBuffers(vertBuffer, primBuffer);
    }

    void setTextureStages( SceneGraphData &sgData )
    {
        mProcessedMaterial->setTextureStages(sgData, getCurPass());
    }

    void setLightingBlendFunc()
    {
        mProcessedMaterial->setLightingBlendFunc();
    }

    void init( SceneGraphData &dat, GFXVertexFlags vertFlags );
    void reInit();
    Material *getMaterial(){ return mMaterial; }
    ProcessedMaterial *getProcessedMaterial()
    {
        return mProcessedMaterial;
    }
    bool hasGlow()
    {
        return mProcessedMaterial->hasGlow();
    }
    U32 getCurPass()
    {
        return getMax( mCurPass, 0 );
    }

    U32 getCurStageNum()
    {
        return mProcessedMaterial->getStageFromPass(getCurPass());
    }
    RenderPassData *getPass(U32 pass)
    {
        return mProcessedMaterial->getPass(pass);
    }

    const SceneGraphData& getSceneGraphData() const
    {
        return mSGData;
    }

    const GFXVertexFlags getVertFlags() const
    {
        return mVertFlags;
    }
};

// This class is used by the lighting system to add additional data to a MatInstance.
// Currently, it just defines a virtual destructor and enforces a bit of type checking.
class MatInstanceHook
{
public:
    virtual ~MatInstanceHook() { };
};

#endif _MATINSTANCE_H_