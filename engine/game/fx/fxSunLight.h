//-----------------------------------------------------------------------------
// Torque Shader Engine - fxLight
// 
// Written by Melvyn May, 27th August 2002.
//-----------------------------------------------------------------------------
// Additional edits by Max Robinson, 1st September, 2005 (Updated to TSE)
//-----------------------------------------------------------------------------

#ifndef _FXSUNLIGHT_H_
#define _FXSUNLIGHT_H_

#ifndef _SCENEOBJECT_H_
#include "sim/sceneObject.h"
#endif

#include "renderInstance/renderInstMgr.h"

//------------------------------------------------------------------------------
// Class: fxSunLight
//------------------------------------------------------------------------------
class fxSunLight : public SceneObject
{
private:
    typedef SceneObject      Parent;

    U32 CheckKeySyntax(StringTableEntry Key);
    void CheckAnimationKeys(void);
    F32 GetLerpKey(StringTableEntry Key, U32 PosFrom, U32 PosTo, F32 ValueFrom, F32 ValueTo, F32 Lerp);
    void AnimateSun(F32 ElapsedTime);
    void InitialiseAnimation(void);
    void ResetAnimation(void);
    bool TestLOS(const Point3F& Pos);


protected:

    enum { fxSunLightConfigChangeMask = BIT(0) };

    bool                  mAddedToScene;

    MRandomLCG               RandomGen;

    // Textures.
    GFXTexHandle            mLocalFlareTextureHandle;
    GFXTexHandle            mRemoteFlareTextureHandle;

    F32                     mElapsedTime;
    S32                     mLastRenderTime;
    F32                     mLocalFlareScale;
    Point3F                 mSunlightPosition;            // Sunlight Frame Position.

public:
    fxSunLight();
    ~fxSunLight();

    // *********************************
    // Configuration Interface.
    // *********************************

    // Debugging.
    void setEnable(bool Status);

    // Media.
    void setFlareBitmaps(const char* LocalName, const char* RemoteName);

    // Sun Orbit.
    void setSunAzimuth(F32 Azimuth);
    void setSunElevation(F32 Elevation);

    // Flare.
    void setFlareTP(bool Status);
    void setFlareColour(ColorF Colour);
    void setFlareBrightness(F32 Brightness);
    void setFlareSize(F32 Size);
    void setLocalScale(F32 Size);
    void setFadeTime(F32 Time);
    void setBlendMode(U32 Mode);

    // Animation Options.
    void setUseColour(bool Status);
    void setUseBrightness(bool Status);
    void setUseRotation(bool Status);
    void setUseSize(bool Status);
    void setUseAzimuth(bool Status);
    void setUseElevation(bool Status);
    void setLerpColour(bool Status);
    void setLerpBrightness(bool Status);
    void setLerpRotation(bool Status);
    void setLerpSize(bool Status);
    void setLerpAzimuth(bool Status);
    void setLerpElevation(bool Status);
    void setLinkFlareSize(bool Status);
    void setSingleColourKeys(bool Status);

    // Animation Extents.
    void setMinColour(ColorF Colour);
    void setMaxColour(ColorF Colour);
    void setMinBrightness(F32 Brightness);
    void setMaxBrightness(F32 Brightness);
    void setMinRotation(F32 Rotation);
    void setMaxRotation(F32 Rotation);
    void setMinSize(F32 Size);
    void setMaxSize(F32 Size);
    void setMinAzimuth(F32 Azimuth);
    void setMaxAzimuth(F32 Azimuth);
    void setMinElevation(F32 Elevation);
    void setMaxElevation(F32 Elevation);

    // Animation Keys.
    void setRedKeys(const char* Keys);
    void setGreenKeys(const char* Keys);
    void setBlueKeys(const char* Keys);
    void setBrightnessKeys(const char* Keys);
    void setRotationKeys(const char* Keys);
    void setSizeKeys(const char* Keys);
    void setAzimuthKeys(const char* Keys);
    void setElevationKeys(const char* Keys);

    // Animation Times.
    void setColourTime(F32 Time);
    void setBrightnessTime(F32 Time);
    void setRotationTime(F32 Time);
    void setSizeTime(F32 Time);
    void setAzimuthTime(F32 Time);
    void setElevationTime(F32 Time);

    // Misc,
    void reset(void);

    // SceneObject
    bool prepRenderImage(SceneState* state, const U32 stateKey, const U32 startZone, const bool modifyBaseZoneState);
    void renderObject(SceneState* state, RenderInst* ri);

    // SimObject      
    bool onAdd();
    void onRemove();
    void inspectPostApply();

    // NetObject
    U32 packUpdate(NetConnection* conn, U32 mask, BitStream* stream);
    void unpackUpdate(NetConnection* conn, BitStream* stream);

    // ConObject.
    static void initPersistFields();

    bool				   mDoneSunLock;

    // Field Data.

    // Debugging.
    bool            mEnable;                  // Light Enable.

    // Media.
    StringTableEntry   mLocalFlareTextureName;         // Local Flare Texture Name.
    StringTableEntry   mRemoteFlareTextureName;      // Remote Flare Texture Name.

