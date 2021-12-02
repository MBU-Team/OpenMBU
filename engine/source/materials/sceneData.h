//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------
#ifndef _SCENEDATA_H_
#define _SCENEDATA_H_

#include "sceneGraph/sceneGraph.h"
#include "sceneGraph/lightManager.h"
#include "sceneGraph/lightInfo.h"

struct VertexData;
class GFXTexHandle;
class GFXCubemap;

//**************************************************************************
// Scene graph data - temp - simulates data scenegraph will provide
//**************************************************************************
struct SceneGraphData
{
    // textures
    GFXTexHandle dynamicLight;
    GFXTexHandle dynamicLightSecondary;
    GFXTexHandle lightmap;
    GFXTexHandle normLightmap;
    GFXTexHandle fogTex;
    GFXTexHandle backBuffTex;
    GFXTexHandle reflectTex;
    GFXTexHandle miscTex;

    // lighting
    LightInfo   light;
    LightInfo   lightSecondary;
    bool        useLightDir;   // use light dir instead of position - used for sunlight outdoors
    bool        matIsInited;   // Set to true in the RenderInstanceMgr's after the MatInstance->setupPass call.

    // fog   
    F32         fogHeightOffset;
    F32         fogInvHeightRange;
    F32         visDist;
    bool        useFog;

    // misc
    Point3F        camPos;
    MatrixF        objTrans;
    VertexData* vertData;
    GFXCubemap* cubemap;
    bool           glowPass;
    bool           refractPass;
    F32 visibility;


    //-----------------------------------------------------------------------
    // Constructor
    //-----------------------------------------------------------------------
    SceneGraphData()
        : lightmap(), normLightmap(), fogTex()
    {
        reset();
    }

    inline void reset()
    {
        dMemset( this, 0, sizeof( SceneGraphData ) );
        visibility = 1.0f;
    }

    inline void setDefaultLights()
    {
        light = *getCurrentClientSceneGraph()->getLightManager()->getDefaultLight();
        lightSecondary = light;
    }

    inline void setFogParams()
    {
        // Fogging...
        useFog            = true;
        fogTex            = getCurrentClientSceneGraph()->getFogTexture();
        fogHeightOffset   = getCurrentClientSceneGraph()->getFogHeightOffset();
        visDist           = getCurrentClientSceneGraph()->getVisibleDistanceMod();
        fogInvHeightRange = getCurrentClientSceneGraph()->getFogInvHeightRange();
    }

};





#endif _SCENEDATA_H_