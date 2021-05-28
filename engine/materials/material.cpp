//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------
#include "material.h"
#include "console/consoleTypes.h"
#include "core/bitStream.h"
#include "../../game/shaders/shdrConsts.h"
#include "sceneData.h"
#include "gfx/gfxDevice.h"
#include "gfx/cubemapData.h"
#include "math/mathIO.h"
#include "materialPropertyMap.h"
#include <sceneGraph/sceneGraph.h>

//****************************************************************************
// Material
//****************************************************************************
IMPLEMENT_CONOBJECT(Material);

F32 Material::mDt = 0.0;
U32 Material::mLastTime = 0;
F32 Material::mAccumTime = 0.0;
SimSet* Material::gMatSet = NULL;
LightInfo Material::smDebugLight;
bool Material::smDebugLightingEnabled = false;

static EnumTable::Enums gBlendOpEnums[] =
{
   { Material::None,         "None" },
   { Material::Mul,          "Mul" },
   { Material::Add,          "Add" },
   { Material::AddAlpha,     "AddAlpha" },
   { Material::Sub,          "Sub" },
   { Material::LerpAlpha,    "LerpAlpha" }
};
EnumTable Material::mBlendOpTable(6, gBlendOpEnums);

static EnumTable::Enums gWaveTypeEnums[] =
{
   { Material::Sin,          "Sin" },
   { Material::Triangle,     "Triangle" },
   { Material::Square,       "Square" },
};
EnumTable Material::mWaveTypeTable(3, gWaveTypeEnums);

#ifdef MB_ULTRA
static EnumTable::Enums gCompressionEnums[] =
{
   { GFXTextureProfile::Compression::None,    "None" },
   { GFXTextureProfile::Compression::DXT1,    "DXT1" },
   { GFXTextureProfile::Compression::DXT2,    "DXT2" },
   { GFXTextureProfile::Compression::DXT3,    "DXT3" },
   { GFXTextureProfile::Compression::DXT4,    "DXT4" },
   { GFXTextureProfile::Compression::DXT5,    "DXT5" }
};
EnumTable Material::mCompressionTypeTable(6, gCompressionEnums);

static EnumTable::Enums gRenderBinEnums[] =
{
   { RenderInstManager::RenderBinTypes::Begin,    "Begin" },
   { RenderInstManager::RenderBinTypes::Interior,    "Interior" },
   { RenderInstManager::RenderBinTypes::InteriorDynamicLighting,    "InteriorDynamicLighting" },
   { RenderInstManager::RenderBinTypes::Mesh,    "Mesh" },
   { RenderInstManager::RenderBinTypes::Sky,    "Sky" },
   { RenderInstManager::RenderBinTypes::SkyShape,    "SkyShape" },
   { RenderInstManager::RenderBinTypes::MiscObject,    "MiscObject" },
   { RenderInstManager::RenderBinTypes::Shadow,    "Shadow" },
   { RenderInstManager::RenderBinTypes::Decal,    "Decal" },
   { RenderInstManager::RenderBinTypes::Refraction,    "Refraction" },
   { RenderInstManager::RenderBinTypes::Water,    "Water" },
   { RenderInstManager::RenderBinTypes::TranslucentPreGlow,    "TranslucentPreGlow" },
   { RenderInstManager::RenderBinTypes::Glow,    "Glow" },
   { RenderInstManager::RenderBinTypes::Foliage,    "Foliage" },
   { RenderInstManager::RenderBinTypes::Translucent,    "Translucent" },
};
EnumTable Material::mRenderBinTable(sizeof(gRenderBinEnums) / sizeof(EnumTable::Enums), gRenderBinEnums);
#endif

