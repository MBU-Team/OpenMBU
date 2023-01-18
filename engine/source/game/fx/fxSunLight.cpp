//-----------------------------------------------------------------------------
// Torque Shader Engine
// Written by Melvyn May, Started on 27th August 2002.
//
// "My code is written for the Torque community, so do your worst with it,
//	just don't rip-it-off and call it your own without even thanking me".
//
//	- Melv.
//
//-----------------------------------------------------------------------------
// Additional edits by Max Robinson, 1st September, 2005 (Updated to TSE)
//-----------------------------------------------------------------------------

#include "console/consoleTypes.h"
#include "core/bitStream.h"
#include "game/gameConnection.h"
#include "sceneGraph/sceneGraph.h"
#include "math/mathUtils.h"
#include "gfx/primBuilder.h"
#include "fxSunLight.h"
#include "renderInstance/renderInstMgr.h"

//------------------------------------------------------------------------------
//
//	Put this in /example/common/editor/EditorGui.cs in [function Creator::init( %this )]
//
//   %Environment_Item[<next item-index in list>] = "fxSunLight";  <-- ADD THIS.
//
//------------------------------------------------------------------------------
//
//	Put the function in /example/common/editor/ObjectBuilderGui.gui [around line 458] ...
//
//	function ObjectBuilderGui::buildfxSunLight(%this)
//	{
//		%this.className = "fxSunLight";
//		%this.process();
//	}
//
//------------------------------------------------------------------------------
//
//	Put the example 'corona.png' in the "/example/common/lighting/" directory.
//
//------------------------------------------------------------------------------

extern bool gEditingMission;

//------------------------------------------------------------------------------

IMPLEMENT_CO_NETOBJECT_V1(fxSunLight);

//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
// Class: fxSunLight
//------------------------------------------------------------------------------

fxSunLight::fxSunLight()
{
    // Setup NetObject.
    mTypeMask |= StaticObjectType | StaticTSObjectType | StaticRenderedObjectType;
    mNetFlags.set(Ghostable | ScopeAlways);
    mAddedToScene = false;

    // Initialise Animation.
    InitialiseAnimation();

    // Reset Done SunLock Flag.
    mDoneSunLock = false;
}

//------------------------------------------------------------------------------

fxSunLight::~fxSunLight()
{
}

//------------------------------------------------------------------------------

void fxSunLight::initPersistFields()
{
    // Initialise parents' persistent fields.
    Parent::initPersistFields();

    // Add our own persistent fields.

    // Enable.
    addGroup("Debugging");
    addField("Enable", TypeBool, Offset(mEnable, fxSunLight));
    endGroup("Debugging");

    addGroup("Media");	// MM: Added Group Header.
    addField("LocalFlareBitmap", TypeFilename, Offset(mLocalFlareTextureName, fxSunLight));
    addField("RemoteFlareBitmap", TypeFilename, Offset(mRemoteFlareTextureName, fxSunLight));
    endGroup("Media");	// MM: Added Group Footer.

    // Sunlight.
    addGroup("Sun Orbit");	// MM: Added Group Header.
    addField("SunAzimuth", TypeF32, Offset(mSunAzimuth, fxSunLight));
    addField("SunElevation", TypeF32, Offset(mSunElevation, fxSunLight));
    addField("LockToRealSun", TypeBool, Offset(mLockToRealSun, fxSunLight));
    endGroup("Sun Orbit");	// MM: Added Group Footer.

    // Flare.
    addGroup("Lens Flare");	// MM: Added Group Header.
    addField("FlareTP", TypeBool, Offset(mFlareTP, fxSunLight));
    addField("Colour", TypeColorF, Offset(mFlareColour, fxSunLight));
    addField("Brightness", TypeF32, Offset(mFlareBrightness, fxSunLight));
    addField("FlareSize", TypeF32, Offset(mFlareSize, fxSunLight));
    addField("LocalScale", TypeF32, Offset(mLocalScale, fxSunLight));
    addField("FadeTime", TypeF32, Offset(mFadeTime, fxSunLight));
    addField("BlendMode", TypeS32, Offset(mBlendMode, fxSunLight));
    endGroup("Lens Flare");	// MM: Added Group Footer.

    // Animation Options.
    addGroup("Animation Options");	// MM: Added Group Header.
    addField("AnimColour", TypeBool, Offset(mUseColour, fxSunLight));
    addField("AnimBrightness", TypeBool, Offset(mUseBrightness, fxSunLight));
    addField("AnimRotation", TypeBool, Offset(mUseRotation, fxSunLight));
    addField("AnimSize", TypeBool, Offset(mUseSize, fxSunLight));
    addField("AnimAzimuth", TypeBool, Offset(mUseAzimuth, fxSunLight));
    addField("AnimElevation", TypeBool, Offset(mUseElevation, fxSunLight));
    addField("LerpColour", TypeBool, Offset(mLerpColour, fxSunLight));
    addField("LerpBrightness", TypeBool, Offset(mLerpBrightness, fxSunLight));
    addField("LerpRotation", TypeBool, Offset(mLerpRotation, fxSunLight));
    addField("LerpSize", TypeBool, Offset(mLerpSize, fxSunLight));
    addField("LerpAzimuth", TypeBool, Offset(mLerpAzimuth, fxSunLight));
    addField("LerpElevation", TypeBool, Offset(mLerpElevation, fxSunLight));
    addField("LinkFlareSize", TypeBool, Offset(mLinkFlareSize, fxSunLight));
    addField("SingleColourKeys", TypeBool, Offset(mSingleColourKeys, fxSunLight));
    endGroup("Animation Options");	// MM: Added Group Footer.

    // Animation Extents.
    addGroup("Animation Extents");	// MM: Added Group Header.
    addField("MinColour", TypeColorF, Offset(mMinColour, fxSunLight));
    addField("MaxColour", TypeColorF, Offset(mMaxColour, fxSunLight));
    addField("MinBrightness", TypeF32, Offset(mMinBrightness, fxSunLight));
    addField("MaxBrightness", TypeF32, Offset(mMaxBrightness, fxSunLight));
    addField("MinRotation", TypeF32, Offset(mMinRotation, fxSunLight));
    addField("MaxRotation", TypeF32, Offset(mMaxRotation, fxSunLight));
    addField("MinSize", TypeF32, Offset(mMinSize, fxSunLight));
    addField("MaxSize", TypeF32, Offset(mMaxSize, fxSunLight));
    addField("MinAzimuth", TypeF32, Offset(mMinAzimuth, fxSunLight));
    addField("MaxAzimuth", TypeF32, Offset(mMaxAzimuth, fxSunLight));
    addField("MinElevation", TypeF32, Offset(mMinElevation, fxSunLight));
    addField("MaxElevation", TypeF32, Offset(mMaxElevation, fxSunLight));
    endGroup("Animation Extents");	// MM: Added Group Footer.

    // Animation Keys.
    addGroup("Animation Keys");	// MM: Added Group Header.
    addField("RedKeys", TypeString, Offset(mRedKeys, fxSunLight));
    addField("GreenKeys", TypeString, Offset(mGreenKeys, fxSunLight));
    addField("BlueKeys", TypeString, Offset(mBlueKeys, fxSunLight));
    addField("BrightnessKeys", TypeString, Offset(mBrightnessKeys, fxSunLight));
    addField("RotationKeys", TypeString, Offset(mRotationKeys, fxSunLight));
    addField("SizeKeys", TypeString, Offset(mSizeKeys, fxSunLight));
    addField("AzimuthKeys", TypeString, Offset(mAzimuthKeys, fxSunLight));
    addField("ElevationKeys", TypeString, Offset(mElevationKeys, fxSunLight));
    endGroup("Animation Keys");	// MM: Added Group Footer.

    // Animation Times.
    addGroup("Animation Times");	// MM: Added Group Header.
    addField("ColourTime", TypeF32, Offset(mColourTime, fxSunLight));
    addField("BrightnessTime", TypeF32, Offset(mBrightnessTime, fxSunLight));
    addField("RotationTime", TypeF32, Offset(mRotationTime, fxSunLight));
    addField("SizeTime", TypeF32, Offset(mSizeTime, fxSunLight));
    addField("AzimuthTime", TypeF32, Offset(mAzimuthTime, fxSunLight));
    addField("ElevationTime", TypeF32, Offset(mElevationTime, fxSunLight));
    endGroup("Animation Times");	// MM: Added Group Footer.
}

//------------------------------------------------------------------------------

// ********************************************************************************
// Debugging.
// ********************************************************************************

void fxSunLight::setEnable(bool Status)
{
    // Set Attribute.
    mEnable = Status;
    // Set Config Change Mask.
    if (isServerObject()) setMaskBits(fxSunLightConfigChangeMask);
}

