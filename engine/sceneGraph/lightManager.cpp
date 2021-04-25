//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "sceneGraph/lightManager.h"
#include "console/console.h"
#include "console/simBase.h"
#include "sceneGraph/sceneGraph.h"
#include "console/consoleTypes.h"

namespace {
    static F32 zeroColor[] = { 0.f, 0.f, 0.f, 0.f };
    static F32 whiteColor[] = { 1.f, 1.f, 1.f, 1.f };

    // global settings for the lightmanager
    static ColorF gOpenGLLightingAmbientColor(0.f, 0.f, 0.f, 0.f);
    static ColorF gOpenGLMaterialAmbientColor(0.f, 0.f, 0.f, 0.f);
    static ColorF gOpenGLMaterialDiffuseColor(1.f, 1.f, 1.f, 1.f);
    static S32 gOpenGLMaxHardwareLights = LightManager::DefaultMaxLights;

    bool colorIsZero(const ColorF& col)
    {
        return((col.red == 0.f) && (col.green == 0.f) && (col.blue == 0.f));
    }
}

//------------------------------------------------------------------------------
// Class LightManager
//------------------------------------------------------------------------------
ConsoleFunction(resetLighting, void, 1, 1, "Resets GL lighting.")
{
    if (!gClientSceneGraph)
    {
        Con::errorf(ConsoleLogEntry::General, "LightManager::cResetLighting: no client scenegraph!");
        return;
    }
    LightManager* lighting = gClientSceneGraph->getLightManager();
    if (!lighting)
    {
        Con::errorf(ConsoleLogEntry::General, "LightManager::cResetLighting: lightmanager not found!");
        return;
    }
    lighting->resetGL();
}

LightManager::LightManager()
{
    mGLInitialized = false;
    mGLLightsInstalled = false;
    mGLLightCount = 0;
    mAmbientLightColor.set(0.f, 0.f, 0.f);
    mVectorLightsAttenuation = 1.f;
    mVectorLightsEnabled = true;

#ifdef DEBUG
    Con::addVariable("$pref::OpenGL::lightingAmbientColor", TypeColorF, &gOpenGLLightingAmbientColor);
    Con::addVariable("$pref::OpenGL::materialAmbientColor", TypeColorF, &gOpenGLMaterialAmbientColor);
    Con::addVariable("$pref::OpenGL::materialDiffuseColor", TypeColorF, &gOpenGLMaterialDiffuseColor);
#endif
    Con::addVariable("$pref::OpenGL::maxHardwareLights", TypeS32, &gOpenGLMaxHardwareLights);
}

void LightManager::setMaxGLLights(U32 max)
{
    /*
       // ask gl for max light value, and check if this is greater..
       GLint glMaxLights = 0;
       glGetIntegerv(GL_MAX_LIGHTS, &glMaxLights);

       // clamp to the number gl returned
       if(max > glMaxLights)
       {
          Con::errorf(ConsoleLogEntry::General, "LightManager::setMaxGLLights: too many lights.");
          max = glMaxLights;
       }

       mMaxGLLights = max;

       // cleanup any installed lights
       if(mGLLightsInstalled)
          uninstallGLLights();
    */
}

// will crash if called prior to PlatformVideo:: initialization!
void LightManager::resetGL()
{
    /*
       if(!mGLInitialized)
          return;

       if(mGLLightsInstalled)
          uninstallGLLights();

       // set ambient color
       glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, (const F32 *)gOpenGLMaterialAmbientColor);
       glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, (const F32 *)gOpenGLMaterialDiffuseColor);

       setMaxGLLights(gOpenGLMaxHardwareLights);
    */
}

void LightManager::registerLights(bool lightingScene)
{
    if (!mGLInitialized)
    {
        mGLInitialized = true;
        resetGL();
    }

    if (mGLLightsInstalled)
    {
        Con::warnf(ConsoleLogEntry::General, "LightManager::registerLights: lights are already installed.");
        uninstallGLLights();
    }

    // clear out the light list
    mLights.clear();

    // walk the lightlist and query objects for their lights
    SimSet* lightSet = Sim::getLightSet();
    for (SimObject** itr = lightSet->begin(); itr != lightSet->end(); itr++)
        (*itr)->registerLights(this, lightingScene);
}

void LightManager::addLight(LightInfo* light)
{
    mLights.push_back(light);
}

void LightManager::removeLight(LightInfo* light)
{
    for (U32 i = 0; i < mLights.size(); i++)
    {
        if (mLights[i] == light)
        {
            mLights.erase_fast(i);
            break;
        }
    }
}

