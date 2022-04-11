//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "sceneGraph/sceneState.h"
#include "sceneGraph/sgUtil.h"
#include "sim/sceneObject.h"
#include "sceneGraph/sceneGraph.h"
#include "terrain/environment/sky.h"
#include "platform/profiler.h"
#include "gfx/gfxDevice.h"
#include "gfx/primBuilder.h"
#include "renderInstance/renderInstMgr.h"
#include "interior/interiorInstance.h"

// MM/JF: Added for mirrorSubObject fix.
void SceneState::setupClipPlanes(ZoneState& rState)
{
    F32 farOverNear = getFarPlane() / getNearPlane();
    Point3F farPosLeftUp = Point3F(rState.frustum[0] * farOverNear, getFarPlane(), rState.frustum[3] * farOverNear);
    Point3F farPosLeftDown = Point3F(rState.frustum[0] * farOverNear, getFarPlane(), rState.frustum[2] * farOverNear);
    Point3F farPosRightUp = Point3F(rState.frustum[1] * farOverNear, getFarPlane(), rState.frustum[3] * farOverNear);
    Point3F farPosRightDown = Point3F(rState.frustum[1] * farOverNear, getFarPlane(), rState.frustum[2] * farOverNear);
    MatrixF temp = mModelview;
    temp.inverse();
    temp.mulP(farPosLeftUp);
    temp.mulP(farPosLeftDown);
    temp.mulP(farPosRightUp);
    temp.mulP(farPosRightDown);
    sgOrientClipPlanes(&rState.clipPlanes[0], getCameraPosition(), farPosLeftUp, farPosLeftDown, farPosRightUp, farPosRightDown);
    rState.clipPlanesValid = true;
}


//--------------------------------------------------------------------------
//--------------------------------------
SceneState::SceneState(SceneState* parent,
    const U32      numZones,
    F64            left,
    F64            right,
    F64            bottom,
    F64            top,
    F64            nearPlane,
    F64            farPlane,
    RectI          viewport,
    const Point3F& camPos,
    const MatrixF& modelview,
    F32            fogDistance,
    F32            visibleDistance,
    ColorF         fogColor,
    U32            numFogVolumes,
    FogVolume* fogVolumes,
    F32            visFactor)
{
    mVisFactor = visFactor;

    mParent = parent;
    mFlipCull = false;

    mBaseZoneState.render = false;
    mBaseZoneState.clipPlanesValid = false;
    mBaseZoneState.frustum[0] = left;
    mBaseZoneState.frustum[1] = right;
    mBaseZoneState.frustum[2] = bottom;
    mBaseZoneState.frustum[3] = top;
    mBaseZoneState.viewport = viewport;
#if defined(TORQUE_DEBUG)
    // Avoid FPU exceptions in ZoneState constructors
    dMemset(mBaseZoneState.clipPlanes, 0, (sizeof mBaseZoneState.clipPlanes));
#endif

    mNearPlane = nearPlane;
    mFarPlane = farPlane;

    mModelview = modelview;
    mCamPosition = camPos;
    mFogDistance = fogDistance;
    mVisibleDistance = visibleDistance;
    mFogColor = fogColor;

    mZoneStates.setSize(numZones);
    for (U32 i = 0; i < numZones; i++)
    {
        mZoneStates[i].render = false;
        mZoneStates[i].clipPlanesValid = false;
    }

    mPortalOwner = NULL;
    mPortalIndex = 0xFFFFFFFF;

    mNumFogVolumes = numFogVolumes;
    mFogVolumes = fogVolumes;
    setupFog();

    mTerrainOverride = false;
}

SceneState::~SceneState()
{
    U32 i;
    for (i = 0; i < mSubsidiaries.size(); i++)
        delete mSubsidiaries[i];
}

void SceneState::setPortal(SceneObject* owner, const U32 index)
{
    mPortalOwner = owner;
    mPortalIndex = index;
}


void SceneState::insertTransformPortal(SceneObject* owner, U32 portalIndex,
    U32 globalZone, const Point3F& traversalStartPoint,
    const bool flipCull)
{
    mTransformPortals.increment();
    mTransformPortals.last().owner = owner;
    mTransformPortals.last().portalIndex = portalIndex;
    mTransformPortals.last().globalZone = globalZone;
    mTransformPortals.last().traverseStart = traversalStartPoint;
    mTransformPortals.last().flipCull = flipCull;
}

