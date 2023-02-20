//-----------------------------------------------
// Synapse Gaming - Lighting System
// Copyright � Synapse Gaming 2003
// Written by John Kabus
//-----------------------------------------------
#include "lightingSystem/sgLighting.h"

#include "gfx/gfxDevice.h"

#include "lightingSystem/sgLightManager.h"
#include "lightingSystem/sgLightingModel.h"
#include "lightingSystem/sgMissionLightingFilter.h"
#include "sceneGraph/lightManager.h"
#include SHADER_CONSTANT_INCLUDE_FILE
#include "interior/interiorInstance.h"
#include "terrain/terrData.h"
#include "game/shadow.h"
#include "lightingSystem/sgLightObject.h"
#include "sim/netConnection.h"
#include "editor/worldEditor.h"
#include "materials/processedCustomMaterial.h"


bool LightManager::sgLightingProperties[sgPropertyCount];
bool LightManager::sgUseSelfIlluminationColor = false;
ColorF LightManager::sgSelfIlluminationColor = ColorF(0.0f, 0.0f, 0.0f);
//bool LightManager::sgDynamicDTSVectorLighting = false;
bool LightManager::sgFilterZones = false;
S32 LightManager::sgZones[2] = { -1, -1 };
//S32 LightManager::sgShadowDetailSize = 0;
//bool LightManager::sgDynamicParticleSystemLighting = true;
U32 LightManager::sgLightingProfileQuality = LightManager::lpqtProduction;
bool LightManager::sgLightingProfileAllowShadows = true;
LightInfo LightManager::sgDefaultLight;
//bool LightManager::sgDetailMaps = true;
//GFXTexHandle LightManager::sgDLightMap = NULL;
S32 LightManager::sgMaxBestLights = 10;
bool LightManager::sgUseDynamicShadows = true;
bool LightManager::sgUseDynamicLightingDualOptimization = true;
//bool LightManager::sgUseDynamicShadowSelfShadowing = true;
//bool LightManager::sgUseDynamicShadowSelfShadowingOnPS_2_0 = true;

bool LightManager::sgUseDynamicRangeLighting = true;
bool LightManager::sgUseDRLHighDynamicRange = false;
bool LightManager::sgAllowDynamicRangeLighting = false;
bool LightManager::sgAllowDRLHighDynamicRange = false;
bool LightManager::sgAllowDRLBloom = false;
bool LightManager::sgAllowDRLToneMapping = false;

U32 LightManager::sgDynamicShadowDetailSize = 0;
U32 LightManager::sgDynamicShadowQuality = 0;
bool LightManager::sgMultipleDynamicShadows = true;
bool LightManager::sgShowCacheStats = false;
F32 LightManager::sgDRLTarget = 0.25;
F32 LightManager::sgDRLMax = 2.0;
F32 LightManager::sgDRLMin = 0.85;
F32 LightManager::sgDRLMultiplier = 1.0;
F32 LightManager::sgBloomCutOff = 0.7;
F32 LightManager::sgBloomAmount = 2.0;
F32 LightManager::sgBloomSeedAmount = 0.32;
F32 LightManager::sgAtlasMaxDynamicLights = 16;
bool LightManager::sgInGUIEditor = false;

bool sgRelightFilter::sgFilterRelight = false;
bool sgRelightFilter::sgFilterRelightVisible = true;
bool sgRelightFilter::sgFilterRelightByDistance = true;
//bool sgRelightFilter::sgFilterRelightByVisiblity;
F32 sgRelightFilter::sgFilterRelightByDistanceRadius = 60;
Point3F sgRelightFilter::sgFilterRelightByDistancePosition;

U32 sgStatistics::sgInteriorLexelCount = 0;
F32 sgStatistics::sgInteriorLexelTime = 0;
U32 sgStatistics::sgInteriorObjectCount = 0;
U32 sgStatistics::sgInteriorObjectIncludedCount = 0;
U32 sgStatistics::sgInteriorObjectIlluminationCount = 0;
U32 sgStatistics::sgInteriorSurfaceIncludedCount = 0;
U32 sgStatistics::sgInteriorSurfaceIlluminationCount = 0;
U32 sgStatistics::sgInteriorSurfaceIlluminatedCount = 0;
U32 sgStatistics::sgInteriorOccluderCount = 0;
U32 sgStatistics::sgTerrainLexelCount = 0;
F32 sgStatistics::sgTerrainLexelTime = 0;
//U32 sgStatistics::sgTerrainOccluderCount = 0;


LightInfo::LightInfo()
{
    mType = Vector;
    mPos = Point3F(0, 0, 0);
    mDirection = Point3F(0, 0, 1);
    mColor = ColorF(0, 0, 0);
    mAmbient = ColorF(0, 0, 0);
    mRadius = 1;
    mScore = 0;

    // default to this for weapon fire, explosions, ...
    sgSupportedFeatures = sgNoSpecCube;
    sgSpotAngle = 90.0;
    sgAssignedToTSObject = false;
    sgCastsShadows = true;
    sgDiffuseRestrictZone = false;
    sgAmbientRestrictZone = false;
    sgZone[0] = -1;
    sgZone[1] = -1;
    sgLocalAmbientAmount = 0.0;
    sgSmoothSpotLight = false;
    sgDoubleSidedAmbient = false;
    sgAssignedToParticleSystem = false;
    sgLightingModelName = NULL;
    sgUseNormals = true;
    sgSpotPlane = PlaneF(0, 0, 0, 0);

    sgLightingTransform.identity();
}