   // Sun Orbit.
    F32               mSunAzimuth;
    F32               mSunElevation;
    bool				   mLockToRealSun;			// Lock to Real Sun Flag.

    // Flare.
    bool            mFlareTP;                  // Flare Third Person Flag.
    ColorF            mFlareColour;               // Flare Colour.
    F32               mFlareBrightness;            // Brightness.
    F32               mFlareSize;                  // Flare Size.
    F32               mLocalScale;                // Coefficient to apply to remote flare size
    F32               mFadeTime;                  // Fade Time.
    U32               mBlendMode;                  // Blend Mode.

    // Animation Options.
    bool            mUseColour;                  // Use Colour Flag.
    bool            mUseBrightness;               // Use Brightness Flag.
    bool            mUseRotation;               // Use Rotation Flag.
    bool            mUseSize;                  // Use Size Flag.
    bool            mUseAzimuth;               // Use Azimuth Flag.
    bool            mUseElevation;               // Use Elevation Flag.
    bool            mLerpColour;               // Lerp Colour Flag.
    bool            mLerpBrightness;            // Lerp Brightness Flag.
    bool            mLerpRotation;               // Lerp Rotation Flag.
    bool            mLerpSize;                  // Lerp Size Flag.
    bool            mLerpAzimuth;               // Lerp Azimuth Flag.
    bool            mLerpElevation;               // Lerp Elevation Flag.
    bool            mLinkFlareSize;               // Link Flare Size Animation.
    bool            mSingleColourKeys;            // Single-Channel Colour Keys.

    // Animation Extents.
    ColorF            mMinColour;                  // Minimum Colour.
    ColorF            mMaxColour;                  // Maximum Colour.
    F32               mMinBrightness;               // Minimum Brightness.
    F32               mMaxBrightness;               // Maximum Brightness.
    F32               mMinRotation;               // Minimum Rotation.
    F32               mMaxRotation;               // Maximum Rotation.
    F32               mMinSize;                  // Minimum Size.
    F32               mMaxSize;                  // Maximum Size.
    F32               mMinAzimuth;               // Minimum Azimuth.
    F32               mMaxAzimuth;               // Maximum Azimuth.
    F32               mMinElevation;               // Minimum Elevation.
    F32               mMaxElevation;               // Maximum Elevation.

    // Animation Keys.
    StringTableEntry   mRedKeys;                  // Red Animation Keys.
    StringTableEntry   mGreenKeys;                  // Green Animation Keys.
    StringTableEntry   mBlueKeys;                  // Blue Animation Keys.
    StringTableEntry   mBrightnessKeys;            // Brightness Animation Keys.
    StringTableEntry   mRotationKeys;               // Rotation Animation Keys.
    StringTableEntry   mSizeKeys;                  // Size Animation Keys.
    StringTableEntry   mAzimuthKeys;               // Size Azimuth Keys.
    StringTableEntry   mElevationKeys;               // Size Elevation Keys.

    // Animation Times.
    F32               mColourTime;               // Colour Time (Seconds).
    F32               mBrightnessTime;            // Brightness Time.
    F32               mRotationTime;               // Rotation Time.
    F32               mSizeTime;                  // Size Time.
    F32               mAzimuthTime;               // Azimuth Time.
    F32               mElevationTime;               // Elevation Time.

    // Current Animation.
    ColorF            mAnimationColour;            // Current Colour.
    F32               mAnimationBrightness;         // Current Brightness.
    F32               mAnimationRotation;            // Current Rotation.
    F32               mAnimationSize;               // Current Size.
    F32               mAnimationAzimuth;            // Current Azimuth.
    F32               mAnimationElevation;         // Current Elevation.

    // Elapsed Times.
    F32               mColourElapsedTime;            // Colour Elapsed Time.
    F32               mBrightnessElapsedTime;         // Brightness Elapsed Time.
    F32               mRotationElapsedTime;         // Rotation Elapsed Time.
    F32               mSizeElapsedTime;            // Size Elapsed Time.
    F32               mAzimuthElapsedTime;         // Azimuth Elapsed Time.
    F32               mElevationElapsedTime;         // Elevation Elapsed Time.

    // Time Scales.
    F32               mColourTimeScale;            // Colour Time Scale.
    F32               mBrightnessTimeScale;         // Brightness Time Scale.
    F32               mRotationTimeScale;            // Rotation Time Scale.
    F32               mSizeTimeScale;               // Size Time Scale.
    F32               mAzimuthTimeScale;            // Azimuth Time Scale.
    F32               mElevationTimeScale;         // Elevation Time Scale.

    // Key Lengths (Validity).
    U32               mRedKeysLength;               // Red Keys Length.
    U32               mGreenKeysLength;            // Green Keys Length.
    U32               mBlueKeysLength;            // Blue Keys Length.
    U32               mBrightnessKeysLength;         // Brightness Keys Length.
    U32               mRotationKeysLength;         // Rotation Keys Length.
    U32               mSizeKeysLength;            // Size Keys Length.
    U32               mAzimuthKeysLength;            // Azimuth Keys Length.
    U32               mElevationKeysLength;         // Elevation Keys Length.

    // Declare Console Object.
    DECLARE_CONOBJECT(fxSunLight);
};

#endif // _FXSUNLIGHT_H_
