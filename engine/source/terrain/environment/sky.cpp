//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "terrain/environment/sky.h"
#include "math/mMath.h"
#include "console/console.h"
#include "console/consoleTypes.h"
#include "core/bitStream.h"
#include "sceneGraph/sceneGraph.h"
#include "sceneGraph/sceneState.h"
#include "terrain/terrData.h"
#include "sim/sceneObject.h"
#include "math/mathIO.h"
#include "sceneGraph/windingClipper.h"
#include "platform/profiler.h"
#include "gfx/primBuilder.h"
#include "gfx/gfxCanon.h"
#include "game/fx/particleEmitter.h"
#include "renderInstance/renderInstMgr.h"

//
#define HORIZON         0.0f
#define FOG_BAN_DETAIL  8
#define RAD             (2 * M_PI)


IMPLEMENT_CO_NETOBJECT_V1(Sky);

//Static Sky variables
bool Sky::smCloudsOn = true;
bool Sky::smCloudOutlineOn = false;
bool Sky::smSkyOn = true;
S32  Sky::smNumCloudsOn = MAX_NUM_LAYERS;

//Static Cloud variables
StormInfo Cloud::mGStormData;
F32 Cloud::mRadius;


//---------------------------------------------------------------------------
Sky::Sky()
{
    mNumCloudLayers = 0;
    mTypeMask |= EnvironmentObjectType;
    mNetFlags.set(Ghostable | ScopeAlways);

    mVisibleDistance = 250;
    mFogDistance = 250;
    mSkyTexturesOn = true;
    mRenderBoxBottom = false;
    mNumFogVolumes = 0;
    mFogLine = 0.0f;
    mRealFog = 0;
    mFogColor.set(128, 128, 128);
    mSolidFillColor.set(0.0, 1.0, 0, 0);
    mWindVelocity.set(1.f, 0.f, 0.f);
    mWindDir.set(0.f, 0.f);
    mNoRenderBans = false;

    mSkyGlow = false;

    mLastVisDisMod = -1;

    for (U32 j = 0; j < MaxFogVolumes; j++)
    {
        mFogVolumes[j].visibleDistance = -1;
        mFogVolumes[j].percentage = 1.0f;

        mFogVolumes[j].color.red = mFogColor.red;
        mFogVolumes[j].color.green = mFogColor.green;
        mFogVolumes[j].color.blue = mFogColor.blue;
    }

    for (S32 i = 0; i < MAX_NUM_LAYERS; ++i)
    {
        mCloudText[i] = "";
        mCloudSpeed[i] = 0.0001 * (1 + (i * 1));
        mCloudHeight[i] = 0;
    }

    mStormCloudData.state = isDone;
    mStormCloudData.speed = 0.0f;
    mStormCloudData.time = 0.0f;

    mStormFogData.time = 0.0f;
    mStormFogData.current = 0;
    mStormFogData.lastTime = 0;
    mStormFogData.startTime = 0;
    mStormFogData.endPercentage = -1.0f;
    for (S32 x = 0; x < MaxFogVolumes; ++x)
    {
        mStormFogData.volume[x].active = false;
        mStormFogData.volume[x].state = isDone;
        mStormFogData.volume[x].speed = 0.0f;
        mStormFogData.volume[x].endPercentage = 1.0f;
    }
    mFogTime = 0.0f;
    mFogVolume = -1;
    mStormCloudsOn = true;
    mStormFogOn = false;
    mSetFog = true;

    mLastForce16Bit = false;
    mLastForcePaletted = false;

    mSkyVB = NULL;
    mBanOffsetHeight = 50.0;

}

//------------------------------------------------------------------------------
Sky::~Sky()
{

}

//---------------------------------------------------------------------------
bool Sky::onAdd()
{
    if (!Parent::onAdd())
        return false;

    mObjBox.min.set(-1e9, -1e9, -1e9);
    mObjBox.max.set(1e9, 1e9, 1e9);
    resetWorldBox();

    // Find out how many fog layers we have.
    for (mNumFogVolumes = 0; mNumFogVolumes < MaxFogVolumes; mNumFogVolumes++)
        if (mFogVolumes[mNumFogVolumes].visibleDistance == -1 || mFogVolumes[mNumFogVolumes].visibleDistance == 0)
            break;

    // Storm fog layers are off by default, others are on.
    for (S32 i = 0; i < mNumFogVolumes; ++i)
        mFogVolumes[i].percentage = mStormFogData.volume[i].active ? 0.0f : 1.0f;

    if (isClientObject() || gSPMode)
    {
        if (!loadDml())
        {
            Con::errorf("Failed to load sky material list: %s", mMaterialListName);
            //return false;
        }

        loadVBPoints();

        initSkyData();
    }
    else
    {
        setWindVelocity(mWindVelocity);
        assignName("Sky");
    }

    addToScene();
    setVisibility();
    return true;
}

//---------------------------------------------------------------------------

void Sky::setSkyMaterial(const char* skyMaterial)
{
    mMaterialListName = StringTable->insert(skyMaterial);
    loadDml();

    setMaskBits(MaterialMask);
}

//---------------------------------------------------------------------------
void Sky::initSkyData()
{
    calcPoints();
    mWindDir = Point2F(mWindVelocity.x, mWindVelocity.y);
    mWindDir.normalize();
    for (S32 i = 0; i < MAX_NUM_LAYERS; ++i)
    {
        mCloudLayer[i].setHeights(mCloudHeight[i], mCloudHeight[i] - 0.05f, 0.00f);
        mCloudLayer[i].setSpeed(mWindDir * mCloudSpeed[i]);
        mCloudLayer[i].setPoints();
    }
    setVisibility();
}

//---------------------------------------------------------------------------
void Sky::setVisibility()
{
    if (mSceneManager)
    {
        mRealFogColor.red = S32(mCeil(mFogColor.red * 255.0f));
        mRealFogColor.green = S32(mCeil(mFogColor.green * 255.0f));
        mRealFogColor.blue = S32(mCeil(mFogColor.blue * 255.0f));
        mRealFogColor.alpha = 255;

        mSceneManager->setFogColor(mFogColor);

        mRealSkyColor.red = S32(mSolidFillColor.red * 255.0f);
        mRealSkyColor.green = S32(mSolidFillColor.green * 255.0f);
        mRealSkyColor.blue = S32(mSolidFillColor.blue * 255.0f);

        mSceneManager->setFogDistance(mFogDistance);
        mSceneManager->setVisibleDistance(mVisibleDistance);
        mSceneManager->setFogVolumes(mNumFogVolumes, mFogVolumes);
    }
}

//---------------------------------------------------------------------------
void Sky::setWindVelocity(const Point3F& vel)
{
    mWindVelocity = vel;
    ParticleEmitter::setWindVelocity(vel);
    if (isServerObject())
        setMaskBits(WindMask);
}

Point3F Sky::getWindVelocity()
{
    return(mWindVelocity);
}

//---------------------------------------------------------------------------
void Sky::onRemove()
{
    mSkyVB = NULL;

    removeFromScene();
    Parent::onRemove();
}

//---------------------------------------------------------------------------
ConsoleMethod(Sky, stormClouds, void, 4, 4, "(bool show, float duration)")
{
    Sky* ctrl = static_cast<Sky*>(object);
    ctrl->stormCloudsOn(dAtoi(argv[2]), dAtof(argv[3]));
}

ConsoleMethod(Sky, stormFog, void, 4, 4, "(float percent, float duration)")
{
    Sky* ctrl = static_cast<Sky*>(object);
    ctrl->stormFogOn(dAtof(argv[2]), dAtof(argv[3]));
}

ConsoleMethod(Sky, realFog, void, 6, 6, "( bool show, float max, float min, float speed )")
{
    Sky* ctrl = static_cast<Sky*>(object);
    ctrl->stormRealFog(dAtoi(argv[2]), dAtof(argv[3]), dAtof(argv[4]), dAtof(argv[5]));
}

ConsoleMethod(Sky, getWindVelocity, const char*, 2, 2, "()")
{
    Sky* sky = static_cast<Sky*>(object);
    char* retBuf = Con::getReturnBuffer(128);

    Point3F vel = sky->getWindVelocity();
    dSprintf(retBuf, 128, "%f %f %f", vel.x, vel.y, vel.z);
    return(retBuf);
}
//---------------------------------------------------------------------------

ConsoleMethod(Sky, setWindVelocity, void, 5, 5, "(float x, float y, float z)")
{
    Sky* sky = static_cast<Sky*>(object);
    if (sky->isClientObject())
        return;

    Point3F vel(dAtof(argv[2]), dAtof(argv[3]), dAtof(argv[4]));
    sky->setWindVelocity(vel);
}

ConsoleMethod(Sky, stormCloudsShow, void, 3, 3, "(bool showClouds)")
{
    Sky* ctrl = static_cast<Sky*>(object);
    ctrl->stormCloudsShow(dAtob(argv[2]));
}

ConsoleMethod(Sky, stormFogShow, void, 3, 3, "(bool show)")
{
    Sky* ctrl = static_cast<Sky*>(object);
    ctrl->stormFogShow(dAtob(argv[2]));
}

//---------------------------------------------------------------------------

ConsoleMethod(Sky, setSkyMaterial, void, 3, 3, "(material)")
{
    Sky* sky = static_cast<Sky*>(object);
    sky->setSkyMaterial(argv[2]);
}

