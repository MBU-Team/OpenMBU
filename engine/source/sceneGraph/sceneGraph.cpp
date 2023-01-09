//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "sceneGraph/sceneGraph.h"
#include "sim/sceneObject.h"
#include "sceneGraph/sceneRoot.h"
#include "sceneGraph/sceneState.h"
#include "sim/netConnection.h"
#include "terrain/environment/sky.h"
#ifdef TORQUE_TERRAIN
#include "terrain/terrData.h"
#endif
#include "sim/decalManager.h"
#include "sceneGraph/detailManager.h"
#include "core/fileStream.h"
#include "platform/profiler.h"
#include "renderInstance/renderInstMgr.h"

#include "gfx/gfxDevice.h"
#include "gfx/debugDraw.h"

const U32 SceneGraph::csmMaxTraversalDepth = 4;
U32 SceneGraph::smStateKey = 0;
SceneGraph* gClientSceneGraph = NULL;
SceneGraph* gServerSceneGraph = NULL;
SceneGraph* gSPModeSceneGraph = NULL;
const U32 SceneGraph::csmRefPoolBlockSize = 4096;
F32 SceneGraph::smVisibleDistanceMod = 1.0;

F32 SceneGraph::mHazeArray[FogTextureDistSize];
U32 SceneGraph::mHazeArrayi[FogTextureDistSize];
F32 SceneGraph::mDistArray[FogTextureDistSize];
bool SceneGraph::renderFog = true;


//------------------------------------------------------------------------------
//-------------------------------------- IMPLEMENTATION
SceneGraph::SceneGraph(bool isClient)
{
    VECTOR_SET_ASSOCIATION(mRefPoolBlocks);
    VECTOR_SET_ASSOCIATION(mZoneManagers);
    VECTOR_SET_ASSOCIATION(mZoneLists);

    mHazeArrayDirty = true;
    mCurrZoneEnd = 0;
    mNumActiveZones = 0;

    mIsClient = isClient;
    mNumFogVolumes = 0;
    mFogDistance = 250;
    mVisibleDistance = 500;
    mFogColor.set(128, 128, 128);

    mCurrSky = NULL;
#ifdef TORQUE_TERRAIN
    mCurrTerrain = NULL;
#endif
    mFreeRefPool = NULL;
    addRefPoolBlock();

    mCurrDecalManager = NULL;
    mGlowBuffer = NULL;
    mReflectPass = false;

    mHeightOffset = 0.0;

    mDisplayTargetResolution.set(0,0);
}

SceneGraph::~SceneGraph()
{
    mCurrZoneEnd = 0;
    mNumActiveZones = 0;

    for (U32 i = 0; i < mRefPoolBlocks.size(); i++) {
        SceneObjectRef* pool = mRefPoolBlocks[i];
        for (U32 j = 0; j < csmRefPoolBlockSize; j++)
            AssertFatal(pool[j].object == NULL, "Error, some object isn't properly out of the bins!");

        delete[] pool;
    }
    mFreeRefPool = NULL;
}


void SceneGraph::addRefPoolBlock()
{
    mRefPoolBlocks.push_back(new SceneObjectRef[csmRefPoolBlockSize]);
    for (U32 i = 0; i < csmRefPoolBlockSize - 1; i++) {
        mRefPoolBlocks.last()[i].object = NULL;
        mRefPoolBlocks.last()[i].prevInBin = NULL;
        mRefPoolBlocks.last()[i].nextInBin = NULL;
        mRefPoolBlocks.last()[i].nextInObj = &(mRefPoolBlocks.last()[i + 1]);
    }
    mRefPoolBlocks.last()[csmRefPoolBlockSize - 1].object = NULL;
    mRefPoolBlocks.last()[csmRefPoolBlockSize - 1].prevInBin = NULL;
    mRefPoolBlocks.last()[csmRefPoolBlockSize - 1].nextInBin = NULL;
    mRefPoolBlocks.last()[csmRefPoolBlockSize - 1].nextInObj = mFreeRefPool;

    mFreeRefPool = &(mRefPoolBlocks.last()[0]);
}


bool SceneGraph::useSpecial = false;