bool LightInfo::sgIsInZone(S32 zone)
{
    if ((zone == sgZone[0]) || (zone == sgZone[1]))
        return true;
    return false;
}

bool LightInfo::sgAllowDiffuseZoneLighting(S32 zone)
{
    if (!sgDiffuseRestrictZone)
        return true;
    if (sgIsInZone(zone))
        return true;
    return false;
}

void LightInfoList::sgRegisterLight(LightInfo* light)
{
    if (!light)
        return;
    // just add the light, we'll try to scan for dupes later...
    push_back(light);
}

void LightInfoList::sgUnregisterLight(LightInfo* light)
{
    // remove all of them...
    LightInfoList& list = *this;
    for (U32 i = 0; i < list.size(); i++)
    {
        if (list[i] != light)
            continue;
        // this moves last to i, which allows
        // the search to continue forward...
        list.erase_fast(i);
        // want to check this location again...
        i--;
    }
}


//-----------------------------------------------



LightInfo* LightManager::getDefaultLight()
{
    return &sgDefaultLight;
}

//-----------------------------------------------


void LightManager::sgBuildDualLightLists(const LightInfoList& list, LightInfoDualList& listdual)
{
    LightInfoList pri, sec;

    // clean up...
    listdual.clear();

    // sort...
    for (U32 i = 0; i < list.size(); i++)
    {
        LightInfo* light = list[i];

        if ((light->mType == LightInfo::Vector) ||
            (light->mType == LightInfo::Ambient))
        {
            // non-group-able...
            listdual.increment();
            listdual.last().sgLightPrimary = light;
            listdual.last().sgLightSecondary = NULL;
            continue;
        }

        // sort...
        if (light->sgCanBeSecondary())
            sec.push_back(light);
        else
            pri.push_back(light);
    }

    // build using primary lights...
    // these can only go in slot one...
    while (pri.size())
    {
        listdual.increment();
        listdual.last().sgLightPrimary = pri[0];
        listdual.last().sgLightSecondary = NULL;
        pri.erase_fast(U32(0));

        if (sec.size() <= 0)
            continue;

        // piggyback any available secondary lights...
        listdual.last().sgLightSecondary = sec[0];
        sec.erase_fast(U32(0));
    }

    // use up any extra secondary lights...
    // treat these as primary lights...
    while (sec.size())
    {
        listdual.increment();
        listdual.last().sgLightPrimary = sec[0];
        listdual.last().sgLightSecondary = NULL;
        sec.erase_fast(U32(0));

        if (sec.size() <= 0)
            continue;

        // piggyback any available secondary lights...
        listdual.last().sgLightSecondary = sec[0];
        sec.erase_fast(U32(0));
    }
}

LightInfo* LightManager::sgGetSpecialLight(sgSpecialLightTypesEnum type)
{
    if (sgSpecialLights[type])
        return sgSpecialLights[type];
    // return a default light...
    return &sgDefaultLight;
}

void LightManager::sgRegisterGlobalLight(LightInfo* light, SimObject* obj, bool zonealreadyset)
{
    if (!zonealreadyset)
    {
        SceneObject* sceneobj = dynamic_cast<SceneObject*>(obj);
        if (sceneobj)
        {
            U32 count = getMin(sceneobj->getNumCurrZones(), U32(2));
            for (U32 i = 0; i < count; i++)
                light->sgZone[i] = sceneobj->getCurrZone(i);
        }
    }

    sgRegisteredGlobalLights.sgRegisterLight(light);

    // not here!!!
    /*if(light->mType == LightInfo::Vector)
    {
    if(sgGetSpecialLight(sgSunLightType) == &sgDefaultLight)
    sgSetSpecialLight(sgSunLightType, light);
    }*/
}

void LightManager::sgRegisterGlobalLights(bool staticlighting)
{
    // make sure we're clean...
    sgUnregisterAllLights();

    // ask all light objects to register themselves...
    SimSet* lightset = Sim::getLightSet();
    for (SimObject** itr = lightset->begin(); itr != lightset->end(); itr++)
        (*itr)->registerLights(this, staticlighting);
}

void LightManager::sgUnregisterAllLights()
{
    sgRegisteredGlobalLights.clear();
    sgRegisteredLocalLights.clear();

    dMemset(&sgSpecialLights, 0, sizeof(sgSpecialLights));
}

void LightManager::sgGetAllUnsortedLights(LightInfoList& list)
{
    list.clear();
    list.merge(sgRegisteredGlobalLights);
    list.merge(sgRegisteredLocalLights);

    // find dupes...
    dQsort(list.address(), list.size(), sizeof(LightInfo*), sgSortLightsByAddress);
    LightInfo* last = NULL;
    for (U32 i = 0; i < list.size(); i++)
    {
        if (list[i] == last)
        {
            list.erase(i);
            i--;
            continue;
        }
        last = list[i];
    }
}

