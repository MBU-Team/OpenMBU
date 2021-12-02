//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------
#ifndef _SHADERFEATURE_H_
#define _SHADERFEATURE_H_

#include "core/tVector.h"
#include "shaderComp.h"
#include "materials/material.h"

struct LangElement;
struct GFXShaderFeatureData;
struct RenderPassData;

//**************************************************************************
/*!
   The ShaderFeature class is the base class for every procedurally generated
   feature. Each feature the engine recognizes is part of the enum
   in GFXShaderFeatureData.  That structure is used to indicate which
   features are present in a shader to be generated.  This is useful as many
   ShaderFeatures will output different code depending on what other
   features are going to be in the shader.

   Shaders are generated using the ShaderFeature interface, so all of the
   decendents interact pretty much the same way.

*/
//**************************************************************************


//**************************************************************************
// Shader Feature
//**************************************************************************
class ShaderFeature
{

protected:
    LangElement* output;

public:
    //**************************************************************************
    /*!
       The Resources structure is used by ShaderFeature to indicate how many
       hardware "resources" it needs.  Resources are things such as how
       many textures it uses and how many texture registers it needs to pass
       information from the vertex to the pixel shader.

       The Resources data can change depending what hardware is available.  For
       instance, pixel 1.x level hardware may need more texture registers than
       pixel 2.0+ hardware because each texture register can only be used with
       its respective texture sampler.

       The data in Resources is used to determine how many features can be
       squeezed into a singe shader.  If a feature requires too many resources
       to fit into the current shader, it will be put into another pass.
    */
    //**************************************************************************
    struct Resources
    {
        U32 numTex;
        U32 numTexReg;

        Resources()
        {
            dMemset(this, 0, sizeof(Resources));
        }
    };


    //-----------------------------------------------------------------------
    // Base functions
    //-----------------------------------------------------------------------
    ShaderFeature();

    /// returns output from a processed vertex or pixel shader
    LangElement* getOutput() { return output; }

    /// Get the incoming base texture coords - useful for bumpmap and detail maps
    Var* getVertBaseTex();

    /// Set up a texture space matrix - to pass into pixel shader
    LangElement* setupTexSpaceMat(Vector<ShaderComponent*>& componentList,
        Var** texSpaceMat);

    /// Get the color assignment - helper function
    LangElement* assignColor(LangElement* elem, bool add = false);


    //-----------------------------------------------------------------------
    // Virtual Functions
    //-----------------------------------------------------------------------

    //-----------------------------------------------------------------------
    /*!
       Process vertex shader - This function is used by each feature to
       generate a list of LangElements that can be traversed and "printed"
       to generate the actual shader code.  The 'output' member is the head
       of that list.

       The componentList is used mostly for access to the "Connector"
       structure which is used to pass data from the vertex to the pixel
       shader.

       The GFXShaderFeatureData parameter is used to determine what other
       features are present for the shader being generated.
    */
    //-----------------------------------------------------------------------
    virtual void processVert(Vector<ShaderComponent*>& componentList,
        GFXShaderFeatureData& fd)
    {
        output = NULL;
    }

    //-----------------------------------------------------------------------
    /*!
       Process pixel shader - This function is used by each feature to
       generate a list of LangElements that can be traversed and "printed"
       to generate the actual shader code.  The 'output' member is the head
       of that list.

       The componentList is used mostly for access to the "Connector"
       structure which is used to pass data from the vertex to the pixel
       shader.

       The GFXShaderFeatureData parameter is used to determine what other
       features are present for the shader being generated.
    */
    //-----------------------------------------------------------------------
    virtual void processPix(Vector<ShaderComponent*>& componentList,
        GFXShaderFeatureData& fd)
    {
        output = NULL;
    }

    /// Identifies what type of blending a feature uses.  This is used to
    /// group features with the same blend operation together in a multipass
    /// situation.
    virtual Material::BlendOp getBlendOp() { return Material::Add; }

    /// Returns the resource requirments of this feature based on what
    /// other features are present.  The "resources" are things such as
    /// texture units, and texture registers of which there can be
    /// very limited numbers.  The resources can vary depending on hardware
    /// and what other features are present.
    virtual Resources getResources(GFXShaderFeatureData& fd)
    {
        Resources temp;
        return temp;
    }

    /// Fills texture related info in RenderPassData for this feature.  It
    /// takes into account the current pass (passData) as well as what other
    /// data is available to the material stage (stageDat).  
    ///
    /// For instance, ReflectCubeFeat would like to modulate its output 
    /// by the alpha channel of another texture.  If the current pass does
    /// not contain a diffuse or bump texture, but the Material does, then
    /// this function allows it to use one of those textures in the current
    /// pass.
    virtual void setTexData(Material::StageData& stageDat,
        GFXShaderFeatureData& fd,
        RenderPassData& passData,
        U32& texIndex) {};

};


//**************************************************************************
/// Vertex position
//**************************************************************************
class VertPosition : public ShaderFeature
{
public:
    virtual void processVert(Vector<ShaderComponent*>& componentList,
        GFXShaderFeatureData& fd);

};

//**************************************************************************
/// Vertex lighting color
//**************************************************************************
class VertLightColor : public ShaderFeature
{
public:
    virtual void processVert(Vector<ShaderComponent*>& componentList,
        GFXShaderFeatureData& fd);