//--------------------------------------------------------------------------
U32 LightManager::installGLLights(const SphereF& sphere)
{
    // score the lights:
    for (U32 i = 0; i < mLights.size(); i++)
    {
        if (!mVectorLightsEnabled && (mLights[i]->mType == LightInfo::Vector))
            mLights[i]->mScore = 0;
        else
            scoreLight(mLights[i], sphere);
    }

    return(installGLLights());
}

U32 LightManager::installGLLights(const Box3F& box)
{
    SphereF sphere;
    box.getCenter(&sphere.center);
    sphere.radius = Point3F(box.max - sphere.center).len();

    return(installGLLights(sphere));
}

//--------------------------------------------------------------------------
U32 LightManager::installGLLights()
{
    AssertFatal(!mGLLightsInstalled, "LightManager::installGLLights: lights are already installed!");
    mGLLightsInstalled = true;

    // sort them
    dQsort(mLights.address(), mLights.size(), sizeof(LightInfo*), compareLights);

    // grab the best ones...
    U32 count = 0;
    while (count < mLights.size())
    {
        if (count >= mMaxGLLights)
            break;

        if (!mLights[count]->mScore)
            break;

        installGLLight(mLights[count++]);
    }

    // enable ambient and lighting always:
 //   glLightModelfv(GL_LIGHT_MODEL_AMBIENT, (const F32 *)ColorF(gOpenGLLightingAmbientColor + mAmbientLightColor));
 //   glEnable(GL_LIGHTING);

    return(count);
}

//--------------------------------------------------------------------------
// - restores ambient light changes
// - restores vector light enabled flag
void LightManager::uninstallGLLights()
{
    if (!mGLLightsInstalled)
    {
        Con::errorf(ConsoleLogEntry::General, "LightManager::uninstallGLLights: no lights installed!");
        return;
    }
    mGLLightsInstalled = false;

    // restore values:
 //   glLightModelfv(GL_LIGHT_MODEL_AMBIENT, (const F32*)gOpenGLLightingAmbientColor);

    mAmbientLightColor.set(0.f, 0.f, 0.f);
    mVectorLightsAttenuation = 1.f;
    mVectorLightsEnabled = true;

    //   glDisable(GL_LIGHTING);
    //   for(U32 i = 0; i < mGLLightCount; i++)
    //      glDisable(GL_LIGHT0 + i);
    mGLLightCount = 0;
}

void LightManager::installGLLight(LightInfo* lightInfo)
{
    AssertFatal(mGLLightCount < mMaxGLLights, "LightManager::installGLLight: too many lights installed");

    /*
       GLfloat lightColor[]   = {lightInfo->mColor.red, lightInfo->mColor.green, lightInfo->mColor.blue, 1.f};
       GLfloat ambientColor[] = {lightInfo->mAmbient.red, lightInfo->mAmbient.green, lightInfo->mAmbient.blue, 1.f};
       GLfloat lightPos[]     = {lightInfo->mPos.x, lightInfo->mPos.y, lightInfo->mPos.z, 1.f};
       GLfloat lightDir[]     = {-lightInfo->mDirection.x, -lightInfo->mDirection.y, -lightInfo->mDirection.z, 0.f};

       U32 light = GL_LIGHT0 + mGLLightCount++;

       glEnable(light);

       switch(lightInfo->mType)
       {
          case LightInfo::Spot:

             // set the light params
             glLightfv(light, GL_POSITION,       (const GLfloat*)lightPos);
             glLightfv(light, GL_SPOT_DIRECTION, (const GLfloat*)lightDir);
             glLightfv(light, GL_DIFFUSE,        (const GLfloat*)lightColor);
             glLightfv(light, GL_AMBIENT,        (const GLfloat*)zeroColor);
             glLightfv(light, GL_SPECULAR,       (const GLfloat*)zeroColor);

             break;

          // falls through to LightInfo::Ambient
          case LightInfo::Vector:
          {
             F32 ambientFactor = (ambientColor[0] + ambientColor[1] + ambientColor[2]) / 3.f;
             F32 factor = mClampF(mVectorLightsAttenuation - ambientFactor, 0.f, 1.f);

             // attenuate (for in shadow...)
             for(U32 i = 0; i < 3; i++)
                lightColor[i] *= factor;
          }

          case LightInfo::Ambient:

             // set the light params
             glLightfv(light, GL_POSITION, (const GLfloat*)lightDir);
             glLightfv(light, GL_DIFFUSE,  (const GLfloat*)lightColor);
             glLightfv(light, GL_AMBIENT,  (const GLfloat*)ambientColor);
             glLightfv(light, GL_SPECULAR, (const GLfloat*)zeroColor);

             break;

          case LightInfo::Point:

             // set the light params
             glLightfv(light, GL_POSITION, (const GLfloat*)lightPos);
             glLightfv(light, GL_DIFFUSE,  (const GLfloat*)lightColor);
             glLightfv(light, GL_AMBIENT,  (const GLfloat*)zeroColor);
             glLightfv(light, GL_SPECULAR, (const GLfloat*)zeroColor);

             glLightf(light, GL_SPOT_CUTOFF, 180.f);

             glLightf(light, GL_CONSTANT_ATTENUATION, 0.f);
             glLightf(light, GL_LINEAR_ATTENUATION, 1.0f);
             glLightf(light, GL_QUADRATIC_ATTENUATION, 0.f);

             break;
       }
    */
}