void SceneState::renderCurrentImages()
{
    GFX->pushWorldMatrix();

    // need to do this AFTER scene traversal, otherwise zones/portals will not
    // work correctly
    for (U32 i = 0; i < mInteriorList.size(); i++)
    {
        InteriorListElem& elem = mInteriorList[i];
        GFX->setWorldMatrix(*elem.worldXform);

        Interior* pInterior = elem.obj->getResource()->getDetailLevel(elem.detailLevel);
        pInterior->prepBatchRender(elem.obj, this);
    }

    GFX->popWorldMatrix();

    gRenderInstManager.sort();
    gRenderInstManager.render();
    gRenderInstManager.clear();

    mInteriorList.clear();



}

void SceneState::setupZoneProjection(const U32 zone)
{
    const ZoneState& rState = getZoneState(zone);
    AssertFatal(rState.render == true, "Error, should never set up a non-rendering zone!");

    /*
       glMatrixMode(GL_PROJECTION);
       glLoadIdentity();
       dglSetFrustum(rState.frustum[0], rState.frustum[1],
                     rState.frustum[2], rState.frustum[3],
                     getNearPlane(), getFarPlane());
       glMatrixMode(GL_MODELVIEW);
       dglSetViewport(rState.viewport);
    */

    /*
       GFX->setFrustum( rState.frustum[0], rState.frustum[1],
                        rState.frustum[2], rState.frustum[3],
                        getNearPlane(), getFarPlane() );

       GFX->setViewport( rState.viewport );
    */
}

void SceneState::setupObjectProjection(const SceneObject* obj)
{
    /*RectI viewport;
    F64   frustum[4] = { 1e10, -1e10, 1e10, -1e10 };

    bool init = false;
    SceneObjectRef* pWalk = obj->mZoneRefHead;
    AssertFatal(pWalk != NULL, "Error, object must exist in at least one zone to call this!");
    while (pWalk) {
        const ZoneState& rState = getZoneState(pWalk->zone);
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
    AssertFatal(init, "Error, at least one zone must be rendered here!");*/

    /*
       glMatrixMode(GL_PROJECTION);
       glLoadIdentity();
       dglSetFrustum(frustum[0], frustum[1],
                     frustum[2], frustum[3],
                     getNearPlane(), getFarPlane());
       glMatrixMode(GL_MODELVIEW);
       dglSetViewport(viewport);
    */

    /*
       GFX->setFrustum( frustum[0], frustum[1],
                        frustum[2], frustum[3],
                        getNearPlane(), getFarPlane() );

       GFX->setViewport( viewport );
    */
}

void SceneState::setupBaseProjection()
{
    //const ZoneState& rState = getBaseZoneState();

    /*
       glMatrixMode(GL_PROJECTION);
       glLoadIdentity();
       dglSetFrustum(rState.frustum[0], rState.frustum[1],
                     rState.frustum[2], rState.frustum[3],
                     getNearPlane(), getFarPlane());
       glMatrixMode(GL_MODELVIEW);
       dglSetViewport(rState.viewport);
    */

    /*
       GFX->setFrustum( rState.frustum[0], rState.frustum[1],
                        rState.frustum[2], rState.frustum[3],
                        getNearPlane(), getFarPlane() );

       GFX->setViewport( rState.viewport );
    */
}


