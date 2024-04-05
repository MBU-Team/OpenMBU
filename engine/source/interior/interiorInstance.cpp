//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "interior/interiorInstance.h"
#include "interior/interior.h"
#include "interior/pathedInterior.h"
#include "console/consoleTypes.h"
#include "sceneGraph/sceneGraph.h"
#include "sceneGraph/sceneState.h"
#include "core/bitStream.h"
#include "gfx/gBitmap.h"
#include "math/mathIO.h"
#include "editor/editor.h"
#include "interior/interiorResObjects.h"
#include "game/trigger.h"
#include "sim/simPath.h"
#include "interior/forceField.h"
#include "sceneGraph/lightManager.h"
#include "collision/convex.h"
#include "sfx/sfxProfile.h"
#include "sfx/sfxEnvironment.h"
#include "core/frameAllocator.h"
#include "sim/netConnection.h"
#include "platform/profiler.h"
#include "gui/core/guiTSControl.h"
#include "math/mathUtils.h"
#include "renderInstance/renderInstMgr.h"
#include "sim/pathManager.h"
#include "materials/material.h"

//--------------------------------------------------------------------------
//-------------------------------------- Local classes, data, and functions
//
const U32 csgMaxZoneSize = 256;
bool sgScopeBoolArray[256];

ConsoleFunctionGroupBegin(Interiors, "");

#if defined(TORQUE_DEBUG) || defined (INTERNAL_RELEASE)
ConsoleFunction(setInteriorRenderMode, void, 2, 2, "(int modeNum)")
{
    S32 mode = dAtoi(argv[1]);
    if (mode < 0 || mode > Interior::ShowDetailLevel)
        mode = 0;

    Interior::smRenderMode = mode;
}

ConsoleFunction(setInteriorFocusedDebug, void, 2, 2, "(bool enable)")
{
    if (dAtob(argv[1])) {
        Interior::smFocusedDebug = true;
    }
    else {
        Interior::smFocusedDebug = false;
    }
}

#endif

ConsoleFunction(isPointInside, bool, 2, 4, "(Point3F pos) or (float x, float y, float z)")
{
    static bool lastValue = false;

    if (!(argc == 2 || argc == 4))
    {
        Con::errorf(ConsoleLogEntry::General, "cIsPointInside: invalid parameters");
        return(lastValue);
    }

    Point3F pos;
    if (argc == 2)
        dSscanf(argv[1], "%g %g %g", &pos.x, &pos.y, &pos.z);
    else
    {
        pos.x = dAtof(argv[1]);
        pos.y = dAtof(argv[2]);
        pos.z = dAtof(argv[3]);
    }

    RayInfo collision;
    if (getCurrentClientContainer()->castRay(pos, Point3F(pos.x, pos.y, pos.z - 2000.f), InteriorObjectType, &collision))
    {
        if (collision.face == -1)
            Con::errorf(ConsoleLogEntry::General, "cIsPointInside: failed to find hit face on interior");
        else
        {
            InteriorInstance* interior = dynamic_cast<InteriorInstance*>(collision.object);
            if (interior)
                lastValue = !interior->getDetailLevel(0)->isSurfaceOutsideVisible(collision.face);
            else
                Con::errorf(ConsoleLogEntry::General, "cIsPointInside: invalid interior on collision");
        }
    }

    return(lastValue);
}


ConsoleFunctionGroupEnd(Interiors);

ConsoleMethod(InteriorInstance, setAlarmMode, void, 3, 3, "(string mode) Mode is 'On' or 'Off'")
{
    AssertFatal(dynamic_cast<InteriorInstance*>(object) != NULL,
        "Error, how did a non-interior get here?");

    bool alarm;
    if (dStricmp(argv[2], "On") == 0)
        alarm = true;
    else
        alarm = false;

    InteriorInstance* interior = static_cast<InteriorInstance*>(object);
    if (interior->isClientObject() || gSPMode) {
        Con::errorf(ConsoleLogEntry::General, "InteriorInstance: client objects may not receive console commands.  Ignored");
        return;
    }

    interior->setAlarmMode(alarm);
}


ConsoleMethod(InteriorInstance, setSkinBase, void, 3, 3, "(string basename)")
{
    AssertFatal(dynamic_cast<InteriorInstance*>(object) != NULL,
        "Error, how did a non-interior get here?");

    InteriorInstance* interior = static_cast<InteriorInstance*>(object);
    if (interior->isClientObject() || gSPMode) {
        Con::errorf(ConsoleLogEntry::General, "InteriorInstance: client objects may not receive console commands.  Ignored");
        return;
    }

    interior->setSkinBase(argv[2]);
}

ConsoleMethod(InteriorInstance, getNumDetailLevels, S32, 2, 2, "")
{
    InteriorInstance* instance = static_cast<InteriorInstance*>(object);
    return(instance->getNumDetailLevels());
}

ConsoleMethod(InteriorInstance, setDetailLevel, void, 3, 3, "(int level)")
{
    InteriorInstance* instance = static_cast<InteriorInstance*>(object);
    if (instance->isServerObject())
    {
        NetConnection* toServer = NetConnection::getConnectionToServer();
        NetConnection* toClient = NetConnection::getLocalClientConnection();
        if (!toClient || !toServer)
            return;

        S32 index = toClient->getGhostIndex(instance);
        if (index == -1)
            return;

        InteriorInstance* clientInstance = dynamic_cast<InteriorInstance*>(toServer->resolveGhost(index));
        if (clientInstance)
            clientInstance->setDetailLevel(dAtoi(argv[2]));
    }
    else
        instance->setDetailLevel(dAtoi(argv[2]));
}

ConsoleMethod(InteriorInstance, magicButton, void, 2, 2, "")
{
    if (object->isGhost())
    {
        Con::errorf("InteriorInstance: client objects may not receive console commands.  Ignored");
        return;
    }

    object->addChildren();
}

//--------------------------------------------------------------------------
//-------------------------------------- Static functions
//
IMPLEMENT_CO_NETOBJECT_V1(InteriorInstance);

//------------------------------------------------------------------------------
//-------------------------------------- InteriorInstance
//
bool InteriorInstance::smDontRestrictOutside = false;
F32  InteriorInstance::smDetailModification = 1.0f;


InteriorInstance::InteriorInstance()
{
    mAlarmState = false;

    mInteriorFileName = NULL;
    mTypeMask = InteriorObjectType | StaticObjectType |
        StaticRenderedObjectType | ShadowCasterObjectType;
    mNetFlags.set(Ghostable | ScopeAlways);


    mShowTerrainInside = false;

    mLMHandle = LM_HANDLE(-1);

    mSkinBase = StringTable->insert("base");
    mAudioProfile = 0;
    mAudioEnvironment = 0;
    mForcedDetailLevel = -1;

    mConvexList = new Convex;
    mCRC = 0;
}

InteriorInstance::~InteriorInstance()
{
    U32 i;

    delete mConvexList;
    mConvexList = NULL;

    
   for (i = 0; i < mMaterialMaps.size(); i++)
   {
      delete mMaterialMaps[i];
      mMaterialMaps[i] = NULL;
   }
}


void InteriorInstance::init()
{
    // Does nothing for the moment
}


void InteriorInstance::destroy()
{
    // Also does nothing for the moment
}


