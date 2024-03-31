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
#include "customMaterial.h"
#include <json/json.h>
#include <sceneGraph/sceneGraph.h>

//--------------------------------------------------------------------------
// getNormalizeCube - static method, returns a normalization cubemap
//--------------------------------------------------------------------------
GFXCubemap * Material::getNormalizeCube()
{
    if(normalizeCube)
        return normalizeCube;
    normalizeCube = GFX->createCubemap();
    normalizeCube->initNormalize(64);
    return normalizeCube;
}

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
   { RenderInstManager::RenderBinTypes::MarbleShadow,    "MarbleShadow" },
   { RenderInstManager::RenderBinTypes::Marble,    "Marble" },
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

GFXCubemapHandle Material::normalizeCube;

//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------
Material::Material()
{
    for (U32 i = 0; i < MAX_STAGES; i++)
    {
        diffuse[i].set(1.0, 1.0, 1.0, 1.0);
        specular[i].set(1.0, 1.0, 1.0, 1.0);
        colorMultiply[i].set(0.0f, 0.0f, 0.0f, 0.0f);
        scrollSpeed[i] = 0.0;
        scrollDir[i].set(0.0f, 0.0f);
        specularPower[i] = 8;
        pixelSpecular[i] = false;
        vertexSpecular[i] = false;
        exposure[i] = 1;
        glow[i] = false;
        emissive[i] = false;
    }

    mType = Base;
    mIsIFL = false;

    castsShadow = true;
    breakable = false;
    doubleSided = false;
    attenuateBackFace = false;
    preload = false;

    translucent = false;
    subPassTranslucent = false;
    translucentBlendOp = LerpAlpha;
    translucentZWrite = false;

    alphaTest = true;
    alphaRef = 1;

    planarReflection = false;

    mCubemapData = NULL;
    dynamicCubemap = NULL;
    cubemapName = NULL;

    dMemset( mPath, 0, sizeof(mPath) );

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
    mLastUpdateTime = 0;
    mapTo = NULL;
}

//--------------------------------------------------------------------------
// Init fields
//--------------------------------------------------------------------------
void Material::initPersistFields()
{
    Parent::initPersistFields();

    addField("diffuse", TypeColorF, Offset(diffuse, Material), MAX_STAGES);
    addField("colorMultiply", TypeColorF, Offset(colorMultiply, Material), MAX_STAGES);
    addField("specular", TypeColorF, Offset(specular, Material), MAX_STAGES);
    addField("specularPower", TypeF32, Offset(specularPower, Material), MAX_STAGES);
    addField("pixelSpecular", TypeBool, Offset(pixelSpecular, Material), MAX_STAGES);

    addField("exposure", TypeS32, Offset(exposure, Material), MAX_STAGES);

    addField("glow", TypeBool, Offset(glow, Material), MAX_STAGES);
    addField("emissive", TypeBool, Offset(emissive, Material), MAX_STAGES);

    addField("translucent", TypeBool, Offset(translucent, Material));
    addField("translucentBlendOp", TypeEnum, Offset(translucentBlendOp, Material), 1, &mBlendOpTable);
    addField("translucentZWrite", TypeBool, Offset(translucentZWrite, Material));

    addField("alphaTest",    TypeBool, Offset(alphaTest,  Material));
    addField("alphaRef",     TypeS32,  Offset(alphaRef,   Material));

    addField("castsShadow", TypeBool, Offset(castsShadow, Material));
    addField("breakable", TypeBool, Offset(breakable, Material));
    addField("doubleSided", TypeBool, Offset(doubleSided, Material));
    addField("attenuateBackFace",    TypeBool, Offset(attenuateBackFace,   Material));
    addField("preload",        TypeBool, Offset(preload, Material));

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
    addField("baseTex", TypeString, Offset(baseTexFilename, Material), MAX_STAGES);
    addField("detailTex", TypeString, Offset(detailFilename, Material), MAX_STAGES);
    addField("bumpTex", TypeString, Offset(bumpFilename, Material), MAX_STAGES);
    addField("envTex", TypeString, Offset(envFilename, Material), MAX_STAGES);

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
    addField("noiseTexFileName", TypeString, Offset(noiseTexFileName, Material));
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

    if( translucentBlendOp >= NumBlendTypes || translucentBlendOp < 0 )
    {
        Con::errorf( "Invalid blend op in material: %s", getName() );
        translucentBlendOp = LerpAlpha;
    }

    SimSet* matSet = getMaterialSet();
    if (matSet)
    {
        matSet->addObject((SimObject*)this);
    }

    // save the current script path for texture lookup later
    const char *curScriptFile = Con::getVariable("Con::File");  // current script file - local materials.cs
    const char *path = dStrrchr( curScriptFile, '/' );
    U32 fileStrLen = path - curScriptFile + 1;
    dStrncpy( mPath, curScriptFile, fileStrLen );
    mPath[fileStrLen] = '\0';

    mapMaterial();

    return true;
}

