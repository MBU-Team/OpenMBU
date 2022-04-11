//-----------------------------------------------------------------------------
// Torque Shader Engine
// 
// Written by Melvyn May, Started on 4th May 2002.
//
// "My code is written for the Torque community, so do your worst with it,
//	just don't rip-it-off and call it your own without even thanking me".
//
//	- Melv.
//
//-----------------------------------------------------------------------------

#include "console/consoleTypes.h"
#include "gfx/gfxDevice.h"
#include "core/bitStream.h"
#include "game/gameConnection.h"
#include "game/gameBase.h"
#include "sceneGraph/sceneState.h"
#include "sceneGraph/sceneGraph.h"
#include "fxLight.h"
#include "gfx/primBuilder.h"
#include "math/mathUtils.h"

//------------------------------------------------------------------------------
//
//	Put this in /example/common/editor/EditorGui.cs in [function Creator::init( %this )]
//
//   %Environment_Item[<next item-index in list>] = "fxLight";  <-- ADD THIS.
//
//------------------------------------------------------------------------------
//
//	Put the function in /example/common/editor/ObjectBuilderGui.gui [around line 458] ...
//
//	function ObjectBuilderGui::buildfxLight(%this)
//	{
//		%this.className = "fxLight";
//		%this.addField("dataBlock", "TypeDataBlock", "fxLight Data", "fxLightData");
//		%this.process();
//	}
//
//------------------------------------------------------------------------------
//
//	Put 'fxLightIcon.jpg' in /example/common/editor/ directory.
//	Put the example 'corona.png' in the "/example/common/lighting/" directory.
//
//------------------------------------------------------------------------------

extern bool gEditingMission;

//------------------------------------------------------------------------------

//IMPLEMENT_GETDATATYPE(fxLightData)
//IMPLEMENT_SETDATATYPE(fxLightData)
IMPLEMENT_CO_DATABLOCK_V1(fxLightData);
IMPLEMENT_CO_NETOBJECT_V1(fxLight);

//------------------------------------------------------------------------------

fxLightData::fxLightData()
{
    // Datablock Light.
    mLightOn = true;
    mRadius = 10.0f;
    mBrightness = 1.0f;
    mColour.set(1, 1, 1);

    // Datablock Sun-Light.
    // Datablock Flare.
    mFlareOn = false;
    mFlareTP = true;
    mFlareTextureName = StringTable->insert("");
    mConstantSizeOn = false;
    mConstantSize = 1.0f;
    mNearSize = 3.0f;
    mFarSize = 0.5f;
    mNearDistance = 10.0f;
    mFarDistance = 30.0f;
    mFadeTime = 0.1f;
    mFlareColour.set(1, 1, 1);
    mBlendMode = 0;

    // Datablock Animation.
    mMinColour.set(0, 0, 0);
    mMaxColour.set(1, 1, 1);
    mMinBrightness = 0.0f;
    mMaxBrightness = 1.0f;
    mMinRadius = 0.1f;
    mMaxRadius = 20.0f;
    mStartOffset.set(-5, 0, 0);
    mEndOffset.set(+5, 0, 0);
    mMinRotation = 0;
    mMaxRotation = 359;
    mSingleColourKeys = true;
    mRedKeys = StringTable->insert("AZA");
    mGreenKeys = StringTable->insert("AZA");
    mBlueKeys = StringTable->insert("AZA");
    mBrightnessKeys = StringTable->insert("AZA");
    mRadiusKeys = StringTable->insert("AZA");
    mOffsetKeys = StringTable->insert("AZA");
    mRotationKeys = StringTable->insert("AZA");
    mColourTime = 5.0f;
    mBrightnessTime = 5.0f;
    mRadiusTime = 5.0f;
    mOffsetTime = 5.0f;
    mRotationTime = 5.0f;
    mLerpColour = true;
    mLerpBrightness = true;
    mLerpRadius = true;
    mLerpOffset = true;
    mLerpRotation = true;
    mUseColour = false;
    mUseBrightness = false;
    mUseRadius = false;
    mUseOffsets = false;
    mUseRotation = false;
    mLinkFlare = true;
    mLinkFlareSize = false;
}

//------------------------------------------------------------------------------

void fxLightData::initPersistFields()
{
    Parent::initPersistFields();


    // Datablock Light.
    addField("LightOn", TypeBool, Offset(mLightOn, fxLightData));
    addField("Radius", TypeF32, Offset(mRadius, fxLightData));
    addField("Brightness", TypeF32, Offset(mBrightness, fxLightData));
    addField("Colour", TypeColorF, Offset(mColour, fxLightData));

    // Flare.
    addField("FlareOn", TypeBool, Offset(mFlareOn, fxLightData));
    addField("FlareTP", TypeBool, Offset(mFlareTP, fxLightData));
    addField("FlareBitmap", TypeFilename, Offset(mFlareTextureName, fxLightData));
    addField("FlareColour", TypeColorF, Offset(mFlareColour, fxLightData));
    addField("ConstantSizeOn", TypeBool, Offset(mConstantSizeOn, fxLightData));
    addField("ConstantSize", TypeF32, Offset(mConstantSize, fxLightData));
    addField("NearSize", TypeF32, Offset(mNearSize, fxLightData));
    addField("FarSize", TypeF32, Offset(mFarSize, fxLightData));
    addField("NearDistance", TypeF32, Offset(mNearDistance, fxLightData));
    addField("FarDistance", TypeF32, Offset(mFarDistance, fxLightData));
    addField("FadeTime", TypeF32, Offset(mFadeTime, fxLightData));
    addField("BlendMode", TypeS32, Offset(mBlendMode, fxLightData));

    // Datablock Animation.
    addField("AnimColour", TypeBool, Offset(mUseColour, fxLightData));
    addField("AnimBrightness", TypeBool, Offset(mUseBrightness, fxLightData));
    addField("AnimRadius", TypeBool, Offset(mUseRadius, fxLightData));
    addField("AnimOffsets", TypeBool, Offset(mUseOffsets, fxLightData));
    addField("AnimRotation", TypeBool, Offset(mUseRotation, fxLightData));
    addField("LinkFlare", TypeBool, Offset(mLinkFlare, fxLightData));
    addField("LinkFlareSize", TypeBool, Offset(mLinkFlareSize, fxLightData));
    addField("MinColour", TypeColorF, Offset(mMinColour, fxLightData));
    addField("MaxColour", TypeColorF, Offset(mMaxColour, fxLightData));
    addField("MinBrightness", TypeF32, Offset(mMinBrightness, fxLightData));
    addField("MaxBrightness", TypeF32, Offset(mMaxBrightness, fxLightData));
    addField("MinRadius", TypeF32, Offset(mMinRadius, fxLightData));
    addField("MaxRadius", TypeF32, Offset(mMaxRadius, fxLightData));
    addField("StartOffset", TypePoint3F, Offset(mStartOffset, fxLightData));
    addField("EndOffset", TypePoint3F, Offset(mEndOffset, fxLightData));
    addField("MinRotation", TypeF32, Offset(mMinRotation, fxLightData));
    addField("MaxRotation", TypeF32, Offset(mMaxRotation, fxLightData));
    addField("SingleColourKeys", TypeBool, Offset(mSingleColourKeys, fxLightData));
    addField("RedKeys", TypeString, Offset(mRedKeys, fxLightData));
    addField("GreenKeys", TypeString, Offset(mGreenKeys, fxLightData));
    addField("BlueKeys", TypeString, Offset(mBlueKeys, fxLightData));
    addField("BrightnessKeys", TypeString, Offset(mBrightnessKeys, fxLightData));
    addField("RadiusKeys", TypeString, Offset(mRadiusKeys, fxLightData));
    addField("OffsetKeys", TypeString, Offset(mOffsetKeys, fxLightData));
    addField("RotationKeys", TypeString, Offset(mRotationKeys, fxLightData));
    addField("ColourTime", TypeF32, Offset(mColourTime, fxLightData));
    addField("BrightnessTime", TypeF32, Offset(mBrightnessTime, fxLightData));
    addField("RadiusTime", TypeF32, Offset(mRadiusTime, fxLightData));
    addField("OffsetTime", TypeF32, Offset(mOffsetTime, fxLightData));
    addField("RotationTime", TypeF32, Offset(mRotationTime, fxLightData));
    addField("LerpColour", TypeBool, Offset(mLerpColour, fxLightData));
    addField("LerpBrightness", TypeBool, Offset(mLerpBrightness, fxLightData));
    addField("LerpRadius", TypeBool, Offset(mLerpRadius, fxLightData));
    addField("LerpOffset", TypeBool, Offset(mLerpOffset, fxLightData));
    addField("LerpRotation", TypeBool, Offset(mLerpRotation, fxLightData));
}

