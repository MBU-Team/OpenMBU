//-----------------------------------------------------------------------------
// Torque Game Engine Advanced
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------
#ifndef _MATERIALS_PROCESSEDMATERIAL_H_
#define _MATERIALS_PROCESSEDMATERIAL_H_

#include "gfx/gfxDevice.h"
#include "materials/material.h"
#include "lightingSystem/sgLightManager.h"
#include "shaderGen/featureMgr.h"
#include "materials/customMaterial.h"

/// This contains the data needed to render a pass.
/// This struct is subject to change.  I'm not thrilled with
/// having a GFXShader reference in a struct likely to be used
/// in fixed function rendering.
struct RenderPassData
{
    GFXTextureObject *  tex[Material::MAX_TEX_PER_PASS];
    U32               texFlags[Material::MAX_TEX_PER_PASS];
    GFXCubemapHandle      cubeMap;

    U32                  numTex;
    U32                  numTexReg;
    GFXShaderFeatureData fData;
    GFXShader *          shader;
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
        dMemset( this, 0, sizeof( RenderPassData ) );
    }

};

/// This is an abstract base class which provides the external
/// interface all subclasses must implement. This interface
/// primarily consists of setting state.  Pass creation
/// is implementation specific, and internal, thus it is
/// not in this base class.
class ProcessedMaterial
{
protected:
    /// Handy enum stolen from GFXShaderFeatureData.  By having it here
    /// we avoid relying on GFXShaderFeatureData for possibly non shaderized
    /// rendering.

    // CodeReview [8/13/2007 btr] We should probably just kill this enum and use the
    // GFXShaderFeatureData version.  I spent way too much time tracking down a bug because I hadn't also
    // updated this enum.  I'm not really sure what the duplication gains us.
    enum
    {
        RTLighting = 0,   // realtime lighting
        TexAnim,
        BaseTex,
        ColorMultiply,
        DynamicLight,
        DynamicLightDual,
        //DynamicLightMask,
        //DynamicLightAttenuateBackFace,
        SelfIllumination,
        LightMap,
        LightNormMap,
        BumpMap,
        DetailMap,
        ExposureX2,
        ExposureX4,
        EnvMap,
        CubeMap,
        // BumpCubeMap,
        // Refraction,
        PixSpecular,
        VertSpecular,
        Translucent,      // Not a feature with shader code behind it, but needed for pixSpecular.
        Visibility,
        Fog,              // keep fog last feature
        NumFeatures,
    };
    /// Our passes.
    Vector<RenderPassData> mPasses;

    /// The material which we are processing.
    Material* mMaterial;

    /// Material::StageData is used here because the shader generator throws a
    /// fit if it's passed anything else.
    Material::StageData   mStages[Material::MAX_STAGES];

    /// If we've already loaded the stage data
    bool mHasSetStageData;

    /// If we glow
    bool mHasGlow;

    /// Stores the previous cull mode, so if we're double sided we can be nice and reset the old cull mode.
    S32 mCullMode;

    /// Number of stages (not to be confused with number of passes)
    U32 mMaxStages;

    /// Vertex flags
    GFXVertexFlags mVertFlags;

    /// Sets the blend state for rendering
    virtual void setBlendState( Material::BlendOp blendOp );

    /// Loads the texture located at filename and gives it the specified profile
    GFXTexHandle createTexture( const char *filename, GFXTextureProfile *profile );
public:
    /// @name State setting functions
    ///
    /// @{

    virtual ~ProcessedMaterial() = default;

    /// Sets the textures needed for rendering the current pass
    virtual void setTextureStages( SceneGraphData &sgData, U32 pass ) = 0;

    /// Sets the shader constans needed for the given pass (if we're using shaders)
    virtual void setShaderConstants(const SceneGraphData &sgData, U32 pass) = 0;

    /// Sets the object transform, which is used to transform things such as light position into world space
    virtual void setObjectXForm(MatrixF xform, U32 pass) = 0;

    /// Sets the transformation matrix, i.e. Model * View * Projection
    virtual void setWorldXForm(MatrixF xform) = 0;

    /// Sets the light info for the given pass
    virtual void setLightInfo(const SceneGraphData& sgData, U32 pass) = 0;

    /// Sets the eye position so we can do view dependent effects such as specular lighting
    virtual void setEyePosition(MatrixF objTrans, Point3F position, U32 pass) = 0;

    /// Sets the given vertex and primitive buffers so we can render geometry
    virtual void setBuffers(GFXVertexBufferHandleBase* vertBuffer, GFXPrimitiveBufferHandle* primBuffer);

    /// Sets the texture transform matrices for texture animations such as scaling and waves
    virtual void setTextureTransforms() = 0;

    /// Sets the blend function needed for dynamic lighting
    virtual void setLightingBlendFunc() = 0;
    /// @}

    /// Initializes us (eg. loads textures, creates passes, generates shaders)
    virtual void init(SceneGraphData& sgData, GFXVertexFlags vertFlags) = 0;

    /// Sets up the given pass.  Returns true if the pass was set up, false if there was an error or if
    /// the specified pass is out of bounds.
    virtual bool setupPass(SceneGraphData& sgData, U32 pass) = 0;

    /// Cleans up the state and resources set by the given pass.
    virtual void cleanup(U32 pass) = 0;

    /// Returns the pass data for the given pass.
    RenderPassData* getPass(U32 pass)
    {
        if(pass >= mPasses.size())
            return NULL;
        return &mPasses[pass];
    }

    const RenderPassData* getPass(U32 pass) const
    {
        return getPass(pass);
    }

    /// Returns the number of stages we're rendering (not to be confused with the number of passes).
    virtual U32 getNumStages() = 0;

    /// Returns the number of passes we are rendering (not to be confused with the number of stages).
    U32 getNumPasses()
    {
        return mPasses.size();
    }

    /// Returns true if any pass glows
    bool hasGlow()
    {
        return mHasGlow;
    }

    U32 getStageFromPass(U32 pass)
    {
        if(pass >= mPasses.size())
            return 0;
        return mPasses[pass].stageNum;
    }
};

#endif