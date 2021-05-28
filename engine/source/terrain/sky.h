//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _SKY_H_
#define _SKY_H_

#ifndef _MMATH_H_
#include "math/mMath.h"
#endif
#ifndef _SCENEOBJECT_H_
#include "sim/sceneObject.h"
#endif
#ifndef _SCENESTATE_H_
#include "sceneGraph/sceneState.h"
#endif
#ifndef _SCENEGRAPH_H_
#include "sceneGraph/sceneGraph.h"
#endif
#ifndef _MPOINT_H_
#include "math/mPoint.h"
#endif
#ifndef _MATERIALLIST_H_
#include "materials/materialList.h"
#endif
#ifndef _GAMEBASE_H_
#include "game/gameBase.h"
#endif

#include "gfx/gfxDevice.h"

#define MAX_NUM_LAYERS 3
#define MAX_BAN_POINTS 20

class SceneGraph;
class SceneState;

enum SkyState
{
    isDone = 0,
    comingIn = 1,
    goingOut = 2
};

typedef struct
{
    bool StormOn;
    bool FadeIn;
    bool FadeOut;
    S32 currentCloud;
    F32 stormSpeed;
    F32 stormDir;
    S32 numCloudLayers;
    F32 fadeSpeed;
    SkyState stormState;
}StormInfo;

typedef struct
{
    SkyState state;
    F32 speed;
    F32 time;
    F32 fadeSpeed;
}StormCloudData;

typedef struct
{
    bool active;
    SkyState state;
    F32 speed;
    F32 endPercentage;
    F32 lastPercentage;
}StormFogVolume;

typedef struct
{
    SkyState state;
    U32 startTime;
    F32 endPercentage;
    F32 time;
    S32 current;
    U32 lastTime;
    StormFogVolume volume[MaxFogVolumes];
}StormFogData;

//---------------------------------------------------------------------------
class Cloud
{
private:
    Point3F mPoints[25];
    Point2F mSpeed;
    F32 mCenterHeight, mInnerHeight, mEdgeHeight;
    F32 mAlpha[25];
    S32 mDown, mOver;
    static F32 mRadius;
    F32 mLastTime, mOffset;
    Point2F mBaseOffset, mTexCoords[25], mTextureScale;
    GFXTexHandle mCloudHandle;

    Point2F alphaCenter;
    Point2F stormUpdate;
    F32 stormAlpha[25];
    F32 mAlphaSave[25];

    static StormInfo mGStormData;
public:
    Cloud();
    ~Cloud();
    void setPoints();
    void setHeights(F32 cHeight, F32 iHeight, F32 eHeight);
    void setTexture(GFXTexHandle);
    void setSpeed(Point2F);
    void setTextPer(F32 cloudTextPer);
    void updateCoord();
    void calcAlpha();
    void render(U32, U32, bool, S32, PlaneF*);
    void updateStorm();
    void calcStorm(F32 speed, F32 fadeSpeed);
    void calcStormAlpha();
    static void startStorm(SkyState);
    static void setRadius(F32 rad) { mRadius = rad; }
    void setRenderPoints(Point3F* renderPoints, Point2F* renderTexPoints, F32* renderAlpha, F32* renderSAlpha, S32 index);
    void clipToPlane(Point3F* points, Point2F* texPoints, F32* alphaPoints, F32* sAlphaPoints, U32& rNumPoints, const PlaneF& rPlane);
};

//--------------------------------------------------------------------------
class Sky : public SceneObject
{
    typedef SceneObject Parent;
private:

    StormCloudData mStormCloudData;
    StormFogData mStormFogData;
    GFXTexHandle mSkyHandle[6];
    StringTableEntry mCloudText[MAX_NUM_LAYERS];
    F32 mCloudHeight[MAX_NUM_LAYERS];
    F32 mCloudSpeed[MAX_NUM_LAYERS];
    Cloud mCloudLayer[MAX_NUM_LAYERS];
    F32 mRadius;
    Point3F mPoints[10];
    Point2F mTexCoord[4];
    StringTableEntry mMaterialListName;

    Point3F mSkyBoxPt;
    Point3F mTopCenterPt;
    Point3F mSpherePt;
    ColorI mRealFogColor;
    ColorI mRealSkyColor;

