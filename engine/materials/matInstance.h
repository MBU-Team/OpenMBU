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

class GFXShader;
class GFXCubemap;
class ShaderFeature;

//**************************************************************************
// Render pass data
//**************************************************************************
struct RenderPassData
{
    GFXTexHandle      tex[Material::MAX_TEX_PER_PASS];
    U32               texFlags[Material::MAX_TEX_PER_PASS];
    GFXCubemap* cubeMap;

    U32                  numTex;
    U32                  numTexReg;
    GFXShaderFeatureData fData;
    GFXShader* shader;
    bool                 translucent;
    bool                 glow;
    Material::BlendOp    blendOp;
    U32                  stageNum;

    RenderPassData()
    {
        reset();
    }

    void reset()
    {
        dMemset(this, 0, sizeof(RenderPassData));
    }

};


//**************************************************************************
// Material Instance
//**************************************************************************
class MatInstance
{

private:
    Material* mMaterial;
    SceneGraphData    mSGData;
    U32               mCurPass;
    bool              mProcessedMaterial;    // TEMP until plugged into T2
    GFXVertexFlags    mVertFlags;
    U32               mMaxStages;
    bool              mHasGlow;

    Vector <RenderPassData> mPasses;


    void createPasses(GFXShaderFeatureData& fd, U32 stageNum);
    void addPass(RenderPassData& rpd,
        U32& texIndex,
        GFXShaderFeatureData& fd,
        U32 stageNum);

    void processMaterial();
    U32  getNumStages();
    void determineFeatures(U32 stageNum, GFXShaderFeatureData& fd);
    void cleanup();
    void setTextureStages(SceneGraphData& sgData);
    void setTextureTransforms();
    F32  getWaveOffset(U32 stage);
    void setPassBlendOp(ShaderFeature* sf,
        RenderPassData& passData,
        U32& texIndex,
        GFXShaderFeatureData& stageFeatures,
        U32 stageNum);

    bool filterGlowPasses(SceneGraphData& sgData);

public:

    /// Create a material instance by reference to a Material.
    MatInstance(Material& mat);
    /// Create a material instance by name.
    MatInstance(char* matName);

    bool setupPass(SceneGraphData& sgData);
    void init(SceneGraphData& dat, GFXVertexFlags vertFlags);
    Material* getMaterial() { return mMaterial; }

    bool hasCubemap();
    bool hasGlow() { return mHasGlow; }
};

#endif _MATINSTANCE_H_