void SceneGraph::renderScene(const U32 objectMask)
{
    mGlowBuffer = static_cast<GlowBuffer*>(Sim::findObject("GlowBufferData"));
    if (!mGlowBuffer)
    {
        static bool glowError = false;
        if (!glowError)
        {
            Con::errorf("Missing glow buffer datablock.  Check glowBuffer.cs");
            glowError = true;
        }
    }


    PROFILE_START(SceneGraphRender);
    if (smVisibleDistanceMod > 1.0f)
        smVisibleDistanceMod = 1.0f;
    else if (smVisibleDistanceMod < 0.5f)
        smVisibleDistanceMod = 0.5f;


    // Determine the camera position, and store off render state...
    MatrixF modelview;
    MatrixF mv;
    Point3F cp;

    modelview = GFX->getWorldMatrix();

    mv = modelview;
    mv.inverse();
    mv.getColumn(3, &cp);
    setBaseCameraPosition(cp);

    gRenderInstManager.setCamPos(cp);

    static bool prevEnvColor;
    static bool prevMultiTex;

    // Set up the base SceneState.
    F32 left, right, top, bottom, nearPlane, farPlane;
    RectI viewport;

    GFX->getFrustum(&left, &right, &bottom, &top, &nearPlane, &farPlane);
    viewport = GFX->getViewport();

    SceneState* pBaseState = new SceneState(NULL,
        mCurrZoneEnd,
        left, right,
        bottom, top,
        nearPlane,
        F64(getVisibleDistanceMod()),
        viewport,
        cp,
        modelview,
        getFogDistanceMod(),
        getVisibleDistanceMod(),
        mFogColor,
        mNumFogVolumes,
        mFogVolumes,
        smVisibleDistanceMod);

    pBaseState->mFlipCull = isReflectPass();

    PROFILE_START(BuildFogTexture);
    buildFogTexture(pBaseState);
    PROFILE_END();

    // Find the start zone...
    SceneObject* startObject;
    U32          startZone;
    findZone(cp, startObject, startZone);


    // grab the lights...
    //PROFILE_START(RegisterLights);
    //mLightManager.sgRegisterGlobalLights(false);
    //PROFILE_END();


    DetailManager::beginPrepRender();

    PROFILE_START(BuildSceneTree);
    buildSceneTree(pBaseState, startObject, startZone, 1, objectMask);
    PROFILE_END();

    DetailManager::endPrepRender();

    PROFILE_START(TraverseScene);
    traverseSceneTree(pBaseState);
    PROFILE_END();

    if (gDebugDraw) gDebugDraw->render();


    // grab the lights...
    //PROFILE_START(UnregisterLights);
    //mLightManager.sgUnregisterAllLights();
    //PROFILE_END();


    delete pBaseState;
    PROFILE_END();
}

//---------------------------
void fogTextureShot(GFXTexHandle fogTexture)
{
    GBitmap* fogBitmap = fogTexture.getBitmap();

    FileStream fStream;
    if (fStream.open("fogTextureTest.png", FileStream::Write) != NULL)
    {
        fogBitmap->writePNG(fStream);
    }
}