//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------
Material::Material()
{
    for (U32 i = 0; i < MAX_STAGES; i++)
    {
        diffuse[i].set(1.0, 1.0, 1.0, 1.0);
        specular[i].set(1.0, 1.0, 1.0, 1.0);
        specularPower[i] = 8;
        pixelSpecular[i] = false;
        vertexSpecular[i] = false;
        exposure[i] = 1;
        glow[i] = false;
        emissive[i] = false;
    }

    mType = base;

    castsShadow = true;
    breakable = false;
    doubleSided = false;

    translucent = false;
    subPassTranslucent = false;
    translucentBlendOp = LerpAlpha;
    translucentZWrite = false;

    planarReflection = false;

    mCubemapData = NULL;
    dynamicCubemap = NULL;
    cubemapName = NULL;

    dMemset(animFlags, 0, sizeof(animFlags));
    dMemset(scrollOffset, 0, sizeof(scrollOffset));

    dMemset(rotSpeed, 0, sizeof(rotSpeed));
    dMemset(rotPivotOffset, 0, sizeof(rotPivotOffset));
    dMemset(rotPos, 0, sizeof(rotPos));

    dMemset(wavePos, 0, sizeof(wavePos));
    dMemset(waveFreq, 0, sizeof(waveFreq));
    dMemset(waveAmp, 0, sizeof(waveAmp));
    dMemset(waveType, 0, sizeof(waveType));

    dMemset(seqFramePerSec, 0, sizeof(seqFramePerSec));
    dMemset(seqSegSize, 0, sizeof(seqSegSize));

    dMemset(baseTexFilename, 0, sizeof(baseTexFilename));
    dMemset(detailFilename, 0, sizeof(detailFilename));
    dMemset(bumpFilename, 0, sizeof(bumpFilename));
    dMemset(envFilename, 0, sizeof(envFilename));

#ifdef MARBLE_BLAST
    friction = 1.0f;
    sound = -1;
    restitution = 1.0f;
    force = 0.0f;
#endif

#ifdef MB_ULTRA
    dMemset(texCompression, 0, sizeof(texCompression));
    renderBin = RenderInstManager::RenderBinTypes::Begin;
    softwareMipOffset = 0.0f;
    noiseTexFileName = NULL;
    fallback = NULL;
#endif

    hasSetStageData = false;
    mapTo = NULL;
}

//--------------------------------------------------------------------------
// Init fields
//--------------------------------------------------------------------------
void Material::initPersistFields()
{
    Parent::initPersistFields();

    addField("diffuse", TypeColorF, Offset(diffuse, Material), MAX_STAGES);
    addField("specular", TypeColorF, Offset(specular, Material), MAX_STAGES);
    addField("specularPower", TypeF32, Offset(specularPower, Material), MAX_STAGES);
    addField("pixelSpecular", TypeBool, Offset(pixelSpecular, Material), MAX_STAGES);

    addField("exposure", TypeS32, Offset(exposure, Material), MAX_STAGES);

    addField("glow", TypeBool, Offset(glow, Material), MAX_STAGES);
    addField("emissive", TypeBool, Offset(emissive, Material), MAX_STAGES);

    addField("translucent", TypeBool, Offset(translucent, Material));
    addField("translucentBlendOp", TypeEnum, Offset(translucentBlendOp, Material), 1, &mBlendOpTable);
    addField("translucentZWrite", TypeBool, Offset(translucentZWrite, Material));

    addField("castsShadow", TypeBool, Offset(castsShadow, Material));
    addField("breakable", TypeBool, Offset(breakable, Material));
    addField("doubleSided", TypeBool, Offset(doubleSided, Material));

    addField("scrollDir", TypePoint2F, Offset(scrollDir, Material), MAX_STAGES);
    addField("scrollSpeed", TypeF32, Offset(scrollSpeed, Material), MAX_STAGES);

    addField("rotSpeed", TypeF32, Offset(rotSpeed, Material), MAX_STAGES);
    addField("rotPivotOffset", TypePoint2F, Offset(rotPivotOffset, Material), MAX_STAGES);

    addField("animFlags", TypeS32, Offset(animFlags, Material), MAX_STAGES);

    addField("waveType", TypeEnum, Offset(waveType, Material), MAX_STAGES, &mWaveTypeTable);
    addField("waveFreq", TypeF32, Offset(waveFreq, Material), MAX_STAGES);
    addField("waveAmp", TypeF32, Offset(waveAmp, Material), MAX_STAGES);

    addField("sequenceFramePerSec", TypeF32, Offset(seqFramePerSec, Material), MAX_STAGES);
    addField("sequenceSegmentSize", TypeF32, Offset(seqSegSize, Material), MAX_STAGES);

    // textures
    addField("baseTex", TypeFilename, Offset(baseTexFilename, Material), MAX_STAGES);
    addField("detailTex", TypeFilename, Offset(detailFilename, Material), MAX_STAGES);
    addField("bumpTex", TypeFilename, Offset(bumpFilename, Material), MAX_STAGES);
    addField("envTex", TypeFilename, Offset(envFilename, Material), MAX_STAGES);

#ifdef MB_ULTRA
    addField("texCompression", TypeEnum, Offset(texCompression, Material), MAX_STAGES, &mCompressionTypeTable);
    addField("renderBin", TypeEnum, Offset(renderBin, Material), 1, &mRenderBinTable);
#endif

    // cubemap
    addField("cubemap", TypeString, Offset(cubemapName, Material));
    addField("dynamicCubemap", TypeBool, Offset(dynamicCubemap, Material));

    addField("planarReflection", TypeBool, Offset(planarReflection, Material));
    addField("mapTo", TypeString, Offset(mapTo, Material));

#ifdef MARBLE_BLAST
    addField("friction", TypeF32, Offset(friction, Material));
    addField("restitution", TypeF32, Offset(restitution, Material));
    addField("force", TypeF32, Offset(force, Material));
    addField("sound", TypeS32, Offset(sound, Material));
#endif

#ifdef MB_ULTRA
    addField("softwareMipOffset", TypeF32, Offset(softwareMipOffset, Material));
    addField("noiseTexFileName", TypeFilename, Offset(noiseTexFileName, Material));
#endif

}