//--------------------------------------------------------------------------
// On remove
//--------------------------------------------------------------------------
void Material::onRemove()
{
    SAFE_DELETE( normalizeCube );

    Parent::onRemove();
}


//--------------------------------------------------------------------------
// Update time
//--------------------------------------------------------------------------
void Material::updateTime()
{
    U32 curTime = Platform::getVirtualMilliseconds();
    if(curTime > mLastTime)
    {
        mDt =  (curTime - mLastTime) / 1000.0f;
        mLastTime = curTime;
        mAccumTime += mDt;
    }
}

bool Material::preloadTextures(Vector<const char*>& errorBuffer)
{
    bool found = true;
    for (int i = 0; i < MAX_STAGES; i++)
    {
        bool foundBaseTex = (!baseTexFilename[i] || didFindTexture(baseTexFilename[i]));
        if (!foundBaseTex)
        {
            //dSprintf(errorBuffer, errorBufferSize, "%s\n    Could not find base texture: %s", errorBuffer,
            //         baseTexFilename[i]);

            errorBuffer.push_back(baseTexFilename[i]);
        }
        found = found && foundBaseTex;

        bool foundDetail = (!detailFilename[i] || didFindTexture(detailFilename[i]));
        if (!foundDetail)
        {
            errorBuffer.push_back(detailFilename[i]);

            //dSprintf(errorBuffer, errorBufferSize, "%s\n    Could not find detail texture: %s", errorBuffer,
            //         detailFilename[i]);
        }
        found = found && foundDetail;

        bool foundBump = (!bumpFilename[i] || didFindTexture(bumpFilename[i]));
        if (!foundBump)
        {
            errorBuffer.push_back(bumpFilename[i]);
            //dSprintf(errorBuffer, errorBufferSize, "%s\n    Could not find bump texture: %s", errorBuffer,
            //         bumpFilename[i]);
        }
        found = found && foundBump;

        bool foundEnv = (!envFilename[i] || didFindTexture(envFilename[i]));
        if (!foundEnv)
        {
            errorBuffer.push_back(envFilename[i]);
            //dSprintf(errorBuffer, errorBufferSize, "%s\n    Could not find env texture: %s", errorBuffer,
            //         envFilename[i]);
        }
        found = found && foundEnv;
    }
    bool foundNoiseTex = (!noiseTexFileName || didFindTexture(noiseTexFileName));
    if (!foundNoiseTex)
    {
        errorBuffer.push_back(noiseTexFileName);
        //dSprintf(errorBuffer, errorBufferSize, "%s\n    Could not find noise texture: %s", errorBuffer,
        //         noiseTexFileName);
    }
    found = found && foundNoiseTex;

    if (mCubemapData != NULL && !dynamicCubemap)
    {
        for (int i = 0; i < 6; i++)
        {
            bool foundCubemap = (!mCubemapData->cubeFaceFile[i] || didFindTexture(mCubemapData->cubeFaceFile[i]));
            if (!foundCubemap)
            {
                errorBuffer.push_back(mCubemapData->cubeFaceFile[i]);
                //dSprintf(errorBuffer, errorBufferSize, "%s\n    Could not find cubemap face texture: %s", errorBuffer,
                //         mCubemapData->cubeFaceFile[i]);
            }
            found = found && foundCubemap;
        }
    }
    return found;
}

bool Material::didFindTexture(const char* filename)
{
    const char* searchFilename = filename;

    if (filename[0] == '.')
    {
        char fullFilename[128];
        dStrncpy(fullFilename, mPath, dStrlen(mPath) + 1);
        dStrcat(fullFilename, filename);
        searchFilename = fullFilename;
    }

    ResourceObject* ro = GBitmap::findBmpResource(searchFilename);
    if (ro)
    {
        return true;
    }

    // Find and load the texture.
    GBitmap* bmp = GBitmap::load(searchFilename);
    return bmp != NULL;
}