//------------------------------------------------------------------------------
void LightManager::getBestLights(const PlaneF* testPlanes, const U32 numPlanes,
    const Point3F& pos, LightInfoList& lightList, const U32 max)
{
    for (U32 i = 0; i < mLights.size(); i++)
        scoreLight(mLights[i], testPlanes, numPlanes, pos);

    fillLightList(lightList, max);
}

void LightManager::getBestLights(const SphereF& sphere, LightInfoList& lightList, const U32 max)
{
    // score the lights
    for (U32 i = 0; i < mLights.size(); i++)
        scoreLight(mLights[i], sphere);

    fillLightList(lightList, max);
}

void LightManager::getBestLights(const Box3F& box, LightInfoList& lightList, const U32 max)
{
    SphereF sphere;
    box.getCenter(&sphere.center);
    sphere.radius = Point3F(box.max - sphere.center).len();

    getBestLights(sphere, lightList, max);
}

//------------------------------------------------------------------------------
void LightManager::fillLightList(LightInfoList& lightList, const U32 max)
{
    dQsort(mLights.address(), mLights.size(), sizeof(LightInfo*), compareLights);

    // grab the best ones...
    U32 count = 0;
    for (U32 i = 0; i < mLights.size(); i++)
    {
        if (!mLights[i]->mScore)
            continue;

        if (count++ >= max)
            break;

        lightList.push_back(mLights[i]);
    }
}

//------------------------------------------------------------------------------
// 0              not lit
// 1->100         point/spot lights
// 100->200       vector/ambient lights
void LightManager::scoreLight(LightInfo* lightInfo, const SphereF& sphere) const
{
    switch (lightInfo->mType)
    {
        case LightInfo::Ambient:
        case LightInfo::Vector:
        {
            // 100 + (color_intensity * 100)
            float intensity = (lightInfo->mColor.red + lightInfo->mAmbient.red) * 0.346f +
                (lightInfo->mColor.green + lightInfo->mAmbient.green) * 0.588f +
                (lightInfo->mColor.blue + lightInfo->mAmbient.blue) * 0.070f;

            lightInfo->mScore = 100 + S32(100.f * mClampF(intensity, 0.f, 1.f));
            break;
        }

        case LightInfo::Point:
        {
            Point3F diff = lightInfo->mPos - sphere.center;
            F32 lenSq = diff.lenSquared();
            F32 sumRadiusSq = (sphere.radius + lightInfo->mRadius) * (sphere.radius + lightInfo->mRadius);

            if (lenSq > sumRadiusSq)
            {
                lightInfo->mScore = 0;
                break;
            }

            F32 radSq = sphere.radius * sphere.radius;
            lightInfo->mScore = 1 + S32(99.f * (getMin((sumRadiusSq - lenSq), radSq) / radSq));
            break;
        }

        default:
            lightInfo->mScore = 1;
            break;
    }
}

void LightManager::scoreLight(LightInfo* lightInfo,
    const PlaneF*  /*testPlanes*/,
    const U32      /*numPlanes*/,
    const Point3F& /*pos*/) const
{
    switch (lightInfo->mType)
    {
        case LightInfo::Ambient:
        case LightInfo::Vector:
        {
            // 100 + (color_intensity * 100)
            float intensity = (lightInfo->mColor.red + lightInfo->mAmbient.red) * 0.346f +
                (lightInfo->mColor.green + lightInfo->mAmbient.green) * 0.588f +
                (lightInfo->mColor.blue + lightInfo->mAmbient.blue) * 0.070f;

            lightInfo->mScore = 100 + S32(100.f * mClampF(intensity, 0.f, 1.f));
            break;
        }

        case LightInfo::Point:
        {
            lightInfo->mScore = 1;
            break;
        }

        default:
            lightInfo->mScore = 1;
            break;
    }
}

//------------------------------------------------------------------------------
S32 QSORT_CALLBACK LightManager::compareLights(const void* a, const void* b)
{
    return((*(LightInfo**)b)->mScore - (*(LightInfo**)a)->mScore);
}
