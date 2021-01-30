//-----------------------------------------------
// Synapse Gaming - Lighting System
// Copyright © Synapse Gaming 2003
// Written by John Kabus
//-----------------------------------------------
#ifndef _SGUNIVERSALSTATICLIGHT_H_
#define _SGUNIVERSALSTATICLIGHT_H_

#include "game/fx/fxLight.h"
#include "game/fx/particleEmitter.h"
#include "game/fx/particleEmitterNode.h"


/**
 * The datablock class for sgUniversalStaticLight.
 */
class sgLightObjectData : public fxLightData
{
    typedef fxLightData Parent;

public:
    /// Enables static lighting (baked into light maps)
    /// instead of Torque's default dynamic lighting.
    bool sgStatic;
    /// Turns this light into a spotlight (only works with static lights).
    bool sgSpot;
    /// The spotlight spread angle.
    F32 sgSpotAngle;
    /// Selects the lighting model.
    /// The default lighting model uses (rad^2 / dist^2)
    /// and creates a distinct drop off.
    /// The advanced lighting model uses (1 - (dist^2 / rad^2))
    /// and creates gradual / global illumination feel (does not perform radiosity).
    bool sgAdvancedLightingModel;
    /// Allows this light to illuminate dts objects.
    bool sgEffectsDTSObjects;
    bool sgCastsShadows;
    bool sgDiffuseRestrictZone;
    bool sgAmbientRestrictZone;
    F32 sgLocalAmbientAmount;
    bool sgSmoothSpotLight;
    bool sgDoubleSidedAmbient;
    StringTableEntry sgLightingModelName;
    bool sgUseNormals;
    U32 sgSupportedFeatures;
    U32 sgMountPoint;
    MatrixF sgMountTransform;

    sgLightObjectData();
    static void  initPersistFields();
    virtual void packData(BitStream* stream);
    virtual void unpackData(BitStream* stream);

    DECLARE_CONOBJECT(sgLightObjectData);
};

/**
 * This class extends fxLight to provide mission level
 * static light objects.  These light objects can also
 * be used as dynamic lights and linked to mission level
 * particle emitters for coordinated particle system
 * light sources (flickering,...).
 */
class sgLightObject : public fxLight
{
    typedef fxLight Parent;
    sgLightObjectData* mDataBlock;

    /// The particle emitter's ghost id is used to sync up the client and server.
    bool sgValidParticleEmitter;
    S32 sgParticleEmitterGhostIndex;
    S32 sgAttachedObjectGhostIndex;
    /// The resolved (from the ghost id) particle emitter.
    SimObjectPtr<ParticleEmitter> sgParticleEmitterPtr;
    SimObjectPtr<GameBase> sgAttachedObjectPtr;

    S32 sgMainZone;

    class sgAnimateState
    {
        bool mUseColour;
        bool mUseBrightness;
        bool mUseRadius;
        bool mUseOffsets;
        bool mUseRotation;
    public:
        sgAnimateState()
        {
            mUseColour = false;
            mUseBrightness = false;
            mUseRadius = false;
            mUseOffsets = false;
            mUseRotation = false;
        }
        void sgCopyState(sgLightObjectData* light)
        {
            mUseColour = light->mUseColour;
            mUseBrightness = light->mUseBrightness;
            mUseRadius = light->mUseRadius;
            mUseOffsets = light->mUseOffsets;
            mUseRotation = light->mUseRotation;
        }
        void sgRestoreState(sgLightObjectData* light)
        {
            light->mUseColour = mUseColour;
            light->mUseBrightness = mUseBrightness;
            light->mUseRadius = mUseRadius;
            light->mUseOffsets = mUseOffsets;
            light->mUseRotation = mUseRotation;
        }
        void sgDisableState(sgLightObjectData* light)
        {
            light->mUseColour = false;
            light->mUseBrightness = false;
            light->mUseRadius = false;
            light->mUseOffsets = false;
            light->mUseRotation = false;
        }
    };

public:
    enum {
        sgLastfxLightMask = fxLightAttachChange,
        sgParticleSystemMask = sgLastfxLightMask << 1,
        sgAttachedObjectMask = sgLastfxLightMask << 2
    };

    LightInfo mLight;
    /// Used to attenuate the color received from the particle emitter.
    F32 sgParticleColorAttenuation;
    /// The particle emitter name field.
    /// This is the unique mission name of the particle emitter.
    StringTableEntry sgParticleEmitterName;

    sgLightObject()
    {
        sgParticleEmitterName = StringTable->insert("");
        sgParticleEmitterGhostIndex = -1;
        sgAttachedObjectGhostIndex = -1;
        sgParticleColorAttenuation = 1.0f;
        sgParticleEmitterPtr = NULL;
        sgAttachedObjectPtr = NULL;
        sgValidParticleEmitter = false;
    }

    void attachToObject(GameBase* obj);
    void detachFromObject();

    bool onAdd();
    bool onNewDataBlock(GameBaseData* dptr);
    void processTick(const Move* move);
    virtual void calculateLightPosition();
    virtual SceneObject* getAttachedObject() { return sgAttachedObjectPtr; }
    void renderObject(SceneState* state, RenderInst* ri);
    virtual void setTransform(const MatrixF& mat);
    void registerLights(LightManager* lightManager, bool lightingScene);
    static void initPersistFields();
    virtual void inspectPostApply();
    U32 packUpdate(NetConnection* con, U32 mask, BitStream* stream);
    void unpackUpdate(NetConnection* con, BitStream* stream);
    /// Tries to turn sgParticleEmitterName into sgParticleEmitterGhostIndex.
    void sgCalculateParticleSystemInfo(NetConnection* con);

    DECLARE_CONOBJECT(sgLightObject);
};

#endif//_SGUNIVERSALSTATICLIGHT_H_