// ********************************************************************************
// Media.
// ********************************************************************************

void fxSunLight::setFlareBitmaps(const char* LocalName, const char* RemoteName)
{
    // Set Flare Texture Names.
    mLocalFlareTextureName = StringTable->insert(LocalName);
    mRemoteFlareTextureName = StringTable->insert(RemoteName);

    // Only let the client load an actual texture.
    if (isClientObject())
    {
        // Reset existing flare texture.
        mLocalFlareTextureHandle = NULL;
        mRemoteFlareTextureHandle = NULL;

        // Load textures if valid.
        if (*mLocalFlareTextureName) mLocalFlareTextureHandle.set(mLocalFlareTextureName, &GFXDefaultStaticDiffuseProfile);
        if (*mRemoteFlareTextureName) mRemoteFlareTextureHandle.set(mRemoteFlareTextureName, &GFXDefaultStaticDiffuseProfile);
    }

    // Set Config Change Mask.
    if (isServerObject()) setMaskBits(fxSunLightConfigChangeMask);
}

// ********************************************************************************
// Sun Orbit.
// ********************************************************************************

void fxSunLight::setSunAzimuth(F32 Azimuth)
{
    // Set Attribute.
    mSunAzimuth = Azimuth;
    // Set Config Change Mask.
    if (isServerObject()) setMaskBits(fxSunLightConfigChangeMask);
}

void fxSunLight::setSunElevation(F32 Elevation)
{
    // Set Attribute.
    mSunElevation = Elevation;
    // Set Config Change Mask.
    if (isServerObject()) setMaskBits(fxSunLightConfigChangeMask);
}

// ********************************************************************************
// Flare.
// ********************************************************************************

void fxSunLight::setFlareTP(bool Status)
{
    // Set Attribute.
    mFlareTP = Status;
    // Set Config Change Mask.
    if (isServerObject()) setMaskBits(fxSunLightConfigChangeMask);
}

void fxSunLight::setFlareColour(ColorF Colour)
{
    // Set Attribute.
    mFlareColour = Colour;
    // Set Config Change Mask.
    if (isServerObject()) setMaskBits(fxSunLightConfigChangeMask);
}

void fxSunLight::setFlareBrightness(F32 Brightness)
{
    // Set Attribute.
    mFlareBrightness = Brightness;
    // Set Config Change Mask.
    if (isServerObject()) setMaskBits(fxSunLightConfigChangeMask);
}

void fxSunLight::setFlareSize(F32 Size)
{
    // Set Attribute.
    mFlareSize = Size;
    // Set Config Change Mask.
    if (isServerObject()) setMaskBits(fxSunLightConfigChangeMask);
}

void fxSunLight::setLocalScale(F32 Size)
{
    // Set Attribute.
    mLocalScale = Size;
    // Set Config Change Mask.
    if (isServerObject()) setMaskBits(fxSunLightConfigChangeMask);
}

void fxSunLight::setFadeTime(F32 Time)
{
    // Set Attribute.
    mFadeTime = Time;
    // Set Config Change Mask.
    if (isServerObject()) setMaskBits(fxSunLightConfigChangeMask);
}

void fxSunLight::setBlendMode(U32 Mode)
{
    // Set Attribute.
    mBlendMode = Mode;
    // Set Config Change Mask.
    if (isServerObject()) setMaskBits(fxSunLightConfigChangeMask);
}

// ********************************************************************************
// Animation Options.
// ********************************************************************************

void fxSunLight::setUseColour(bool Status)
{
    // Set Attribute.
    mUseColour = Status;
    // Set Config Change Mask.
    if (isServerObject()) setMaskBits(fxSunLightConfigChangeMask);
}

void fxSunLight::setUseBrightness(bool Status)
{
    // Set Attribute.
    mUseBrightness = Status;
    // Set Config Change Mask.
    if (isServerObject()) setMaskBits(fxSunLightConfigChangeMask);
}

void fxSunLight::setUseRotation(bool Status)
{
    // Set Attribute.
    mUseRotation = Status;
    // Set Config Change Mask.
    if (isServerObject()) setMaskBits(fxSunLightConfigChangeMask);
}

void fxSunLight::setUseSize(bool Status)
{
    // Set Attribute.
    mUseSize = Status;
    // Set Config Change Mask.
    if (isServerObject()) setMaskBits(fxSunLightConfigChangeMask);
}

void fxSunLight::setUseAzimuth(bool Status)
{
    // Set Attribute.
    mUseAzimuth = Status;
    // Set Config Change Mask.
    if (isServerObject()) setMaskBits(fxSunLightConfigChangeMask);
}

void fxSunLight::setUseElevation(bool Status)
{
    // Set Attribute.
    mUseElevation = Status;
    // Set Config Change Mask.
    if (isServerObject()) setMaskBits(fxSunLightConfigChangeMask);
}

void fxSunLight::setLerpColour(bool Status)
{
    // Set Attribute.
    mLerpColour = Status;
    // Set Config Change Mask.
    if (isServerObject()) setMaskBits(fxSunLightConfigChangeMask);
}

void fxSunLight::setLerpBrightness(bool Status)
{
    // Set Attribute.
    mLerpBrightness = Status;
    // Set Config Change Mask.
    if (isServerObject()) setMaskBits(fxSunLightConfigChangeMask);
}

void fxSunLight::setLerpRotation(bool Status)
{
    // Set Attribute.
    mLerpRotation = Status;
    // Set Config Change Mask.
    if (isServerObject()) setMaskBits(fxSunLightConfigChangeMask);
}

void fxSunLight::setLerpSize(bool Status)
{
    // Set Attribute.
    mLerpSize = Status;
    // Set Config Change Mask.
    if (isServerObject()) setMaskBits(fxSunLightConfigChangeMask);
}

void fxSunLight::setLerpAzimuth(bool Status)
{
    // Set Attribute.
    mLerpAzimuth = Status;
    // Set Config Change Mask.
    if (isServerObject()) setMaskBits(fxSunLightConfigChangeMask);
}

void fxSunLight::setLerpElevation(bool Status)
{
    // Set Attribute.
    mLerpElevation = Status;
    // Set Config Change Mask.
    if (isServerObject()) setMaskBits(fxSunLightConfigChangeMask);
}

void fxSunLight::setLinkFlareSize(bool Status)
{
    // Set Attribute.
    mLinkFlareSize = Status;
    // Set Config Change Mask.
    if (isServerObject()) setMaskBits(fxSunLightConfigChangeMask);
}

void fxSunLight::setSingleColourKeys(bool Status)
{
    // Set Attribute.
    mSingleColourKeys = Status;
    // Set Config Change Mask.
    if (isServerObject()) setMaskBits(fxSunLightConfigChangeMask);
}

// ********************************************************************************
// Animation Extents.
// ********************************************************************************

void fxSunLight::setMinColour(ColorF Colour)
{
    // Set Attribute.
    mMinColour = Colour;
    // Set Config Change Mask.
    if (isServerObject()) setMaskBits(fxSunLightConfigChangeMask);
}

void fxSunLight::setMaxColour(ColorF Colour)
{
    // Set Attribute.
    mMaxColour = Colour;
    // Set Config Change Mask.
    if (isServerObject()) setMaskBits(fxSunLightConfigChangeMask);
}

void fxSunLight::setMinBrightness(F32 Brightness)
{
    // Set Attribute.
    mMinBrightness = Brightness;
    // Set Config Change Mask.
    if (isServerObject()) setMaskBits(fxSunLightConfigChangeMask);
}

void fxSunLight::setMaxBrightness(F32 Brightness)
{
    // Set Attribute.
    mMaxBrightness = Brightness;
    // Set Config Change Mask.
    if (isServerObject()) setMaskBits(fxSunLightConfigChangeMask);
}

void fxSunLight::setMinRotation(F32 Rotation)
{
    // Set Attribute.
    mMinRotation = Rotation;
    // Set Config Change Mask.
    if (isServerObject()) setMaskBits(fxSunLightConfigChangeMask);
}

void fxSunLight::setMaxRotation(F32 Rotation)
{
    // Set Attribute.
    mMaxRotation = Rotation;
    // Set Config Change Mask.
    if (isServerObject()) setMaskBits(fxSunLightConfigChangeMask);
}

void fxSunLight::setMinSize(F32 Size)
{
    // Set Attribute.
    mMinSize = Size;
    // Set Config Change Mask.
    if (isServerObject()) setMaskBits(fxSunLightConfigChangeMask);
}

void fxSunLight::setMaxSize(F32 Size)
{
    // Set Attribute.
    mMaxSize = Size;
    // Set Config Change Mask.
    if (isServerObject()) setMaskBits(fxSunLightConfigChangeMask);
}