bool SceneState::isObjectRendered(const SceneObject* obj)
{
    if (obj->mHidden)
        return false;

    const SceneObjectRef* pWalk = obj->mZoneRefHead;

    static F32 darkToOGLCoord[16] = { 1, 0,  0, 0,
                                      0, 0, -1, 0,
                                      0, 1,  0, 0,
                                      0, 0,  0, 1 };
    static MatrixF darkToOGLMatrix;
    static bool matrixInitialized = false;
    if (matrixInitialized == false)
    {
        F32* m = darkToOGLMatrix;
        for (U32 i = 0; i < 16; i++)
            m[i] = darkToOGLCoord[i];
        darkToOGLMatrix.transpose();
        matrixInitialized = true;
    }

    while (pWalk != NULL) {
        if (getZoneState(pWalk->zone).render == true)
        {
            ZoneState& rState = getZoneStateNC(pWalk->zone);
            if (rState.clipPlanesValid == false)
            {
                setupClipPlanes(rState);
            }

            const Box3F& rObjBox = obj->getObjBox();
            const Point3F& rScale = obj->getScale();

            Point3F center;
            rObjBox.getCenter(&center);
            center.convolve(rScale);

            Point3F xRad((rObjBox.max.x - rObjBox.min.x) * 0.5 * rScale.x, 0, 0);
            Point3F yRad(0, (rObjBox.max.y - rObjBox.min.y) * 0.5 * rScale.y, 0);
            Point3F zRad(0, 0, (rObjBox.max.z - rObjBox.min.z) * 0.5 * rScale.z);

            obj->getRenderTransform().mulP(center);
            obj->getRenderTransform().mulV(xRad);
            obj->getRenderTransform().mulV(yRad);
            obj->getRenderTransform().mulV(zRad);

            bool render = true;
            for (U32 i = 0; i < 5; i++) {
                if (rState.clipPlanes[i].whichSideBox(center, xRad, yRad, zRad, Point3F(0, 0, 0)) == PlaneF::Back) {
                    render = false;
                    break;
                }
            }

            if (render)
                return true;
        }

        pWalk = pWalk->nextInObj;
    }

    return false;
}

//--------------------------------------------------------------------------
//--------------------------------------

bool checkFogBandBoxVisible(F32 dist, F32 haze, F32 low, F32 high, Vector<SceneState::FogBand>& fb)
{
    // if there are no fog bands, no fog - it's visible
    if (!fb.size())
        return true;
    // if the first fog band is unfogged and the box
    // is inside the band, it's visible
    if (fb[0].isFog == false && low < fb[0].cap)
        return true;

    // check the case of the camera in a fog band
    if (fb[0].isFog)
    {
        // if the low point is in the fog, we check that

        if (low < fb[0].cap)
        {
            if (haze + dist * fb[0].factor < 1)
                return true;
            // if low and high are both in the fog band
            // and low isn't visible, neither is high
            if (high < fb[0].cap)
                return false;
            // check the high point...
            F32 highDist = mSqrt(high * high + dist * dist - low * low);
            return haze + (fb[0].cap / high) * highDist * fb[0].factor < 1;
        }
        // ok, both low and high are above the cap of the plane
        // so we have to check only the high point (bigger triangle means less fog
        // applied (higher top means steeper slope on the hypotenuse))

        F32 highDist = mSqrt(high * high + dist * dist - low * low);
        return haze + (fb[0].cap / high) * highDist * fb[0].factor < 1;
    }
    // ok, fb[0] is not fogged, meaning there is an empty layer
    // followed by a fog plane, followed by the box.

    // we only test the one fog volume for visibility of the box...
    F32 fogStart = fb[0].cap;
    F32 fogEnd = fogStart + fb[1].cap;

    // if the low is in the fog band, we have to check
    // low, followed by possibly high
    // if low is above the fog band we only have to check high point
    if (low > fogEnd)
    {
        // only check the high point through the fog
        F32 highDist = mSqrt(high * high + dist * dist - low * low);
        return haze + (fb[1].cap / high) * highDist * fb[1].factor < 1;
    }
    // last case, low is in the fog band
    // check low vis:
    if (haze + fb[1].factor * dist * (low - fogStart) / low < 1)
        return true;
    // if the high point is in the same fog band, it's not visible
    if (high < fogEnd)
        return false;
    // ok, check the high point
    F32 highDist = mSqrt(high * high + dist * dist - low * low);
    return haze + (fb[1].cap / high) * highDist * fb[1].factor < 1;
}