//---------------------------------------------------------------------------
void Sky::initPersistFields()
{
    Parent::initPersistFields();

    addGroup("Media");
    addField("materialList", TypeFilename, Offset(mMaterialListName, Sky));
    endGroup("Media");

    addGroup("Clouds");
    addField("cloudText", TypeString, Offset(mCloudText, Sky), MAX_NUM_LAYERS);
    addField("cloudHeightPer", TypeF32, Offset(mCloudHeight, Sky), MAX_NUM_LAYERS);
    addField("cloudSpeed1", TypeF32, Offset(mCloudSpeed[0], Sky));
    addField("cloudSpeed2", TypeF32, Offset(mCloudSpeed[1], Sky));
    addField("cloudSpeed3", TypeF32, Offset(mCloudSpeed[2], Sky));
    endGroup("Clouds");

    addGroup("Visibility");
    addField("visibleDistance", TypeF32, Offset(mVisibleDistance, Sky));
    endGroup("Visibility");

    addGroup("Fog");
    addField("fogDistance", TypeF32, Offset(mFogDistance, Sky));
    addField("fogColor", TypeColorF, Offset(mFogColor, Sky));
    // Should be using the console's array support for these...
    addField("fogStorm1", TypeBool, Offset(mStormFogData.volume[0].active, Sky));
    addField("fogStorm2", TypeBool, Offset(mStormFogData.volume[1].active, Sky));
    addField("fogStorm3", TypeBool, Offset(mStormFogData.volume[2].active, Sky));
    addField("fogVolume1", TypePoint3F, Offset(mFogVolumes[0], Sky));
    addField("fogVolume2", TypePoint3F, Offset(mFogVolumes[1], Sky));
    addField("fogVolume3", TypePoint3F, Offset(mFogVolumes[2], Sky));
    endGroup("Fog");

    addGroup("Wind");
    addField("windVelocity", TypePoint3F, Offset(mWindVelocity, Sky));
    endGroup("Wind");

    addGroup("Misc");
    addField("SkySolidColor", TypeColorF, Offset(mSolidFillColor, Sky));
    addField("useSkyTextures", TypeBool, Offset(mSkyTexturesOn, Sky));
    addField("renderBottomTexture", TypeBool, Offset(mRenderBoxBottom, Sky));
    addField("noRenderBans", TypeBool, Offset(mNoRenderBans, Sky));
    addField("renderBanOffsetHeight", TypeF32, Offset(mBanOffsetHeight, Sky));

    addField("skyGlow", TypeBool, Offset(mSkyGlow, Sky));
    addField("skyGlowColor", TypeColorF, Offset(mSkyGlowColor, Sky));
    endGroup("Misc");

}

void Sky::consoleInit()
{
#if defined(TORQUE_DEBUG)
    Con::addVariable("pref::CloudOutline", TypeBool, &smCloudOutlineOn);
#endif
    Con::addVariable("pref::CloudsOn", TypeBool, &smCloudsOn);

    Con::addVariable("pref::NumCloudLayers", TypeS32, &smNumCloudsOn);
    Con::addVariable("pref::SkyOn", TypeBool, &smSkyOn);
}
//---------------------------------------------------------------------------
void Sky::unpackUpdate(NetConnection*, BitStream* stream)
{
    if (stream->readFlag())
    {
        mMaterialListName = stream->readSTString();
        loadDml();

        stream->read(&mFogColor.red);
        stream->read(&mFogColor.green);
        stream->read(&mFogColor.blue);
        stream->read(&mNumFogVolumes);
        stream->read(&mSkyTexturesOn);
        stream->read(&mRenderBoxBottom);
        stream->read(&mSolidFillColor.red);
        stream->read(&mSolidFillColor.green);
        stream->read(&mSolidFillColor.blue);
        mNoRenderBans = stream->readFlag();
        stream->read(&mBanOffsetHeight);

        U32 i;
        for (i = 0; i < mNumFogVolumes; i++)
        {
            stream->read(&mFogVolumes[i].visibleDistance);
            stream->read(&mFogVolumes[i].minHeight);
            stream->read(&mFogVolumes[i].maxHeight);

            stream->read(&mFogVolumes[i].color.red);
            stream->read(&mFogVolumes[i].color.green);
            stream->read(&mFogVolumes[i].color.blue);

            stream->read(&mStormFogData.volume[i].active);
        }
        for (i = 0; i < MAX_NUM_LAYERS; i++)
        {
            mCloudText[i] = stream->readSTString();
            stream->read(&mCloudHeight[i]);
            stream->read(&mCloudSpeed[i]);
        }
        initSkyData();
        Point3F vel;
        if (mathRead(*stream, &vel))
            setWindVelocity(vel);

        stream->read(&mFogVolume);
        if (stream->readFlag())
        {
            U32 state, stormTimeDiff;
            stream->read(&mFogPercentage);
            stream->read(&mStormFogData.time);
            stream->read(&state);
            stream->read(&stormTimeDiff);
            stream->read(&mStormFogData.endPercentage);
            mStormFogData.state = SkyState(state);
            if (mStormFogData.time)
            {
                Con::printf("Server Storm Time: %u", stormTimeDiff);
                Con::printf("Get Current Time: %u", Sim::getCurrentTime());
                mStormFogOn = true;
                mStormFogData.lastTime = Sim::getCurrentTime() - stormTimeDiff;
                Con::printf("READ OFFSET: %f", F32(Sim::getCurrentTime() - mStormFogData.lastTime) / 32.0f);
                for (S32 x = 0; x < mNumFogVolumes; ++x)
                {
                    if (mStormFogData.volume[x].active)
                        mFogVolumes[x].percentage = mFogPercentage;
                    mStormFogData.volume[x].endPercentage =
                        mStormFogData.volume[x].active ? mStormFogData.endPercentage :
                        mFogVolumes[x].percentage;
                    mStormFogData.volume[x].speed = (mStormFogData.endPercentage - mFogVolumes[x].percentage) / ((mStormFogData.time * 32.0f) / (F32)mNumFogVolumes);
                    mStormFogData.volume[x].state = mStormFogData.state;
                    if (mStormFogData.volume[x].state == comingIn)
                        mStormFogData.current = 0;
                    else
                        mStormFogData.current = mNumFogVolumes - 1;
                }
            }
        }
    }

    if (stream->readFlag())
    {
        mMaterialListName = stream->readSTString();
        loadDml();
    }

    if (stream->readFlag())
        stream->read(&mStormCloudsOn);

    if (stream->readFlag())
    {
        stream->read(&mStormFogOn);
        if (!mStormFogOn)
        {
            for (S32 x = 0; x < mNumFogVolumes; x++)
                if (mStormFogData.volume[x].active)
                    mFogVolumes[x].percentage = 0.0f;
            mSetFog = true;
        }
    }

    if (stream->readFlag())
    {
        stream->read(&mVisibleDistance);
        stream->read(&mFogDistance);
        initSkyData();
    }
    if (stream->readFlag())
    {
        U32 state;
        stream->read(&state);
        mStormCloudData.state = SkyState(state);
        stream->read(&mStormCloudData.time);

        if (mStormCloudData.time > 0.0f)
        {
            mStormCloudData.speed = ((mRadius * 2) * F32(mNumCloudLayers + 1)) / (mStormCloudData.time * 32.0f);
            if (mNumCloudLayers)
                mStormCloudData.fadeSpeed = 1.0f / (((mStormCloudData.time * 32.0f) / F32(mNumCloudLayers + 1)) / mNumCloudLayers);
            startStorm();
        }
    }
    if (stream->readFlag())
    {
        stream->read(&mFogPercentage);
        stream->read(&mStormFogData.time);
        stream->read(&mFogVolume);
        if (mStormFogData.time)
        {
            mStormFogData.lastTime = Sim::getCurrentTime();
            startStormFog();
        }
    }
    if (stream->readFlag())
    {
        stream->read(&mRealFog);
        stream->read(&mRealFogMax);
        stream->read(&mRealFogMin);
        stream->read(&mRealFogSpeed);
        if (mRealFog)
        {
            for (S32 x = 0; x < mNumFogVolumes; ++x)
            {
                mStormFogData.volume[x].lastPercentage = mRealFogMax;
                mStormFogData.volume[x].endPercentage = mRealFogMin;
                mStormFogData.volume[x].speed = -(((mNumFogVolumes - x) * (mNumFogVolumes - x)) * mRealFogSpeed);
            }
            F32 save = mStormFogData.volume[0].speed;
            mStormFogData.volume[0].speed = mStormFogData.volume[1].speed;
            mStormFogData.volume[1].speed = save;
        }
    }

    if (stream->readFlag())
    {
        Point3F vel;
        if (mathRead(*stream, &vel))
            setWindVelocity(vel);
    }

    if (stream->readFlag())
    {
        if (mSkyGlow = stream->readFlag())
        {
            stream->read(&mSkyGlowColor.red);
            stream->read(&mSkyGlowColor.green);
            stream->read(&mSkyGlowColor.blue);
        }
    }

}

