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
    S32               mCurPass;
    bool              mProcessedMaterial;
    GFXVertexFlags    mVertFlags;
    U32               mMaxStages;
    S32               mCullMode;
    bool              mHasGlow;

    Vector <RenderPassData> mPasses;

    static Vector<MatInstance*> mMatInstList;

    void createPasses(GFXShaderFeatureData& fd, U32 stageNum);
    void addPass(RenderPassData& rpd,
        U32& texIndex,
        GFXShaderFeatureData& fd,
        U32 stageNum);

    void processMaterial();
    U32  getNumStages();
    void determineFeatures(U32 stageNum, GFXShaderFeatureData& fd);
    void cleanup();
    void setTextureTransforms();
    F32  getWaveOffset(U32 stage);
    void setPassBlendOp(ShaderFeature* sf,
        RenderPassData& passData,
        U32& texIndex,
        GFXShaderFeatureData& stageFeatures,
        U32 stageNum);

    bool filterGlowPasses(SceneGraphData& sgData);
    void construct();

private:
    enum materialTypeEnum
    {
        mtStandard,
        mtDynamicLightingSingle,
        mtDynamicLightingDual
    };
    materialTypeEnum materialType;
    LightInfo::sgFeatures dynamicLightingFeatures;
    MatInstance* dynamicLightingMaterials_Single[LightInfo::sgFeatureCount];
    MatInstance* dynamicLightingMaterials_Dual[LightInfo::sgFeatureCount];
    void clearDynamicLightingMaterials()
    {
        for (U32 i = 0; i < LightInfo::sgFeatureCount; i++)
        {
            if (dynamicLightingMaterials_Single[i])
                delete dynamicLightingMaterials_Single[i];
            dynamicLightingMaterials_Single[i] = NULL;

            if (dynamicLightingMaterials_Dual[i])
                delete dynamicLightingMaterials_Dual[i];
            dynamicLightingMaterials_Dual[i] = NULL;
        }
    }
public:
    bool isDynamicLightingMaterial() { return (materialType != mtStandard); }
    bool isDynamicLightingMaterial_Dual() { return (materialType == mtDynamicLightingDual); }
    static MatInstance* getDynamicLightingMaterial(MatInstance* root, LightInfo* light, bool tryfordual)
    {
        if (!root) return NULL;
        // in order from most features to least...
        // we scan in reverse order (from least to most)
        // this provides the opportunity to combine like
        // materials making batching more efficient, by
        // leaving a level NULL as the next level up is
        // the same (not yet done)...
        AssertFatal((root->materialType == mtStandard), "Calling getDynamicLightingMaterial on dynamic lighting material!");
        AssertFatal((light->sgSupportedFeatures < LightInfo::sgFeatureCount), "Invalid light features.");
        if (tryfordual && LightManager::sgAllowDynamicLightingDualOptimization())
        {
            // try dual...
            for (S32 i = light->sgSupportedFeatures; i >= 0; i--)
            {
                if (root->dynamicLightingMaterials_Dual[i] && root->dynamicLightingMaterials_Dual[i]->isDynamicLightingMaterial())
                    return root->dynamicLightingMaterials_Dual[i];
            }
        }
        // look for single
        for (S32 i = light->sgSupportedFeatures; i >= 0; i--)
        {
            if (root->dynamicLightingMaterials_Single[i] && root->dynamicLightingMaterials_Single[i]->isDynamicLightingMaterial())
                return root->dynamicLightingMaterials_Single[i];
        }
        return NULL;
    }

    void setTextureStages(SceneGraphData& sgData, U32 pass);

    /// Create a material instance by reference to a Material.
    MatInstance(Material& mat);
    /// Create a material instance by name.
    MatInstance(const char* matName);

    void setLightmaps(SceneGraphData& sgData);

    virtual ~MatInstance();
    static void reInitInstances();

    bool setupPass(SceneGraphData& sgData);
    void init(SceneGraphData& dat, GFXVertexFlags vertFlags);
    void reInit();
    Material* getMaterial() { return mMaterial; }
    bool hasGlow() { return mHasGlow; }
    U32 getCurPass()
    {
        return getMax(mCurPass, 0);
    }

    U32 getCurStageNum()
    {
        if (getCurPass() >= mPasses.size())
            return 0;
        return mPasses[mCurPass].stageNum;
    }
    RenderPassData* getPass(U32 pass)
    {
        if (pass >= mPasses.size())
            return NULL;
        return &mPasses[pass];
    }

    bool hasCubemap();
};

#endif _MATINSTANCE_H_