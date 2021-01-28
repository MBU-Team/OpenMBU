//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------
#ifndef _CUSTOMMATERIAL_H_
#define _CUSTOMMATERIAL_H_

#ifndef _GAMEBASE_H_
#include "game/gameBase.h"
#endif

#ifndef _MATERIAL_H_
#include "materials/material.h"
#endif

#include "sceneData.h"

class ShaderData;

//**************************************************************************
// Custom Material
//**************************************************************************
class CustomMaterial : public Material
{
   typedef Material Parent;

public:

   //-----------------------------------------------------------------------
   // Enums
   //-----------------------------------------------------------------------
   enum CustomConsts
   {
      MAX_PASSES = 8,
      NUM_FALLBACK_VERSIONS = 2,
   };


   //----------------- data ----------------------
   StringTableEntry  texFilename[MAX_TEX_PER_PASS];
   GFXTexHandle      tex[MAX_TEX_PER_PASS];
   CustomMaterial *  pass[MAX_PASSES];
   CustomMaterial *  fallback;

   CustomMaterial *dynamicLightingMaterial;

   U8             startDataMarker;  // used for pack/unpackData

   F32            mVersion;   // 0 = legacy, 1 = DX 8.1, 2 = DX 9.0
   BlendOp        blendOp;
   bool           refract;


protected:
   U32            mMaxTex;
   ShaderData *   mShaderData;
   const char *   mShaderDataName;
   S32            mCurPass;
   S32            mCullMode;
   U32            mFlags[MAX_TEX_PER_PASS];

   U8             endDataMarker;  // used for pack/unpackData

   //----------------- procedures ----------------------
   bool onAdd();
   void onRemove();
   MatrixF setupTexAnim( AnimType animType, Point2F &scrollDir, 
                         F32 scrollSpeed, U32 texUnit );
   void cleanup();
   void setMultipassProjection();
   void setupStages( SceneGraphData &sgData );
   bool setNextRefractPass( bool );

   virtual void mapMaterial();


public:
   CustomMaterial();

   static void initPersistFields();
   static void updateTime();

   void setLightmaps( SceneGraphData &sgData );

   void setupSubPass( SceneGraphData &sgData );
   bool setFallbackVersion( SceneGraphData &sgData );

   virtual void setShaderConstants( const SceneGraphData &sgData, U32 stageNum );

   //----------------------------------------------------------------------
   // Sets up next rendering pass.  Will increment through passes until it
   // runs out.  Returns false if interated through all passes.
   //----------------------------------------------------------------------
   virtual bool setupPass( SceneGraphData &sgData );

   virtual void setStageData();

   DECLARE_CONOBJECT(CustomMaterial);
};

#endif _CUSTOMMATERIAL_H_