//---------------------------------------------------------------------------
U32 Sky::packUpdate(NetConnection*, U32 mask, BitStream* stream)
{
    if (stream->writeFlag(mask & InitMask))
    {
        stream->writeString(mMaterialListName);
        stream->write(mFogColor.red);
        stream->write(mFogColor.green);
        stream->write(mFogColor.blue);
        stream->write(mNumFogVolumes);
        stream->write(mSkyTexturesOn);
        stream->write(mRenderBoxBottom);
        stream->write(mSolidFillColor.red);
        stream->write(mSolidFillColor.green);
        stream->write(mSolidFillColor.blue);
        stream->writeFlag(mNoRenderBans);
        stream->write(mBanOffsetHeight);


        U32 i;
        for (i = 0; i < mNumFogVolumes; i++)
        {
            stream->write(mFogVolumes[i].visibleDistance);
            stream->write(mFogVolumes[i].minHeight);
            stream->write(mFogVolumes[i].maxHeight);

            stream->write(mFogVolumes[i].color.red);
            stream->write(mFogVolumes[i].color.green);
            stream->write(mFogVolumes[i].color.blue);

            stream->write(mStormFogData.volume[i].active);
        }
        for (i = 0; i < MAX_NUM_LAYERS; i++)
        {
            stream->writeString(mCloudText[i]);
            stream->write(mCloudHeight[i]);
            stream->write(mCloudSpeed[i]);
        }
        mathWrite(*stream, mWindVelocity);

        stream->write(mFogVolume);
        U32 currentTime = Sim::getCurrentTime();
        U32 stormTimeDiff = currentTime - mStormFogData.startTime;
        if (stream->writeFlag(F32(stormTimeDiff) / 1000.0f < mStormFogData.time))
        {
            stream->write(mFogPercentage);
            stream->write(mStormFogData.time);
            stream->write(U32(mStormFogData.state));
            stream->write(stormTimeDiff);
            stream->write(mStormFogData.endPercentage);
            Con::printf("WRITE OFFSET: %f", F32(stormTimeDiff) / 32.0f);
        }
    }

    if (stream->writeFlag(mask & MaterialMask))
        stream->writeString(mMaterialListName);

    if (stream->writeFlag(mask & StormCloudsOnMask))
        stream->write(mStormCloudsOn);

    if (stream->writeFlag(mask & StormFogOnMask && !(mask & InitMask)))
        stream->write(mStormFogOn);

    if (stream->writeFlag(mask & VisibilityMask))
    {
        stream->write(mVisibleDistance);
        stream->write(mFogDistance);
    }

    if (stream->writeFlag(mask & StormCloudMask))
    {
        stream->write(U32(mStormCloudData.state));
        stream->write(mStormCloudData.time);
    }

    if (stream->writeFlag(mask & StormFogMask && !(mask & InitMask)))
    {
        stream->write(mStormFogData.endPercentage);
        stream->write(mStormFogData.time);
        stream->write(mFogVolume);
        mStormFogData.startTime = Sim::getCurrentTime();
    }

    if (stream->writeFlag(mask & StormRealFogMask))
    {
        stream->write(mRealFog);
        stream->write(mRealFogMax);
        stream->write(mRealFogMin);
        stream->write(mRealFogSpeed);
    }

    if (stream->writeFlag(mask & WindMask))
        mathWrite(*stream, mWindVelocity);

    if (stream->writeFlag(mask & SkyGlowMask))
    {
        if (stream->writeFlag(mSkyGlow))
        {
            stream->write(mSkyGlowColor.red);
            stream->write(mSkyGlowColor.green);
            stream->write(mSkyGlowColor.blue);
        }
    }

    return 0;
}

//---------------------------------------------------------------------------
void Sky::inspectPostApply()
{
    for (mNumFogVolumes = 0; mNumFogVolumes < MaxFogVolumes; mNumFogVolumes++)
        if (mFogVolumes[mNumFogVolumes].visibleDistance == -1 || mFogVolumes[mNumFogVolumes].visibleDistance == 0)
            break;
    setMaskBits(InitMask | VisibilityMask | SkyGlowMask);
}

//---------------------------------------------------------------------------
void Sky::renderObject(SceneState* state, RenderInst* ri)
{
    //   GFX_Canonizer("Sky::renderObject", __FILE__, __LINE__);


    GFX->setBaseRenderState();

    RectI viewport;

    viewport = GFX->getViewport();

    // Clear the objects viewport to the fog color.  This is something of a dirty trick,
    //  since we want an identity projection matrix here...
    MatrixF proj = GFX->getProjectionMatrix();

    state->setupObjectProjection(this);

    GFX->setProjectionMatrix(MatrixF(true));

    GFX->pushWorldMatrix();
    GFX->setWorldMatrix(MatrixF(true));

    GFX->setZEnable(true);
    GFX->setZWriteEnable(false);

    GFX->setAlphaBlendEnable(false);
    GFX->setSrcBlend(GFXBlendOne);
    GFX->setDestBlend(GFXBlendZero);

    // Is it a glow pass?
 //   if(image->glow)
 //      PrimBuild::color3f(mSkyGlowColor.red,mSkyGlowColor.green,mSkyGlowColor.blue);
 //   else
    PrimBuild::color3i(U8(mRealFogColor.red), U8(mRealFogColor.green), U8(mRealFogColor.blue));

    GFX->setupGenericShaders(GFXDevice::GSColor);

    PrimBuild::begin(GFXTriangleFan, 4);
    PrimBuild::vertex3f(-1, -1, 1);
    PrimBuild::vertex3f(-1, 1, 1);
    PrimBuild::vertex3f(1, 1, 1);
    PrimBuild::vertex3f(1, -1, 1);
    PrimBuild::end();


    // this fixes oblique frustum clip prob on planar reflections
    /*if (getCurrentClientSceneGraph()->isReflectPass())
    {
        GFX->setProjectionMatrix(getCurrentClientSceneGraph()->getNonClipProjection());
    }
    else
    {*/
        GFX->setProjectionMatrix(proj);
    //}

    GFX->popWorldMatrix();

    // On input.  Finalize the projection matrix...
    state->setupObjectProjection(this);

    GFX->pushWorldMatrix();

    Point3F camPos = state->getCameraPosition();

    MatrixF tMat(1);
    tMat.setPosition(camPos);

    GFX->multWorld(tMat);

    GFX->setZEnable(true);
    GFX->setAlphaBlendEnable(false);

    render(state);

    GFX->setZWriteEnable(true);
    GFX->setZEnable(true);

    GFX->setProjectionMatrix(proj);

    GFX->popWorldMatrix();

    GFX->setViewport(viewport);
}

//---------------------------------------------------------------------------
bool Sky::prepRenderImage(SceneState* state, const U32 stateKey,
    const U32 startZone, const bool modifyBaseState)
{
    startZone, modifyBaseState;
    AssertFatal(modifyBaseState == false, "Error, should never be called with this parameter set");
    AssertFatal(startZone == 0xFFFFFFFF, "Error, startZone should indicate -1");

    if (isLastState(state, stateKey))
        return false;
    setLastState(state, stateKey);

    // This should be sufficient for most objects that don't manage zones, and
    //  don't need to return a specialized RenderImage...
    if (state->isObjectRendered(this))
    {
        RenderInst* ri = gRenderInstManager.allocInst();
        ri->obj = this;
        ri->state = state;
        ri->type = RenderInstManager::RIT_Sky;
        gRenderInstManager.addInst(ri);

    }

    return false;
}

//---------------------------------------------------------------------------
void Sky::render(SceneState* state)
{
    PROFILE_START(SkyRender);

    F32 banHeights[2] = { -(mSpherePt.z - 1),-(mSpherePt.z - 1) };
    F32 alphaBan[2] = { 0.0f, 0.0f };
    F32 depthInFog = 0.0f;
    S32 index = 0;
    Point3F camPos;

    if (getCurrentClientSceneGraph())
    {
        F32 currentVisDis = getCurrentClientSceneGraph()->getVisibleDistanceMod();
        if (mLastVisDisMod != currentVisDis)
        {
            calcPoints();
            for (S32 i = 0; i < MAX_NUM_LAYERS; ++i)
                mCloudLayer[i].setPoints();

            mLastVisDisMod = currentVisDis;
        }
    }

    if (mNumFogVolumes)
    {
        camPos = state->getCameraPosition();

        if (getCurrentClientSceneGraph()->isReflectPass())
        {
            camPos = getCurrentClientSceneGraph()->mNormCamPos;
        }

        depthInFog = -(camPos.z - mFogLine);
    }

    // Calculates alpha values and ban heights
    if (depthInFog > 0.0f)
    {
        calcAlphas_Heights(camPos.z, banHeights, alphaBan, depthInFog);
    }
    else // Not in fog so setup default values
    {
        alphaBan[0] = 0.0f;
        alphaBan[1] = 0.0f;
        banHeights[0] = HORIZON;
        banHeights[1] = banHeights[0] + mBanOffsetHeight;
    }

    // if lower ban is at top of box then no clipping plane is needed
    if (banHeights[0] >= mSpherePt.z)
        banHeights[0] = banHeights[1] = mSpherePt.z;

    //Renders the 6 sides of the sky box
    if (alphaBan[1] < 1.0f || mNumFogVolumes == 0)
        renderSkyBox(banHeights[0], alphaBan[1]);

    // if completly fogged out then no need to render
    if (alphaBan[1] < 1.0f || depthInFog < 0.0f)
    {
        if (smCloudsOn && mStormCloudsOn && smSkyOn)
        {
            F32 ang = mAtan(banHeights[0], mSkyBoxPt.x);
            F32 xyval = mSin(ang);
            F32 zval = mCos(ang);
            PlaneF planes[4];
            planes[0] = PlaneF(xyval, 0.0f, zval, 0.0f);
            planes[1] = PlaneF(-xyval, 0.0f, zval, 0.0f);
            planes[2] = PlaneF(0.0f, xyval, zval, 0.0f);
            planes[3] = PlaneF(0.0f, -xyval, zval, 0.0f);

            S32 numRender = (smNumCloudsOn > mNumCloudLayers) ? mNumCloudLayers : smNumCloudsOn;
            for (S32 x = 0; x < numRender; ++x)
                mCloudLayer[x].render(Sim::getCurrentTime(), x, smCloudOutlineOn, mNumCloudLayers, planes);
        }

        if (!mNoRenderBans)
        {
            Point3F banPoints[2][MAX_BAN_POINTS];
            Point3F cornerPoints[MAX_BAN_POINTS];

            // Calculate upper, lower, and corner ban points
            calcBans(banHeights, banPoints, cornerPoints);

            GFX->setTexture(0, NULL);

            // Renders the side, top, and corner bans
            renderBans(alphaBan, banHeights, banPoints, cornerPoints);
        }

        GFX->setAlphaBlendEnable(false);
    }

    if (mSetFog)
    {
        mSceneManager->setFogVolumes(mNumFogVolumes, mFogVolumes);
        mSetFog = false;
    }
    if (mStormFogOn && mStormFogData.volume[mStormFogData.current].state != isDone)
        updateFog();
    if (mStormFogOn && mRealFog)
        updateRealFog();
    PROFILE_END();
}