void fxSunLight::setMinAzimuth(F32 Azimuth)
{
    // Set Attribute.
    mMinAzimuth = Azimuth;
    // Set Config Change Mask.
    if (isServerObject()) setMaskBits(fxSunLightConfigChangeMask);
}

void fxSunLight::setMaxAzimuth(F32 Azimuth)
{
    // Set Attribute.
    mMaxAzimuth = Azimuth;
    // Set Config Change Mask.
    if (isServerObject()) setMaskBits(fxSunLightConfigChangeMask);
}

void fxSunLight::setMinElevation(F32 Elevation)
{
    // Set Attribute.
    mMinElevation = Elevation;
    // Set Config Change Mask.
    if (isServerObject()) setMaskBits(fxSunLightConfigChangeMask);
}

void fxSunLight::setMaxElevation(F32 Elevation)
{
    // Set Attribute.
    mMaxElevation = Elevation;
    // Set Config Change Mask.
    if (isServerObject()) setMaskBits(fxSunLightConfigChangeMask);
}

// ********************************************************************************
// Animation Keys.
// ********************************************************************************

void fxSunLight::setRedKeys(const char* Keys)
{
    // Set Attribute.
    mRedKeys = StringTable->insert(Keys);
    // Set Config Change Mask.
    if (isServerObject()) setMaskBits(fxSunLightConfigChangeMask);
}

void fxSunLight::setGreenKeys(const char* Keys)
{
    // Set Attribute.
    mGreenKeys = StringTable->insert(Keys);
    // Set Config Change Mask.
    if (isServerObject()) setMaskBits(fxSunLightConfigChangeMask);
}

void fxSunLight::setBlueKeys(const char* Keys)
{
    // Set Attribute.
    mBlueKeys = StringTable->insert(Keys);
    // Set Config Change Mask.
    if (isServerObject()) setMaskBits(fxSunLightConfigChangeMask);
}

void fxSunLight::setBrightnessKeys(const char* Keys)
{
    // Set Attribute.
    mBrightnessKeys = StringTable->insert(Keys);
    // Set Config Change Mask.
    if (isServerObject()) setMaskBits(fxSunLightConfigChangeMask);
}

void fxSunLight::setRotationKeys(const char* Keys)
{
    // Set Attribute.
    mRotationKeys = StringTable->insert(Keys);
    // Set Config Change Mask.
    if (isServerObject()) setMaskBits(fxSunLightConfigChangeMask);
}

void fxSunLight::setSizeKeys(const char* Keys)
{
    // Set Attribute.
    mSizeKeys = StringTable->insert(Keys);
    // Set Config Change Mask.
    if (isServerObject()) setMaskBits(fxSunLightConfigChangeMask);
}

void fxSunLight::setAzimuthKeys(const char* Keys)
{
    // Set Attribute.
    mAzimuthKeys = StringTable->insert(Keys);
    // Set Config Change Mask.
    if (isServerObject()) setMaskBits(fxSunLightConfigChangeMask);
}

void fxSunLight::setElevationKeys(const char* Keys)
{
    // Set Attribute.
    mElevationKeys = StringTable->insert(Keys);
    // Set Config Change Mask.
    if (isServerObject()) setMaskBits(fxSunLightConfigChangeMask);
}

// ********************************************************************************
// Animation Times.
// ********************************************************************************

void fxSunLight::setColourTime(F32 Time)
{
    // Check for error.
    if (mColourTime <= 0) return;
    // Set Attribute.
    mColourTime = Time;
    // Set Config Change Mask.
    if (isServerObject()) setMaskBits(fxSunLightConfigChangeMask);
}

void fxSunLight::setBrightnessTime(F32 Time)
{
    // Check for error.
    if (mBrightnessTime <= 0) return;
    // Set Attribute.
    mBrightnessTime = Time;
    // Set Config Change Mask.
    if (isServerObject()) setMaskBits(fxSunLightConfigChangeMask);
}

void fxSunLight::setRotationTime(F32 Time)
{
    // Check for error.
    if (mRotationTime <= 0) return;
    // Set Attribute.
    mRotationTime = Time;
    // Set Config Change Mask.
    if (isServerObject()) setMaskBits(fxSunLightConfigChangeMask);
}

void fxSunLight::setSizeTime(F32 Time)
{
    // Check for error.
    if (mSizeTime <= 0) return;
    // Set Attribute.
    mSizeTime = Time;
    // Set Config Change Mask.
    if (isServerObject()) setMaskBits(fxSunLightConfigChangeMask);
}

void fxSunLight::setAzimuthTime(F32 Time)
{
    // Check for error.
    if (mAzimuthTime <= 0) return;
    // Set Attribute.
    mAzimuthTime = Time;
    // Set Config Change Mask.
    if (isServerObject()) setMaskBits(fxSunLightConfigChangeMask);
}

void fxSunLight::setElevationTime(F32 Time)
{
    // Check for error.
    if (mElevationTime <= 0) return;
    // Set Attribute.
    mElevationTime = Time;
    // Set Config Change Mask.
    if (isServerObject()) setMaskBits(fxSunLightConfigChangeMask);
}


// ********************************************************************************
// Misc.
// ********************************************************************************

void fxSunLight::reset(void)
{
    // Reset Animation.
    ResetAnimation();
    // Set Config Change Mask.
    if (isServerObject()) setMaskBits(fxSunLightConfigChangeMask);
}

//------------------------------------------------------------------------------

// ********************************************************************************
// Debugging.
// ********************************************************************************

ConsoleMethod(fxSunLight, setEnable, void, 3, 3, "(status)")
{
    object->setEnable(dAtob(argv[2]));
}

// ********************************************************************************
// Media.
// ********************************************************************************

ConsoleMethod(fxSunLight, setFlareBitmaps, void, 4, 4, "(local, remote)")
{
    object->setFlareBitmaps(argv[2], argv[3]);
}

// ********************************************************************************
// Sun Orbit.
// ********************************************************************************

ConsoleMethod(fxSunLight, setSunAzimuth, void, 3, 3, "(azimuth)")
{
    object->setSunAzimuth(dAtof(argv[2]));
}

ConsoleMethod(fxSunLight, setSunElevation, void, 3, 3, "(elevation)")
{
    object->setSunElevation(dAtof(argv[2]));
}

// ********************************************************************************
// Flare.
// ********************************************************************************

ConsoleMethod(fxSunLight, setFlareTP, void, 3, 3, "(status)")
{
    object->setFlareTP(dAtob(argv[2]));
}

ConsoleMethod(fxSunLight, setFlareColour, void, 5, 5, "(r,g,b)")
{
    object->setFlareColour(ColorF(dAtof(argv[2]), dAtof(argv[3]), dAtof(argv[4])));
}

ConsoleMethod(fxSunLight, setFlareBrightness, void, 3, 3, "(brightness)")
{
    object->setFlareBrightness(dAtof(argv[2]));
}

ConsoleMethod(fxSunLight, setFlareSize, void, 3, 3, "(size)")
{
    object->setFlareSize(dAtof(argv[2]));
}

ConsoleMethod(fxSunLight, setLocalScale, void, 3, 3, "(size)")
{
    object->setLocalScale(dAtof(argv[2]));
}

ConsoleMethod(fxSunLight, setFadeTime, void, 3, 3, "(time)")
{
    object->setFadeTime(dAtof(argv[2]));
}

ConsoleMethod(fxSunLight, setBlendMode, void, 3, 3, "(mode)")
{
    object->setBlendMode(dAtoi(argv[2]));
}

// ********************************************************************************
// Animation Options.
// ********************************************************************************

ConsoleMethod(fxSunLight, setUseColour, void, 3, 3, "(status)")
{
    object->setUseColour(dAtob(argv[2]));
}

ConsoleMethod(fxSunLight, setUseBrightness, void, 3, 3, "(status)")
{
    object->setUseBrightness(dAtob(argv[2]));
}

ConsoleMethod(fxSunLight, setUseRotation, void, 3, 3, "(status)")
{
    object->setUseRotation(dAtob(argv[2]));
}

ConsoleMethod(fxSunLight, setUseSize, void, 3, 3, "(status)")
{
    object->setUseSize(dAtob(argv[2]));
}

ConsoleMethod(fxSunLight, setUseAzimuth, void, 3, 3, "(status)")
{
    object->setUseAzimuth(dAtob(argv[2]));
}

ConsoleMethod(fxSunLight, setUseElevation, void, 3, 3, "(status)")
{
    object->setUseElevation(dAtob(argv[2]));
}

ConsoleMethod(fxSunLight, setLerpColour, void, 3, 3, "(status)")
{
    object->setLerpColour(dAtob(argv[2]));
}

ConsoleMethod(fxSunLight, setLerpBrightness, void, 3, 3, "(status)")
{
    object->setLerpBrightness(dAtob(argv[2]));
}

