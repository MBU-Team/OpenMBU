//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------
#ifndef _CUSTOMMATERIAL_H_
#define _CUSTOMMATERIAL_H_

//#ifndef _GAMEBASE_H_
//#include "game/gameBase.h"
//#endif

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
    CustomMaterial* pass[MAX_PASSES];
    Material* fallback;

    CustomMaterial* dynamicLightingMaterial;
    CustomMaterial* dynamicLightingMaskMaterial;

    U8             startDataMarker;  // used for pack/unpackData

    F32            mVersion;   // 0 = legacy, 1 = DX 8.1, 2 = DX 9.0
    BlendOp        blendOp;
    bool           refract;


protected:
    U32            mMaxTex;
    S32            mCurPass;
    S32            mCullMode;
    U32            mFlags[MAX_TEX_PER_PASS];

    U8             endDataMarker;  // used for pack/unpackData

    //----------------- procedures ----------------------
    bool onAdd();
    void onRemove();

    virtual void mapMaterial();


public:
    CustomMaterial();

    static void initPersistFields();
    static void updateTime();
    virtual bool preloadTextures(Vector<const char*>& errorBuffer);
    const char* mShaderDataName;
    ShaderData* mShaderData;

    DECLARE_CONOBJECT(CustomMaterial);
};

#endif _CUSTOMMATERIAL_H_