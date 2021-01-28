//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------
#ifndef _SCENEDATA_H_
#define _SCENEDATA_H_

#include "sceneGraph/lightManager.h"

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

   // fog   
   F32         fogHeightOffset;
   F32         fogInvHeightRange;
   F32         visDist;
   bool        useFog;
   
   // misc
   Point3F        camPos;
   MatrixF        objTrans;
   VertexData *   vertData;
   GFXCubemap *   cubemap;
   bool           glowPass;
   bool           refractPass;


   //-----------------------------------------------------------------------
   // Constructor
   //-----------------------------------------------------------------------
   SceneGraphData()
      : lightmap(), normLightmap(), fogTex()
   {
      dMemset( this, 0, sizeof( SceneGraphData ) );
   }

};





#endif _SCENEDATA_H_