//------------------------------------------------------------------------------

bool fxLightData::onAdd()
{
    if (Parent::onAdd() == false)
        return false;

    return true;
}

//------------------------------------------------------------------------------

void fxLightData::packData(BitStream* stream)
{
    // Parent packing.
    Parent::packData(stream);

    // Datablock Light.
    stream->write(mLightOn);
    stream->write(mRadius);
    stream->write(mBrightness);
    stream->write(mColour);

    // Datablock Flare.
    stream->writeString(mFlareTextureName);
    stream->write(mFlareColour);
    stream->write(mFlareOn);
    stream->write(mFlareTP);
    stream->write(mConstantSizeOn);
    stream->write(mConstantSize);
    stream->write(mNearSize);
    stream->write(mFarSize);
    stream->write(mNearDistance);
    stream->write(mFarDistance);
    stream->write(mFadeTime);
    stream->write(mBlendMode);

    // Datablock Animation.
    stream->writeFlag(mUseColour);
    stream->writeFlag(mUseBrightness);
    stream->writeFlag(mUseRadius);
    stream->writeFlag(mUseOffsets);
    stream->writeFlag(mUseRotation);
    stream->writeFlag(mLinkFlare);
    stream->writeFlag(mLinkFlareSize);
    stream->write(mMinColour);
    stream->write(mMaxColour);
    stream->write(mMinBrightness);
    stream->write(mMaxBrightness);
    stream->write(mMinRadius);
    stream->write(mMaxRadius);
    stream->write(mStartOffset.x);
    stream->write(mStartOffset.y);
    stream->write(mStartOffset.z);
    stream->write(mEndOffset.x);
    stream->write(mEndOffset.y);
    stream->write(mEndOffset.z);
    stream->write(mMinRotation);
    stream->write(mMaxRotation);
    stream->writeFlag(mSingleColourKeys);
    stream->writeString(mRedKeys);
    stream->writeString(mGreenKeys);
    stream->writeString(mBlueKeys);
    stream->writeString(mBrightnessKeys);
    stream->writeString(mRadiusKeys);
    stream->writeString(mOffsetKeys);
    stream->writeString(mRotationKeys);
    stream->write(mColourTime);
    stream->write(mBrightnessTime);
    stream->write(mRadiusTime);
    stream->write(mOffsetTime);
    stream->write(mRotationTime);
    stream->writeFlag(mLerpColour);
    stream->writeFlag(mLerpBrightness);
    stream->writeFlag(mLerpRadius);
    stream->writeFlag(mLerpOffset);
    stream->writeFlag(mLerpRotation);
}

//------------------------------------------------------------------------------

void fxLightData::unpackData(BitStream* stream)
{
    // Parent unpacking.
    Parent::unpackData(stream);

    // Datablock Light.
    stream->read(&mLightOn);
    stream->read(&mRadius);
    stream->read(&mBrightness);
    stream->read(&mColour);

    // Flare.
    mFlareTextureName = StringTable->insert(stream->readSTString());
    stream->read(&mFlareColour);
    stream->read(&mFlareOn);
    stream->read(&mFlareTP);
    stream->read(&mConstantSizeOn);
    stream->read(&mConstantSize);
    stream->read(&mNearSize);
    stream->read(&mFarSize);
    stream->read(&mNearDistance);
    stream->read(&mFarDistance);
    stream->read(&mFadeTime);
    stream->read(&mBlendMode);

    // Datablock Animation.
    mUseColour = stream->readFlag();
    mUseBrightness = stream->readFlag();
    mUseRadius = stream->readFlag();
    mUseOffsets = stream->readFlag();
    mUseRotation = stream->readFlag();
    mLinkFlare = stream->readFlag();
    mLinkFlareSize = stream->readFlag();
    stream->read(&mMinColour);
    stream->read(&mMaxColour);
    stream->read(&mMinBrightness);
    stream->read(&mMaxBrightness);
    stream->read(&mMinRadius);
    stream->read(&mMaxRadius);
    stream->read(&mStartOffset.x);
    stream->read(&mStartOffset.y);
    stream->read(&mStartOffset.z);
    stream->read(&mEndOffset.x);
    stream->read(&mEndOffset.y);
    stream->read(&mEndOffset.z);
    stream->read(&mMinRotation);
    stream->read(&mMaxRotation);
    mSingleColourKeys = stream->readFlag();
    mRedKeys = stream->readSTString();
    mGreenKeys = stream->readSTString();
    mBlueKeys = stream->readSTString();
    mBrightnessKeys = stream->readSTString();
    mRadiusKeys = stream->readSTString();
    mOffsetKeys = stream->readSTString();
    mRotationKeys = stream->readSTString();
    stream->read(&mColourTime);
    stream->read(&mBrightnessTime);
    stream->read(&mRadiusTime);
    stream->read(&mOffsetTime);
    stream->read(&mRotationTime);
    mLerpColour = stream->readFlag();
    mLerpBrightness = stream->readFlag();
    mLerpRadius = stream->readFlag();
    mLerpOffset = stream->readFlag();
    mLerpRotation = stream->readFlag();
}