void LightManager::sgSetupLights(SceneObject* obj)
{
    sgResetLights();

    bool outside = false;
    for (U32 i = 0; i < obj->getNumCurrZones(); i++)
    {
        if (obj->getCurrZone(i) == 0)
        {
            outside = true;
            break;
        }
    }

    sgSetProperty(sgReceiveSunLightProp, obj->receiveSunLight);
    sgSetProperty(sgAdaptiveSelfIlluminationProp, obj->useAdaptiveSelfIllumination);
    sgSetProperty(sgCustomAmbientLightingProp, obj->useCustomAmbientLighting);
    sgSetProperty(sgCustomAmbientForSelfIlluminationProp, obj->customAmbientForSelfIllumination);

    ColorF ambientColor;
    ColorF selfillum = obj->customAmbientLighting;

    LightInfo* sun = sgGetSpecialLight(sgSunLightType);
    if (obj->getLightingAmbientColor(&ambientColor))
    {
        const F32 directionalFactor = 0.5f;
        const F32 ambientFactor = 0.5f;

        dMemset(&obj->mLightingInfo.smAmbientLight, 0, sizeof(obj->mLightingInfo.smAmbientLight));

        LightInfo& light = obj->mLightingInfo.smAmbientLight;
        light.mType = LightInfo::Ambient;
        light.mDirection = VectorF(0.0, 0.0, -1.0);
        light.sgCastsShadows = sun ? sun->sgCastsShadows : false;

        // players, vehicles, ...
        if (obj->overrideOptions)
        {
            if (outside)
            {
                light.mType = LightInfo::Vector;
                light.mDirection = sun ? sun->mDirection : Point3F(0.f, 0.707f, -0.707f);
            }
            //else
            //{
            //light.mColor = ambientColor * directionalFactor;
            //light.mAmbient = ambientColor * ambientFactor;
            //}
        }// beyond here are the static dts options...
        else if (sgAllowDiffuseCustomAmbient())
        {
            light.mColor = obj->customAmbientLighting * 0.25;
            light.mAmbient = obj->customAmbientLighting * 0.75;
        }
        else if (sgAllowReceiveSunLight() && sun)
        {
            light.mType = LightInfo::Vector;
            if (outside)
                light.mDirection = sun->mDirection;
            if (obj->receiveLMLighting)
                light.mColor = ambientColor;// * 0.8f;
            else
                light.mColor = sun->mColor;
            light.mAmbient = sun->mAmbient;
            //light.mAmbient.red = sun->mAmbient.red * 0.346f;
            //light.mAmbient.green = sun->mAmbient.green * 0.588f;
            //light.mAmbient.blue = sun->mAmbient.blue * 0.070f;
        }
        else if (obj->receiveLMLighting)
        {
            light.mColor = ambientColor * directionalFactor;
            light.mAmbient = ambientColor * ambientFactor;
        }

        if (sgAllowCollectSelfIlluminationColor())
        {
            selfillum = light.mAmbient + light.mColor;
            selfillum.clamp();
        }

        light.mPos = light.mDirection * -10000.0;
        sgRegisterLocalLight(&obj->mLightingInfo.smAmbientLight);
    }

    // install assigned baked lights from simgroup...
    // step one get the objects...
    U32 i;
    NetConnection* connection = NetConnection::getConnectionToServer();

    for (i = 0; i < obj->lightIds.size(); i++)
    {
        SimObject* sim = connection->resolveGhost(obj->lightIds[i]);
        if (!sim)
            continue;

        sgLightObject* light = dynamic_cast<sgLightObject*>(sim);
        if (!light)
            continue;

        sgLightObjectData* data = static_cast<sgLightObjectData*>(light->getDataBlock());
        if ((data) && (data->sgStatic) && (data->mLightOn) && (light->mEnable))
        {
            light->mLight.sgAssignedToTSObject = true;
            sgRegisterLocalLight(&light->mLight);
        }
    }

    // step three install dynamic lights...
    sgUseSelfIlluminationColor = sgGetProperty(sgAdaptiveSelfIlluminationProp);
    if (sgUseSelfIlluminationColor)
        sgSelfIlluminationColor = selfillum;

    sgSetupZoneLighting(true, obj);

    sgFindBestLights(obj->getRenderWorldBox(), sgMaxBestLights, Point3F(0, 0, 0), false);
}

void LightManager::sgSetupLights(SceneObject* obj, const Point3F& camerapos,
    const Point3F& cameradir, F32 viewdist, S32 maxlights)
{
    sgResetLights();

    sgSetupZoneLighting(true, obj);

    Box3F box;
    box.max = camerapos + Point3F(viewdist, viewdist, viewdist);
    box.min = camerapos - Point3F(viewdist, viewdist, viewdist);
    sgFindBestLights(box, maxlights, cameradir, true);
}