//--------------------------------------------------------------------------
// Inspection
static SFXProfile* saveAudioProfile = 0;
static SFXEnvironment* saveAudioEnvironment = 0;
void InteriorInstance::inspectPreApply()
{
    saveAudioProfile = mAudioProfile;
    saveAudioEnvironment = mAudioEnvironment;
}

void InteriorInstance::inspectPostApply()
{
    if ((mAudioProfile != saveAudioProfile) || (mAudioEnvironment != saveAudioEnvironment))
        setMaskBits(AudioMask);

    // Update the Transform on Editor Apply.
    setMaskBits(TransformMask);
}

//--------------------------------------------------------------------------
//-------------------------------------- Console functionality
//
void InteriorInstance::initPersistFields()
{
    Parent::initPersistFields();

    addGroup("Media");
    addField("interiorFile", TypeFilename, Offset(mInteriorFileName, InteriorInstance));
    endGroup("Media");

    addGroup("Audio");
    addField("audioProfile", TypeSFXProfilePtr, Offset(mAudioProfile, InteriorInstance));
    addField("audioEnvironment", TypeSFXEnvironmentPtr, Offset(mAudioEnvironment, InteriorInstance));
    endGroup("Audio");

    addGroup("Misc");
    addField("showTerrainInside", TypeBool, Offset(mShowTerrainInside, InteriorInstance));
    endGroup("Misc");

}

void InteriorInstance::consoleInit()
{
    //-------------------------------------- Class level variables
    Con::addVariable("pref::Interior::ShowEnvironmentMaps", TypeBool, &Interior::smRenderEnvironmentMaps);
    Con::addVariable("pref::Interior::VertexLighting", TypeBool, &Interior::smUseVertexLighting);
    Con::addVariable("pref::Interior::TexturedFog", TypeBool, &Interior::smUseTexturedFog);
    Con::addVariable("pref::Interior::lockArrays", TypeBool, &Interior::smLockArrays);

    Con::addVariable("pref::Interior::detailAdjust", TypeF32, &InteriorInstance::smDetailModification);

    // DEBUG ONLY!!!
#ifdef TORQUE_DEBUG
    Con::addVariable("Interior::DontRestrictOutside", TypeBool, &smDontRestrictOutside);
#endif
}

//--------------------------------------------------------------------------
void InteriorInstance::renewOverlays()
{
    /*
       StringTableEntry baseName = dStricmp(mSkinBase, "base") == 0 ? "blnd" : mSkinBase;

       for (U32 i = 0; i < mMaterialMaps.size(); i++) {
          MaterialList* pMatList = mMaterialMaps[i];

          for (U32 j = 0; j < pMatList->mMaterialNames.size(); j++) {
             const char* pName     = pMatList->mMaterialNames[j];
             const U32 len = dStrlen(pName);
             if (len < 6)
                continue;

             const char* possible = pName + (len - 5);
             if (dStricmp(".blnd", possible) == 0) {
                char newName[256];
                AssertFatal(len < 200, "InteriorInstance::renewOverlays: Error, len exceeds allowed name length");

                dStrncpy(newName, pName, possible - pName);
                newName[possible - pName] = '\0';
                dStrcat(newName, ".");
                dStrcat(newName, baseName);

                TextureHandle test = TextureHandle(newName, MeshTexture, false);
                if (test.getGLName() != 0) {
                   pMatList->mMaterials[j] = test;
                } else {
                   pMatList->mMaterials[j] = TextureHandle(pName, MeshTexture, false);
                }
             }
          }
       }
    */
}


//--------------------------------------------------------------------------
void InteriorInstance::setSkinBase(const char* newBase)
{
    if (dStricmp(mSkinBase, newBase) == 0)
        return;

    mSkinBase = StringTable->insert(newBase);

    if (isServerObject())
        setMaskBits(SkinBaseMask);
    else
        renewOverlays();
}


//--------------------------------------------------------------------------
// from resManager.cc
extern U32 calculateCRC(void* buffer, S32 len, U32 crcVal);

bool InteriorInstance::onAdd()
{
    U32 i;

    // Load resource
    mInteriorRes = ResourceManager->load(mInteriorFileName, true);
    if (bool(mInteriorRes) == false) {
        Con::errorf(ConsoleLogEntry::General, "Unable to load interior: %s", mInteriorFileName);
        NetConnection::setLastError("Unable to load interior: %s", mInteriorFileName);
        return false;
    }

    // Check for materials

    bool foundAllMaterials = true;
    for (i = 0; i < mInteriorRes->getNumDetailLevels(); i++) {
        Interior* pInterior = mInteriorRes->getDetailLevel(i);
        HashTable<U32, bool> materialUsages;
        for (int j = 0; j < pInterior->mSurfaces.size(); j++)
        {
            Interior::Surface& surf = pInterior->mSurfaces[j];
            materialUsages.insertUnique(surf.textureIndex, true);
        }
        for (int j = 0; j < pInterior->mMaterialList->size(); j++)
        {
            if (materialUsages.find(j) == materialUsages.end()) continue;
            Material* mat = pInterior->mMaterialList->getMappedMaterial(j);
            if (mat != NULL)
            {
                //char errorBuff[4096];
                //errorBuff[0] = '\0';
                Vector<const char*> errorBuff;
                foundAllMaterials = foundAllMaterials && mat->preloadTextures(errorBuff);

                if (!errorBuff.empty())
                {
                    Con::errorf(ConsoleLogEntry::General, "Error preloading material(%s):", pInterior->mMaterialList->getMaterialName(j));
                    Con::errorf("{");
                    for (U32 k = 0; k < errorBuff.size(); k++)
                    {
                        Con::errorf("   missing file %s", errorBuff[k]);
                    }
                    Con::errorf("}");
                }

                //if (dStrlen(errorBuff) > 0)
                //    Con::errorf(ConsoleLogEntry::General, "Error preloading material(%s):\n{%s\n}", pInterior->mMaterialList->getMaterialName(j), errorBuff);
            }
        }
    }
    if (!foundAllMaterials) {
        Con::errorf(ConsoleLogEntry::General, "Unable to load interior due to missing materials: %s", mInteriorFileName);
        NetConnection::setLastError("Unable to load interior due to missing materials: %s", mInteriorFileName);
        return false;
    }



    if (!isClientObject())
        mCRC = mInteriorRes.getCRC();

    if (isClientObject() || gSPMode)
    {
        //if (mCRC != mInteriorRes.getCRC())
        //{
        //    NetConnection::setLastError("Local interior file '%s' does not match version on server.", mInteriorFileName);
        //    return false;
        //}
        for (i = 0; i < mInteriorRes->getNumDetailLevels(); i++) {
            // ok, if the material list load failed...
            // if this is a local connection, we'll assume that's ok
            // and just have white textures...
            // otherwise we want to return false.
            Interior* pInterior = mInteriorRes->getDetailLevel(i);
            if (!pInterior->prepForRendering(mInteriorRes.getFilePath()))
            {
                if (!bool(mServerObject))
                {
                    return false;
                }
            }
        }

        // copy planar reflect list from top detail level - for now
        Interior* pInterior = mInteriorRes->getDetailLevel(0);
        mReflectPlanes = pInterior->mReflectPlanes;
        if (mReflectPlanes.size())
        {
            // if any reflective planes, add interior to the reflective set of objects
            SimSet* reflectSet = dynamic_cast<SimSet*>(Sim::findObject("reflectiveSet"));
            reflectSet->addObject((SimObject*)this);
        }

    }

    if (!Parent::onAdd())
        return false;

    // Ok, everything's groovy!  Let's cache our hashed filename for renderimage sorting...
    mInteriorFileHash = _StringTable::hashString(mInteriorFileName);

    // Setup bounding information
    mObjBox = mInteriorRes->getDetailLevel(0)->getBoundingBox();
    resetWorldBox();
    setRenderTransform(mObjToWorld);


    // Do any handle loading, etc. required.

    //if (isClientObject() || gSPMode) {

        for (i = 0; i < mInteriorRes->getNumDetailLevels(); i++) {
            Interior* pInterior = mInteriorRes->getDetailLevel(i);

            // Force the lightmap manager to download textures if we're
            // running the mission editor.  Normally they are only
            // downloaded after the whole scene is lit.
            /*gInteriorLMManager.addInstance(pInterior->getLMHandle(), mLMHandle, this);
            if (gEditingMission) {
                gInteriorLMManager.useBaseTextures(pInterior->getLMHandle(), mLMHandle);
                gInteriorLMManager.downloadGLTextures(pInterior->getLMHandle());
            }*/

            // Install material list
            mMaterialMaps.push_back(new MaterialList(pInterior->mMaterialList));
        }

        // renewOverlays();
    //}
    //else {
    //
    //}

    addToScene();
    return true;
}