    virtual void processPix(Vector<ShaderComponent*>& componentList,
        GFXShaderFeatureData& fd);

    virtual Material::BlendOp getBlendOp() { return Material::None; }
};


//**************************************************************************
/// Base texture
//**************************************************************************
class BaseTexFeat : public ShaderFeature
{
public:
    virtual void processVert(Vector<ShaderComponent*>& componentList,
        GFXShaderFeatureData& fd);

    virtual void processPix(Vector<ShaderComponent*>& componentList,
        GFXShaderFeatureData& fd);

    virtual Material::BlendOp getBlendOp() { return Material::LerpAlpha; }

    virtual Resources getResources(GFXShaderFeatureData& fd);

    // Sets textures and texture flags for current pass
    virtual void setTexData(Material::StageData& stageDat,
        GFXShaderFeatureData& fd,
        RenderPassData& passData,
        U32& texIndex);
};

//**************************************************************************
/// Lightmap
//**************************************************************************
class LightmapFeat : public ShaderFeature
{
public:
    virtual void processVert(Vector<ShaderComponent*>& componentList,
        GFXShaderFeatureData& fd);

    virtual void processPix(Vector<ShaderComponent*>& componentList,
        GFXShaderFeatureData& fd);

    virtual Material::BlendOp getBlendOp() { return Material::LerpAlpha; }

    virtual Resources getResources(GFXShaderFeatureData& fd);

    // Sets textures and texture flags for current pass
    virtual void setTexData(Material::StageData& stageDat,
        GFXShaderFeatureData& fd,
        RenderPassData& passData,
        U32& texIndex);
};

//**************************************************************************
/// Detail map
//**************************************************************************
class DetailFeat : public ShaderFeature
{
public:
    virtual void processVert(Vector<ShaderComponent*>& componentList,
        GFXShaderFeatureData& fd);

    virtual void processPix(Vector<ShaderComponent*>& componentList,
        GFXShaderFeatureData& fd);

    virtual Resources getResources(GFXShaderFeatureData& fd);

    virtual Material::BlendOp getBlendOp() { return Material::Mul; }

    // Sets textures and texture flags for current pass
    virtual void setTexData(Material::StageData& stageDat,
        GFXShaderFeatureData& fd,
        RenderPassData& passData,
        U32& texIndex);
};


//**************************************************************************
/// Light Normal Map
//**************************************************************************
class LightNormMapFeat : public ShaderFeature
{
public:
    virtual void processVert(Vector<ShaderComponent*>& componentList,
        GFXShaderFeatureData& fd);

    virtual void processPix(Vector<ShaderComponent*>& componentList,
        GFXShaderFeatureData& fd);

    virtual Material::BlendOp getBlendOp() { return Material::None; }

    virtual Resources getResources(GFXShaderFeatureData& fd);

    // Sets textures and texture flags for current pass
    virtual void setTexData(Material::StageData& stageDat,
        GFXShaderFeatureData& fd,
        RenderPassData& passData,
        U32& texIndex);
};

//**************************************************************************
/// Reflect Cubemap
//**************************************************************************
class ReflectCubeFeat : public ShaderFeature
{
public:
    virtual void processVert(Vector<ShaderComponent*>& componentList,
        GFXShaderFeatureData& fd);

    virtual void processPix(Vector<ShaderComponent*>& componentList,
        GFXShaderFeatureData& fd);

    virtual Resources getResources(GFXShaderFeatureData& fd);

    // Sets textures and texture flags for current pass
    virtual void setTexData(Material::StageData& stageDat,
        GFXShaderFeatureData& fd,
        RenderPassData& passData,
        U32& texIndex);
};

//**************************************************************************
/// Fog
//**************************************************************************
class FogFeat : public ShaderFeature
{
public:
    virtual void processVert(Vector<ShaderComponent*>& componentList,
        GFXShaderFeatureData& fd);

    virtual void processPix(Vector<ShaderComponent*>& componentList,
        GFXShaderFeatureData& fd);

    virtual Resources getResources(GFXShaderFeatureData& fd);

    // Sets textures and texture flags for current pass
    virtual void setTexData(Material::StageData& stageDat,
        GFXShaderFeatureData& fd,
        RenderPassData& passData,
        U32& texIndex);

    virtual Material::BlendOp getBlendOp() { return Material::LerpAlpha; }

};

//**************************************************************************
/// Tex Anim
//**************************************************************************
class TexAnim : public ShaderFeature
{
public:

    virtual Material::BlendOp getBlendOp() { return Material::None; }

};

//**************************************************************************
/// Visibility
//**************************************************************************
class VisibilityFeat : public ShaderFeature
{
public:

    virtual void processPix(Vector<ShaderComponent*>& componentList, GFXShaderFeatureData& fd);

    virtual Material::BlendOp getBlendOp() { return Material::None; }
};

//**************************************************************************
/// ColorMultiply
//**************************************************************************
class ColorMultiplyFeat : public ShaderFeature
{
public:
    virtual void processPix( Vector<ShaderComponent*> &componentList, GFXShaderFeatureData &fd );
    virtual Material::BlendOp getBlendOp(){ return Material::None; }
};

#endif _SHADERFEATURE_H_