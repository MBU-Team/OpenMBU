//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "sim/sceneObject.h"
#include "sceneGraph/sceneGraph.h"
#include "console/consoleTypes.h"
#include "collision/extrudedPolyList.h"
#include "collision/earlyOutPolyList.h"
#include "platform/profiler.h"

#include "platform/profiler.h"
#include "interior/interior.h"
#include "interior/interiorInstance.h"
#ifdef TORQUE_TERRAIN
#include "terrain/terrData.h"
#include "atlas/runtime/atlasInstance2.h"
#endif
#include "gfx/gBitmap.h"
#include "lightingSystem/sgLightObject.h"
#include "sim/netConnection.h"

IMPLEMENT_CONOBJECT(SceneObject);

const U32 Container::csmNumBins = 16;
const F32 Container::csmBinSize = 64;
const F32 Container::csmTotalBinSize = Container::csmBinSize * Container::csmNumBins;
U32       Container::smCurrSeqKey = 1;
const U32 Container::csmRefPoolBlockSize = 4096;

// Statics used by buildPolyList methods
AbstractPolyList* sPolyList;
SphereF sBoundingSphere;
Box3F sBoundingBox;

// Statics used by collide methods
ExtrudedPolyList sExtrudedPolyList;
Polyhedron sBoxPolyhedron;

//--------------------------------------------------------------------------
//-------------------------------------- Console callbacks
//

ConsoleMethod(SceneObject, getTransform, const char*, 2, 2, "Get transform of object.")
{
    char* returnBuffer = Con::getReturnBuffer(256);
    const MatrixF& mat = object->getTransform();
    Point3F pos;
    mat.getColumn(3, &pos);
    AngAxisF aa(mat);
    dSprintf(returnBuffer, 256, "%g %g %g %g %g %g %g",
        pos.x, pos.y, pos.z, aa.axis.x, aa.axis.y, aa.axis.z, aa.angle);
    return returnBuffer;
}

ConsoleMethod(SceneObject, getPosition, const char*, 2, 2, "Get position of object.")
{
    char* returnBuffer = Con::getReturnBuffer(256);
    const MatrixF& mat = object->getTransform();
    Point3F pos;
    mat.getColumn(3, &pos);
    dSprintf(returnBuffer, 256, "%g %g %g", pos.x, pos.y, pos.z);
    return returnBuffer;
}

ConsoleMethod(SceneObject, getForwardVector, const char*, 2, 2, "Returns a vector indicating the direction this object is facing.")
{
    char* returnBuffer = Con::getReturnBuffer(256);
    const MatrixF& mat = object->getTransform();
    Point3F dir;
    mat.getColumn(1, &dir);
    dSprintf(returnBuffer, 256, "%g %g %g", dir.x, dir.y, dir.z);
    return returnBuffer;
}

ConsoleMethod(SceneObject, setTransform, void, 3, 3, "(Transform T)")
{
    Point3F pos;
    const MatrixF& tmat = object->getTransform();
    tmat.getColumn(3, &pos);
    AngAxisF aa(tmat);

    dSscanf(argv[2], "%g %g %g %g %g %g %g",
        &pos.x, &pos.y, &pos.z, &aa.axis.x, &aa.axis.y, &aa.axis.z, &aa.angle);

    MatrixF mat;
    aa.setMatrix(&mat);
    mat.setColumn(3, pos);
    object->setTransform(mat);
}

ConsoleMethod(SceneObject, getScale, const char*, 2, 2, "Get scaling as a Point3F.")
{
    char* returnBuffer = Con::getReturnBuffer(256);
    const VectorF& scale = object->getScale();
    dSprintf(returnBuffer, 256, "%g %g %g",
        scale.x, scale.y, scale.z);
    return(returnBuffer);
}

ConsoleMethod(SceneObject, setScale, void, 3, 3, "(Point3F scale)")
{
    VectorF scale(0.f, 0.f, 0.f);
    dSscanf(argv[2], "%g %g %g", &scale.x, &scale.y, &scale.z);
    object->setScale(scale);
}

ConsoleMethod(SceneObject, getWorldBox, const char*, 2, 2, "Returns six fields, two Point3Fs, containing the min and max points of the worldbox.")
{
    char* returnBuffer = Con::getReturnBuffer(256);
    const Box3F& box = object->getWorldBox();
    dSprintf(returnBuffer, 256, "%g %g %g %g %g %g",
        box.min.x, box.min.y, box.min.z,
        box.max.x, box.max.y, box.max.z);
    return returnBuffer;
}

ConsoleMethod(SceneObject, getWorldBoxCenter, const char*, 2, 2, "Returns the center of the world bounding box.")
{
    char* returnBuffer = Con::getReturnBuffer(256);
    const Box3F& box = object->getWorldBox();
    Point3F center;
    box.getCenter(&center);

    dSprintf(returnBuffer, 256, "%g %g %g", center.x, center.y, center.z);
    return returnBuffer;
}

ConsoleMethod(SceneObject, getObjectBox, const char*, 2, 2, "Returns the bounding box relative to the object's origin.")
{
    char* returnBuffer = Con::getReturnBuffer(256);
    const Box3F& box = object->getObjBox();
    dSprintf(returnBuffer, 256, "%g %g %g %g %g %g",
        box.min.x, box.min.y, box.min.z,
        box.max.x, box.max.y, box.max.z);
    return returnBuffer;
}

ConsoleFunctionGroupBegin(Containers, "Functions for ray casting and spatial queries.\n\n"
    "@note These only work server-side.");

ConsoleFunction(containerBoxEmpty, bool, 4, 6, "(bitset mask, Point3F center, float xRadius, float yRadius, float zRadius)"
    "See if any objects of given types are present in box of given extent.\n\n"
    "@note Extent parameter is last since only one radius is often needed. If one radius is provided, "
    "the yRadius and zRadius are assumed to be the same.\n"
    "@param  mask   Indicates the type of objects we are checking against.\n"
    "@param  center Center of box.\n"
    "@param  xRadius See above.\n"
    "@param  yRadius See above.\n"
    "@param  zRadius See above.")
{
    Point3F  center;
    Point3F  extent;
    U32      mask = dAtoi(argv[1]);
    dSscanf(argv[2], "%g %g %g", &center.x, &center.y, &center.z);
    extent.x = dAtof(argv[3]);
    extent.y = argc > 4 ? dAtof(argv[4]) : extent.x;
    extent.z = argc > 5 ? dAtof(argv[5]) : extent.x;

    Box3F    B(center - extent, center + extent, true);

    EarlyOutPolyList polyList;
    polyList.mPlaneList.clear();
    polyList.mNormal.set(0, 0, 0);
    polyList.mPlaneList.setSize(6);
    polyList.mPlaneList[0].set(B.min, VectorF(-1, 0, 0));
    polyList.mPlaneList[1].set(B.max, VectorF(0, 1, 0));
    polyList.mPlaneList[2].set(B.max, VectorF(1, 0, 0));
    polyList.mPlaneList[3].set(B.min, VectorF(0, -1, 0));
    polyList.mPlaneList[4].set(B.min, VectorF(0, 0, -1));
    polyList.mPlaneList[5].set(B.max, VectorF(0, 0, 1));

    return !getCurrentServerContainer()->buildPolyList(B, mask, &polyList);
}

ConsoleFunction(initContainerRadiusSearch, void, 4, 4, "(Point3F pos, float radius, bitset mask)"
    "Start a search for items within radius of pos, filtering by bitset mask.")
{
    F32 x, y, z;
    dSscanf(argv[1], "%g %g %g", &x, &y, &z);
    F32 r = dAtof(argv[2]);
    U32 mask = dAtoi(argv[3]);

    getCurrentServerContainer()->initRadiusSearch(Point3F(x, y, z), r, mask);
}

ConsoleFunction(containerSearchNext, S32, 1, 1, "Get next item from a search started with initContainerRadiusSearch.")
{
    return gServerContainer.containerSearchNext();
}

ConsoleFunction(containerSearchCurrDist, F32, 1, 1, "Get distance of the center of the current item from the center of the current initContainerRadiusSearch.")
{
    return gServerContainer.containerSearchCurrDist();
}

ConsoleFunction(containerSearchCurrRadiusDist, F32, 1, 1, "Get the distance of the closest point of the current item from the center of the current initContainerRadiusSearch.")
{
    return gServerContainer.containerSearchCurrRadiusDist();
}

