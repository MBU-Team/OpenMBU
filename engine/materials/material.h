//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------
#ifndef _MATERIAL_H_
#define _MATERIAL_H_

#ifndef _GAMEBASE_H_
#include "game/gameBase.h"
#endif

#include "materials/miscShdrDat.h"
#include "gfx/gfxTextureHandle.h"
#include "gfx/gfxStructs.h"

class CubemapData;
struct SceneGraphData;
class GFXCubemap;

//**************************************************************************
// Basic Material
//**************************************************************************
class Material : public SimObject
{
    typedef SimObject Parent;

public:

    //-----------------------------------------------------------------------
    // Enums
    //-----------------------------------------------------------------------
    enum Constants
    {
        MAX_TEX_PER_PASS = 8,         // number of textures per pass
        MAX_STAGES = 4,
    };


    enum TexType
    {
        Standard = 1,
        Detail,
        Bump,
        Env,
        Cube,
        SGCube,  // scene graph cube - probably dynamic
        Lightmap,
        NormLightmap,
        Mask,
        Fog,
        BackBuff,
        ReflectBuff,
        Misc,
        DynamicLight,
        DynamicLightSecondary
    };

    enum BlendOp
    {
        None = 0,
        Mul,
        Add,
        AddAlpha,      // add modulated with alpha channel
        Sub,
        LerpAlpha,     // linear interpolation modulated with alpha channel
    };

    enum AnimType
    {
        Scroll = 1,
        Rotate = 2,
        Wave = 4,
        Scale = 8,
        Sequence = 16,
    };

    enum WaveType
    {
        Sin = 0,
        Triangle,
        Square,
    };

    enum MatType
    {
        base = 0,
        custom = 1,
    };

    struct StageData
    {
        GFXTexHandle      tex[GFXShaderFeatureData::NumFeatures];
        GFXCubemap* cubemap;
        bool              lightmap;

        StageData()
        {
            cubemap = NULL;
            lightmap = false;
        }
    };


    //-----------------------------------------------------------------------
    // Data
    //-----------------------------------------------------------------------
    StringTableEntry  baseTexFilename[MAX_STAGES];
    StringTableEntry  detailFilename[MAX_STAGES];
    StringTableEntry  bumpFilename[MAX_STAGES];
    StringTableEntry  envFilename[MAX_STAGES];
    StageData         stages[MAX_STAGES];
    ColorF            diffuse[MAX_STAGES];
    ColorF            specular[MAX_STAGES];
    F32               specularPower[MAX_STAGES];
    bool              pixelSpecular[MAX_STAGES];
    bool              vertexSpecular[MAX_STAGES];

#ifdef MB_ULTRA
    // Fall back in case this material is unsupported on the current GPU
    Material*         fallback;
#endif

    // yes this should be U32 - we test for 2 or 4...
    U32               exposure[MAX_STAGES];

    U32               animFlags[MAX_STAGES];
    Point2F           scrollDir[MAX_STAGES];
    F32               scrollSpeed[MAX_STAGES];
    Point2F           scrollOffset[MAX_STAGES];

    F32               rotSpeed[MAX_STAGES];
    Point2F           rotPivotOffset[MAX_STAGES];
    F32               rotPos[MAX_STAGES];

    F32               wavePos[MAX_STAGES];
    F32               waveFreq[MAX_STAGES];
    F32               waveAmp[MAX_STAGES];
    U32               waveType[MAX_STAGES];

    F32               seqFramePerSec[MAX_STAGES];
    F32               seqSegSize[MAX_STAGES];

    bool              glow[MAX_STAGES];          // entire stage glows
    bool              emissive[MAX_STAGES];


    bool              castsShadow;
    bool              breakable;
    bool              doubleSided;

    const char* cubemapName;

    CubemapData* mCubemapData;
    bool              dynamicCubemap;

    bool              translucent;
    BlendOp           translucentBlendOp;
    bool              translucentZWrite;

    bool              planarReflection;

#ifdef MARBLE_BLAST
    float friction;
    float restitution;
    float force;
    int sound;
#endif

#ifdef MB_ULTRA
    float softwareMipOffset;
    const char* noiseTexFileName;
#endif

    const char* mapTo; // map Material to this texture name

public:
    static F32 mDt;
    static F32 mAccumTime;

protected:
    static U32 mLastTime;
    static SimSet* gMatSet;

    static LightInfo smDebugLight;
    static bool smDebugLightingEnabled;

    bool  hasSetStageData;
    MatType mType;

    static EnumTable mBlendOpTable;
    static EnumTable mWaveTypeTable;

    virtual void mapMaterial();

    bool onAdd();
    void onRemove();

    GFXTexHandle createTexture(const char* filename, GFXTextureProfile* profile);

public:
    Material();

    static void initPersistFields();
    static void updateTime();
    static SimSet* getMaterialSet();

    static LightInfo* getDebugLight();
    static bool isDebugLightingEnabled() {return smDebugLightingEnabled; }

    virtual void setShaderConstants(const SceneGraphData& sgData, U32 stageNum);
    void setBlendState(Material::BlendOp blendOp);

    virtual void setStageData();

    MatType getType() { return mType; }

    DECLARE_CONOBJECT(Material);
};

#endif _MATERIAL_H_
