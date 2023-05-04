//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "gfx/gBitmap.h"
#include "math/mathIO.h"
#include "core/bitStream.h"
#include "console/consoleTypes.h"
#include "sceneGraph/sceneGraph.h"
#include "terrain/terrData.h"
#include "terrain/environment//sun.h"

IMPLEMENT_CO_NETOBJECT_V1(Sun);

//-----------------------------------------------------------------------------

Sun::Sun()
{
    mNetFlags.set(Ghostable | ScopeAlways);
    mTypeMask = EnvironmentObjectType;

    mLight.mType = LightInfo::Vector;
    mLight.mDirection.set(0.f, 0.707f, -0.707f);
    mLight.mColor.set(0.7f, 0.7f, 0.7f);
    mLight.mAmbient.set(0.3f, 0.3f, 0.3f);
    mLight.mShadowColor.set(0.f, 0.f, 0.f, 0.4f);

    mLight.sgCastsShadows = true;
    mLight.sgDoubleSidedAmbient = true;
    mLight.sgUseNormals = true;
    mLight.sgZone[0] = 0;

    useBloom = false;
    useToneMapping = false;
    useDynamicRangeLighting = false;

    DRLHighDynamicRange = false;
    DRLTarget = 0.5;
    DRLMax = 1.4;
    DRLMin = 0.5;
    DRLMultiplier = 1.1;

    bloomCutOff = 0.8;
    bloomAmount = 0.25;
    bloomSeedAmount = 1.0;
}

//-----------------------------------------------------------------------------

void Sun::conformLight()
{
    mLight.mDirection.normalize();
    //mLight.mColor.clamp();
    mLight.mAmbient.clamp();
    mLight.mShadowColor.clamp();
}

//-----------------------------------------------------------------------------

bool Sun::onAdd()
{
    if (!Parent::onAdd())
        return(false);

    if (isClientObject() || gSPMode)
        Sim::getLightSet()->addObject(this);

    if (!isClientObject())
        conformLight();

    return(true);
}

void Sun::onRemove()
{
    Parent::onRemove();

    // remove my light
    //if (isClientObject())
     //   gClientSceneGraph->getLightManager()->sgUnregisterGlobalLight(&mLight);
}

struct SunLight
{
    Point3F     mPos;
    VectorF     mDirection;
    ColorF      mColor;
    ColorF      mAmbient;
    ColorF      mShadowColor;
};

void Sun::registerLights(LightManager* lightManager, bool relight)
{
    mRegisteredLight = mLight;
    LightManager::sgGetFilteredLightColor(mRegisteredLight.mColor, mRegisteredLight.mAmbient, 0);

#ifdef MB_ULTRA
    // Color correction to match MBU as close as possible (this took several hours of trial and error, your welcome.)
    mRegisteredLight.mAmbient *= ColorF(1.18f, 1.06f, 0.95f);
#endif

#ifdef MB_ULTRA_PREVIEWS
    // This code is not here on MBU x360 but it achieves the same result.
    // It likely works on x360 just because it uses the old TSE 0.1 render system.
    static SunLight previewSun = {mRegisteredLight.mPos, mRegisteredLight.mDirection, mRegisteredLight.mColor, mRegisteredLight.mAmbient, mRegisteredLight.mShadowColor};

    if (gSPMode)
        previewSun = {mRegisteredLight.mPos, mRegisteredLight.mDirection, mRegisteredLight.mColor, mRegisteredLight.mAmbient, mRegisteredLight.mShadowColor};
    else
    {
        mRegisteredLight.mPos = previewSun.mPos;
        mRegisteredLight.mDirection = previewSun.mDirection;
        mRegisteredLight.mColor = previewSun.mColor;
        mRegisteredLight.mAmbient = previewSun.mAmbient;
        mRegisteredLight.mShadowColor = previewSun.mShadowColor;
    }
#endif

    if (relight)
    {
        // static lighting not affected by this option when using the sun...
        mRegisteredLight.sgCastsShadows = true;
        lightManager->sgRegisterGlobalLight(&mRegisteredLight, this, true);
    }
    else
        lightManager->sgSetSpecialLight(LightManager::sgSunLightType, &mRegisteredLight);

    LightManager::sgSetAllowDynamicRangeLighting(useDynamicRangeLighting);
    LightManager::sgSetAllowHighDynamicRangeLighting(DRLHighDynamicRange);
    LightManager::sgSetAllowDRLBloom(useBloom);
    LightManager::sgSetAllowDRLToneMapping(useToneMapping);

    LightManager::sgDRLTarget = DRLTarget;
    LightManager::sgDRLMax = DRLMax;
    LightManager::sgDRLMin = DRLMin;
    LightManager::sgDRLMultiplier = DRLMultiplier;

    LightManager::sgBloomCutOff = bloomCutOff;
    LightManager::sgBloomAmount = bloomAmount;
    LightManager::sgBloomSeedAmount = bloomSeedAmount;
}