ConsoleFunction(containerRayCast, const char*, 4, 5, "( Point3F start, Point3F end, bitset mask, SceneObject exempt=NULL )"
    "Cast a ray from start to end, checking for collision against items matching mask.\n\n"
    "If exempt is specified, then it is temporarily excluded from collision checks (For "
    "instance, you might want to exclude the player if said player was firing a weapon.)\n"
    "@returns A string containing either null, if nothing was struck, or these fields:\n"
    "            - The ID of the object that was struck.\n"
    "            - The x, y, z position that it was struck.\n"
    "            - The x, y, z of the normal of the face that was struck.")
{
    char* returnBuffer = Con::getReturnBuffer(256);
    Point3F start, end;
    dSscanf(argv[1], "%g %g %g", &start.x, &start.y, &start.z);
    dSscanf(argv[2], "%g %g %g", &end.x, &end.y, &end.z);
    U32 mask = dAtoi(argv[3]);

    SceneObject* pExempt = NULL;
    if (argc > 4) {
        U32 exemptId = dAtoi(argv[4]);
        Sim::findObject(exemptId, pExempt);
    }
    if (pExempt)
        pExempt->disableCollision();

    RayInfo rinfo;
    S32 ret = 0;
    if (getCurrentServerContainer()->castRay(start, end, mask, &rinfo) == true)
        ret = rinfo.object->getId();

    if (pExempt)
        pExempt->enableCollision();

    // add the hit position and normal?
    if (ret)
    {
        dSprintf(returnBuffer, 256, "%d %g %g %g %g %g %g",
            ret, rinfo.point.x, rinfo.point.y, rinfo.point.z,
            rinfo.normal.x, rinfo.normal.y, rinfo.normal.z);
    }
    else
    {
        returnBuffer[0] = '0';
        returnBuffer[1] = '\0';
    }

    return(returnBuffer);
}

ConsoleFunctionGroupEnd(Containers);

// Utility method for bin insertion
void getBinRange(const F32 min,
    const F32 max,
    U32& minBin,
    U32& maxBin)
{
    AssertFatal(max >= min, "Error, bad range! in getBinRange");

    if ((max - min) >= (Container::csmTotalBinSize - Container::csmBinSize))
    {
        F32 minCoord = mFmod(min, Container::csmTotalBinSize);
        if (minCoord < 0.0f)
        {
            minCoord += Container::csmTotalBinSize;

            // This is truly lame, but it can happen.  There must be a better way to
            //  deal with this.
            if (minCoord == Container::csmTotalBinSize)
                minCoord = Container::csmTotalBinSize - 0.01;
        }

        AssertFatal(minCoord >= 0.0 && minCoord < Container::csmTotalBinSize, "Bad minCoord");

        minBin = U32(minCoord / Container::csmBinSize);
        AssertFatal(minBin < Container::csmNumBins, avar("Error, bad clipping! (%g, %d)", minCoord, minBin));

        maxBin = minBin + (Container::csmNumBins - 1);
        return;
    }
    else
    {

        F32 minCoord = mFmod(min, Container::csmTotalBinSize);

        if (minCoord < 0.0f)
        {
            minCoord += Container::csmTotalBinSize;

            // This is truly lame, but it can happen.  There must be a better way to
            //  deal with this.
            if (minCoord == Container::csmTotalBinSize)
                minCoord = Container::csmTotalBinSize - 0.01;
        }
        AssertFatal(minCoord >= 0.0 && minCoord < Container::csmTotalBinSize, "Bad minCoord");

        F32 maxCoord = mFmod(max, Container::csmTotalBinSize);
        if (maxCoord < 0.0f) {
            maxCoord += Container::csmTotalBinSize;

            // This is truly lame, but it can happen.  There must be a better way to
            //  deal with this.
            if (maxCoord == Container::csmTotalBinSize)
                maxCoord = Container::csmTotalBinSize - 0.01;
        }
        AssertFatal(maxCoord >= 0.0 && maxCoord < Container::csmTotalBinSize, "Bad maxCoord");

        minBin = U32(minCoord / Container::csmBinSize);
        maxBin = U32(maxCoord / Container::csmBinSize);
        AssertFatal(minBin < Container::csmNumBins, avar("Error, bad clipping(min)! (%g, %d)", maxCoord, minBin));
        AssertFatal(minBin < Container::csmNumBins, avar("Error, bad clipping(max)! (%g, %d)", maxCoord, maxBin));

        // MSVC6 seems to be generating some bad floating point code around
        // here when full optimizations are on.  The min != max test should
        // not be needed, but it clears up the VC issue.
        if (min != max && minCoord > maxCoord)
            maxBin += Container::csmNumBins;

        AssertFatal(maxBin >= minBin, "Error, min should always be less than max!");
    }
}


//--------------------------------------------------------------------------
//-------------------------------------- SceneObject implementation
//
LightInfo SceneObject::LightingInfo::smAmbientLight;

SceneObject::SceneObject()
{
    overrideOptions = true;
    receiveLMLighting = true;
    receiveSunLight = true;
    useAdaptiveSelfIllumination = false;
    useCustomAmbientLighting = false;
    customAmbientForSelfIllumination = false;
    customAmbientLighting = ColorF(0.0f, 0.0f, 0.0f);
    lightGroupName = NULL;

    mContainer = 0;
    mTypeMask = DefaultObjectType;
    mCollisionCount = 0;
    mGlobalBounds = false;

    mObjScale.set(1, 1, 1);
    mObjToWorld.identity();
    mWorldToObj.identity();

    mObjBox = Box3F(Point3F(0, 0, 0), Point3F(0, 0, 0));
    mWorldBox = Box3F(Point3F(0, 0, 0), Point3F(0, 0, 0));
    mWorldSphere = SphereF(Point3F(0, 0, 0), 0);

    mRenderObjToWorld.identity();
    mRenderWorldToObj.identity();
    mRenderWorldBox = Box3F(Point3F(0, 0, 0), Point3F(0, 0, 0));
    mRenderWorldSphere = SphereF(Point3F(0, 0, 0), 0);

    mContainerSeqKey = 0;

    mBinRefHead = NULL;

    mSceneManager = NULL;
    mZoneRangeStart = 0xFFFFFFFF;

    mNumCurrZones = 0;
    mZoneRefHead = NULL;

    mLastState = NULL;
    mLastStateKey = 0;

    mBinMinX = 0xFFFFFFFF;
    mBinMaxX = 0xFFFFFFFF;
    mBinMinY = 0xFFFFFFFF;
    mBinMaxY = 0xFFFFFFFF;

    mHidden = false;

#ifdef MB_ULTRA
    mRenderShadow = false;
#endif
}

SceneObject::~SceneObject()
{
    AssertFatal(mZoneRangeStart == 0xFFFFFFFF && mSceneManager == NULL,
        "Error, SceneObject not properly removed from sceneGraph");
    AssertFatal(mZoneRefHead == NULL && mBinRefHead == NULL,
        "Error, still linked in reference lists!");

    unlink();
}

//----------------------------------------------------------------------------
const char* SceneObject::scriptThis()
{
    return Con::getIntArg(getId());
}


//--------------------------------------------------------------------------
void SceneObject::buildConvex(const Box3F&, Convex*)
{
    return;
}

bool SceneObject::buildPolyList(AbstractPolyList*, const Box3F&, const SphereF&)
{
    return false;
}

bool SceneObject::buildRenderPolyList(AbstractPolyList* polyList, const Box3F& box, const SphereF& sphere)
{
    return buildPolyList(polyList, box, sphere);
}

BSPNode* SceneObject::buildCollisionBSP(BSPTree*, const Box3F&, const SphereF&)
{
    return NULL;
}

bool SceneObject::castRay(const Point3F&, const Point3F&, RayInfo*)
{
    return false;
}

bool SceneObject::collideBox(const Point3F& start, const Point3F& end, RayInfo* info)
{
    const F32* pStart = (const F32*)start;
    const F32* pEnd = (const F32*)end;
    const F32* pMin = (const F32*)mObjBox.min;
    const F32* pMax = (const F32*)mObjBox.max;

    F32 maxStartTime = -1;
    F32 minEndTime = 1;
    F32 startTime;
    F32 endTime;

    // used for getting normal
    U32 hitIndex;
    U32 side;

    // walk the axis
    for (U32 i = 0; i < 3; i++)
    {
        //
        if (pStart[i] < pEnd[i])
        {
            if (pEnd[i] < pMin[i] || pStart[i] > pMax[i])
                return(false);

            F32 dist = pEnd[i] - pStart[i];

            startTime = (pStart[i] < pMin[i]) ? (pMin[i] - pStart[i]) / dist : -1;
            endTime = (pEnd[i] > pMax[i]) ? (pMax[i] - pStart[i]) / dist : 1;
            side = 1;
        }
        else
        {
            if (pStart[i] < pMin[i] || pEnd[i] > pMax[i])
                return(false);

            F32 dist = pStart[i] - pEnd[i];
            startTime = (pStart[i] > pMax[i]) ? (pStart[i] - pMax[i]) / dist : -1;
            endTime = (pEnd[i] < pMin[i]) ? (pStart[i] - pMin[i]) / dist : 1;
            side = 0;
        }

        //
        if (startTime > maxStartTime)
        {
            maxStartTime = startTime;
            hitIndex = i * 2 + side;
        }
        if (endTime < minEndTime)
            minEndTime = endTime;
        if (minEndTime < maxStartTime)
            return(false);
    }

    // fail if inside
    if (maxStartTime < 0.f)
        return(false);

    //
    static Point3F boxNormals[] = {
       Point3F(1, 0, 0),
       Point3F(-1, 0, 0),
       Point3F(0, 1, 0),
       Point3F(0,-1, 0),
       Point3F(0, 0, 1),
       Point3F(0, 0,-1),
    };

    //
    info->t = maxStartTime;
    info->object = this;
    mObjToWorld.mulV(boxNormals[hitIndex], &info->normal);
    info->material = 0;
    return(true);
}