void LightManager::sgSetupLights(SceneObject* obj, const Box3F& box, S32 maxlights)
{
    sgResetLights();
    sgSetupZoneLighting(true, obj);
    sgFindBestLights(box, maxlights, Point3F(0, 0, 0), true);
}

void LightManager::sgResetLights()
{
    sgSetupZoneLighting(false, NULL);
    sgRegisteredLocalLights.clear();
    sgBestLights.clear();
}

void LightManager::sgFindBestLights(const Box3F& box, S32 maxlights, const Point3F& viewdi, bool camerabased)
{
    sgBestLights.clear();

    // gets them all and removes any dupes...
    sgGetAllUnsortedLights(sgBestLights);

    SphereF sphere;
    box.getCenter(&sphere.center);
    sphere.radius = Point3F(box.max - sphere.center).len();

    for (U32 i = 0; i < sgBestLights.size(); i++)
        sgScoreLight(sgBestLights[i], box, sphere, camerabased);

    dQsort(sgBestLights.address(), sgBestLights.size(), sizeof(LightInfo*), sgSortLightsByScore);

    for (U32 i = 0; i < sgBestLights.size(); i++)
    {
        if ((sgBestLights[i]->mScore > 0) && (i < maxlights))
            continue;

        sgBestLights.setSize(i);
        break;
    }
}

void LightManager::sgScoreLight(LightInfo* light, const Box3F& box, const SphereF& sphere, bool camerabased)
{
    if (sgFilterZones && light->sgDiffuseRestrictZone)
    {
        bool allowdiffuse = false;
        if (sgZones[0] > -1)
        {
            if (light->sgAllowDiffuseZoneLighting(sgZones[0]))
                allowdiffuse = true;
            else if (sgZones[1] > -1)
            {
                if (light->sgAllowDiffuseZoneLighting(sgZones[1]))
                    allowdiffuse = true;
            }
        }

        if (!allowdiffuse)
        {
            light->mScore = 0;
            return;
        }
    }


    F32 distintensity = 1;
    F32 colorintensity = 1;
    F32 weight = SG_LIGHTMANAGER_DYNAMIC_PRIORITY;

    if (camerabased)
    {
        sgLightingModel& model = sgLightingModelManager::sgGetLightingModel(light->sgLightingModelName);
        model.sgSetState(light);
        F32 maxrad = model.sgGetMaxRadius(true);
        model.sgResetState();

        Point3F vect = sphere.center - light->mPos;
        F32 dist = vect.len();
        F32 distlightview = sphere.radius + maxrad;

        if (distlightview <= 0)
            distintensity = 0.0f;
        else
        {
            distintensity = 1.0f - (dist / distlightview);
            distintensity = mClampF(distintensity, 0.0f, 1.0f);
        }
    }
    else
    {
        // side test...
        if ((light->mType == LightInfo::Spot) || (light->mType == LightInfo::SGStaticSpot))
        {
            bool anyfront = false;
            F32 x, y, z;

            for (U32 i = 0; i < 8; i++)
            {
                if (i & 0x1)
                    x = box.max.x;
                else
                    x = box.min.x;

                if (i & 0x2)
                    y = box.max.y;
                else
                    y = box.min.y;

                if (i & 0x4)
                    z = box.max.z;
                else
                    z = box.min.z;

                if (light->sgSpotPlane.whichSide(Point3F(x, y, z)) == PlaneF::Back)
                    continue;

                anyfront = true;
                break;
            }

            if (!anyfront)
            {
                light->mScore = 0;
                return;
            }
        }

        if ((light->mType == LightInfo::Vector) || (light->mType == LightInfo::Ambient))
        {
            colorintensity =
                (light->mColor.red + light->mAmbient.red) * 0.346f +
                (light->mColor.green + light->mAmbient.green) * 0.588f +
                (light->mColor.blue + light->mAmbient.blue) * 0.070f;
            distintensity = 1;
            weight = SG_LIGHTMANAGER_SUN_PRIORITY;
        }
        else
        {
            if (light->sgAssignedToParticleSystem)
            {
                colorintensity = SG_PARTICLESYSTEMLIGHT_FIXED_INTENSITY;
            }
            else
            {
                colorintensity =
                    (light->mColor.red * 0.3333f) +
                    (light->mColor.green * 0.3333f) +
                    (light->mColor.blue * 0.3333f);
            }

            sgLightingModel& model = sgLightingModelManager::sgGetLightingModel(light->sgLightingModelName);
            model.sgSetState(light);
            distintensity = model.sgScoreLight(light, sphere);
            model.sgResetState();

            if (light->sgAssignedToTSObject)
                weight = SG_LIGHTMANAGER_ASSIGNED_PRIORITY;
            else if ((light->mType == LightInfo::SGStaticPoint) || (light->mType == LightInfo::SGStaticSpot))
                weight = SG_LIGHTMANAGER_STATIC_PRIORITY;
        }
    }

    F32 intensity = colorintensity * distintensity;
    if (intensity < SG_MIN_LEXEL_INTENSITY)
        intensity = 0;
    light->mScore = S32(intensity * weight * 1024.0f);
}

