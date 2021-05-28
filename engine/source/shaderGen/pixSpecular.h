//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------
#ifndef _PIXSPECULAR_H_
#define _PIXSPECULAR_H_

#include "shaderFeature.h"


//**************************************************************************
// DX 9.0 Per-pixel specular
//**************************************************************************
class PixelSpecular : public ShaderFeature
{

public:
    virtual void processVert(Vector<ShaderComponent*>& componentList,
        GFXShaderFeatureData& fd);

    virtual void processPix(Vector<ShaderComponent*>& componentList,
        GFXShaderFeatureData& fd);

    virtual Resources getResources(GFXShaderFeatureData& fd);
};





#endif _PIXSPECULAR_H_