void Material::updateTimeBasedParams()
{
    if (mLastUpdateTime != mLastTime)
    {
        for (U32 i = 0; i < MAX_STAGES; i++)
        {
            scrollOffset[i] += scrollDir[i] * scrollSpeed[i] * mDt;
            rotPos[i] += rotSpeed[i] * mDt;
            wavePos[i] += waveFreq[i] * mDt;
        }
        mLastUpdateTime = mLastTime;
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

    if( mapTo && dStrstr( mapTo, ".ifl" ) )
    {
        mIsIFL = true;
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
/*GFXTexHandle Material::createTexture(const char* filename, GFXTextureProfile* profile)
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
}*/

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

bool loadMaterialsFromJson(const char* path)
{
    Con::errorf("Loading materials from JSON file: %s", path);
    
    Stream* fs = ResourceManager->openStream(path);
    if (fs == NULL) return false;

    U32 size = fs->getStreamSize();

    char* jsonBuf = new char[size + 1];

    if (!fs->read(size, jsonBuf))
    {
        delete[] jsonBuf;
        return false;
    }
    jsonBuf[size] = '\0';
    ResourceManager->closeStream(fs);

    Json::Value root;
    Json::CharReaderBuilder builder;
    Json::CharReader* reader = builder.newCharReader();

    std::string errs;
    if (!reader->parse(jsonBuf, jsonBuf + size, &root, &errs)) 
    {
        delete reader;
        delete[] jsonBuf;
        Con::errorf("JSON Parse error: %s", errs.c_str());
        return false;
    }
    else 
    {
        delete reader;
        delete[] jsonBuf;
        
        Json::Value materialDict = root["materials"];
        Json::Value shaderDict = root["shaders"];
        Json::Value cubemapDict = root["cubemaps"];

        // Do shaders first
        if (!shaderDict.isNull())
        {
            for (Json::ValueIterator it = shaderDict.begin(); it != shaderDict.end(); it++) {
                Json::Value key = it.key();
                Json::Value val = *it;

                if (Sim::findObject(key.asCString()) != NULL)
                    continue; // Don't add

                ShaderData* shader = new ShaderData();
                shader->assignName(key.asCString());
                shader->pixVersion = val.get("pixVersion", 1.0).asFloat();
                shader->DXPixelShaderName = StringTable->insert(val.get("pixelShader", "").asCString(), true);
                shader->DXVertexShaderName = StringTable->insert(val.get("vertexShader", "").asCString(), true);
                shader->useDevicePixVersion = val.get("useDevicePixVersion", false).asBool();
                shader->registerObject(); // Finally create the shader

                // fix paths
                char fpath[256];
                const char* seppath = dStrrchr(path, '/');
                U32 fileStrLen = seppath - path + 1;
                dStrncpy(fpath, path, fileStrLen);
                fpath[fileStrLen] = '\0';

                shader->setPath(fpath);
            }
        }

        // then cubemaps
        if (!cubemapDict.isNull())
        {
            for (Json::ValueIterator it = cubemapDict.begin(); it != cubemapDict.end(); it++) {
                Json::Value key = it.key();
                Json::Value val = *it;

                if (Sim::findObject(key.asCString()) != NULL)
                    continue; // Don't add

                CubemapData* cubeMap = new CubemapData();
                cubeMap->assignName(key.asCString());
                for (int i = 0; i < 6; i++)
                {
                    cubeMap->cubeFaceFile[i] = StringTable->insert(val[i].asCString());
                }

                // fix paths
                char fpath[256];
                const char* seppath = dStrrchr(path, '/');
                U32 fileStrLen = seppath - path + 1;
                dStrncpy(fpath, path, fileStrLen);
                fpath[fileStrLen] = '\0';
                dStrncpy(cubeMap->mPath, fpath, 256);

                cubeMap->registerObject();
            }
        }

        // Now do the materials
        if (!materialDict.isNull())
        {
            for (Json::ValueIterator it = materialDict.begin(); it != materialDict.end(); it++) {
                Json::Value key = it.key();
                Json::Value val = *it;

                if (Sim::findObject(key.asCString()) != NULL)
                    continue; // Don't add

                bool isCustom = val.get("custom", false).asBool();

                Material* mat = isCustom ? new CustomMaterial() : new Material();
                mat->assignName(key.asCString());
                mat->setModStaticFields(true);

                // Non array members
                mat->translucent = val.get("translucent", false).asBool();
                std::string tbo = val.get("translucentBlendOp", "LerpAlpha").asString();
                for (int i = 0; i < 6; i++)
                {
                    if (tbo == gBlendOpEnums[i].label)
                    {
                        mat->translucentBlendOp = (Material::BlendOp)gBlendOpEnums[i].index;
                        break;
                    }
                }
                mat->translucentZWrite = val.get("translucentZWrite", false).asBool();
                mat->alphaTest = val.get("alphaTest", true).asBool();
                mat->alphaRef = val.get("alphaRef", 1).asFloat();
                mat->castsShadow = val.get("castsShadow", true).asBool();
                mat->breakable = val.get("breakable", false).asBool();
                mat->doubleSided = val.get("doubleSided", false).asBool();
                mat->attenuateBackFace = val.get("attenuateBackFace", false).asBool();
                mat->preload = val.get("preload", false).asBool();
                std::string renderbin = val.get("renderBin", "Begin").asString();
                for (int i = 0; i < 17; i++)
                {
                    if (renderbin == gRenderBinEnums[i].label)
                    {
                        mat->renderBin = (RenderInstManager::RenderBinTypes)gRenderBinEnums[i].index;
                        break;
                    }
                }
                mat->cubemapName = val["cubemap"].isString() ? StringTable->insert(val["cubemap"].asCString(), false) : NULL;
                mat->dynamicCubemap = val.get("dynamicCubemap", false).asBool();
                mat->planarReflection = val.get("planarReflection", false).asBool();
                mat->mapTo = val["mapTo"].isString() ? StringTable->insert(val["mapTo"].asCString(), false) : NULL;
                mat->friction = val.get("friction", 1.0).asFloat();
                mat->restitution = val.get("restitution", 1.0).asFloat();
                mat->force = val.get("force", 0.0).asFloat();
                mat->sound = val.get("sound", -1).asInt();
                mat->softwareMipOffset = val.get("softwareMipOffset", 0.0).asFloat();
                mat->noiseTexFileName = val["noiseTexFileName"].isString() ? StringTable->insert(val["noiseTexFileName"].asCString(), false) : NULL;

                // Array members

#define READ_ARRAYED_FIELD(fname) \
{ \
    Json::Value aval = val[fname]; \
    if (aval.isArray()) \
    { \
        for (int i = 0; i < aval.size(); i++) \
        { \
            Json::Value elem = aval[i]; \
            if (!elem.isNull()) { \
                char* retBuf = Con::getReturnBuffer(1024); \
                dSprintf(retBuf, 1024, "%d", i); \
                mat->setDataField(fname ##_ts, StringTable->insert(retBuf, false), elem.asString().c_str()); \
            } \
        } \
    } \
}
                READ_ARRAYED_FIELD("diffuse");
                READ_ARRAYED_FIELD("colorMultiply");
                READ_ARRAYED_FIELD("specular");
                READ_ARRAYED_FIELD("specularPower");
                READ_ARRAYED_FIELD("pixelSpecular");
                READ_ARRAYED_FIELD("exposure");
                READ_ARRAYED_FIELD("glow");
                READ_ARRAYED_FIELD("emissive");
                READ_ARRAYED_FIELD("scrollSpeed");
                READ_ARRAYED_FIELD("scrollDir");
                READ_ARRAYED_FIELD("rotSpeed");
                READ_ARRAYED_FIELD("rotPivotOffset");
                READ_ARRAYED_FIELD("animFlags");
                READ_ARRAYED_FIELD("waveType");
                READ_ARRAYED_FIELD("waveFreq");
                READ_ARRAYED_FIELD("waveAmp");
                READ_ARRAYED_FIELD("sequenceFramePerSec");
                READ_ARRAYED_FIELD("sequenceSegmentSize");
                READ_ARRAYED_FIELD("baseTex");
                READ_ARRAYED_FIELD("detailTex");
                READ_ARRAYED_FIELD("bumpTex");
                READ_ARRAYED_FIELD("envTex");
                READ_ARRAYED_FIELD("texCompression");

                // Custom non array members
                if (isCustom)
                {
                    CustomMaterial* cmat = dynamic_cast<CustomMaterial*>(mat);
                    cmat->mVersion = val.get("version", 1.1).asFloat();
                    std::string bo = val.get("blendOp", "Add").asString();
                    for (int i = 0; i < 6; i++)
                    {
                        if (tbo == gBlendOpEnums[i].label)
                        {
                            cmat->blendOp = (Material::BlendOp)gBlendOpEnums[i].index;
                            break;
                        }
                    }
                    cmat->refract = val.get("refract", false).asBool();
                    cmat->mShaderDataName = val["shader"].isString() ? StringTable->insert(val["shader"].asCString(), false) : NULL;
                    const char* fallbackName = val["fallback"].isString() ? val["fallback"].asCString() : NULL;
                    if (fallbackName != NULL)
                        cmat->fallback = dynamic_cast<Material*>(Sim::findObject(fallbackName));

                    // Custom array members
                    READ_ARRAYED_FIELD("texture");
                    READ_ARRAYED_FIELD("pass");
                }

                // Create the object
                mat->setModStaticFields(false);
                mat->registerObject();

                // Fix paths
                const char* seppath = dStrrchr(path, '/');
                U32 fileStrLen = seppath - path + 1;
                dStrncpy(mat->getPath(), path, fileStrLen);
                mat->getPath()[fileStrLen] = '\0';
            }

            return true;
        }
    }   
}

ConsoleFunction(loadMaterialJson, bool, 2, 2, "loadMaterialJson(jsonFile)")
{
    char path[1024];
    Con::expandScriptFilename(path, 1024, argv[1]);
    return loadMaterialsFromJson(path);
}