void SceneObject::disableCollision()
{
    mCollisionCount++;
    AssertFatal(mCollisionCount < 50, "Wow, that's too much");
}

bool SceneObject::isDisplacable() const
{
    return false;
}

Point3F SceneObject::getMomentum() const
{
    AssertFatal(false, "(SceneObject::getMomentum): Should never be called, this is just a default");
    return Point3F(0, 0, 0);
}

void SceneObject::setMomentum(const Point3F&)
{
    AssertFatal(false, "(SceneObject::setMomentum): Should never be called, this is just a default");
}


F32 SceneObject::getMass() const
{
    AssertFatal(false, "(SceneObject::getMass): Should never be called, this is just a default");
    return 1.0;
}


bool SceneObject::displaceObject(const Point3F&)
{
    AssertFatal(false, "(SceneObject::displaceObject): Should never be called, this is just a default");
    return false;
}


void SceneObject::enableCollision()
{
    if (mCollisionCount)
        --mCollisionCount;
}

bool SceneObject::onAdd()
{
    if (Parent::onAdd() == false)
        return false;

    mWorldToObj = mObjToWorld;
    mWorldToObj.affineInverse();
    resetWorldBox();

    setRenderTransform(mObjToWorld);
    return true;
}

void SceneObject::addToScene()
{
    if (isClientObject() || gSPMode)
    {
        getCurrentClientContainer()->addObject(this);
        getCurrentClientSceneGraph()->addObjectToScene(this);
    }
    else
    {
        getCurrentServerContainer()->addObject(this);
        gServerSceneGraph->addObjectToScene(this);
    }
}

void SceneObject::onRemove()
{
    Parent::onRemove();
}

void SceneObject::inspectPostApply()
{
    if (isServerObject()) {
        setTransform(getTransform());
        setScale(getScale());
    }
}

void SceneObject::removeFromScene()
{
    if (mSceneManager != NULL)
        mSceneManager->removeObjectFromScene(this);
    if (getContainer())
        getContainer()->removeObject(this);
}

void SceneObject::setTransform(const MatrixF& mat)
{
    PROFILE_START(SceneObjectSetTransform);
    mObjToWorld = mWorldToObj = mat;
    mWorldToObj.affineInverse();

    resetWorldBox();

    if (mSceneManager != NULL && mNumCurrZones != 0) {
        mSceneManager->zoneRemove(this);
        mSceneManager->zoneInsert(this);
        if (getContainer())
            getContainer()->checkBins(this);
    }

    if (isClientObject() || gSPMode)
        mLightingInfo.mDirty = true;

    setRenderTransform(mat);
    PROFILE_END();
}

void SceneObject::setScale(const VectorF& scale)
{
    mObjScale = scale;

#ifdef MB_ULTRA
    resetWorldBox();

    if (mSceneManager != NULL && mNumCurrZones != 0) {
        mSceneManager->zoneRemove(this);
        mSceneManager->zoneInsert(this);
        if (getContainer())
            getContainer()->checkBins(this);
    }

    if (isClientObject() || gSPMode)
        mLightingInfo.mDirty = true;

#else
    setTransform(MatrixF(mObjToWorld));
#endif

    // Make sure that any subclasses of me get a chance to react to the
    // scale being changed.
    onScaleChanged();

    setMaskBits(ScaleMask);
}

void SceneObject::resetWorldBox()
{
    AssertFatal(mObjBox.isValidBox(), "Bad object box!");

    mWorldBox = mObjBox;
    mWorldBox.min.convolve(mObjScale);
    mWorldBox.max.convolve(mObjScale);
    mObjToWorld.mul(mWorldBox);

    AssertFatal(mWorldBox.isValidBox(), "Bad world box!");

    // Create mWorldSphere from mWorldBox
    mWorldBox.getCenter(&mWorldSphere.center);
    mWorldSphere.radius = (mWorldBox.max - mWorldSphere.center).len();
}

void SceneObject::setRenderTransform(const MatrixF& mat)
{
    PROFILE_START(SceneObj_setRenderTransform);
    mRenderObjToWorld = mRenderWorldToObj = mat;
    mRenderWorldToObj.affineInverse();

    AssertFatal(mObjBox.isValidBox(), "Bad object box!");
    resetRenderWorldBox();
    PROFILE_END();
}


void SceneObject::resetRenderWorldBox()
{
    AssertFatal(mObjBox.isValidBox(), "Bad object box!");
    mRenderWorldBox = mObjBox;
    mRenderWorldBox.min.convolve(mObjScale);
    mRenderWorldBox.max.convolve(mObjScale);
    mRenderObjToWorld.mul(mRenderWorldBox);
    // #if defined(__linux__) || defined(__OpenBSD__)
    //    if( !mRenderWorldBox.isValidBox() ) {
    //       // reset
    //       mRenderWorldBox.min.set( 0.0f, 0.0f, 0.0f );
    //       mRenderWorldBox.max.set( 0.0f, 0.0f, 0.0f );
    //    }
    // #else
    AssertFatal(mRenderWorldBox.isValidBox(), "Bad world box!");
    //#endif

       // Create mRenderWorldSphere from mRenderWorldBox
    mRenderWorldBox.getCenter(&mRenderWorldSphere.center);
    mRenderWorldSphere.radius = (mRenderWorldBox.max - mRenderWorldSphere.center).len();
}


void SceneObject::initPersistFields()
{
    Parent::initPersistFields();
    addGroup("Transform"); // MM: Added group header.
    addField("position", TypeMatrixPosition, Offset(mObjToWorld, SceneObject));
    addField("rotation", TypeMatrixRotation, Offset(mObjToWorld, SceneObject));
    //addField("rotationEuler", TypeMatrixEulerRotation, Offset(mObjToWorld, SceneObject));
    addField("scale", TypePoint3F, Offset(mObjScale, SceneObject));
    endGroup("Transform"); // MM: Added group footer.
    addGroup("Visibility");
    addField("hidden", TypeBool, Offset(mHidden, SceneObject));
    endGroup("Visibility");

#ifdef MB_ULTRA
    addGroup("Lighting");
    addField("reanderShadow", TypeBool, Offset(mRenderShadow, SceneObject));
    endGroup("Lighting");
#endif
}

bool SceneObject::onSceneAdd(SceneGraph* pGraph)
{
    mSceneManager = pGraph;
    mSceneManager->zoneInsert(this);
    return true;
}

void SceneObject::onSceneRemove()
{
    mSceneManager->zoneRemove(this);
    mSceneManager = NULL;
}

void SceneObject::onScaleChanged()
{
    // Override this function where you need to specially handle something
    // when the size of your object has been changed.
}

bool SceneObject::prepRenderImage(SceneState*, const U32, const U32, const bool)
{
    return false;
}

bool SceneObject::scopeObject(const Point3F&        /*rootPosition*/,
    const F32             /*rootDistance*/,
    bool*                 /*zoneScopeState*/)
{
    AssertFatal(false, "Error, this should never be called on a bare (non-zonemanaging) object.  All zone managers must override this function");
    return false;
}


//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
// A quick note about these three functions.  They should only be called
//  on zoneManagers, but since we don't want to force every non-zoneManager
//  to implement them, they assert out instead of being pure virtual.
//
bool SceneObject::getOverlappingZones(SceneObject*, U32*, U32* numZones)
{
    AssertISV(false, "Pure virtual (essentially) function called.  Should never execute this");
    *numZones = 0;
    return false;
}

U32 SceneObject::getPointZone(const Point3F&)
{
    AssertISV(false, "Error, (essentially) pure virtual function called.  Any object this is called on should override this function");
    return 0;
}

void SceneObject::transformModelview(const U32, const MatrixF&, MatrixF*)
{
    AssertISV(false, "Error, (essentially) pure virtual function called.  Any object this is called on should override this function");
}

void SceneObject::transformPosition(const U32, Point3F&)
{
    AssertISV(false, "Error, (essentially) pure virtual function called.  Any object this is called on should override this function");
}

bool SceneObject::computeNewFrustum(const U32, const F64*, const F64, const F64,
    const RectI&, F64*, RectI&, const bool)
{
    AssertISV(false, "Error, (essentially) pure virtual function called.  Any object this is called on should override this function");
    return false;
}

void SceneObject::openPortal(const U32   /*portalIndex*/,
    SceneState* /*pCurrState*/,
    SceneState* /*pParentState*/)
{
    AssertISV(false, "Error, (essentially) pure virtual function called.  Any object this is called on should override this function");
}

void SceneObject::closePortal(const U32   /*portalIndex*/,
    SceneState* /*pCurrState*/,
    SceneState* /*pParentState*/)
{
    AssertISV(false, "Error, (essentially) pure virtual function called.  Any object this is called on should override this function");
}

void SceneObject::getWSPortalPlane(const U32 /*portalIndex*/, PlaneF*)
{
    AssertISV(false, "Error, (essentially) pure virtual function called.  Any object this is called on should override this function");
}


//----------------------------------------------------------------------------
//-------------------------------------- Container implementation
//
Container::Link::Link()
{
    next = prev = this;
}

void Container::Link::unlink()
{
    next->prev = prev;
    prev->next = next;
    next = prev = this;
}

