//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "platform/platform.h"
#include "core/dnet.h"
#include "sfx/sfxSystem.h"
#include "game/gameConnection.h"
#include "game/moveManager.h"
#include "console/consoleTypes.h"
#include "core/bitStream.h"
#include "ts/tsPartInstance.h"
#include "ts/tsShapeInstance.h"
#include "sceneGraph/sceneGraph.h"
#include "sceneGraph/sceneState.h"
#include "game/shadow.h"
#include "game/fx/explosion.h"
#include "game/shapeBase.h"
#ifdef TORQUE_TERRAIN
#include "terrain/waterBlock.h"
#endif
#include "materials/material.h"
#include "game/debris.h"
#include "terrain/environment/sky.h"
#include "game/physicalZone.h"
#include "sceneGraph/detailManager.h"
#include "math/mathUtils.h"
#include "math/mMatrix.h"
#include "math/mRandom.h"
#include "platform/profiler.h"
#include "gfx/gfxCubemap.h"
#include "renderInstance/renderInstMgr.h"
#include "game/gameProcess.h"

#include "game/marble/marble.h"

bool gNoRenderAstrolabe = false;
bool gForceNotHidden = false;

IMPLEMENT_CO_DATABLOCK_V1(ShapeBaseData);


//----------------------------------------------------------------------------
// Timeout for non-looping sounds on a channel
static SimTime sAudioTimeout = 500;
bool ShapeBase::gRenderEnvMaps = true;
F32  ShapeBase::sWhiteoutDec = 0.007;
F32  ShapeBase::sDamageFlashDec = 0.007;
U32  ShapeBase::sLastRenderFrame = 0;

static const char* sDamageStateName[] =
{
    // Index by enum ShapeBase::DamageState
    "Enabled",
    "Disabled",
    "Destroyed"
};


//----------------------------------------------------------------------------

ShapeBaseData::ShapeBaseData()
{
    shadowEnable = false;
    shadowCanMove = false;
    shadowCanAnimate = false;
    shadowSelfShadow = false;
    shadowSize = 64;
    shadowAnimationFrameSkip = 5;
    shadowMaxVisibleDistance = 80.0f;
    shadowProjectionDistance = 10.0f;
    shadowSphereAdjust = 1.0;
    shadowBias = 0.0005;


    shapeName = "";
    cloakTexName = "";
    mass = 1;
    drag = 0;
    density = 1;
    maxEnergy = 0;
    maxDamage = 1.0;
    disabledLevel = 1.0;
    destroyedLevel = 1.0;
    repairRate = 0.0033;
    eyeNode = -1;
    shadowNode = -1;
    cameraNode = -1;
    damageSequence = -1;
    hulkSequence = -1;
    cameraMaxDist = 0;
    cameraMinDist = 0.2;
    cameraDefaultFov = 90.f;
    cameraMinFov = 5.f;
    cameraMaxFov = 120.f;
    emap = false;
    aiAvoidThis = false;
    isInvincible = false;
    renderWhenDestroyed = true;
    debris = NULL;
    debrisID = 0;
    debrisShapeName = NULL;
    explosion = NULL;
    explosionID = 0;
    underwaterExplosion = NULL;
    underwaterExplosionID = 0;
    firstPersonOnly = false;
    useEyePoint = false;
    dynamicReflection = false;

#ifdef MB_ULTRA
    renderGemAura = false;
    glass = false;
    astrolabe = false;
    astrolabePrime = false;
    gemAuraTextureName = "";
    gemAuraTexture = NULL;
    referenceColor.set(1.0f, 1.0f, 1.0f, 1.0f);
#endif

    observeThroughObject = false;
    computeCRC = false;


    // no shadows by default
    genericShadowLevel = 2.0f;
    noShadowLevel = 2.0f;

    inheritEnergyFromMount = false;

    for (U32 j = 0; j < NumHudRenderImages; j++)
    {
        hudImageNameFriendly[j] = 0;
        hudImageNameEnemy[j] = 0;
        hudRenderCenter[j] = false;
        hudRenderModulated[j] = false;
        hudRenderAlways[j] = false;
        hudRenderDistance[j] = false;
        hudRenderName[j] = false;
    }
}

static ShapeBaseData gShapeBaseDataProto;

ShapeBaseData::~ShapeBaseData()
{

}

bool ShapeBaseData::preload(bool server, char errorBuffer[256])
{
    if (!Parent::preload(server, errorBuffer))
        return false;
    bool shapeError = false;

    // Resolve objects transmitted from server
    if (!server) {

        if (!explosion && explosionID != 0)
        {
            if (Sim::findObject(explosionID, explosion) == false)
            {
                Con::errorf(ConsoleLogEntry::General, "ShapeBaseData::preload: Invalid packet, bad datablockId(explosion): 0x%x", explosionID);
            }
            AssertFatal(!(explosion && ((explosionID < DataBlockObjectIdFirst) || (explosionID > DataBlockObjectIdLast))),
                "ShapeBaseData::preload: invalid explosion data");
        }

        if (!underwaterExplosion && underwaterExplosionID != 0)
        {
            if (Sim::findObject(underwaterExplosionID, underwaterExplosion) == false)
            {
                Con::errorf(ConsoleLogEntry::General, "ShapeBaseData::preload: Invalid packet, bad datablockId(underwaterExplosion): 0x%x", underwaterExplosionID);
            }
            AssertFatal(!(underwaterExplosion && ((underwaterExplosionID < DataBlockObjectIdFirst) || (underwaterExplosionID > DataBlockObjectIdLast))),
                "ShapeBaseData::preload: invalid underwaterExplosion data");
        }

        if (!debris && debrisID != 0)
        {
            Sim::findObject(debrisID, debris);
            AssertFatal(!(debris && ((debrisID < DataBlockObjectIdFirst) || (debrisID > DataBlockObjectIdLast))),
                "ShapeBaseData::preload: invalid debris data");
        }


        if (debrisShapeName && debrisShapeName[0] != '\0' && !bool(debrisShape))
        {
            debrisShape = ResourceManager->load(debrisShapeName);
            if (bool(debrisShape) == false)
            {
                dSprintf(errorBuffer, 256, "ShapeBaseData::load: Couldn't load shape \"%s\"", debrisShapeName);
                return false;
            }
            else
            {
                if (!server && !debrisShape->preloadMaterialList() && NetConnection::filesWereDownloaded())
                    shapeError = true;

                TSShapeInstance* pDummy = new TSShapeInstance(debrisShape, !server);
                delete pDummy;
            }
        }
    }

    //
    if (shapeName && shapeName[0]) {
        S32 i;

        // Resolve shapename
        shape = ResourceManager->load(shapeName, computeCRC);
        if (!bool(shape)) {
            dSprintf(errorBuffer, 256, "ShapeBaseData: Couldn't load shape \"%s\"", shapeName);
            return false;
        }
        if (!server && !shape->preloadMaterialList() && NetConnection::filesWereDownloaded())
            shapeError = true;

        if (computeCRC)
        {
            Con::printf("Validation required for shape: %s", shapeName);
            if (server)
                mCRC = shape.getCRC();
            else if (mCRC != shape.getCRC())
            {
                dSprintf(errorBuffer, 256, "Shape \"%s\" does not match version on server.", shapeName);
                return false;
            }
        }
        // Resolve details and camera node indexes.
        for (i = 0; i < shape->details.size(); i++)
        {
            char* name = (char*)shape->names[shape->details[i].nameIndex];

            if (dStrstr((const char*)dStrlwr(name), "collision-"))
            {
                collisionDetails.push_back(i);
                collisionBounds.increment();

                shape->computeBounds(collisionDetails.last(), collisionBounds.last());
                shape->getAccelerator(collisionDetails.last());

                if (!shape->bounds.isContained(collisionBounds.last()))
                {
                    Con::warnf("Warning: shape %s collision detail %d (Collision-%d) bounds exceed that of shape.", shapeName, collisionDetails.size() - 1, collisionDetails.last());
                    collisionBounds.last() = shape->bounds;
                }
                else if (collisionBounds.last().isValidBox() == false)
                {
                    Con::errorf("Error: shape %s-collision detail %d (Collision-%d) bounds box invalid!", shapeName, collisionDetails.size() - 1, collisionDetails.last());
                    collisionBounds.last() = shape->bounds;
                }

                // The way LOS works is that it will check to see if there is a LOS detail that matches
                // the the collision detail + 1 + MaxCollisionShapes (this variable name should change in
                // the future). If it can't find a matching LOS it will simply use the collision instead.
                // We check for any "unmatched" LOS's further down
                LOSDetails.increment();

                char buff[128];
                dSprintf(buff, sizeof(buff), "LOS-%d", i + 1 + MaxCollisionShapes);
                U32 los = shape->findDetail(buff);
                if (los == -1)
                    LOSDetails.last() = i;
                else
                    LOSDetails.last() = los;
            }
        }

        // Snag any "unmatched" LOS details
        for (i = 0; i < shape->details.size(); i++)
        {
            char* name = (char*)shape->names[shape->details[i].nameIndex];

            if (dStrstr((const char*)dStrlwr(name), "los-"))
            {
                // See if we already have this LOS
                bool found = false;
                for (U32 j = 0; j < LOSDetails.size(); j++)
                {
                    if (LOSDetails[j] == i)
                    {
                        found = true;
                        break;
                    }
                }

                if (!found)
                    LOSDetails.push_back(i);
            }
        }

        debrisDetail = shape->findDetail("Debris-17");
        eyeNode = shape->findNode("eye");
        cameraNode = shape->findNode("cam");
        if (cameraNode == -1)
            cameraNode = eyeNode;

        // Resolve mount point node indexes
        for (i = 0; i < NumMountPoints; i++) {
            char fullName[256];
            dSprintf(fullName, sizeof(fullName), "mount%d", i);
            mountPointNode[i] = shape->findNode(fullName);
        }

        // find the AIRepairNode - hardcoded to be the last node in the array...
        mountPointNode[AIRepairNode] = shape->findNode("AIRepairNode");

        //
        hulkSequence = shape->findSequence("Visibility");
        damageSequence = shape->findSequence("Damage");

        //
        F32 w = shape->bounds.len_y() / 2;
        if (cameraMaxDist < w)
            cameraMaxDist = w;
    }

    if (!server)
    {
        /*
              // grab all the hud images
              for(U32 i = 0; i < NumHudRenderImages; i++)
              {
                 if(hudImageNameFriendly[i] && hudImageNameFriendly[i][0])
                    hudImageFriendly[i] = TextureHandle(hudImageNameFriendly[i], BitmapTexture);

                 if(hudImageNameEnemy[i] && hudImageNameEnemy[i][0])
                    hudImageEnemy[i] = TextureHandle(hudImageNameEnemy[i], BitmapTexture);
              }
        */
    }

    return !shapeError;
}


void ShapeBaseData::initPersistFields()
{
    Parent::initPersistFields();


    addGroup("Shadows");
    addField("shadowEnable", TypeBool, Offset(shadowEnable, ShapeBaseData));
    addField("shadowCanMove", TypeBool, Offset(shadowCanMove, ShapeBaseData));
    addField("shadowCanAnimate", TypeBool, Offset(shadowCanAnimate, ShapeBaseData));
    addField("shadowSelfShadow", TypeBool, Offset(shadowSelfShadow, ShapeBaseData));
    addField("shadowSize", TypeS32, Offset(shadowSize, ShapeBaseData));
    addField("shadowAnimationFrameSkip", TypeS32, Offset(shadowAnimationFrameSkip, ShapeBaseData));
    addField("shadowMaxVisibleDistance", TypeF32, Offset(shadowMaxVisibleDistance, ShapeBaseData));
    addField("shadowProjectionDistance", TypeF32, Offset(shadowProjectionDistance, ShapeBaseData));
    addField("shadowSphereAdjust", TypeF32, Offset(shadowSphereAdjust, ShapeBaseData));
    addField("shadowBias", TypeF32, Offset(shadowBias, ShapeBaseData));
    endGroup("Shadows");


    addGroup("Render");
    addField("shapeFile", TypeFilename, Offset(shapeName, ShapeBaseData));
    addField("emap", TypeBool, Offset(emap, ShapeBaseData));

#ifdef MB_ULTRA
    addField("referenceColor", TypeColorF, Offset(referenceColor, ShapeBaseData));
#endif

    endGroup("Render");

    addGroup("Destruction", "Parameters related to the destruction effects of this object.");
    addField("explosion", TypeExplosionDataPtr, Offset(explosion, ShapeBaseData));
    addField("underwaterExplosion", TypeExplosionDataPtr, Offset(underwaterExplosion, ShapeBaseData));
    addField("debris", TypeDebrisDataPtr, Offset(debris, ShapeBaseData));
    addField("renderWhenDestroyed", TypeBool, Offset(renderWhenDestroyed, ShapeBaseData));
    addField("debrisShapeName", TypeFilename, Offset(debrisShapeName, ShapeBaseData));
    endGroup("Destruction");

    addGroup("Physics");
    addField("mass", TypeF32, Offset(mass, ShapeBaseData));
    addField("drag", TypeF32, Offset(drag, ShapeBaseData));
    addField("density", TypeF32, Offset(density, ShapeBaseData));
    endGroup("Physics");

    addGroup("Damage/Energy");
    addField("maxEnergy", TypeF32, Offset(maxEnergy, ShapeBaseData));
    addField("maxDamage", TypeF32, Offset(maxDamage, ShapeBaseData));
    addField("disabledLevel", TypeF32, Offset(disabledLevel, ShapeBaseData));
    addField("destroyedLevel", TypeF32, Offset(destroyedLevel, ShapeBaseData));
    addField("repairRate", TypeF32, Offset(repairRate, ShapeBaseData));
    addField("inheritEnergyFromMount", TypeBool, Offset(inheritEnergyFromMount, ShapeBaseData));
    addField("isInvincible", TypeBool, Offset(isInvincible, ShapeBaseData));
    endGroup("Damage/Energy");

    addGroup("Camera");
    addField("cameraMaxDist", TypeF32, Offset(cameraMaxDist, ShapeBaseData));
    addField("cameraMinDist", TypeF32, Offset(cameraMinDist, ShapeBaseData));
    addField("cameraDefaultFov", TypeF32, Offset(cameraDefaultFov, ShapeBaseData));
    addField("cameraMinFov", TypeF32, Offset(cameraMinFov, ShapeBaseData));
    addField("cameraMaxFov", TypeF32, Offset(cameraMaxFov, ShapeBaseData));
    addField("firstPersonOnly", TypeBool, Offset(firstPersonOnly, ShapeBaseData));
    addField("useEyePoint", TypeBool, Offset(useEyePoint, ShapeBaseData));
    addField("observeThroughObject", TypeBool, Offset(observeThroughObject, ShapeBaseData));
    endGroup("Camera");

    // This hud code is going to get ripped out soon...
    addGroup("HUD", "@deprecated Likely to be removed soon.");
    addField("hudImageName", TypeFilename, Offset(hudImageNameFriendly, ShapeBaseData), NumHudRenderImages);
    addField("hudImageNameFriendly", TypeFilename, Offset(hudImageNameFriendly, ShapeBaseData), NumHudRenderImages);
    addField("hudImageNameEnemy", TypeFilename, Offset(hudImageNameEnemy, ShapeBaseData), NumHudRenderImages);
    addField("hudRenderCenter", TypeBool, Offset(hudRenderCenter, ShapeBaseData), NumHudRenderImages);
    addField("hudRenderModulated", TypeBool, Offset(hudRenderModulated, ShapeBaseData), NumHudRenderImages);
    addField("hudRenderAlways", TypeBool, Offset(hudRenderAlways, ShapeBaseData), NumHudRenderImages);
    addField("hudRenderDistance", TypeBool, Offset(hudRenderDistance, ShapeBaseData), NumHudRenderImages);
    addField("hudRenderName", TypeBool, Offset(hudRenderName, ShapeBaseData), NumHudRenderImages);
    endGroup("HUD");

    addGroup("Misc");
    addField("aiAvoidThis", TypeBool, Offset(aiAvoidThis, ShapeBaseData));
    addField("computeCRC", TypeBool, Offset(computeCRC, ShapeBaseData));
    addField("dynamicReflection", TypeBool, Offset(dynamicReflection, ShapeBaseData));
    endGroup("Misc");

#ifdef MB_ULTRA
    addField("renderGemAura", TypeBool, Offset(renderGemAura, ShapeBaseData));
    addField("glass", TypeBool, Offset(glass, ShapeBaseData));
    addField("astrolabe", TypeBool, Offset(astrolabe, ShapeBaseData));
    addField("astrolabePrime", TypeBool, Offset(astrolabePrime, ShapeBaseData));
    addField("gemAuraTextureName", TypeFilename, Offset(gemAuraTextureName, ShapeBaseData));
#endif

}