void InteriorInstance::onRemove()
{
    mConvexList->nukeList();

    if (isClientObject())
    {
        if (bool(mInteriorRes) && mLMHandle != 0xFFFFFFFF)
        {
            for (U32 i = 0; i < mInteriorRes->getNumDetailLevels(); i++)
            {
                Interior* pInterior = mInteriorRes->getDetailLevel(i);
                if (pInterior->getLMHandle() != 0xFFFFFFFF)
                    gInteriorLMManager.removeInstance(pInterior->getLMHandle(), mLMHandle);
            }
        }
    }

    removeFromScene();
    Parent::onRemove();
}


//--------------------------------------------------------------------------
bool InteriorInstance::onSceneAdd(SceneGraph* pGraph)
{
    AssertFatal(mInteriorRes, "Error, should not have been added to the scene if there's no interior!");

    if (Parent::onSceneAdd(pGraph) == false)
        return false;

    if (mInteriorRes->getDetailLevel(0)->mZones.size() > 1) {
        AssertWarn(getNumCurrZones() == 1 || gSPMode, "There should be one and only one zone for an interior that manages zones");
        mSceneManager->registerZones(this, (mInteriorRes->getDetailLevel(0)->mZones.size() - 1));
    }

    return true;
}


//--------------------------------------------------------------------------
void InteriorInstance::onSceneRemove()
{
    AssertFatal(mInteriorRes, "Error, should not have been added to the scene if there's no interior!");

    if (isManagingZones())
        mSceneManager->unregisterZones(this);

    Parent::onSceneRemove();
}


//--------------------------------------------------------------------------
bool InteriorInstance::getOverlappingZones(SceneObject* obj,
    U32* zones,
    U32* numZones)
{
    MatrixF xForm(true);
    Point3F invScale(1.0f / getScale().x,
        1.0f / getScale().y,
        1.0f / getScale().z);
    xForm.scale(invScale);
    xForm.mul(getWorldTransform());
    xForm.mul(obj->getTransform());
    xForm.scale(obj->getScale());

    U32 waterMark = FrameAllocator::getWaterMark();

    U16* zoneVector = (U16*)FrameAllocator::alloc(mInteriorRes->getDetailLevel(0)->mZones.size() * sizeof(U16));
    U32 numRetZones = 0;

    bool outsideToo = mInteriorRes->getDetailLevel(0)->scanZones(obj->getObjBox(),
        xForm,
        zoneVector,
        &numRetZones);
    if (numRetZones > SceneObject::MaxObjectZones) {
        Con::warnf(ConsoleLogEntry::General, "Too many zones returned for query on %s.  Returning first %d",
            mInteriorFileName, SceneObject::MaxObjectZones);
    }

    for (U32 i = 0; i < getMin(numRetZones, U32(SceneObject::MaxObjectZones)); i++)
        zones[i] = zoneVector[i] + mZoneRangeStart - 1;
    *numZones = numRetZones;

    FrameAllocator::setWaterMark(waterMark);

    return outsideToo;
}


//--------------------------------------------------------------------------
U32 InteriorInstance::getPointZone(const Point3F& p)
{
    AssertFatal(mInteriorRes, "Error, no interior!");

    Point3F osPoint = p;
    mWorldToObj.mulP(osPoint);
    osPoint.convolveInverse(mObjScale);

    S32 zone = mInteriorRes->getDetailLevel(0)->getZoneForPoint(osPoint);

    // If we're in solid (-1) or outside, we need to return 0
    if (zone == -1 || zone == 0)
        return 0;

    return (zone - 1) + mZoneRangeStart;
}

// does a hack check to determine how much a point is 'inside'.. should have
// portals prebuilt with the transfer energy to each other portal in the zone
// from the neighboring zone.. these values can be used to determine the factor
// from within an individual zone.. also, each zone could be marked with
// average material property for eax environment audio
// ~0: outside -> 1: inside
bool InteriorInstance::getPointInsideScale(const Point3F& pos, F32* pScale)
{
    AssertFatal(mInteriorRes, "InteriorInstance::getPointInsideScale: no interior");

    Interior* interior = mInteriorRes->getDetailLevel(0);

    Point3F p = pos;
    mWorldToObj.mulP(p);
    p.convolveInverse(mObjScale);

    U32 zoneIndex = interior->getZoneForPoint(p);
    if (zoneIndex == -1)  // solid?
    {
        *pScale = 1.f;
        return(true);
    }
    else if (zoneIndex == 0) // outside?
    {
        *pScale = 0.f;
        return(true);
    }

    U32 waterMark = FrameAllocator::getWaterMark();
    const Interior::Portal** portals = (const Interior::Portal**)FrameAllocator::alloc(256 * sizeof(const Interior::Portal*));
    U32 numPortals = 0;

    Interior::Zone& zone = interior->mZones[zoneIndex];

    U32 i;
    for (i = 0; i < zone.portalCount; i++)
    {
        const Interior::Portal& portal = interior->mPortals[interior->mZonePortalList[zone.portalStart + i]];
        if (portal.zoneBack == 0 || portal.zoneFront == 0) {
            AssertFatal(numPortals < 256, "Error, overflow in temporary portal buffer!");
            portals[numPortals++] = &portal;
        }
    }

    // inside?
    if (numPortals == 0)
    {
        *pScale = 1.f;

        FrameAllocator::setWaterMark(waterMark);
        return(true);
    }

    Point3F* portalCenters = (Point3F*)FrameAllocator::alloc(numPortals * sizeof(Point3F));
    U32 numPortalCenters = 0;

    // scale using the distances to the portals in this zone...
    for (i = 0; i < numPortals; i++)
    {
        const Interior::Portal* portal = portals[i];
        if (!portal->triFanCount)
            continue;

        Point3F center(0, 0, 0);
        for (U32 j = 0; j < portal->triFanCount; j++)
        {
            const Interior::TriFan& fan = interior->mWindingIndices[portal->triFanStart + j];
            U32 numPoints = fan.windingCount;

            if (!numPoints)
                continue;

            for (U32 k = 0; k < numPoints; k++)
            {
                const Point3F& a = interior->mPoints[interior->mWindings[fan.windingStart + k]].point;
                center += a;
            }

            center /= numPoints;
            portalCenters[numPortalCenters++] = center;
        }
    }

    // 'magic' check here...
    F32 magic = Con::getFloatVariable("Interior::insideDistanceFalloff", 10.f);

    F32 val = 0.f;
    for (i = 0; i < numPortalCenters; i++)
        val += 1.f - mClampF(Point3F(portalCenters[i] - p).len() / magic, 0.f, 1.f);

    *pScale = 1.f - mClampF(val, 0.f, 1.f);

    FrameAllocator::setWaterMark(waterMark);
    return(true);
}