void SceneGraph::buildFogTexture(SceneState* pState)
{
    if (isReflectPass())
    {
        return;
    }

    if (!bool(mFogTexture))
    {
        // This texture should be Dynamic AND KeepBitmap - not possible
        //  right now - bramage
        mFogTexture = GFXTexHandle(new GBitmap(FogTextureDistSize, FogTextureHeightSize, false, GFXFormatR8G8B8A8), &GFXDefaultPersistentProfile, true);
        mFogTextureIntensity = mFogTexture;
    }

    // If there is no scenestate, lets just create an empty fog (needed for GuiObjectView currently)
    if (!pState)
    {
        GBitmap* fogBitmap = mFogTexture.getBitmap();
        U8* bits = fogBitmap->getWritableBits();
        dMemset(bits, 0, fogBitmap->bytesPerPixel * fogBitmap->getWidth() * fogBitmap->getHeight());
        mFogTexture.refresh();
        //GBitmap* blackfogBitmap = mBlackFogTexture.getBitmap();
        //U8* blackbits = blackfogBitmap->getWritableBits();
        //dMemset(blackbits, 0, blackfogBitmap->bytesPerPixel * blackfogBitmap->getWidth() * blackfogBitmap->getHeight());
        //mBlackFogTexture.refresh();
        return;
    }

    const Point3F& cp = pState->getCameraPosition();

    F32 heightRange = 250.0;

#ifdef TORQUE_TERRAIN
    // build the fog texture
    TerrainBlock* block = getCurrentTerrain();

    if (block)
    {
        GridSquare* sq = block->findSquare(TerrainBlock::BlockShift, Point2I(0, 0));
        heightRange = fixedToFloat(sq->maxHeight - sq->minHeight);
        mHeightOffset = fixedToFloat(sq->minHeight);
    }
#endif

    mInvHeightRange = 1 / heightRange;
    mInvVisibleDistance = 1 / getVisibleDistanceMod();

    GBitmap* fogBitmap = mFogTexture.getBitmap();

    F32 heightStep = heightRange / F32(fogBitmap->getHeight());

    // inset distance half a texel (to the texel center)
    F32 distStart = getVisibleDistanceMod() - (getVisibleDistanceMod() / (fogBitmap->getWidth() * 2));
    ColorI fogColor(mFogColor);

    if (mHazeArrayDirty)
    {
        F32 distStep = -getVisibleDistanceMod() / F32(fogBitmap->getWidth());
        F32 dist = distStart;
        for (U32 i = 0; i < FogTextureDistSize; i++)
        {
            mHazeArray[i] = pState->getHaze(dist);
            F32 prevDist = dist;
            dist += distStep;
            mDistArray[i] = dist / prevDist;
        }
        mHazeArrayDirty = false;
    }

    F32 ht = mHeightOffset + (heightStep / 2) - cp.z;

    for (U32 j = 0; j < fogBitmap->getHeight(); j++)
    {
        F32 dist = distStart;
        U32* ptr = (U32*)fogBitmap->getAddress(0, j);

        // fog texture goes from dist = visibleDistance at u = 0 to dist = 0 at u = 1
        // makes the math on the texture computation cleaner
        F32 fogStart = pState->getFog(dist, ht);

        for (U32 i = 0; i < fogBitmap->getWidth(); i++)
        {
            U32 fog = (U32)((fogStart + mHazeArray[i]) * 255);
            fogStart *= mDistArray[i];
            if (fog > 255)
                fog = 255;

            // NOTE: the two platforms want their results in diff orders.
            // Mac is typically endian flipped of PC ARGB format (i.e., BGRA).
            // above getRGBEndian gets the RGB component in proper order, but
            // we still need to mix-in the alpha component in the right location.

            fogColor.alpha = fog;

            // mac code needs alpha low - don't ask me why. This isn't typical, but code is around PC-memory-order.
#if defined(TORQUE_BIG_ENDIAN)
            *ptr++ = fogColor.getBGRAPack();
#else            // intel wants alpha high
            * ptr++ = fogColor.getARGBPack();
#endif
        }
        ht += heightStep;
    }

    mFogTexture.refresh();

    static bool init = false;
    if (!init)
    {
        Con::setBoolVariable("$takeFogShot", false);
        init = true;
    }

    if (Con::getBoolVariable("$takeFogShot"))
    {
        fogTextureShot(mFogTexture);
        Con::setBoolVariable("$takeFogShot", false);
    }

    //   if(takeShot)
    //      fogTextureShot(mFogTexture);
}

//--------------------------------------------------------------------------
void SceneGraph::traverseSceneTree(SceneState* pState)
{
    // DMM FIX: only handles trees one deep for now

    if (pState->mSubsidiaries.size() != 0) {
        for (U32 i = 0; i < pState->mSubsidiaries.size(); i++)
            traverseSceneTree(pState->mSubsidiaries[i]);
    }

    if (pState->mParent != NULL) {
        // Comes from a transform portal.  Let's see if we need to flip the cull

        // Now, the index gives the TransformPortal index in the Parent...
        SceneObject* pPortalOwner = pState->mPortalOwner;
        U32 portalIndex = pState->mPortalIndex;
        AssertFatal(pPortalOwner != NULL && portalIndex != 0xFFFFFFFF,
            "Hm, this should never happen.  We should always have an owner and an index here");

        // Ok, open the portal.  Opening and closing the portals is a tricky bit of
        //  work, since we have to get the z values just right.  We're going to toss
        //  the responsibility onto the shoulders of the object that owns the portal.
        pPortalOwner->openPortal(portalIndex, pState, pState->mParent);

        //      if (pState->mFlipCull)
        //         glCullFace(GL_FRONT);

              // Render the objects in this subsidiary...
        pState->renderCurrentImages();

        //      if (pState->mFlipCull)
        //         glCullFace(GL_BACK);

              // close the portal
        pPortalOwner->closePortal(portalIndex, pState, pState->mParent);
    }
    else {
        pState->renderCurrentImages();
    }

}


//----------------------------------------------------------------------------
struct ScopingInfo {
    Point3F        scopePoint;
    F32            scopeDist;
    F32            scopeDistSquared;
    const bool* zoneScopeStates;
    NetConnection* connection;
};


