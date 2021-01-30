//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------
#ifndef _BUMP_H_
#define _BUMP_H_

#include "shaderFeature.h"

struct   RenderPassData;
class    MultiLine;

//**************************************************************************
/*!
   Bump mapping feature for shader generation

   This feature will apply bump mapping to the shader being constructed.
   It will do this differently depending on whether a light-normal map is
   present.

   If one is, then it is probably an interior, and the light
   direction for the bumping comes from the light-normal map that is
   precalculated for the interior.

   If the map is not present, then the light direction for the bump is
   passed in as a vertex shader constant.

*/
//**************************************************************************
class BumpFeat : public ShaderFeature
{
    LangElement* addBumpCoords(Vector<ShaderComponent*>& componentList);

    /// Setup lightVec in vertex shader
    Var* setupLightVec(Vector<ShaderComponent*>& componentList,
        MultiLine* meta,
        GFXShaderFeatureData& fd);

    /// Get lightVec from tex coord or light normal map - pixel shader
    Var* getLightVec(Vector<ShaderComponent*>& componentList,
        GFXShaderFeatureData& fd,
        MultiLine* meta);


public:
    virtual void processVert(Vector<ShaderComponent*>& componentList,
        GFXShaderFeatureData& fd);

    virtual void processPix(Vector<ShaderComponent*>& componentList,
        GFXShaderFeatureData& fd);

    virtual Material::BlendOp getBlendOp() { return Material::LerpAlpha; }

    virtual Resources getResources(GFXShaderFeatureData& fd);

    /// Sets textures and texture flags for current pass
    virtual void setTexData(Material::StageData& stageDat,
        GFXShaderFeatureData& fd,
        RenderPassData& passData,
        U32& texIndex);
};



#endif _BUMP_H_