void LightManager::sgInit()
{
    for (U32 i = 0; i < sgPropertyCount; i++)
        sgLightingProperties[i] = false;

    //Con::addVariable("$pref::OpenGL::sgDynamicDTSVectorLighting", TypeBool, &sgDynamicDTSVectorLighting);
    //Con::addVariable("$pref::TS::sgShadowDetailSize", TypeS32, &sgShadowDetailSize);
    //Con::addVariable("$pref::OpenGL::sgDynamicParticleSystemLighting", TypeBool, &sgDynamicParticleSystemLighting);
    Con::addVariable("$pref::LightManager::sgMaxBestLights", TypeS32, &sgMaxBestLights);
    Con::addVariable("$pref::LightManager::sgLightingProfileQuality", TypeS32, &sgLightingProfileQuality);
    Con::addVariable("$pref::LightManager::sgLightingProfileAllowShadows", TypeBool, &sgLightingProfileAllowShadows);

    Con::addVariable("$pref::LightManager::sgUseDynamicShadows", TypeBool, &sgUseDynamicShadows);
    //Con::addVariable("$pref::LightManager::sgUseDynamicShadowSelfShadowing", TypeBool, &sgUseDynamicShadowSelfShadowing);
    //Con::addVariable("$pref::LightManager::sgUseDynamicShadowSelfShadowingOnPS_2_0", TypeBool, &sgUseDynamicShadowSelfShadowingOnPS_2_0);
    //Con::addVariable("$pref::LightManager::sgUseBloom", TypeBool, &sgUseBloom);
    //Con::addVariable("$pref::LightManager::sgUseToneMapping", TypeBool, &sgUseToneMapping);
    Con::addVariable("$pref::LightManager::sgUseDynamicRangeLighting", TypeBool, &sgUseDynamicRangeLighting);
    Con::addVariable("$pref::LightManager::sgUseDRLHighDynamicRange", TypeBool, &sgUseDRLHighDynamicRange);
    Con::addVariable("$pref::LightManager::sgUseDynamicLightingDualOptimization", TypeBool, &sgUseDynamicLightingDualOptimization);

    Con::addVariable("$pref::LightManager::sgDynamicShadowDetailSize", TypeS32, &sgDynamicShadowDetailSize);
    Con::addVariable("$pref::LightManager::sgDynamicShadowQuality", TypeS32, &sgDynamicShadowQuality);
    Con::addVariable("$pref::LightManager::sgMultipleDynamicShadows", TypeBool, &sgMultipleDynamicShadows);
    Con::addVariable("$pref::LightManager::sgShowCacheStats", TypeBool, &sgShowCacheStats);

    //Con::addVariable("$pref::LightManager::sgDRLTarget", TypeF32, &sgDRLTarget);
    //Con::addVariable("$pref::LightManager::sgDRLMax", TypeF32, &sgDRLMax);
    //Con::addVariable("$pref::LightManager::sgDRLMin", TypeF32, &sgDRLMin);
    //Con::addVariable("$pref::LightManager::sgDRLMultiplier", TypeF32, &sgDRLMultiplier);

    //Con::addVariable("$pref::LightManager::sgBloomCutOff", TypeF32, &sgBloomCutOff);
    //Con::addVariable("$pref::LightManager::sgBloomAmount", TypeF32, &sgBloomAmount);
    //Con::addVariable("$pref::LightManager::sgBloomSeedAmount", TypeF32, &sgBloomSeedAmount);

    Con::addVariable("$pref::LightManager::sgAtlasMaxDynamicLights", TypeF32, &sgAtlasMaxDynamicLights);

    Con::addVariable("$LightManager::sgInGUIEditor", TypeBool, &sgInGUIEditor);

    sgRelightFilter::sgInit();
}
/*
bool LightManager::sgAllowDetailMaps()
{
return true;
}
*/
void LightManager::sgGetFilteredLightColor(ColorF& color, ColorF& ambient, S32 objectzone)
{
    sgMissionLightingFilter* filterbasezone = NULL;
    sgMissionLightingFilter* filtercurrentzone = NULL;
    SimSet* filters = Sim::getsgMissionLightingFilterSet();

    for (SimObject** itr = filters->begin(); itr != filters->end(); itr++)
    {
        sgMissionLightingFilter* filter = dynamic_cast<sgMissionLightingFilter*>(*itr);
        if (!filter)
            continue;

        S32 zone = filter->getCurrZone(0);
        if (zone == 0)
            filterbasezone = filter;
        if (zone == objectzone)
        {
            filtercurrentzone = filter;
            break;
        }
    }

    if (filtercurrentzone)
        filterbasezone = filtercurrentzone;

    if (!filterbasezone)
        return;

    sgMissionLightingFilterData* datablock = (sgMissionLightingFilterData*)filterbasezone->getDataBlock();

    if (!datablock)
        return;

    ColorF composite = datablock->sgLightingFilter * datablock->sgLightingIntensity;

    color *= composite;
    color.clamp();

    ambient *= composite;
    ambient.clamp();

    if (!datablock->sgCinematicFilter)
        return;

    // must use the lighting filter intensity
    // to lock the reference value relative
    // to the lighting intensity
    composite = datablock->sgCinematicFilterReferenceColor *
        datablock->sgCinematicFilterReferenceIntensity *
        datablock->sgLightingIntensity;

    F32 intensity = color.red + color.green + color.blue + ambient.red + ambient.green + ambient.blue;
    F32 intensityref = composite.red + composite.green + composite.blue;

    intensity -= intensityref;

    // blue is too intense...
    if (intensity > 0.0f)
        intensity *= 0.25f;

    F32 redoffset = 1.0f - ((intensity) * 0.1f * datablock->sgCinematicFilterAmount);
    F32 blueoffset = 1.0f + ((intensity) * 0.1f * datablock->sgCinematicFilterAmount);
    F32 greenoffset = 1.0f - ((1.0f - getMin(redoffset, blueoffset)) * 0.5f);

    ColorF multiplier = ColorF(redoffset, greenoffset, blueoffset);

    color *= multiplier;
    color.clamp();
    ambient *= multiplier;
    ambient.clamp();
}
/*
// adds proper support for self-illumination...
void sgLightManager::sgSetAmbientSelfIllumination(LightInfo *lightinfo, F32 *lightColor,
F32 *ambientColor)
{
if(sgAllowCollectSelfIlluminationColor())
{
if(lightinfo->mType == LightInfo::Vector)
sgSelfIlluminationColor = ColorF(lightColor[0], lightColor[1], lightColor[2]);
else
sgSelfIlluminationColor = ColorF(ambientColor[0], ambientColor[1], ambientColor[2]);
}
}
*/