//--------------------------------------------------------------------------
// renderObject - this function is called pretty much only for debug rendering
//--------------------------------------------------------------------------
void InteriorInstance::renderObject(SceneState* state, RenderInst* ri)
{
    if (gEditingMission && isHidden())
        return;


    GFX->pushWorldMatrix();


    // setup world matrix - for fixed function
    MatrixF world = GFX->getWorldMatrix();
    world.mul(getRenderTransform());
    world.scale(getScale());
    GFX->setWorldMatrix(world);

    // setup world matrix - for shaders
    MatrixF proj = GFX->getProjectionMatrix();
    proj.mul(world);
    proj.transpose();
    GFX->setVertexShaderConstF(0, (float*)&proj, 4);


    PROFILE_START(IRO_DebugRender);

#ifdef TORQUE_DEBUG   
    Interior* pInterior = mInteriorRes->getDetailLevel(0);
    SceneGraphData sgData;
    sgData = pInterior->setupSceneGraphInfo((InteriorInstance*)ri->obj, state);
    ZoneVisDeterminer zoneVis = pInterior->setupZoneVis((InteriorInstance*)ri->obj, state);
    pInterior->debugRender(zoneVis, sgData, (InteriorInstance*)ri->obj);
#endif

    PROFILE_END();


    GFX->popWorldMatrix();
}


//--------------------------------------------------------------------------
bool InteriorInstance::scopeObject(const Point3F& rootPosition,
    const F32             /*rootDistance*/,
    bool* zoneScopeState)
{
    AssertFatal(isManagingZones(), "Error, should be a zone manager if we are called on to scope the scene!");
    if (bool(mInteriorRes) == false)
        return false;

    Interior* pInterior = getDetailLevel(0);
    AssertFatal(pInterior->mZones.size() <= csgMaxZoneSize, "Error, too many zones!  Increase max");
    bool* pInteriorScopingState = sgScopeBoolArray;
    dMemset(pInteriorScopingState, 0, sizeof(bool) * pInterior->mZones.size());

    // First, let's transform the point into the interior's space
    Point3F interiorRoot = rootPosition;
    getWorldTransform().mulP(interiorRoot);
    interiorRoot.convolveInverse(getScale());

    S32 realStartZone = getPointZone(rootPosition);
    if (realStartZone != 0)
        realStartZone = realStartZone - mZoneRangeStart + 1;

    bool continueOut = pInterior->scopeZones(realStartZone,
        interiorRoot,
        pInteriorScopingState);

    // Copy pInteriorScopingState to zoneScopeState
    for (S32 i = 1; i < pInterior->mZones.size(); i++)
        zoneScopeState[i + mZoneRangeStart - 1] = pInteriorScopingState[i];

    return continueOut;
}


//--------------------------------------------------------------------------
U32 InteriorInstance::calcDetailLevel(SceneState* state, const Point3F& wsPoint)
{
    AssertFatal(mInteriorRes, "Error, should not try to calculate the deatil level without a resource to work with!");
    AssertFatal(getNumCurrZones() > 0, "Error, must belong to a zone for this to work");

    if (smDetailModification < 0.3)
        smDetailModification = 0.3;
    if (smDetailModification > 1.0)
        smDetailModification = 1.0;

    // Early out for simple interiors
    if (mInteriorRes->getNumDetailLevels() == 1)
        return 0;

    if ((mForcedDetailLevel >= 0) && (mForcedDetailLevel < mInteriorRes->getNumDetailLevels()))
        return(mForcedDetailLevel);

    Point3F osPoint = wsPoint;
    mRenderWorldToObj.mulP(osPoint);
    osPoint.convolveInverse(mObjScale);

    // First, see if the point is in the object space bounding box of the highest detail
    //  If it is, then the detail level is zero.
    if (mObjBox.isContained(osPoint))
        return 0;

    // Otherwise, we're going to have to do some ugly trickery to get the projection.
    //  I've stolen the worldToScreenScale from dglMatrix, we'll have to calculate the
    //  projection of the bounding sphere of the lowest detail level.
    //  worldToScreenScale = (near * view.extent.x) / (right - left)
    RectI viewport;
    F64   frustum[4] = { 1e10, -1e10, 1e10, -1e10 };

    bool init = false;
    SceneObjectRef* pWalk = mZoneRefHead;
    AssertFatal(pWalk != NULL, "Error, object must exist in at least one zone to call this!");
    while (pWalk) {
        const SceneState::ZoneState& rState = state->getZoneState(pWalk->zone);
        if (rState.render == true) {
            // frustum
            if (rState.frustum[0] < frustum[0]) frustum[0] = rState.frustum[0];
            if (rState.frustum[1] > frustum[1]) frustum[1] = rState.frustum[1];
            if (rState.frustum[2] < frustum[2]) frustum[2] = rState.frustum[2];
            if (rState.frustum[3] > frustum[3]) frustum[3] = rState.frustum[3];

            // viewport
            if (init == false)
                viewport = rState.viewport;
            else
                viewport.unionRects(rState.viewport);

            init = true;
        }
        pWalk = pWalk->nextInObj;
    }
    AssertFatal(init, "Error, at least one zone must be rendered here!");

    F32 worldToScreenScale = (state->getNearPlane() * viewport.extent.x) / (frustum[1] - frustum[0]);
    const SphereF& lowSphere = mInteriorRes->getDetailLevel(mInteriorRes->getNumDetailLevels() - 1)->mBoundingSphere;
    F32 dist = (lowSphere.center - osPoint).len();
    F32 projRadius = (lowSphere.radius / dist) * worldToScreenScale;

    // Scale the projRadius based on the objects maximum scale axis
    projRadius *= getMax(mFabs(mObjScale.x), getMax(mFabs(mObjScale.y), mFabs(mObjScale.z)));

    // Multiply based on detail preference...
    projRadius *= smDetailModification;

    // Ok, now we have the projected radius, we need to search through the interiors to
    //  find the largest interior that will support this projection.
    U32 final = mInteriorRes->getNumDetailLevels() - 1;
    for (U32 i = 0; i < mInteriorRes->getNumDetailLevels() - 1; i++) {
        Interior* pDetail = mInteriorRes->getDetailLevel(i);

        if (pDetail->mMinPixels < projRadius) {
            final = i;
            break;
        }
    }

    // Ok, that's it.
    return final;
}