ConsoleMethod(fxSunLight, setLerpRotation, void, 3, 3, "(status)")
{
    object->setLerpRotation(dAtob(argv[2]));
}

ConsoleMethod(fxSunLight, setLerpSize, void, 3, 3, "(status)")
{
    object->setLerpSize(dAtob(argv[2]));
}

ConsoleMethod(fxSunLight, setLerpAzimuth, void, 3, 3, "(status)")
{
    object->setLerpAzimuth(dAtob(argv[2]));
}

ConsoleMethod(fxSunLight, setLerpElevation, void, 3, 3, "(status)")
{
    object->setLerpElevation(dAtob(argv[2]));
}

ConsoleMethod(fxSunLight, setLinkFlareSize, void, 3, 3, "(status)")
{
    object->setLinkFlareSize(dAtob(argv[2]));
}

ConsoleMethod(fxSunLight, setSingleColourKeys, void, 3, 3, "(status)")
{
    object->setSingleColourKeys(dAtob(argv[2]));
}

// ********************************************************************************
// Animation Extents.
// ********************************************************************************

ConsoleMethod(fxSunLight, setMinColour, void, 5, 5, "(r,g,b)")
{
    object->setMinColour(ColorF(dAtof(argv[2]), dAtof(argv[3]), dAtof(argv[4])));
}

ConsoleMethod(fxSunLight, setMaxColour, void, 5, 5, "(r,g,b)")
{
    object->setMaxColour(ColorF(dAtof(argv[2]), dAtof(argv[3]), dAtof(argv[4])));
}

ConsoleMethod(fxSunLight, setMinBrightness, void, 3, 3, "(brightness)")
{
    object->setMinBrightness(dAtof(argv[2]));
}

ConsoleMethod(fxSunLight, setMaxBrightness, void, 3, 3, "(brightness)")
{
    object->setMaxBrightness(dAtof(argv[2]));
}

ConsoleMethod(fxSunLight, setMinRotation, void, 3, 3, "(rotation)")
{
    object->setMinRotation(dAtof(argv[2]));
}

ConsoleMethod(fxSunLight, setMaxRotation, void, 3, 3, "(rotation)")
{
    object->setMaxRotation(dAtof(argv[2]));
}

ConsoleMethod(fxSunLight, setMinSize, void, 3, 3, "(size)")
{
    object->setMinSize(dAtof(argv[2]));
}

ConsoleMethod(fxSunLight, setMaxSize, void, 3, 3, "(size)")
{
    object->setMaxSize(dAtof(argv[2]));
}

ConsoleMethod(fxSunLight, setMinAzimuth, void, 3, 3, "(azimuth)")
{
    object->setMinAzimuth(dAtof(argv[2]));
}

ConsoleMethod(fxSunLight, setMaxAzimuth, void, 3, 3, "(azimuth)")
{
    object->setMaxAzimuth(dAtof(argv[2]));
}

ConsoleMethod(fxSunLight, setMinElevation, void, 3, 3, "(elevation)")
{
    object->setMinElevation(dAtof(argv[2]));
}

ConsoleMethod(fxSunLight, setMaxElevation, void, 3, 3, "(elevation)")
{
    object->setMaxElevation(dAtof(argv[2]));
}

// ********************************************************************************
// Animation Keys.
// ********************************************************************************

ConsoleMethod(fxSunLight, setRedKeys, void, 3, 3, "(keys)")
{
    object->setRedKeys(argv[2]);
}

ConsoleMethod(fxSunLight, setGreenKeys, void, 3, 3, "(keys)")
{
    object->setGreenKeys(argv[2]);
}

ConsoleMethod(fxSunLight, setBlueKeys, void, 3, 3, "(keys)")
{
    object->setBlueKeys(argv[2]);
}

ConsoleMethod(fxSunLight, setBrightnessKeys, void, 3, 3, "(keys)")
{
    object->setBrightnessKeys(argv[2]);
}

ConsoleMethod(fxSunLight, setRotationKeys, void, 3, 3, "(keys)")
{
    object->setRotationKeys(argv[2]);
}

ConsoleMethod(fxSunLight, setSizeKeys, void, 3, 3, "(keys)")
{
    object->setSizeKeys(argv[2]);
}

ConsoleMethod(fxSunLight, setAzimuthKeys, void, 3, 3, "(keys)")
{
    object->setAzimuthKeys(argv[2]);
}

ConsoleMethod(fxSunLight, setElevationKeys, void, 3, 3, "(keys)")
{
    object->setElevationKeys(argv[2]);
}

// ********************************************************************************
// Animation Times.
// ********************************************************************************

ConsoleMethod(fxSunLight, setColourTime, void, 3, 3, "(time)")
{
    object->setColourTime(dAtof(argv[2]));
}

ConsoleMethod(fxSunLight, setBrightnessTime, void, 3, 3, "(time)")
{
    object->setBrightnessTime(dAtof(argv[2]));
}

ConsoleMethod(fxSunLight, setRotationTime, void, 3, 3, "(time)")
{
    object->setRotationTime(dAtof(argv[2]));
}

ConsoleMethod(fxSunLight, setSizeTime, void, 3, 3, "(time)")
{
    object->setSizeTime(dAtof(argv[2]));
}

ConsoleMethod(fxSunLight, setAzimuthTime, void, 3, 3, "(time)")
{
    object->setAzimuthTime(dAtof(argv[2]));
}

ConsoleMethod(fxSunLight, setElevationTime, void, 3, 3, "(time)")
{
    object->setElevationTime(dAtof(argv[2]));
}

// ********************************************************************************
// Misc.
// ********************************************************************************

ConsoleMethod(fxSunLight, reset, void, 2, 2, "()")
{
    object->reset();
}

//------------------------------------------------------------------------------