bool SceneState::isBoxFogVisible(F32 dist, F32 top, F32 bottom)
{
    F32 camZ = mCamPosition.z;
    if (mFlipCull)
    {
        camZ = getCurrentClientSceneGraph()->mNormCamPos.z;
    }

    float haze = 0;
    if (dist > mFogDistance) {
        float distFactor = (dist - mFogDistance) * mFogScale - 1.0;
        haze = 1.0 - distFactor * distFactor;
    }
    F32 distSq = dist * dist;
    // the object is below:
    if (top < camZ)
    {
        return checkFogBandBoxVisible(dist, haze, camZ - top, camZ - bottom, mNegFogBands);
    }
    else if (bottom > camZ)
    {
        return checkFogBandBoxVisible(dist, haze, bottom - camZ, top - camZ, mPosFogBands);
    }
    else
    {
        // spans the fog...
        if (!mNegFogBands.size() || !mPosFogBands.size() || !mPosFogBands[0].isFog)
            return true;
        // ok, we know there is at least one fog band and the camera is in it.
        // check if the object is visible through the fog...
        if (haze + dist * mPosFogBands[0].factor < 1)
            return true;

        // ok, check the top stretch...
        // we know now that since the box spans the horizontal,
        // that dist is a horizontal (deltaZ = 0)
        // so we want the segment of the hypotenuse that goes through
        // the fog.

        F32 ht = top - camZ;
        // don't do it if the top is in the fog
        if (ht > mPosFogBands[0].cap)
        {
            if (haze + (mPosFogBands[0].cap / ht) * mSqrt(dist * dist + ht * ht) * mPosFogBands[0].factor < 1)
                return true;
        }

        // ok, last chance, check the bottom segment
        ht = camZ - bottom;
        if (ht < mNegFogBands[0].cap)
            return false;
        return haze + (mNegFogBands[0].cap / ht) * mSqrt(dist * dist + ht * ht) * mNegFogBands[0].factor < 1;
    }
}

void SceneState::setupFog()
{

    if (mVisibleDistance == mFogDistance) {
        // FIXME: arbitrary large constant
        mFogScale = 1000.0f;
    }
    else {
        mFogScale = 1.0 / (mVisibleDistance - mFogDistance);
    }

    // construct positive fog volumes
    mPosFogBands.clear();

    F32 camZ = mCamPosition.z;
    if (mFlipCull)
    {
        camZ = getCurrentClientSceneGraph()->mNormCamPos.z;
    }

    S32 i;
    for (i = 0; i < mNumFogVolumes; i++)
    {
        if (camZ < mFogVolumes[i].maxHeight)
            break;
    }
    if (i < mNumFogVolumes)
    {
        float prevHeight = camZ;
        for (; i < mNumFogVolumes; i++)
        {
            if (prevHeight < mFogVolumes[i].minHeight)
            {
                FogBand fb;
                fb.isFog = false;
                fb.color.set(mFogVolumes[i].color.red,
                    mFogVolumes[i].color.green,
                    mFogVolumes[i].color.blue,
                    mFogVolumes[i].color.alpha);
                fb.cap = mFogVolumes[i].minHeight - prevHeight;
                prevHeight = mFogVolumes[i].minHeight;
                mPosFogBands.push_back(fb);
            }
            FogBand fb;
            fb.isFog = true;
            fb.cap = mFogVolumes[i].maxHeight - prevHeight;
            fb.color.set(mFogVolumes[i].color.red,
                mFogVolumes[i].color.green,
                mFogVolumes[i].color.blue,
                mFogVolumes[i].color.alpha);
            fb.factor = (1 / (mFogVolumes[i].visibleDistance * mVisFactor)) * mFogVolumes[i].percentage;
            prevHeight = mFogVolumes[i].maxHeight;
            mPosFogBands.push_back(fb);
        }
    }
    // construct negative fog volumes
    mNegFogBands.clear();
    for (i = mNumFogVolumes - 1; i >= 0; i--)
    {
        if (camZ > mFogVolumes[i].minHeight)
            break;
    }
    if (i >= 0)
    {
        float prevHeight = camZ;
        for (; i >= 0; i--)
        {
            if (prevHeight > mFogVolumes[i].maxHeight)
            {
                FogBand fb;
                fb.isFog = false;
                fb.cap = prevHeight - mFogVolumes[i].maxHeight;
                prevHeight = mFogVolumes[i].maxHeight;
                fb.color.set(mFogVolumes[i].color.red,
                    mFogVolumes[i].color.green,
                    mFogVolumes[i].color.blue,
                    mFogVolumes[i].color.alpha);
                mNegFogBands.push_back(fb);
            }
            FogBand fb;
            fb.isFog = true;
            fb.cap = prevHeight - mFogVolumes[i].minHeight;
            fb.factor = (1 / (mFogVolumes[i].visibleDistance * mVisFactor)) * mFogVolumes[i].percentage;
            prevHeight = mFogVolumes[i].minHeight;
            fb.color.set(mFogVolumes[i].color.red,
                mFogVolumes[i].color.green,
                mFogVolumes[i].color.blue,
                mFogVolumes[i].color.alpha);
            mNegFogBands.push_back(fb);
        }
    }
}