//------------------------------------------------------------------------------



//------------------------------------------------------------------------------
// Class: fxLight
//------------------------------------------------------------------------------

fxLight::fxLight()
{
    // Setup NetObject.
    mTypeMask |= StaticObjectType | StaticTSObjectType | StaticRenderedObjectType;
    mNetFlags.set(Ghostable | ScopeAlways);
    mAddedToScene = false;

    // Reset DataBlock.
    mDataBlock = NULL;

    // Reset Attachment stuff.
    mAttached = false;//mAttachWait = mAttachValid = false;
    //mpAttachedObject = NULL;

    // Initialise Animation.
    InitialiseAnimation();

    // Set Light Type.
    mLight.mType = LightInfo::Point;

    mountPoint = 0;
    mountTransform.identity();
}

//------------------------------------------------------------------------------

fxLight::~fxLight()
{
}

//------------------------------------------------------------------------------

void fxLight::initPersistFields()
{
    // Initialise parents' persistent fields.
    Parent::initPersistFields();

    // Add our own persistent fields.
    addField("Enable", TypeBool, Offset(mEnable, fxLight));
    addField("IconSize", TypeF32, Offset(mIconSize, fxLight));
}

//------------------------------------------------------------------------------

void fxLight::processTick(const Move* m)
{
    // Process Parent.
    Parent::processTick(m);

    // Are we attached?	
    /*if (isServerObject() && mAttachWait)
    {
        //Con::printf("fxLight: Checking Object...");
        // Get Attached Object.
        GameBase* Obj = getProcessAfter();

        // Write Object Attach/Detach Flag.
        if (Obj != NULL)
        {
            // Get Server Connection.
            GameConnection* con = GameConnection::getLocalClientConnection();
            // Problem?
            if (!con)
            {
                Con::printf("fxLight: Could not get Game Connection!!!");
                return;
            }
            S32 ghostIndex = con->getGhostIndex(Obj);
            if (ghostIndex == -1)
            {
                return;
            //Con::printf("fxLight: Attached but no GHOST available yet...");
            }
            else
            {
                Con::printf("fxLight: Attached and found GHOST!!!  YIPEE YEAY!!!!");

                // Finished waiting for attachment.
                mAttachWait = false;
                // Attachment is now valid to send.
                mAttachValid = true;
                // Send to the client.
                setMaskBits(fxLightAttachChange);
            }
        }
        else
        {
            //Con::printf("fxLight: Attached but no object available, stopping search.");

            // Finished waiting for attachment.
            mAttachWait = false;
        }
    }*/
}

//------------------------------------------------------------------------------

void fxLight::setEnable(bool Status)
{
    // Set Attribute.
    mEnable = Status;
    // Set Config Change Mask.
    if (isServerObject()) setMaskBits(fxLightConfigChangeMask);
}

void fxLight::setFlareBitmap(const char* Name)
{
    // Set Flare Texture Name.
    mDataBlock->mFlareTextureName = StringTable->insert(Name);

    // Only let the client load an actual texture.
    if (isClientObject())
    {
        // Reset existing flare texture.
        mFlareTextureHandle = NULL;

        // Got a nice flare texture?
        if (*mDataBlock->mFlareTextureName)
        {
            // Yes, so load it.
            mFlareTextureHandle = GFXTexHandle(mDataBlock->mFlareTextureName, &GFXDefaultStaticDiffuseProfile);
        }
    }

    // Set Config Change Mask.
    if (isServerObject()) setMaskBits(fxLightConfigChangeMask);
}

void fxLight::reset(void)
{
    // Reset Animation.
    ResetAnimation();
    // Set Config Change Mask.
    if (isServerObject()) setMaskBits(fxLightConfigChangeMask);
}

/*void fxLight::attachToObject(const char* ObjectName)
{
    // Find the Selected Object.
    GameBase *Obj = dynamic_cast<GameBase*>(Sim::findObject(ObjectName));

    // Check we found it.
    if (!Obj)
    {
        Con::warnf("Couldn't find %s object to attach to!", ObjectName);
        return;
    }

    // If the object has a name then output attachment.
    //if (getName()) Con::printf("%s attached to object %s.", getName(), ObjectName);

    // set-up dependency.
    processAfter(Obj);

    // Make sure we know if it's deleted.
    deleteNotify(Obj);

    // Set Config Change Mask.
    if (isServerObject()) mAttachWait = true;
}

void fxLight::detachFromObject(void)
{
    // Return if nothing to do!
    if (!getProcessAfter()) return;

    // If the object has a name then output detachment.
    //if (getName()) Con::printf("%s detached from object %s.", getName(), getProcessAfter()->getName());

    // Don't need delete notification now.
    clearNotify(getProcessAfter());

    // Clear dependency.
    clearProcessAfter();

    // Set Config Change Mask.
    if (isServerObject())
    {
        // Signal Attach is no invalid.
        mAttachValid = false;
        // Tell the client it's all over.
        setMaskBits(fxLightAttachChange);
    }
}*/

//------------------------------------------------------------------------------

ConsoleMethod(fxLight, setEnable, void, 3, 3, "(bool enabled)")
{
    object->setEnable(dAtob(argv[2]));
}

ConsoleMethod(fxLight, reset, void, 2, 2, "() Reset the light.")
{
    object->reset();
}

/*ConsoleMethod( fxLight, attachToObject, void, 3, 3, "(SimObject obj) Attach to the SimObject obj.")
{
    object->attachToObject(argv[2]);
}

ConsoleMethod( fxLight, detachFromObject, void, 2, 2, "() Detach from the object previously set by attachToObject.")
{
    object->detachFromObject();
}*/

//------------------------------------------------------------------------------

void fxLight::registerLights(LightManager* lightManager, bool lightingScene)
{
    // Exit if disabled.
    if (!mEnable) return;

    // Animate Light.
    AnimateLight();

    // Return if lighting scene or light off.
    if (lightingScene || !mDataBlock->mLightOn) return;

    // Setup light frame.
    mLight.mPos = mAnimationPosition;
    mLight.mRadius = mAnimationRadius;
    mLight.mColor = mAnimationColour;

    // Add light to light manager.
    lightManager->sgRegisterGlobalLight(&mLight, this, false);
}

//------------------------------------------------------------------------------