ConsoleMethod(ShapeBaseData, checkDeployPos, bool, 3, 3, "(Transform xform)")
{
    if (bool(object->shape) == false)
        return false;

    Point3F pos(0, 0, 0);
    AngAxisF aa(Point3F(0, 0, 1), 0);
    dSscanf(argv[2], "%g %g %g %g %g %g %g",
        &pos.x, &pos.y, &pos.z, &aa.axis.x, &aa.axis.y, &aa.axis.z, &aa.angle);
    MatrixF mat;
    aa.setMatrix(&mat);
    mat.setColumn(3, pos);

    Box3F objBox = object->shape->bounds;
    Point3F boxCenter = (objBox.min + objBox.max) * 0.5;
    objBox.min = boxCenter + (objBox.min - boxCenter) * 0.9;
    objBox.max = boxCenter + (objBox.max - boxCenter) * 0.9;

    Box3F wBox = objBox;
    mat.mul(wBox);

    EarlyOutPolyList polyList;
    polyList.mNormal.set(0, 0, 0);
    polyList.mPlaneList.clear();
    polyList.mPlaneList.setSize(6);
    polyList.mPlaneList[0].set(objBox.min, VectorF(-1, 0, 0));
    polyList.mPlaneList[1].set(objBox.max, VectorF(0, 1, 0));
    polyList.mPlaneList[2].set(objBox.max, VectorF(1, 0, 0));
    polyList.mPlaneList[3].set(objBox.min, VectorF(0, -1, 0));
    polyList.mPlaneList[4].set(objBox.min, VectorF(0, 0, -1));
    polyList.mPlaneList[5].set(objBox.max, VectorF(0, 0, 1));

    for (U32 i = 0; i < 6; i++)
    {
        PlaneF temp;
        mTransformPlane(mat, Point3F(1, 1, 1), polyList.mPlaneList[i], &temp);
        polyList.mPlaneList[i] = temp;
    }

    if (gServerContainer.buildPolyList(wBox, InteriorObjectType | StaticShapeObjectType, &polyList))
        return false;
    return true;
}


ConsoleMethod(ShapeBaseData, getDeployTransform, const char*, 4, 4, "(Point3F pos, Point3F normal)")
{
    Point3F normal;
    Point3F position;
    dSscanf(argv[2], "%g %g %g", &position.x, &position.y, &position.z);
    dSscanf(argv[3], "%g %g %g", &normal.x, &normal.y, &normal.z);
    normal.normalize();

    VectorF xAxis;
    if (mFabs(normal.z) > mFabs(normal.x) && mFabs(normal.z) > mFabs(normal.y))
        mCross(VectorF(0, 1, 0), normal, &xAxis);
    else
        mCross(VectorF(0, 0, 1), normal, &xAxis);

    VectorF yAxis;
    mCross(normal, xAxis, &yAxis);

    MatrixF testMat(true);
    testMat.setColumn(0, xAxis);
    testMat.setColumn(1, yAxis);
    testMat.setColumn(2, normal);
    testMat.setPosition(position);

    char* returnBuffer = Con::getReturnBuffer(256);
    Point3F pos;
    testMat.getColumn(3, &pos);
    AngAxisF aa(testMat);
    dSprintf(returnBuffer, 256, "%g %g %g %g %g %g %g",
        pos.x, pos.y, pos.z, aa.axis.x, aa.axis.y, aa.axis.z, aa.angle);
    return returnBuffer;
}

void ShapeBaseData::packData(BitStream* stream)
{
    Parent::packData(stream);

    if (stream->writeFlag(computeCRC))
        stream->write(mCRC);


    stream->writeFlag(shadowEnable);
    stream->writeFlag(shadowCanMove);
    stream->writeFlag(shadowCanAnimate);
    stream->writeFlag(shadowSelfShadow);
    stream->write(shadowSize);
    stream->write(shadowAnimationFrameSkip);
    stream->write(shadowMaxVisibleDistance);
    stream->write(shadowProjectionDistance);
    stream->write(shadowSphereAdjust);
    stream->write(shadowBias);


    stream->writeString(shapeName);
    stream->writeString(cloakTexName);
    if (stream->writeFlag(mass != gShapeBaseDataProto.mass))
        stream->write(mass);
    if (stream->writeFlag(drag != gShapeBaseDataProto.drag))
        stream->write(drag);
    if (stream->writeFlag(density != gShapeBaseDataProto.density))
        stream->write(density);
    if (stream->writeFlag(maxEnergy != gShapeBaseDataProto.maxEnergy))
        stream->write(maxEnergy);
    if (stream->writeFlag(cameraMaxDist != gShapeBaseDataProto.cameraMaxDist))
        stream->write(cameraMaxDist);
    if (stream->writeFlag(cameraMinDist != gShapeBaseDataProto.cameraMinDist))
        stream->write(cameraMinDist);
    cameraDefaultFov = mClampF(cameraDefaultFov, cameraMinFov, cameraMaxFov);
    if (stream->writeFlag(cameraDefaultFov != gShapeBaseDataProto.cameraDefaultFov))
        stream->write(cameraDefaultFov);
    if (stream->writeFlag(cameraMinFov != gShapeBaseDataProto.cameraMinFov))
        stream->write(cameraMinFov);
    if (stream->writeFlag(cameraMaxFov != gShapeBaseDataProto.cameraMaxFov))
        stream->write(cameraMaxFov);
    stream->writeString(debrisShapeName);

    stream->writeFlag(observeThroughObject);

    if (stream->writeFlag(debris != NULL))
    {
        stream->writeRangedU32(packed ? static_cast<SimObjectId>(reinterpret_cast<size_t>(debris)) :
            debris->getId(), DataBlockObjectIdFirst, DataBlockObjectIdLast);
    }

    stream->writeFlag(emap);
    stream->writeFlag(isInvincible);
    stream->writeFlag(renderWhenDestroyed);

    if (stream->writeFlag(explosion != NULL))
    {
        stream->writeRangedU32(explosion->getId(), DataBlockObjectIdFirst, DataBlockObjectIdLast);
    }

    if (stream->writeFlag(underwaterExplosion != NULL))
    {
        stream->writeRangedU32(underwaterExplosion->getId(), DataBlockObjectIdFirst, DataBlockObjectIdLast);
    }

    stream->writeFlag(inheritEnergyFromMount);
    stream->writeFlag(firstPersonOnly);
    stream->writeFlag(useEyePoint);
    stream->writeFlag(dynamicReflection);

#ifdef MB_ULTRA
    stream->writeFlag(renderGemAura);
    stream->writeFlag(glass);
    stream->writeFlag(astrolabe);
    stream->writeFlag(astrolabePrime);
    stream->writeString(gemAuraTextureName, 255);
#endif
}

