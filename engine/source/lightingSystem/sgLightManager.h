//-----------------------------------------------
// Synapse Gaming - Lighting System
// Copyright � Synapse Gaming 2003
// Written by John Kabus
//-----------------------------------------------
#ifndef _SGLIGHTMANAGER_H_
#define _SGLIGHTMANAGER_H_

#ifndef _MMATH_H_
#include "math/mMath.h"
#endif
#ifndef _COLOR_H_
#include "core/color.h"
#endif
#ifndef _TVECTOR_H_
#include "core/tVector.h"
#endif
#ifndef _DATACHUNKER_H_
#include "core/dataChunker.h"
#endif
#include "console/console.h"
#include "console/consoleTypes.h"
#include "core/stringTable.h"
#include "gfx/gfxDevice.h"
#include "sceneGraph/lightInfo.h"

class Material;
class ProcessedMaterial;
class SceneObject;

static void sgFindObjectsCallback(SceneObject* obj, void* val)
{
    Vector<SceneObject*>* list = (Vector<SceneObject*>*)val;
    list->push_back(obj);
}


//-----------------------------------------------
// Original name maintained due to widespread use...



class LightInfoList : public Vector<LightInfo*>
{
public:
    void sgRegisterLight(LightInfo* light);
    void sgUnregisterLight(LightInfo* light);
};

class LightInfoDual
{
public:
    LightInfo* sgLightPrimary;
    LightInfo* sgLightSecondary;

    LightInfoDual()
    {
        sgLightPrimary = NULL;
        sgLightSecondary = NULL;
    }
};

class LightInfoDualList : public Vector<LightInfoDual>
{
public:
};


//-----------------------------------------------
// Original name maintained due to widespread use...

class LightManager
{
public:
    enum sgSpecialLightTypesEnum
    {
        sgSunLightType,

        sgSpecialLightTypesCount
    };

    LightManager()
    {
        dMemset(&sgSpecialLights, 0, sizeof(sgSpecialLights));
        sgInit();
    }


    // Returns a "default" light info that callers should not free.  Used for instances where we don't actually care about
    // the light
    virtual LightInfo* getDefaultLight();

    // registered before scene traversal...
    void sgRegisterGlobalLight(LightInfo* light, SimObject* obj, bool zonealreadyset);
    void sgUnregisterGlobalLight(LightInfo* light) { sgRegisteredGlobalLights.sgUnregisterLight(light); }
    // registered per object...
    void sgRegisterLocalLight(LightInfo* light) { sgRegisteredLocalLights.sgRegisterLight(light); }
    void sgUnregisterLocalLight(LightInfo* light) { sgRegisteredLocalLights.sgUnregisterLight(light); }

    void sgRegisterGlobalLights(bool staticlighting);
    void sgUnregisterAllLights();

    /// Returns all unsorted and un-scored lights (both global and local).
    void sgGetAllUnsortedLights(LightInfoList& list);
    /// Copies the best lights list - this DOESN'T find the lights!  Call
    /// sgSetupLights to populate the list.
    void sgGetBestLights(LightInfoList& list)
    {
        list.clear();
        list.merge(sgBestLights);
    }
    /// Accepts a pre-filtered list instead of using the best lights list
    /// so interiors can filter lights against zones and then build the
    /// the dual list...
    void sgBuildDualLightLists(const LightInfoList& list, LightInfoDualList& listdual);

    /// For DST objects.  Finds the best lights
    /// including a composite based on the environmental
    /// ambient lighting amount *and installs them in OpenGL*.
    void sgSetupLights(SceneObject* obj);
    /// For the terrain and Atlas.  Finds the best
    /// lights in the viewing area based on distance to camera.
    void sgSetupLights(SceneObject* obj, const Point3F& camerapos,
        const Point3F& cameradir, F32 viewdist, S32 maxlights);
    /// Finds the best lights that overlap with the bounding box
    /// based on the box center.
    void sgSetupLights(SceneObject* obj, const Box3F& box, S32 maxlights);
    /// Reset the best lights list and all associated data.
    void sgResetLights();

    /// Sets shader constants / textures for light infos
    virtual void setLightInfo(ProcessedMaterial* pmat, const Material* mat, const SceneGraphData& sgData, U32 pass);

    /// Sets the blend state for a lighting pass
    virtual void setLightingBlendFunc();