//---------------------------------------------------------------------------
void Sky::calcAlphas_Heights(F32 zCamPos, F32* banHeights,
    F32* alphaBan, F32 depthInFog)
{
    F32 sideA, lower = 0.0f, upper = 0.0f;
    F32 visValue, visDis = 0.0f;
    F32 visRatio, ratioVal, setVis;

    visDis = setVis = mVisibleDistance +
        (mFogVolumes[mNumFogVolumes - 1].visibleDistance - mVisibleDistance) *
        mFogVolumes[mNumFogVolumes - 1].percentage;

    for (S32 x = 0; x < mNumFogVolumes - 1; ++x)
    {
        F32 distance = mVisibleDistance +
            (mFogVolumes[x].visibleDistance - mVisibleDistance) *
            mFogVolumes[x].percentage;
        if (distance < setVis)
        {
            visValue = (zCamPos < mFogVolumes[x].minHeight) ?
                mFogVolumes[x].maxHeight - mFogVolumes[x].minHeight :
                mFogVolumes[x].maxHeight - zCamPos;
            if (visValue > 0.0f)
            {
                ratioVal = setVis / distance;
                visRatio = visValue / distance;
                visDis -= (distance * visRatio) * ratioVal;
            }
        }
    }

    //Calculate upper Height
    if (visDis > 0.0f)
        upper = (mSkyBoxPt.x * depthInFog) / (visDis * 0.2f);

    banHeights[1] = mSpherePt.z;

    if (upper < mSpherePt.z)
    {
        banHeights[1] = upper;
        if (banHeights[1] < mBanOffsetHeight)
            banHeights[1] = mBanOffsetHeight + HORIZON;
    }

    if (visDis > depthInFog)
    {
        sideA = mSqrt((visDis * visDis) - (depthInFog * depthInFog));
        lower = (mSkyBoxPt.x * depthInFog) / sideA;

        //Calculate lower Height
        banHeights[0] = mSpherePt.z;
        if (lower < mSpherePt.z)
            banHeights[0] = lower;

        if (banHeights[0] == mSpherePt.z && banHeights[1] == mSpherePt.z)
        {
            F32 temp = ((lower - mSpherePt.z) * (sideA / depthInFog));
            if (temp <= mSkyBoxPt.x)
                alphaBan[1] = temp / mSkyBoxPt.x;
            else
                alphaBan[1] = 1.0f;

            alphaBan[0] = 1.0f;
        }
        else
        {
            alphaBan[0] = banHeights[0] / mSpherePt.z;
            alphaBan[1] = 0.0f;
        }
    }
    else
    {
        alphaBan[1] = alphaBan[0] = 1.0f;
        banHeights[0] = banHeights[1] = mSpherePt.z;
    }
    banHeights[0] *= mFogVolumes[0].percentage;
    banHeights[1] *= mFogVolumes[0].percentage;

    if (banHeights[1] < mBanOffsetHeight)
        banHeights[1] = mBanOffsetHeight + HORIZON;
}

void Sky::setRenderPoints(Point3F* renderPoints, S32 index)
{
    renderPoints[0].set(mPoints[index].x, mPoints[index].y, mPoints[index].z);
    renderPoints[1].set(mPoints[index + 1].x, mPoints[index + 1].y, mPoints[index + 1].z);
    renderPoints[2].set(mPoints[index + 6].x, mPoints[index + 6].y, mPoints[index + 6].z);
    renderPoints[3].set(mPoints[index + 5].x, mPoints[index + 5].y, mPoints[index + 5].z);
}

void Sky::calcTexCoords(Point2F* texCoords, Point3F* renderPoints, S32 index, F32 lowerBanHeight)
{

    for (S32 x = 0; x < 4; ++x)
        texCoords[x].set(mTexCoord[x].x, mTexCoord[x].y);
    S32 length = (S32)(mFabs(mPoints[index].z) + mFabs(mPoints[index + 5].z));
    F32 per = mPoints[index].z - renderPoints[3].z;

    texCoords[3].y = texCoords[2].y = (per / length);
}

//---------------------------------------------------------------------------
void Sky::renderSkyBox(F32 lowerBanHeight, F32 alphaBanUpper)
{
    S32 side, index = 0, val;
    U32 numPoints;
    Point3F renderPoints[4];
    Point2F texCoords[4];

    GFX->setBaseRenderState();
#ifndef MB_FLIP_SKY
    GFX->setZEnable(false);
#endif
    GFX->setZWriteEnable(false);
    GFX->setCullMode(GFXCullNone);
    PrimBuild::color4f(1, 1, 1, 1);

    if (!mSkyTexturesOn || !smSkyOn)
    {
        GFX->setTexture(0, NULL);
        PrimBuild::color3i(U8(mRealSkyColor.red), U8(mRealSkyColor.green), U8(mRealSkyColor.blue));
    }

    for (side = 0; side < ((mRenderBoxBottom) ? 6 : 5); ++side)
    {
        if ((lowerBanHeight != mSpherePt.z || (side == 4 && alphaBanUpper < 1.0f)) && mSkyHandle[side])
        {
            GFX->setAlphaBlendEnable(false);
            if (mSkyTexturesOn && smSkyOn)
            {
                GFX->setTexture(0, mSkyHandle[side]);
            } else {
                GFX->setTexture(0, NULL);
                PrimBuild::color3i(U8(mRealSkyColor.red), U8(mRealSkyColor.green), U8(mRealSkyColor.blue));
            }
            GFX->setTextureStageMagFilter(0, GFXTextureFilterLinear);
            GFX->setTextureStageMinFilter(0, GFXTextureFilterLinear);
            GFX->setTextureStageColorOp(0, GFXTOPModulate);
            GFX->setTextureStageColorOp(1, GFXTOPDisable);
            GFX->setTextureStageAddressModeU(0, GFXAddressClamp);
            GFX->setTextureStageAddressModeV(0, GFXAddressClamp);

            // If it's one of the sides...
            if (side < 4)
            {
                numPoints = 4;
                setRenderPoints(renderPoints, index);

                if (!mNoRenderBans)
                    sgUtil_clipToPlane(renderPoints, numPoints, PlaneF(0.0f, 0.0f, 1.0f, -lowerBanHeight));

                if (numPoints)
                {
                    calcTexCoords(texCoords, renderPoints, index, lowerBanHeight);

                    GFX->setupGenericShaders(GFXDevice::GSModColorTexture);

                    PrimBuild::begin(GFXTriangleFan, 4);
                    PrimBuild::texCoord2f(texCoords[0].x, texCoords[0].y);
                    PrimBuild::vertex3f(renderPoints[0].x, renderPoints[0].y, renderPoints[0].z);

                    PrimBuild::texCoord2f(texCoords[1].x, texCoords[1].y);
                    PrimBuild::vertex3f(renderPoints[1].x, renderPoints[1].y, renderPoints[1].z);

                    PrimBuild::texCoord2f(texCoords[2].x, texCoords[2].y);
                    PrimBuild::vertex3f(renderPoints[2].x, renderPoints[2].y, renderPoints[2].z);

                    PrimBuild::texCoord2f(texCoords[3].x, texCoords[3].y);
                    PrimBuild::vertex3f(renderPoints[3].x, renderPoints[3].y, renderPoints[3].z);
                    PrimBuild::end();

                }
                index++;
            }
            else
            {
                index = 3;
                val = -1;
                if (side == 5)
                {
                    index = 5;
                    val = 1;
                }

                GFX->setupGenericShaders(GFXDevice::GSModColorTexture);

                PrimBuild::begin(GFXTriangleFan, 4);
                PrimBuild::texCoord2f(mTexCoord[0].x, mTexCoord[0].y);
                PrimBuild::vertex3f(mPoints[index].x, mPoints[index].y, mPoints[index].z);
                PrimBuild::texCoord2f(mTexCoord[1].x, mTexCoord[1].y);
                PrimBuild::vertex3f(mPoints[index + (1 * val)].x, mPoints[index + (1 * val)].y, mPoints[index + (1 * val)].z);
                PrimBuild::texCoord2f(mTexCoord[2].x, mTexCoord[2].y);
                PrimBuild::vertex3f(mPoints[index + (2 * val)].x, mPoints[index + (2 * val)].y, mPoints[index + (2 * val)].z);
                PrimBuild::texCoord2f(mTexCoord[3].x, mTexCoord[3].y);
                PrimBuild::vertex3f(mPoints[index + (3 * val)].x, mPoints[index + (3 * val)].y, mPoints[index + (3 * val)].z);
                PrimBuild::end();
            }

        }
    }
}