void Container::Link::linkAfter(Container::Link* ptr)
{
    next = ptr->next;
    next->prev = this;
    prev = ptr;
    prev->next = this;
}

//----------------------------------------------------------------------------

Container gServerContainer;
Container gClientContainer;
Container gSPModeContainer;

Container::Container()
{
    mEnd.next = mEnd.prev = &mStart;
    mStart.next = mStart.prev = &mEnd;

    if (!sBoxPolyhedron.edgeList.size())
    {
        Box3F box;
        box.min.set(-1, -1, -1);
        box.max.set(+1, +1, +1);
        MatrixF imat(1);
        sBoxPolyhedron.buildBox(imat, box);
    }

    mBinArray = new SceneObjectRef[csmNumBins * csmNumBins];
    for (U32 i = 0; i < csmNumBins; i++)
    {
        U32 base = i * csmNumBins;
        for (U32 j = 0; j < csmNumBins; j++)
        {
            mBinArray[base + j].object = NULL;
            mBinArray[base + j].nextInBin = NULL;
            mBinArray[base + j].prevInBin = NULL;
            mBinArray[base + j].nextInObj = NULL;
        }
    }
    mOverflowBin.object = NULL;
    mOverflowBin.nextInBin = NULL;
    mOverflowBin.prevInBin = NULL;
    mOverflowBin.nextInObj = NULL;

    VECTOR_SET_ASSOCIATION(mRefPoolBlocks);
    VECTOR_SET_ASSOCIATION(mSearchList);

    mFreeRefPool = NULL;
    addRefPoolBlock();

    cleanupSearchVectors();
}

Container::~Container()
{
    for (U32 i = 0; i < mRefPoolBlocks.size(); i++)
    {
        SceneObjectRef* pool = mRefPoolBlocks[i];
        for (U32 j = 0; j < csmRefPoolBlockSize; j++)
        {
            // Depressingly, this can give weird results if its pointing at bad memory...
            if (pool[j].object != NULL)
                Con::warnf("Error, a %s (%x) isn't properly out of the bins!", pool[j].object->getClassName(), pool[j].object);

            // If you're getting this it means that an object created didn't
            // remove itself from its container before we destroyed the
            // container. Typically you get this behavior from particle
            // emitters, as they try to hang around until all their particles
            // die. In general it's benign, though if you get it for things
            // that aren't particle emitters it can be a bad sign!
        }

        delete[] pool;
    }
    mFreeRefPool = NULL;

    cleanupSearchVectors();
}

bool Container::addObject(SceneObject* obj)
{
    AssertFatal(obj->mContainer == NULL, "Adding already added object.");
    obj->mContainer = this;
    obj->linkAfter(&mStart);

    insertIntoBins(obj);
    return true;
}

bool Container::removeObject(SceneObject* obj)
{
    AssertFatal(obj->mContainer == this, "Trying to remove from wrong container.");
    removeFromBins(obj);

    obj->mContainer = 0;
    obj->unlink();
    return true;
}