    /// Allows us to set textures during the Material::setTextureStage call
    virtual bool setTextureStage(const SceneGraphData& sgData, const U32 currTexFlag, const U32 textureSlot);


private:
    LightInfo* sgSpecialLights[sgSpecialLightTypesCount];
public:
    LightInfo* sgGetSpecialLight(sgSpecialLightTypesEnum type);
    void sgSetSpecialLight(sgSpecialLightTypesEnum type, LightInfo* light) { sgSpecialLights[type] = light; }
    void sgClearSpecialLights();

private:
    // registered before scene traversal...
    LightInfoList sgRegisteredGlobalLights;
    // registered per object...
    LightInfoList sgRegisteredLocalLights;

    // best lights per object...
    LightInfoList sgBestLights;
    void sgFindBestLights(const Box3F& box, S32 maxlights, const Point3F& viewdir, bool camerabased);

    // used in DTS lighting...
    void sgScoreLight(LightInfo* light, const Box3F& box, const SphereF& sphere, bool camerabased);

public:
    enum lightingProfileQualityType
    {
        // highest quality - for in-game and final tweaks...
        lpqtProduction = 0,
        // medium quality - for lighting layout...
        lpqtDesign = 1,
        // low quality - for object placement...
        lpqtDraft = 2
    };
    enum sgLightingPropertiesEnum
    {
        sgReceiveSunLightProp,
        sgAdaptiveSelfIlluminationProp,
        sgCustomAmbientLightingProp,
        sgCustomAmbientForSelfIlluminationProp,

        sgPropertyCount
    };
    static bool sgGetProperty(U32 prop)
    {
        AssertFatal((prop < sgPropertyCount), "Invalid property type!");
        return sgLightingProperties[prop];
    }
    static void sgSetProperty(U32 prop, bool val)
    {
        AssertFatal((prop < sgPropertyCount), "Invalid property type!");
        sgLightingProperties[prop] = val;
    }
    static bool sgAllowDiffuseCustomAmbient() { return sgLightingProperties[sgCustomAmbientLightingProp] && (!sgLightingProperties[sgCustomAmbientForSelfIlluminationProp]); }
    static bool sgAllowAdaptiveSelfIllumination() { return sgLightingProperties[sgAdaptiveSelfIlluminationProp]; }
    static bool sgAllowCollectSelfIlluminationColor() { return !sgLightingProperties[sgCustomAmbientLightingProp]; }
    static bool sgAllowReceiveSunLight() { return sgLightingProperties[sgReceiveSunLightProp] && (!sgAllowDiffuseCustomAmbient()); }
private:
    static bool sgLightingProperties[sgPropertyCount];
    static U32 sgLightingProfileQuality;
    static bool sgLightingProfileAllowShadows;
    static LightInfo sgDefaultLight;
    //static bool sgDetailMaps;
    static bool sgUseDynamicShadows;
    static U32 sgDynamicShadowQuality;
    static bool sgUseDynamicLightingDualOptimization;
    //static bool sgUseDynamicShadowSelfShadowing;
    //static bool sgUseDynamicShadowSelfShadowingOnPS_2_0;

    static bool sgUseSelfIlluminationColor;
    static ColorF sgSelfIlluminationColor;
    //static bool sgDynamicDTSVectorLighting;
    //static bool sgDynamicParticleSystemLighting;
    static bool sgFilterZones;
    static S32 sgZones[2];
    //static S32 sgShadowDetailSize;
    static S32 sgMaxBestLights;
    static bool sgInGUIEditor;

    // user prefs...
    static bool sgUseDynamicRangeLighting;
    static bool sgUseDRLHighDynamicRange;

    // mission properties...
    static bool sgAllowDynamicRangeLighting;
    static bool sgAllowDRLHighDynamicRange;
    static bool sgAllowDRLBloom;
    static bool sgAllowDRLToneMapping;

public:
    static U32 sgDynamicShadowDetailSize;
    static bool sgMultipleDynamicShadows;
    static bool sgShowCacheStats;

    static F32 sgDRLTarget;
    static F32 sgDRLMax;
    static F32 sgDRLMin;
    static F32 sgDRLMultiplier;
    static F32 sgBloomCutOff;
    static F32 sgBloomAmount;
    static F32 sgBloomSeedAmount;
    static F32 sgAtlasMaxDynamicLights;