//---------------------------------------------------------------------------
void Sky::calcBans(F32* banHeights, Point3F banPoints[][MAX_BAN_POINTS], Point3F* cornerPoints)
{
    F32 incRad = RAD / F32(FOG_BAN_DETAIL * 2);
    MatrixF ban;
    Point4F point;
    S32 index, x;
    F32 height = banHeights[0];

    F32 value = banHeights[0] / mSkyBoxPt.z;
    F32 mulVal = -(mSqrt(1 - (value * value))); // lowerBan Multiple
    index = 0;

    // Calculates the upper and lower bans
    for (x = 0; x < 2; ++x)
    {
        for (F32 angle = 0.0f; angle <= RAD + incRad; angle += incRad)
        {
            ban.set(Point3F(0.0f, 0.0f, angle));
            point.set(mulVal * mSkyBoxPt.x, 0.0f, 0.0f, 1.0f);
            ban.mul(point);
            banPoints[x][index].set(point.x, point.y, height);
            index++;
        }
        height = banHeights[1];
        value = banHeights[1] / mSkyBoxPt.x;
        value = mClampF(value, 0.0, 1.0);
        mulVal = -(mSqrt(1.0 - (value * value))); // upperBan Multiple
        index = 0;
    }

    // Calculates the filler points needed between the lower ban and the clipping plane
    index = 2;
    cornerPoints[0].set(mPoints[3].x, mPoints[3].y, banHeights[0] - 1);
    cornerPoints[1].set(mPoints[3].x, 0.0f, banHeights[0] - 1);

    for (x = 0; x < (FOG_BAN_DETAIL / 2.0f) + 1.0f; ++x)
        cornerPoints[index++].set(banPoints[0][x].x, banPoints[0][x].y, banPoints[0][x].z);
    cornerPoints[index].set(0.0f, mPoints[3].y, banHeights[0] - 1);
}

//---------------------------------------------------------------------------
void Sky::renderBans(F32* alphaBan, F32* banHeights, Point3F banPoints[][MAX_BAN_POINTS], Point3F* cornerPoints)
{
    S32 side, x, index = 0;
    F32 angle;
    U8 UalphaIn = U8(alphaBan[1] * 255);
    U8 UalphaOut = U8(alphaBan[0] * 255);

    GFX->setBaseRenderState();
    GFX->setZEnable(false);
    GFX->setZWriteEnable(false);
    GFX->setTextureStageColorOp(0, GFXTOPDisable);
    GFX->setAlphaBlendEnable(true);
    GFX->setSrcBlend(GFXBlendSrcAlpha);
    GFX->setDestBlend(GFXBlendInvSrcAlpha);

    //Renders the side bans
    if (banHeights[0] < mSpherePt.z)
    {
        GFX->setupGenericShaders(GFXDevice::GSColor);
        PrimBuild::begin(GFXTriangleStrip, 2 * (FOG_BAN_DETAIL * 2 + 1));
        for (x = 0; x < FOG_BAN_DETAIL * 2 + 1; ++x)
        {
            PrimBuild::color4i(U8(mRealFogColor.red), U8(mRealFogColor.green), U8(mRealFogColor.blue), 255);
            PrimBuild::vertex3f(banPoints[0][index].x, banPoints[0][index].y, banPoints[0][index].z);

            PrimBuild::color4i(U8(mRealFogColor.red), U8(mRealFogColor.green), U8(mRealFogColor.blue), UalphaOut);
            PrimBuild::vertex3f(banPoints[1][index].x, banPoints[1][index].y, banPoints[1][index].z);
            ++index;
        }
        PrimBuild::end();
    }

    //Renders the top ban
    GFX->setupGenericShaders(GFXDevice::GSColor);
    PrimBuild::begin(GFXTriangleFan, 2 * (FOG_BAN_DETAIL * 2 + 1));

    PrimBuild::color4i(U8(mRealFogColor.red), U8(mRealFogColor.green), U8(mRealFogColor.blue), UalphaIn);
    PrimBuild::vertex3f(mTopCenterPt.x, mTopCenterPt.y, mTopCenterPt.z);

    for (x = 0; x < FOG_BAN_DETAIL * 2 + 1; ++x)
    {
        PrimBuild::color4i(U8(mRealFogColor.red), U8(mRealFogColor.green), U8(mRealFogColor.blue), UalphaOut);
        PrimBuild::vertex3f(banPoints[1][x].x, banPoints[1][x].y, banPoints[1][x].z);
    }

    PrimBuild::end();

    GFX->pushWorldMatrix();

    angle = 0.0f;

    //Renders the filler
    for (side = 0; side < 4; ++side)
    {
        // Rotate stuff
        AngAxisF rotAAF(Point3F(0, 0, 1), angle);
        MatrixF m;
        rotAAF.setMatrix(&m);
        GFX->multWorld(m);

        GFX->setupGenericShaders(GFXDevice::GSColor);

        PrimBuild::begin(GFXTriangleFan, FOG_BAN_DETAIL);
        for (x = 0; x < FOG_BAN_DETAIL; ++x)
        {
            PrimBuild::color4i(U8(mRealFogColor.red), U8(mRealFogColor.green), U8(mRealFogColor.blue), 255);
            PrimBuild::vertex3f(cornerPoints[x].x, cornerPoints[x].y, cornerPoints[x].z);
        }
        PrimBuild::end();

        angle += M_PI / 2.f; // 90.0f;
    }

    GFX->popWorldMatrix();
}

//---------------------------------------------------------------------------
void Sky::startStorm()
{
    mStormCloudsOn = true;
    Cloud::startStorm(mStormCloudData.state);
    for (int i = 0; i < mNumCloudLayers; ++i)
        mCloudLayer[i].calcStorm(mStormCloudData.speed, mStormCloudData.fadeSpeed);
}

void Sky::stormCloudsShow(bool show)
{
    mStormCloudsOn = show;
    setMaskBits(StormCloudsOnMask);
}

void Sky::stormFogShow(bool show)
{
    mStormFogOn = show;
    setMaskBits(StormFogOnMask);
}

//---------------------------------------------------------------------------
void Sky::startStormFog()
{
    if (mStormFogOn)
        for (S32 x = 0; x < mNumFogVolumes; ++x)
            mFogVolumes[x].percentage = mStormFogData.volume[x].endPercentage;

    mStormFogOn = true;
    if (mFogVolume < 0)
        for (S32 x = 0; x < mNumFogVolumes; ++x)
        {
            mStormFogData.volume[x].speed = (mFogPercentage - mFogVolumes[x].percentage) / ((mStormFogData.time * 32.0f) / (F32)mNumFogVolumes);
            mStormFogData.volume[x].state = (mStormFogData.volume[x].endPercentage > mFogPercentage) ? goingOut : comingIn;
            mStormFogData.volume[x].endPercentage = mStormFogData.volume[x].active ? mFogPercentage :
                mFogVolumes[x].percentage;
            if (mStormFogData.volume[x].state == comingIn)
                mStormFogData.current = 0;
            else
                mStormFogData.current = mNumFogVolumes - 1;
        }
    else if (mFogVolume < mNumFogVolumes)
    {
        mStormFogData.volume[mFogVolume].speed = (mFogPercentage - mFogVolumes[mFogVolume].percentage) / ((mStormFogData.time * 32.0f) / (F32)mNumFogVolumes);
        mStormFogData.volume[mFogVolume].state = (mFogVolumes[mFogVolume].percentage > mFogPercentage) ? goingOut : comingIn;
        mStormFogData.volume[mFogVolume].endPercentage = mFogPercentage;
        mStormFogData.current = mFogVolume;
    }
}

//---------------------------------------------------------------------------
void Sky::updateFog()
{
    F32 overFlow, offset = 1.0f;
    U32 currentTime = Sim::getCurrentTime();

    if (mStormFogData.lastTime != 0)
        offset = F32(currentTime - mStormFogData.lastTime) / 32.0f;
    Con::printf("OFFSET: %f", offset);
    mStormFogData.lastTime = currentTime;

    mFogVolumes[mStormFogData.current].percentage += (mStormFogData.volume[mStormFogData.current].speed * offset);
    do
    {
        Con::printf("CURRENT: %d PERCENTAGE: %f TIME: %u", mStormFogData.current, mFogVolumes[mStormFogData.current].percentage, currentTime);
        overFlow = 0.0f;
        if (mStormFogData.volume[mStormFogData.current].state == comingIn && mFogVolumes[mStormFogData.current].percentage >= mStormFogData.volume[mStormFogData.current].endPercentage)
        {
            overFlow = mFogVolumes[mStormFogData.current].percentage - mStormFogData.volume[mStormFogData.current].endPercentage;
            mFogVolumes[mStormFogData.current].percentage = mStormFogData.volume[mStormFogData.current].endPercentage;
            mStormFogData.volume[mStormFogData.current].state = isDone;
            if (++mStormFogData.current >= mNumFogVolumes)
            {
                mStormFogData.current -= 1;
                mStormFogData.lastTime = 0;
                mStormFogOn = false;
                Con::printf("FOG IS DONE");
            }
            else
                mFogVolumes[mStormFogData.current].percentage += overFlow;
        }
        else if (mStormFogData.volume[mStormFogData.current].state == goingOut && mFogVolumes[mStormFogData.current].percentage <= mStormFogData.volume[mStormFogData.current].endPercentage)
        {
            overFlow = mStormFogData.volume[mStormFogData.current].endPercentage - mFogVolumes[mStormFogData.current].percentage;
            mFogVolumes[mStormFogData.current].percentage = mStormFogData.volume[mStormFogData.current].endPercentage;
            mStormFogData.volume[mStormFogData.current].state = isDone;
            if (--mStormFogData.current < 0)
            {
                mStormFogData.current += 1;
                mStormFogData.lastTime = 0;
                mStormFogOn = false;
                Con::printf("FOG IS DONE");
            }
            else
                mFogVolumes[mStormFogData.current].percentage -= overFlow;
        }
    } while (overFlow > 0.0f && mStormFogOn);
    //   if(mStormFogData.volume[mStormFogData.current].state != done)
    mSceneManager->setFogVolumes(mNumFogVolumes, mFogVolumes);
}