    MaterialList mMaterialList;
    ColorF mFogColor;
    bool mSkyTexturesOn;
    bool mRenderBoxBottom;
    ColorF mSolidFillColor;
    F32 mFogDistance;
    F32 mVisibleDistance;
    U32 mNumFogVolumes;
    FogVolume mFogVolumes[MaxFogVolumes];
    F32 mFogLine;
    F32 mFogTime;
    F32 mFogPercentage;
    S32 mFogVolume;
    S32 mRealFog;
    F32 mRealFogMax;
    F32 mRealFogMin;
    F32 mRealFogSpeed;
    bool mNoRenderBans;
    F32 mBanOffsetHeight;

    bool mLastForce16Bit;
    bool mLastForcePaletted;

    SkyState mFogState;

    S32 mNumCloudLayers;
    Point3F mWindVelocity;

    F32 mLastVisDisMod;

    GFXVertexBufferHandle<GFXVertexPCT> mSkyVB;

    static bool smCloudsOn;
    static bool smCloudOutlineOn;
    static bool smSkyOn;
    static S32  smNumCloudsOn;

    bool mStormCloudsOn;
    bool mStormFogOn;
    bool mSetFog;

    bool mSkyGlow;
    ColorF mSkyGlowColor;

    void calcPoints();
    void loadVBPoints();

protected:
    bool onAdd();
    void onRemove();

    bool prepRenderImage(SceneState* state, const U32 stateKey, const U32 startZone, const bool modifyBaseZoneState = false);
    void renderObject(SceneState* state, RenderInst* ri);
    void render(SceneState* state);
    void calcAlphas_Heights(F32 zCamPos, F32* banHeights, F32* alphaBan, F32 DepthInFog);
    void renderSkyBox(F32 lowerBanHeight, F32 alphaIn);
    void calcBans(F32* banHeights, Point3F banPoints[][MAX_BAN_POINTS], Point3F* cornerPoints);
    void renderBans(F32* alphaBan, F32* banHeights, Point3F banPoints[][MAX_BAN_POINTS], Point3F* cornerPoints);
    void inspectPostApply();
    void startStorm();
    void setVisibility();
    void initSkyData();
    bool loadDml();
    void updateFog();
    void updateRealFog();
    void startStormFog();
    void setRenderPoints(Point3F* renderPoints, S32 index);
    void calcTexCoords(Point2F* texCoords, Point3F* renderPoints, S32 index, F32 lowerBanHeight);
public:
    Point2F mWindDir;
    enum NetMaskBits {
        InitMask = BIT(0),
        VisibilityMask = BIT(1),
        StormCloudMask = BIT(2),
        StormFogMask = BIT(3),
        StormRealFogMask = BIT(4),
        WindMask = BIT(5),
        StormCloudsOnMask = BIT(6),
        StormFogOnMask = BIT(7),
        SkyGlowMask = BIT(8),
        MaterialMask = BIT(9)
    };
    enum Constants {
        EnvMapMaterialOffset = 6,
        CloudMaterialOffset = 7
    };

    Sky();
    ~Sky();

    F32 getVisibleDistance() const { return mVisibleDistance; }

    /// @name Storm management.
    /// @{
    void stormCloudsShow(bool);
    void stormFogShow(bool);
    void stormCloudsOn(S32 state, F32 time);
    void stormFogOn(F32 percentage, F32 time);
    void stormRealFog(S32 value, F32 max, F32 min, F32 speed);
    /// @}

    /// @name Wind velocity
    /// @{

    void setWindVelocity(const Point3F&);
    Point3F getWindVelocity();
    /// @}

    /// @name Environment mapping
    /// @{

 //   TextureHandle getEnvironmentMap() { return mMaterialList.getMaterial(EnvMapMaterialOffset); }
    /// @}

    /// Torque infrastructure
    DECLARE_CONOBJECT(Sky);
    static void initPersistFields();
    static void consoleInit();

    U32  packUpdate(NetConnection* conn, U32 mask, BitStream* stream);
    void unpackUpdate(NetConnection* conn, BitStream* stream);

    void updateVisibility();

    void setSkyMaterial(const char* skyMaterial);
};




#endif