    static bool sgAllowDynamicShadows() { return sgUseDynamicShadows; }
    static U32 sgGetDynamicShadowQuality()
    {
        F32 psversion = GFX->getPixelShaderVersion();
        if ((psversion >= 3.0) && (sgDynamicShadowQuality < 1))
            return 0;
        if ((psversion >= 2.0) && (sgDynamicShadowQuality < 2))
            return 1;
        return 2;

        //F32 psversion = GFX->getPixelShaderVersion();
        //if((psversion >= 3.0) || ((psversion >= 2.0) && sgUseDynamicShadowSelfShadowingOnPS_2_0))
        //	return sgUseDynamicShadowSelfShadowing;
        //return false;
    }
    static bool sgAllowDynamicLightingDualOptimization() { return sgUseDynamicLightingDualOptimization; }
    static bool sgAllowDRLSystem() { return sgUseDynamicRangeLighting && (sgAllowDynamicRangeLighting || sgAllowDRLBloom) && !sgInGUIEditor; }
    static bool sgAllowFullDynamicRangeLighting() { return sgAllowDRLSystem() && sgAllowDynamicRangeLighting; }
    static bool sgAllowFullHighDynamicRangeLighting() { return sgAllowDRLSystem() && sgUseDRLHighDynamicRange && sgAllowDRLHighDynamicRange; }
    static bool sgAllowBloom() { return sgAllowDRLSystem() && sgAllowDRLBloom; }
    static bool sgAllowToneMapping() { return sgAllowDRLSystem() && sgAllowDRLToneMapping; }
    static ColorF sgGetSelfIlluminationColor(ColorF defaultcol)
    {
        if (sgUseSelfIlluminationColor)
            return sgSelfIlluminationColor;
        return defaultcol;
    }
    static void sgSetAllowDynamicRangeLighting(bool enable) { sgAllowDynamicRangeLighting = enable; }
    static void sgSetAllowHighDynamicRangeLighting(bool enable) { sgAllowDRLHighDynamicRange = enable; }
    static void sgSetAllowDRLBloom(bool enable) { sgAllowDRLBloom = enable; }
    static void sgSetAllowDRLToneMapping(bool enable) { sgAllowDRLToneMapping = enable; }

    //static GFXTexHandle sgDLightMap;
    static void sgInit();
    //static bool sgAllowDetailMaps();
    static bool sgAllowShadows() { return sgLightingProfileAllowShadows; }
    static bool sgAllowFullLightMaps() { return (sgLightingProfileQuality == lpqtProduction); }
    static U32 sgGetLightMapScale()
    {
        if (sgLightingProfileQuality == lpqtDesign) return 2;
        if (sgLightingProfileQuality == lpqtDraft) return 4;
        return 1;
    }
    static void sgGetFilteredLightColor(ColorF& color, ColorF& ambient, S32 objectzone);
    // adds proper support for self-illumination...
    //static void sgSetAmbientSelfIllumination(LightInfo *lightinfo, F32 *lightColor,
    //		F32 *ambientColor);
    static void sgSetupZoneLighting(bool enable, SimObject* sobj);

private:
    static S32 QSORT_CALLBACK sgSortLightsByAddress(const void*, const void*);
    static S32 QSORT_CALLBACK sgSortLightsByScore(const void*, const void*);
};

class sgRelightFilter
{
public:
    static bool sgFilterRelight;
    static bool sgFilterRelightVisible;
    static bool sgFilterRelightByDistance;
    //static bool sgFilterRelightByVisiblity;
    static F32 sgFilterRelightByDistanceRadius;
    static Point3F sgFilterRelightByDistancePosition;
    static void sgInit();
    static bool sgAllowLighting(const Box3F& box, bool forcefilter);
    static void sgRenderAllowedObjects(void* worldeditor);
};

class sgStatistics
{
public:
    static U32 sgInteriorLexelCount;
    static F32 sgInteriorLexelTime;
    static U32 sgInteriorObjectCount;
    static U32 sgInteriorObjectIncludedCount;
    static U32 sgInteriorObjectIlluminationCount;
    static U32 sgInteriorSurfaceIncludedCount;
    static U32 sgInteriorSurfaceIlluminationCount;
    static U32 sgInteriorSurfaceIlluminatedCount;
    static U32 sgInteriorOccluderCount;
    static U32 sgTerrainLexelCount;
    static F32 sgTerrainLexelTime;
    //static U32 sgTerrainOccluderCount;

    static void sgClear();
    static void sgPrint();
};


#endif//_SGLIGHTMANAGER_H_
