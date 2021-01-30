//-----------------------------------------------------------------------------
// Torque Shader Engine - fxLight
// 
// Written by Melvyn May, 4th May 2002.
//-----------------------------------------------------------------------------

#ifndef _FXLIGHTDB_H_
#define _FXLIGHTDB_H_

#ifndef _GAMEBASE_H_
#include "game/gameBase.h"
#endif
#ifndef _SCENEOBJECT_H_
#include "sim/sceneObject.h"
#endif

#define FXLIGHTDBICONTEXTURE		"common/lighting/fxlighticon.png"


class fxLightData : public GameBaseData
{
public:
    typedef GameBaseData Parent;

    fxLightData();

    bool onAdd();
    static void  initPersistFields();
    virtual void packData(BitStream* stream);
    virtual void unpackData(BitStream* stream);

    // Datablock Flare.
    StringTableEntry	mFlareTextureName;				// Flare Texture Name.

    // Datablock Light.
    bool				mLightOn;						// Light On Flag.
    F32					mRadius;						// Radius.
    F32					mBrightness;					// Brightness.
    ColorF				mColour;						// Colour.

    // Datablock Flare.
    bool				mFlareOn;						// Flare On Flag.
    bool				mFlareTP;						// Flare Third Person Flag.
    ColorF				mFlareColour;					// Flare Colour.
    bool				mConstantSizeOn;				// Flare Use Constant Size Flag.
    F32					mConstantSize;					// Flare Constant Size.
    F32					mNearSize;						// Near Size.
    F32					mFarSize;						// Far Size.
    F32					mNearDistance;					// Near Distance.
    F32					mFarDistance;					// Far Distance.
    F32					mFadeTime;						// Fade Time.
    U32					mBlendMode;						// Blend Mode.
    bool				mLinkFlare;						// Link Flare Animation.
    bool				mLinkFlareSize;					// Link Flare Size Animation.

    // Datablock Animation.
    ColorF				mMinColour;						// Minimum Colour.
    ColorF				mMaxColour;						// Maximum Colour.
    F32					mMinBrightness;					// Minimum Brightness.
    F32					mMaxBrightness;					// Maximum Brightness.
    F32					mMinRadius;						// Minimum Radius.
    F32					mMaxRadius;						// Maximum Radius.
    Point3F				mStartOffset;					// Start Offset.
    Point3F				mEndOffset;						// Stop Offset.
    F32					mMinRotation;					// Minimum Rotation.
    F32					mMaxRotation;					// Maximum Rotation.
    bool				mSingleColourKeys;				// Single-Channel Colour Keys.
    StringTableEntry	mRedKeys;						// Red Animation Keys.
    StringTableEntry	mGreenKeys;						// Green Animation Keys.
    StringTableEntry	mBlueKeys;						// Blue Animation Keys.
    StringTableEntry	mBrightnessKeys;				// Brightness Animation Keys.
    StringTableEntry	mRadiusKeys;					// Radius Animation Keys.
    StringTableEntry	mOffsetKeys;					// Offset Animation Keys.
    StringTableEntry	mRotationKeys;					// Rotation Animation Keys.
    F32					mColourTime;					// Colour Time (Seconds).
    F32					mBrightnessTime;				// Brightness Time.
    F32					mRadiusTime;					// Radius Time.
    F32					mOffsetTime;					// Offset Time.
    F32					mRotationTime;					// Rotation Time.
    bool				mLerpColour;					// Lerp Colour Flag.
    bool				mLerpBrightness;				// Lerp Brightness Flag.
    bool				mLerpRadius;					// Lerp Radius Flag.
    bool				mLerpOffset;					// Lerp Offset Flag.
    bool				mLerpRotation;					// Lerp Rotation Flag.
    bool				mUseColour;						// Use Colour Flag.
    bool				mUseBrightness;					// Use Brightness Flag.
    bool				mUseRadius;						// Use Radius Flag.
    bool				mUseOffsets;					// Use Position Offsets Flag.
    bool				mUseRotation;					// Use Rotation Flag.


    DECLARE_CONOBJECT(fxLightData);
};

//------------------------------------------------------------------------------
// Class: fxLight
//------------------------------------------------------------------------------
class fxLight : public GameBase
{
private:
    typedef GameBase		Parent;
    fxLightData* mDataBlock;
protected:

    U32 CheckKeySyntax(StringTableEntry Key);
    void CheckAnimationKeys(void);
    F32 GetLerpKey(StringTableEntry Key, U32 PosFrom, U32 PosTo, F32 ValueFrom, F32 ValueTo, F32 Lerp);
    void AnimateLight(void);
    void InitialiseAnimation(void);
    void ResetAnimation(void);
    bool TestLOS(const Point3F& ObjectPosition, SceneObject* AttachedObj);


protected:

    enum {
        fxLightConfigChangeMask = BIT(0),
        fxLightAttachChange = BIT(1)
    };

    bool						mAddedToScene;

    MRandomLCG					RandomGen;

    // Textures.
    GFXTexHandle				mIconTextureHandle;
    GFXTexHandle				mFlareTextureHandle;

    LightInfo					mLight;
    U32							mLastAnimateTime;
    U32							mLastRenderTime;
    F32							mFlareScale;
    bool						mAttached;
    //GameBase*					mpAttachedObject;
    //bool						mAttachWait;
    //bool						mAttachValid;

public:
    fxLight();
    ~fxLight();


    U32 mountPoint;
    MatrixF mountTransform;
    virtual void buildObjectBox()
    {
        Point3F Min, Max;
        Min.set(0, 0, 0);
        Max.set(0, 0, 0);
        // Calculate Extents.
        Min.setMin(mAnimationOffset);
        Max.setMax(mAnimationOffset);
        Min += Point3F(-0.5f, -0.5f, -0.5f);
        Max += Point3F(+0.5f, +0.5f, +0.5f);
        mObjBox.min.set(Min);
        mObjBox.max.set(Max);
        resetWorldBox();
    }
    virtual void calculateLightPosition()
    {
        // Yes, so set to attached position.
        //mAnimationPosition = mpAttachedObject->getPosition();

        // Set Current Position.
        //setPosition(mAnimationPosition);
    }
    virtual SceneObject* getAttachedObject() { return NULL; }


    // *********************************
    // Configuration Interface.
    // *********************************

    // Light.
    void setEnable(bool Status);
    void setFlareBitmap(const char* Name);

    // Misc,
    void reset(void);
    //void attachToObject(const char* ObjectName);
    //void detachFromObject(void);

    // GameBase.
    bool onNewDataBlock(GameBaseData* dptr);
    void processTick(const Move*);

    // SceneObject
    bool prepRenderImage(SceneState* state, const U32 stateKey, const U32 startZone, const bool modifyBaseZoneState);
    void renderObject(SceneState* state, RenderInst* ri);

    // SimObject      
    bool onAdd();
    void onRemove();
    //void onDeleteNotify(SimObject*);

    void inspectPostApply();
    void registerLights(LightManager* lightManager, bool lightingScene);

    // NetObject
    U32 packUpdate(NetConnection*, U32, BitStream*);
    void unpackUpdate(NetConnection*, BitStream*);

    // ConObject.
    static void initPersistFields();

    // Field Data.

    bool				mEnable;						// Light Enable.

    // Basis Light.
    F32					mIconSize;						// Icon Size.

    // Current Animation.
    ColorF				mAnimationColour;				// Current Colour.
    F32					mAnimationBrightness;			// Current Brightness.
    F32					mAnimationRadius;				// Current Radius.
    Point3F				mAnimationPosition;				// Current Position.
    Point3F				mAnimationOffset;				// Current Offset.
    F32					mAnimationRotation;				// Current Rotation.

    // Elapsed Times.
    F32					mColourElapsedTime;				// Colour Elapsed Time.
    F32					mBrightnessElapsedTime;			// Brightness Elapsed Time.
    F32					mRadiusElapsedTime;				// Radius Elapsed Time.
    F32					mOffsetElapsedTime;				// Offset Elapsed Time.
    F32					mRotationElapsedTime;			// Rotation Elapsed Time.

    // Time Scales.
    F32					mColourTimeScale;				// Colour Time Scale.
    F32					mBrightnessTimeScale;			// Brightness Time Scale.
    F32					mRadiusTimeScale;				// Radius Time Scale.
    F32					mOffsetTimeScale;				// Offset Time Scale.
    F32					mRotationTimeScale;				// Rotation Time Scale.

    // Key Lengths (Validity).
    U32					mRedKeysLength;					// Red Keys Length.
    U32					mGreenKeysLength;				// Green Keys Length.
    U32					mBlueKeysLength;				// Blue Keys Length.
    U32					mBrightnessKeysLength;			// Brightness Keys Length.
    U32					mRadiusKeysLength;				// Radius Keys Length.
    U32					mOffsetKeysLength;				// Offset Keys Length.
    U32					mRotationKeysLength;			// Rotation Keys Length.

    // Declare Console Object.
    DECLARE_CONOBJECT(fxLight);
};

#endif // _FXLIGHTDB_H_