inline void scopeCallback(SceneObject* obj, ScopingInfo* pInfo)
{
    NetConnection* ptr = pInfo->connection;

    if (obj->isScopeable()) {
        F32 difSq = (obj->getWorldSphere().center - pInfo->scopePoint).lenSquared();
        if (difSq < pInfo->scopeDistSquared) {
            // Not even close, it's in...
            ptr->objectInScope(obj);
        }
        else {
            // Check a little more closely...
            F32 realDif = mSqrt(difSq);
            if (realDif - obj->getWorldSphere().radius < pInfo->scopeDist) {
                ptr->objectInScope(obj);
            }
        }
    }
}

void SceneGraph::scopeScene(const Point3F& scopePosition,
    const F32      scopeDistance,
    NetConnection* netConnection)
{
    // Find the start zone...
    SceneObject* startObject;
    U32          startZone;
    findZone(scopePosition, startObject, startZone);

    // Search proceeds from the baseObject, and starts in the baseZone.
    // General Outline:
    //    - Traverse up the tree, stopping at either the root, or the last zone manager
    //       that prevents traversal outside
    //    - This will set up the array of zone states, either scoped or unscoped.
    //       loop through all the objects, placing them in scope if they are in
    //       a scoped zone.

    // Objects (in particular, those managers that are part of the initial up
    //  traversal) keep track of whether or not they have done their scope traversal
    //  by a key which is the same key used for renderState determination

    SceneObject* pTraversalRoot = startObject;
    U32          rootZone = startZone;
    bool* zoneScopeState = new bool[mCurrZoneEnd];
    dMemset(zoneScopeState, 0, sizeof(bool) * mCurrZoneEnd);

    smStateKey++;
    while (true) {
        // Anything that we encounter in our up traversal is scoped
        if (pTraversalRoot->isScopeable())
            netConnection->objectInScope(pTraversalRoot);

        pTraversalRoot->mLastStateKey = smStateKey;
        if (pTraversalRoot->scopeObject(scopePosition, scopeDistance,
            zoneScopeState)) {
            // Continue upwards
            if (pTraversalRoot->getNumCurrZones() != 1) {
                Con::errorf(ConsoleLogEntry::General,
                    "Error, must have one and only one zone to be a traversal root.  %s has %d",
                    pTraversalRoot->getName(), pTraversalRoot->getNumCurrZones());
            }

            rootZone = pTraversalRoot->getCurrZone(0);
            pTraversalRoot = getZoneOwner(rootZone);
        }
        else {
            // Terminate.  This is the traveral root...
            break;
        }
    }

    S32 i;

    // Note that we start at 1 here rather than 0, since if the root was going to be
    //  scoped, it would have been scoped in the up traversal rather than at this stage.
    //  Also, it doesn't have a CurrZone(0), so that's bad... :)
    for (i = 1; i < mZoneManagers.size(); i++) {
        if (mZoneManagers[i].obj->mLastStateKey != smStateKey &&
            zoneScopeState[mZoneManagers[i].obj->getCurrZone(0)] == true) {
            // Scope the zones in this manager...
            mZoneManagers[i].obj->scopeObject(scopePosition, scopeDistance, zoneScopeState);
        }
    }


    ScopingInfo info;
    info.scopePoint = scopePosition;
    info.scopeDist = scopeDistance;
    info.scopeDistSquared = scopeDistance * scopeDistance;
    info.zoneScopeStates = zoneScopeState;
    info.connection = netConnection;

    for (i = 0; i < mCurrZoneEnd; i++) {
        // Zip through the zone lists...
        if (zoneScopeState[i] == true) {
            // Scope zone i...
            SceneObjectRef* pList = mZoneLists[i];
            SceneObjectRef* pWalk = pList->nextInBin;
            while (pWalk != NULL) {
                SceneObject* pObject = pWalk->object;
                if (pObject->mLastStateKey != smStateKey) {
                    pObject->mLastStateKey = smStateKey;
                    scopeCallback(pObject, &info);
                }

                pWalk = pWalk->nextInBin;
            }
        }
    }

    delete[] zoneScopeState;
    zoneScopeState = NULL;
}


