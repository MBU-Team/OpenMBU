//-----------------------------------------------------------------------------
// Torque Game Engine Advanced
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------
#ifndef _MATERIALS_PROCESSEDFFMATERIAL_H_
#define _MATERIALS_PROCESSEDFFMATERIAL_H_

#include "processedMaterial.h"

/// Fixed function rendering.  Does not load or use shaders.  Does not rely on GFXShaderFeatureData.
/// Tries very hard to not rely on anything possibly related to shaders. 
///
/// @note Does not always succeed.
class ProcessedFFMaterial : public ProcessedMaterial
{
public:
    ProcessedFFMaterial();
    ProcessedFFMaterial(Material &mat);
    /// @name Render state setting
    ///
    /// @{

    /// Sets necessary textures and texture ops for rendering
    virtual void setTextureStages( SceneGraphData &sgData, U32 pass );

    /// Stubs out setting base shader constants
    virtual void setShaderConstants(const SceneGraphData &sgData, U32 pass);

    /// This is only used by shaders, stubbed out here.
    virtual void setObjectXForm(MatrixF xform, U32 pass);

    /// Expects xform to be Model * View * Projection
    virtual void setWorldXForm(MatrixF xform);

    virtual void setLightInfo(const SceneGraphData& sgData, U32 pass);

    /// Sets the eye position.
    ///
    /// @note Likely to be a stub
    virtual void setEyePosition(MatrixF objTrans, Point3F position, U32 pass);

    /// Sets the blend function for dynamic lighting
    virtual void setLightingBlendFunc();
    /// @}

    /// Creates necessary passes, blend ops, etc.
    virtual void init(SceneGraphData& sgData, GFXVertexFlags vertFlags);

    /// Sets up the given pass
    ///
    /// @returns false if the pass could not be set up
    virtual bool setupPass(SceneGraphData& sgData, U32 pass);

    /// Returns the number of stages we're using (not to be confused with the number of passes)
    virtual U32 getNumStages();

    /// Sets texture transform matrices for texture animations
    virtual void setTextureTransforms();

protected:
    struct FixedFuncFeatureData
    {
        enum
        {
            BaseTex,
            Lightmap,
            NoiseTex,
            NumFeatures
        };
        bool features[NumFeatures];
    };

    /// @name Internal functions
    ///
    /// @{

    /// Adds a pass for the given stage
    virtual void addPass(U32 stageNum, FixedFuncFeatureData& featData);

    /// Undoes all state changes for the given pass (or will anyways)
    virtual void cleanup(U32 pass);

    /// Chooses a blend op for the pass during pass creation
    virtual void setPassBlendOp();

    /// Creates all necessary passes for the given stage
    void createPasses( U32 stageNum, SceneGraphData& sgData );

    /// Determine what features we need
    void determineFeatures(U32 stageNum, FixedFuncFeatureData& featData, SceneGraphData& sgData);

    /// Loads all the textures for all of the stages in the Material
    virtual void setStageData();

    /// Sets light info for the first light
    virtual void setPrimaryLightInfo(MatrixF objTrans, LightInfo* light, U32 pass);

    /// Sets light info for the second light
    virtual void setSecondaryLightInfo(MatrixF objTrans, LightInfo* light);
    /// @}
};

#endif