void Container::addRefPoolBlock()
{
    mRefPoolBlocks.push_back(new SceneObjectRef[csmRefPoolBlockSize]);
    for (U32 i = 0; i < csmRefPoolBlockSize - 1; i++)
    {
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


void Container::insertIntoBins(SceneObject* obj)
{
    AssertFatal(obj != NULL, "No object?");
    AssertFatal(obj->mBinRefHead == NULL, "Error, already have a bin chain!");

    // The first thing we do is find which bins are covered in x and y...
    const Box3F* pWBox = &obj->getWorldBox();

    U32 minX, maxX, minY, maxY;
    getBinRange(pWBox->min.x, pWBox->max.x, minX, maxX);
    getBinRange(pWBox->min.y, pWBox->max.y, minY, maxY);

    // Store the current regions for later queries
    obj->mBinMinX = minX;
    obj->mBinMaxX = maxX;
    obj->mBinMinY = minY;
    obj->mBinMaxY = maxY;

    // For huge objects, dump them into the overflow bin.  Otherwise, everything
    //  goes into the grid...
    if ((maxX - minX + 1) < csmNumBins || (maxY - minY + 1) < csmNumBins && !obj->isGlobalBounds())
    {
        SceneObjectRef** pCurrInsert = &obj->mBinRefHead;

        for (U32 i = minY; i <= maxY; i++)
        {
            U32 insertY = i % csmNumBins;
            U32 base = insertY * csmNumBins;
            for (U32 j = minX; j <= maxX; j++)
            {
                U32 insertX = j % csmNumBins;

                SceneObjectRef* ref = allocateObjectRef();

                ref->object = obj;
                ref->nextInBin = mBinArray[base + insertX].nextInBin;
                ref->prevInBin = &mBinArray[base + insertX];
                ref->nextInObj = NULL;

                if (mBinArray[base + insertX].nextInBin)
                    mBinArray[base + insertX].nextInBin->prevInBin = ref;
                mBinArray[base + insertX].nextInBin = ref;

                *pCurrInsert = ref;
                pCurrInsert = &ref->nextInObj;
            }
        }
    }
    else
    {
        SceneObjectRef* ref = allocateObjectRef();

        ref->object = obj;
        ref->nextInBin = mOverflowBin.nextInBin;
        ref->prevInBin = &mOverflowBin;
        ref->nextInObj = NULL;

        if (mOverflowBin.nextInBin)
            mOverflowBin.nextInBin->prevInBin = ref;
        mOverflowBin.nextInBin = ref;

        obj->mBinRefHead = ref;
    }
}

void Container::insertIntoBins(SceneObject* obj,
    U32 minX, U32 maxX,
    U32 minY, U32 maxY)
{
    PROFILE_START(InsertBins);
    AssertFatal(obj != NULL, "No object?");

    AssertFatal(obj->mBinRefHead == NULL, "Error, already have a bin chain!");
    // Store the current regions for later queries
    obj->mBinMinX = minX;
    obj->mBinMaxX = maxX;
    obj->mBinMinY = minY;
    obj->mBinMaxY = maxY;

    // For huge objects, dump them into the overflow bin.  Otherwise, everything
    //  goes into the grid...
    //
    if ((maxX - minX + 1) < csmNumBins || (maxY - minY + 1) < csmNumBins && !obj->isGlobalBounds())
    {
        SceneObjectRef** pCurrInsert = &obj->mBinRefHead;

        for (U32 i = minY; i <= maxY; i++)
        {
            U32 insertY = i % csmNumBins;
            U32 base = insertY * csmNumBins;
            for (U32 j = minX; j <= maxX; j++)
            {
                U32 insertX = j % csmNumBins;

                SceneObjectRef* ref = allocateObjectRef();

                ref->object = obj;
                ref->nextInBin = mBinArray[base + insertX].nextInBin;
                ref->prevInBin = &mBinArray[base + insertX];
                ref->nextInObj = NULL;

                if (mBinArray[base + insertX].nextInBin)
                    mBinArray[base + insertX].nextInBin->prevInBin = ref;
                mBinArray[base + insertX].nextInBin = ref;

                *pCurrInsert = ref;
                pCurrInsert = &ref->nextInObj;
            }
        }
    }
    else
    {
        SceneObjectRef* ref = allocateObjectRef();

        ref->object = obj;
        ref->nextInBin = mOverflowBin.nextInBin;
        ref->prevInBin = &mOverflowBin;
        ref->nextInObj = NULL;

        if (mOverflowBin.nextInBin)
            mOverflowBin.nextInBin->prevInBin = ref;
        mOverflowBin.nextInBin = ref;
        obj->mBinRefHead = ref;
    }
    PROFILE_END();
}

void Container::removeFromBins(SceneObject* obj)
{
    PROFILE_START(RemoveFromBins);
    AssertFatal(obj != NULL, "No object?");

    SceneObjectRef* chain = obj->mBinRefHead;
    obj->mBinRefHead = NULL;

    while (chain)
    {
        SceneObjectRef* trash = chain;
        chain = chain->nextInObj;

        AssertFatal(trash->prevInBin != NULL, "Error, must have a previous entry in the bin!");
        if (trash->nextInBin)
            trash->nextInBin->prevInBin = trash->prevInBin;
        trash->prevInBin->nextInBin = trash->nextInBin;

        freeObjectRef(trash);
    }
    PROFILE_END();
}


void Container::checkBins(SceneObject* obj)
{
    AssertFatal(obj != NULL, "No object?");

    PROFILE_START(CheckBins);
    if (obj->mBinRefHead == NULL)
    {
        insertIntoBins(obj);
        PROFILE_END();
        return;
    }

    // Otherwise, the object is already in the bins.  Let's see if it has strayed out of
    //  the bins that it's currently in...
    const Box3F* pWBox = &obj->getWorldBox();

    U32 minX, maxX, minY, maxY;
    getBinRange(pWBox->min.x, pWBox->max.x, minX, maxX);
    getBinRange(pWBox->min.y, pWBox->max.y, minY, maxY);

    if (obj->mBinMinX != minX || obj->mBinMaxX != maxX ||
        obj->mBinMinY != minY || obj->mBinMaxY != maxY)
    {
        // We have to rebin the object
        removeFromBins(obj);
        insertIntoBins(obj, minX, maxX, minY, maxY);
    }
    PROFILE_END();
}


void Container::findObjects(const Box3F& box, U32 mask, FindCallback callback, void* key)
{
    PROFILE_START(ContainerFindObjects);
    U32 minX, maxX, minY, maxY;
    getBinRange(box.min.x, box.max.x, minX, maxX);
    getBinRange(box.min.y, box.max.y, minY, maxY);
    smCurrSeqKey++;
    for (U32 i = minY; i <= maxY; i++)
    {
        U32 insertY = i % csmNumBins;
        U32 base = insertY * csmNumBins;
        for (U32 j = minX; j <= maxX; j++)
        {
            U32 insertX = j % csmNumBins;

            SceneObjectRef* chain = mBinArray[base + insertX].nextInBin;
            while (chain)
            {
                if (chain->object->getContainerSeqKey() != smCurrSeqKey)
                {
                    chain->object->setContainerSeqKey(smCurrSeqKey);

                    if ((chain->object->getType() & mask) != 0 &&
                        chain->object->isCollisionEnabled() && !chain->object->isHidden())
                    {
                        if (chain->object->getWorldBox().isOverlapped(box) || chain->object->isGlobalBounds())
                        {
                            (*callback)(chain->object, key);
                        }
                    }
                }
                chain = chain->nextInBin;
            }
        }
    }
    SceneObjectRef* chain = mOverflowBin.nextInBin;
    while (chain)
    {
        if (chain->object->getContainerSeqKey() != smCurrSeqKey)
        {
            chain->object->setContainerSeqKey(smCurrSeqKey);

            if ((chain->object->getType() & mask) != 0 &&
                chain->object->isCollisionEnabled() && !chain->object->isHidden())
            {
                if (chain->object->getWorldBox().isOverlapped(box) || chain->object->isGlobalBounds())
                {
                    (*callback)(chain->object, key);
                }
            }
        }
        chain = chain->nextInBin;
    }
    PROFILE_END();
}


void Container::polyhedronFindObjects(const Polyhedron& polyhedron, U32 mask, FindCallback callback, void* key)
{
    U32 i;
    Box3F box;
    box.min.set(1e9, 1e9, 1e9);
    box.max.set(-1e9, -1e9, -1e9);
    for (i = 0; i < polyhedron.pointList.size(); i++)
    {
        box.min.setMin(polyhedron.pointList[i]);
        box.max.setMax(polyhedron.pointList[i]);
    }

    U32 minX, maxX, minY, maxY;
    getBinRange(box.min.x, box.max.x, minX, maxX);
    getBinRange(box.min.y, box.max.y, minY, maxY);
    smCurrSeqKey++;
    for (i = minY; i <= maxY; i++)
    {
        U32 insertY = i % csmNumBins;
        U32 base = insertY * csmNumBins;
        for (U32 j = minX; j <= maxX; j++)
        {
            U32 insertX = j % csmNumBins;

            SceneObjectRef* chain = mBinArray[base + insertX].nextInBin;
            while (chain)
            {
                if (chain->object->getContainerSeqKey() != smCurrSeqKey)
                {
                    chain->object->setContainerSeqKey(smCurrSeqKey);

                    if ((chain->object->getType() & mask) != 0 &&
                        chain->object->isCollisionEnabled())
                    {
                        if (chain->object->getWorldBox().isOverlapped(box) || chain->object->isGlobalBounds())
                        {
                            (*callback)(chain->object, key);
                        }
                    }
                }
                chain = chain->nextInBin;
            }
        }
    }
    SceneObjectRef* chain = mOverflowBin.nextInBin;
    while (chain)
    {
        if (chain->object->getContainerSeqKey() != smCurrSeqKey)
        {
            chain->object->setContainerSeqKey(smCurrSeqKey);

            if ((chain->object->getType() & mask) != 0 &&
                chain->object->isCollisionEnabled())
            {
                if (chain->object->getWorldBox().isOverlapped(box) || chain->object->isGlobalBounds())
                {
                    (*callback)(chain->object, key);
                }
            }
        }
        chain = chain->nextInBin;
    }
}


//----------------------------------------------------------------------------
// DMMNOTE: There are still some optimizations to be done here.  In particular:
//           - After checking the overflow bin, we can potentially shorten the line
//             that we rasterize against the grid if there is a collision with say,
//             the terrain.
//           - The optimal grid size isn't necessarily what we have set here. possibly
//             a resolution of 16 meters would give better results
//           - The line rasterizer is pretty lame.  Unfortunately we can't use a
//             simple bres. here, since we need to check every grid element that the line
//             passes through, which bres does _not_ do for us.  Possibly there's a
//             rasterizer for anti-aliased lines that will serve better than what
//             we have below.
//
bool Container::castRay(const Point3F& start, const Point3F& end, U32 mask, RayInfo* info)
{
    PROFILE_START(ContainerCastRay);
    F32 currentT = 2.0;
    smCurrSeqKey++;

    SceneObjectRef* chain = mOverflowBin.nextInBin;
    while (chain)
    {
        SceneObject* ptr = chain->object;
        if (ptr->getContainerSeqKey() != smCurrSeqKey)
        {
            ptr->setContainerSeqKey(smCurrSeqKey);

            // In the overflow bin, the world box is always going to intersect the line,
            //  so we can omit that test...
            if ((ptr->getType() & mask) != 0 &&
                ptr->isCollisionEnabled() == true)
            {
                Point3F xformedStart, xformedEnd;
                ptr->mWorldToObj.mulP(start, &xformedStart);
                ptr->mWorldToObj.mulP(end, &xformedEnd);
                xformedStart.convolveInverse(ptr->mObjScale);
                xformedEnd.convolveInverse(ptr->mObjScale);

                RayInfo ri;
                if (ptr->castRay(xformedStart, xformedEnd, &ri))
                {
                    if (ri.t < currentT)
                    {
                        *info = ri;
                        info->point.interpolate(start, end, info->t);
                        currentT = ri.t;
                    }
                }
            }
        }
        chain = chain->nextInBin;
    }

    // These are just for rasterizing the line against the grid.  We want the x coord
    //  of the start to be <= the x coord of the end
    Point3F normalStart, normalEnd;
    if (start.x <= end.x)
    {
        normalStart = start;
        normalEnd = end;
    }
    else
    {
        normalStart = end;
        normalEnd = start;
    }

    // Ok, let's scan the grids.  The simplest way to do this will be to scan across in
    //  x, finding the y range for each affected bin...
    U32 minX, maxX;
    U32 minY, maxY;
    //if (normalStart.x == normalEnd.x)
    //   Con::printf("X start = %g, end = %g", normalStart.x, normalEnd.x);

    getBinRange(normalStart.x, normalEnd.x, minX, maxX);
    getBinRange(getMin(normalStart.y, normalEnd.y),
        getMax(normalStart.y, normalEnd.y), minY, maxY);

    //if (normalStart.x == normalEnd.x && minX != maxX)
    //   Con::printf("X min = %d, max = %d", minX, maxX);
    //if (normalStart.y == normalEnd.y && minY != maxY)
    //   Con::printf("Y min = %d, max = %d", minY, maxY);

       // We'll optimize the case that the line is contained in one bin row or column, which
       //  will be quite a few lines.  No sense doing more work than we have to...
       //
    if ((mFabs(normalStart.x - normalEnd.x) < csmTotalBinSize && minX == maxX) ||
        (mFabs(normalStart.y - normalEnd.y) < csmTotalBinSize && minY == maxY))
    {
        U32 count;
        U32 incX, incY;
        if (minX == maxX)
        {
            count = maxY - minY + 1;
            incX = 0;
            incY = 1;
        }
        else
        {
            count = maxX - minX + 1;
            incX = 1;
            incY = 0;
        }

        U32 x = minX;
        U32 y = minY;
        for (U32 i = 0; i < count; i++)
        {
            U32 checkX = x % csmNumBins;
            U32 checkY = y % csmNumBins;

            SceneObjectRef* chain = mBinArray[(checkY * csmNumBins) + checkX].nextInBin;
            while (chain)
            {
                SceneObject* ptr = chain->object;
                if (ptr->getContainerSeqKey() != smCurrSeqKey)
                {
                    ptr->setContainerSeqKey(smCurrSeqKey);

                    if ((ptr->getType() & mask) != 0 &&
                        ptr->isCollisionEnabled() == true)
                    {
                        if (ptr->getWorldBox().collideLine(start, end) || chain->object->isGlobalBounds())
                        {
                            Point3F xformedStart, xformedEnd;
                            ptr->mWorldToObj.mulP(start, &xformedStart);
                            ptr->mWorldToObj.mulP(end, &xformedEnd);
                            xformedStart.convolveInverse(ptr->mObjScale);
                            xformedEnd.convolveInverse(ptr->mObjScale);

                            RayInfo ri;
                            if (ptr->castRay(xformedStart, xformedEnd, &ri))
                            {
                                if (ri.t < currentT)
                                {
                                    *info = ri;
                                    info->point.interpolate(start, end, info->t);
                                    currentT = ri.t;
                                }
                            }
                        }
                    }
                }
                chain = chain->nextInBin;
            }

            x += incX;
            y += incY;
        }
    }
    else
    {
        // Oh well, let's earn our keep.  We know that after the above conditional, we're
        //  going to cross at least one boundary, so that simplifies our job...

        F32 currStartX = normalStart.x;

        AssertFatal(currStartX != normalEnd.x, "This is going to cause problems in Container::castRay");
        while (currStartX != normalEnd.x)
        {
            F32 currEndX = getMin(currStartX + csmTotalBinSize, normalEnd.x);

            F32 currStartT = (currStartX - normalStart.x) / (normalEnd.x - normalStart.x);
            F32 currEndT = (currEndX - normalStart.x) / (normalEnd.x - normalStart.x);

            F32 y1 = normalStart.y + (normalEnd.y - normalStart.y) * currStartT;
            F32 y2 = normalStart.y + (normalEnd.y - normalStart.y) * currEndT;

            U32 subMinX, subMaxX;
            getBinRange(currStartX, currEndX, subMinX, subMaxX);

            F32 subStartX = currStartX;
            F32 subEndX = currStartX;

            if (currStartX < 0.0f)
                subEndX -= mFmod(subEndX, csmBinSize);
            else
                subEndX += (csmBinSize - mFmod(subEndX, csmBinSize));

            for (U32 currXBin = subMinX; currXBin <= subMaxX; currXBin++)
            {
                U32 checkX = currXBin % csmNumBins;

                F32 subStartT = (subStartX - currStartX) / (currEndX - currStartX);
                F32 subEndT = getMin(F32((subEndX - currStartX) / (currEndX - currStartX)), 1.f);

                F32 subY1 = y1 + (y2 - y1) * subStartT;
                F32 subY2 = y1 + (y2 - y1) * subEndT;

                U32 newMinY, newMaxY;
                getBinRange(getMin(subY1, subY2), getMax(subY1, subY2), newMinY, newMaxY);

                for (U32 i = newMinY; i <= newMaxY; i++)
                {
                    U32 checkY = i % csmNumBins;

                    SceneObjectRef* chain = mBinArray[(checkY * csmNumBins) + checkX].nextInBin;
                    while (chain)
                    {
                        SceneObject* ptr = chain->object;
                        if (ptr->getContainerSeqKey() != smCurrSeqKey)
                        {
                            ptr->setContainerSeqKey(smCurrSeqKey);

                            if ((ptr->getType() & mask) != 0 &&
                                ptr->isCollisionEnabled() == true)
                            {
                                if (ptr->getWorldBox().collideLine(start, end))
                                {
                                    Point3F xformedStart, xformedEnd;
                                    ptr->mWorldToObj.mulP(start, &xformedStart);
                                    ptr->mWorldToObj.mulP(end, &xformedEnd);
                                    xformedStart.convolveInverse(ptr->mObjScale);
                                    xformedEnd.convolveInverse(ptr->mObjScale);

                                    RayInfo ri;
                                    if (ptr->castRay(xformedStart, xformedEnd, &ri))
                                    {
                                        if (ri.t < currentT)
                                        {
                                            *info = ri;
                                            info->point.interpolate(start, end, info->t);
                                            currentT = ri.t;
                                        }
                                    }
                                }
                            }
                        }
                        chain = chain->nextInBin;
                    }
                }

                subStartX = subEndX;
                subEndX = getMin(subEndX + csmBinSize, currEndX);
            }

            currStartX = currEndX;
        }
    }

    // Bump the normal into worldspace if appropriate.
    if (currentT != 2)
    {
        PlaneF fakePlane;
        fakePlane.x = info->normal.x;
        fakePlane.y = info->normal.y;
        fakePlane.z = info->normal.z;
        fakePlane.d = 0;

        PlaneF result;
        mTransformPlane(info->object->getTransform(), info->object->getScale(), fakePlane, &result);
        info->normal = result;

        PROFILE_END();
        return true;
    }
    else
    {
        // Do nothing and exit...
        PROFILE_END();
        return false;
    }

}

// collide with the objects projected object box
bool Container::collideBox(const Point3F& start, const Point3F& end, U32 mask, RayInfo* info)
{
    F32 currentT = 2;
    for (Link* itr = mStart.next; itr != &mEnd; itr = itr->next)
    {
        SceneObject* ptr = static_cast<SceneObject*>(itr);
        if (ptr->getType() & mask && !ptr->mCollisionCount)
        {
            Point3F xformedStart, xformedEnd;
            ptr->mWorldToObj.mulP(start, &xformedStart);
            ptr->mWorldToObj.mulP(end, &xformedEnd);
            xformedStart.convolveInverse(ptr->mObjScale);
            xformedEnd.convolveInverse(ptr->mObjScale);

            RayInfo ri;
            if (ptr->collideBox(xformedStart, xformedEnd, &ri))
            {
                if (ri.t < currentT)
                {
                    *info = ri;
                    info->point.interpolate(start, end, info->t);
                    currentT = ri.t;
                }
            }
        }
    }
    return currentT != 2;
}

//----------------------------------------------------------------------------

static void buildCallback(SceneObject* object, void* key)
{
    Container::CallbackInfo* info = reinterpret_cast<Container::CallbackInfo*>(key);
    object->buildPolyList(info->polyList, info->boundingBox, info->boundingSphere);
}


//----------------------------------------------------------------------------

bool Container::buildPolyList(const Box3F& box, U32 mask, AbstractPolyList* polyList, FindCallback callback, void* key)
{
    CallbackInfo info;
    info.boundingBox = box;
    info.polyList = polyList;
    info.key = key;

    // Build bounding sphere
    info.boundingSphere.center = (info.boundingBox.min + info.boundingBox.max) * 0.5;
    VectorF bv = box.max - info.boundingSphere.center;
    info.boundingSphere.radius = bv.len();

    sPolyList = polyList;
    findObjects(box, mask, callback ? callback : buildCallback, &info);
    return !polyList->isEmpty();
}


//----------------------------------------------------------------------------

bool Container::buildCollisionList(const Box3F& box,
    const Point3F& start,
    const Point3F& end,
    const VectorF& velocity,
    U32            mask,
    CollisionList* collisionList,
    FindCallback   callback,
    void* key,
    const Box3F* queryExpansion)
{
    VectorF vector = end - start;
    if (mFabs(vector.x) + mFabs(vector.y) + mFabs(vector.z) == 0)
        return false;
    CallbackInfo info;
    info.key = key;

    // Polylist bounding box & sphere
    info.boundingBox.min = info.boundingBox.max = start;
    info.boundingBox.min.setMin(end);
    info.boundingBox.max.setMax(end);
    info.boundingBox.min += box.min;
    info.boundingBox.max += box.max;
    info.boundingSphere.center = (info.boundingBox.min + info.boundingBox.max) * 0.5;
    VectorF bv = info.boundingBox.max - info.boundingSphere.center;
    info.boundingSphere.radius = bv.len();

    // Box polyhedron edges & planes normals are always the same,
    // just need to fill in the vertices and plane.d
    Point3F* point = &sBoxPolyhedron.pointList[0];
    point[0].x = point[1].x = point[4].x = point[5].x = box.min.x + start.x;
    point[0].y = point[3].y = point[4].y = point[7].y = box.min.y + start.y;
    point[2].x = point[3].x = point[6].x = point[7].x = box.max.x + start.x;
    point[1].y = point[2].y = point[5].y = point[6].y = box.max.y + start.y;
    point[0].z = point[1].z = point[2].z = point[3].z = box.min.z + start.z;
    point[4].z = point[5].z = point[6].z = point[7].z = box.max.z + start.z;

    PlaneF* plane = &sBoxPolyhedron.planeList[0];
    plane[0].d = point[0].x;
    plane[3].d = point[0].y;
    plane[4].d = point[0].z;
    plane[1].d = -point[6].y;
    plane[2].d = -point[6].x;
    plane[5].d = -point[6].z;

    // Extruded
    sExtrudedPolyList.extrude(sBoxPolyhedron, vector);
    sExtrudedPolyList.setVelocity(velocity);
    sExtrudedPolyList.setCollisionList(collisionList);
    if (velocity.isZero())
    {
        sExtrudedPolyList.clearInterestNormal();
    }
    else
    {
        Point3F normVec = velocity;
        normVec.normalize();
        sExtrudedPolyList.setInterestNormal(normVec);
    }
    info.polyList = &sExtrudedPolyList;

    // Optional expansion of the query box
    Box3F queryBox = info.boundingBox;
    if (queryExpansion)
    {
        queryBox.min += queryExpansion->min;
        queryBox.max += queryExpansion->max;
    }

    // Query main database
    findObjects(queryBox, mask, callback ? callback : buildCallback, &info);
    sExtrudedPolyList.adjustCollisionTime();
    return collisionList->count != 0;
}


//----------------------------------------------------------------------------

bool Container::buildCollisionList(const Polyhedron& polyhedron,
    const Point3F& start, const Point3F& end,
    const VectorF& velocity,
    U32 mask, CollisionList* collisionList,
    FindCallback callback, void* key)
{
    VectorF vector = end - start;
    if (mFabs(vector.x) + mFabs(vector.y) + mFabs(vector.z) == 0)
        return false;

    CallbackInfo info;
    info.key = key;

    // Polylist bounding box & sphere
    Point3F min(1e10, 1e10, 1e10);
    Point3F max(-1e10, -1e10, -1e10);
    for (U32 i = 0; i < polyhedron.pointList.size(); i++)
    {
        min.setMin(polyhedron.pointList[i]);
        max.setMax(polyhedron.pointList[i]);
    }

    info.boundingBox.min = info.boundingBox.max = Point3F(0, 0, 0);
    info.boundingBox.min.setMin(vector);
    info.boundingBox.max.setMax(vector);
    info.boundingBox.min += min;
    info.boundingBox.max += max;
    info.boundingSphere.center = (info.boundingBox.min + info.boundingBox.max) * 0.5;
    VectorF bv = info.boundingBox.max - info.boundingSphere.center;
    info.boundingSphere.radius = bv.len();

    // Extruded
    sExtrudedPolyList.extrude(polyhedron, vector);
    sExtrudedPolyList.setVelocity(velocity);
    if (velocity.isZero())
    {
        sExtrudedPolyList.clearInterestNormal();
    }
    else
    {
        Point3F normVec = velocity;
        normVec.normalize();
        sExtrudedPolyList.setInterestNormal(normVec);
    }
    sExtrudedPolyList.setCollisionList(collisionList);
    info.polyList = &sExtrudedPolyList;

    Box3F queryBox = info.boundingBox;

    // Query main database
    findObjects(queryBox, mask, callback ? callback : buildCallback, &info);
    sExtrudedPolyList.adjustCollisionTime();
    return collisionList->count != 0;
}


void Container::cleanupSearchVectors()
{
    for (U32 i = 0; i < mSearchList.size(); i++)
        delete mSearchList[i];
    mSearchList.clear();
    mCurrSearchPos = -1;
}


static Point3F sgSortReferencePoint;
int QSORT_CALLBACK cmpSearchPointers(const void* inP1, const void* inP2)
{
    SimObjectPtr<SceneObject>** p1 = (SimObjectPtr<SceneObject>**)inP1;
    SimObjectPtr<SceneObject>** p2 = (SimObjectPtr<SceneObject>**)inP2;

    Point3F temp;
    F32 d1, d2;

    if (bool(**p1))
    {
        (**p1)->getWorldBox().getCenter(&temp);
        d1 = (temp - sgSortReferencePoint).len();
    }
    else
    {
        d1 = 0;
    }
    if (bool(**p2))
    {
        (**p2)->getWorldBox().getCenter(&temp);
        d2 = (temp - sgSortReferencePoint).len();
    }
    else
    {
        d2 = 0;
    }

    if (d1 > d2)
        return 1;
    else if (d1 < d2)
        return -1;
    else
        return 0;
}

void Container::initRadiusSearch(const Point3F& searchPoint,
    const F32      searchRadius,
    const U32      searchMask)
{
    AssertFatal(this == &gServerContainer, "Abort.  Searches only allowed on server container");
    cleanupSearchVectors();

    mSearchReferencePoint = searchPoint;

    Box3F queryBox(searchPoint, searchPoint);
    queryBox.min -= Point3F(searchRadius, searchRadius, searchRadius);
    queryBox.max += Point3F(searchRadius, searchRadius, searchRadius);

    SimpleQueryList queryList;
    findObjects(queryBox, searchMask, SimpleQueryList::insertionCallback, &queryList);

    F32 radiusSquared = searchRadius * searchRadius;

    const F32* pPoint = &searchPoint.x;
    for (U32 i = 0; i < queryList.mList.size(); i++)
    {
        const F32* bMins;
        const F32* bMaxs;
        bMins = &queryList.mList[i]->getWorldBox().min.x;
        bMaxs = &queryList.mList[i]->getWorldBox().max.x;
        F32 sum = 0;
        for (U32 j = 0; j < 3; j++)
        {
            if (pPoint[j] < bMins[j])
                sum += (pPoint[j] - bMins[j]) * (pPoint[j] - bMins[j]);
            else if (pPoint[j] > bMaxs[j])
                sum += (pPoint[j] - bMaxs[j]) * (pPoint[j] - bMaxs[j]);
        }
        if (sum < radiusSquared || queryList.mList[i]->isGlobalBounds())
        {
            mSearchList.push_back(new SimObjectPtr<SceneObject>);
            *(mSearchList.last()) = queryList.mList[i];
        }
    }
    if (mSearchList.size() != 0)
    {
        sgSortReferencePoint = mSearchReferencePoint;
        dQsort(mSearchList.address(), mSearchList.size(),
            sizeof(SimObjectPtr<SceneObject>*), cmpSearchPointers);
    }
}

U32 Container::containerSearchNext()
{
    AssertFatal(this == &gServerContainer, "Abort.  Searches only allowed on server container");

    if (mCurrSearchPos >= mSearchList.size())
        return 0;

    mCurrSearchPos++;
    while (mCurrSearchPos < mSearchList.size() && bool(*mSearchList[mCurrSearchPos]) == false)
        mCurrSearchPos++;

    if (mCurrSearchPos == mSearchList.size())
        return 0;

    return (*mSearchList[mCurrSearchPos])->getId();
}


F32 Container::containerSearchCurrDist()
{
    AssertFatal(this == &gServerContainer, "Abort.  Searches only allowed on server container");
    AssertFatal(mCurrSearchPos != -1, "Error, must call containerSearchNext before containerSearchCurrDist");

    if (mCurrSearchPos == -1 || mCurrSearchPos >= mSearchList.size() ||
        bool(*mSearchList[mCurrSearchPos]) == false)
        return 0.0;

    Point3F pos;
    (*mSearchList[mCurrSearchPos])->getWorldBox().getCenter(&pos);
    return (pos - mSearchReferencePoint).len();
}

F32 Container::containerSearchCurrRadiusDist()
{
    AssertFatal(this == &gServerContainer, "Abort.  Searches only allowed on server container");
    AssertFatal(mCurrSearchPos != -1, "Error, must call containerSearchNext before containerSearchCurrDist");

    if (mCurrSearchPos == -1 || mCurrSearchPos >= mSearchList.size() ||
        bool(*mSearchList[mCurrSearchPos]) == false)
        return 0.0;

    Point3F pos;
    (*mSearchList[mCurrSearchPos])->getWorldBox().getCenter(&pos);

    F32 dist = (pos - mSearchReferencePoint).len();

    F32 min = (*mSearchList[mCurrSearchPos])->getWorldBox().len_x();
    if ((*mSearchList[mCurrSearchPos])->getWorldBox().len_y() < min)
        min = (*mSearchList[mCurrSearchPos])->getWorldBox().len_y();
    if ((*mSearchList[mCurrSearchPos])->getWorldBox().len_z() < min)
        min = (*mSearchList[mCurrSearchPos])->getWorldBox().len_z();

    dist -= min;
    if (dist < 0)
        dist = 0;

    return dist;
}

//----------------------------------------------------------------------------
void SimpleQueryList::insertionCallback(SceneObject* obj, void* key)
{
    SimpleQueryList* pList = (SimpleQueryList*)key;
    pList->insertObject(obj);
}

Point3F SceneObject::getPosition() const
{
    Point3F pos;
    mObjToWorld.getColumn(3, &pos);
    return pos;
}

Point3F SceneObject::getRenderPosition() const
{
    Point3F pos;
    mRenderObjToWorld.getColumn(3, &pos);
    return pos;
}

void SceneObject::setPosition(const Point3F& pos)
{
    MatrixF xform = mObjToWorld;
    xform.setColumn(3, pos);
    setTransform(xform);
}

void SceneObject::setHidden(bool hidden)
{
    if (hidden != mHidden) {
        mHidden = hidden;
        setMaskBits(HiddenMask);
    }
}

//--------------------------------------------------------------------------
SceneObject::LightingInfo::LightingInfo()
{
    mUseInfo = false;
    mDirty = false;
    mHasLastColor = false;

    // set the colors to half white for invalid surfaces and all...
    mDefaultColor.set(0.5f, 0.5f, 0.5f);
    //mAlarmColor.set(0.5f, 0.5f, 0.5f);
    mLastColor.set(0.5f, 0.5f, 0.5f);

    mLastTime = 0;
}

//--------------------------------------------------------------------------
void SceneObject::installLights()
{
    /*
       // install the lights:
       LightManager * lightManager = gClientSceneGraph->getLightManager();
       AssertFatal(lightManager!=NULL, "SceneObject::installLights: LightManager not found");

       ColorF ambientColor;
       if(getLightingAmbientColor(&ambientColor))
       {
          switch(mLightingInfo.mLightSource)
          {
             case LightingInfo::Interior:
             {
                // ambient/directional contributions
                const F32 directionalFactor = 0.3f;
                const F32 ambientFactor = 0.7f;

                LightInfo & light = mLightingInfo.smAmbientLight;

                light.mType = LightInfo::Ambient;
                light.mDirection = VectorF(0.57735f, 0.57735f, -0.57735f);
                light.mColor = ambientColor * directionalFactor;
                light.mAmbient = ambientColor * ambientFactor;

                lightManager->addLight(&mLightingInfo.smAmbientLight);
                lightManager->setVectorLightsEnabled(false);

                break;
             }

             case LightingInfo::Terrain:
             {
                F32 factor = mClampF((ambientColor.red + ambientColor.green + ambientColor.blue) / 3.f, 0.f, 1.f);
                lightManager->setVectorLightsAttenuation(factor);
                break;
             }
          }
       }
       lightManager->installGLLights(getRenderWorldBox());
    */
}

void SceneObject::uninstallLights()
{
    /*
       LightManager * lightManager = gClientSceneGraph->getLightManager();
       AssertFatal(lightManager!=NULL, "SceneObject::uninstallLights: LightManager not found");

       lightManager->removeLight(&mLightingInfo.smAmbientLight);

       // resets all ambient/vector settings
       lightManager->uninstallGLLights();
    */
}

//--------------------------------------------------------------------------
// Lighting update: not used directly by sceneobject...
// - if an interior, which contains this object, moves then this value will be incorrect
bool SceneObject::getLightingAmbientColor(ColorF* col)
{
#if YOU_ARE_INSANE
    AssertFatal(col != NULL, "SceneObject::getLightingAmbientColor: invalid color ptr");

    const F32 cRayLength = 100.f;             // down/up
    //const F32 cTerrainRayLength = 10.f;       // height above terrain for no ambient
    const F32 cColorStep = 0.2f;              // amount to add per 100ms

    PROFILE_START(GetLightingAmbientColor);

    // query a new value?
    if (mLightingInfo.mDirty)
    {
        mLightingInfo.mDirty = false;

        Point3F pos;
        getRenderWorldBox().getCenter(&pos);

        // check if shadowed:
        disableCollision();

        mLightingInfo.mUseInfo = false;
        //mLightingInfo.mInterior = 0;

        // Ambient light is determined by the surface we are standing on.
        RayInfo collision;
        if (getCurrentClientContainer()->castRay(pos, Point3F(pos.x, pos.y, pos.z - cRayLength),
            InteriorObjectType | TerrainObjectType | AtlasObjectType, &collision))
        {
            bool found = true;
            AtlasInstance2* atlas = dynamic_cast<AtlasInstance2*>(collision.object);
            InteriorInstance* interior = dynamic_cast<InteriorInstance*>(collision.object);
            TerrainBlock* terrain = dynamic_cast<TerrainBlock*>(collision.object);

            Point2F uv;
            //GFXTexHandle lightmap;
            GBitmap* lightmap = NULL;

            if (atlas)
            {
                Box3F box = atlas->getObjBox();
                atlas->getRenderWorldTransform().mulP(collision.point);
                uv.x = mClampF((collision.point.x / (box.max.x - box.min.x)), 0.0, 1.0);
                uv.y = mClampF((collision.point.y / (box.max.y - box.min.y)), 0.0, 1.0);
                lightmap = atlas->lightMapTex.getBitmap();
            }
            else if (terrain)
            {
                F32 terrainlength = (terrain->getSquareSize() * TerrainBlock::BlockSize);
                uv.x = (collision.point.x + (terrainlength * 0.5)) / terrainlength;
                uv.y = (collision.point.y + (terrainlength * 0.5)) / terrainlength;
                // similar to x = x & width...
                uv.x = uv.x - F32(U32(uv.x));
                uv.y = uv.y - F32(U32(uv.y));
                lightmap = terrain->lightMap;
            }
            else if (interior)
            {
                interior->getRenderWorldTransform().mulP(collision.point);
                Interior* detail = interior->getDetailLevel(0);
                AssertFatal((detail), "SceneObject::getLightingAmbientColor: invalid interior");
                if (collision.face < detail->getSurfaceCount())
                {
                    const Interior::Surface& surface = detail->getSurface(collision.face);
                    const Interior::TexGenPlanes& texgen = detail->getLMTexGenEQ(collision.face);

                    if (detail->getLMHandle() != LM_HANDLE(-1) && interior->getLMHandle() != LM_HANDLE(-1))
                    {
                        lightmap = gInteriorLMManager.getHandle(detail->getLMHandle(),
                            interior->getLMHandle(), detail->getNormalLMapIndex(collision.face)).getBitmap();

                        uv.x = mDot(texgen.planeX, collision.point) + texgen.planeX.d;
                        uv.y = mDot(texgen.planeY, collision.point) + texgen.planeY.d;

                        U32 size = (uv.x * F32(lightmap->getWidth()));
                        size = mClamp(size, surface.mapOffsetX, (surface.mapOffsetX + surface.mapSizeX));
                        uv.x = F32(size) / F32(lightmap->getWidth());

                        size = (uv.y * F32(lightmap->getHeight()));
                        size = mClamp(size, surface.mapOffsetY, (surface.mapOffsetY + surface.mapSizeY));
                        uv.y = F32(size) / F32(lightmap->getHeight());
                    }
                }
                else
                {
                    found = false;
                }
            }
            else
            {
                found = false;
            }

            if (found && lightmap)//((GFXTextureObject *)lightmap))
            {
                // lighting look up...
                //ColorF col = ((GFXTextureObject *)lightmap)->sampleTexel(uv);
                ColorF col = lightmap->sampleTexel(uv.x, uv.y);
                //Con::printf("Color: %f %f %f", col.red, col.green, col.blue);

                // terrain lighting is dim - look into this (same thing done in shaders)...
                if (terrain)
                    col *= 2.0;

                mLightingInfo.mDefaultColor = col;
                mLightingInfo.mUseInfo = true;
            }
        }

        enableCollision();
    }
    PROFILE_END();

    // has a value?
    if (mLightingInfo.mUseInfo)
    {
        // currently in an interior which has an alarm state?
        ColorF color;
        color = mLightingInfo.mDefaultColor;

        S32 time = Platform::getVirtualMilliseconds();

        // has a previous color?
        if (mLightingInfo.mHasLastColor)
        {
            // do each componant
            F32* pColor = const_cast<F32*>((const F32*)color);
            F32* pLastColor = const_cast<F32*>((const F32*)mLightingInfo.mLastColor);

            // cColorStep is amount added per 100ms
            F32 step = (F32(time - mLightingInfo.mLastTime) / 100.f) * cColorStep;

            for (U32 i = 0; i < 3; i++)
            {
                if (pColor[i] > pLastColor[i])
                    pColor[i] = mClampF(pLastColor[i] + step, 0.f, pColor[i]);
                else if (pColor[i] < pLastColor[i])
                    pColor[i] = mClampF(pLastColor[i] - step, pColor[i], 1.f);
            }
        }

        mLightingInfo.mHasLastColor = true;
        mLightingInfo.mLastColor = color;
        mLightingInfo.mLastTime = time;
        *col = color;
        return(true);
    }
    else
    {
        mLightingInfo.mHasLastColor = true;
        mLightingInfo.mDefaultColor = ColorF(0.5, 0.5, 0.5);
        mLightingInfo.mLastColor = mLightingInfo.mDefaultColor;
        *col = mLightingInfo.mDefaultColor;
        return true;
    }

    return(false);
#else
    *col = ColorF(1.0, 1.0, 1.0);
    return true;
#endif
}
Point3F SceneObject::getVelocity() const
{
    return Point3F(0, 0, 0);
}

void SceneObject::setVelocity(const Point3F&)
{
    // derived objects should track velocity if they want...
}

void SceneObject::findLights(const char* name, NetConnection* con)
{
    SimObject* object = Sim::findObject(name);
    if (!object)
        return;
    SimGroup* group = dynamic_cast<SimGroup*>(object);
    if (!group)
    {
        sgLightObject* light = dynamic_cast<sgLightObject*>(object);
        if (!light)
            return;

        // its a light object...
        S32 id = con->getGhostIndex(light);
        if (id > -1)
            lightIds.push_back(id);
    }
    else
    {
        // its a group object so get the contained light objects...
        for (SimObject** obj = group->begin(); obj != group->end(); obj++)
        {
            sgLightObject* light = dynamic_cast<sgLightObject*>(*obj);
            if (!light)
                continue;
            S32 id = con->getGhostIndex(light);
            if (id > -1)
                lightIds.push_back(id);
        }
    }
}

/// converts lightGroupName into a list of sgUniversalStaticLight objects.
void SceneObject::findLightGroup(NetConnection* con)
{
    AssertFatal((isServerObject()), "Client object called 'findClientLightGroup'.");

    lightIds.clear();

    if (lightGroupName && (dStrlen(lightGroupName) > 0))
    {
        char* lightname = new char[dStrlen(lightGroupName) + 1];
        dStrcpy(lightname, lightGroupName);
        char* currentname = lightname;
        char* delimiter = NULL;

        while ((delimiter = dStrchr(currentname, ';')) != NULL)
        {
            delimiter[0] = 0;

            findLights(currentname, con);

            currentname = &delimiter[1];
        }

        findLights(currentname, con);

        delete[] lightname;
    }
}

ConsoleMethod(SceneObject, setHidden, void, 3, 3, "(bool show)")
{
    object->setHidden(dAtob(argv[2]));
}

ConsoleMethod(SceneObject, isHidden, bool, 2, 2, "")
{
    return object->isHidden();
}