//------------------------------------------------------------------------------
bool SceneGraph::addObjectToScene(SceneObject* obj)
{
#ifdef TORQUE_TERRAIN
    if (obj->getType() & TerrainObjectType) {
        // Double check
        AssertFatal(dynamic_cast<TerrainBlock*>(obj) != NULL, "Not a terrain, but a terrain type?");
        mCurrTerrain = static_cast<TerrainBlock*>(obj);
    }
#endif
    if (obj->getType() & EnvironmentObjectType) {
        if (dynamic_cast<Sky*>(obj) != NULL) {
            mCurrSky = static_cast<Sky*>(obj);
        }
    }

    if (obj->getType() & DecalManagerObjectType) {
        if (dynamic_cast<DecalManager*>(obj) != NULL) {
            mCurrDecalManager = static_cast<DecalManager*>(obj);
        }
    }

    if (obj->getType() & WaterObjectType)
    {
        addToWaterList(obj);
    }

    return obj->onSceneAdd(this);
}


//------------------------------------------------------------------------------
void SceneGraph::removeObjectFromScene(SceneObject* obj)
{
    if (obj->mSceneManager != NULL) {
        AssertFatal(obj->mSceneManager == this, "Error, removing from the wrong sceneGraph!");

#ifdef TORQUE_TERRAIN
        if (obj->getType() & TerrainObjectType) {
            // Double check
            AssertFatal(dynamic_cast<TerrainBlock*>(obj) != NULL, "Not a terrain, but a terrain type?");
            if (mCurrTerrain == static_cast<TerrainBlock*>(obj))
                mCurrTerrain = NULL;
        }
#endif
        if (obj->getType() & EnvironmentObjectType) {
            if (dynamic_cast<Sky*>(obj) != NULL && mCurrSky == static_cast<Sky*>(obj))
                mCurrSky = NULL;
        }

        if (obj->getType() & DecalManagerObjectType) {
            if (dynamic_cast<DecalManager*>(obj) != NULL && mCurrDecalManager == static_cast<DecalManager*>(obj))
                mCurrDecalManager = NULL;
        }

        if (obj->getType() & WaterObjectType)
        {
            removeFromWaterList(obj);
        }

        obj->onSceneRemove();
    }
}


//------------------------------------------------------------------------------
void SceneGraph::registerZones(SceneObject* obj, U32 numZones)
{
    AssertFatal(alreadyManagingZones(obj) == false, "Error, added zones twice!");
    compactZonesCheck();

    U32 i;
    U32 retVal = mCurrZoneEnd;
    mCurrZoneEnd += numZones;
    mNumActiveZones += numZones;

    mZoneLists.increment(numZones);
    for (i = mCurrZoneEnd - numZones; i < mCurrZoneEnd; i++) {
        mZoneLists[i] = new SceneObjectRef;
        mZoneLists[i]->object = obj;
        mZoneLists[i]->nextInBin = NULL;
        mZoneLists[i]->prevInBin = NULL;
        mZoneLists[i]->nextInObj = NULL;
    }

    ZoneManager newEntry;
    newEntry.obj = obj;
    newEntry.numZones = numZones;
    newEntry.zoneRangeStart = retVal;
    mZoneManagers.push_back(newEntry);
    obj->mZoneRangeStart = retVal;

    // Since we now have new zones in this space, we need to rezone any intersecting
    //  objects.  Query the container database to find all intersecting/contained
    //  objects, and rezone them
    Container* pQueryContainer = this == gClientSceneGraph ? &gClientContainer :
        (this == gServerSceneGraph ? &gServerContainer :
            (this == gSPModeSceneGraph ? &gSPModeContainer : nullptr));
    // query
    SimpleQueryList list;
    pQueryContainer->findObjects(obj->mWorldBox, 0xFFFFFFFF, SimpleQueryList::insertionCallback, &list);

    // DMM: Horrendously inefficient.  We should do the rejection test against
    //  obj here
    for (i = 0; i < list.mList.size(); i++) {
        SceneObject* rezoneObj = list.mList[i];

        // Make sure this is actually a SceneObject, is not the zone manager,
        //  and is added to the scene manager.
        if (rezoneObj != NULL && rezoneObj != obj &&
            rezoneObj->mSceneManager != NULL)
            rezoneObject(rezoneObj);
    }
}


