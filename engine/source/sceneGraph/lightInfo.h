//-----------------------------------------------------------------------------
// Torque Game Engine Advanced
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _LIGHTINFO_H_
#define _LIGHTINFO_H_

#ifndef _PLATFORM_H_
#include "platform/platform.h"
#endif

#ifndef _TVECTOR_H_
#include "core/tVector.h"
#endif

#ifndef _GFXSTRUCTS_H_
#include "gfx/gfxStructs.h"
#endif

struct SceneGraphData;

class LightInfo
{
    friend class LightManager;

public:
    enum Type {
        Point = 0,
        Spot = 1,
        Vector = 2,
        Ambient = 3,
        SGStaticPoint,
        SGStaticSpot
    };
    Type        mType;

    Point3F     mPos;
    VectorF     mDirection;
    ColorF      mColor;
    ColorF      mAmbient;
    ColorF      mShadowColor;
    F32         mRadius;

    //private:
    S32         mScore;

public:
    enum sgFeatures
    {
        // in order from most features to least...
        sgFull = 0,
        sgNoCube,
        sgNoSpecCube,
        sgFeatureCount
    };
    sgFeatures sgSupportedFeatures;

    bool sgCanBeSecondary() { return sgSupportedFeatures >= sgNoSpecCube; }

    static bool sgAllowSpecular(sgFeatures features) { return features < sgNoSpecCube; }
    static bool sgAllowCubeMapping(sgFeatures features) { return features < sgNoCube; }

    F32 sgSpotAngle;
    bool sgAssignedToTSObject;
    bool sgCastsShadows;
    bool sgDiffuseRestrictZone;
    bool sgAmbientRestrictZone;
    S32 sgZone[2];
    F32 sgLocalAmbientAmount;
    bool sgSmoothSpotLight;
    bool sgDoubleSidedAmbient;
    bool sgAssignedToParticleSystem;
    StringTableEntry sgLightingModelName;
    bool sgUseNormals;
    Point3F sgTempModelInfo;
    MatrixF sgLightingTransform;
    PlaneF sgSpotPlane;

    LightInfo();
    bool sgIsInZone(S32 zone);
    bool sgAllowDiffuseZoneLighting(S32 zone);

    // Sets up a GFX fixed function light
    void setGFXLight(LightInfo* light)
    {
        switch (mType) {
            case LightInfo::SGStaticPoint :
            case LightInfo::Point :
                light->mType = LightInfo::Point;
                break;
            case LightInfo::Spot :
            case LightInfo::SGStaticSpot :
                light->mType = LightInfo::Spot;
                break;
            case LightInfo::Vector:
                light->mType = LightInfo::Vector;
                break;
            case LightInfo::Ambient:
                light->mType = LightInfo::Ambient;
                break;
        }
        light->mPos = mPos;
        light->mDirection = mDirection;
        light->mColor = mColor;
        light->mAmbient = mAmbient;
        light->mShadowColor = mShadowColor;
        light->mRadius = mRadius;
        light->sgSpotAngle = sgSpotAngle;
    }
};

#endif