//--------------------------------------------------------------------------
bool InteriorInstance::prepRenderImage(SceneState* state, const U32 stateKey,
    const U32 startZone, const bool modifyBaseState)
{
    if (isLastState(state, stateKey))
        return false;

    if (gEditingMission && isHidden())
        return false;

    PROFILE_START(InteriorPrepRenderImage);

    setLastState(state, stateKey);

    U32 realStartZone;
    if (startZone != 0xFFFFFFFF) {
        AssertFatal(startZone != 0, "Hm.  This really shouldn't happen.  Should only get inside zones here");
        AssertFatal(isManagingZones(), "Must be managing zones if we're here...");

        realStartZone = startZone - mZoneRangeStart + 1;
    }
    else {
        realStartZone = getPointZone(state->getCameraPosition());
        if (realStartZone != 0)
            realStartZone = realStartZone - mZoneRangeStart + 1;
    }

    bool render = true;
    if (modifyBaseState == false) {
        // Regular query.  We only return a render zone if our parent zone is rendered.
        //  Otherwise, we always render
        if (state->isObjectRendered(this) == false)
        {
            PROFILE_END();
            return false;
        }
    }
    else {
        if (mShowTerrainInside == true)
            state->enableTerrainOverride();
    }

    U32 detailLevel = 0;
    if (startZone == 0xFFFFFFFF)
        detailLevel = calcDetailLevel(state, state->getCameraPosition());


    U32 baseZoneForPrep = getCurrZone(0);
    bool multipleZones = false;
    if (getNumCurrZones() > 1) {
        U32 numRenderedZones = 0;
        baseZoneForPrep = 0xFFFFFFFF;
        for (U32 i = 0; i < getNumCurrZones(); i++) {
            if (state->getZoneState(getCurrZone(i)).render == true) {
                numRenderedZones++;
                if (baseZoneForPrep == 0xFFFFFFFF)
                    baseZoneForPrep = getCurrZone(i);
            }
        }

        if (numRenderedZones > 1)
            multipleZones = true;
    }

    bool continueOut = mInteriorRes->getDetailLevel(0)->prepRender(state,
        baseZoneForPrep,
        realStartZone, mZoneRangeStart,
        mRenderObjToWorld, mObjScale,
        modifyBaseState & !smDontRestrictOutside,
        smDontRestrictOutside | multipleZones,
        state->mFlipCull);
    if (smDontRestrictOutside)
        continueOut = true;


    // need to delay the batching because zone information is not complete until
    // the entire scene tree is built.
    SceneState::InteriorListElem elem;
    elem.obj = this;
    elem.stateKey = stateKey;
    elem.startZone = 0xFFFFFFFF;
    elem.detailLevel = detailLevel;
    elem.worldXform = gRenderInstManager.allocXform();
    *elem.worldXform = GFX->getWorldMatrix();

    state->insertInterior(elem);


    PROFILE_END();
    return continueOut;
}


//--------------------------------------------------------------------------
bool InteriorInstance::castRay(const Point3F& s, const Point3F& e, RayInfo* info)
{
    info->object = this;
    return mInteriorRes->getDetailLevel(0)->castRay(s, e, info);
}

//------------------------------------------------------------------------------
void InteriorInstance::setTransform(const MatrixF& mat)
{
    Parent::setTransform(mat);

    // Since the interior is a static object, it's render transform changes 1 to 1
    //  with it's collision transform
    setRenderTransform(mat);

    if (isServerObject())
        setMaskBits(TransformMask);
}


//------------------------------------------------------------------------------
bool InteriorInstance::buildPolyList(AbstractPolyList* list, const Box3F& wsBox, const SphereF&)
{
    if (bool(mInteriorRes) == false)
        return false;

    // Setup collision state data
    list->setTransform(&getTransform(), getScale());
    list->setObject(this);

    return mInteriorRes->getDetailLevel(0)->buildPolyList(list, wsBox, mWorldToObj, getScale());
}



void InteriorInstance::buildConvex(const Box3F& box, Convex* convex)
{
    if (bool(mInteriorRes) == false)
        return;

    mConvexList->collectGarbage();

    Box3F realBox = box;
    mWorldToObj.mul(realBox);
    realBox.min.convolveInverse(mObjScale);
    realBox.max.convolveInverse(mObjScale);

    if (realBox.isOverlapped(getObjBox()) == false)
        return;

    U32 waterMark = FrameAllocator::getWaterMark();

    if ((convex->getObject()->getType() & VehicleObjectType) &&
        mInteriorRes->getDetailLevel(0)->mVehicleConvexHulls.size() > 0)
    {
        // Can never have more hulls than there are hulls in the interior...
        U16* hulls = (U16*)FrameAllocator::alloc(mInteriorRes->getDetailLevel(0)->mVehicleConvexHulls.size() * sizeof(U16));
        U32 numHulls = 0;

        Interior* pInterior = mInteriorRes->getDetailLevel(0);
        if (pInterior->getIntersectingVehicleHulls(realBox, hulls, &numHulls) == false) {
            FrameAllocator::setWaterMark(waterMark);
            return;
        }

        for (U32 i = 0; i < numHulls; i++) {
            // See if this hull exists in the working set already...
            Convex* cc = 0;
            CollisionWorkingList& wl = convex->getWorkingList();
            for (CollisionWorkingList* itr = wl.wLink.mNext; itr != &wl; itr = itr->wLink.mNext) {
                if (itr->mConvex->getType() == InteriorConvexType &&
                    (static_cast<InteriorConvex*>(itr->mConvex)->getObject() == this &&
                        static_cast<InteriorConvex*>(itr->mConvex)->hullId == -S32(hulls[i] + 1))) {
                    cc = itr->mConvex;
                    break;
                }
            }
            if (cc)
                continue;

            // Create a new convex.
            InteriorConvex* cp = new InteriorConvex;
            mConvexList->registerObject(cp);
            convex->addToWorkingList(cp);
            cp->mObject = this;
            cp->pInterior = pInterior;
            cp->hullId = -S32(hulls[i] + 1);
            cp->box.min.x = pInterior->mVehicleConvexHulls[hulls[i]].minX;
            cp->box.min.y = pInterior->mVehicleConvexHulls[hulls[i]].minY;
            cp->box.min.z = pInterior->mVehicleConvexHulls[hulls[i]].minZ;
            cp->box.max.x = pInterior->mVehicleConvexHulls[hulls[i]].maxX;
            cp->box.max.y = pInterior->mVehicleConvexHulls[hulls[i]].maxY;
            cp->box.max.z = pInterior->mVehicleConvexHulls[hulls[i]].maxZ;
        }
    }
    else
    {
        // Can never have more hulls than there are hulls in the interior...
        U16* hulls = (U16*)FrameAllocator::alloc(mInteriorRes->getDetailLevel(0)->mConvexHulls.size() * sizeof(U16));
        U32 numHulls = 0;

        Interior* pInterior = mInteriorRes->getDetailLevel(0);
        if (pInterior->getIntersectingHulls(realBox, hulls, &numHulls) == false) {
            FrameAllocator::setWaterMark(waterMark);
            return;
        }

        for (U32 i = 0; i < numHulls; i++) {
            // See if this hull exists in the working set already...
            Convex* cc = 0;
            CollisionWorkingList& wl = convex->getWorkingList();
            for (CollisionWorkingList* itr = wl.wLink.mNext; itr != &wl; itr = itr->wLink.mNext) {
                if (itr->mConvex->getType() == InteriorConvexType &&
                    (static_cast<InteriorConvex*>(itr->mConvex)->getObject() == this &&
                        static_cast<InteriorConvex*>(itr->mConvex)->hullId == hulls[i])) {
                    cc = itr->mConvex;
                    break;
                }
            }
            if (cc)
                continue;

            // Create a new convex.
            InteriorConvex* cp = new InteriorConvex;
            mConvexList->registerObject(cp);
            convex->addToWorkingList(cp);
            cp->mObject = this;
            cp->pInterior = pInterior;
            cp->hullId = hulls[i];
            cp->box.min.x = pInterior->mConvexHulls[hulls[i]].minX;
            cp->box.min.y = pInterior->mConvexHulls[hulls[i]].minY;
            cp->box.min.z = pInterior->mConvexHulls[hulls[i]].minZ;
            cp->box.max.x = pInterior->mConvexHulls[hulls[i]].maxX;
            cp->box.max.y = pInterior->mConvexHulls[hulls[i]].maxY;
            cp->box.max.z = pInterior->mConvexHulls[hulls[i]].maxZ;
        }
    }
    FrameAllocator::setWaterMark(waterMark);
}