//------------------------------------------------------------------------------
void SceneGraph::unregisterZones(SceneObject* obj)
{
    AssertFatal(alreadyManagingZones(obj) == true, "Error, not managing any zones!");

    // First, let's nuke the lists associated with this object.  We can leave the
    //  horizontal references in the objects in place, they'll be freed before too
    //  long.
    for (U32 i = 0; i < mZoneManagers.size(); i++) {
        if (obj == mZoneManagers[i].obj) {
            AssertFatal(mNumActiveZones >= mZoneManagers[i].numZones, "Too many zones removed");

            for (U32 j = mZoneManagers[i].zoneRangeStart;
                j < mZoneManagers[i].zoneRangeStart + mZoneManagers[i].numZones; j++) {
                SceneObjectRef* pList = mZoneLists[j];
                SceneObjectRef* pWalk = pList->nextInBin;

                // We have to tree pList a little differently, since it's not a traditional
                //  link.  We can just delete it at the end...
                pList->object = NULL;
                delete pList;
                mZoneLists[j] = NULL;

                while (pWalk) {
                    AssertFatal(pWalk->object != NULL, "Error, must have an object!");
                    SceneObjectRef* pTrash = pWalk;
                    pWalk = pWalk->nextInBin;

                    pTrash->nextInBin = pTrash;
                    pTrash->prevInBin = pTrash;

                    // Ok, now we need to zip through the list in the object to find
                    //  this and remove it since we aren't doubly linked...
                    SceneObjectRef** ppRef = &pTrash->object->mZoneRefHead;
                    bool found = false;
                    while (*ppRef) {
                        if (*ppRef == pTrash) {
                            // Remove it
                            *ppRef = (*ppRef)->nextInObj;
                            found = true;

                            pTrash->object = NULL;
                            pTrash->nextInBin = NULL;
                            pTrash->prevInBin = NULL;
                            pTrash->nextInObj = NULL;
                            pTrash->zone = 0xFFFFFFFF;
                            freeObjectRef(pTrash);
                            break;
                        }

                        ppRef = &(*ppRef)->nextInObj;
                    }
                    AssertFatal(found == true, "Error, should have found that reference!");
                }
            }

            mNumActiveZones -= mZoneManagers[i].numZones;
            mZoneManagers.erase(i);
            obj->mZoneRangeStart = 0xFFFFFFFF;

            // query
            if ((mIsClient == true &&  obj != gClientSceneRoot) ||
                (mIsClient == false && obj != gServerSceneRoot) &&
                                       obj != gSPModeSceneRoot)
            {
                Container* pQueryContainer = this == gClientSceneGraph ? &gClientContainer :
                    (this == gServerSceneGraph ? &gServerContainer :
                        (this == gSPModeSceneGraph ? &gSPModeContainer : nullptr));
                SimpleQueryList list;
                pQueryContainer->findObjects(obj->mWorldBox, 0xFFFFFFFF, SimpleQueryList::insertionCallback, &list);
                for (i = 0; i < list.mList.size(); i++) {
                    SceneObject* rezoneObj = list.mList[i];
                    if (rezoneObj != NULL && rezoneObj != obj && rezoneObj->mSceneManager != NULL)
                        rezoneObject(rezoneObj);
                }
            }
            return;
        }
    }
    compactZonesCheck();

    // Other assert already ensured we will terminate properly...
    AssertFatal(false, "Error, impossible condition reached!");
}


//------------------------------------------------------------------------------
void SceneGraph::compactZonesCheck()
{
    if (mNumActiveZones > (mCurrZoneEnd / 2))
        return;

    // DMMTODO: Compact zones...
    //
}


//------------------------------------------------------------------------------
bool SceneGraph::alreadyManagingZones(SceneObject* obj) const
{
    for (U32 i = 0; i < mZoneManagers.size(); i++)
        if (obj == mZoneManagers[i].obj)
            return true;
    return false;
}


//------------------------------------------------------------------------------
void SceneGraph::findZone(const Point3F& p, SceneObject*& owner, U32& zone)
{
    // Since there is no zone information maintained by the sceneGraph
    //  any more, this is quite brain-dead.  Maybe fix this?  DMM
    //
    U32 currZone = 0;
    SceneObject* currOwner = mZoneManagers[0].obj;

    PROFILE_START(SG_FindZone);
    while (true)
    {
        bool cont = false;

        // Loop, but don't consider the root...
        for (U32 i = 1; i < mZoneManagers.size(); i++)
        {
            AssertWarn(mZoneManagers[i].obj->getNumCurrZones() == 1 || (i == 0 && mZoneManagers[i].obj->getNumCurrZones() == 0) || gSPMode,
                "ZoneManagers are only allowed to belong to one and only one zone!");
            if (mZoneManagers[i].obj->getCurrZone(0) == currZone)
            {
                // Test to see if the point is inside
                U32 testZone = mZoneManagers[i].obj->getPointZone(p);
                if (testZone != 0)
                {
                    // Point is in this manager, reset, and descend
                    cont = true;
                    currZone = testZone;
                    currOwner = mZoneManagers[i].obj;
                    break;
                }
            }
        }

        // Have we gone as far as we can?
        if (cont == false)
            break;
    }

    zone = currZone;
    owner = currOwner;
    PROFILE_END();
}