void fxSunLight::AnimateSun(F32 ElapsedTime)
{
    // ********************************************
    // Calculate Colour.
    // ********************************************

    // Animating Colour?
    if (mUseColour)
    {
        U32 PosFrom;
        U32 PosTo;
        F32	LerpFactor;

        // Yes, so adjust time-base.
        mColourElapsedTime += ElapsedTime;

        // Adjust to Bounds.
        while (mColourElapsedTime > mColourTime) { mColourElapsedTime -= mColourTime; };

        // Scale Time.
        F32 ScaledTime = mColourElapsedTime * mColourTimeScale;

        // Calculate Position From.
        PosFrom = (U32)(mFloor(ScaledTime));

        // Are we Lerping?
        if (mLerpColour)
        {
            // Yes, so calculate Position To.
            PosTo = (U32)(mCeil(ScaledTime));

            // Calculate Lerp Factor.
            LerpFactor = ScaledTime - PosFrom;
        }
        else
        {
            // No, so clamp Position.
            PosTo = PosFrom;

            // Clamp lerp factor.
            LerpFactor = 0.0f;
        }

        // Reset RGB Set Flag.
        bool RedSet = false;
        bool GreenSet = false;
        bool BlueSet = false;

        // ********************************************
        // Red<GREEN,BLUE> Keys.
        // ********************************************
        if (mRedKeysLength)
        {
            // Are we using single-channel colour keys?
            if (mSingleColourKeys)
            {
                // Yes, so calculate rgb from red keys.
                mAnimationColour.red = GetLerpKey(mRedKeys, PosFrom, PosTo, mMinColour.red, mMaxColour.red, LerpFactor);
                mAnimationColour.green = GetLerpKey(mRedKeys, PosFrom, PosTo, mMinColour.green, mMaxColour.green, LerpFactor);
                mAnimationColour.blue = GetLerpKey(mRedKeys, PosFrom, PosTo, mMinColour.blue, mMaxColour.blue, LerpFactor);

                // Flag RGB Set.
                RedSet = GreenSet = BlueSet = true;
            }
            else
            {
                // No, so calculate Red only.
                mAnimationColour.red = GetLerpKey(mRedKeys, PosFrom, PosTo, mMinColour.red, mMaxColour.red, LerpFactor);

                // Flag Red Set.
                RedSet = true;
            }
        }

        // ********************************************
        // Green Keys.
        // ********************************************
        if (!mSingleColourKeys && mGreenKeysLength)
        {
            // Calculate Green.
            mAnimationColour.green = GetLerpKey(mGreenKeys, PosFrom, PosTo, mMinColour.green, mMaxColour.green, LerpFactor);

            // Flag Green Set.
            GreenSet = true;
        }

        // ********************************************
        // Blue Keys.
        // ********************************************
        if (!mSingleColourKeys && mBlueKeysLength)
        {
            // Calculate Blue.
            mAnimationColour.blue = GetLerpKey(mGreenKeys, PosFrom, PosTo, mMinColour.blue, mMaxColour.blue, LerpFactor);

            // Flag Blue Set.
            BlueSet = true;
        }

        // Set to static colour if we failed to set RGB correctly.
        if (!RedSet || !GreenSet || !BlueSet) mAnimationColour = mFlareColour;
    }
    else
    {
        // No, so set to static Colour.
        mAnimationColour = mFlareColour;
    }


    // ********************************************
    // Calculate Brightness.
    // ********************************************

    // Animating Brightness?
    if (mUseBrightness)
    {
        U32 PosFrom;
        U32 PosTo;
        F32	LerpFactor;

        // Yes, so adjust time-base.
        mBrightnessElapsedTime += ElapsedTime;

        // Adjust to Bounds.
        while (mBrightnessElapsedTime > mBrightnessTime) { mBrightnessElapsedTime -= mBrightnessTime; };

        // Scale Time.
        F32 ScaledTime = mBrightnessElapsedTime * mBrightnessTimeScale;

        // Calculate Position From.
        PosFrom = (U32)(mFloor(ScaledTime));

        // Are we Lerping?
        if (mLerpBrightness)
        {
            // Yes, so calculate Position To.
            PosTo = (U32)(mCeil(ScaledTime));

            // Calculate Lerp Factor.
            LerpFactor = ScaledTime - PosFrom;
        }
        else
        {
            // No, so clamp Position.
            PosTo = PosFrom;

            // Clamp lerp factor.
            LerpFactor = 0.0f;
        }

        // ********************************************
        // Brightness Keys.
        // ********************************************
        if (mBrightnessKeysLength)
        {
            // No, so calculate Brightness.
            mAnimationBrightness = GetLerpKey(mBrightnessKeys, PosFrom, PosTo, mMinBrightness, mMaxBrightness, LerpFactor);
        }
        else
        {
            // Set to static brightness if we failed to set Brightness correctly.
            mAnimationBrightness = mFlareBrightness;
        }
    }
    else
    {
        // No, so set to static Brightness.
        mAnimationBrightness = mFlareBrightness;
    }

    // Adjust Colour by Brightness.
    mAnimationColour *= ColorF(mAnimationBrightness, mAnimationBrightness, mAnimationBrightness);


    // ********************************************
    // Calculate Rotation.
    // ********************************************

    // Animating Rotation?
    if (mUseRotation)
    {
        U32 PosFrom;
        U32 PosTo;
        F32	LerpFactor;

        // Yes, so adjust time-base.
        mRotationElapsedTime += ElapsedTime;

        // Adjust to Bounds.
        while (mRotationElapsedTime > mRotationTime) { mRotationElapsedTime -= mRotationTime; };

        // Scale Time.
        F32 ScaledTime = mRotationElapsedTime * mRotationTimeScale;

        // Calculate Position From.
        PosFrom = (U32)(mFloor(ScaledTime));

        // Are we Lerping?
        if (mLerpRotation)
        {
            // Yes, so calculate Position To.
            PosTo = (U32)(mCeil(ScaledTime));

            // Calculate Lerp Factor.
            LerpFactor = ScaledTime - PosFrom;
        }
        else
        {
            // No, so clamp Position.
            PosTo = PosFrom;

            // Clamp lerp factor.
            LerpFactor = 0.0f;
        }

        // ********************************************
        // Rotation Keys.
        // ********************************************
        if (mRotationKeysLength)
        {
            // No, so calculate Rotation.
            mAnimationRotation = GetLerpKey(mRotationKeys, PosFrom, PosTo, mMinRotation, mMaxRotation, LerpFactor);
        }
        else
        {
            // Set to static Rotation if we failed to set Rotation correctly.
            mAnimationRotation = mMinRotation;
        }
    }
    else
    {
        // No, so set to static Rotation.
        mAnimationRotation = 0;
    }


    // ********************************************
    // Calculate Size.
    // ********************************************

    // Animating Size?
    if (mUseSize)
    {
        U32 PosFrom;
        U32 PosTo;
        F32	LerpFactor;

        // Yes, so adjust time-base.
        mSizeElapsedTime += ElapsedTime;

        // Adjust to Bounds.
        while (mSizeElapsedTime > mSizeTime) { mSizeElapsedTime -= mSizeTime; };

        // Scale Time.
        F32 ScaledTime = mSizeElapsedTime * mSizeTimeScale;

        // Calculate Position From.
        PosFrom = (U32)(mFloor(ScaledTime));

        // Are we Lerping?
        if (mLerpSize)
        {
            // Yes, so calculate Position To.
            PosTo = (U32)(mCeil(ScaledTime));

            // Calculate Lerp Factor.
            LerpFactor = ScaledTime - PosFrom;
        }
        else
        {
            // No, so clamp Position.
            PosTo = PosFrom;

            // Clamp lerp factor.
            LerpFactor = 0.0f;
        }

        // ********************************************
        // Size Keys.
        // ********************************************
        if (mSizeKeysLength)
        {
            // No, so calculate Size.
            mAnimationSize = GetLerpKey(mSizeKeys, PosFrom, PosTo, mMinSize, mMaxSize, LerpFactor);
        }
        else
        {
            // Set to static Size if we failed to set Size correctly.
            mAnimationSize = mMinSize;
        }
    }
    else
    {
        // No, so set to static Size.
        mAnimationSize = mFlareSize;
    }


    // ********************************************
    // Calculate Azimuth.
    // ********************************************

    // Animating Size?
    if (mUseAzimuth)
    {
        U32 PosFrom;
        U32 PosTo;
        F32	LerpFactor;

        // Yes, so adjust time-base.
        mAzimuthElapsedTime += ElapsedTime;

        // Adjust to Bounds.
        while (mAzimuthElapsedTime > mAzimuthTime) { mAzimuthElapsedTime -= mAzimuthTime; };

        // Scale Time.
        F32 ScaledTime = mAzimuthElapsedTime * mAzimuthTimeScale;

        // Calculate Position From.
        PosFrom = (U32)(mFloor(ScaledTime));

        // Are we Lerping?
        if (mLerpAzimuth)
        {
            // Yes, so calculate Position To.
            PosTo = (U32)(mCeil(ScaledTime));

            // Calculate Lerp Factor.
            LerpFactor = ScaledTime - PosFrom;
        }
        else
        {
            // No, so clamp Position.
            PosTo = PosFrom;

            // Clamp lerp factor.
            LerpFactor = 0.0f;
        }

        // ********************************************
        // Azimuth Keys.
        // ********************************************
        if (mAzimuthKeysLength)
        {
            // No, so calculate Size.
            mAnimationAzimuth = GetLerpKey(mAzimuthKeys, PosFrom, PosTo, mMinAzimuth, mMaxAzimuth, LerpFactor);
        }
        else
        {
            // Set to static Azimuth if we failed to set Size correctly.
            mAnimationAzimuth = mMinAzimuth;
        }
    }
    else
    {
        // No, so set to static Azimuth.
        mAnimationAzimuth = 0;
    }


    // ********************************************
    // Calculate Elevation.
    // ********************************************

    // Animating Size?
    if (mUseElevation)
    {
        U32 PosFrom;
        U32 PosTo;
        F32	LerpFactor;

        // Yes, so adjust time-base.
        mElevationElapsedTime += ElapsedTime;

        // Adjust to Bounds.
        while (mElevationElapsedTime > mElevationTime) { mElevationElapsedTime -= mElevationTime; };

        // Scale Time.
        F32 ScaledTime = mElevationElapsedTime * mElevationTimeScale;

        // Calculate Position From.
        PosFrom = (S32)(mFloor(ScaledTime));

        // Are we Lerping?
        if (mLerpElevation)
        {
            // Yes, so calculate Position To.
            PosTo = (S32)(mCeil(ScaledTime));

            // Calculate Lerp Factor.
            LerpFactor = ScaledTime - PosFrom;
        }
        else
        {
            // No, so clamp Position.
            PosTo = PosFrom;

            // Clamp lerp factor.
            LerpFactor = 0.0f;
        }

        // ********************************************
        // Elevation Keys.
        // ********************************************
        if (mElevationKeysLength)
        {
            // No, so calculate Size.
            mAnimationElevation = GetLerpKey(mElevationKeys, PosFrom, PosTo, mMinElevation, mMaxElevation, LerpFactor);
        }
        else
        {
            // Set to static Elevation if we failed to set Size correctly.
            mAnimationElevation = mMinElevation;
        }
    }
    else
    {
        // No, so set to static Elevation.
        mAnimationElevation = 0;
    }


    // ********************************************
    // Calculate Sun Position.
    // ********************************************

    // *************************************************************
    // Calculate sun position in the sky relative from the
    // controlling object eye transform.
    // *************************************************************

    // Calculate Sun Vector.
    Point3F SunlightOffset;

    // Fetch Sun Azimuth/Elevation (animated if using it).
    F32 mTempAzimuth = mUseAzimuth ? mAnimationAzimuth : mSunAzimuth;
    F32 mTempElevation = mUseElevation ? mAnimationElevation : mSunElevation;

    // Adjust orbit calculation for elevations exceeding -90 or +90 so
    // that we can effectively have elevations from -360/+360 handled
    // correctly.  We can then happily animate full cyclic sun orbits.
    if (mTempElevation < -90)
    {
        // Adjust Azimuth/Elevations.
        mTempElevation = -90.0f + (mTempElevation + 90.0f);
        mTempAzimuth += (mTempAzimuth >= 180.0f) ? -180.0f : 180.0f;

    }
    else if (mTempElevation > +90)
    {
        // Adjust Azimuth/Elevations.
        mTempElevation = 90.0f - (mTempElevation - 90.0f);
        mTempAzimuth += (mTempAzimuth >= 180.0f) ? -180.0f : 180.0f;
    }

    // Create clamped Yaw/Patch and calculate sun direction.
    F32		Yaw = mDegToRad(mClampF(mTempAzimuth, 0, 359));
    F32		Pitch = mDegToRad(mClampF(mTempElevation, -360, +360));
    MathUtils::getVectorFromAngles(SunlightOffset, Yaw, Pitch);

    // Get Control Camera Eye Position.
    Point3F eyePos;
    MatrixF eye;
    GameConnection* conn = GameConnection::getConnectionToServer();
    conn->getControlCameraTransform(0, &eye);
    eye.getColumn(3, &eyePos);

    // Calculate new Sun Position.
    F32 Radius = getCurrentClientSceneGraph()->getVisibleDistance() * 0.999f;
    mSunlightPosition = eyePos + (Radius * SunlightOffset);
}