//------------------------------------------------------------------------------
U32 InteriorInstance::packUpdate(NetConnection* c, U32 mask, BitStream* stream)
{
    U32 retMask = Parent::packUpdate(c, mask, stream);

    if (stream->writeFlag((mask & InitMask) != 0)) {
        // Initial update, write the whole kit and kaboodle
        stream->write(mCRC);

        stream->writeString(mInteriorFileName);
        stream->writeFlag(mShowTerrainInside);

        // Write the transform (do _not_ use writeAffineTransform.  Since this is a static
        //  object, the transform must be RIGHT THE *&)*$&^ ON or it will goof up the
        //  syncronization between the client and the server.
        mathWrite(*stream, mObjToWorld);
        mathWrite(*stream, mObjScale);

        // Write the alarm state
        stream->writeFlag(mAlarmState);

        // Write the skinbase
        stream->writeString(mSkinBase);

        // audio profile
        if (stream->writeFlag(mAudioProfile))
            stream->writeRangedU32(mAudioProfile->getId(), DataBlockObjectIdFirst, DataBlockObjectIdLast);

        // audio environment:
        if (stream->writeFlag(mAudioEnvironment))
            stream->writeRangedU32(mAudioEnvironment->getId(), DataBlockObjectIdFirst, DataBlockObjectIdLast);

    }
    else
    {
        if (stream->writeFlag((mask & TransformMask) != 0)) {
            mathWrite(*stream, mObjToWorld);
            mathWrite(*stream, mObjScale);
        }

        stream->writeFlag(mAlarmState);


        if (stream->writeFlag(mask & SkinBaseMask))
            stream->writeString(mSkinBase);

        // audio update:
        if (stream->writeFlag(mask & AudioMask))
        {
            // profile:
            if (stream->writeFlag(mAudioProfile))
                stream->writeRangedU32(mAudioProfile->getId(), DataBlockObjectIdFirst, DataBlockObjectIdLast);

            // environment:
            if (stream->writeFlag(mAudioEnvironment))
                stream->writeRangedU32(mAudioEnvironment->getId(), DataBlockObjectIdFirst, DataBlockObjectIdLast);
        }
    }

    return retMask;
}


//------------------------------------------------------------------------------
void InteriorInstance::unpackUpdate(NetConnection* c, BitStream* stream)
{
    Parent::unpackUpdate(c, stream);

    MatrixF temp;
    Point3F tempScale;

    if (stream->readFlag()) {
        // Initial Update
        // CRC
        stream->read(&mCRC);

        // File
        mInteriorFileName = stream->readSTString();

        // Terrain flag
        mShowTerrainInside = stream->readFlag();

        // Transform
        mathRead(*stream, &temp);
        mathRead(*stream, &tempScale);
        setScale(tempScale);
        setTransform(temp);

        // Alarm state: Note that we handle this ourselves on the initial update
        //  so that the state is always full on or full off...
        mAlarmState = stream->readFlag();

        mSkinBase = stream->readSTString();

        // audio profile:
        if (stream->readFlag())
        {
            U32 profileId = stream->readRangedU32(DataBlockObjectIdFirst, DataBlockObjectIdLast);
            mAudioProfile = dynamic_cast<SFXProfile*>(Sim::findObject(profileId));
        }
        else
            mAudioProfile = 0;

        // audio environment:
        if (stream->readFlag())
        {
            U32 profileId = stream->readRangedU32(DataBlockObjectIdFirst, DataBlockObjectIdLast);
            mAudioEnvironment = dynamic_cast<SFXEnvironment*>(Sim::findObject(profileId));
        }
        else
            mAudioEnvironment = 0;
    }
    else
    {
        // Normal update
        if (stream->readFlag()) {
            mathRead(*stream, &temp);
            mathRead(*stream, &tempScale);
            setScale(tempScale);
            setTransform(temp);
        }

        setAlarmMode(stream->readFlag());


        if (stream->readFlag()) {
            mSkinBase = stream->readSTString();
            renewOverlays();
        }

        // audio update:
        if (stream->readFlag())
        {
            // profile:
            if (stream->readFlag())
            {
                U32 profileId = stream->readRangedU32(DataBlockObjectIdFirst, DataBlockObjectIdLast);
                mAudioProfile = dynamic_cast<SFXProfile*>(Sim::findObject(profileId));
            }
            else
                mAudioProfile = 0;

            // environment:
            if (stream->readFlag())
            {
                U32 profileId = stream->readRangedU32(DataBlockObjectIdFirst, DataBlockObjectIdLast);
                mAudioEnvironment = dynamic_cast<SFXEnvironment*>(Sim::findObject(profileId));
            }
            else
                mAudioEnvironment = 0;
        }
    }
}


//------------------------------------------------------------------------------
Interior* InteriorInstance::getDetailLevel(const U32 level)
{
    return mInteriorRes->getDetailLevel(level);
}

U32 InteriorInstance::getNumDetailLevels()
{
    return mInteriorRes->getNumDetailLevels();
}

//--------------------------------------------------------------------------
//-------------------------------------- Alarm functionality
//
void InteriorInstance::setAlarmMode(const bool alarm)
{
    if (mInteriorRes->getDetailLevel(0)->mHasAlarmState == false)
        return;

    if (mAlarmState == alarm)
        return;

    mAlarmState = alarm;
    if (isServerObject())
    {
        setMaskBits(AlarmMask);
    }
    else
    {
        // DMMTODO: Invalidate current light state
    }
}



void InteriorInstance::createTriggerTransform(const InteriorResTrigger* trigger, MatrixF* transform)
{
    Point3F offset;
    MatrixF xform = getTransform();
    xform.getColumn(3, &offset);

    Point3F triggerOffset = trigger->mOffset;
    triggerOffset.convolve(mObjScale);
    getTransform().mulV(triggerOffset);
    offset += triggerOffset;
    xform.setColumn(3, offset);

    *transform = xform;
}