/// Sets shader constants / textures for light infos
void LightManager::setLightInfo(ProcessedMaterial* pmat, const Material* mat, const SceneGraphData& sgData, U32 pass)
{
    U32 stageNum = pmat->getStageFromPass(pass);

    // Light number 1
    MatrixF objTrans = sgData.objTrans;
    objTrans.inverse();

    const LightInfo light = sgData.light;
    Point3F lightPos = light.mPos;
    Point3F lightDir = light.mDirection;
    ColorF lightAmbient = light.mAmbient;

    objTrans.mulV(lightPos);
    objTrans.mulV(lightDir);
    
    // TODO: FIGURE OUT MATH
    if (dStrstr(mat->getName(), "ringglass")) {
        lightPos.set(-0.068099, 0.012973, -0.086792);
        lightAmbient *= ColorF(1 / 1.18f, 1 / 1.06f, 1 / 0.95f);
    }

    Point4F lightPosModel(lightPos.x, lightPos.y, lightPos.z, light.sgTempModelInfo[0]);
    GFX->setVertexShaderConstF(VC_LIGHT_POS1, (float*)&lightPosModel, 1);
    GFX->setVertexShaderConstF(VC_LIGHT_DIR1, (float*)&lightDir, 1);
    GFX->setVertexShaderConstF(VC_LIGHT_DIFFUSE1, (float*)&(light.mColor), 1);
    GFX->setPixelShaderConstF(PC_AMBIENT_COLOR, (float*)&(lightAmbient), 1);

    if(!mat->emissive[stageNum])
        GFX->setPixelShaderConstF(PC_DIFF_COLOR, (float*)&(light.mColor), 1);
    else
    {
        ColorF selfillum = LightManager::sgGetSelfIlluminationColor(mat->diffuse[stageNum]);
        GFX->setPixelShaderConstF(PC_DIFF_COLOR, (float*)&selfillum, 1);
    }
    //const MatrixF lightingmat = light.sgLightingTransform;
    //GFX->setVertexShaderConstF(VC_LIGHT_TRANS, (float*)&lightingmat, 4);

    // Light number 2
    //const LightInfo light2 = sgData.lightSecondary;
    ////AssertFatal(light2 != NULL, "ProcessedSGLightedMaterial::setSecondaryLightInfo: null light");
    //lightPos = light2.mPos;
    //objTrans.mulP(lightPos);

    //Point4F lightPosModel2(lightPos.x, lightPos.y, lightPos.z, light2.sgTempModelInfo[0]);
    //GFX->setVertexShaderConstF(VC_LIGHT_POS2, (float*)&lightPosModel2, 1);
    //GFX->setPixelShaderConstF(PC_DIFF_COLOR2, (float*)&light2.mColor, 1);

    //const MatrixF lightingmat2 = light2.sgLightingTransform;
    //GFX->setVertexShaderConstF(VC_LIGHT_TRANS2, (float*)&lightingmat2, 4);

    //ProcessedCustomMaterial* pcm = dynamic_cast<ProcessedCustomMaterial*>(pmat);
    //if (!pcm)
    //{
    //    // Set the dynamic light textures
    //    const RenderPassData* rpass = pmat->getPass(pass);
    //    if (rpass)
    //    {
    //        for (U32 i = 0; i < rpass->numTex; i++)
    //        {
    //            setTextureStage(sgData, rpass->texFlags[i], i);
    //        }
    //    }
    //}
    //} else {
    //    // Processed custom materials store their texflags in a different way, so
    //    // just tell it to update its textures.
    //    SceneGraphData& temp = const_cast<SceneGraphData&>(sgData);
    //    pcm->setTextureStages(temp, pass);
    //}
}