void fxLight::AnimateLight(void)
{
    // Calculate Elapsed Time.
    F32 mElapsedTime = (F32)((Platform::getRealMilliseconds() - mLastAnimateTime) / 1000.0f);

    // Reset Last Animation Time.
    mLastAnimateTime = Platform::getRealMilliseconds();


    mDataBlock->mColourTime = getMax(mDataBlock->mColourTime, 0.1f);
    mDataBlock->mBrightnessTime = getMax(mDataBlock->mBrightnessTime, 0.1f);
    mDataBlock->mRadiusTime = getMax(mDataBlock->mRadiusTime, 0.1f);
    mDataBlock->mOffsetTime = getMax(mDataBlock->mOffsetTime, 0.1f);
    mDataBlock->mRotationTime = getMax(mDataBlock->mRotationTime, 0.1f);


    // ********************************************
    // Calculate Colour.
    // ********************************************

    // Animating Colour?
    if (mDataBlock->mUseColour)
    {
        U32 PosFrom;
        U32 PosTo;
        F32	LerpFactor;

        // Yes, so adjust time-base.
        mColourElapsedTime += mElapsedTime;

        // Adjust to Bounds.
        while (mColourElapsedTime > mDataBlock->mColourTime) { mColourElapsedTime -= mDataBlock->mColourTime; };

        // Scale Time.
        F32 ScaledTime = mColourElapsedTime * mColourTimeScale;

        // Calculate Position From.
        PosFrom = mFloor(ScaledTime);

        // Are we Lerping?
        if (mDataBlock->mLerpColour)
        {
            // Yes, so calculate Position To.
            PosTo = mCeil(ScaledTime);

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
            if (mDataBlock->mSingleColourKeys)
            {
                // Yes, so calculate rgb from red keys.
                mAnimationColour.red = GetLerpKey(mDataBlock->mRedKeys, PosFrom, PosTo, mDataBlock->mMinColour.red, mDataBlock->mMaxColour.red, LerpFactor);
                mAnimationColour.green = GetLerpKey(mDataBlock->mRedKeys, PosFrom, PosTo, mDataBlock->mMinColour.green, mDataBlock->mMaxColour.green, LerpFactor);
                mAnimationColour.blue = GetLerpKey(mDataBlock->mRedKeys, PosFrom, PosTo, mDataBlock->mMinColour.blue, mDataBlock->mMaxColour.blue, LerpFactor);

                // Flag RGB Set.
                RedSet = GreenSet = BlueSet = true;
            }
            else
            {
                // No, so calculate Red only.
                mAnimationColour.red = GetLerpKey(mDataBlock->mRedKeys, PosFrom, PosTo, mDataBlock->mMinColour.red, mDataBlock->mMaxColour.red, LerpFactor);

                // Flag Red Set.
                RedSet = true;
            }
        }

        // ********************************************
        // Green Keys.
        // ********************************************
        if (!mDataBlock->mSingleColourKeys && mGreenKeysLength)
        {
            // Calculate Green.
            mAnimationColour.green = GetLerpKey(mDataBlock->mGreenKeys, PosFrom, PosTo, mDataBlock->mMinColour.green, mDataBlock->mMaxColour.green, LerpFactor);

            // Flag Green Set.
            GreenSet = true;
        }

        // ********************************************
        // Blue Keys.
        // ********************************************
        if (!mDataBlock->mSingleColourKeys && mBlueKeysLength)
        {
            // Calculate Blue.
            mAnimationColour.blue = GetLerpKey(mDataBlock->mGreenKeys, PosFrom, PosTo, mDataBlock->mMinColour.blue, mDataBlock->mMaxColour.blue, LerpFactor);

            // Flag Blue Set.
            BlueSet = true;
        }

        // Set to static colour if we failed to set RGB correctly.
        if (!RedSet || !GreenSet || !BlueSet) mAnimationColour = mDataBlock->mColour;
    }
    else
    {
        // No, so set to static Colour.
        mAnimationColour = mDataBlock->mColour;
    }


    // ********************************************
    // Calculate Brightness.
    // ********************************************

    // Animating Brightness?
    if (mDataBlock->mUseBrightness)
    {
        U32 PosFrom;
        U32 PosTo;
        F32	LerpFactor;

        // Yes, so adjust time-base.
        mBrightnessElapsedTime += mElapsedTime;

        // Adjust to Bounds.
        while (mBrightnessElapsedTime > mDataBlock->mBrightnessTime) { mBrightnessElapsedTime -= mDataBlock->mBrightnessTime; };

        // Scale Time.
        F32 ScaledTime = mBrightnessElapsedTime * mBrightnessTimeScale;

        // Calculate Position From.
        PosFrom = mFloor(ScaledTime);

        // Are we Lerping?
        if (mDataBlock->mLerpBrightness)
        {
            // Yes, so calculate Position To.
            PosTo = mCeil(ScaledTime);

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
            mAnimationBrightness = GetLerpKey(mDataBlock->mBrightnessKeys, PosFrom, PosTo, mDataBlock->mMinBrightness, mDataBlock->mMaxBrightness, LerpFactor);
        }
        else
        {
            // Set to static brightness if we failed to set Brightness correctly.
            mAnimationBrightness = mDataBlock->mBrightness;
        }
    }
    else
    {
        // No, so set to static Brightness.
        mAnimationBrightness = mDataBlock->mBrightness;
    }

    // Adjust Colour by Brightness.
    mAnimationColour *= ColorF(mAnimationBrightness, mAnimationBrightness, mAnimationBrightness);


    // ********************************************
    // Calculate Radius.
    // ********************************************

    // Animating Radius?
    if (mDataBlock->mUseRadius)
    {
        U32 PosFrom;
        U32 PosTo;
        F32	LerpFactor;

        // Yes, so adjust time-base.
        mRadiusElapsedTime += mElapsedTime;

        // Adjust to Bounds.
        while (mRadiusElapsedTime > mDataBlock->mRadiusTime) { mRadiusElapsedTime -= mDataBlock->mRadiusTime; };

        // Scale Time.
        F32 ScaledTime = mRadiusElapsedTime * mRadiusTimeScale;

        // Calculate Position From.
        PosFrom = mFloor(ScaledTime);

        // Are we Lerping?
        if (mDataBlock->mLerpRadius)
        {
            // Yes, so calculate Position To.
            PosTo = mCeil(ScaledTime);

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
        // Radius Keys.
        // ********************************************
        if (mRadiusKeysLength)
        {
            // No, so calculate Radius.
            mAnimationRadius = GetLerpKey(mDataBlock->mRadiusKeys, PosFrom, PosTo, mDataBlock->mMinRadius, mDataBlock->mMaxRadius, LerpFactor);
        }
        else
        {
            // Set to static Radius if we failed to set Radius correctly.
            mAnimationRadius = mDataBlock->mRadius;
        }
    }
    else
    {
        // No, so set to static Radius.
        mAnimationRadius = mDataBlock->mRadius;
    }


    // ********************************************
    // Calculate Rotation.
    // ********************************************

    // Animating Rotation?
    if (mDataBlock->mUseRotation)
    {
        U32 PosFrom;
        U32 PosTo;
        F32	LerpFactor;

        // Yes, so adjust time-base.
        mRotationElapsedTime += mElapsedTime;

        // Adjust to Bounds.
        while (mRotationElapsedTime > mDataBlock->mRotationTime) { mRotationElapsedTime -= mDataBlock->mRotationTime; };

        // Scale Time.
        F32 ScaledTime = mRotationElapsedTime * mRotationTimeScale;

        // Calculate Position From.
        PosFrom = mFloor(ScaledTime);

        // Are we Lerping?
        if (mDataBlock->mLerpRotation)
        {
            // Yes, so calculate Position To.
            PosTo = mCeil(ScaledTime);

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
            mAnimationRotation = GetLerpKey(mDataBlock->mRotationKeys, PosFrom, PosTo, mDataBlock->mMinRotation, mDataBlock->mMaxRotation, LerpFactor);
        }
        else
        {
            // Set to static Rotation if we failed to set Rotation correctly.
            mAnimationRotation = mDataBlock->mMinRotation;
        }
    }
    else
    {
        // No, so set to static Rotation.
        mAnimationRotation = 0;
    }



    // ********************************************
    // Calculate Offsets.
    // ********************************************

    // Are we attached?
    if (mAttached)
        calculateLightPosition();

    // Reset Animation Offset.
    mAnimationOffset.set(0, 0, 0);

    // Are we animating Offsets?
    if (mDataBlock->mUseOffsets)
    {
        U32 PosFrom;
        U32 PosTo;
        F32	LerpFactor;

        // Yes, so adjust time-base.
        mOffsetElapsedTime += mElapsedTime;

        // Adjust to Bounds.
        while (mOffsetElapsedTime > mDataBlock->mOffsetTime) { mOffsetElapsedTime -= mDataBlock->mOffsetTime; };

        // Scale Time.
        F32 ScaledTime = mOffsetElapsedTime * mOffsetTimeScale;

        // Calculate Position From.
        PosFrom = mFloor(ScaledTime);

        // Are we Lerping?
        if (mDataBlock->mLerpRadius)
        {
            // Yes, so calculate Position To.
            PosTo = mCeil(ScaledTime);

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
        // Offset Keys.
        // ********************************************
        if (mOffsetKeysLength)
        {
            // No, so get static Position.
            if (!mAttached) getRenderTransform().getColumn(3, &mAnimationPosition);

            // Calculte Current Offset.
            //
            // NOTE:- We store this here in-case we need it for the flare position.
            mAnimationOffset.x = GetLerpKey(mDataBlock->mOffsetKeys, PosFrom, PosTo, mDataBlock->mStartOffset.x, mDataBlock->mEndOffset.x, LerpFactor);
            mAnimationOffset.y = GetLerpKey(mDataBlock->mOffsetKeys, PosFrom, PosTo, mDataBlock->mStartOffset.y, mDataBlock->mEndOffset.y, LerpFactor);
            mAnimationOffset.z = GetLerpKey(mDataBlock->mOffsetKeys, PosFrom, PosTo, mDataBlock->mStartOffset.z, mDataBlock->mEndOffset.z, LerpFactor);

            // Lerp to Offset Position.
            mAnimationPosition += mAnimationOffset;
        }
        else
        {
            // Set to static Position if we failed to set Position correctly.
            if (!mAttached) getRenderTransform().getColumn(3, &mAnimationPosition);
        }
    }
    else
    {
        // No, so set to static Position.
        if (!mAttached) getRenderTransform().getColumn(3, &mAnimationPosition);
    }

    // Construct a world box to include light animation.
    //
    // NOTE:-	We do this so that we always scope the object if we have it's
    //			potential volume in the view.

    buildObjectBox();
}

//------------------------------------------------------------------------------

void fxLight::InitialiseAnimation(void)
{
    // Enable Light.
    mEnable = true;

    // Basis Light.
    mIconSize = 1.0f;

    // Texture Handles.
    mIconTextureHandle =
        mFlareTextureHandle = NULL;

    // Reset Flare Scale.
    mFlareScale = 0.0f;

    // Reset Animation.
    ResetAnimation();
}

//------------------------------------------------------------------------------

void fxLight::ResetAnimation(void)
{
    mAnimationOffset = Point3F(0.0, 0.0, 0.0);


    // Check Animation Keys.
    CheckAnimationKeys();

    // Reset Times.
    mColourElapsedTime =
        mBrightnessElapsedTime =
        mRadiusElapsedTime =
        mOffsetElapsedTime =
        mRotationElapsedTime = 0.0f;

    // Reset Last Animation Time.
    mLastAnimateTime = mLastRenderTime = F32(Platform::getVirtualMilliseconds());

    // Exit if no datablock is ready.
    if (!mDataBlock) return;

    // Check Flare Details.
    if (mDataBlock->mFarDistance < 0) mDataBlock->mFarDistance = 0.001f;
    if (mDataBlock->mNearDistance >= mDataBlock->mFarDistance) mDataBlock->mNearDistance = mDataBlock->mFarDistance - 0.001f;	// Stop Division by 0!
    if (mDataBlock->mBlendMode > 2) mDataBlock->mBlendMode = 0;
}

//------------------------------------------------------------------------------

F32 fxLight::GetLerpKey(StringTableEntry Key, U32 PosFrom, U32 PosTo, F32 ValueFrom, F32 ValueTo, F32 Lerp)
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

U32 fxLight::CheckKeySyntax(StringTableEntry Key)
{
    // Return problem.
    if (!Key) return 0;

    // Copy KeyCheck.
    StringTableEntry KeyCheck = Key;

    // Give benifit of doubt!
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

void fxLight::CheckAnimationKeys(void)
{
    if (!mDataBlock) return;

    // Check Key Validities.
    mRedKeysLength = CheckKeySyntax(mDataBlock->mRedKeys);
    mGreenKeysLength = CheckKeySyntax(mDataBlock->mGreenKeys);
    mBlueKeysLength = CheckKeySyntax(mDataBlock->mBlueKeys);
    mBrightnessKeysLength = CheckKeySyntax(mDataBlock->mBrightnessKeys);
    mRadiusKeysLength = CheckKeySyntax(mDataBlock->mRadiusKeys);
    mOffsetKeysLength = CheckKeySyntax(mDataBlock->mOffsetKeys);
    mRotationKeysLength = CheckKeySyntax(mDataBlock->mRotationKeys);

    // Calculate Time Scales.
    if (mDataBlock->mColourTime) mColourTimeScale = (mRedKeysLength - 1) / mDataBlock->mColourTime;
    if (mDataBlock->mBrightnessTime) mBrightnessTimeScale = (mBrightnessKeysLength - 1) / mDataBlock->mBrightnessTime;
    if (mDataBlock->mRadiusTime) mRadiusTimeScale = (mRadiusKeysLength - 1) / mDataBlock->mRadiusTime;
    if (mDataBlock->mOffsetTime) mOffsetTimeScale = (mOffsetKeysLength - 1) / mDataBlock->mOffsetTime;
    if (mDataBlock->mRotationTime) mRotationTimeScale = (mRotationKeysLength - 1) / mDataBlock->mRotationTime;
}

//------------------------------------------------------------------------------

bool fxLight::onNewDataBlock(GameBaseData* dptr)
{
    // Cast new Datablock.
    mDataBlock = dynamic_cast<fxLightData*>(dptr);
    // Check for problems.
    if (!mDataBlock || !Parent::onNewDataBlock(dptr))return false;

    // Reset Animation.
    ResetAnimation();

    // Set Flare Bitmap.
    setFlareBitmap(mDataBlock->mFlareTextureName);

    // Script-it.
    scriptOnNewDataBlock();

    // Return OK.
    return true;
}

//------------------------------------------------------------------------------

bool fxLight::onAdd()
{
    // Add Parent.
    if (!Parent::onAdd()) return(false);

    buildObjectBox();

    // Set the Render Transform.
    setRenderTransform(mObjToWorld);

    // Add to Scene.
    addToScene();
    mAddedToScene = true;

    // Only on client.
    if (isClientObject())
    {
        // Fetch Textures.
        mIconTextureHandle = GFXTexHandle(FXLIGHTDBICONTEXTURE, &GFXDefaultStaticDiffuseProfile);
        setFlareBitmap(mDataBlock->mFlareTextureName);

        // Add object to Light Set.
        Sim::getLightSet()->addObject(this);
    }

    // Return OK.
    return(true);
}

//------------------------------------------------------------------------------

void fxLight::onRemove()
{
    // Detach from Object.
    //detachFromObject();

    // Remove from Scene.
    removeFromScene();
    mAddedToScene = false;

    // Only on client.
    if (isClientObject())
    {
        // Remove Texture References.
        mIconTextureHandle = mFlareTextureHandle = NULL;

        // Remove object from Light Set.
        Sim::getLightSet()->removeObject(this);
    }

    // Do Parent.
    Parent::onRemove();
}

//------------------------------------------------------------------------------

/*void fxLight::onDeleteNotify(SimObject* Obj)
{
    // Server?
    if (isServerObject())
    {
        // Yes, so detach the server way!
        detachFromObject();
    }
    else
    {
        // No, so do it manually (if needed).
        if (mAttached) clearNotify(mpAttachedObject);

        // Store it happening.
        mAttached = false;
    }

    // Do Parent.
    Parent::onDeleteNotify(Obj);
}*/

//------------------------------------------------------------------------------

void fxLight::inspectPostApply()
{
    // Reset Animation.
    ResetAnimation();

    // Set Parent.
    Parent::inspectPostApply();

    // Fetch Icon Texture.
    if (isServerObject()) mIconTextureHandle = GFXTexHandle(FXLIGHTDBICONTEXTURE, &GFXDefaultStaticDiffuseProfile);

    // Set Config Change Mask.
    setMaskBits(fxLightConfigChangeMask);
}

//------------------------------------------------------------------------------

bool fxLight::prepRenderImage(SceneState* state, const U32 stateKey, const U32 startZone,
    const bool modifyBaseZoneState)
{
    // No need to render without a flare when not editing mission!
    if (!mDataBlock->mFlareOn && !gEditingMission) return false;

    // Return if last state.
    if (isLastState(state, stateKey)) return false;
    // Set Last State.
    setLastState(state, stateKey);

    // Is Object Rendered?
    if (state->isObjectRendered(this))
    {
        /*// Yes, so get a SceneRenderImage.
        SceneRenderImage* image = new SceneRenderImage;
        // Populate it.
        image->obj = this;
        image->isTranslucent = true;
        image->sortType = SceneRenderImage::EndSort;
        // Insert it into the scene images.
        state->insertRenderImage(image);*/

        RenderInst* ri = gRenderInstManager.allocInst();
        ri->obj = this;
        ri->state = state;
        ri->type = RenderInstManager::RIT_ObjectTranslucent;
        ri->translucent = true;
        ri->calcSortPoint(this, state->getCameraPosition());
        gRenderInstManager.addInst(ri);
    }

    return false;
}

//------------------------------------------------------------------------------

void fxLight::renderObject(SceneState* state, RenderInst* ri)
{
    if (!mEnable || !mDataBlock->mLightOn) return;

    // Check we are in Canonical State.
    //AssertFatal(dglIsInCanonicalState(), "Error, GL not in canonical state on entry");

    // Cannot render without appropriate texture!
    if (!mDataBlock->mFlareOn && !mIconTextureHandle) return;
    if (mDataBlock->mFlareOn && !mFlareTextureHandle) return;

    MatrixF ModelView;
    MatrixF RXF;
    Point4F Position;
    F32		BBRadius;

    // Calculate Elapsed Time.
    F32 mElapsedTime = (F32)((Platform::getRealMilliseconds() - mLastRenderTime) / 1000.0f);

    // Reset Last Render Time.
    mLastRenderTime = Platform::getRealMilliseconds();

    // Screen Viewport.
    //RectI viewport;

    // Setup out the Projection Matrix/Viewport.
    //glMatrixMode(GL_PROJECTION);
    //glPushMatrix();
    //dglGetViewport(&viewport);

    RectI viewport = GFX->getViewport();
    MatrixF projection = GFX->getProjectionMatrix();
    MatrixF world = GFX->getWorldMatrix();
    MatrixF view = GFX->getViewMatrix();

    GFX->disableShaders();

    state->setupBaseProjection();

    // Setup Billboard View.
    //
    // NOTE:-	We need to adjust for any animated offset.
    //glPushMatrix();

    // Get Object Transform.
    RXF = getRenderTransform();
    RXF.getColumn(3, &Position);

    // Translate transform to absolute position.
    Position.x = mAnimationPosition.x;
    Position.y = mAnimationPosition.y;
    Position.z = mAnimationPosition.z;

    // Set new Position.
    RXF.setColumn(3, Position);
    //dglMultMatrix(&RXF);
    GFX->multWorld(RXF);

    // Finish-up billboard.
    //dglGetModelview(&ModelView);

    MatrixF worldmod = GFX->getWorldMatrix();
    MatrixF viewmod = GFX->getViewMatrix();

    ModelView.mul(viewmod, worldmod);
    ModelView.getColumn(3, &Position);
    ModelView.identity();
    ModelView.setColumn(3, Position);
    //dglLoadMatrix(&ModelView);

    GFX->setWorldMatrix(ModelView);
    MatrixF ident;
    ident.identity();
    GFX->setViewMatrix(ident);

    // Setup our rendering state.
    //glEnable(GL_BLEND);
    //glEnable(GL_TEXTURE_2D);
    //glDisable(GL_DEPTH_TEST);

    GFX->setBaseRenderState();
    GFX->setCullMode(GFXCullNone);
    GFX->setLightingEnable(false);
    GFX->setAlphaBlendEnable(true);
    GFX->setZEnable(false);
    GFX->setZWriteEnable(false);
    GFX->setTextureStageColorOp(0, GFXTOPModulate);
    //GFX->setTexture(0, sgShadow);
    GFX->setTextureStageColorOp(1, GFXTOPDisable);
    GFX->setSrcBlend(GFXBlendOne);
    GFX->setDestBlend(GFXBlendZero);

    // Select User Blend Mode.
    switch (mDataBlock->mBlendMode)
    {
    case 0:
        //glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        GFX->setSrcBlend(GFXBlendSrcAlpha);
        GFX->setDestBlend(GFXBlendOne);
        break;
    case 1:
        //glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA); 
        GFX->setSrcBlend(GFXBlendSrcAlpha);
        GFX->setDestBlend(GFXBlendInvSrcAlpha);
        break;
    case 2:
        //glBlendFunc(GL_ONE,GL_ONE); 
        GFX->setSrcBlend(GFXBlendOne);
        GFX->setDestBlend(GFXBlendOne);
        break;

    default:
        // Duh, User error.
        Con::printf("fxLight: Invalid Blend Mode Selected!");
        //glBlendFunc(GL_SRC_ALPHA, GL_ONE); 
        GFX->setSrcBlend(GFXBlendSrcAlpha);
        GFX->setDestBlend(GFXBlendOne);
        break;
    }

    // Is the Flare On?
    if (mDataBlock->mFlareOn && mEnable)
    {
        // Flare On ...

        // Modulate Texture.
        //glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
        GFX->setTextureStageColorOp(0, GFXTOPModulate);

        Point3F ScreenPoint;
        Point3F ObjectPoint;

        // Get Animation Position.
        ObjectPoint = mAnimationPosition;


        //
        // Calculate screen point, on screen?
        //if (dglPointToScreen(ObjectPoint, ScreenPoint))
        MathUtils::projectWorldToScreen(ScreenPoint, ObjectPoint, viewport, ModelView, projection);
        if (true)
        {
            // Fetch Eye Position.
            Point3F	eyePos = state->getCameraPosition();

            // Calculate Distance from Light.
            F32 Distance = (ObjectPoint - eyePos).len();

            // Reset Billboard Radius.
            BBRadius = 1.0f;

            // Are we using Constant Size?
            if (mDataBlock->mConstantSizeOn)
            {
                // Yes, so simply size to Constant Size.
                BBRadius *= mDataBlock->mConstantSize;
            }
            else
            {
                // No, so calculate current size (in world units) according to distance.

                // Are we below the near size distance?
                if (Distance <= mDataBlock->mNearDistance)
                {
                    // Yes, so clamp at near size.
                    BBRadius *= mDataBlock->mNearSize;
                }
                // Are we above the far size distance?
                else if (Distance >= mDataBlock->mFarDistance)
                {
                    // Yes, so clamp at far size.
                    BBRadius *= mDataBlock->mFarSize;
                }
                else
                {
                    // In the ramp so calculate current size according to distance.
                    BBRadius = mDataBlock->mNearSize + ((Distance - mDataBlock->mNearDistance) * ((mDataBlock->mFarSize - mDataBlock->mNearSize) / (mDataBlock->mFarDistance - mDataBlock->mNearDistance)));
                }
            }
        }
        else
        {
            // Console Warning.
            //Con::warnf("Flare point off screen!  How did this happen huh?");

            // Calculate Billboard Radius.
            BBRadius = 0;
        }

        // Do we have Line-of-sight?
        if (TestLOS(mAnimationPosition, getAttachedObject()))
        {
            // Yes, so are we fully showing?
            if (mFlareScale < 1.0f)
            {
                // No, so calculate new scale.
                mFlareScale += mElapsedTime / mDataBlock->mFadeTime;

                // Check new scale.
                if (mFlareScale > 1.0f) mFlareScale = 1.0f;
            }
        }
        else
        {
            // No, so are we fully hidden?
            if (mFlareScale > 0.0f)
            {
                // No, so calculate new scale.
                mFlareScale -= mElapsedTime / mDataBlock->mFadeTime;

                // Check new scale.
                if (mFlareScale < 0.0f) mFlareScale = 0.0f;
            }
        }

        // Scale BB Size.
        BBRadius *= mFlareScale;

        // Are we linking flare size?
        if (mDataBlock->mLinkFlareSize)
        {
            // Yes, so Scale by LUMINANCE (better for modern colour monitors).
            BBRadius *= (mAnimationColour.red * 0.212671f) +
                (mAnimationColour.green * 0.715160f) +
                (mAnimationColour.blue * 0.072169f);
        }

        // Select Icon Texture.
        //glBindTexture(GL_TEXTURE_2D, mFlareTextureHandle.getGLName());
        GFX->setTexture(0, mFlareTextureHandle);

        // Are we linking the flare?
        if (mDataBlock->mLinkFlare)
        {
            // Yes, so set Flare Colour to animation.
            PrimBuild::color3f(mAnimationColour.red, mAnimationColour.green, mAnimationColour.blue);
        }
        else
        {
            // No, so set it to constant flare colour.
            PrimBuild::color3f(mDataBlock->mFlareColour.red, mDataBlock->mFlareColour.green, mDataBlock->mFlareColour.blue);
        }
    }
    else
    {
        // Icon On ...

        // Icon Blend Function.
        //glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
        GFX->setSrcBlend(GFXBlendSrcAlpha);
        GFX->setDestBlend(GFXBlendInvSrcAlpha);

        // Replace Texture.
        //glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
        GFX->setTextureStageColorOp(0, GFXTOPSelectARG1);
        GFX->setTextureStageColorArg1(0, GFXTATexture);
        GFX->setTextureStageColorOp(1, GFXTOPDisable);

        // Calculate Billboard Radius.
        BBRadius = mIconSize / 2;

        // Select Icon Texture.
        //glBindTexture(GL_TEXTURE_2D, mIconTextureHandle.getGLName());
        GFX->setTexture(0, mIconTextureHandle);
    }

    // Only draw if needed.
    if (BBRadius > 0)
    {
        // Rotate, if we have a rotation.
        //if (mAnimationRotation != 0.0f) glRotatef(mAnimationRotation, 0, 1, 0);

        // Draw Billboard.
        //glBegin(GL_QUADS);
        PrimBuild::begin(GFXTriangleFan, 4);
        PrimBuild::texCoord2f(0, 0);
        PrimBuild::vertex3f(-BBRadius, 0, BBRadius);
        PrimBuild::texCoord2f(1, 0);
        PrimBuild::vertex3f(BBRadius, 0, BBRadius);
        PrimBuild::texCoord2f(1, 1);
        PrimBuild::vertex3f(BBRadius, 0, -BBRadius);
        PrimBuild::texCoord2f(0, 1);
        PrimBuild::vertex3f(-BBRadius, 0, -BBRadius);
        PrimBuild::end();
        //glEnd();
    }

    // Restore rendering state.
    GFX->setTextureStageColorOp(0, GFXTOPDisable);
    GFX->setCullMode(GFXCullCCW);
    GFX->setLightingEnable(false);
    GFX->setAlphaBlendEnable(false);
    GFX->setZEnable(true);
    GFX->setZWriteEnable(true);

    //glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
    //glEnable(GL_DEPTH_TEST);
    //glDisable(GL_TEXTURE_2D);
    //glDisable(GL_BLEND);

    // Restore our nice, friendly and dull canonical state.
    //glPopMatrix();
    //glMatrixMode(GL_PROJECTION);
    //glPopMatrix();
    //glMatrixMode(GL_MODELVIEW);
    //dglSetViewport(viewport);

    GFX->setWorldMatrix(world);
    GFX->setViewMatrix(view);
    GFX->setProjectionMatrix(projection);
    GFX->setViewport(viewport);

    // Check we have restored Canonical State.
    //AssertFatal(dglIsInCanonicalState(), "Error, GL not in canonical state on exit");
}

//------------------------------------------------------------------------------

bool fxLight::TestLOS(const Point3F& ObjectPosition, SceneObject* AttachedObj)
{
    // Valid Control Object types (for now).
    const U32 ObjectMask = PlayerObjectType | VehicleObjectType | CameraObjectType;

    // Get Server Connection.
    GameConnection* con = GameConnection::getConnectionToServer();
    // Problem?
    if (!con) return false;

    // Get the Control Object.
    ShapeBase* ControlObj = con->getControlObject();
    // Valid Control Objects?
    if (!ControlObj || !(ControlObj->getType() & ObjectMask)) return false;
    // Kill flare if Third-person not available.
    if (!ControlObj->isFirstPerson() && !mDataBlock->mFlareTP) return false;

    // Fetch Eye Position.
    Point3F eyePos;
    MatrixF eye;
    con->getControlCameraTransform(0, &eye);
    eye.getColumn(3, &eyePos);

    // Get our object center.
    Point3F endPos = ObjectPosition;

    // LOS Mask.
    static U32 losMask = STATIC_COLLISION_MASK |
        ShapeBaseObjectType |
        StaticTSObjectType |
        PlayerObjectType;

    // Stop Collisions with our Control Object (in first person).
    if (ControlObj->isFirstPerson()) ControlObj->disableCollision();
    // If we are attached to an object then disable it's collision.
    if (AttachedObj) AttachedObj->disableCollision();

    // Store old Object Box.
    Box3F OldObjBox = mObjBox;

    // Set LOS Test Object Box.
    mObjBox.min.set(-0.5, -0.5, -0.5);
    mObjBox.max.set(0.5, 0.5, 0.5);
    // Reset the World Box.
    resetWorldBox();

    // Perform a ray cast on the client container database.
    RayInfo info;
    bool los = !getCurrentClientContainer()->castRay(eyePos, endPos, losMask, &info);

    // Restore old Object Box.
    mObjBox = OldObjBox;
    // Reset the World Box.
    resetWorldBox();

    // If we are attached to an object then enable it's collision.
    if (AttachedObj) AttachedObj->enableCollision();
    // Continue Collisions with our Control Object (in first person).
    if (ControlObj->isFirstPerson()) ControlObj->enableCollision();

    // Return LOS result.
    return los;
};

//------------------------------------------------------------------------------

U32 fxLight::packUpdate(NetConnection* con, U32 mask, BitStream* stream)
{
    // Pack Parent.
    U32 retMask = Parent::packUpdate(con, mask, stream);

    /*
        // Write Attach Flag.
        if (stream->writeFlag((mask & fxLightAttachChange) && mAttachValid))
        {
            // Get Attached Object.
            GameBase* Obj = getProcessAfter();

            // Write Object Attach/Detach Flag.
            if (stream->writeFlag(Obj != NULL))
            {
                // Get the GhostID of the object.
                S32 GhostID = con->getGhostIndex(Obj);
                // Send it to the client.
                stream->writeRangedU32(U32(GhostID), 0, NetConnection::MaxGhostCount);
            }
            // Invalidate Attachment.
            //mAttachValid = false;
        }
    */

    // Write Config Change Flag.
    if (stream->writeFlag(mask & fxLightConfigChangeMask))
    {
        // Position.
        stream->writeAffineTransform(mObjToWorld);

        // Enable.
        stream->write(mEnable);

        // Basis Light.
        stream->write(mIconSize);
    }

    // Were done ...
    return(retMask);
}

//------------------------------------------------------------------------------

void fxLight::unpackUpdate(NetConnection* con, BitStream* stream)
{
    // Unpack Parent.
    Parent::unpackUpdate(con, stream);

    /*
        // Read Attach Position.
        if (stream->readFlag())
        {
            // Read Attach/Detach Flag.
            mAttached = stream->readFlag();

            // Read Position.
            if (mAttached)
            {
                // Get the ObjectID.
                S32 ObjectID = stream->readRangedU32(0, NetConnection::MaxGhostCount);
                // Resolve it.
                NetObject* pObject = con->resolveGhost(ObjectID);
                if (pObject != NULL)
                {
                    // Get the object.
                    mpAttachedObject = dynamic_cast<GameBase*>(pObject);

                    // Make sure we know if it's deleted.
                    deleteNotify(mpAttachedObject);

                    //Con::warnf(ConsoleLogEntry::General, "fxLight::unpack: attached to object.");
                }
                else
                {
                    //Con::warnf(ConsoleLogEntry::General, "fxLight::unpack: could not resolve attachment targetID!");
                    //Con::warnf(ConsoleLogEntry::General, "fxLight::unpack: recovering by detaching object.");
                    mAttached = false;
                }
            }
            else
            {
                // Reset Any Attachment.
                mAttached = false;
            }
        }
    */

    // Read Config Change Details?
    if (stream->readFlag())
    {
        MatrixF		ObjectMatrix;

        // Position.
        stream->readAffineTransform(&ObjectMatrix);

        // Enable.
        stream->read(&mEnable);

        // Basis Light.
        stream->read(&mIconSize);

        // Set Transform.
        setTransform(ObjectMatrix);

        // Reset Animation.
        ResetAnimation();
    }
}

//------------------------------------------------------------------------------