bool InteriorInstance::readLightmaps(GBitmap**** lightmaps)
{
    AssertFatal(mInteriorRes, "Error, no interior loaded!");
    AssertFatal(lightmaps, "Error, no lightmaps or numdetails result pointers");
    AssertFatal(*lightmaps == NULL, "Error, already have a pointer in the lightmaps result field!");

    // Load resource
    Stream* pStream = ResourceManager->openStream(mInteriorFileName);
    if (pStream == NULL) {
        Con::errorf(ConsoleLogEntry::General, "Unable to load interior: %s", mInteriorFileName);
        return false;
    }

    InteriorResource* pResource = new InteriorResource;
    bool success = pResource->read(*pStream);
    ResourceManager->closeStream(pStream);

    if (success == false)
    {
        delete pResource;
        return false;
    }
    AssertFatal(pResource->getNumDetailLevels() == mInteriorRes->getNumDetailLevels(),
        "Mismatched detail levels!");

    *lightmaps = new GBitmap * *[mInteriorRes->getNumDetailLevels()];

    for (U32 i = 0; i < pResource->getNumDetailLevels(); i++)
    {
        Interior* pInterior = pResource->getDetailLevel(i);
        (*lightmaps)[i] = new GBitmap * [pInterior->mLightmaps.size()];
        for (U32 j = 0; j < pInterior->mLightmaps.size(); j++)
        {
            ((*lightmaps)[i])[j] = pInterior->mLightmaps[j];
            pInterior->mLightmaps[j] = NULL;
        }
        pInterior->mLightmaps.clear();
    }

    delete pResource;
    return true;
}

//--------------------------------------------------------------------------
// This is a callback for objects that have reflections and are added to
// the "reflectiveSet" SimSet.
//--------------------------------------------------------------------------
void InteriorInstance::updateReflection()
{
    static MatrixF rotMat(EulerF((M_PI / 2.0), 0.0, 0.0));
    static MatrixF invRotMat(EulerF(-(M_PI / 2.0), 0.0, 0.0));

    // grab camera transform from tsCtrl
    GuiTSCtrl* tsCtrl;
    tsCtrl = dynamic_cast<GuiTSCtrl*>(Sim::findObject("PlayGui"));
    CameraQuery query;
    if (!tsCtrl->processCameraQuery(&query)) return;

    // store current matrices
    GFX->pushWorldMatrix();
    MatrixF proj = GFX->getProjectionMatrix();


    // set up projection - must match that of main camera
    Point2I resolution = getCurrentClientSceneGraph()->getDisplayTargetResolution();
    GFX->setFrustum(mRadToDeg(query.fov),
        F32(resolution.x) / F32(resolution.y),
        query.nearPlane, query.farPlane);


    // set up camera transform relative to interior
    MatrixF invObjTrans = getRenderTransform();
    invObjTrans.inverse();
    MatrixF relCamTrans = invObjTrans * query.cameraMatrix;


    getCurrentClientSceneGraph()->setReflectPass(true);

    GFXTextureTargetRef myTarg = GFX->allocRenderToTextureTarget();

    for (U32 i = 0; i < mReflectPlanes.size(); i++)
    {
        ReflectPlane& rf = mReflectPlanes[i];

        // set up reflect texture
        if (!rf.getTex())
        {
            rf.setupTex(512);
        }

        MatrixF camReflectTrans = rf.getCameraReflection(relCamTrans);
        MatrixF camTrans = getRenderTransform() * camReflectTrans;
        camTrans.inverse();

        GFX->setWorldMatrix(camTrans);

        // use relative reflect transform for modelview since clip plane is in interior space
        camTrans = camReflectTrans;
        camTrans.inverse();

        // set new projection matrix
        getCurrentClientSceneGraph()->setNonClipProjection((MatrixF&)GFX->getProjectionMatrix());
        MatrixF clipProj = rf.getFrustumClipProj(camTrans);
        GFX->setProjectionMatrix(clipProj);

        // render a frame
        GFX->pushActiveRenderTarget();
        myTarg->attachTexture(GFXTextureTarget::Color0, rf.getTex() );
        myTarg->attachTexture(GFXTextureTarget::DepthStencil, rf.getDepth() );
        GFX->setActiveRenderTarget(myTarg);
        GFX->setZEnable(true);
        GFX->clear(GFXClearZBuffer | GFXClearStencil | GFXClearTarget, ColorI(0, 0, 0), 1.0f, 0);

        U32 objTypeFlag = -1;
        objTypeFlag ^= TerrainObjectType | StaticObjectType | StaticRenderedObjectType;

        getCurrentClientSceneGraph()->renderScene(objTypeFlag);

        GFX->popActiveRenderTarget();
    }


    // cleanup

    // clear zbuffer
    GFX->clear(GFXClearZBuffer | GFXClearStencil, ColorI(255, 0, 255), 1.0f, 0);

    getCurrentClientSceneGraph()->setReflectPass(false);
    GFX->popWorldMatrix();
    GFX->setProjectionMatrix(proj);



}

S32 InteriorInstance::getSurfaceZone(U32 surfaceindex, Interior* detail)
{
    AssertFatal(((surfaceindex >= 0) && (surfaceindex < detail->surfaceZones.size())), "Bad surface index!");
    S32 zone = detail->surfaceZones[surfaceindex];
    if (zone > -1)
        return zone + mZoneRangeStart;
    return getCurrZone(0);
}

Material* InteriorInstance::getMaterial(U32 material)
{
    if (mMaterialMaps.size() && mMaterialMaps[0] != NULL)
    {
        return mMaterialMaps[0]->getMappedMaterial(material);
    }

    return NULL;
}

void InteriorInstance::addChildren()
{
    if(mInteriorRes)
    {
        Vector<U32> usedTriggerIds;

        SimGroup* group = this->getGroup();
        Con::errorf("There are %d game entities", mInteriorRes->getNumGameEntities());

        for (U32 i = 0; i < mInteriorRes->getNumGameEntities(); i++)
        {
            ItrGameEntity* entity = mInteriorRes->getGameEntity(i);
            ConsoleObject* conObj = ConsoleObject::create(entity->mGameClass);
            SceneObject* obj = dynamic_cast<SceneObject*>(conObj);
            if (obj)
            {
                GameBase* gbObj = dynamic_cast<GameBase*>(conObj);
                if (gbObj)
                    gbObj->setField("dataBlock", entity->mDataBlock);

                const char* name = StringTable->insert("");

                obj->setModStaticFields(true);
                EulerF angles(0.0f, 0.0f, 0.0f);
                for (auto& entry : entity->mDictionary)
                {
                    if (dStricmp(entry.name, "angles") == 0)
                    {
                        dSscanf(entry.value, "%g %g %g", &angles.x, &angles.y, &angles.z);
                    }
                    else
                    {
                        if (dStricmp(entry.name, "name") == 0)
                            name = entry.value;
                        obj->setDataField(StringTable->insert(entry.name), nullptr, entry.value);
                    }
                }
                obj->setModStaticFields(false);

                Point3F origin = entity->mPos;
                origin *= this->mObjScale;

                angles.x = mDegToRad(angles.x);
                angles.y = mDegToRad(angles.y);
                angles.z = mDegToRad(angles.z);

                MatrixF rotMat(true);
                rotMat.setEulerFromTrenchbroom(angles);

                MatrixF trans = this->getTransform();
                trans.mulP(origin);

                MatrixF xform(true);
                xform.mul(rotMat);
                xform.setPosition(origin);

                obj->setTransform(xform);

                bool success = obj->registerObject();
                if (success)
                {
                    Con::errorf("Created entity: %s: %s", entity->mGameClass, entity->mDataBlock);

                    bool found = false;
                    if (dStricmp(entity->mDataBlock, "checkPointShape") == 0 && dStrlen(name) > 0)
                    {
                        for (U32 j = 0; j < mInteriorRes->getNumTriggers(); j++)
                        {
                            if (std::find(usedTriggerIds.begin(), usedTriggerIds.end(), j) != usedTriggerIds.end())
                                continue;

                            InteriorResTrigger* resTrigger = mInteriorRes->getTrigger(j);

                            if (dStricmp(resTrigger->mDataBlock, "checkPointTrigger") != 0)
                                continue;

                            if (dStricmp(resTrigger->mName, name) != 0)
                                continue;

                            Trigger* trigger = new Trigger();
                            trigger->setField("dataBlock", resTrigger->mDataBlock);

                            for (auto& entry : resTrigger->mDictionary)
                            {
                                trigger->setDataField(StringTable->insert(entry.name), nullptr, entry.value);
                            }

                            MatrixF newXForm;
                            this->createTriggerTransform(resTrigger, &newXForm);
                            trigger->setTriggerPolyhedron(resTrigger->mPolyhedron);
                            trigger->setScale(this->mObjScale);
                            trigger->setTransform(newXForm);
                            bool success2 = trigger->registerObject();
                            if (success2)
                            {
                                Con::errorf("Created checkpoint trigger: %s", name);
                                SimGroup* checkpointGroup = new SimGroup();
                                checkpointGroup->registerObject();
                                group->addObject(checkpointGroup);
                                checkpointGroup->assignName(name);
                                checkpointGroup->addObject(obj);
                                checkpointGroup->addObject(trigger);
                                found = true;
                                usedTriggerIds.push_back(j);
                            } else {
                                Con::errorf("Failed to register checkpoint trigger: %s", name);
                                delete trigger;
                            }
                        }
                    }

                    if (!found)
                    {
                        group->addObject(obj);
                    }
                } else {
                    Con::errorf("Failed to register entity: %s: %s", entity->mGameClass, entity->mDataBlock);
                    delete obj;
                }
            } else {
                Con::errorf("Invalid game class for entity: %s", entity->mGameClass);
                delete conObj;
            }
        }
        this->addDoors(false, usedTriggerIds);
        this->addTriggers(usedTriggerIds);
    }
}