void SceneState::getFogs(float dist, float deltaZ, ColorF* array, U32& numFogs)
{
    numFogs = 0;
    Vector<FogBand>* band;
    if (deltaZ < 0)
    {
        deltaZ = -deltaZ;
        band = &mNegFogBands;
    }
    else
        band = &mPosFogBands;

    float ht = deltaZ;
    for (int i = 0; i < band->size(); i++)
    {
        FogBand& bnd = (*band)[i];

        if (ht < bnd.cap)
        {
            if (bnd.isFog)
                array[numFogs++] = ColorF(bnd.color.red, bnd.color.green, bnd.color.blue, dist * bnd.factor);
            break;
        }
        float subDist = dist * bnd.cap / ht;
        if (bnd.isFog)
        {
            array[numFogs++] = ColorF(bnd.color.red,
                bnd.color.green,
                bnd.color.blue,
                subDist * bnd.factor);
        }
        dist -= subDist;
        ht -= bnd.cap;
    }
}

F32 SceneState::getFog(float dist, float deltaZ, S32 volKey)
{
    float haze = 0;
    Vector<FogBand>* band;
    if (deltaZ < 0)
    {
        deltaZ = -deltaZ;
        band = &mNegFogBands;
    }
    else
        band = &mPosFogBands;

    if (band->size() < 1)
        return haze;

    float ht = deltaZ;
    FogBand& bnd = (*band)[volKey];

    if (ht < bnd.cap)
    {
        if (bnd.isFog)
            haze += dist * bnd.factor;
    }
    else
    {
        float subDist = dist * bnd.cap / ht;
        if (bnd.isFog)
            haze += subDist * bnd.factor;
    }

    return haze;
}

F32 SceneState::getFog(float dist, float deltaZ)
{
    float haze = 0;
    Vector<FogBand>* band;
    if (deltaZ < 0)
    {
        deltaZ = -deltaZ;
        band = &mNegFogBands;
    }
    else
        band = &mPosFogBands;

    float ht = deltaZ;
    for (int i = 0; i < band->size(); i++)
    {
        FogBand& bnd = (*band)[i];

        if (ht < bnd.cap)
        {
            if (bnd.isFog)
                haze += dist * bnd.factor;
            break;
        }
        float subDist = dist * bnd.cap / ht;
        if (bnd.isFog)
            haze += subDist * bnd.factor;
        dist -= subDist;
        ht -= bnd.cap;
    }
    return haze;
}

F32 SceneState::getHazeAndFog(float dist, float deltaZ)
{
    float haze = 0;

    if (dist > mFogDistance) {
        if (dist > mVisibleDistance)
            return 1.0;

        float distFactor = (dist - mFogDistance) * mFogScale - 1.0;
        haze = 1.0 - distFactor * distFactor;
    }

    Vector<FogBand>* band;
    if (deltaZ < 0)
    {
        deltaZ = -deltaZ;
        band = &mNegFogBands;
    }
    else
        band = &mPosFogBands;

    float ht = deltaZ;
    for (int i = 0; i < band->size(); i++)
    {
        FogBand& bnd = (*band)[i];

        if (ht < bnd.cap)
        {
            if (bnd.isFog)
                haze += dist * bnd.factor;
            break;
        }
        float subDist = dist * bnd.cap / ht;
        if (bnd.isFog)
            haze += subDist * bnd.factor;
        dist -= subDist;
        ht -= bnd.cap;
    }
    if (haze > 1)
        return 1;
    return haze;
}