void LightManager::setLightingBlendFunc()
{
    // don't break the material multipass rendering...
    GFX->setAlphaBlendEnable(true);
    GFX->setSrcBlend(GFXBlendSrcAlpha);
    GFX->setDestBlend(GFXBlendOne);
}

bool LightManager::setTextureStage(const SceneGraphData& sgData, const U32 currTexFlag, const U32 textureSlot)
{
    switch (currTexFlag)
    {
        case Material::DynamicLight:
            //GFX->setTextureBorderColor(textureSlot, ColorI(0, 0, 0, 0));
            GFX->setTextureStageAddressModeU(textureSlot, GFXAddressClamp);
            GFX->setTextureStageAddressModeV(textureSlot, GFXAddressClamp);
            GFX->setTextureStageAddressModeW(textureSlot, GFXAddressClamp);
            GFX->setTexture(textureSlot, sgData.dynamicLight.getPointer());
            return true;
            break;

        case Material::DynamicLightSecondary:
            //GFX->setTextureBorderColor(textureSlot, ColorI(0, 0, 0, 0));
            GFX->setTextureStageAddressModeU(textureSlot, GFXAddressClamp);
            GFX->setTextureStageAddressModeV(textureSlot, GFXAddressClamp);
            GFX->setTextureStageAddressModeW(textureSlot, GFXAddressClamp);
            GFX->setTexture(textureSlot, sgData.dynamicLightSecondary.getPointer());
            return true;
            break;

//        case Material::DynamicLightMask:
//            SG_CHECK_LIGHT(sgData.light);
//            GFX->setCubeTexture(textureSlot, static_cast<sgLightInfo*>(sgData.light)->sgLightMask);
//            return true;
//            break;
    }
    return false;
}

void LightManager::sgSetupZoneLighting(bool enable, SimObject* sobj)
{
    sgFilterZones = false;
    // these must be -2...
    sgZones[0] = -2;
    sgZones[1] = -2;
    if (!enable)
        return;
    if (dynamic_cast<InteriorInstance*>(sobj))
        return;
    //if(dynamic_cast<TerrainBlock *>(sobj))
    //	return;
    SceneObject* obj = dynamic_cast<SceneObject*>(sobj);
    if (!obj)
        return;
    sgFilterZones = true;
    U32 count = getMin(obj->getNumCurrZones(), U32(2));
    for (U32 i = 0; i < count; i++)
    {
        sgZones[i] = obj->getCurrZone(i);
    }
}
/*
void LightManager::sgScoreLight(LightInfo *lightinfo, const SphereF &sphere)
{
    if(LightManager::sgFilterZones && lightinfo->sgDiffuseRestrictZone)
    {
        bool allowdiffuse = false;
        if(LightManager::sgZones[0] > -1)
        {
            if(lightinfo->sgAllowDiffuseZoneLighting(LightManager::sgZones[0]))
                allowdiffuse = true;
            else if(LightManager::sgZones[1] > -1)
            {
                if(lightinfo->sgAllowDiffuseZoneLighting(LightManager::sgZones[1]))
                    allowdiffuse = true;
            }
        }

        if(!allowdiffuse)
        {
            lightinfo->mScore = 0;
            return;
        }
    }

    F32 distintensity = 1;
    F32 colorintensity = 1;
    F32 weight = SG_LIGHTMANAGER_DYNAMIC_PRIORITY;

    if((lightinfo->mType == LightInfo::Vector) || (lightinfo->mType == LightInfo::Ambient))
    {
        colorintensity = (lightinfo->mColor.red   + lightinfo->mAmbient.red) * 0.346f +
            (lightinfo->mColor.green + lightinfo->mAmbient.green) * 0.588f +
            (lightinfo->mColor.blue  + lightinfo->mAmbient.blue) * 0.070f;
        distintensity = 1;
        weight = SG_LIGHTMANAGER_SUN_PRIORITY;
    }
    else
    {
        if(lightinfo->sgAssignedToParticleSystem)
        {
            colorintensity = SG_PARTICLESYSTEMLIGHT_FIXED_INTENSITY;
        }
        else
        {
            colorintensity = (lightinfo->mColor.red * 0.3333f) +
                (lightinfo->mColor.green * 0.3333f) +
                (lightinfo->mColor.blue * 0.3333f);
        }

        sgLightingModel &model = sgLightingModelManager::sgGetLightingModel(sglightinfo.sgLightingModelName);
        model.sgSetState(lightInfo);
        distintensity = model.sgScoreLight(lightinfo, sphere);
        model.sgResetState();

        if(lightinfo->sgAssignedToTSObject)
            weight = SG_LIGHTMANAGER_ASSIGNED_PRIORITY;
        else if((lightinfo->mType == LightInfo::SGStaticPoint) || (lightinfo->mType == LightInfo::SGStaticSpot))
            weight = SG_LIGHTMANAGER_STATIC_PRIORITY;
    }

    F32 intensity = colorintensity * distintensity;
    if(intensity < 0.004f)
        intensity = 0;
    lightinfo->mScore = S32(intensity * weight * 1024.0f);
}
*/
void sgStatistics::sgClear()
{
    sgInteriorLexelCount = 0;
    sgInteriorLexelTime = 0;
    sgInteriorObjectCount = 0;
    sgInteriorObjectIncludedCount = 0;
    sgInteriorObjectIlluminationCount = 0;
    sgInteriorSurfaceIncludedCount = 0;
    sgInteriorSurfaceIlluminationCount = 0;
    sgInteriorSurfaceIlluminatedCount = 0;
    sgInteriorOccluderCount = 0;
    sgTerrainLexelCount = 0;
    sgTerrainLexelTime = 0;
    //sgTerrainOccluderCount = 0;
}