void InteriorInstance::addDoors(bool hide, Vector<U32>& usedTriggerIds)
{
    for (U32 i = 0; i < mInteriorRes->getNumInteriorPathFollowers(); i++)
    {
        InteriorPathFollower* follower = mInteriorRes->getInteriorPathFollower(i);
        PathedInterior* pi = new PathedInterior();
        pi->mName = StringTable->insert(follower->mName);
        pi->mInteriorResIndex = follower->mInteriorResIndex;
        pi->mPathIndex = follower->mPathIndex;
        pi->mOffset = follower->mOffset;
        pi->mInteriorResName = this->mInteriorFileName;
        pi->setField("dataBlock", follower->mDataBlock);

        for (auto& entry : follower->mDictionary)
        {
            pi->setDataField(entry.name, nullptr, entry.value);
        }

        SimGroup* doorGroup = new SimGroup();

        char nameBuffer[256];
        dSprintf(nameBuffer, 255, "%s_g", pi->mName);
        doorGroup->registerObject();
        this->getGroup()->addObject(doorGroup);
        doorGroup->assignName(nameBuffer);

        Path* path = new Path();
        path->registerObject();
        doorGroup->addObject(path);
        U32 pathId = gServerPathManager->allocatePathId();
        path->mPathIndex = pathId;
        for (U32 k = 0; k < follower->mWayPoints.size(); k++)
        {
            Marker* marker = new Marker();

            marker->mSeqNum = k;
            marker->mMSToNext = follower->mWayPoints[k].msToNext;
            marker->mSmoothingType = follower->mWayPoints[k].smoothingType;

            marker->registerObject();
            path->addObject(marker);

            Point3F markerPos = follower->mWayPoints[k].pos;
            markerPos *= this->mObjScale;

            MatrixF transform = this->getTransform();
            transform.mulP(markerPos);

            MatrixF newXForm(true);
            newXForm.setPosition(markerPos);

            marker->setTransform(newXForm);
        }

        for (U32 k = 0; k < follower->mTriggerIds.size(); k++)
        {
            U32 triggerId = follower->mTriggerIds[k];
            if (std::find(usedTriggerIds.begin(), usedTriggerIds.end(), triggerId) != usedTriggerIds.end())
                continue;

            InteriorResTrigger* resTrigger = mInteriorRes->getTrigger(triggerId);

            if (dStricmp(resTrigger->mDataBlock, "TriggerGotoTarget") != 0)
                continue;
            usedTriggerIds.push_back(triggerId);

            Trigger* trigger = new Trigger();
            trigger->setField("dataBlock", resTrigger->mDataBlock);

            for (auto& entry : resTrigger->mDictionary)
            {
                trigger->setDataField(StringTable->insert(entry.name), nullptr, entry.value);
            }

            MatrixF newXForm;
            this->createTriggerTransform(resTrigger, &newXForm);
            trigger->setTriggerPolyhedron(resTrigger->mPolyhedron);
            trigger->setScale(this->mObjScale);
            trigger->setTransform(newXForm);
            trigger->registerObject();
            doorGroup->addObject(trigger, resTrigger->mName);
        }

        MatrixF piTransform(true);
        Point3F piOffset = pi->mOffset;
        piOffset *= this->mObjScale;

        MatrixF trans = this->getTransform();
        trans.mulP(piOffset);

        piOffset = -piOffset;
        piTransform.setPosition(piOffset);

        MatrixF trans2 = this->getTransform();
        piTransform.mul(trans2);

        pi->mBaseTransform = piTransform;
        pi->mBaseScale = this->mObjScale;

        doorGroup->addObject(pi, pi->mName);

        if (hide)
            doorGroup->setHidden(hide);

        if (!pi->registerObject())
        {
            Con::warnf("Warning, could not register door.  Door skipped!");
            delete pi;
        }
    }
}

void InteriorInstance::addTriggers(Vector<U32>& usedTriggerIds)
{
    SimGroup* group = this->getGroup();
    Con::errorf("There are %d triggers", mInteriorRes->getNumTriggers() - usedTriggerIds.size());

    for (U32 i = 0; i < mInteriorRes->getNumTriggers(); i++)
    {
        if (std::find(usedTriggerIds.begin(), usedTriggerIds.end(), i) != usedTriggerIds.end())
            continue;
        usedTriggerIds.push_back(i);
        InteriorResTrigger* resTrigger = mInteriorRes->getTrigger(i);

        Trigger* trigger = new Trigger();
        trigger->setField("dataBlock", resTrigger->mDataBlock);

        for (auto& entry : resTrigger->mDictionary)
        {
            trigger->setDataField(StringTable->insert(entry.name), nullptr, entry.value);
        }

        MatrixF newXForm;
        this->createTriggerTransform(resTrigger, &newXForm);
        trigger->setTriggerPolyhedron(resTrigger->mPolyhedron);
        trigger->setScale(this->mObjScale);
        trigger->setTransform(newXForm);
        bool success = trigger->registerObject();
        if (success)
        {
            Con::errorf("Created trigger: %s", resTrigger->mDataBlock);
            group->addObject(trigger);
        } else {
            Con::errorf("Failed to register trigger: %s", resTrigger->mDataBlock);
            delete trigger;
        }
    }
}
