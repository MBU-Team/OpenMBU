//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------
#ifndef _SHADERGEN_H_
#define _SHADERGEN_H_

#include "platform/types.h"
#include "langElement.h"
#include "shaderFeature.h"
#include "shaderComp.h"
#include "gfx/gfxShader.h"
#include "gfx/gfxStructs.h"

class Material;

//**************************************************************************
/*!
   The ShaderGen class takes shader feature data (usually created by
   MatInstance) and creates a vertex/pixel shader pair in text files
   to be later compiled by a shader manager.

   It accomplishes this task by creating a group of shader "components" and
   "features" that output bits of high level shader code.  Shader components
   translate to structures in HLSL that indicate incoming vertex data,
   data that is output from the vertex shader to the pixel shader, and data
   such as constants and textures that are passed directly to the shader
   from the app.

   Shader features are separable shader functions that can be turned on or
   off.  Examples would be bumpmapping and specular highlights.  See
   GFXShaderFeatureData for the current list of features supported.

   ShaderGen processes all of the features that are present for a desired
   shader, and then prints them out to the respective vertex or pixel
   shader file.

   For more information on shader features and components see the
   ShaderFeature and ShaderComponent classes.
*/
//**************************************************************************



//**************************************************************************
// Shader generator
//**************************************************************************
class ShaderGen
{

private:
    //-----------------------------------------------------------------------
    // Private data
    //-----------------------------------------------------------------------
    GFXShaderFeatureData mFeatureData;
    GFXVertexFlags       mVertFlags;

    Vector< ShaderComponent*> mComponents;

    //-----------------------------------------------------------------------
    // Private procedures
    //-----------------------------------------------------------------------
    void init();
    void uninit();

    /// Intialize vertex definition - This function sets up the structure in 
    /// the shader indicating what vertex information is available.
    void initVertexDef();

    /// Creates all the various shader components that will be filled in when 
    /// the shader features are processed.
    void createComponents();

    /// print out the processed features to the file stream
    void printFeatures(Stream& stream);

    void printShaderHeader(Stream& stream);

    void processPixFeatures();
    void printPixShader(Stream& stream);

    void processVertFeatures();
    void printVertShader(Stream& stream);

public:
    ShaderGen();

    /// vertFile and pixFile are filled in by this function.  They point to 
    /// the vertex and pixel shader files.  pixVersion is also filled in by
    /// this function.
    void generateShader(const GFXShaderFeatureData& featureData,
        char* vertFile,
        char* pixFile,
        F32* pixVersion,
        GFXVertexFlags);

};

extern ShaderGen gShaderGen;

#endif _SHADERGEN_H_