//------------------------------------------------------------------------------

void fxSunLight::InitialiseAnimation(void)
{
    // Debugging.
    mEnable = true;

    // Media.
    //
    // Texture Handles.
    mLocalFlareTextureHandle = NULL;
    mRemoteFlareTextureHandle = NULL;
    // Flare Texture Names.
    mLocalFlareTextureName = StringTable->insert("common/lighting/corona");
    mRemoteFlareTextureName = StringTable->insert("common/lighting/corona");

    // Sun Orbit.
    mSunAzimuth = 0.0f;
    mSunElevation = 30.0f;
    mLockToRealSun = true;

    // Flare.
    mFlareTP = true;
    mFlareColour.set(1, 1, 1);
    mFlareBrightness = 1.0f;
    mFlareSize = 1.0f;
    mLocalScale = 1.0f;
    mFadeTime = 0.1f;
    mBlendMode = 0;

    // Animation Options.
    mUseColour = false;
    mUseBrightness = false;
    mUseRotation = false;
    mUseSize = false;
    mUseAzimuth = false;
    mUseElevation = false;
    mLerpColour = true;
    mLerpBrightness = true;
    mLerpRotation = true;
    mLerpSize = true;
    mLerpAzimuth = true;
    mLerpElevation = true;
    mLinkFlareSize = false;
    mSingleColourKeys = true;

    // Animation Extents.
    mMinColour.set(0, 0, 0);
    mMaxColour.set(1, 1, 1);
    mMinBrightness = 0.0f;
    mMaxBrightness = 1.0f;
    mMinRotation = 0;
    mMaxRotation = 359;
    mMinSize = 0.5f;
    mMaxSize = 1.0f;
    mMinAzimuth = 0.0f;
    mMaxAzimuth = 359.0f;
    mMinElevation = -30.0f;
    mMaxElevation = 180.0f + 30.0f;

    // Animation Keys.
    mRedKeys = StringTable->insert("AZA");
    mGreenKeys = StringTable->insert("AZA");
    mBlueKeys = StringTable->insert("AZA");
    mBrightnessKeys = StringTable->insert("AZA");
    mRotationKeys = StringTable->insert("AZA");
    mSizeKeys = StringTable->insert("AZA");
    mAzimuthKeys = StringTable->insert("AZ");
    mElevationKeys = StringTable->insert("AZ");

    // Animation Times.
    mColourTime = 5.0f;
    mBrightnessTime = 5.0f;
    mRotationTime = 5.0f;
    mSizeTime = 5.0f;
    mAzimuthTime = 5.0f;
    mElevationTime = 5.0f;

    // Reset Local Flare Scale.
    mLocalFlareScale = 0.0f;

    // Reset Animation.
    ResetAnimation();
}

//------------------------------------------------------------------------------

void fxSunLight::ResetAnimation(void)
{
    // Check Animation Keys.
    CheckAnimationKeys();

    // Reset Times.
    mColourElapsedTime =
        mBrightnessElapsedTime =
        mRotationElapsedTime =
        mSizeElapsedTime =
        mAzimuthElapsedTime =
        mElevationElapsedTime = 0.0f;

    // Reset Last Animation Time.
    mLastRenderTime = Platform::getVirtualMilliseconds();

    // Check Flare Details.
    if (mBlendMode > 2) mBlendMode = 0;
}

//------------------------------------------------------------------------------

F32 fxSunLight::GetLerpKey(StringTableEntry Key, U32 PosFrom, U32 PosTo, F32 ValueFrom, F32 ValueTo, F32 Lerp)
{
    // Get Key at Selected Positions.
    char KeyFrameFrom = dToupper(*(Key + PosFrom)) - 65;
    char KeyFrameTo = dToupper(*(Key + PosTo)) - 65;

    // Calculate Range.
    F32 ValueRange = (ValueTo - ValueFrom) / 25.0f;
    // Calculate Key Lerp.
    F32 KeyFrameLerp = (KeyFrameTo - KeyFrameFrom) * Lerp;

    // Return Lerped Value.
    return ValueFrom + ((KeyFrameFrom + KeyFrameLerp) * ValueRange);
}

//------------------------------------------------------------------------------

U32 fxSunLight::CheckKeySyntax(StringTableEntry Key)
{
    // Return problem.
    if (!Key) return 0;

    // Copy KeyCheck.
    StringTableEntry KeyCheck = Key;

    // Give benefit of doubt!
    bool KeyValid = true;

    // Check Key-frame validity.
    do
    {
        if (dToupper(*KeyCheck) < 'A' && dToupper(*KeyCheck) > 'Z') KeyValid = false;

    } while (*(KeyCheck++));

    // Return result.
    if (KeyValid)
        return dStrlen(Key);
    else
        return 0;
}

//------------------------------------------------------------------------------

void fxSunLight::CheckAnimationKeys(void)
{
    // Check Key Validities.
    mRedKeysLength = CheckKeySyntax(mRedKeys);
    mGreenKeysLength = CheckKeySyntax(mGreenKeys);
    mBlueKeysLength = CheckKeySyntax(mBlueKeys);
    mBrightnessKeysLength = CheckKeySyntax(mBrightnessKeys);
    mRotationKeysLength = CheckKeySyntax(mRotationKeys);
    mSizeKeysLength = CheckKeySyntax(mSizeKeys);
    mAzimuthKeysLength = CheckKeySyntax(mAzimuthKeys);
    mElevationKeysLength = CheckKeySyntax(mElevationKeys);

    // Calculate Time Scales.
    if (mColourTime) mColourTimeScale = (mRedKeysLength - 1) / mColourTime;
    if (mBrightnessTime) mBrightnessTimeScale = (mBrightnessKeysLength - 1) / mBrightnessTime;
    if (mRotationTime) mRotationTimeScale = (mRotationKeysLength - 1) / mRotationTime;
    if (mSizeTime) mSizeTimeScale = (mSizeKeysLength - 1) / mSizeTime;
    if (mAzimuthTime) mAzimuthTimeScale = (mAzimuthKeysLength - 1) / mAzimuthTime;
    if (mElevationTime) mElevationTimeScale = (mElevationKeysLength - 1) / mElevationTime;
}

//------------------------------------------------------------------------------

bool fxSunLight::onAdd()
{
    // Add Parent.
    if (!Parent::onAdd()) return(false);

    // Set Default Object Box.
    mObjBox.min.set(-1e5, -1e5, -1e5);
    mObjBox.max.set(1e5, 1e5, 1e5);
    // Reset the World Box.
    resetWorldBox();
    // Set the Render Transform.
    setRenderTransform(mObjToWorld);

    // Add to Scene.
    addToScene();
    mAddedToScene = true;

    // Only on client.
    if (isClientObject())
    {
        // Fetch Textures.
        setFlareBitmaps(mLocalFlareTextureName, mRemoteFlareTextureName);
    }

    // Return OK.
    return(true);
}

//------------------------------------------------------------------------------