//--------------------------------------------------------------------------
// On add - verify data settings
//--------------------------------------------------------------------------
bool Material::onAdd()
{
    if (Parent::onAdd() == false)
        return false;

    if (cubemapName)
    {
        mCubemapData = static_cast<CubemapData*>(Sim::findObject(cubemapName));
    }

    SimSet* matSet = getMaterialSet();
    if (matSet)
    {
        matSet->addObject((SimObject*)this);
    }

    mapMaterial();

    setStageData();

    return true;
}

//--------------------------------------------------------------------------
// On remove
//--------------------------------------------------------------------------
void Material::onRemove()
{

    Parent::onRemove();
}

//--------------------------------------------------------------------------
// Set stage data
//--------------------------------------------------------------------------
void Material::setStageData()
{
    if (hasSetStageData) return;
    hasSetStageData = true;

    U32 i;

    for (i = 0; i < MAX_STAGES; i++)
    {
        if (baseTexFilename[i] && baseTexFilename[i][0])
        {
            stages[i].tex[GFXShaderFeatureData::BaseTex] = createTexture(baseTexFilename[i], &GFXDefaultStaticDiffuseProfile);
        }

        if (detailFilename[i] && detailFilename[i][0])
        {
            stages[i].tex[GFXShaderFeatureData::DetailMap] = createTexture(detailFilename[i], &GFXDefaultStaticDiffuseProfile);
        }

        if (bumpFilename[i] && bumpFilename[i][0])
        {
            stages[i].tex[GFXShaderFeatureData::BumpMap] = createTexture(bumpFilename[i], &GFXDefaultStaticDiffuseProfile);
        }

        if (envFilename[i] && envFilename[i][0])
        {
            stages[i].tex[GFXShaderFeatureData::EnvMap] = createTexture(envFilename[i], &GFXDefaultStaticDiffuseProfile);
        }
    }

    if (mCubemapData)
    {
        mCubemapData->createMap();
        stages[0].cubemap = mCubemapData->cubemap;
    }
}


//--------------------------------------------------------------------------
// Update time
//--------------------------------------------------------------------------
void Material::updateTime()
{
    U32 curTime = Platform::getVirtualMilliseconds();
    mDt = (curTime - mLastTime) / 1000.0;
    mLastTime = curTime;
    mAccumTime += mDt;
}

