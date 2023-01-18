//-----------------------------------------------------------------------------
// Torque Game Engine Advanced
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------
#ifndef _MATERIALS_PROCESSEDCUSTOMMATERIAL_H_
#define _MATERIALS_PROCESSEDCUSTOMMATERIAL_H_

#include "processedShaderMaterial.h"

/// Custom processed material. 
///
/// @note Mostly just copy/paste from CustomMaterial
class ProcessedCustomMaterial : public ProcessedShaderMaterial
{
    /// Texture flags
    U32            mFlags[CustomMaterial::MAX_TEX_PER_PASS];

    /// How many textures we have
    U32            mMaxTex;

    ProcessedCustomMaterial* mCustomPasses[CustomMaterial::MAX_PASSES];
    U32 mNumCustomPasses;
public:
    ProcessedCustomMaterial(Material &mat);
    ~ProcessedCustomMaterial();

    virtual bool doesRefract(U32 pass) const;

    virtual bool setupPass(SceneGraphData& sgData, U32 pass);
    virtual void init(SceneGraphData& sgData, GFXVertexFlags vertFlags);
    virtual bool hasCubemap(U32 pass);
    virtual void setTextureStages(SceneGraphData &sgData, U32 pass );
    virtual void cleanup(U32 pass);

protected:
    /// @name Internal functions
    ///
    /// @{
    virtual bool setupPassInternal(SceneGraphData& sgData, U32 pass);
    /// Actually does pass setup
    virtual void setupSubPass(SceneGraphData& sgData, U32 pass);

    virtual void setStageData();
    /// @}
};

#endif