void fxSunLight::onRemove()
{
    // Remove from Scene.
    removeFromScene();
    mAddedToScene = false;

    // Only on client.
    if (isClientObject())
    {
        // Remove Texture References.
        mLocalFlareTextureHandle = NULL;
        mRemoteFlareTextureHandle = NULL;
    }

    // Do Parent.
    Parent::onRemove();
}

//------------------------------------------------------------------------------

void fxSunLight::inspectPostApply()
{
    // Reset Animation.
    ResetAnimation();

    // Set Parent.
    Parent::inspectPostApply();

    // Set Config Change Mask.
    setMaskBits(fxSunLightConfigChangeMask);
}

//------------------------------------------------------------------------------

bool fxSunLight::prepRenderImage(SceneState* state, const U32 stateKey, const U32 startZone,
    const bool modifyBaseZoneState)
{
    // No need to render if disabled.
    if (!mEnable) return false;

    // Return if last state.
    if (isLastState(state, stateKey)) return false;
    // Set Last State.
    setLastState(state, stateKey);

    // Is Object Rendered?
    if (state->isObjectRendered(this))
    {
        RenderInst* ri = gRenderInstManager.allocInst();
        ri->obj = this;
        ri->state = state;
        ri->type = RenderInstManager::RIT_Begin;
        gRenderInstManager.addInst(ri);

        RenderInst* ri2 = gRenderInstManager.allocInst();
        *ri2 = *ri;
        ri2->type = RenderInstManager::RIT_Object;
        gRenderInstManager.addInst(ri2);
    }

    // Are we locking to real sun and have not done it yet?
    if (mLockToRealSun /*&& !mDoneSunLock*/)
    {
        // Yes, so calculate real sun elevation/azimuth.

        // Do we have any lights?
        const LightInfo* sunlight = getCurrentClientSceneGraph()->getLightManager()->sgGetSpecialLight(LightManager::sgSunLightType);

        // Yes, so fetch sunlight ( always first light ).
        VectorF sunVector = -sunlight->mDirection;

        // Calculate Pitch/Yaw.
        F32 Yaw, Pitch;
        MathUtils::getAnglesFromVector(sunVector, Yaw, Pitch);

        // Set Elevation/Azimiuth.
        setSunElevation(mRadToDeg(Pitch));
        setSunAzimuth(mRadToDeg(Yaw));

        // Flag sun lock occurred.
        mDoneSunLock = true;
    }

    return false;
}

//------------------------------------------------------------------------------

void fxSunLight::renderObject(SceneState* state, RenderInst* ri)
{
    // Setup
    // -------------------------------------------------------------------------
     // Find if it's the Local or Remote Sun.
    bool LocalSun = ri->type == RenderInstManager::RIT_Object;

    // Update animations
    //
    // NOTE:-	We are doing this on the remote sun because then both suns will
    //			have the same animation positions.
    if (!LocalSun && !getCurrentClientSceneGraph()->isReflectPass())
    {
        // Yes, so calculate Elapsed Time.
        mElapsedTime = (F32)((Platform::getVirtualMilliseconds() - mLastRenderTime) / 1000.0f);

        // Reset Last Render Time.
        mLastRenderTime = Platform::getVirtualMilliseconds();

        // Animate Sun.
        AnimateSun(mElapsedTime);
    }

    // Cannot render without appropriate textures!
    if ((!mLocalFlareTextureHandle && LocalSun) ||
        (!mRemoteFlareTextureHandle && !LocalSun)) return;

    // Get the Control Object.
    GameConnection* conn = GameConnection::getConnectionToServer();
    if (!conn)
        return;
    ShapeBase* ControlObj = conn->getControlObject();

    // Fetch Eye Position.
    Point3F	eyePos;
    MatrixF	eye;
    ControlObj->getEyeTransform(&eye);
    eye.getColumn(3, &eyePos);

    // Calculate Billboard Radius (in world units) to be constant, independant of distance.
   // Takes into account distance, viewport size, and specified size in editor
    F32 BBRadius = (((mSunlightPosition - eyePos).len()) / (GFX->getViewport().extent.x / 640.0)) / 2;
    BBRadius *= mAnimationSize;

    // Is this the Local Sun?
    if (LocalSun)
    {
        BBRadius *= mLocalScale;

        if (!getCurrentClientSceneGraph()->isReflectPass())
        {
            // Yes, so do we have Line-of-sight?
            if (TestLOS(mSunlightPosition))
            {
                // Yes, so are we fully showing?
                if (mLocalFlareScale < 1.0f)
                {
                    // No, so calculate new scale.
                    mLocalFlareScale += mElapsedTime / mFadeTime;

                    // Check new scale.
                    if (mLocalFlareScale > 1.0f) mLocalFlareScale = 1.0f;
                }
            }
            else
            {
                // No, so are we fully hidden?
                if (mLocalFlareScale > 0.0f)
                {
                    // No, so calculate new scale.
                    mLocalFlareScale -= mElapsedTime / mFadeTime;

                    // Check new scale.
                    if (mLocalFlareScale < 0.0f)
                        mLocalFlareScale = 0.0f;

                    // No need to render
                    if (mLocalFlareScale == 0.0f)
                        return;
                }
            }
            BBRadius *= mLocalFlareScale;
        }
    }

    // Are we linking flare size?
    if (mLinkFlareSize)
    {
        // Yes, so Scale by LUMINANCE.
        BBRadius *= (mAnimationColour.red * 0.212671f) +
            (mAnimationColour.green * 0.715160f) +
            (mAnimationColour.blue * 0.072169f);
    }

    // Render
    // -------------------------------------------------------------------------
    GFX->setBaseRenderState();

    // Setup Alpha
    GFX->setAlphaTestEnable(true);
    GFX->setAlphaRef(1);
    GFX->setAlphaFunc(GFXCmpGreaterEqual);

    // Select appropriate depth test for sun.
    if (LocalSun)
    {
        GFX->setZEnable(false);
        GFX->setZWriteEnable(false);
    }
    else
    {
        GFX->setZEnable(true);
        GFX->setZWriteEnable(false);
    }

    // Select User Blend Mode.
    GFX->setAlphaBlendEnable(true);
    switch (mBlendMode)
    {
    case 0:
        GFX->setSrcBlend(GFXBlendSrcAlpha);
        GFX->setDestBlend(GFXBlendOne);
        break;
    case 1:
        GFX->setSrcBlend(GFXBlendSrcAlpha);
        GFX->setDestBlend(GFXBlendInvSrcAlpha);
        break;
    case 2:
        GFX->setSrcBlend(GFXBlendOne);
        GFX->setDestBlend(GFXBlendOne);
        break;

    default:
        // Duh, User error.
        Con::printf("fxSunLight: Invalid Blend Mode Selected!");
        GFX->setSrcBlend(GFXBlendSrcAlpha);
        GFX->setDestBlend(GFXBlendOne);
        break;
    }

    // Modulate Texture.
    GFX->setTextureStageColorOp(0, GFXTOPModulate);
    GFX->setTextureStageColorArg1(0, GFXTATexture);
    GFX->setTextureStageColorArg2(0, GFXTADiffuse);

    GFX->setTextureStageAlphaOp(0, GFXTOPModulate);
    GFX->setTextureStageAlphaArg1(0, GFXTATexture);
    GFX->setTextureStageAlphaArg2(0, GFXTADiffuse);

    // Initialize points with basic info
    Point3F points[4];
    points[0] = Point3F(-BBRadius, 0.0, -BBRadius);
    points[1] = Point3F(BBRadius, 0.0, -BBRadius);
    points[2] = Point3F(BBRadius, 0.0, BBRadius);
    points[3] = Point3F(-BBRadius, 0.0, BBRadius);

    // Get info we need to adjust points
    MatrixF camView = GFX->getWorldMatrix();
    camView.transpose();

    F32 sy, cy;
    mSinCos(mAnimationRotation, sy, cy);

    // Finalize points
    for (int i = 0; i < 4; i++)
    {
        // rotate
        points[i].set(cy * points[i].x - sy * points[i].z, 0.0, sy * points[i].x + cy * points[i].z);
        // align with camera
        camView.mulP(points[i]);
        // offset
        points[i] += mSunlightPosition;
    }

    // Draw it
    PrimBuild::color3f(mAnimationColour.red, mAnimationColour.green, mAnimationColour.blue);
    GFX->setTexture(0, LocalSun ? mLocalFlareTextureHandle : mRemoteFlareTextureHandle);

    PrimBuild::begin(GFXTriangleFan, 4);
    PrimBuild::texCoord2f(0, 0);
    PrimBuild::vertex3fv(points[0]);
    PrimBuild::texCoord2f(1, 0);
    PrimBuild::vertex3fv(points[1]);
    PrimBuild::texCoord2f(1, 1);
    PrimBuild::vertex3fv(points[2]);
    PrimBuild::texCoord2f(0, 1);
    PrimBuild::vertex3fv(points[3]);
    PrimBuild::end();

    // Exit
    GFX->setZWriteEnable(true);
    GFX->setZEnable(true);
    GFX->setAlphaTestEnable(false);
    GFX->setBaseRenderState();
}