//--------------------------------------------------------------------------
// Set vertex and pixel shader constants
//--------------------------------------------------------------------------
void Material::setShaderConstants(const SceneGraphData& sgData, U32 stageNum)
{

    // set eye positions
    //-------------------------
    Point3F eyePos = sgData.camPos;
    MatrixF objTrans = sgData.objTrans;

    objTrans.transpose();
    GFX->setVertexShaderConstF(VC_OBJ_TRANS, (float*)&objTrans, 4);
    objTrans = sgData.objTrans;
    objTrans.affineInverse();

    objTrans.mulP(eyePos);
    GFX->setVertexShaderConstF(VC_EYE_POS, (float*)&eyePos, 1);


    // fill in primary light
    //-------------------------
    Point3F lightPos = sgData.light.mPos;
    Point3F lightDir = sgData.light.mDirection;
    objTrans.mulP(lightPos);
    objTrans.mulV(lightDir);

    Point4F lightPosModel(lightPos.x, lightPos.y, lightPos.z, sgData.light.sgTempModelInfo[0]);
    GFX->setVertexShaderConstF(VC_LIGHT_POS1, (float*)&lightPosModel, 1);
    GFX->setVertexShaderConstF(VC_LIGHT_DIR1, (float*)&lightDir, 1);
    GFX->setVertexShaderConstF(VC_LIGHT_DIFFUSE1, (float*)&sgData.light.mColor, 1);
    GFX->setPixelShaderConstF(PC_AMBIENT_COLOR, (float*)&sgData.light.mAmbient, 1);

    if (!emissive[stageNum])
        GFX->setPixelShaderConstF(PC_DIFF_COLOR, (float*)&sgData.light.mColor, 1);
    else
    {
        ColorF selfillum = LightManager::sgGetSelfIlluminationColor(diffuse[stageNum]);
        GFX->setPixelShaderConstF(PC_DIFF_COLOR, (float*)&selfillum, 1);
    }

    MatrixF lightingmat = sgData.light.sgLightingTransform;
    GFX->setVertexShaderConstF(VC_LIGHT_TRANS, (float*)&lightingmat, 4);


    // fill in secondary light
    //-------------------------
    lightPos = sgData.lightSecondary.mPos;
    lightDir = sgData.lightSecondary.mDirection;
    objTrans.mulP(lightPos);
    objTrans.mulV(lightDir);

    lightPosModel = Point4F(lightPos.x, lightPos.y, lightPos.z, sgData.lightSecondary.sgTempModelInfo[0]);
    GFX->setVertexShaderConstF(VC_LIGHT_POS2, (float*)&lightPosModel, 1);
    GFX->setPixelShaderConstF(PC_DIFF_COLOR2, (float*)&sgData.lightSecondary.mColor, 1);

    lightingmat = sgData.lightSecondary.sgLightingTransform;
    GFX->setVertexShaderConstF(VC_LIGHT_TRANS2, (float*)&lightingmat, 4);


    // fill in cubemap data
    //-------------------------
    if (mCubemapData || sgData.cubemap)
    {
        Point3F cubeEyePos = sgData.camPos - sgData.objTrans.getPosition();
        GFX->setVertexShaderConstF(VC_CUBE_EYE_POS, (float*)&cubeEyePos, 1);

        MatrixF cubeTrans = sgData.objTrans;
        cubeTrans.setPosition(Point3F(0.0, 0.0, 0.0));
        cubeTrans.transpose();
        GFX->setVertexShaderConstF(VC_CUBE_TRANS, (float*)&cubeTrans, 3);
    }

    // this is OK for now, will need to change later to support different
    // specular values per pass in custom materials
    //-------------------------
    GFX->setPixelShaderConstF(PC_MAT_SPECCOLOR, (float*)&specular[stageNum], 1);
    GFX->setPixelShaderConstF(PC_MAT_SPECPOWER, (float*)&specularPower[stageNum], 1);
    GFX->setVertexShaderConstF(VC_MAT_SPECPOWER, (float*)&specularPower[stageNum], 1);

    // fog
    //-------------------------
    Point4F fogData;
    fogData.x = sgData.fogHeightOffset;
    fogData.y = sgData.fogInvHeightRange;
    fogData.z = sgData.visDist;
    GFX->setVertexShaderConstF(VC_FOGDATA, (float*)&fogData, 1);

    // set detail scale
 //   F32 scale = 2.0;
 //   GFX->setVertexShaderConstF( VC_DETAIL_SCALE, &scale, 1 );

   // Visibility
    F32 visibility = sgData.visibility;
    GFX->setPixelShaderConstF(PC_VISIBILITY, &visibility, 1);
}