void sgStatistics::sgPrint()
{
    Con::printf("");
    Con::printf("  Lighting Pack lighting system stats:");
    Con::printf("    Interior Lexel Count:                %d", sgInteriorLexelCount);
    Con::printf("    Interior Lexel Time:                 %f", sgInteriorLexelTime);
    Con::printf("    Interior Object Count:               %d", sgInteriorObjectCount);
    Con::printf("    Interior Object Included Count:      %d", sgInteriorObjectIncludedCount);
    Con::printf("    Interior Object Illumination Count:  %d", sgInteriorObjectIlluminationCount);
    Con::printf("    Interior Surface Included Count:     %d", sgInteriorSurfaceIncludedCount);
    Con::printf("    Interior Surface Illumination Count: %d", sgInteriorSurfaceIlluminationCount);
    Con::printf("    Interior Surface Illuminated Count:  %d", sgInteriorSurfaceIlluminatedCount);
    Con::printf("    Interior Occluder Count:             %d", sgInteriorOccluderCount);
    Con::printf("    Terrain Lexel Count:                 %d", sgTerrainLexelCount);
    Con::printf("    Terrain Lexel Time:                  %f", sgTerrainLexelTime);
}

S32 QSORT_CALLBACK LightManager::sgSortLightsByAddress(const void* a, const void* b)
{
    return ((*(LightInfo**)b) - (*(LightInfo**)a));
}

S32 QSORT_CALLBACK LightManager::sgSortLightsByScore(const void* a, const void* b)
{
    return((*(LightInfo**)b)->mScore - (*(LightInfo**)a)->mScore);
}

void sgRelightFilter::sgInit()
{
    Con::addVariable("SceneLighting::sgFilterRelight", TypeBool, &sgRelightFilter::sgFilterRelight);
    Con::addVariable("SceneLighting::sgFilterRelightVisible", TypeBool, &sgRelightFilter::sgFilterRelightVisible);
    Con::addVariable("SceneLighting::sgFilterRelightByDistance", TypeBool, &sgRelightFilter::sgFilterRelightByDistance);
    //Con::addVariable("SceneLighting::sgFilterRelightByVisiblity", TypeBool, &sgRelightFilter::sgFilterRelightByVisiblity);
    Con::addVariable("SceneLighting::sgFilterRelightByDistanceRadius", TypeF32, &sgRelightFilter::sgFilterRelightByDistanceRadius);
    Con::addVariable("SceneLighting::sgFilterRelightByDistancePosition", TypePoint3F, &sgRelightFilter::sgFilterRelightByDistancePosition);
}

bool sgRelightFilter::sgAllowLighting(const Box3F& box, bool forcefilter)
{
    if ((sgRelightFilter::sgFilterRelight && sgRelightFilter::sgFilterRelightByDistance) || forcefilter)
    {
        if (!sgRelightFilter::sgFilterRelightVisible)
            return false;

        Point3F min, max;

        min = EditTSCtrl::smCamPos;
        min.x = min.x - sgRelightFilter::sgFilterRelightByDistanceRadius;
        min.y = min.y - sgRelightFilter::sgFilterRelightByDistanceRadius;
        min.z = min.z - sgRelightFilter::sgFilterRelightByDistanceRadius;

        max = EditTSCtrl::smCamPos;
        max.x = max.x + sgRelightFilter::sgFilterRelightByDistanceRadius;
        max.y = max.y + sgRelightFilter::sgFilterRelightByDistanceRadius;
        max.z = max.z + sgRelightFilter::sgFilterRelightByDistanceRadius;

        Box3F lightbox = Box3F(min, max);

        if (!box.isOverlapped(lightbox))
            return false;
    }

    return true;
}

void sgRelightFilter::sgRenderAllowedObjects(void* editor)
{
    U32 i;
    WorldEditor* worldeditor = (WorldEditor*)editor;
    Vector<SceneObject*> objects;
    gServerContainer.findObjects(InteriorObjectType, sgFindObjectsCallback, &objects);
    for (i = 0; i < objects.size(); i++)
    {
        SceneObject* obj = objects[i];
        if (worldeditor->objClassIgnored(obj))
            continue;

        if (!sgRelightFilter::sgAllowLighting(obj->getWorldBox(), true))
            continue;

        ColorI color = ColorI(255, 0, 255);
        worldeditor->renderObjectBox(obj, color);
    }
}

