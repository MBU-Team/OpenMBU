//-----------------------------------------------------------------------------
// Torque Game Engine Advanced
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------
#ifndef _MATERIALS_PROCESSEDSHADERMATERIAL_H_
#define _MATERIALS_PROCESSEDSHADERMATERIAL_H_

#include "processedMaterial.h"

/// ShaderGen shader material.  Mostly just copy/paste from MatInstance.
class ProcessedShaderMaterial : public ProcessedMaterial
{
public:

    ProcessedShaderMaterial();
    ProcessedShaderMaterial(Material &mat);

    /// @name Render state setting
    ///
    /// @{

    /// Binds all of the necessary textures for the given pass
    virtual void setTextureStages( SceneGraphData &sgData, U32 pass );

    /// Sets all of the necessary shader constants for the given pass
    virtual void setShaderConstants(const SceneGraphData &sgData, U32 pass);

    /// Sets VC_OBJ_TRANS in the current shader and additionally sets VC_CUBE_TRANS if we have a cubemap.
    virtual void setObjectXForm(MatrixF xform, U32 pass);

    /// Sets the transformation matrix (Model * View * Projection) for the current shader
    virtual void setWorldXForm(MatrixF xform);

    /// Sets the light info for a given pass
    virtual void setLightInfo(const SceneGraphData& sgData, U32 pass);

    /// Sets VC_EYE_POS in the current shader and additionally sets VC_CUBE_EYE_POS if we have a cubemaps
    virtual void setEyePosition(MatrixF objTrans, Point3F position, U32 pass);

    /// Sets the lighting blend function
    virtual void setLightingBlendFunc();

    /// @}

    /// Creates the passes, generates our shaders, and all that other initialization stuff
    virtual void init(SceneGraphData& sgData, GFXVertexFlags vertFlags);

    /// Sets up the given pass for rendering
    virtual bool setupPass(SceneGraphData& sgData, U32 pass);

    /// Gets the number of stages we're using (not to be confused with the number of passes!)
    virtual U32 getNumStages();

    /// Sets texture transformation matrices for texture animations such as scale and wave
    virtual void setTextureTransforms();

protected:
    /// @name Internal functions
    ///
    /// @{

    /// Adds a pass for the given stage.
    virtual void addPass( RenderPassData &rpd,
                          U32 &texIndex,
                          GFXShaderFeatureData &fd,
                          U32 stageNum,
                          SceneGraphData& sgData );
    /// Cleans up the given pass (or will anyways).
    virtual void cleanup(U32 pass);

    /// Chooses a blend op for the given pass
    virtual void setPassBlendOp( ShaderFeature *sf,
                                 RenderPassData &passData,
                                 U32 &texIndex,
                                 GFXShaderFeatureData &stageFeatures,
                                 U32 stageNum,
                                 SceneGraphData& sgData );

    /// Creates passes for the given stage
    virtual void createPasses( GFXShaderFeatureData &fd, U32 stageNum, SceneGraphData& sgData );

    /// Fills in the GFXShaderFeatureData for the given stage
    virtual void determineFeatures( U32 stageNum, GFXShaderFeatureData &fd, SceneGraphData& sgData );

    /// Do we have a cubemap on pass?
    virtual bool hasCubemap(U32 pass);

    /// Used by setTextureTransforms
    F32 getWaveOffset( U32 stage );

    /// Loads textures and such for all of our stages
    virtual void setStageData();

    /// @}

    void setPrimaryLightConst(const LightInfo* light, const MatrixF& objTrans, const U32 stageNum);
};

#endif