//--------------------------------------------------------------------------
// Set blend state
//--------------------------------------------------------------------------
void Material::setBlendState(Material::BlendOp blendOp)
{

    switch (blendOp)
    {
    case Add:
    {
        GFX->setSrcBlend(GFXBlendOne);
        GFX->setDestBlend(GFXBlendOne);
        break;
    }
    case AddAlpha:
    {
        GFX->setSrcBlend(GFXBlendSrcAlpha);
        GFX->setDestBlend(GFXBlendOne);
        break;
    }
    case Mul:
    {
        GFX->setSrcBlend(GFXBlendDestColor);
        GFX->setDestBlend(GFXBlendZero);
        break;
    }
    case LerpAlpha:
    {
        GFX->setSrcBlend(GFXBlendSrcAlpha);
        GFX->setDestBlend(GFXBlendInvSrcAlpha);
        break;
    }

    default:
        // default to LerpAlpha
        GFX->setSrcBlend(GFXBlendSrcAlpha);
        GFX->setDestBlend(GFXBlendInvSrcAlpha);
        break;
    }

}

//--------------------------------------------------------------------------
// Map this material to the texture specified in the "mapTo" data variable
//--------------------------------------------------------------------------
void Material::mapMaterial()
{
    if (!getName())
    {
        Con::warnf("Unnamed Material!  Could not map to: %s", mapTo);
        return;
    }

    // If mapTo not defined in script, try to use the base texture name instead
    if (!mapTo)
    {
        if (!baseTexFilename[0])
        {
            Con::warnf("Could not map	material: %s to a	texture name, need to specify	'mapTo' or 'baseTex[0]'	parameter", getName());
            return;
        }
        else
        {
            // extract filename from base texture
            mapTo = (char*)dStrrchr(baseTexFilename[0], '/') + 1;
            if (mapTo == (char*)0x001)
            {
                // no '/' character, must be no path, just the filename
                mapTo = baseTexFilename[0];
            }
        }
    }

    // add mapping to MaterialPropertyMap
    MaterialPropertyMap* pMap = MaterialPropertyMap::get();
    if (pMap == NULL)
    {
        Con::errorf(ConsoleLogEntry::General, "Error, cannot find the global material map object");
    }
    else
    {
        char* namePtrs[2];
        char matName[128] = { "material: " };

        U32 strLen = dStrlen(matName);
        dStrcpy(&matName[strLen], getName());
        namePtrs[0] = (char*)mapTo;
        namePtrs[1] = matName;
        pMap->addMapping(2, (const char**)namePtrs);
    }
}

//--------------------------------------------------------------------------
// createTexture - passthrough function to GFX.  This function searches
//   for textures in the current script loading directory if no path is 
//   specified in the texture script parameter.
//--------------------------------------------------------------------------
GFXTexHandle Material::createTexture(const char* filename, GFXTextureProfile* profile)
{
    // if '/', then path is specified, open normally
    if (dStrstr(filename, "/"))
    {
        return GFXTexHandle(filename, profile);
    }

    // otherwise, 
 //   const char *curScriptFile = Con::getCurrentFile();  // current script file - local materials.cs
    const char* curScriptFile = Con::getVariable("Con::File");  // current script file - local materials.cs
    const char* path = dStrrchr(curScriptFile, '/');

    U32 fileStrLen = path - curScriptFile + 1;

    char fullFilename[128];
    dStrncpy(fullFilename, curScriptFile, fileStrLen);
    fullFilename[fileStrLen] = '\0';
    dStrcat(fullFilename, filename);

    return GFXTexHandle(fullFilename, profile);
}

//--------------------------------------------------------------------------
// getMaterialSet - returns "MaterialSet" SimSet
//--------------------------------------------------------------------------
SimSet* Material::getMaterialSet()
{
    if (gMatSet)
    {
        return gMatSet;
    }

    gMatSet = static_cast<SimSet*>(Sim::findObject("MaterialSet"));
    return gMatSet;
}

LightInfo* Material::getDebugLight()
{
    LightInfo *light = getCurrentClientSceneGraph()->getLightManager()->sgGetSpecialLight(LightManager::sgSunLightType);
    if (!light)
        return &smDebugLight;

    dMemcpy(&smDebugLight, light, sizeof(smDebugLight));
    smDebugLight.sgTempModelInfo[0] = 0.0f;

    return &smDebugLight;
}