//------------------------------------------------------------------------------

bool fxSunLight::TestLOS(const Point3F& ObjectPosition)
{
    // Valid Control Object types (for now).
    const U32 ObjectMask = PlayerObjectType | VehicleObjectType | CameraObjectType;

    // Get Server Connection.
    GameConnection* conn = GameConnection::getConnectionToServer();
    // Problem?
    if (!conn) return false;

    // Do a check to see if it is on screen
    Point3F in = ObjectPosition; Point3F out; RectI viewport = GFX->getViewport();
    MatrixF wdMat = GFX->getWorldMatrix();
    wdMat.mul(GFX->getViewMatrix());
    MatrixF pjMat = GFX->getProjectionMatrix();
    MathUtils::projectWorldToScreen(in,out,viewport,wdMat,pjMat);

    if (out.z < 0.0 || out.z > 1.0 || out.x > viewport.extent.x || out.x < 0 || out.y > viewport.extent.y || out.y < 0)
        return false;

    // Get the Control Object.
    ShapeBase* ControlObj = conn->getControlObject();
    // Valid Control Objects?
    if (!ControlObj || !(ControlObj->getType() & ObjectMask)) return false;
    // Kill flare if Third-person not available.
    if (!ControlObj->isFirstPerson() && !mFlareTP) return false;

    // Fetch Eye Position.
    Point3F eyePos;
    MatrixF eye;
    conn->getControlCameraTransform(0, &eye);
    eye.getColumn(3, &eyePos);

    // Get our object center.
    Point3F endPos = ObjectPosition;

    // LOS Mask.
    static U32 losMask = STATIC_COLLISION_MASK |
        ShapeBaseObjectType |
        StaticTSObjectType |
        ItemObjectType |
        PlayerObjectType;

    // Stop Collisions with our Control Object (in first person).
    if (ControlObj->isFirstPerson()) ControlObj->disableCollision();

    // Store old Object Box.
    Box3F OldObjBox = mObjBox;

    // Set LOS Test Object Box.
    mObjBox.min.set(-0.5, -0.5, -0.5);
    mObjBox.max.set(0.5, 0.5, 0.5);
    // Reset the World Box.
    resetWorldBox();
    setRenderTransform(mObjToWorld);

    // Perform a ray cast on the client container database.
    RayInfo info;
    bool los = !getCurrentClientContainer()->castRay(eyePos, endPos, losMask, &info);

    // Restore old Object Box.
    mObjBox = OldObjBox;
    // Reset the World Box.
    resetWorldBox();
    setRenderTransform(mObjToWorld);

    // Continue Collisions with our Control Object (in first person).
    if (ControlObj->isFirstPerson()) ControlObj->enableCollision();

    // Return LOS result.
    return los;
};

//------------------------------------------------------------------------------

U32 fxSunLight::packUpdate(NetConnection* con, U32 mask, BitStream* stream)
{
    // Pack Parent.
    U32 retMask = Parent::packUpdate(con, mask, stream);

    // Write Config Change Flag.
    if (stream->writeFlag(mask & fxSunLightConfigChangeMask))
    {
        // Position.
        stream->writeAffineTransform(mObjToWorld);

        // Debugging.
        stream->write(mEnable);

        // Media.
        stream->writeString(mLocalFlareTextureName);
        stream->writeString(mRemoteFlareTextureName);

        // Sun Orbit.
        stream->write(mSunAzimuth);
        stream->write(mSunElevation);

        // Flare.
        stream->write(mFlareTP);
        stream->write(mFlareColour);
        stream->write(mFlareBrightness);
        stream->write(mFlareSize);
        stream->write(mLocalScale);
        stream->write(mFadeTime);
        stream->write(mBlendMode);

        // Animation Options.
        stream->writeFlag(mUseColour);
        stream->writeFlag(mUseBrightness);
        stream->writeFlag(mUseRotation);
        stream->writeFlag(mUseSize);
        stream->writeFlag(mUseAzimuth);
        stream->writeFlag(mUseElevation);
        stream->writeFlag(mLerpColour);
        stream->writeFlag(mLerpBrightness);
        stream->writeFlag(mLerpRotation);
        stream->writeFlag(mLerpSize);
        stream->writeFlag(mLerpAzimuth);
        stream->writeFlag(mLerpElevation);
        stream->writeFlag(mLinkFlareSize);
        stream->writeFlag(mSingleColourKeys);

        // Animation Extents.
        stream->write(mMinColour);
        stream->write(mMaxColour);
        stream->write(mMinBrightness);
        stream->write(mMaxBrightness);
        stream->write(mMinRotation);
        stream->write(mMaxRotation);
        stream->write(mMinSize);
        stream->write(mMaxSize);
        stream->write(mMinAzimuth);
        stream->write(mMaxAzimuth);
        stream->write(mMinElevation);
        stream->write(mMaxElevation);

        // Animation Keys.
        stream->writeString(mRedKeys);
        stream->writeString(mGreenKeys);
        stream->writeString(mBlueKeys);
        stream->writeString(mBrightnessKeys);
        stream->writeString(mRotationKeys);
        stream->writeString(mSizeKeys);
        stream->writeString(mAzimuthKeys);
        stream->writeString(mElevationKeys);

        // Animation Times.
        stream->write(mColourTime);
        stream->write(mBrightnessTime);
        stream->write(mRotationTime);
        stream->write(mSizeTime);
        stream->write(mAzimuthTime);
        stream->write(mElevationTime);
    }

    // Were done ...
    return(retMask);
}

//------------------------------------------------------------------------------

void fxSunLight::unpackUpdate(NetConnection* con, BitStream* stream)
{
    // Unpack Parent.
    Parent::unpackUpdate(con, stream);

    // Read Config Change Details?
    if (stream->readFlag())
    {
        MatrixF		ObjectMatrix;

        // Position.
        stream->readAffineTransform(&ObjectMatrix);

        // Debugging.
        stream->read(&mEnable);

        // Media.
        const char* Local = stream->readSTString();
        const char* Remote = stream->readSTString();
        setFlareBitmaps(Local, Remote);

        // Sun Orbit.
        stream->read(&mSunAzimuth);
        stream->read(&mSunElevation);

        // Flare.
        stream->read(&mFlareTP);
        stream->read(&mFlareColour);
        stream->read(&mFlareBrightness);
        stream->read(&mFlareSize);
        stream->read(&mLocalScale);
        stream->read(&mFadeTime);
        stream->read(&mBlendMode);

        // Animation Options.
        mUseColour = stream->readFlag();
        mUseBrightness = stream->readFlag();
        mUseRotation = stream->readFlag();
        mUseSize = stream->readFlag();
        mUseAzimuth = stream->readFlag();
        mUseElevation = stream->readFlag();
        mLerpColour = stream->readFlag();
        mLerpBrightness = stream->readFlag();
        mLerpRotation = stream->readFlag();
        mLerpSize = stream->readFlag();
        mLerpAzimuth = stream->readFlag();
        mLerpElevation = stream->readFlag();
        mLinkFlareSize = stream->readFlag();
        mSingleColourKeys = stream->readFlag();

        // Animation Extents.
        stream->read(&mMinColour);
        stream->read(&mMaxColour);
        stream->read(&mMinBrightness);
        stream->read(&mMaxBrightness);
        stream->read(&mMinRotation);
        stream->read(&mMaxRotation);
        stream->read(&mMinSize);
        stream->read(&mMaxSize);
        stream->read(&mMinAzimuth);
        stream->read(&mMaxAzimuth);
        stream->read(&mMinElevation);
        stream->read(&mMaxElevation);

        // Animation Keys.
        mRedKeys = stream->readSTString();
        mGreenKeys = stream->readSTString();
        mBlueKeys = stream->readSTString();
        mBrightnessKeys = stream->readSTString();
        mRotationKeys = stream->readSTString();
        mSizeKeys = stream->readSTString();
        mAzimuthKeys = stream->readSTString();
        mElevationKeys = stream->readSTString();

        // Animation Times.
        stream->read(&mColourTime);
        stream->read(&mBrightnessTime);
        stream->read(&mRotationTime);
        stream->read(&mSizeTime);
        stream->read(&mAzimuthTime);
        stream->read(&mElevationTime);

        // Set Transform.
        setTransform(ObjectMatrix);

        // Reset Animation.
        ResetAnimation();
    }
}