//------------------------------------------------------------------------------
void SceneGraph::rezoneObject(SceneObject* obj)
{
    AssertFatal(obj->mSceneManager != NULL && obj->mSceneManager == this, "Error, bad or no scenemanager here!");
    PROFILE_START(SG_Rezone);

    if (obj->mZoneRefHead != NULL) {
        // Remove the object from the zone lists...
        SceneObjectRef* walk = obj->mZoneRefHead;
        while (walk) {
            SceneObjectRef* remove = walk;
            walk = walk->nextInObj;

            remove->prevInBin->nextInBin = remove->nextInBin;
            if (remove->nextInBin)
                remove->nextInBin->prevInBin = remove->prevInBin;

            remove->nextInObj = NULL;
            remove->nextInBin = NULL;
            remove->prevInBin = NULL;
            remove->object = NULL;
            remove->zone = U32(-1);

            freeObjectRef(remove);
        }
        obj->mZoneRefHead = NULL;
    }


    U32 numMasterZones = 0;
    SceneObject* masterZoneOwners[SceneObject::MaxObjectZones];
    U32          masterZoneBuffer[SceneObject::MaxObjectZones];

    S32 i;
    for (i = S32(mZoneManagers.size()) - 1; i >= 0; i--) {
        // Careful, zone managers are in the list at this point...
        if (obj == mZoneManagers[i].obj)
            continue;

        if (mZoneManagers[i].obj->getWorldBox().isOverlapped(obj->getWorldBox()) == false)
            continue;

        // We have several possible outcomes here
        //  1: Object completely contained in zoneManager
        //  2: object overlaps manager. (outside zone is included)
        //  3: Object completely contains manager (outside zone not included)
        // In case 3, we ignore the possibility that the object resides in
        //  zones managed by the manager, and we can continue
        // In case 1 and 2, we need to query the manager for zones.
        // In case 1, we break out of the loop, unless the object is not a
        //  part of the managers interior zones.
        // In case 2, we need to continue querying the database until we
        //  stop due to one of the above conditions (guaranteed to happen
        //  when we reach the sceneRoot.  (Zone 0)
        //
        if (obj->getWorldBox().isContained(mZoneManagers[i].obj->getWorldBox())) {
            // case 3
            continue;
        }

        // Query the zones...
        U32 numZones = 0;
        U32 zoneBuffer[SceneObject::MaxObjectZones];

        bool outsideIncluded = mZoneManagers[i].obj->getOverlappingZones(obj, zoneBuffer, &numZones);
        AssertFatal(numZones != 0 || outsideIncluded == true, "Hm, no zones, but not in the outside zone?  Impossible!");

        // Copy the included zones out
        if (numMasterZones + numZones > SceneObject::MaxObjectZones)
            Con::errorf(ConsoleLogEntry::General, "Zone Overflow!  Object will NOT render correctly.  Copying out as many as possible");
        numZones = getMin(numZones, SceneObject::MaxObjectZones - numMasterZones);

        for (U32 j = 0; j < numZones; j++) {
            masterZoneBuffer[numMasterZones] = zoneBuffer[j];
            masterZoneOwners[numMasterZones++] = mZoneManagers[i].obj;
        }

        if (outsideIncluded == false) {
            // case 3.  We can stop the search at this point...
            break;
        }
        else {
            // Case 2.  We need to continue searching...
            // ...
        }
    }
    // Copy the found zones into the buffer...
    AssertFatal(numMasterZones != 0, "Error, no zones found?  Should always find root at least.");

    obj->mNumCurrZones = numMasterZones;
    for (i = 0; i < numMasterZones; i++) {
        // Insert into zone masterZoneBuffer[i]
        SceneObjectRef* zoneList = mZoneLists[masterZoneBuffer[i]];
        AssertFatal(zoneList != NULL, "Error, no list for this zone!");

        SceneObjectRef* newRef = allocateObjectRef();

        // Get it into the list
        newRef->zone = masterZoneBuffer[i];
        newRef->object = obj;
        newRef->nextInBin = zoneList->nextInBin;
        newRef->prevInBin = zoneList;
        if (zoneList->nextInBin)
            zoneList->nextInBin->prevInBin = newRef;
        zoneList->nextInBin = newRef;

        // Now get it into the objects chain...
        newRef->nextInObj = obj->mZoneRefHead;
        obj->mZoneRefHead = newRef;
    }
    PROFILE_END();
}

