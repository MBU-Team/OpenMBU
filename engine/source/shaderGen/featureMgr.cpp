//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------
#include "featureMgr.h"
#include "bump.h"
#include "pixSpecular.h"
#include "lightingSystem/sgLightingFeatures.h"

FeatureMgr gFeatureMgr;


//****************************************************************************
// Shader Manager
//****************************************************************************
FeatureMgr::FeatureMgr()
{
    init();
}

//----------------------------------------------------------------------------
// Destructor
//----------------------------------------------------------------------------
FeatureMgr::~FeatureMgr()
{
    shutdown();
}

//----------------------------------------------------------------------------
// Shutdown
//----------------------------------------------------------------------------
void FeatureMgr::shutdown()
{
    for (U32 i = 0; i < mFeatures.size(); i++)
    {
        if (mFeatures[i])
        {
            delete mFeatures[i];
            mFeatures[i] = NULL;
        }
    }
}

//----------------------------------------------------------------------------
// Init
//----------------------------------------------------------------------------
void FeatureMgr::init()
{
    mFeatures.setSize(GFXShaderFeatureData::NumFeatures);

    mFeatures[GFXShaderFeatureData::RTLighting] = new VertLightColor;
    mFeatures[GFXShaderFeatureData::TexAnim] = new TexAnim;
    mFeatures[GFXShaderFeatureData::BaseTex] = new BaseTexFeat;
    mFeatures[ GFXShaderFeatureData::ColorMultiply ] = new ColorMultiplyFeat;
    mFeatures[GFXShaderFeatureData::DynamicLight] = new DynamicLightingFeature;
    mFeatures[GFXShaderFeatureData::DynamicLightDual] = new DynamicLightingDualFeature;
    mFeatures[GFXShaderFeatureData::SelfIllumination] = new SelfIlluminationFeature;
    mFeatures[GFXShaderFeatureData::LightMap] = new LightmapFeat;
    mFeatures[GFXShaderFeatureData::LightNormMap] = new LightNormMapFeat;
    mFeatures[GFXShaderFeatureData::BumpMap] = new BumpFeat;
    mFeatures[GFXShaderFeatureData::DetailMap] = new DetailFeat;
    mFeatures[GFXShaderFeatureData::ExposureX2] = new ExposureFeatureX2;
    mFeatures[GFXShaderFeatureData::ExposureX4] = new ExposureFeatureX4;
    mFeatures[GFXShaderFeatureData::EnvMap] = NULL;
    mFeatures[GFXShaderFeatureData::CubeMap] = new ReflectCubeFeat;
    mFeatures[GFXShaderFeatureData::PixSpecular] = new PixelSpecular;
    mFeatures[GFXShaderFeatureData::VertSpecular] = NULL;
    mFeatures[GFXShaderFeatureData::Translucent] = new ShaderFeature;
    mFeatures[GFXShaderFeatureData::Visibility] = new VisibilityFeat;
    mFeatures[GFXShaderFeatureData::Fog] = new FogFeat;


    // auxiliary features
    mAuxFeatures.push_back(new VertPosition);
}

//----------------------------------------------------------------------------
// Get feature
//----------------------------------------------------------------------------
ShaderFeature* FeatureMgr::get(U32 index)
{
    if (index >= mFeatures.size())
    {
        AssertFatal(false, "Feature request out of range.");
        return NULL;
    }
    return mFeatures[index];
}

//----------------------------------------------------------------------------
// getAux - get auxiliary feature
//----------------------------------------------------------------------------
ShaderFeature* FeatureMgr::getAux(U32 index)
{
    if (index >= mAuxFeatures.size())
    {
        return NULL;
    }
    return mAuxFeatures[index];
}