//-----------------------------------------------------------------------------

void Sun::inspectPostApply()
{
    conformLight();
    setMaskBits(UpdateMask);
}

ConsoleMethod(Sun, apply, void, 2, 2, "")
{
    object->inspectPostApply();
}

void Sun::unpackUpdate(NetConnection*, BitStream* stream)
{
    if (stream->readFlag())
    {
        // direction -> color -> ambient
        mathRead(*stream, &mLight.mDirection);

        stream->read(&mLight.mColor.red);
        stream->read(&mLight.mColor.green);
        stream->read(&mLight.mColor.blue);
        stream->read(&mLight.mColor.alpha);

        stream->read(&mLight.mAmbient.red);
        stream->read(&mLight.mAmbient.green);
        stream->read(&mLight.mAmbient.blue);
        stream->read(&mLight.mAmbient.alpha);

        stream->read(&mLight.mShadowColor.red);
        stream->read(&mLight.mShadowColor.green);
        stream->read(&mLight.mShadowColor.blue);
        stream->read(&mLight.mShadowColor.alpha);

        mLight.sgCastsShadows = stream->readFlag();

        stream->read(&useBloom);
        stream->read(&useToneMapping);
        stream->read(&useDynamicRangeLighting);

        stream->read(&DRLHighDynamicRange);
        stream->read(&DRLTarget);
        stream->read(&DRLMax);
        stream->read(&DRLMin);
        stream->read(&DRLMultiplier);

        stream->read(&bloomCutOff);
        stream->read(&bloomAmount);
        stream->read(&bloomSeedAmount);
    }
}

U32 Sun::packUpdate(NetConnection*, U32 mask, BitStream* stream)
{
    if (stream->writeFlag(mask & UpdateMask))
    {
        // direction -> color -> ambient
        mathWrite(*stream, mLight.mDirection);

        stream->write(mLight.mColor.red);
        stream->write(mLight.mColor.green);
        stream->write(mLight.mColor.blue);
        stream->write(mLight.mColor.alpha);

        stream->write(mLight.mAmbient.red);
        stream->write(mLight.mAmbient.green);
        stream->write(mLight.mAmbient.blue);
        stream->write(mLight.mAmbient.alpha);

        stream->write(mLight.mShadowColor.red);
        stream->write(mLight.mShadowColor.green);
        stream->write(mLight.mShadowColor.blue);
        stream->write(mLight.mShadowColor.alpha);

        stream->writeFlag(mLight.sgCastsShadows);

        stream->write(useBloom);
        stream->write(useToneMapping);
        stream->write(useDynamicRangeLighting);

        stream->write(DRLHighDynamicRange);
        stream->write(DRLTarget);
        stream->write(DRLMax);
        stream->write(DRLMin);
        stream->write(DRLMultiplier);

        stream->write(bloomCutOff);
        stream->write(bloomAmount);
        stream->write(bloomSeedAmount);
    }
    return(0);
}

//-----------------------------------------------------------------------------

void Sun::initPersistFields()
{
    Parent::initPersistFields();
    addGroup("Misc");
    addField("direction", TypePoint3F, Offset(mLight.mDirection, Sun));
    addField("color", TypeColorF, Offset(mLight.mColor, Sun));
    addField("ambient", TypeColorF, Offset(mLight.mAmbient, Sun));
    addField("shadowColor", TypeColorF, Offset(mLight.mShadowColor, Sun));

    addField("castsShadows", TypeBool, Offset(mLight.sgCastsShadows, Sun));
    endGroup("Misc");


    addGroup("Scene Lighting");
    addField("useBloom", TypeBool, Offset(useBloom, Sun));
    addField("useToneMapping", TypeBool, Offset(useToneMapping, Sun));
    addField("useDynamicRangeLighting", TypeBool, Offset(useDynamicRangeLighting, Sun));

    addField("DRLHighDynamicRange", TypeBool, Offset(DRLHighDynamicRange, Sun));
    addField("DRLTarget", TypeF32, Offset(DRLTarget, Sun));
    addField("DRLMax", TypeF32, Offset(DRLMax, Sun));
    addField("DRLMin", TypeF32, Offset(DRLMin, Sun));
    addField("DRLMultiplier", TypeF32, Offset(DRLMultiplier, Sun));

    addField("bloomCutOff", TypeF32, Offset(bloomCutOff, Sun));
    addField("bloomAmount", TypeF32, Offset(bloomAmount, Sun));
    addField("bloomSeedAmount", TypeF32, Offset(bloomSeedAmount, Sun));
    endGroup("Scene Lighting");
}