void SceneGraph::zoneInsert(SceneObject* obj)
{
    PROFILE_START(SG_ZoneInsert);
    AssertFatal(obj->mNumCurrZones == 0, "Error, already entered into zone list...");

    rezoneObject(obj);

    if (obj->isManagingZones()) {
        // Query the container database to find all intersecting/contained
        //  objects, and rezone them
        Container* pQueryContainer = this == gClientSceneGraph ? &gClientContainer :
            (this == gServerSceneGraph ? &gServerContainer :
                (this == gSPModeSceneGraph ? &gSPModeContainer : nullptr));
        // query
        SimpleQueryList list;
        pQueryContainer->findObjects(obj->mWorldBox, 0xFFFFFFFF, SimpleQueryList::insertionCallback, &list);

        // DMM: Horrendously inefficient.  We should do the rejection test against
        //  obj here, but the zoneManagers are so infrequently inserted and removed that
        //  it really doesn't matter...
        for (U32 i = 0; i < list.mList.size(); i++)
        {
            SceneObject* rezoneObj = list.mList[i];

            // Make sure this is actually a SceneObject, is not the zone manager,
            //  and is added to the scene manager.
            if (rezoneObj != NULL && rezoneObj != obj &&
                rezoneObj->mSceneManager == this)
                rezoneObject(rezoneObj);
        }
    }
    PROFILE_END();
}


//------------------------------------------------------------------------------
void SceneGraph::zoneRemove(SceneObject* obj)
{
    PROFILE_START(SG_ZoneRemove);
    obj->mNumCurrZones = 0;

    // Remove the object from the zone lists...
    SceneObjectRef* walk = obj->mZoneRefHead;
    while (walk) {
        SceneObjectRef* remove = walk;
        walk = walk->nextInObj;

        remove->prevInBin->nextInBin = remove->nextInBin;
        if (remove->nextInBin)
            remove->nextInBin->prevInBin = remove->prevInBin;

        remove->nextInObj = NULL;
        remove->nextInBin = NULL;
        remove->prevInBin = NULL;
        remove->object = NULL;
        remove->zone = U32(-1);

        freeObjectRef(remove);
    }
    obj->mZoneRefHead = NULL;
    PROFILE_END();
}

void SceneGraph::setFogColor(ColorF color)
{
    mFogColor = color;
}

void SceneGraph::setVisibleDistance(F32 dist)
{
    mHazeArrayDirty = true;
    mVisibleDistance = dist;
}

void SceneGraph::setFogDistance(F32 dist)
{
    mHazeArrayDirty = true;
    mFogDistance = dist;
}

void SceneGraph::setFogVolumes(U32 numFogVolumes, FogVolume* fogVolumes)
{
    mNumFogVolumes = getMin(numFogVolumes, U32(MaxFogVolumes));
    for (U32 i = 0; i < mNumFogVolumes; i++)
        mFogVolumes[i] = fogVolumes[i];
}

void SceneGraph::getWaterObjectList(SimpleQueryList& sql)
{
    sql.mList.setSize(mWaterList.size());
    for (S32 i = 0; i < mWaterList.size(); i++)
        sql.mList[i] = mWaterList[i];
}

void SceneGraph::addToWaterList(SceneObject* obj)
{
#if defined(TORQUE_DEBUG)
    for (S32 i = 0; i < mWaterList.size(); i++)
    {
        AssertFatal(mWaterList[i] != obj, "Error, object already on water list!");
    }
#endif

    mWaterList.push_back(obj);
}

void SceneGraph::removeFromWaterList(SceneObject* obj)
{
#if defined(TORQUE_DEBUG)
    bool found = false;
    for (S32 i = 0; i < mWaterList.size(); i++)
    {
        if (obj == mWaterList[i])
            found = true;
    }
    AssertFatal(found, "Error, object not on water list!");
#endif

    for (S32 i = 0; i < mWaterList.size(); i++)
    {
        if (mWaterList[i] == obj)
        {
            mWaterList.erase(i);
            return;
        }
    }
}

void SceneGraph::setDisplayTargetResolution( const Point2I &size )
{
    mDisplayTargetResolution = size;
}

const Point2I & SceneGraph::getDisplayTargetResolution() const
{
    return mDisplayTargetResolution;
}