//---------------------------------------------------------------------------
void Sky::updateRealFog()
{
    for (S32 x = 0; x < mNumFogVolumes; ++x)
    {
        mFogVolumes[x].percentage += mStormFogData.volume[x].speed;
        if ((mStormFogData.volume[x].speed < 0.0f && mFogVolumes[x].percentage <= mStormFogData.volume[x].endPercentage) ||
            (mStormFogData.volume[x].speed > 0.0f && mFogVolumes[x].percentage >= mStormFogData.volume[x].endPercentage))
        {
            mFogVolumes[x].percentage = mStormFogData.volume[x].endPercentage;
            F32 save = mStormFogData.volume[x].lastPercentage;
            mStormFogData.volume[x].lastPercentage = mStormFogData.volume[x].endPercentage;
            mStormFogData.volume[x].endPercentage = save;

            mStormFogData.volume[x].speed *= -1;
        }
    }
    mSceneManager->setFogVolumes(mNumFogVolumes, mFogVolumes);
}

//---------------------------------------------------------------------------
// Load vertex buffer points
//---------------------------------------------------------------------------
void Sky::loadVBPoints()
{
    mSkyVB.set(GFX, 24, GFXBufferTypeStatic);
    mSkyVB.lock();

    Point3F points[8];

#define fillPoints( PointIndex, x, y, z ){\
   points[PointIndex].set( x, y, z ); }

#define fillVerts( PointIndex, SkyIndex, TU, TV ){\
   dMemcpy( &mSkyVB[SkyIndex], points[PointIndex], sizeof(Point3F) );\
   mSkyVB[SkyIndex].color.set( 255, 255, 255 );\
   mSkyVB[SkyIndex].texCoord.x = TU;\
   mSkyVB[SkyIndex].texCoord.y = TV;}

    fillPoints(0, -1.0, -1.0, 1.0);
    fillPoints(1, 1.0, -1.0, 1.0);
    fillPoints(2, 1.0, 1.0, 1.0);
    fillPoints(3, -1.0, 1.0, 1.0);
    fillPoints(4, -1.0, -1.0, -1.0);
    fillPoints(5, 1.0, -1.0, -1.0);
    fillPoints(6, 1.0, 1.0, -1.0);
    fillPoints(7, -1.0, 1.0, -1.0);


    fillVerts(0, 0, 0.0, 1.0);
    fillVerts(1, 1, 1.0, 1.0);
    fillVerts(2, 2, 1.0, 0.0);
    fillVerts(3, 3, 0.0, 0.0);

    fillVerts(4, 4, 0.0, 0.0);
    fillVerts(5, 5, 1.0, 0.0);
    fillVerts(6, 6, 1.0, 1.0);
    fillVerts(7, 7, 0.0, 1.0);

    fillVerts(0, 8, 0.0, 0.0);
    fillVerts(1, 9, 1.0, 0.0);
    fillVerts(5, 10, 1.0, 1.0);
    fillVerts(4, 11, 0.0, 1.0);

    fillVerts(2, 12, 0.0, 0.0);
    fillVerts(3, 13, 1.0, 0.0);
    fillVerts(7, 14, 1.0, 1.0);
    fillVerts(6, 15, 0.0, 1.0);

    fillVerts(3, 16, 0.0, 0.0);
    fillVerts(0, 17, 1.0, 0.0);
    fillVerts(4, 18, 1.0, 1.0);
    fillVerts(7, 19, 0.0, 1.0);

    fillVerts(1, 20, 0.0, 0.0);
    fillVerts(2, 21, 1.0, 0.0);
    fillVerts(6, 22, 1.0, 1.0);
    fillVerts(5, 23, 0.0, 1.0);

    mSkyVB.unlock();
}

//---------------------------------------------------------------------------
void Sky::calcPoints()
{
    S32 x, y, xval = 1, yval = -1, zval = 1;
    F32 textureDim;

    F32 visDisMod = mVisibleDistance;
    if (getCurrentClientSceneGraph())
        visDisMod = getCurrentClientSceneGraph()->getVisibleDistanceMod();
    mRadius = visDisMod * 0.20f;

    Cloud::setRadius(mRadius);

    Point3F tpt(1, 1, 1);
    tpt.normalize(mRadius);

    mPoints[0] = mPoints[4] = Point3F(-tpt.x, -tpt.y, tpt.z);
    mPoints[5] = mPoints[9] = Point3F(-tpt.x, -tpt.y, -tpt.z);

    for (x = 1; x < 4; ++x)
    {
        mPoints[x] = Point3F(tpt.x * xval, tpt.y * yval, tpt.z);
        mPoints[x + 5] = Point3F(tpt.x * xval, tpt.y * yval, -tpt.z);

        if (yval > 0 && xval > 0)
            xval *= -1;
        if (yval < 0)
            yval *= -1;
    }

    mFogLine = 0.0f;
    for (x = 0; x < mNumFogVolumes; ++x)
        mFogLine = (mFogVolumes[x].maxHeight > mFogLine) ? mFogVolumes[x].maxHeight : mFogLine;

    textureDim = 512;
    if (mSkyHandle[0])
        textureDim = mSkyHandle[0].getWidth();

    mTexCoord[0].set(0.f, 0.f);
    mTexCoord[1].set(1.f, 0.f);
    mTexCoord[2].set(1.f, 1.f);
    mTexCoord[3].set(0.f, 1.f);

    for (U32 i = 0; i < 4; i++)
    {
        mTexCoord[i] *= (textureDim - 1.0f) / textureDim;
        mTexCoord[i] += Point2F(0.5 / textureDim, 0.5 / textureDim);
    }

    mSpherePt = mSkyBoxPt = mPoints[1];
    mSpherePt.set(mSpherePt.x, 0.0f, mSpherePt.z);
    mSpherePt.normalize(mSkyBoxPt.x);
    mTopCenterPt.set(0.0f, 0.0f, mSkyBoxPt.z);
}

//---------------------------------------------------------------------------
bool Sky::loadDml()
{

    char path[1024], * p;
    dStrcpy(path, mMaterialListName);
    if ((p = dStrrchr(path, '/')) != NULL)
        *p = 0;
    mNumCloudLayers = 0;
    Stream* stream = ResourceManager->openStream(mMaterialListName);
    if (stream == NULL)
    {
//#if defined(TORQUE_DEBUG)
        Con::warnf("Sky material list is missing: %s", mMaterialListName);
//#else
//        // ASSERT?? !!!!!!TBD
//#endif
        return false;
    }
    else
    {// !!!!!TBD dhc - there's no fricking error checking here or in materialList.read!!!!
        if(!mMaterialList.read(*stream))
        {
            Con::warnf("Sky material list is corrupt: %s", mMaterialListName);
            ResourceManager->closeStream(stream);
            return false;
        }
        ResourceManager->closeStream(stream);
        if (!mMaterialList.load(SkyTexture, path))
        {
            Con::warnf("Failed to load textures for sky material list: %s", mMaterialListName);
            //return false;
        }

        for (S32 x = 0; x < 6; ++x)
            mSkyHandle[x] = mMaterialList.getMaterial(x);

        //for (S32 x = 0; x < mMaterialList.size() - CloudMaterialOffset; ++x, ++mNumCloudLayers)
        //    mCloudLayer[x].setTexture(mMaterialList.getMaterial(x + CloudMaterialOffset));

    }

    return true;
}

//---------------------------------------------------------------------------
void Sky::updateVisibility()
{
    setVisibility();
    setMaskBits(VisibilityMask);
}

//---------------------------------------------------------------------------
void Sky::stormCloudsOn(S32 state, F32 time)
{
    mStormCloudData.state = (state) ? comingIn : goingOut;
    mStormCloudData.time = time;

    setMaskBits(StormCloudMask);
}

//---------------------------------------------------------------------------
void Sky::stormFogOn(F32 percentage, F32 time)
{
    mStormFogData.time = time;

    if (mStormFogData.endPercentage >= 0.0f)
    {
        mStormFogData.state = (mStormFogData.endPercentage > percentage) ? goingOut : comingIn;
        mFogPercentage = mStormFogData.endPercentage;
    }
    else
        mStormFogData.state = (mFogPercentage > percentage) ? goingOut : comingIn;
    mStormFogData.endPercentage = percentage;

    setMaskBits(StormFogMask);
}

//---------------------------------------------------------------------------
void Sky::stormRealFog(S32 value, F32 max, F32 min, F32 speed)
{
    mRealFog = value;
    mRealFogMax = max;
    mRealFogMin = min;
    mRealFogSpeed = speed;
    setMaskBits(StormRealFogMask);
}

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
// Cloud Code
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
Cloud::Cloud()
{
    mDown = 5;
    mOver = 1;
    mBaseOffset.set(0, 0);
    mTextureScale.set(1, 1);
    mCenterHeight = 0.5;
    mInnerHeight = 0.45;
    mEdgeHeight = 0.4;
    mLastTime = 0;
    mOffset = 0;
    mSpeed.set(1, 1);
    mGStormData.currentCloud = MAX_NUM_LAYERS;
    mGStormData.fadeSpeed = 0.0f;
    mGStormData.StormOn = false;
    mGStormData.stormState = isDone;
    for (int i = 0; i < 25; ++i)
        stormAlpha[i] = 1.0f;
    mRadius = 1.0f;
}

//---------------------------------------------------------------------------
Cloud::~Cloud()
{
}

//---------------------------------------------------------------------------
void Cloud::updateCoord()
{
    mBaseOffset += mSpeed * mOffset;
    if (mSpeed.x < 0)
        mBaseOffset.x -= mCeil(mBaseOffset.x);
    else
        mBaseOffset.x -= mFloor(mBaseOffset.x);
    if (mSpeed.y < 0)
        mBaseOffset.y -= mCeil(mBaseOffset.y);
    else
        mBaseOffset.y -= mFloor(mBaseOffset.y);
}