void ShapeBaseData::unpackData(BitStream* stream)
{
    Parent::unpackData(stream);
    computeCRC = stream->readFlag();
    if (computeCRC)
        stream->read(&mCRC);


    shadowEnable = stream->readFlag();
    shadowCanMove = stream->readFlag();
    shadowCanAnimate = stream->readFlag();
    shadowSelfShadow = stream->readFlag();
    stream->read(&shadowSize);
    stream->read(&shadowAnimationFrameSkip);
    stream->read(&shadowMaxVisibleDistance);
    stream->read(&shadowProjectionDistance);
    stream->read(&shadowSphereAdjust);
    stream->read(&shadowBias);


    shapeName = stream->readSTString();
    cloakTexName = stream->readSTString();
    if (stream->readFlag())
        stream->read(&mass);
    else
        mass = gShapeBaseDataProto.mass;

    if (stream->readFlag())
        stream->read(&drag);
    else
        drag = gShapeBaseDataProto.drag;

    if (stream->readFlag())
        stream->read(&density);
    else
        density = gShapeBaseDataProto.density;

    if (stream->readFlag())
        stream->read(&maxEnergy);
    else
        maxEnergy = gShapeBaseDataProto.maxEnergy;

    if (stream->readFlag())
        stream->read(&cameraMaxDist);
    else
        cameraMaxDist = gShapeBaseDataProto.cameraMaxDist;

    if (stream->readFlag())
        stream->read(&cameraMinDist);
    else
        cameraMinDist = gShapeBaseDataProto.cameraMinDist;

    if (stream->readFlag())
        stream->read(&cameraDefaultFov);
    else
        cameraDefaultFov = gShapeBaseDataProto.cameraDefaultFov;

    if (stream->readFlag())
        stream->read(&cameraMinFov);
    else
        cameraMinFov = gShapeBaseDataProto.cameraMinFov;

    if (stream->readFlag())
        stream->read(&cameraMaxFov);
    else
        cameraMaxFov = gShapeBaseDataProto.cameraMaxFov;

    debrisShapeName = stream->readSTString();

    observeThroughObject = stream->readFlag();

    if (stream->readFlag())
    {
        debrisID = stream->readRangedU32(DataBlockObjectIdFirst, DataBlockObjectIdLast);
    }

    emap = stream->readFlag();
    isInvincible = stream->readFlag();
    renderWhenDestroyed = stream->readFlag();

    if (stream->readFlag())
    {
        explosionID = stream->readRangedU32(DataBlockObjectIdFirst, DataBlockObjectIdLast);
    }

    if (stream->readFlag())
    {
        underwaterExplosionID = stream->readRangedU32(DataBlockObjectIdFirst, DataBlockObjectIdLast);
    }

    inheritEnergyFromMount = stream->readFlag();
    firstPersonOnly = stream->readFlag();
    useEyePoint = stream->readFlag();
    dynamicReflection = stream->readFlag();

#ifdef MB_ULTRA
    renderGemAura = stream->readFlag();
    glass = stream->readFlag();
    astrolabe = stream->readFlag();
    astrolabePrime = stream->readFlag();
    gemAuraTextureName = stream->readSTString(false);
    gemAuraTexture.set(gemAuraTextureName, &GFXDefaultStaticDiffuseProfile);
#endif
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

Chunker<ShapeBase::CollisionTimeout> sTimeoutChunker;
ShapeBase::CollisionTimeout* ShapeBase::sFreeTimeoutList = 0;


//----------------------------------------------------------------------------

IMPLEMENT_CO_NETOBJECT_V1(ShapeBase);

ShapeBase::ShapeBase()
{
    mTypeMask |= ShapeBaseObjectType;

    mDrag = 0;
    mBuoyancy = 0;
    mWaterCoverage = 0;
    mLiquidType = 0;
    mLiquidHeight = 0.0f;
    //mControllingClient = 0;
    mControllingObject = 0;

    mGravityMod = 1.0;
    mAppliedForce.set(0, 0, 0);

    mTimeoutList = 0;
    mDataBlock = NULL;
    mShapeInstance = 0;
    mEnergy = 0;
    mRechargeRate = 0;
    mDamage = 0;
    mRepairRate = 0;
    mRepairReserve = 0;
    mDamageState = Enabled;
    mDamageThread = 0;
    mHulkThread = 0;
    mLastRenderFrame = 0;
    mLastRenderDistance = 0;

    mCloaked = false;
    mCloakLevel = 0.0;

    mMount.object = 0;
    mMount.link = 0;
    mMount.list = 0;

    mHidden = false;

    for (int a = 0; a < MaxSoundThreads; a++) {
        mSoundThread[a].play = false;
        mSoundThread[a].profile = 0;
        mSoundThread[a].sound = 0;
    }

    S32 i;
    for (i = 0; i < MaxScriptThreads; i++) {
        mScriptThread[i].sequence = -1;
        mScriptThread[i].thread = 0;
        mScriptThread[i].sound = 0;
        mScriptThread[i].state = Thread::Stop;
        mScriptThread[i].atEnd = false;
        mScriptThread[i].forward = true;
        mScriptThread[i].id = 0;
    }

    for (i = 0; i < MaxTriggerKeys; i++)
        mTrigger[i] = false;

    mDamageFlash = 0.0;
    mWhiteOut = 0.0;

    mInvincibleEffect = 0.0f;
    mInvincibleDelta = 0.0f;
    mInvincibleCount = 0.0f;
    mInvincibleSpeed = 0.0f;
    mInvincibleTime = 0.0f;
    mInvincibleFade = 0.1;
    mInvincibleOn = false;

    mIsControlled = false;

    mConvexList = new Convex;
    mCameraFov = 90.f;
    mShieldNormal.set(0, 0, 1);

    mFadeOut = true;
    mFading = false;
    mFadeVal = 1.0;
    mFadeTime = 1.0;
    mFadeElapsedTime = 0.0;
    mFadeDelay = 0.0;
    mFlipFadeVal = false;
    mLightTime = 0;
    damageDir.set(0, 0, 1);

#ifdef MB_ULTRA
    mRenderScale = Point3F(1.0f, 1.0f, 1.0f);
#endif

    mDynamicCubemap = NULL;
}


ShapeBase::~ShapeBase()
{
    delete mConvexList;
    mConvexList = NULL;

    AssertFatal(mMount.link == 0, "ShapeBase::~ShapeBase: An object is still mounted");
    if (mShapeInstance && (mShapeInstance->getDebrisRefCount() == 0))
    {
        delete mShapeInstance;
    }

    CollisionTimeout* ptr = mTimeoutList;
    while (ptr) {
        CollisionTimeout* cur = ptr;
        ptr = ptr->next;
        cur->next = sFreeTimeoutList;
        sFreeTimeoutList = cur;
    }

    /*if (mDynamicCubemap)
    {
        delete mDynamicCubemap;
        mDynamicCubemap = NULL;
    }*/
}


//----------------------------------------------------------------------------

bool ShapeBase::onAdd()
{
    if (!Parent::onAdd())
        return false;

    mCreateTime = Platform::getVirtualMilliseconds();

    // Resolve sounds that arrived in the initial update
    S32 i;
    for (i = 0; i < MaxSoundThreads; i++)
        updateAudioState(mSoundThread[i]);

    for (i = 0; i < MaxScriptThreads; i++)
    {
        Thread& st = mScriptThread[i];
        if (st.thread)
            updateThread(st);
    }

    if (isClientObject() || gSPMode)
    {
        if (mDataBlock->dynamicReflection)
        {
            SimSet* reflectSet = dynamic_cast<SimSet*>(Sim::findObject("reflectiveSet"));
            reflectSet->addObject(this);

            mDynamicCubemap = GFX->createCubemap();
            mDynamicCubemap->initDynamic(256);
        }

        /*
              if(mDataBlock->cloakTexName != StringTable->insert(""))
                mCloakTexture = TextureHandle(mDataBlock->cloakTexName, MeshTexture, false);
        */
        //one of the mounted images must have a light source...
        for (S32 i = 0; i < MaxMountedImages; i++)
        {
            ShapeBaseImageData* imageData = getMountedImage(i);
            if (imageData != NULL && imageData->lightType != ShapeBaseImageData::NoLight)
            {
                Sim::getLightSet()->addObject(this);
                break;
            }
        }
    }

    return true;
}

void ShapeBase::onRemove()
{
    mConvexList->nukeList();

    unmount();
    Parent::onRemove();

    // Stop any running sounds on the client
    if (gSPMode || isGhost())
        for (S32 i = 0; i < MaxSoundThreads; i++)
            stopAudio(i);
}


void ShapeBase::onSceneRemove()
{
    mConvexList->nukeList();
    Parent::onSceneRemove();
}

bool ShapeBase::onNewDataBlock(GameBaseData* dptr)
{
    if (Parent::onNewDataBlock(dptr) == false)
        return false;

    mDataBlock = dynamic_cast<ShapeBaseData*>(dptr);
    if (!mDataBlock)
        return false;

    setMaskBits(DamageMask);
    mDamageThread = 0;
    mHulkThread = 0;

    // Even if loadShape succeeds, there may not actually be
    // a shape assigned to this object.
    if (bool(mDataBlock->shape)) {
        delete mShapeInstance;
        mShapeInstance = new TSShapeInstance(mDataBlock->shape, isClientObject() || gSPMode);
        if (isClientObject() || gSPMode)
            mShapeInstance->cloneMaterialList();

        mObjBox = mDataBlock->shape->bounds;
        resetWorldBox();

        // Initialize the threads
        for (U32 i = 0; i < MaxScriptThreads; i++) {
            Thread& st = mScriptThread[i];
            if (st.sequence != -1) {
                // TG: Need to see about supressing non-cyclic sounds
                // if the sequences were actived before the object was
                // ghosted.
                // TG: Cyclic animations need to have a random pos if
                // they were started before the object was ghosted.

                // If there was something running on the old shape, the thread
                // needs to be reset. Otherwise we assume that it's been
                // initialized either by the constructor or from the server.
                bool reset = st.thread != 0;
                st.thread = 0;
                setThreadSequence(i, st.sequence, reset);
            }
        }

        if (mDataBlock->damageSequence != -1) {
            mDamageThread = mShapeInstance->addThread();
            mShapeInstance->setSequence(mDamageThread,
                mDataBlock->damageSequence, 0);
        }
        if (mDataBlock->hulkSequence != -1) {
            mHulkThread = mShapeInstance->addThread();
            mShapeInstance->setSequence(mHulkThread,
                mDataBlock->hulkSequence, 0);
        }
    }

    if ((gSPMode || isGhost()) && mSkinNameHandle.isValidString() && mShapeInstance) {

        mShapeInstance->reSkin(mSkinNameHandle);

        mSkinHash = _StringTable::hashString(mSkinNameHandle.getString());

    }

    //
    mEnergy = 0;
    mDamage = 0;
    mDamageState = Enabled;
    mRepairReserve = 0;
    updateMass();
    updateDamageLevel();
    updateDamageState();

    mDrag = mDataBlock->drag;
    mCameraFov = mDataBlock->cameraDefaultFov;
    return true;
}

void ShapeBase::onDeleteNotify(SimObject* obj)
{
    if (obj == getProcessAfter())
        clearProcessAfter();
    Parent::onDeleteNotify(obj);
    if (obj == mMount.object)
        unmount();
}

void ShapeBase::onImpact(SceneObject* obj, VectorF vec)
{
    if (!isGhost()) {
        char buff1[256];
        char buff2[32];

        dSprintf(buff1, sizeof(buff1), "%g %g %g", vec.x, vec.y, vec.z);
        dSprintf(buff2, sizeof(buff2), "%g", vec.len());
        Con::executef(mDataBlock, 5, "onImpact", scriptThis(), obj->getIdString(), buff1, buff2);
    }
}

void ShapeBase::onImpact(VectorF vec)
{
    if (!isGhost()) {
        char buff1[256];
        char buff2[32];

        dSprintf(buff1, sizeof(buff1), "%g %g %g", vec.x, vec.y, vec.z);
        dSprintf(buff2, sizeof(buff2), "%g", vec.len());
        Con::executef(mDataBlock, 5, "onImpact", scriptThis(), "0", buff1, buff2);
    }
}


//----------------------------------------------------------------------------

void ShapeBase::processTick(const Move* move)
{
    // Energy management
    if (mDamageState == Enabled && mDataBlock->inheritEnergyFromMount == false) {
        F32 store = mEnergy;
        mEnergy += mRechargeRate;
        if (mEnergy > mDataBlock->maxEnergy)
            mEnergy = mDataBlock->maxEnergy;
        else
            if (mEnergy < 0)
                mEnergy = 0;

        // Virtual setEnergyLevel is used here by some derived classes to
        // decide whether they really want to set the energy mask bit.
        if (mEnergy != store)
            setEnergyLevel(mEnergy);
    }

    // Repair management
    if (mDataBlock->isInvincible == false)
    {
        F32 store = mDamage;
        mDamage -= mRepairRate;
        mDamage = mClampF(mDamage, 0.f, mDataBlock->maxDamage);

        if (mRepairReserve > mDamage)
            mRepairReserve = mDamage;
        if (mRepairReserve > 0.0)
        {
            F32 rate = getMin(mDataBlock->repairRate, mRepairReserve);
            mDamage -= rate;
            mRepairReserve -= rate;
        }

        if (store != mDamage)
        {
            updateDamageLevel();
            if (isServerObject()) {
                char delta[100];
                dSprintf(delta, sizeof(delta), "%g", mDamage - store);
                setMaskBits(DamageMask);
                Con::executef(mDataBlock, 3, "onDamage", scriptThis(), delta);
            }
        }
    }

    if (isServerObject()) {
        // Server only...
        advanceThreads(TickSec);
        updateServerAudio();

        // update wet state
        setImageWetState(0, mWaterCoverage > 0.4); // more than 40 percent covered

        if (mFading)
        {
            F32 dt = TickMs / 1000.0;
            F32 newFadeET = mFadeElapsedTime + dt;
            if (mFadeElapsedTime < mFadeDelay && newFadeET >= mFadeDelay)
                setMaskBits(CloakMask);
            mFadeElapsedTime = newFadeET;
            if (mFadeElapsedTime > mFadeTime + mFadeDelay)
            {
                mFadeVal = F32(!mFadeOut);
                mFading = false;
            }
        }
    }

    // Advance images
    for (int i = 0; i < MaxMountedImages; i++)
    {
        if (mMountedImageList[i].dataBlock != NULL)
            updateImageState(i, TickSec);
    }

    // Call script on trigger state changes
    if (move && mDataBlock && isServerObject()) {
        for (S32 i = 0; i < MaxTriggerKeys; i++) {
            if (move->trigger[i] != mTrigger[i]) {
                mTrigger[i] = move->trigger[i];
                char buf1[20], buf2[20];
                dSprintf(buf1, sizeof(buf1), "%d", i);
                dSprintf(buf2, sizeof(buf2), "%d", (move->trigger[i] ? 1 : 0));
                Con::executef(mDataBlock, 4, "onTrigger", scriptThis(), buf1, buf2);
            }
        }
    }

    // Update the damage flash and the whiteout
    //
    if (mDamageFlash > 0.0)
    {
        mDamageFlash -= sDamageFlashDec;
        if (mDamageFlash <= 0.0)
            mDamageFlash = 0.0;
    }
    if (mWhiteOut > 0.0)
    {
        mWhiteOut -= sWhiteoutDec;
        if (mWhiteOut <= 0.0)
            mWhiteOut = 0.0;
    }

    if (mThreadCmd.delayTicks >= 0)
    {
        mThreadCmd.delayTicks--;
        if (mThreadCmd.delayTicks < 0)
        {
            if (setThreadSequence(mThreadCmd.slot, mThreadCmd.seq, true, 1.0f))
                setTimeScale(mThreadCmd.slot, mThreadCmd.timeScale);
            setThreadDir(mThreadCmd.slot, mThreadCmd.timeScale > 0.0f);
        }
    }
}

void ShapeBase::advanceTime(F32 dt)
{
    // On the client, the shape threads and images are
    // advanced at framerate.
    if (!gSPMode)
        advanceThreads(dt);
    updateAudioPos();
    for (int i = 0; i < MaxMountedImages; i++)
        if (mMountedImageList[i].dataBlock)
            updateImageAnimation(i, dt);

    // Cloaking takes 0.5 seconds
    if (mCloaked && mCloakLevel != 1.0) {
        mCloakLevel += dt * 2;
        if (mCloakLevel >= 1.0)
            mCloakLevel = 1.0;
    }
    else if (!mCloaked && mCloakLevel != 0.0) {
        mCloakLevel -= dt * 2;
        if (mCloakLevel <= 0.0)
            mCloakLevel = 0.0;
    }
    if (mInvincibleOn)
        updateInvincibleEffect(dt);

    if (mFading && !gSPMode) // why disable this for spmode? - Matt
    {
        mFadeElapsedTime += dt;
        if (mFadeElapsedTime > mFadeTime)
        {
            mFadeVal = F32(!mFadeOut);
            mFading = false;
        }
        else
        {
            mFadeVal = mFadeElapsedTime / mFadeTime;
            if (mFadeOut)
                mFadeVal = 1 - mFadeVal;
        }
    }

#ifdef MB_ULTRA
    if (mAnimateScale)
    {
        float scale = mDot(mRenderScale, mRenderScale);
        if (scale <= mDot(mObjScale, mObjScale))
            scale = 0.1f;
        else
            scale = 0.4f;

        scale = dt / scale * 2.302585124969482;
        scale = 1.0 / (scale * (scale * 0.2349999994039536 * scale) + scale + 1.0 + 0.4799999892711639 * scale * scale);
        mRenderScale *= scale;
        scale = 1.0 - scale;
        mRenderScale += scale * mObjScale;
    }
#endif
}


//----------------------------------------------------------------------------

//void ShapeBase::setControllingClient(GameConnection* client)
//{
//   mControllingClient = client;
//
//   // piggybacks on the cloak update
//   setMaskBits(CloakMask);
//}

void ShapeBase::setControllingObject(ShapeBase* obj)
{
    if (obj) {
        setProcessTick(false);
        // Even though we don't processTick, we still need to
        // process after the controller in case anyone is mounted
        // on this object.
        processAfter(obj);
    }
    else {
        setProcessTick(true);
        clearProcessAfter();
        // Catch the case of the controlling object actually
        // mounted on this object.
        if (mControllingObject->mMount.object == this)
            mControllingObject->processAfter(this);
    }
    mControllingObject = obj;
}

ShapeBase* ShapeBase::getControlObject()
{
    return 0;
}

void ShapeBase::setControlObject(ShapeBase*)
{
}

bool ShapeBase::isFirstPerson()
{
    // Always first person as far as the server is concerned.
    if (!isGhost())
        return true;

    if (GameConnection* con = getControllingClient())
        return con->getControlObject() == this && con->isFirstPerson();
    return false;
}

// Camera: (in degrees) ------------------------------------------------------
F32 ShapeBase::getCameraFov()
{
    return(mCameraFov);
}

F32 ShapeBase::getDefaultCameraFov()
{
    return(mDataBlock->cameraDefaultFov);
}

bool ShapeBase::isValidCameraFov(F32 fov)
{
    return((fov >= mDataBlock->cameraMinFov) && (fov <= mDataBlock->cameraMaxFov));
}

void ShapeBase::setCameraFov(F32 fov)
{
    mCameraFov = mClampF(fov, mDataBlock->cameraMinFov, mDataBlock->cameraMaxFov);
}

//----------------------------------------------------------------------------
static void scopeCallback(SceneObject* obj, void* conPtr)
{
    NetConnection* ptr = reinterpret_cast<NetConnection*>(conPtr);
    if (obj->isScopeable())
        ptr->objectInScope(obj);
}

void ShapeBase::onCameraScopeQuery(NetConnection* cr, CameraScopeQuery* query)
{
    // update the camera query
    query->camera = this;
    // bool grabEye = true;
    if (GameConnection* con = dynamic_cast<GameConnection*>(cr))
    {
        // get the fov from the connection (in deg)
        F32 fov;
        if (con->getControlCameraFov(&fov))
        {
            query->fov = mDegToRad(fov / 2);
            query->sinFov = mSin(query->fov);
            query->cosFov = mCos(query->fov);
        }
    }

    // failed to query the camera info?
    // if(grabEye)    LH - always use eye as good enough, avoid camera animate
    {
        MatrixF eyeTransform;
        getEyeTransform(&eyeTransform);
        eyeTransform.getColumn(3, &query->pos);
        eyeTransform.getColumn(1, &query->orientation);
    }

    // grab the visible distance from the sky
    Sky* sky = getCurrentServerSceneGraph()->getCurrentSky();
    if (sky)
        query->visibleDistance = sky->getVisibleDistance();
    else
        query->visibleDistance = 1000.f;

    // First, we are certainly in scope, and whatever we're riding is too...
    cr->objectInScope(this);
    if (isMounted())
        cr->objectInScope(mMount.object);

    if (mSceneManager == NULL)
    {
        // Scope everything...
        gServerContainer.findObjects(0xFFFFFFFF, scopeCallback, cr);
        return;
    }

    // update the scenemanager
    mSceneManager->scopeScene(query->pos, query->visibleDistance, cr);

    // let the (game)connection do some scoping of its own (commandermap...)
    cr->doneScopingScene();
}


//----------------------------------------------------------------------------
F32 ShapeBase::getEnergyLevel()
{
    if (mDataBlock->inheritEnergyFromMount == false)
        return mEnergy;
    else if (isMounted()) {
        return getObjectMount()->getEnergyLevel();
    }
    else {
        return 0.0f;
    }
}

F32 ShapeBase::getEnergyValue()
{
    if (mDataBlock->inheritEnergyFromMount == false) {
        F32 maxEnergy = mDataBlock->maxEnergy;
        if (maxEnergy > 0.f)
            return (mEnergy / mDataBlock->maxEnergy);
    }
    else if (isMounted()) {
        F32 maxEnergy = getObjectMount()->mDataBlock->maxEnergy;
        if (maxEnergy > 0.f)
            return (getObjectMount()->getEnergyLevel() / maxEnergy);
    }
    return 0.0f;
}

void ShapeBase::setEnergyLevel(F32 energy)
{
    if (mDataBlock->inheritEnergyFromMount == false) {
        if (mDamageState == Enabled) {
            mEnergy = (energy > mDataBlock->maxEnergy) ?
                mDataBlock->maxEnergy : (energy < 0) ? 0 : energy;
        }
    }
    else {
        // Pass the set onto whatever we're mounted to...
        if (isMounted())
            getObjectMount()->setEnergyLevel(energy);
    }
}

void ShapeBase::setDamageLevel(F32 damage)
{
    if (!mDataBlock->isInvincible) {
        F32 store = mDamage;
        mDamage = mClampF(damage, 0.f, mDataBlock->maxDamage);

        if (store != mDamage) {
            updateDamageLevel();
            if (isServerObject()) {
                setMaskBits(DamageMask);
                char delta[100];
                dSprintf(delta, sizeof(delta), "%g", mDamage - store);
                Con::executef(mDataBlock, 3, "onDamage", scriptThis(), delta);
            }
        }
    }
}

//----------------------------------------------------------------------------

static F32 sWaterDensity = 1;
static F32 sWaterViscosity = 15;
static F32 sWaterCoverage = 0;
static U32 sWaterType = 0;
static F32 sWaterHeight = 0.0f;

static void waterFind(SceneObject* obj, void* key)
{
    /*
       ShapeBase* shape = reinterpret_cast<ShapeBase*>(key);
       WaterBlock* wb   = dynamic_cast<WaterBlock*>(obj);
       AssertFatal(wb != NULL, "Error, not a water block!");
       if (wb == NULL) {
          sWaterCoverage = 0;
          return;
       }

       const Box3F& wbox = obj->getWorldBox();
       const Box3F& sbox = shape->getWorldBox();
       sWaterType = 0;
       if (wbox.isOverlapped(sbox)) {
          sWaterType = wb->getLiquidType();
          if (wbox.max.z < sbox.max.z)
             sWaterCoverage = (wbox.max.z - sbox.min.z) / (sbox.max.z - sbox.min.z);
          else
             sWaterCoverage = 1;

          sWaterViscosity = wb->getViscosity();
          sWaterDensity   = wb->getDensity();
          sWaterHeight    = wb->getSurfaceHeight();
       }
    */
}

void physicalZoneFind(SceneObject* obj, void* key)
{
    ShapeBase* shape = reinterpret_cast<ShapeBase*>(key);
    PhysicalZone* pz = dynamic_cast<PhysicalZone*>(obj);
    AssertFatal(pz != NULL, "Error, not a physical zone!");
    if (pz == NULL || pz->testObject(shape) == false) {
        return;
    }

    if (pz->isActive()) {
        shape->mGravityMod *= pz->getGravityMod();
        shape->mAppliedForce += pz->getForce();
    }
}

void findRouter(SceneObject* obj, void* key)
{
    if (obj->getTypeMask() & WaterObjectType)
        waterFind(obj, key);
    else if (obj->getTypeMask() & PhysicalZoneObjectType)
        physicalZoneFind(obj, key);
    else {
        AssertFatal(false, "Error, must be either water or physical zone here!");
    }
}

void ShapeBase::updateContainer()
{
    // Update container drag and buoyancy properties
    mDrag = mDataBlock->drag;
    mBuoyancy = 0;
    sWaterCoverage = 0;
    mGravityMod = 1.0;
    mAppliedForce.set(0, 0, 0);
    mContainer->findObjects(getWorldBox(), WaterObjectType | PhysicalZoneObjectType, findRouter, this);
    sWaterCoverage = mClampF(sWaterCoverage, 0, 1);
    mWaterCoverage = sWaterCoverage;
    mLiquidType = sWaterType;
    mLiquidHeight = sWaterHeight;
    if (mWaterCoverage >= 0.1f) {
        mDrag = mDataBlock->drag * sWaterViscosity * sWaterCoverage;
        mBuoyancy = (sWaterDensity / mDataBlock->density) * sWaterCoverage;
    }
}


//----------------------------------------------------------------------------

void ShapeBase::applyRepair(F32 amount)
{
    // Repair increases the repair reserve
    if (amount > 0 && ((mRepairReserve += amount) > mDamage))
        mRepairReserve = mDamage;
}

void ShapeBase::applyDamage(F32 amount)
{
    if (amount > 0)
        setDamageLevel(mDamage + amount);
}

F32 ShapeBase::getDamageValue()
{
    // Return a 0-1 damage value.
    return mDamage / mDataBlock->maxDamage;
}

void ShapeBase::updateDamageLevel()
{
    if (mDamageThread) {
        // mDamage is already 0-1 on the client
        if (mDamage >= mDataBlock->destroyedLevel) {
            if (getDamageState() == Destroyed)
                mShapeInstance->setPos(mDamageThread, 0);
            else
                mShapeInstance->setPos(mDamageThread, 1);
        }
        else {
            mShapeInstance->setPos(mDamageThread, mDamage / mDataBlock->destroyedLevel);
        }
    }
}


//----------------------------------------------------------------------------

void ShapeBase::setDamageState(DamageState state)
{
    if (mDamageState == state)
        return;

    const char* script = 0;
    const char* lastState = 0;

    if (!isGhost()) {
        if (state != getDamageState())
            setMaskBits(DamageMask);

        lastState = getDamageStateName();
        switch (state) {
        case Destroyed: {
            if (mDamageState == Enabled)
                setDamageState(Disabled);
            script = "onDestroyed";
            break;
        }
        case Disabled:
            if (mDamageState == Enabled)
                script = "onDisabled";
            break;
        case Enabled:
            script = "onEnabled";
            break;
        }
    }

    mDamageState = state;
    if (mDamageState == Destroyed && isServerObject())
    {
        blowUp();
    }
    if (mDamageState != Enabled) {
        mRepairReserve = 0;
        mEnergy = 0;
    }
    if (script) {
        // Like to call the scripts after the state has been intialize.
        // This should only end up being called on the server.
        Con::executef(mDataBlock, 3, script, scriptThis(), lastState);
    }
    updateDamageState();
    updateDamageLevel();
}

bool ShapeBase::setDamageState(const char* state)
{
    for (S32 i = 0; i < NumDamageStates; i++)
        if (!dStricmp(state, sDamageStateName[i])) {
            setDamageState(DamageState(i));
            return true;
        }
    return false;
}

const char* ShapeBase::getDamageStateName()
{
    return sDamageStateName[mDamageState];
}

void ShapeBase::updateDamageState()
{
    if (mHulkThread) {
        F32 pos = (mDamageState == Destroyed) ? 1 : 0;
        if (mShapeInstance->getPos(mHulkThread) != pos) {
            mShapeInstance->setPos(mHulkThread, pos);

            if (isClientObject() || gSPMode)
                mShapeInstance->animate();
        }
    }
}


//----------------------------------------------------------------------------

void ShapeBase::blowUp()
{
    Point3F center;
    mObjBox.getCenter(&center);
    center += getPosition();
    MatrixF trans = getTransform();
    trans.setPosition(center);

    // explode
    Explosion* pExplosion = NULL;

    if (pointInWater((Point3F&)center) && mDataBlock->underwaterExplosion)
    {
        pExplosion = new Explosion;
        pExplosion->onNewDataBlock(mDataBlock->underwaterExplosion);
    }
    else
    {
        if (mDataBlock->explosion)
        {
            pExplosion = new Explosion;
            pExplosion->onNewDataBlock(mDataBlock->explosion);
        }
    }

    if (pExplosion)
    {
        pExplosion->setTransform(trans);
        pExplosion->setInitialState(center, damageDir);
        if (pExplosion->registerObject() == false)
        {
            Con::errorf(ConsoleLogEntry::General, "ShapeBase(%s)::explode: couldn't register explosion",
                mDataBlock->getName());
            delete pExplosion;
            pExplosion = NULL;
        }
    }

    if (!isGhost() && !gSPMode)
        return;

    TSShapeInstance* debShape = NULL;

    if (mDataBlock->debrisShape.isNull())
    {
        return;
    }
    else
    {
        debShape = new TSShapeInstance(mDataBlock->debrisShape, true);
    }


    Vector< TSPartInstance* > partList;
    TSPartInstance::breakShape(debShape, 0, partList, NULL, NULL, 0);

    if (!mDataBlock->debris)
    {
        mDataBlock->debris = new DebrisData;
    }

    // cycle through partlist and create debris pieces
    for (U32 i = 0; i < partList.size(); i++)
    {
        //Point3F axis( 0.0, 0.0, 1.0 );
        Point3F randomDir = MathUtils::randomDir(damageDir, 0, 50);

        Debris* debris = new Debris;
        debris->setPartInstance(partList[i]);
        debris->init(center, randomDir);
        debris->onNewDataBlock(mDataBlock->debris);
        debris->setTransform(trans);

        if (!debris->registerObject())
        {
            Con::warnf(ConsoleLogEntry::General, "Could not register debris for class: %s", mDataBlock->getName());
            delete debris;
            debris = NULL;
        }
        else
        {
            debShape->incDebrisRefCount();
        }
    }

    damageDir.set(0, 0, 1);
}


//----------------------------------------------------------------------------
void ShapeBase::mountObject(ShapeBase* obj, U32 node)
{
    //   if (obj->mMount.object == this)
    //      return;
    if (obj->mMount.object)
        obj->unmount();

    // Since the object is mounting to us, nothing should be colliding with it for a while
    obj->mConvexList->nukeList();

    obj->mMount.object = this;
    obj->mMount.node = (node >= 0 && node < ShapeBaseData::NumMountPoints) ? node : 0;
    obj->mMount.link = mMount.list;
    mMount.list = obj;
    if (obj != getControllingObject())
        obj->processAfter(this);
    obj->deleteNotify(this);
    obj->setMaskBits(MountedMask);
    obj->onMount(this, node);
}


void ShapeBase::unmountObject(ShapeBase* obj)
{
    if (obj->mMount.object == this) {

        // Find and unlink the object
        for (ShapeBase** ptr = &mMount.list; (*ptr); ptr = &((*ptr)->mMount.link))
        {
            if (*ptr == obj)
            {
                *ptr = obj->mMount.link;
                break;
            }
        }
        if (obj != getControllingObject())
            obj->clearProcessAfter();
        obj->clearNotify(this);
        obj->mMount.object = 0;
        obj->mMount.link = 0;
        obj->setMaskBits(MountedMask);
        obj->onUnmount(this, obj->mMount.node);
    }
}

void ShapeBase::unmount()
{
    if (mMount.object)
        mMount.object->unmountObject(this);
}

void ShapeBase::onMount(ShapeBase* obj, S32 node)
{
    if (!isGhost()) {
        char buff1[32];
        dSprintf(buff1, sizeof(buff1), "%d", node);
        Con::executef(mDataBlock, 4, "onMount", scriptThis(), obj->scriptThis(), buff1);
    }
}

void ShapeBase::onUnmount(ShapeBase* obj, S32 node)
{
    if (!isGhost()) {
        char buff1[32];
        dSprintf(buff1, sizeof(buff1), "%d", node);
        Con::executef(mDataBlock, 4, "onUnmount", scriptThis(), obj->scriptThis(), buff1);
    }
}

S32 ShapeBase::getMountedObjectCount()
{
    S32 count = 0;
    for (ShapeBase* itr = mMount.list; itr; itr = itr->mMount.link)
        count++;
    return count;
}

ShapeBase* ShapeBase::getMountedObject(S32 idx)
{
    if (idx >= 0) {
        S32 count = 0;
        for (ShapeBase* itr = mMount.list; itr; itr = itr->mMount.link)
            if (count++ == idx)
                return itr;
    }
    return 0;
}

S32 ShapeBase::getMountedObjectNode(S32 idx)
{
    if (idx >= 0) {
        S32 count = 0;
        for (ShapeBase* itr = mMount.list; itr; itr = itr->mMount.link)
            if (count++ == idx)
                return itr->mMount.node;
    }
    return -1;
}

ShapeBase* ShapeBase::getMountNodeObject(S32 node)
{
    for (ShapeBase* itr = mMount.list; itr; itr = itr->mMount.link)
        if (itr->mMount.node == node)
            return itr;
    return 0;
}

Point3F ShapeBase::getAIRepairPoint()
{
    if (mDataBlock->mountPointNode[ShapeBaseData::AIRepairNode] < 0)
        return Point3F(0, 0, 0);
    MatrixF xf(true);
    getMountTransform(ShapeBaseData::AIRepairNode, &xf);
    Point3F pos(0, 0, 0);
    xf.getColumn(3, &pos);
    return pos;
}

//----------------------------------------------------------------------------

void ShapeBase::getEyeTransform(MatrixF* mat)
{
    // Returns eye to world space transform
    S32 eyeNode = mDataBlock->eyeNode;
    if (eyeNode != -1)
        mat->mul(getTransform(), mShapeInstance->mNodeTransforms[eyeNode]);
    else
        *mat = getTransform();
}

void ShapeBase::getRenderEyeTransform(MatrixF* mat)
{
    // Returns eye to world space transform
    S32 eyeNode = mDataBlock->eyeNode;
    if (eyeNode != -1)
        mat->mul(getRenderTransform(), mShapeInstance->mNodeTransforms[eyeNode]);
    else
        *mat = getRenderTransform();
}

void ShapeBase::getCameraTransform(F32* pos, MatrixF* mat)
{
    // Returns camera to world space transform
    // Handles first person / third person camera position

    if (isServerObject() && mShapeInstance)
        mShapeInstance->animateNodeSubtrees(true);

    if (*pos != 0)
    {
        F32 min, max;
        Point3F offset;
        MatrixF eye, rot;
        getCameraParameters(&min, &max, &offset, &rot);
        getRenderEyeTransform(&eye);
        mat->mul(eye, rot);

        // Use the eye transform to orient the camera
        VectorF vp, vec;
        vp.x = vp.z = 0;
        vp.y = -(max - min) * *pos;
        eye.mulV(vp, &vec);

        // Use the camera node's pos.
        Point3F osp, sp;
        if (mDataBlock->cameraNode != -1) {
            mShapeInstance->mNodeTransforms[mDataBlock->cameraNode].getColumn(3, &osp);

            // Scale the camera position before applying the transform
            const Point3F& scale = getScale();
            osp.convolve(scale);

            getRenderTransform().mulP(osp, &sp);
        }
        else
            getRenderTransform().getColumn(3, &sp);

        // Make sure we don't extend the camera into anything solid
        Point3F ep = sp + vec + offset;
        disableCollision();
        if (isMounted())
            getObjectMount()->disableCollision();
        RayInfo collision;
        if (mContainer->castRay(sp, ep,
            (0xFFFFFFFF & ~(WaterObjectType |
                GameBaseObjectType |
                DefaultObjectType)),
            &collision) == true) {
            F32 veclen = vec.len();
            F32 adj = (-mDot(vec, collision.normal) / veclen) * 0.1;
            F32 newPos = getMax(0.0f, collision.t - adj);
            if (newPos == 0.0f)
                eye.getColumn(3, &ep);
            else
                ep = sp + offset + (vec * newPos);
        }
        mat->setColumn(3, ep);
        if (isMounted())
            getObjectMount()->enableCollision();
        enableCollision();
    }
    else
    {
        getRenderEyeTransform(mat);
    }
}

// void ShapeBase::getCameraTransform(F32* pos,MatrixF* mat)
// {
//    // Returns camera to world space transform
//    // Handles first person / third person camera position

//    if (isServerObject() && mShapeInstance)
//       mShapeInstance->animateNodeSubtrees(true);

//    if (*pos != 0) {
//       F32 min,max;
//       Point3F offset;
//       MatrixF eye,rot;
//       getCameraParameters(&min,&max,&offset,&rot);
//       getRenderEyeTransform(&eye);
//       mat->mul(eye,rot);

//       // Use the eye transform to orient the camera
//       VectorF vp,vec;
//       vp.x = vp.z = 0;
//       vp.y = -(max - min) * *pos;
//       eye.mulV(vp,&vec);

//       // Use the camera node's pos.
//       Point3F osp,sp;
//       if (mDataBlock->cameraNode != -1) {
//          mShapeInstance->mNodeTransforms[mDataBlock->cameraNode].getColumn(3,&osp);
//          getRenderTransform().mulP(osp,&sp);
//       }
//       else
//          getRenderTransform().getColumn(3,&sp);

//       // Make sure we don't extend the camera into anything solid
//       Point3F ep = sp + vec;
//       ep += offset;
//       disableCollision();
//       if (isMounted())
//          getObjectMount()->disableCollision();
//       RayInfo collision;
//       if (mContainer->castRay(sp,ep,(0xFFFFFFFF & ~(WaterObjectType|ForceFieldObjectType|GameBaseObjectType|DefaultObjectType)),&collision)) {
//          *pos = collision.t *= 0.9;
//          if (*pos == 0)
//             eye.getColumn(3,&ep);
//          else
//             ep = sp + vec * *pos;
//       }
//       mat->setColumn(3,ep);
//       if (isMounted())
//          getObjectMount()->enableCollision();
//       enableCollision();
//    }
//    else
//    {
//       getRenderEyeTransform(mat);
//    }
// }


// void ShapeBase::getRenderCameraTransform(F32* pos,MatrixF* mat)
// {
//    // Returns camera to world space transform
//    // Handles first person / third person camera position

//    if (isServerObject() && mShapeInstance)
//       mShapeInstance->animateNodeSubtrees(true);

//    if (*pos != 0) {
//       F32 min,max;
//       Point3F offset;
//       MatrixF eye,rot;
//       getCameraParameters(&min,&max,&offset,&rot);
//       getRenderEyeTransform(&eye);
//       mat->mul(eye,rot);

//       // Use the eye transform to orient the camera
//       VectorF vp,vec;
//       vp.x = vp.z = 0;
//       vp.y = -(max - min) * *pos;
//       eye.mulV(vp,&vec);

//       // Use the camera node's pos.
//       Point3F osp,sp;
//       if (mDataBlock->cameraNode != -1) {
//          mShapeInstance->mNodeTransforms[mDataBlock->cameraNode].getColumn(3,&osp);
//          getRenderTransform().mulP(osp,&sp);
//       }
//       else
//          getRenderTransform().getColumn(3,&sp);

//       // Make sure we don't extend the camera into anything solid
//       Point3F ep = sp + vec;
//       ep += offset;
//       disableCollision();
//       if (isMounted())
//          getObjectMount()->disableCollision();
//       RayInfo collision;
//       if (mContainer->castRay(sp,ep,(0xFFFFFFFF & ~(WaterObjectType|ForceFieldObjectType|GameBaseObjectType|DefaultObjectType)),&collision)) {
//          *pos = collision.t *= 0.9;
//          if (*pos == 0)
//             eye.getColumn(3,&ep);
//          else
//             ep = sp + vec * *pos;
//       }
//       mat->setColumn(3,ep);
//       if (isMounted())
//          getObjectMount()->enableCollision();
//       enableCollision();
//    }
//    else
//    {
//       getRenderEyeTransform(mat);
//    }
// }

void ShapeBase::getCameraParameters(F32* min, F32* max, Point3F* off, MatrixF* rot)
{
    *min = mDataBlock->cameraMinDist;
    *max = mDataBlock->cameraMaxDist;
    off->set(0, 0, 0);
    rot->identity();
}


//----------------------------------------------------------------------------
F32 ShapeBase::getDamageFlash() const
{
    return mDamageFlash;
}

void ShapeBase::setDamageFlash(const F32 flash)
{
    mDamageFlash = flash;
    if (mDamageFlash < 0.0)
        mDamageFlash = 0;
    else if (mDamageFlash > 1.0)
        mDamageFlash = 1.0;
}


//----------------------------------------------------------------------------
F32 ShapeBase::getWhiteOut() const
{
    return mWhiteOut;
}

void ShapeBase::setWhiteOut(const F32 flash)
{
    mWhiteOut = flash;
    if (mWhiteOut < 0.0)
        mWhiteOut = 0;
    else if (mWhiteOut > 1.5)
        mWhiteOut = 1.5;
}


//----------------------------------------------------------------------------

bool ShapeBase::onlyFirstPerson() const
{
    return mDataBlock->firstPersonOnly;
}

bool ShapeBase::useObjsEyePoint() const
{
    return mDataBlock->useEyePoint;
}


//----------------------------------------------------------------------------
F32 ShapeBase::getInvincibleEffect() const
{
    return mInvincibleEffect;
}

void ShapeBase::setupInvincibleEffect(F32 time, F32 speed)
{
    if (isClientObject())
    {
        mInvincibleCount = mInvincibleTime = time;
        mInvincibleSpeed = mInvincibleDelta = speed;
        mInvincibleEffect = 0.0f;
        mInvincibleOn = true;
        mInvincibleFade = 1.0f;
    }
    else
    {
        mInvincibleTime = time;
        mInvincibleSpeed = speed;
        setMaskBits(InvincibleMask);
    }
}

void ShapeBase::updateInvincibleEffect(F32 dt)
{
    if (mInvincibleCount > 0.0f)
    {
        if (mInvincibleEffect >= ((0.3 * mInvincibleFade) + 0.05f) && mInvincibleDelta > 0.0f)
            mInvincibleDelta = -mInvincibleSpeed;
        else if (mInvincibleEffect <= 0.05f && mInvincibleDelta < 0.0f)
        {
            mInvincibleDelta = mInvincibleSpeed;
            mInvincibleFade = mInvincibleCount / mInvincibleTime;
        }
        mInvincibleEffect += mInvincibleDelta;
        mInvincibleCount -= dt;
    }
    else
    {
        mInvincibleEffect = 0.0f;
        mInvincibleOn = false;
    }
}

//----------------------------------------------------------------------------
void ShapeBase::setVelocity(const VectorF&)
{
}

void ShapeBase::applyImpulse(const Point3F&, const VectorF&)
{
}

F32 ShapeBase::getMass() const
{
    return mMass;
}


//----------------------------------------------------------------------------

void ShapeBase::playAudio(U32 slot, SFXProfile* profile)
{
    AssertFatal(slot < MaxSoundThreads, "ShapeBase::playSound: Incorrect argument");
    Sound& st = mSoundThread[slot];
    if (profile && (!st.play || st.profile != profile)) {
        setMaskBits(SoundMaskN << slot);
        st.play = true;
        st.profile = profile;
        updateAudioState(st);
    }
}

void ShapeBase::stopAudio(U32 slot)
{
    AssertFatal(slot < MaxSoundThreads, "ShapeBase::stopSound: Incorrect argument");
    Sound& st = mSoundThread[slot];
    if (st.play) {
        st.play = false;
        setMaskBits(SoundMaskN << slot);
        updateAudioState(st);
    }
}

void ShapeBase::updateServerAudio()
{
    // Timeout non-looping sounds
    for (int i = 0; i < MaxSoundThreads; i++) {
        Sound& st = mSoundThread[i];
        if (st.play && st.timeout && st.timeout < Sim::getCurrentTime()) {
            clearMaskBits(SoundMaskN << i);
            st.play = false;
        }
    }
}

void ShapeBase::updateAudioState(Sound& st)
{
    SFX_DELETE(st.sound);

    if (st.play && st.profile)
    {
        if (isGhost())
        {
            if (Sim::findObject(static_cast<SimObjectId>(reinterpret_cast<size_t>(st.profile)), st.profile))
            {
                st.sound = SFX->createSource(st.profile, &getTransform());
                if (st.sound)
                    st.sound->play();
            }
            else
                st.play = false;
        }
        else
        {
            // Non-looping sounds timeout on the server
            st.timeout = 0;
            if (!st.profile->getDescription()->mIsLooping)
                st.timeout = Sim::getCurrentTime() + sAudioTimeout;
        }
    }
    else
        st.play = false;
}

void ShapeBase::updateAudioPos()
{
    for (int i = 0; i < MaxSoundThreads; i++)
    {
        SFXSource* source = mSoundThread[i].sound;
        if (source)
            source->setTransform(getTransform());
    }
}

//----------------------------------------------------------------------------

bool ShapeBase::setThreadSequence(U32 slot, S32 seq, bool reset, F32 timeScale)
{
    Thread& st = mScriptThread[slot];
    if (st.thread && st.sequence == seq && st.state == Thread::Play && reset)
        return true;

    if (seq < MaxSequenceIndex) {
        setMaskBits(ThreadMaskN << slot);
        st.sequence = seq;
        if (reset) {
            st.state = Thread::Play;
            st.id++;
            st.timeScale = timeScale;
            st.atEnd = false;
            st.forward = true;
        }
        if (mShapeInstance) {
            if (!st.thread)
                st.thread = mShapeInstance->addThread();
            mShapeInstance->setSequence(st.thread, seq, 0);
            mShapeInstance->setTimeScale(st.thread, st.timeScale);
            stopThreadSound(st);
            updateThread(st);
        }
        return true;
    }
    return false;
}

bool ShapeBase::setTimeScale(U32 slot, F32 timeScale)
{
    if (mScriptThread[slot].sequence == -1)
        return false;

    if (timeScale != mScriptThread[slot].timeScale)
    {
        setMaskBits(ThreadMaskN << slot);
        mScriptThread[slot].timeScale = timeScale;
        updateThread(mScriptThread[slot]);
    }

    return true;
}

void ShapeBase::updateThread(Thread& st)
{
    switch (st.state) {
    case Thread::Stop:
        mShapeInstance->setTimeScale(st.thread, 1);
        mShapeInstance->setPos(st.thread, 0);
        // Drop through to pause state
    case Thread::Pause:
        mShapeInstance->setTimeScale(st.thread, 0);
        stopThreadSound(st);
        break;
    case Thread::Play:
        if (st.atEnd) {
            mShapeInstance->setTimeScale(st.thread, 1);
            mShapeInstance->setPos(st.thread, st.forward ? 1 : 0);
            mShapeInstance->setTimeScale(st.thread, 0);
            stopThreadSound(st);
        }
        else {
            mShapeInstance->setTimeScale(st.thread, st.forward ? 1 : -1);
            if (!st.sound)
                startSequenceSound(st);
        }
        break;
    }
}

bool ShapeBase::stopThread(U32 slot)
{
    Thread& st = mScriptThread[slot];
    if (st.sequence != -1 && st.state != Thread::Stop) {
        setMaskBits(ThreadMaskN << slot);
        st.state = Thread::Stop;
        updateThread(st);
        return true;
    }
    return false;
}

bool ShapeBase::pauseThread(U32 slot)
{
    Thread& st = mScriptThread[slot];
    if (st.sequence != -1 && st.state != Thread::Pause) {
        setMaskBits(ThreadMaskN << slot);
        st.state = Thread::Pause;
        updateThread(st);
        return true;
    }
    return false;
}

bool ShapeBase::playThread(U32 slot)
{
    Thread& st = mScriptThread[slot];
    if (st.sequence != -1 && st.state != Thread::Play) {
        setMaskBits(ThreadMaskN << slot);
        st.state = Thread::Play;
        updateThread(st);
        return true;
    }
    return false;
}

void ShapeBase::playThreadDelayed(const ThreadCmd& cmd)
{
    mThreadCmd = cmd;
}

bool ShapeBase::setThreadDir(U32 slot, bool forward)
{
    Thread& st = mScriptThread[slot];
    if (st.sequence != -1) {
        if (st.forward != forward) {
            setMaskBits(ThreadMaskN << slot);
            st.forward = forward;
            st.atEnd = false;
            updateThread(st);
        }
        return true;
    }
    return false;
}

void ShapeBase::stopThreadSound(Thread& thread)
{
    if (thread.sound) {
    }
}

void ShapeBase::startSequenceSound(Thread& thread)
{
    if (!isGhost() || !thread.thread)
        return;
    stopThreadSound(thread);
}

void ShapeBase::advanceThreads(F32 dt)
{
    bool anim = false;

    for (U32 i = 0; i < MaxScriptThreads; i++) {
        Thread& st = mScriptThread[i];
        if (st.thread) {
            if (!mShapeInstance->getShape()->sequences[st.sequence].isCyclic() && !st.atEnd &&
                (st.forward ? mShapeInstance->getPos(st.thread) >= 1.0 :
                    mShapeInstance->getPos(st.thread) <= 0)) {
                st.atEnd = true;
                updateThread(st);
                if (!isGhost()) {
                    char slot[16];
                    dSprintf(slot, sizeof(slot), "%d", i);
                    Con::executef(mDataBlock, 3, "onEndSequence", scriptThis(), slot);
                }
            }
            mShapeInstance->advanceTime(dt, st.thread);
            anim = true;
        }
    }

    if (anim)
        mShapeInstance->animate();
}


//----------------------------------------------------------------------------

TSShape const* ShapeBase::getShape()
{
    return mShapeInstance ? mShapeInstance->getShape() : 0;
}


void ShapeBase::calcClassRenderData()
{
    // This is truly lame, but I didn't want to duplicate the whole preprender logic
    //  in the player as well as the renderImage logic.  DMM
}


bool ShapeBase::prepRenderImage(SceneState* state, const U32 stateKey,
    const U32 startZone, const bool modifyBaseState)
{
    AssertFatal(modifyBaseState == false, "Error, should never be called with this parameter set");
    AssertFatal(startZone == 0xFFFFFFFF, "Error, startZone should indicate -1");

    if (isLastState(state, stateKey))
        return false;
    setLastState(state, stateKey);

#ifdef MB_ULTRA
    if (gNoRenderAstrolabe && mDataBlock != NULL && (mDataBlock->astrolabe || mDataBlock->astrolabePrime) ||
       (getDamageState() == Destroyed && !mDataBlock->renderWhenDestroyed))
        return false;
#else
    if ((getDamageState() == Destroyed) && (!mDataBlock->renderWhenDestroyed))
        return false;
#endif
    // Select detail levels on mounted items
    // but... always draw the control object's mounted images
    // in high detail (I can't believe I'm commenting this hack :)
    F32 saveError = TSShapeInstance::smScreenError;
    GameConnection* con = GameConnection::getConnectionToServer();
    bool fogExemption = false;
    ShapeBase* co = NULL;
    if (con && ((co = con->getControlObject()) != NULL))
    {
        if (co == this || co->getObjectMount() == this)
        {
            TSShapeInstance::smScreenError = 0.001;
            fogExemption = true;
        }
    }

    if (state->isObjectRendered(this) || getCurrentClientSceneGraph()->isReflectPass())
    {
        mLastRenderFrame = sLastRenderFrame;
        // get shape detail and fog information...we might not even need to be drawn
        Point3F cameraOffset;
        getRenderTransform().getColumn(3, &cameraOffset);
        cameraOffset -= state->getCameraPosition();
        F32 dist = cameraOffset.len();
        if (dist < 0.01)
            dist = 0.01;
        F32 fogAmount = state->getHazeAndFog(dist, cameraOffset.z);
        F32 invScale = (1.0f / getMax(getMax(mObjScale.x, mObjScale.y), mObjScale.z));
        if (mShapeInstance)
            DetailManager::selectPotentialDetails(mShapeInstance, dist, invScale);

        if (mShapeInstance)
            mShapeInstance->animate();

        if ((fogAmount > 0.99f && fogExemption == false) ||
            (mShapeInstance && mShapeInstance->getCurrentDetail() < 0) ||
            (!mShapeInstance && !gShowBoundingBox)) {
            // no, don't draw anything
            return false;
        }


        for (U32 i = 0; i < MaxMountedImages; i++)
        {
            MountedImage& image = mMountedImageList[i];
            if (image.dataBlock && image.shapeInstance)
            {
                DetailManager::selectPotentialDetails(image.shapeInstance, dist, invScale);

                if (mCloakLevel == 0.0f && image.shapeInstance->hasSolid() && mFadeVal == 1.0f)
                {
                    prepBatchRender(state, i);

                    // render debug
                    if (gShowBoundingBox)
                    {
                        RenderInst* ri = gRenderInstManager.allocInst();
                        ri->obj = this;
                        ri->state = state;
                        ri->type = RenderInstManager::RIT_Object;
                        ri->mountedObjIdx = i;
                        gRenderInstManager.addInst(ri);
                    }
                }
            }
        }
        TSShapeInstance::smScreenError = saveError;

        if (mDataBlock->shadowEnable)
        {
            RenderInst* ri = gRenderInstManager.allocInst();
            ri->obj = this;
            ri->state = state;
            ri->type = RenderInstManager::RIT_Shadow;
            gRenderInstManager.addInst(ri);
        }

        // render debug
        if (gShowBoundingBox)
        {
            RenderInst* ri = gRenderInstManager.allocInst();
            ri->obj = this;
            ri->state = state;
            ri->type = RenderInstManager::RIT_Object;
            ri->mountedObjIdx = -1;
            gRenderInstManager.addInst(ri);
        }

        prepBatchRender(state, -1);

        calcClassRenderData();
    }

    return false;
}

//----------------------------------------------------------------------------
// prepBatchRender
//----------------------------------------------------------------------------
void ShapeBase::prepBatchRender(SceneState* state, S32 mountedImageIndex)
{
    // CHANGES IN HERE SHOULD BE DUPLICATED IN TSSTATIC!

    RectI viewport = GFX->getViewport();
    MatrixF proj = GFX->getProjectionMatrix();

    GameConnection* conn = GameConnection::getConnectionToServer();
    if (conn)
    {
        ShapeBase *control = conn->getControlObject();
        if (control)
        {
            F32 fov;
            conn->getControlCameraFov(&fov);

            float pixSize = ((mWorldSphere.radius / (state->getCameraPosition() - mWorldSphere.center).len()) * viewport.len_y()) / fov;

            if (pixSize < Con::getFloatVariable("$cullSize", 0.05f))
                return;
        }
    }
    
    GFX->pushWorldMatrix();

    MatrixF world = GFX->getWorldMatrix();
    TSMesh::setCamTrans(world);
    TSMesh::setSceneState(state);
    TSMesh::setCubemap(mDynamicCubemap);
    TSMesh::setObject(this);


    getCurrentClientSceneGraph()->getLightManager()->sgSetupLights(this);

    if (mountedImageIndex != -1)
    {
        MountedImage& image = mMountedImageList[mountedImageIndex];
        if (image.dataBlock && image.shapeInstance && DetailManager::selectCurrentDetail(image.shapeInstance))
        {
            MatrixF mat;
            getRenderImageTransform(mountedImageIndex, &mat);

#ifdef MB_ULTRA
            if ((mTypeMask & PlayerObjectType) != 0)
            {
                Point3F pos(mat[3], mat[7], mat[11]);
                // Matt: why in the heck did they do it like this...
                ((Marble*)this)->mGravityRenderFrame.setMatrix(&mat);
                mat[3] = pos.x;
                mat[7] = pos.y;
                mat[11] = pos.z;
                mat.scale(mRenderScale);
            }
#endif
            GFX->setWorldMatrix(mat);

            image.shapeInstance->animate();
            image.shapeInstance->render();
        }

    }
    else
    {
        MatrixF mat = getRenderTransform();
#ifdef MB_ULTRA
        if (!mAnimateScale)
            mRenderScale = mObjScale;
        mat.scale(mRenderScale);
#else
        mat.scale(mObjScale);
#endif
        GFX->setWorldMatrix(mat);

        bool serverobj = this->isServerObject();

        mShapeInstance->animate();
        mShapeInstance->render();
    }

    getCurrentClientSceneGraph()->getLightManager()->sgResetLights();


    GFX->popWorldMatrix();
    GFX->setProjectionMatrix(proj);
    GFX->setViewport(viewport);

}

//----------------------------------------------------------------------------
// renderObject - render debug data
//----------------------------------------------------------------------------
void ShapeBase::renderObject(SceneState* state, RenderInst* ri)
{
    RectI viewport = GFX->getViewport();
    MatrixF proj = GFX->getProjectionMatrix();

    // Debugging Bounding Box
    if (!mShapeInstance || gShowBoundingBox)
    {
        GFX->pushWorldMatrix();
        GFX->multWorld(getRenderTransform());

        Point3F box1, box2;
        box1 = (mObjBox.max - mObjBox.min) * 0.5;
        box2 = (mObjBox.min + mObjBox.max) * 0.5;
        wireCube(box1, box2);

        GFX->popWorldMatrix();
    }
}

void ShapeBase::renderShadow(SceneState* state, RenderInst* ri)
{
    RectI viewport = GFX->getViewport();
    MatrixF proj = GFX->getProjectionMatrix();

    // hack until new scenegraph in place
    MatrixF world = GFX->getWorldMatrix();
    TSMesh::setCamTrans(world);
    TSMesh::setSceneState(state);
    TSMesh::setCubemap(mDynamicCubemap);
    TSMesh::setGlow(false);
    TSMesh::setRefract(false);

    getCurrentClientSceneGraph()->getLightManager()->sgSetupLights(this);

    Point3F cam = state->getCameraPosition() - getRenderPosition();
    shadows.sgRender(this, getShapeInstance(), cam.len());

    getCurrentClientSceneGraph()->getLightManager()->sgResetLights();

    GFX->setProjectionMatrix(proj);
    GFX->setViewport(viewport);
}

/*void ShapeBase::renderShadow(F32 dist, F32 fogAmount)
{
   if (Shadow::getGlobalShadowDetailLevel()<mDataBlock->noShadowLevel)
      return;
   if (mShapeInstance->getShape()->subShapeFirstTranslucentObject.empty() ||
       mShapeInstance->getShape()->subShapeFirstTranslucentObject[0] == 0)
      return;

   // for safety, since I can't yet guarantee when this gets called.
   dglNPatchEnd(); // stop NPatching stuff, on the object whose shadow is being drawn.

   if (!mShadow)
      mShadow = new Shadow();

   if (Shadow::getGlobalShadowDetailLevel() < mDataBlock->genericShadowLevel)
      mShadow->setGeneric(true);
   else
      mShadow->setGeneric(false);

   Point3F lightDir(0.57f,0.57f,-0.57f);
   F32 shadowLen = 10.0f * mShapeInstance->getShape()->radius;
   Point3F pos = mShapeInstance->getShape()->center;
   // this is a bit of a hack...move generic shadows towards feet/base of shape
   if (Shadow::getGlobalShadowDetailLevel() < mDataBlock->genericShadowLevel)
      pos *= 0.5f;
   S32 shadowNode = mDataBlock->shadowNode;
   if (shadowNode>=0)
   {
      // adjust for movement of shape outside of bounding box by tracking this node
      Point3F offset;
      mShapeInstance->mNodeTransforms[shadowNode].getColumn(3,&offset);
      offset -= mShapeInstance->getShape()->defaultTranslations[shadowNode];
      offset.z = 0.0f;
      pos += offset;
   }
   pos.convolve(mObjScale);
   getRenderTransform().mulP(pos);

   // pos is where shadow will be centered (in world space)
   mShadow->setRadius(mShapeInstance, mObjScale);
   if (!mShadow->prepare(pos, lightDir, shadowLen, mObjScale, dist, fogAmount, mShapeInstance))
      return;

   F32 maxScale = getMax(mObjScale.x,getMax(mObjScale.y,mObjScale.z));

   if (mShadow->needBitmap())
   {
      mShadow->beginRenderToBitmap();
      mShadow->selectShapeDetail(mShapeInstance,dist,maxScale);
      mShadow->renderToBitmap(mShapeInstance, getRenderTransform(), pos, mObjScale);

      // render mounted items to shadow bitmap
      for (U32 i = 0; i < MaxMountedImages; i++)
      {
         MountedImage& image = mMountedImageList[i];
         if (image.dataBlock && image.shapeInstance)
         {
            MatrixF mat;
            getRenderImageTransform(i,&mat);
            mShadow->selectShapeDetail(image.shapeInstance,dist,maxScale);
            mShadow->renderToBitmap(image.shapeInstance,mat,pos,Point3F(1,1,1));
         }
      }

      // We only render the first mounted object for now...
      if (mMount.link && mMount.link->mShapeInstance)
      {
         Point3F linkScale = mMount.link->getScale();
         maxScale = getMax(linkScale.x,getMax(linkScale.y,linkScale.z));
         mShadow->selectShapeDetail(mMount.link->mShapeInstance,dist,maxScale);
         mShadow->renderToBitmap(mMount.link->mShapeInstance,
                                 mMount.link->getRenderTransform(),
                                 pos,
                                 linkScale);
      }

      mShadow->endRenderToBitmap();
   }

   mShadow->render();
}*/

//----------------------------------------------------------------------------
// This is a callback for objects that have reflections and are added to
// the "reflectiveSet" SimSet.
//----------------------------------------------------------------------------
void ShapeBase::updateReflection()
{
    if (mDynamicCubemap.isValid())
    {
        Point3F pos = getPosition();
        mDynamicCubemap->updateDynamic(pos);
    }
}


//----------------------------------------------------------------------------

static ColorF cubeColors[8] = {
   ColorF(0, 0, 0), ColorF(1, 0, 0), ColorF(0, 1, 0), ColorF(0, 0, 1),
   ColorF(1, 1, 0), ColorF(1, 0, 1), ColorF(0, 1, 1), ColorF(1, 1, 1)
};

static Point3F cubePoints[8] = {
   Point3F(-1, -1, -1), Point3F(-1, -1,  1), Point3F(-1,  1, -1), Point3F(-1,  1,  1),
   Point3F(1, -1, -1), Point3F(1, -1,  1), Point3F(1,  1, -1), Point3F(1,  1,  1)
};

static U32 cubeFaces[6][4] = {
   { 0, 2, 6, 4 }, { 0, 2, 3, 1 }, { 0, 1, 5, 4 },
   { 3, 2, 6, 7 }, { 7, 6, 4, 5 }, { 3, 7, 5, 1 }
};

void ShapeBase::wireCube(const Point3F& size, const Point3F& pos)
{
    GFX->drawWireCube(size, pos, ColorI(255, 255, 255));

    /*
       glDisable(GL_CULL_FACE);

       for(int i = 0; i < 6; i++) {
          glBegin(GL_LINE_LOOP);
          for(int vert = 0; vert < 4; vert++) {
             int idx = cubeFaces[i][vert];
             glVertex3f(cubePoints[idx].x * size.x + pos.x, cubePoints[idx].y * size.y + pos.y, cubePoints[idx].z * size.z + pos.z);
          }
          glEnd();
       }
    */
}


//----------------------------------------------------------------------------

bool ShapeBase::castRay(const Point3F& start, const Point3F& end, RayInfo* info)
{
    if (mShapeInstance)
    {
        RayInfo shortest;
        shortest.t = 1e8;

        info->object = NULL;
        for (U32 i = 0; i < mDataBlock->LOSDetails.size(); i++)
        {
            mShapeInstance->animate(mDataBlock->LOSDetails[i]);
            if (mShapeInstance->castRay(start, end, info, mDataBlock->LOSDetails[i]))
            {
                info->object = this;
                if (info->t < shortest.t)
                    shortest = *info;
            }
        }

        if (info->object == this)
        {
            // Copy out the shortest time...
            *info = shortest;
            return true;
        }
    }

    return false;
}


//----------------------------------------------------------------------------

bool ShapeBase::buildPolyList(AbstractPolyList* polyList, const Box3F&, const SphereF&)
{
    if (mShapeInstance) {
        bool ret = false;

        polyList->setTransform(&mObjToWorld, mObjScale);
        polyList->setObject(this);

        for (U32 i = 0; i < mDataBlock->collisionDetails.size(); i++)
        {
            mShapeInstance->buildPolyList(polyList, mDataBlock->collisionDetails[i]);
            ret = true;
        }

        return ret;
    }

    return false;
}


void ShapeBase::buildConvex(const Box3F& box, Convex* convex)
{
    if (mShapeInstance == NULL)
        return;

    // These should really come out of a pool
    mConvexList->collectGarbage();

    Box3F realBox = box;
    mWorldToObj.mul(realBox);
    realBox.min.convolveInverse(mObjScale);
    realBox.max.convolveInverse(mObjScale);

    if (realBox.isOverlapped(getObjBox()) == false)
        return;

    for (U32 i = 0; i < mDataBlock->collisionDetails.size(); i++)
    {
        Box3F newbox = mDataBlock->collisionBounds[i];
        newbox.min.convolve(mObjScale);
        newbox.max.convolve(mObjScale);
        mObjToWorld.mul(newbox);
        if (box.isOverlapped(newbox) == false)
            continue;

        // See if this hull exists in the working set already...
        Convex* cc = 0;
        CollisionWorkingList& wl = convex->getWorkingList();
        for (CollisionWorkingList* itr = wl.wLink.mNext; itr != &wl; itr = itr->wLink.mNext) {
            if (itr->mConvex->getType() == ShapeBaseConvexType &&
                (static_cast<ShapeBaseConvex*>(itr->mConvex)->pShapeBase == this &&
                    static_cast<ShapeBaseConvex*>(itr->mConvex)->hullId == i)) {
                cc = itr->mConvex;
                break;
            }
        }
        if (cc)
            continue;

        // Create a new convex.
        ShapeBaseConvex* cp = new ShapeBaseConvex;
        mConvexList->registerObject(cp);
        convex->addToWorkingList(cp);
        cp->mObject = this;
        cp->pShapeBase = this;
        cp->hullId = i;
        cp->box = mDataBlock->collisionBounds[i];
        cp->transform = 0;
        cp->findNodeTransform();
    }
}


//----------------------------------------------------------------------------

#ifdef MARBLE_BLAST
void ShapeBase::queueCollision(ShapeBase* obj, const VectorF& vec, const U32 surfaceId)
#else
void ShapeBase::queueCollision(ShapeBase* obj, const VectorF& vec)
#endif
{
    // Add object to list of collisions.
    SimTime time = Sim::getCurrentTime();
    S32 num = obj->getId();

    CollisionTimeout** adr = &mTimeoutList;
    CollisionTimeout* ptr = mTimeoutList;

#ifdef MARBLE_BLAST
    const Material* mat = obj->getMaterial(surfaceId);
#endif

    while (ptr) {
        if (ptr->objectNumber == num) {
            if (ptr->expireTime < time || (obj->getTypeMask() & ItemObjectType) != 0) {
                ptr->expireTime = time + CollisionTimeoutValue;
                ptr->object = obj;
                ptr->vector = vec;
            }
            return;
        }
        // Recover expired entries
        if (ptr->expireTime < time) {
            CollisionTimeout* cur = ptr;
            *adr = ptr->next;
            ptr = ptr->next;
            cur->next = sFreeTimeoutList;
            sFreeTimeoutList = cur;
        }
        else {
            adr = &ptr->next;
            ptr = ptr->next;
        }
    }

    // New entry for the object
    if (sFreeTimeoutList != NULL)
    {
        ptr = sFreeTimeoutList;
        sFreeTimeoutList = ptr->next;
        ptr->next = NULL;
    }
    else
    {
        ptr = sTimeoutChunker.alloc();
    }

    ptr->object = obj;
    ptr->objectNumber = obj->getId();
    ptr->vector = vec;
#ifdef MARBLE_BLAST
    ptr->material = mat;
#endif
    ptr->expireTime = time + CollisionTimeoutValue;
    ptr->next = mTimeoutList;

    mTimeoutList = ptr;
}

void ShapeBase::notifyCollision()
{
    // Notify all the objects that were just stamped during the queueing
    // process.
    SimTime expireTime = Sim::getCurrentTime() + CollisionTimeoutValue;
    for (CollisionTimeout* ptr = mTimeoutList; ptr; ptr = ptr->next)
    {
        if (ptr->expireTime == expireTime && ptr->object)
        {
            SimObjectPtr<ShapeBase> safePtr(ptr->object);
            SimObjectPtr<ShapeBase> safeThis(this);
            onCollision(ptr->object, ptr->vector, NULL);
            ptr->object = 0;

            if (!bool(safeThis))
                return;

            if (bool(safePtr))
            {
#ifdef MARBLE_BLAST
                safePtr->onCollision(this, ptr->vector, ptr->material);
#else
                safePtr->onCollision(this, ptr->vector);
#endif
            }

            if (!bool(safeThis))
                return;
        }
    }
}

#ifdef MARBLE_BLAST
void ShapeBase::onCollision(ShapeBase* object, VectorF vec, const Material* mat)
{
    char buff1[256];
    char buff2[32];

    dSprintf(buff1, sizeof(buff1), "%g %g %g", vec.x, vec.y, vec.z);
    dSprintf(buff2, sizeof(buff2), "%g", vec.len());

    const char* matName = mat ? mat->getName() : "DefaultMaterial";

    const char* funcName = isServerObject() ? "onCollision" : "onClientCollision";

    Con::executef(mDataBlock, 6, funcName, scriptThis(), object->scriptThis(), buff1, buff2, matName);
}
#else
void ShapeBase::onCollision(ShapeBase* object, VectorF vec)
{
    if (!isGhost()) {
        char buff1[256];
        char buff2[32];

        dSprintf(buff1, sizeof(buff1), "%g %g %g", vec.x, vec.y, vec.z);
        dSprintf(buff2, sizeof(buff2), "%g", vec.len());
        Con::executef(mDataBlock, 5, "onCollision", scriptThis(), object->scriptThis(), buff1, buff2);
    }
}
#endif

//--------------------------------------------------------------------------
bool ShapeBase::pointInWater(Point3F& point)
{
    SimpleQueryList sql;
    if (isServerObject())
        getCurrentServerSceneGraph()->getWaterObjectList(sql);
    else
        getCurrentClientSceneGraph()->getWaterObjectList(sql);

#ifdef TORQUE_TERRAIN
    for (U32 i = 0; i < sql.mList.size(); i++)
    {
        WaterBlock* pBlock = dynamic_cast<WaterBlock*>(sql.mList[i]);
        if (pBlock && pBlock->isUnderwater(point))
            return true;
    }
#endif

    return false;
}

//----------------------------------------------------------------------------

void ShapeBase::writePacketData(GameConnection* connection, BitStream* stream)
{
    Parent::writePacketData(connection, stream);

    stream->write(getEnergyLevel());
    stream->write(mRechargeRate);
}

void ShapeBase::readPacketData(GameConnection* connection, BitStream* stream)
{
    Parent::readPacketData(connection, stream);

    F32 energy;
    stream->read(&energy);
    setEnergyLevel(energy);

    stream->read(&mRechargeRate);
}

U32 ShapeBase::getPacketDataChecksum(GameConnection* connection)
{
    // just write the packet data into a buffer
    // then we can CRC the buffer.  This should always let us
    // know when there is a checksum problem.

    static U8 buffer[1500] = { 0, };
    BitStream stream(buffer, sizeof(buffer));

    writePacketData(connection, &stream);
    U32 byteCount = stream.getPosition();
    U32 ret = calculateCRC(buffer, byteCount, 0xFFFFFFFF);
    dMemset(buffer, 0, byteCount);
    return ret;
}

F32 ShapeBase::getUpdatePriority(CameraScopeQuery* camInfo, U32 updateMask, S32 updateSkips)
{
    // If it's the scope object, must be high priority
    if (camInfo->camera == this) {
        // Most priorities are between 0 and 1, so this
        // should be something larger.
        return 10.0f;
    }
    if (camInfo->camera && (camInfo->camera->getType() & ShapeBaseObjectType))
    {
        // see if the camera is mounted to this...
        // if it is, this should have a high priority
        if (((ShapeBase*)camInfo->camera)->getObjectMount() == this)
            return 10.0f;
    }
    return Parent::getUpdatePriority(camInfo, updateMask, updateSkips);
}

U32 ShapeBase::packUpdate(NetConnection* con, U32 mask, BitStream* stream)
{
    U32 retMask = Parent::packUpdate(con, mask, stream);

    if (mask & InitialUpdateMask) {
        // mask off sounds that aren't playing
        S32 i;
        for (i = 0; i < MaxSoundThreads; i++)
            if (!mSoundThread[i].play)
                mask &= ~(SoundMaskN << i);

        // mask off threads that aren't running
        for (i = 0; i < MaxScriptThreads; i++)
            if (mScriptThread[i].sequence == -1)
                mask &= ~(ThreadMaskN << i);

        // mask off images that aren't updated
        for (i = 0; i < MaxMountedImages; i++)
            if (!mMountedImageList[i].dataBlock)
                mask &= ~(ImageMaskN << i);
    }

    if (!stream->writeFlag(mask & (NameMask | DamageMask | SoundMask |
        ThreadMask | ImageMask | CloakMask | MountedMask | InvincibleMask |
        ShieldMask | SkinMask)))
        return retMask;

    if (stream->writeFlag(mask & DamageMask)) {
        stream->writeFloat(mClampF(mDamage / mDataBlock->maxDamage, 0.f, 1.f), DamageLevelBits);
        stream->writeInt(mDamageState, NumDamageStateBits);
        stream->writeNormalVector(damageDir, 8);
    }

    if (stream->writeFlag(mask & ThreadMask)) {
        for (int i = 0; i < MaxScriptThreads; i++) {
            Thread& st = mScriptThread[i];
            if (stream->writeFlag(st.sequence != -1 && (mask & (ThreadMaskN << i)))) {
                stream->writeInt(st.sequence, ThreadSequenceBits);
                stream->writeInt(st.id, 4);
                stream->writeInt(st.state, 2);
                stream->writeFlag(st.forward);
                stream->writeFlag(st.atEnd);
            }
        }
    }

    if (stream->writeFlag(mask & SoundMask)) {
        for (int i = 0; i < MaxSoundThreads; i++) {
            Sound& st = mSoundThread[i];
            if (stream->writeFlag(mask & (SoundMaskN << i)))
                if (stream->writeFlag(st.play))
                    stream->writeRangedU32(st.profile->getId(), DataBlockObjectIdFirst,
                        DataBlockObjectIdLast);
        }
    }

    if (stream->writeFlag(mask & ImageMask)) {
        for (int i = 0; i < MaxMountedImages; i++)
            if (stream->writeFlag(mask & (ImageMaskN << i))) {
                MountedImage& image = mMountedImageList[i];
                if (stream->writeFlag(image.dataBlock))
                    stream->writeInt(image.dataBlock->getId() - DataBlockObjectIdFirst,
                        DataBlockObjectIdBitSize);
                con->packStringHandleU(stream, image.skinNameHandle);
                stream->writeFlag(image.wet);
                stream->writeFlag(image.ammo);
                stream->writeFlag(image.loaded);
                stream->writeFlag(image.target);
                stream->writeFlag(image.triggerDown);
                stream->writeInt(image.fireCount, 3);
                if (mask & InitialUpdateMask)
                    stream->writeFlag(isImageFiring(i));
            }
    }

    // Group some of the uncommon stuff together.
    if (stream->writeFlag(mask & (NameMask | ShieldMask | CloakMask | InvincibleMask | SkinMask))) {
        if (stream->writeFlag(mask & CloakMask)) {
            // cloaking
            stream->writeFlag(mCloaked);

            // piggyback control update
            stream->writeFlag(bool(getControllingClient()));

            // fading
            if (stream->writeFlag(mFading && mFadeElapsedTime >= mFadeDelay)) {
                stream->writeFlag(mFadeOut);
                stream->write(mFadeTime);
            }
            else
                stream->writeFlag(mFadeVal == 1.0f);
        }
        if (stream->writeFlag(mask & NameMask)) {
            con->packStringHandleU(stream, mShapeNameHandle);
        }
        if (stream->writeFlag(mask & ShieldMask)) {
            stream->writeNormalVector(mShieldNormal, ShieldNormalBits);
            stream->writeFloat(getEnergyValue(), EnergyLevelBits);
        }
        if (stream->writeFlag(mask & InvincibleMask)) {
            stream->write(mInvincibleTime);
            stream->write(mInvincibleSpeed);
        }

        if (stream->writeFlag(mask & SkinMask)) {

            con->packStringHandleU(stream, mSkinNameHandle);

        }
    }

    if (mask & MountedMask) {
        if (mMount.object) {
            S32 gIndex = con->getGhostIndex(mMount.object);
            if (stream->writeFlag(gIndex != -1)) {
                stream->writeFlag(true);
                stream->writeInt(gIndex, NetConnection::GhostIdBitSize);
                stream->writeInt(mMount.node, ShapeBaseData::NumMountPointBits);
            }
            else
                // Will have to try again later
                retMask |= MountedMask;
        }
        else
            // Unmount if this isn't the initial packet
            if (stream->writeFlag(!(mask & InitialUpdateMask)))
                stream->writeFlag(false);
    }
    else
        stream->writeFlag(false);

    return retMask;
}

void ShapeBase::unpackUpdate(NetConnection* con, BitStream* stream)
{
    Parent::unpackUpdate(con, stream);
    mLastRenderFrame = sLastRenderFrame; // make sure we get a process after the event...

    if (!stream->readFlag())
        return;

    if (stream->readFlag()) {
        mDamage = mClampF(stream->readFloat(DamageLevelBits) * mDataBlock->maxDamage, 0.f, mDataBlock->maxDamage);
        DamageState prevState = mDamageState;
        mDamageState = DamageState(stream->readInt(NumDamageStateBits));
        stream->readNormalVector(&damageDir, 8);
        //if (prevState != Destroyed && mDamageState == Destroyed && isProperlyAdded())
        //    blowUp();
        updateDamageLevel();
        updateDamageState();
    }

    if (stream->readFlag()) {
        for (S32 i = 0; i < MaxScriptThreads; i++) {
            if (stream->readFlag()) {
                Thread& st = mScriptThread[i];
                U32 oldID = st.id;
                U32 seq = stream->readInt(ThreadSequenceBits);
                st.id = stream->readInt(4);
                st.state = stream->readInt(2);
                st.forward = stream->readFlag();
                st.atEnd = stream->readFlag();
                if (st.sequence == seq && st.id == oldID)
                    updateThread(st);
                else
                    setThreadSequence(i, seq, false);
            }
        }
    }

    if (stream->readFlag())
    {
        for (S32 i = 0; i < MaxSoundThreads; i++)
        {
            if (stream->readFlag())
            {
                Sound& st = mSoundThread[i];
                st.play = stream->readFlag();
                if (st.play)
                {
                    st.profile = (SFXProfile*)stream->readRangedU32(DataBlockObjectIdFirst,
                        DataBlockObjectIdLast);
                }

                if (isProperlyAdded())
                    updateAudioState(st);
            }
        }
    }

    if (stream->readFlag()) {
        for (int i = 0; i < MaxMountedImages; i++) {
            if (stream->readFlag()) {
                MountedImage& image = mMountedImageList[i];
                ShapeBaseImageData* imageData = 0;
                if (stream->readFlag()) {
                    SimObjectId id = stream->readInt(DataBlockObjectIdBitSize) +
                        DataBlockObjectIdFirst;
                    if (!Sim::findObject(id, imageData)) {
                        con->setLastError("Invalid packet (mounted images).");
                        return;
                    }
                }

                StringHandle skinDesiredNameHandle = con->unpackStringHandleU(stream);

                image.wet = stream->readFlag();

                image.ammo = stream->readFlag();

                image.loaded = stream->readFlag();

                image.target = stream->readFlag();

                image.triggerDown = stream->readFlag();

                int count = stream->readInt(3);

                if ((image.dataBlock != imageData) || (image.skinNameHandle != skinDesiredNameHandle)) {

                    setImage(i, imageData, skinDesiredNameHandle, image.loaded, image.ammo, image.triggerDown);

                }

                if (isProperlyAdded()) {
                    // Normal processing
                    if (count != image.fireCount)
                    {
                        image.fireCount = count;
                        setImageState(i, getImageFireState(i), true);

                        if (imageData && imageData->lightType == ShapeBaseImageData::WeaponFireLight)
                        {
                            mLightTime = Sim::getCurrentTime();
                        }
                    }
                    updateImageState(i, 0);
                }
                else
                {
                    bool firing = stream->readFlag();
                    if (imageData)
                    {
                        // Initial state
                        image.fireCount = count;
                        if (firing)
                            setImageState(i, getImageFireState(i), true);
                    }
                }
            }
        }
    }

    if (stream->readFlag())
    {
        if (stream->readFlag())     // Cloaked and control
        {
            setCloakedState(stream->readFlag());
            mIsControlled = stream->readFlag();

            if ((mFading = stream->readFlag()) == true) {
                mFadeOut = stream->readFlag();
                if (mFadeOut)
                    mFadeVal = 1.0f;
                else
                    mFadeVal = 0;
                stream->read(&mFadeTime);
                mFadeDelay = 0;
                mFadeElapsedTime = 0;
            }
            else
                mFadeVal = F32(stream->readFlag());
        }
        if (stream->readFlag()) { // NameMask
            mShapeNameHandle = con->unpackStringHandleU(stream);
        }
        if (stream->readFlag())     // ShieldMask
        {
            // Cloaking, Shield, and invul masking
            Point3F shieldNormal;
            stream->readNormalVector(&shieldNormal, ShieldNormalBits);
            F32 energyPercent = stream->readFloat(EnergyLevelBits);
        }
        if (stream->readFlag()) {  // InvincibleMask
            F32 time, speed;
            stream->read(&time);
            stream->read(&speed);
            setupInvincibleEffect(time, speed);
        }

        if (stream->readFlag()) {  // SkinMask

            StringHandle skinDesiredNameHandle = con->unpackStringHandleU(stream);;

            if (mSkinNameHandle != skinDesiredNameHandle) {

                mSkinNameHandle = skinDesiredNameHandle;

                if (mShapeInstance) {

                    mShapeInstance->reSkin(mSkinNameHandle);

                    if (mSkinNameHandle.isValidString()) {

                        mSkinHash = _StringTable::hashString(mSkinNameHandle.getString());

                    }

                }

            }

        }
    }

    if (stream->readFlag()) {
        if (stream->readFlag()) {
            S32 gIndex = stream->readInt(NetConnection::GhostIdBitSize);
            ShapeBase* obj = dynamic_cast<ShapeBase*>(con->resolveGhost(gIndex));
            S32 node = stream->readInt(ShapeBaseData::NumMountPointBits);
            if (!obj)
            {
                con->setLastError("Invalid packet from server.");
                return;
            }
            obj->mountObject(this, node);
        }
        else
            unmount();
    }
}


//--------------------------------------------------------------------------

void ShapeBase::forceUncloak(const char* reason)
{
    AssertFatal(isServerObject(), "ShapeBase::forceUncloak: server only call");
    if (!mCloaked)
        return;

    Con::executef(mDataBlock, 3, "onForceUncloak", scriptThis(), reason ? reason : "");
}

void ShapeBase::setCloakedState(bool cloaked)
{
    if (cloaked == mCloaked)
        return;

    if (isServerObject())
        setMaskBits(CloakMask);

    // Have to do this for the client, if we are ghosted over in the initial
    //  packet as cloaked, we set the state immediately to the extreme
    if (isProperlyAdded() == false) {
        mCloaked = cloaked;
        if (mCloaked)
            mCloakLevel = 1.0;
        else
            mCloakLevel = 0.0;
    }
    else {
        mCloaked = cloaked;
    }
}


//--------------------------------------------------------------------------

void ShapeBase::setHidden(bool hidden)
{
    if (hidden != mHidden) {
        Parent::setHidden(hidden);
        // need to set a mask bit to make the ghost manager delete copies of this object
        // hacky, but oh well.
        setMaskBits(CloakMask);
        /*if (mHidden)
           addToScene();
        else
           removeFromScene();*/

        //mHidden = hidden;
    }
}

void ShapeBase::onUnhide()
{
    mCreateTime = Platform::getVirtualMilliseconds();
}

//--------------------------------------------------------------------------

void ShapeBaseConvex::findNodeTransform()
{
    S32 dl = pShapeBase->mDataBlock->collisionDetails[hullId];

    TSShapeInstance* si = pShapeBase->getShapeInstance();
    TSShape* shape = si->getShape();

    const TSShape::Detail* detail = &shape->details[dl];
    const S32 subs = detail->subShapeNum;
    const S32 start = shape->subShapeFirstObject[subs];
    const S32 end = start + shape->subShapeNumObjects[subs];

    // Find the first object that contains a mesh for this
    // detail level. There should only be one mesh per
    // collision detail level.
    for (S32 i = start; i < end; i++)
    {
        const TSShape::Object* obj = &shape->objects[i];
        if (obj->numMeshes && detail->objectDetailNum < obj->numMeshes)
        {
            nodeTransform = &si->mNodeTransforms[obj->nodeIndex];
            return;
        }
    }
    return;
}

const MatrixF& ShapeBaseConvex::getTransform() const
{
    // If the transform isn't specified, it's assumed to be the
    // origin of the shape.
    const MatrixF& omat = (transform != 0) ? *transform : mObject->getTransform();

    // Multiply on the mesh shape offset
    // tg: Returning this static here is not really a good idea, but
    // all this Convex code needs to be re-organized.
    if (nodeTransform) {
        static MatrixF mat;
        mat.mul(omat, *nodeTransform);
        return mat;
    }
    return omat;
}

Box3F ShapeBaseConvex::getBoundingBox() const
{
    const MatrixF& omat = (transform != 0) ? *transform : mObject->getTransform();
    return getBoundingBox(omat, mObject->getScale());
}

Box3F ShapeBaseConvex::getBoundingBox(const MatrixF& mat, const Point3F& scale) const
{
    Box3F newBox = box;
    newBox.min.convolve(scale);
    newBox.max.convolve(scale);
    mat.mul(newBox);
    return newBox;
}

Point3F ShapeBaseConvex::support(const VectorF& v) const
{
    TSShape::ConvexHullAccelerator* pAccel =
        pShapeBase->mShapeInstance->getShape()->getAccelerator(pShapeBase->mDataBlock->collisionDetails[hullId]);
    AssertFatal(pAccel != NULL, "Error, no accel!");

    F32 currMaxDP = mDot(pAccel->vertexList[0], v);
    U32 index = 0;
    for (U32 i = 1; i < pAccel->numVerts; i++) {
        F32 dp = mDot(pAccel->vertexList[i], v);
        if (dp > currMaxDP) {
            currMaxDP = dp;
            index = i;
        }
    }

    return pAccel->vertexList[index];
}


void ShapeBaseConvex::getFeatures(const MatrixF& mat, const VectorF& n, ConvexFeature* cf)
{
    cf->material = 0;
    cf->object = mObject;

    TSShape::ConvexHullAccelerator* pAccel =
        pShapeBase->mShapeInstance->getShape()->getAccelerator(pShapeBase->mDataBlock->collisionDetails[hullId]);
    AssertFatal(pAccel != NULL, "Error, no accel!");

    F32 currMaxDP = mDot(pAccel->vertexList[0], n);
    U32 index = 0;
    U32 i;
    for (i = 1; i < pAccel->numVerts; i++) {
        F32 dp = mDot(pAccel->vertexList[i], n);
        if (dp > currMaxDP) {
            currMaxDP = dp;
            index = i;
        }
    }

    const U8* emitString = pAccel->emitStrings[index];
    U32 currPos = 0;
    U32 numVerts = emitString[currPos++];
    for (i = 0; i < numVerts; i++) {
        cf->mVertexList.increment();
        U32 index = emitString[currPos++];
        mat.mulP(pAccel->vertexList[index], &cf->mVertexList.last());
    }

    U32 numEdges = emitString[currPos++];
    for (i = 0; i < numEdges; i++) {
        U32 ev0 = emitString[currPos++];
        U32 ev1 = emitString[currPos++];
        cf->mEdgeList.increment();
        cf->mEdgeList.last().vertex[0] = ev0;
        cf->mEdgeList.last().vertex[1] = ev1;
    }

    U32 numFaces = emitString[currPos++];
    for (i = 0; i < numFaces; i++) {
        cf->mFaceList.increment();
        U32 plane = emitString[currPos++];
        mat.mulV(pAccel->normalList[plane], &cf->mFaceList.last().normal);
        for (U32 j = 0; j < 3; j++)
            cf->mFaceList.last().vertex[j] = emitString[currPos++];
    }
}


void ShapeBaseConvex::getPolyList(AbstractPolyList* list)
{
    list->setTransform(&pShapeBase->getTransform(), pShapeBase->getScale());
    list->setObject(pShapeBase);

    pShapeBase->mShapeInstance->animate(pShapeBase->mDataBlock->collisionDetails[hullId]);
    pShapeBase->mShapeInstance->buildPolyList(list, pShapeBase->mDataBlock->collisionDetails[hullId]);
}


//--------------------------------------------------------------------------

bool ShapeBase::isInvincible()
{
    if (mDataBlock)
    {
        return mDataBlock->isInvincible;
    }
    return false;
}

void ShapeBase::startFade(F32 fadeTime, F32 fadeDelay, bool fadeOut)
{
    setMaskBits(CloakMask);
    mFadeElapsedTime = 0;
    mFading = true;
    if (fadeDelay < 0)
        fadeDelay = 0;
    if (fadeTime < 0)
        fadeTime = 0;
    mFadeTime = fadeTime;
    mFadeDelay = fadeDelay;
    mFadeOut = fadeOut;
    mFadeVal = F32(mFadeOut);
}

//--------------------------------------------------------------------------

void ShapeBase::setShapeName(const char* name)
{
    if (!isGhost()) {
        if (name[0] != '\0') {
            // Use tags for better network performance
            // Should be a tag, but we'll convert to one if it isn't.
            if (name[0] == StringTagPrefixByte)
                mShapeNameHandle = StringHandle(U32(dAtoi(name + 1)));
            else
                mShapeNameHandle = StringHandle(name);
        }
        else {
            mShapeNameHandle = StringHandle();
        }
        setMaskBits(NameMask);
    }
}


void ShapeBase::setSkinName(const char* name)
{
    if (!isGhost()) {
        if (name[0] != '\0') {

            // Use tags for better network performance
            // Should be a tag, but we'll convert to one if it isn't.
            if (name[0] == StringTagPrefixByte) {
                mSkinNameHandle = StringHandle(U32(dAtoi(name + 1)));
            }
            else {
                mSkinNameHandle = StringHandle(name);
            }
        }
        else {
            mSkinNameHandle = StringHandle();
        }
        setMaskBits(SkinMask);
    }
}

Material* ShapeBase::getMaterial(U32 material)
{
    MaterialList* list;
    if (mShapeInstance && ((list = mShapeInstance->getMaterialList()) != NULL || (list = mShapeInstance->getShape()->materialList) != NULL))
    {
        return list->getMappedMaterial(material);
    }

    return NULL;
}

//----------------------------------------------------------------------------
ConsoleMethod(ShapeBase, playAudio, bool, 4, 4, "(int slot, SFXProfile profile)")
{
    U32 slot = dAtoi(argv[2]);
    if (slot >= 0 && slot < ShapeBase::MaxScriptThreads) {
        SFXProfile* profile;
        if (Sim::findObject(argv[3], profile)) {
            object->playAudio(slot, profile);
            return true;
        }
    }
    return false;
}

ConsoleMethod(ShapeBase, stopAudio, bool, 3, 3, "(int slot)")
{
    U32 slot = dAtoi(argv[2]);
    if (slot >= 0 && slot < ShapeBase::MaxScriptThreads) {
        object->stopAudio(slot);
        return true;
    }
    return false;
}


//----------------------------------------------------------------------------
ConsoleMethod(ShapeBase, playThread, bool, 3, 6, "(int slot, string sequenceName)")
{
    U32 slot = dAtoi(argv[2]);
    if (slot >= ShapeBase::MaxScriptThreads)
    {
        Con::errorf("ShapeBase::playThread: slot out of range");
        return false;
    }

    if (argc != 6)
    {
        if (argc < 4)
        {
            if (object->playThread(slot))
                return true;
        } else
        {
            if (object->getShape())
            {
                S32 seq = object->getShape()->findSequence(argv[3]);
                if (seq != -1 && object->setThreadSequence(slot, seq))
                {
                    if (argc != 5)
                        return true;

                    return object->setTimeScale(slot, dAtof(argv[4]));
                }
            }
        }

        return false;
    }

    U32 cmd = dAtoi(argv[2]);
    S32 seq = -1;
    if (object->getShape())
        seq = object->getShape()->findSequence(argv[3]);
    else
        return false;

    ShapeBase::ThreadCmd tcmd;
    tcmd.slot = cmd;
    tcmd.seq = seq;
    tcmd.timeScale = dAtof(argv[4]);
    tcmd.delayTicks = dAtoi(argv[5]) / 32;

    object->playThreadDelayed(tcmd);

    return true;
}

ConsoleMethod(ShapeBase, setThreadDir, bool, 4, 4, "(int slot, bool isForward)")
{
    int slot = dAtoi(argv[2]);
    if (slot >= 0 && slot < ShapeBase::MaxScriptThreads) {
        if (object->setThreadDir(slot, dAtob(argv[3])))
            return true;
    }
    return false;
}

ConsoleMethod(ShapeBase, stopThread, bool, 3, 3, "(int slot)")
{
    int slot = dAtoi(argv[2]);
    if (slot >= 0 && slot < ShapeBase::MaxScriptThreads) {
        if (object->stopThread(slot))
            return true;
    }
    return false;
}

ConsoleMethod(ShapeBase, pauseThread, bool, 3, 3, "(int slot)")
{
    int slot = dAtoi(argv[2]);
    if (slot >= 0 && slot < ShapeBase::MaxScriptThreads) {
        if (object->pauseThread(slot))
            return true;
    }
    return false;
}


//----------------------------------------------------------------------------
ConsoleMethod(ShapeBase, mountObject, bool, 4, 4, "( ShapeBase object, int slot )"
    "Mount ourselves on an object in the specified slot.")
{
    ShapeBase* target;
    if (Sim::findObject(argv[2], target)) {
        S32 node = -1;
        dSscanf(argv[3], "%d", &node);
        if (node >= 0 && node < ShapeBaseData::NumMountPoints)
            object->mountObject(target, node);
        return true;
    }
    return false;
}

ConsoleMethod(ShapeBase, unmountObject, bool, 3, 3, "(ShapeBase obj)"
    "Unmount an object from ourselves.")
{
    ShapeBase* target;
    if (Sim::findObject(argv[2], target)) {
        object->unmountObject(target);
        return true;
    }
    return false;
}

ConsoleMethod(ShapeBase, unmount, void, 2, 2, "Unmount from the currently mounted object if any.")
{
    object->unmount();
}

ConsoleMethod(ShapeBase, isMounted, bool, 2, 2, "Are we mounted?")
{
    return object->isMounted();
}

ConsoleMethod(ShapeBase, getObjectMount, S32, 2, 2, "Returns the ShapeBase we're mounted on.")
{
    return object->isMounted() ? object->getObjectMount()->getId() : 0;
}

ConsoleMethod(ShapeBase, getMountedObjectCount, S32, 2, 2, "")
{
    return object->getMountedObjectCount();
}

ConsoleMethod(ShapeBase, getMountedObject, S32, 3, 3, "(int slot)")
{
    ShapeBase* mobj = object->getMountedObject(dAtoi(argv[2]));
    return mobj ? mobj->getId() : 0;
}

ConsoleMethod(ShapeBase, getMountedObjectNode, S32, 3, 3, "(int node)")
{
    return object->getMountedObjectNode(dAtoi(argv[2]));
}

ConsoleMethod(ShapeBase, getMountNodeObject, S32, 3, 3, "(int node)")
{
    ShapeBase* mobj = object->getMountNodeObject(dAtoi(argv[2]));
    return mobj ? mobj->getId() : 0;
}


//----------------------------------------------------------------------------
ConsoleMethod(ShapeBase, mountImage, bool, 4, 6, "(ShapeBaseImageData image, int slot, bool loaded=true, string skinTag=NULL)")
{
    ShapeBaseImageData* imageData;
    if (Sim::findObject(argv[2], imageData)) {
        U32 slot = dAtoi(argv[3]);
        bool loaded = (argc == 5) ? dAtob(argv[4]) : true;
        StringHandle team;
        if (argc == 6)
        {
            if (argv[5][0] == StringTagPrefixByte)
                team = StringHandle(U32(dAtoi(argv[5] + 1)));
        }
        if (slot >= 0 && slot < ShapeBase::MaxMountedImages)
            object->mountImage(imageData, slot, loaded, team);
    }
    return false;
}

ConsoleMethod(ShapeBase, unmountImage, bool, 3, 3, "(int slot)")
{
    int slot = dAtoi(argv[2]);
    if (slot >= 0 && slot < ShapeBase::MaxMountedImages)
        return object->unmountImage(slot);
    return false;
}

ConsoleMethod(ShapeBase, getMountedImage, S32, 3, 3, "(int slot)")
{
    int slot = dAtoi(argv[2]);
    if (slot >= 0 && slot < ShapeBase::MaxMountedImages)
        if (ShapeBaseImageData* data = object->getMountedImage(slot))
            return data->getId();
    return 0;
}

ConsoleMethod(ShapeBase, getPendingImage, S32, 3, 3, "(int slot)")
{
    int slot = dAtoi(argv[2]);
    if (slot >= 0 && slot < ShapeBase::MaxMountedImages)
        if (ShapeBaseImageData* data = object->getPendingImage(slot))
            return data->getId();
    return 0;
}

ConsoleMethod(ShapeBase, isImageFiring, bool, 3, 3, "(int slot)")
{
    int slot = dAtoi(argv[2]);
    if (slot >= 0 && slot < ShapeBase::MaxMountedImages)
        return object->isImageFiring(slot);
    return false;
}

ConsoleMethod(ShapeBase, isImageMounted, bool, 3, 3, "(ShapeBaseImageData db)")
{
    ShapeBaseImageData* imageData;
    if (Sim::findObject(argv[2], imageData))
        return object->isImageMounted(imageData);
    return false;
}

ConsoleMethod(ShapeBase, getMountSlot, S32, 3, 3, "(ShapeBaseImageData db)")
{
    ShapeBaseImageData* imageData;
    if (Sim::findObject(argv[2], imageData))
        return object->getMountSlot(imageData);
    return -1;
}

ConsoleMethod(ShapeBase, getImageSkinTag, S32, 3, 3, "(int slot)")
{
    int slot = dAtoi(argv[2]);
    if (slot >= 0 && slot < ShapeBase::MaxMountedImages)
        return object->getImageSkinTag(slot).getIndex();
    return -1;
}

ConsoleMethod(ShapeBase, getImageState, const char*, 3, 3, "(int slot)")
{
    int slot = dAtoi(argv[2]);
    if (slot >= 0 && slot < ShapeBase::MaxMountedImages)
        return object->getImageState(slot);
    return "Error";
}

ConsoleMethod(ShapeBase, getImageTrigger, bool, 3, 3, "(int slot)")
{
    int slot = dAtoi(argv[2]);
    if (slot >= 0 && slot < ShapeBase::MaxMountedImages)
        return object->getImageTriggerState(slot);
    return false;
}

ConsoleMethod(ShapeBase, setImageTrigger, bool, 4, 4, "(int slot, bool isTriggered)")
{
    int slot = dAtoi(argv[2]);
    if (slot >= 0 && slot < ShapeBase::MaxMountedImages) {
        object->setImageTriggerState(slot, dAtob(argv[3]));
        return object->getImageTriggerState(slot);
    }
    return false;
}

ConsoleMethod(ShapeBase, getImageAmmo, bool, 3, 3, "(int slot)")
{
    int slot = dAtoi(argv[2]);
    if (slot >= 0 && slot < ShapeBase::MaxMountedImages)
        return object->getImageAmmoState(slot);
    return false;
}

ConsoleMethod(ShapeBase, setImageAmmo, bool, 4, 4, "(int slot, bool hasAmmo)")
{
    int slot = dAtoi(argv[2]);
    if (slot >= 0 && slot < ShapeBase::MaxMountedImages) {
        bool ammo = dAtob(argv[3]);
        object->setImageAmmoState(slot, dAtob(argv[3]));
        return ammo;
    }
    return false;
}

ConsoleMethod(ShapeBase, getImageLoaded, bool, 3, 3, "(int slot)")
{
    int slot = dAtoi(argv[2]);
    if (slot >= 0 && slot < ShapeBase::MaxMountedImages)
        return object->getImageLoadedState(slot);
    return false;
}

ConsoleMethod(ShapeBase, setImageLoaded, bool, 4, 4, "(int slot, bool loaded)")
{
    int slot = dAtoi(argv[2]);
    if (slot >= 0 && slot < ShapeBase::MaxMountedImages) {
        bool loaded = dAtob(argv[3]);
        object->setImageLoadedState(slot, dAtob(argv[3]));
        return loaded;
    }
    return false;
}

ConsoleMethod(ShapeBase, getMuzzleVector, const char*, 3, 3, "(int slot)")
{
    int slot = dAtoi(argv[2]);
    if (slot >= 0 && slot < ShapeBase::MaxMountedImages) {
        VectorF v;
        object->getMuzzleVector(slot, &v);
        char* buff = Con::getReturnBuffer(100);
        dSprintf(buff, 100, "%g %g %g", v.x, v.y, v.z);
        return buff;
    }
    return "0 1 0";
}

ConsoleMethod(ShapeBase, getMuzzlePoint, const char*, 3, 3, "(int slot)")
{
    int slot = dAtoi(argv[2]);
    if (slot >= 0 && slot < ShapeBase::MaxMountedImages) {
        Point3F p;
        object->getMuzzlePoint(slot, &p);
        char* buff = Con::getReturnBuffer(100);
        dSprintf(buff, 100, "%g %g %g", p.x, p.y, p.z);
        return buff;
    }
    return "0 0 0";
}

ConsoleMethod(ShapeBase, getSlotTransform, const char*, 3, 3, "(int slot)")
{
    int slot = dAtoi(argv[2]);
    MatrixF xf(true);
    if (slot >= 0 && slot < ShapeBase::MaxMountedImages)
        object->getMountTransform(slot, &xf);

    Point3F pos;
    xf.getColumn(3, &pos);
    AngAxisF aa(xf);
    char* buff = Con::getReturnBuffer(200);
    dSprintf(buff, 200, "%g %g %g %g %g %g %g",
        pos.x, pos.y, pos.z, aa.axis.x, aa.axis.y, aa.axis.z, aa.angle);
    return buff;
}

ConsoleMethod(ShapeBase, getAIRepairPoint, const char*, 2, 2, "Get the position at which the AI should stand to repair things.")
{
    Point3F pos = object->getAIRepairPoint();
    char* buff = Con::getReturnBuffer(200);
    dSprintf(buff, 200, "%g %g %g", pos.x, pos.y, pos.z);
    return buff;
}

ConsoleMethod(ShapeBase, getVelocity, const char*, 2, 2, "")
{
    const VectorF& vel = object->getVelocity();
    char* buff = Con::getReturnBuffer(100);
    dSprintf(buff, 100, "%g %g %g", vel.x, vel.y, vel.z);
    return buff;
}

ConsoleMethod(ShapeBase, setVelocity, bool, 3, 3, "(Vector3F vel)")
{
    VectorF vel(0, 0, 0);
    dSscanf(argv[2], "%g %g %g", &vel.x, &vel.y, &vel.z);
    object->setVelocity(vel);
    return true;
}

ConsoleMethod(ShapeBase, applyImpulse, bool, 4, 4, "(Point3F Pos, VectorF vel)")
{
    Point3F pos(0, 0, 0);
    VectorF vel(0, 0, 0);
    dSscanf(argv[2], "%g %g %g", &pos.x, &pos.y, &pos.z);
    dSscanf(argv[3], "%g %g %g", &vel.x, &vel.y, &vel.z);
    object->applyImpulse(pos, vel);
    return true;
}

ConsoleMethod(ShapeBase, getEyeVector, const char*, 2, 2, "")
{
    MatrixF mat;
    object->getEyeTransform(&mat);
    VectorF v2;
    mat.getColumn(1, &v2);
    char* buff = Con::getReturnBuffer(100);
    dSprintf(buff, 100, "%g %g %g", v2.x, v2.y, v2.z);
    return buff;
}

ConsoleMethod(ShapeBase, getEyePoint, const char*, 2, 2, "")
{
    MatrixF mat;
    object->getEyeTransform(&mat);
    Point3F ep;
    mat.getColumn(3, &ep);
    char* buff = Con::getReturnBuffer(100);
    dSprintf(buff, 100, "%g %g %g", ep.x, ep.y, ep.z);
    return buff;
}

ConsoleMethod(ShapeBase, getEyeTransform, const char*, 2, 2, "")
{
    MatrixF mat;
    object->getEyeTransform(&mat);

    Point3F pos;
    mat.getColumn(3, &pos);
    AngAxisF aa(mat);
    char* buff = Con::getReturnBuffer(100);
    dSprintf(buff, 100, "%g %g %g %g %g %g %g",
        pos.x, pos.y, pos.z, aa.axis.x, aa.axis.y, aa.axis.z, aa.angle);
    return buff;
}

ConsoleMethod(ShapeBase, setEnergyLevel, void, 3, 3, "(float level)")
{
    object->setEnergyLevel(dAtof(argv[2]));
}

ConsoleMethod(ShapeBase, getEnergyLevel, F32, 2, 2, "")
{
    return object->getEnergyLevel();
}

ConsoleMethod(ShapeBase, getEnergyPercent, F32, 2, 2, "")
{
    return object->getEnergyValue();
}

ConsoleMethod(ShapeBase, setDamageLevel, void, 3, 3, "(float level)")
{
    object->setDamageLevel(dAtof(argv[2]));
}

ConsoleMethod(ShapeBase, getDamageLevel, F32, 2, 2, "")
{
    return object->getDamageLevel();
}

ConsoleMethod(ShapeBase, getDamagePercent, F32, 2, 2, "")
{
    return object->getDamageValue();
}

ConsoleMethod(ShapeBase, setDamageState, bool, 3, 3, "(string state)")
{
    return object->setDamageState(argv[2]);
}

ConsoleMethod(ShapeBase, getDamageState, const char*, 2, 2, "")
{
    return object->getDamageStateName();
}

ConsoleMethod(ShapeBase, isDestroyed, bool, 2, 2, "")
{
    return object->isDestroyed();
}

ConsoleMethod(ShapeBase, isDisabled, bool, 2, 2, "True if the state is not Enabled.")
{
    return object->getDamageState() != ShapeBase::Enabled;
}

ConsoleMethod(ShapeBase, isEnabled, bool, 2, 2, "")
{
    return object->getDamageState() == ShapeBase::Enabled;
}

ConsoleMethod(ShapeBase, applyDamage, void, 3, 3, "(float amt)")
{
    object->applyDamage(dAtof(argv[2]));
}

ConsoleMethod(ShapeBase, applyRepair, void, 3, 3, "(float amt)")
{
    object->applyRepair(dAtof(argv[2]));
}

ConsoleMethod(ShapeBase, setRepairRate, void, 3, 3, "(float amt)")
{
    F32 rate = dAtof(argv[2]);
    if (rate < 0)
        rate = 0;
    object->setRepairRate(rate);
}

ConsoleMethod(ShapeBase, getRepairRate, F32, 2, 2, "")
{
    return object->getRepairRate();
}

ConsoleMethod(ShapeBase, setRechargeRate, void, 3, 3, "(float rate)")
{
    object->setRechargeRate(dAtof(argv[2]));
}

ConsoleMethod(ShapeBase, getRechargeRate, F32, 2, 2, "")
{
    return object->getRechargeRate();
}

ConsoleMethod(ShapeBase, getControllingClient, S32, 2, 2, "Returns a GameConnection.")
{
    if (GameConnection* con = object->getControllingClient())
        return con->getId();
    return 0;
}

ConsoleMethod(ShapeBase, getControllingObject, S32, 2, 2, "")
{
    if (ShapeBase* con = object->getControllingObject())
        return con->getId();
    return 0;
}

// return true if can cloak, otherwise the reason why object cannot cloak
ConsoleMethod(ShapeBase, canCloak, bool, 2, 2, "")
{
    return true;
}

ConsoleMethod(ShapeBase, setCloaked, void, 3, 3, "(bool isCloaked)")
{
    bool cloaked = dAtob(argv[2]);
    if (object->isServerObject())
        object->setCloakedState(cloaked);
}

ConsoleMethod(ShapeBase, isCloaked, bool, 2, 2, "")
{
    return object->getCloakedState();
}

ConsoleMethod(ShapeBase, setDamageFlash, void, 3, 3, "(float lvl)")
{
    F32 flash = dAtof(argv[2]);
    if (object->isServerObject())
        object->setDamageFlash(flash);
}

ConsoleMethod(ShapeBase, getDamageFlash, F32, 2, 2, "")
{
    return object->getDamageFlash();
}

ConsoleMethod(ShapeBase, setWhiteOut, void, 3, 3, "(float flashLevel)")
{
    F32 flash = dAtof(argv[2]);
    if (object->isServerObject())
        object->setWhiteOut(flash);
}

ConsoleMethod(ShapeBase, getWhiteOut, F32, 2, 2, "")
{
    return object->getWhiteOut();
}

ConsoleMethod(ShapeBase, getCameraFov, F32, 2, 2, "")
{
    if (object->isServerObject())
        return object->getCameraFov();
    return 0.0;
}

ConsoleMethod(ShapeBase, setCameraFov, void, 3, 3, "(float fov)")
{
    if (object->isServerObject())
        object->setCameraFov(dAtof(argv[2]));
}

ConsoleMethod(ShapeBase, setInvincibleMode, void, 4, 4, "(float time, float speed)")
{
    object->setupInvincibleEffect(dAtof(argv[2]), dAtof(argv[3]));
}

ConsoleFunction(setShadowDetailLevel, void, 2, 2, "setShadowDetailLevel(val 0...1);")
{
    argc;
    F32 val = dAtof(argv[1]);
    if (val < 0.0f)
        val = 0.0f;
    else if (val > 1.0f)
        val = 1.0f;

    if (mFabs(Shadow::getGlobalShadowDetailLevel() - val) < 0.001f)
        return;

    // shadow details determined in two places:
    // 1. setGlobalShadowDetailLevel
    // 2. static shape header has some #defines that determine
    //    at what level of shadow detail each type of
    //    object uses a generic shadow or no shadow at all
    Shadow::setGlobalShadowDetailLevel(val);
    Con::setFloatVariable("$pref::Shadows", val);
}

ConsoleMethod(ShapeBase, startFade, void, 5, 5, "( int fadeTimeMS, int fadeDelayMS, bool fadeOut )")
{
    U32   fadeTime;
    U32   fadeDelay;
    bool  fadeOut;

    dSscanf(argv[2], "%d", &fadeTime);
    dSscanf(argv[3], "%d", &fadeDelay);
    fadeOut = dAtob(argv[4]);

    object->startFade(fadeTime / 1000.0, fadeDelay / 1000.0, fadeOut);
}

ConsoleMethod(ShapeBase, setDamageVector, void, 3, 3, "(Vector3F origin)")
{
    VectorF normal;
    dSscanf(argv[2], "%g %g %g", &normal.x, &normal.y, &normal.z);
    normal.normalize();
    object->setDamageDir(VectorF(normal.x, normal.y, normal.z));
}

ConsoleMethod(ShapeBase, setShapeName, void, 3, 3, "(string tag)")
{
    object->setShapeName(argv[2]);
}


ConsoleMethod(ShapeBase, setSkinName, void, 3, 3, "(string tag)")
{
    object->setSkinName(argv[2]);
}

ConsoleMethod(ShapeBase, getShapeName, const char*, 2, 2, "")
{
    return object->getShapeName();
}


ConsoleMethod(ShapeBase, getSkinName, const char*, 2, 2, "")
{
    return object->getSkinName();
}

//----------------------------------------------------------------------------
void ShapeBase::consoleInit()
{
    Con::addVariable("SB::DFDec", TypeF32, &sDamageFlashDec);
    Con::addVariable("SB::WODec", TypeF32, &sWhiteoutDec);
    Con::addVariable("pref::environmentMaps", TypeBool, &gRenderEnvMaps);
}