//---------------------------------------------------------------------------
void Cloud::setHeights(F32 cHeight, F32 iHeight, F32 eHeight)
{
    mCenterHeight = cHeight;
    mInnerHeight = iHeight;
    mEdgeHeight = eHeight;
}

//---------------------------------------------------------------------------

void Cloud::setTexture(GFXTexHandle textHand)
{
    mCloudHandle = textHand;
}

//---------------------------------------------------------------------------
void Cloud::setSpeed(Point2F setSpeed)
{
    mSpeed = setSpeed;
}

//---------------------------------------------------------------------------
void Cloud::setPoints()
{
    S32 x, y;
    F32 xyDiff = mRadius / 2;
    F32 cDis = mRadius * mCenterHeight,
        upDis = mRadius * mInnerHeight,
        edgeZ = mRadius * mEdgeHeight;

    // We're dealing with a hemisphere so calculate some heights.
    F32 zValue[25] = {
                         edgeZ,   edgeZ,   edgeZ,   edgeZ,   edgeZ,
                         edgeZ,   upDis,   upDis,   upDis,   edgeZ,
                         edgeZ,   upDis,   cDis,    upDis,   edgeZ,
                         edgeZ,   upDis,   upDis,   upDis,   edgeZ,
                         edgeZ,   edgeZ,   edgeZ,   edgeZ,   edgeZ
    };

    for (y = 0; y < 5; ++y)
        for (x = 0; x < 5; ++x)
            mPoints[y * 5 + x].set(-mRadius + (xyDiff * x), mRadius - (xyDiff * y), zValue[y * 5 + x]);

    // 0, 4, 20, 24 are the four corners of the grid...
    // the goal here is to make the cloud layer more "spherical"?

    /*Point3F vec = (mPoints[5]  + ((mPoints[1]  - mPoints[5])  * 0.5f)) - mPoints[6];
    mPoints[0] =   mPoints[6]  + (vec * 2.0f);

    vec =         (mPoints[9]  + ((mPoints[3]  - mPoints[9])  * 0.5f)) - mPoints[8];
    mPoints[4] =   mPoints[8]  + (vec * 2.0f);

    vec =         (mPoints[21] + ((mPoints[15] - mPoints[21]) * 0.5f)) - mPoints[16];
    mPoints[20] =  mPoints[16] + (vec * 2.0f);

    vec =         (mPoints[23] + ((mPoints[19] - mPoints[23]) * 0.5f)) - mPoints[18];
    mPoints[24] =  mPoints[18] + (vec * 2.0f); */

    calcAlpha();
}

//---------------------------------------------------------------------------
void Cloud::calcAlpha()
{
    for (S32 i = 0; i < 25; ++i)
    {
        mAlpha[i] = 1.3f - ((mPoints[i] - Point3F(0, 0, mPoints[i].z)).len()) / mRadius;
        if (mAlpha[i] < 0.4f)
            mAlpha[i] = 0.0f;
        else if (mAlpha[i] > 0.8f)
            mAlpha[i] = 1.0f;
    }
}

//---------------------------------------------------------------------------
void Cloud::render(U32 currentTime, U32 cloudLayer, bool outlineOn, S32 numLayers, PlaneF* planes)
{
    if (cloudLayer != Con::getIntVariable("onlyCloudLayer", 2))
        return;

    mGStormData.numCloudLayers = numLayers;
    mOffset = 1.0f;
    U32 numPoints;
    Point3F renderPoints[128];
    Point2F renderTexPoints[128];
    F32 renderAlpha[128];
    F32 renderSAlpha[128];

    if (mLastTime != 0)
        mOffset = (currentTime - mLastTime) / 32.0f;
    mLastTime = currentTime;

    if (!mCloudHandle || (mGStormData.StormOn && mGStormData.currentCloud < cloudLayer))
        return;

    S32 start = 0, i, j, k;
    updateCoord();
    for (S32 x = 0; x < 5; x++)
        for (S32 y = 0; y < 5; y++)
            mTexCoords[y * 5 + x].set(x * mTextureScale.x + mBaseOffset.x,
                y * mTextureScale.y + mBaseOffset.y);

    if (mGStormData.StormOn && mGStormData.currentCloud == cloudLayer)
        updateStorm();

    GFX->setBaseRenderState();

    if (true || !outlineOn)
    {
        GFX->setTexture(0, mCloudHandle);
        GFX->setTextureStageColorOp(0, GFXTOPModulate);
        GFX->setTextureStageColorOp(1, GFXTOPDisable);
        GFX->setAlphaBlendEnable(true);
        GFX->setSrcBlend(GFXBlendSrcAlpha);
        GFX->setDestBlend(GFXBlendInvSrcAlpha);
        GFX->setZEnable(false);
        GFX->setZWriteEnable(false);

        GFX->setTextureStageAddressModeU(0, GFXAddressWrap);
        GFX->setTextureStageAddressModeV(0, GFXAddressWrap);
    }

    for (i = 0; i < 4; ++i)
    {
        start = i * 5;
        for (j = 0; j < 4; ++j)
        {
            numPoints = 4;
            setRenderPoints(renderPoints, renderTexPoints, renderAlpha, renderSAlpha, start);

            for (S32 i = 0; i < 4; ++i)
                clipToPlane(renderPoints, renderTexPoints, renderAlpha, renderSAlpha,
                    numPoints, planes[i]);

            if (numPoints)
            {
                GFX->setupGenericShaders(GFXDevice::GSModColorTexture);
                PrimBuild::begin(GFXTriangleFan, numPoints);

                for (k = 0; k < numPoints; ++k)
                {
                    PrimBuild::color4f(1.0, 1.0, 1.0, renderAlpha[k] * renderSAlpha[k]);

                    PrimBuild::texCoord2f(renderTexPoints[k].x, renderTexPoints[k].y);
                    PrimBuild::vertex3f(renderPoints[k].x, renderPoints[k].y, renderPoints[k].z);
                }

                PrimBuild::end();
            }

            ++start;
        }
    }
    GFX->setAlphaBlendEnable(false);
    GFX->setZEnable(true);
    GFX->setZWriteEnable(true);

}

void Cloud::setRenderPoints(Point3F* renderPoints, Point2F* renderTexPoints,
    F32* renderAlpha, F32* renderSAlpha, S32 index)
{
    S32 offset[4] = { 0,5,6,1 };
    for (S32 x = 0; x < 4; ++x)
    {
        renderPoints[x].set(
            mPoints[index + offset[x]].x,
            mPoints[index + offset[x]].y,
            mPoints[index + offset[x]].z
        );
        renderTexPoints[x].set(
            mTexCoords[index + offset[x]].x,
            mTexCoords[index + offset[x]].y
        );

        renderAlpha[x] = mAlpha[index + offset[x]];
        renderSAlpha[x] = stormAlpha[index + offset[x]];
    }
}


//---------------------------------------------------------------------------
void Cloud::setTextPer(F32 cloudTextPer)
{
    mTextureScale.set(cloudTextPer / 4.0, cloudTextPer / 4.0);
}

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//    Storm Code
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
void Cloud::updateStorm()
{
    if (!mGStormData.FadeOut && !mGStormData.FadeIn) {
        alphaCenter += (stormUpdate * mOffset);
        F32 update, center;
        if (mGStormData.stormDir == 'x') {
            update = stormUpdate.x;
            center = alphaCenter.x;
        }
        else {
            update = stormUpdate.y;
            center = alphaCenter.y;
        }

        if (mGStormData.stormState == comingIn) {
            if ((update > 0 && center > 0) || (update < 0 && center < 0))
                mGStormData.FadeIn = true;
        }
        else
            if ((update > 0 && center > mRadius * 2) || (update < 0 && center < -mRadius * 2)) {
                //            Con::printf("Cloud %d is done.", mGStormData.currentCloud);
                mGStormData.StormOn = --mGStormData.currentCloud >= 0;
                if (mGStormData.StormOn) {
                    mGStormData.FadeOut = true;
                    return;
                }
            }
    }
    calcStormAlpha();
}

//---------------------------------------------------------------------------
void Cloud::calcStormAlpha()
{
    if (mGStormData.FadeIn)
    {
        bool done = true;
        for (int i = 0; i < 25; ++i)
        {
            stormAlpha[i] += (mGStormData.fadeSpeed * mOffset);
            if (stormAlpha[i] >= 1.0f)
                stormAlpha[i] = 1.0f;
            else
                done = false;
        }
        if (done)
        {
            //         Con::printf("Cloud %d is done.", mGStormData.currentCloud);
            mGStormData.StormOn = ++mGStormData.currentCloud < mGStormData.numCloudLayers;
            mGStormData.FadeIn = false;
        }
    }
    else if (mGStormData.FadeOut)
    {
        bool done = true;
        for (int i = 0; i < 25; ++i)
        {
            stormAlpha[i] -= (mGStormData.fadeSpeed * mOffset);
            if (stormAlpha[i] <= mAlphaSave[i])
                stormAlpha[i] = mAlphaSave[i];
            else
                done = false;
        }
        if (done)
            mGStormData.FadeOut = false;
    }
    else
        for (int i = 0; i < 25; ++i)
        {
            stormAlpha[i] = 1.0f - ((Point3F(mPoints[i].x - alphaCenter.x, mPoints[i].y - alphaCenter.y, mPoints[i].z).len()) / mRadius);
            if (stormAlpha[i] < 0.0f)
                stormAlpha[i] = 0.0f;
            else if (stormAlpha[i] > 1.0f)
                stormAlpha[i] = 1.0f;
        }
}

//---------------------------------------------------------------------------
void Cloud::calcStorm(F32 speed, F32 fadeSpeed)
{
    float tempX, tempY;
    float windSlop = 0.0f;

    if (mSpeed.x != 0)
        windSlop = mSpeed.y / mSpeed.x;

    tempX = (mSpeed.x < 0) ? -mSpeed.x : mSpeed.x;
    tempY = (mSpeed.y < 0) ? -mSpeed.y : mSpeed.y;

    if (tempX >= tempY)
    {
        alphaCenter.x = (mSpeed.x < 0) ? mRadius * -2 : mRadius * 2;
        alphaCenter.y = windSlop * alphaCenter.x;

        stormUpdate.x = alphaCenter.x > 0.0f ? -speed : speed;
        stormUpdate.y = alphaCenter.y > 0.0f ? -speed * windSlop : speed * windSlop;
        mGStormData.stormDir = 'x';
    }
    else
    {
        alphaCenter.y = (mSpeed.y < 0) ? mRadius * 2 : mRadius * -2;
        alphaCenter.x = windSlop * alphaCenter.y;

        /*      if(windSlop != 0)
                 alphaCenter.x = (1/windSlop)*alphaCenter.y;
              else
                 alphaCenter.x = 0.0f;
        */
        stormUpdate.y = alphaCenter.y > 0.0f ? -speed : speed;
        stormUpdate.x = alphaCenter.x > 0.0f ? -speed * (1 / windSlop) : speed * (1 / windSlop);

        mGStormData.stormDir = 'y';
    }

    mGStormData.fadeSpeed = fadeSpeed;

    for (int i = 0; i < 25; ++i)
    {
        mAlphaSave[i] = 1.0f - (mPoints[i].len() / mRadius);
        if (mAlphaSave[i] < 0.0f)
            mAlphaSave[i] = 0.0f;
        else if (mAlphaSave[i] > 1.0f)
            mAlphaSave[i] = 1.0f;
    }
    if (mGStormData.stormState == goingOut)
        alphaCenter.set(0.0f, 0.0f);
}

//---------------------------------------------------------------------------
void Cloud::startStorm(SkyState state)
{
    mGStormData.StormOn = true;
    mGStormData.stormState = state;
    if (state == goingOut)
    {
        mGStormData.FadeOut = true;
        mGStormData.FadeIn = false;
        mGStormData.currentCloud = mGStormData.numCloudLayers - 1;
    }
    else
    {
        mGStormData.FadeIn = false;
        mGStormData.FadeOut = false;
        mGStormData.currentCloud = 0;
    }
}

void Cloud::clipToPlane(Point3F* points, Point2F* texPoints, F32* alphaPoints,
    F32* sAlphaPoints, U32& rNumPoints, const PlaneF& rPlane)
{
    S32 start = -1;
    for (U32 i = 0; i < rNumPoints; i++)
    {
        if (rPlane.whichSide(points[i]) == PlaneF::Front)
        {
            start = i;
            break;
        }
    }

    // Nothing was in front of the plane...
    if (start == -1)
    {
        rNumPoints = 0;
        return;
    }

    U32     numFinalPoints = 0;
    Point3F finalPoints[128];
    Point2F finalTexPoints[128];
    F32     finalAlpha[128];
    F32     finalSAlpha[128];

    U32 baseStart = start;
    U32 end = (start + 1) % rNumPoints;

    while (end != baseStart)
    {
        const Point3F& rStartPoint = points[start];
        const Point3F& rEndPoint = points[end];

        const Point2F& rStartTexPoint = texPoints[start];
        const Point2F& rEndTexPoint = texPoints[end];

        PlaneF::Side fSide = rPlane.whichSide(rStartPoint);
        PlaneF::Side eSide = rPlane.whichSide(rEndPoint);

        S32 code = fSide * 3 + eSide;
        switch (code) {
        case 4:   // f f
        case 3:   // f o
        case 1:   // o f
        case 0:   // o o
         // No Clipping required

         //Alpha
            finalAlpha[numFinalPoints] = alphaPoints[start];
            finalSAlpha[numFinalPoints] = sAlphaPoints[start];

            //Points
            finalPoints[numFinalPoints] = points[start];
            finalTexPoints[numFinalPoints++] = texPoints[start];

            start = end;
            end = (end + 1) % rNumPoints;
            break;


        case 2: { // f b
            // In this case, we emit the front point, Insert the intersection,
            //  and advancing to point to first point that is in front or on...

            //Alpha
            finalAlpha[numFinalPoints] = alphaPoints[start];
            finalSAlpha[numFinalPoints] = sAlphaPoints[start];

            //Points
            finalPoints[numFinalPoints] = points[start];
            finalTexPoints[numFinalPoints++] = texPoints[start];

            Point3F vector = rEndPoint - rStartPoint;
            F32 t = -(rPlane.distToPlane(rStartPoint) / mDot(rPlane, vector));

            //Alpha
            finalAlpha[numFinalPoints] = alphaPoints[start] + ((alphaPoints[end] - alphaPoints[start]) * t);
            finalSAlpha[numFinalPoints] = sAlphaPoints[start] + ((sAlphaPoints[end] - sAlphaPoints[start]) * t);

            //Polygon Points
            Point3F intersection = rStartPoint + (vector * t);
            finalPoints[numFinalPoints] = intersection;

            //Texture Points
            Point2F texVec = rEndTexPoint - rStartTexPoint;

            Point2F texIntersection = rStartTexPoint + (texVec * t);
            finalTexPoints[numFinalPoints++] = texIntersection;

            U32 endSeek = (end + 1) % rNumPoints;
            while (rPlane.whichSide(points[endSeek]) == PlaneF::Back)
                endSeek = (endSeek + 1) % rNumPoints;

            end = endSeek;
            start = (end + (rNumPoints - 1)) % rNumPoints;

            const Point3F& rNewStartPoint = points[start];
            const Point3F& rNewEndPoint = points[end];

            const Point2F& rNewStartTexPoint = texPoints[start];
            const Point2F& rNewEndTexPoint = texPoints[end];

            vector = rNewEndPoint - rNewStartPoint;
            t = -(rPlane.distToPlane(rNewStartPoint) / mDot(rPlane, vector));

            //Alpha
            alphaPoints[start] = alphaPoints[start] + ((alphaPoints[end] - alphaPoints[start]) * t);
            sAlphaPoints[start] = sAlphaPoints[start] + ((sAlphaPoints[end] - sAlphaPoints[start]) * t);

            //Polygon Points
            intersection = rNewStartPoint + (vector * t);
            points[start] = intersection;

            //Texture Points
            texVec = rNewEndTexPoint - rNewStartTexPoint;

            texIntersection = rNewStartTexPoint + (texVec * t);
            texPoints[start] = texIntersection;
        }
              break;

        case -1: {// o b
            // In this case, we emit the front point, and advance to point to first
            //  point that is in front or on...
            //

            //Alpha
            finalAlpha[numFinalPoints] = alphaPoints[start];
            finalSAlpha[numFinalPoints] = sAlphaPoints[start];

            //Points
            finalPoints[numFinalPoints] = points[start];
            finalTexPoints[numFinalPoints++] = texPoints[start];

            U32 endSeek = (end + 1) % rNumPoints;
            while (rPlane.whichSide(points[endSeek]) == PlaneF::Back)
                endSeek = (endSeek + 1) % rNumPoints;

            end = endSeek;
            start = (end + (rNumPoints - 1)) % rNumPoints;

            const Point3F& rNewStartPoint = points[start];
            const Point3F& rNewEndPoint = points[end];

            const Point2F& rNewStartTexPoint = texPoints[start];
            const Point2F& rNewEndTexPoint = texPoints[end];

            Point3F vector = rNewEndPoint - rNewStartPoint;
            F32 t = -(rPlane.distToPlane(rNewStartPoint) / mDot(rPlane, vector));

            //Alpha
            alphaPoints[start] = alphaPoints[start] + ((alphaPoints[end] - alphaPoints[start]) * t);
            sAlphaPoints[start] = sAlphaPoints[start] + ((sAlphaPoints[end] - sAlphaPoints[start]) * t);

            //Polygon Points
            Point3F intersection = rNewStartPoint + (vector * t);
            points[start] = intersection;

            //Texture Points
            Point2F texVec = rNewEndTexPoint - rNewStartTexPoint;

            Point2F texIntersection = rNewStartTexPoint + (texVec * t);
            texPoints[start] = texIntersection;
        }
               break;

        case -2:  // b f
        case -3:  // b o
        case -4:  // b b
         // In the algorithm used here, this should never happen...
            AssertISV(false, "SGUtil::clipToPlane: error in polygon clipper");
            break;

        default:
            AssertFatal(false, "SGUtil::clipToPlane: bad outcode");
            break;
        }
    }

    // Emit the last point.

    //Alpha
    finalAlpha[numFinalPoints] = alphaPoints[start];
    finalSAlpha[numFinalPoints] = sAlphaPoints[start];

    //Points
    finalPoints[numFinalPoints] = points[start];
    finalTexPoints[numFinalPoints++] = texPoints[start];
    AssertFatal(numFinalPoints >= 3, avar("Error, this shouldn't happen!  Invalid winding in clipToPlane: %d", numFinalPoints));

    // Copy the new rWinding, and we're set!

    //Alpha
    dMemcpy(alphaPoints, finalAlpha, numFinalPoints * sizeof(F32));
    dMemcpy(sAlphaPoints, finalSAlpha, numFinalPoints * sizeof(F32));

    //Points
    dMemcpy(points, finalPoints, numFinalPoints * sizeof(Point3F));
    dMemcpy(texPoints, finalTexPoints, numFinalPoints * sizeof(Point2F));

    rNumPoints = numFinalPoints;
    AssertISV(rNumPoints <= 128, "MaxWindingPoints exceeded in scenegraph.  Fatal error.");
}
