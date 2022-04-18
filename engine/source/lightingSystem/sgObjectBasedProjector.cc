//-----------------------------------------------
// Synapse Gaming - Lighting System
// Copyright � Synapse Gaming 2003
// Written by John Kabus
//-----------------------------------------------
#include "gfx/gfxDevice.h"
#include "gfx/primBuilder.h"
#include "math/mathUtils.h"
#include "game/shapeBase.h"
#include "sceneGraph/sceneGraph.h"
#include "interior/interiorInstance.h"
#ifdef TORQUE_TERRAIN
#include "atlas/runtime/atlasInstance2.h"
#endif
#include "lightingSystem/sgLighting.h"
#include "lightingSystem/sgLightingModel.h"
#include "lightingSystem/sgObjectBasedProjector.h"
#include "lightingSystem/sgObjectShadows.h"
#include "lightingSystem/sgFormatManager.h"

//extern SceneGraph* gClientSceneGraph;


Point2I sgShadowSharedZBuffer::sgSize = Point2I(0, 0);
GFXTexHandle sgShadowSharedZBuffer::sgZBuffer = NULL;


void sgShadowSharedZBuffer::sgPrepZBuffer(const Point2I& size)
{
    if ((size.x > sgSize.x) || (size.y > sgSize.y))
    {
        // recreate...
        sgZBuffer = NULL;
        sgSize = size;
        sgZBuffer = GFXTexHandle(size.x, size.y,
            sgFormatManager::sgShadowZTextureFormat, &ShadowZTargetTextureProfile);
    }
}

GFXTexHandle& sgShadowSharedZBuffer::sgGetZBuffer()
{
    return sgZBuffer;
}

void sgShadowSharedZBuffer::sgClear()
{
    sgZBuffer = NULL;
}

//-----------------------------------------------

sgObjectBasedProjector::sgObjectBasedProjector(SceneObject* parentobject)
{
    sgParentObject = parentobject;
}

sgObjectBasedProjector::~sgObjectBasedProjector()
{
}

//-----------------------------------------------

bool sgShadowProjector::sgShadow::sgSetLOD(S32 lod, Point2I size)
{
    if (lod == sgCurrentLOD)
        return false;
    sgRelease();
    sgShadowTextureCache::sgAcquire(sgShadowTexture, size, sgShadowTextureFormat);
    sgCurrentLOD = lod;
    return true;
}

void sgShadowProjector::sgShadow::sgRelease()
{
    sgCurrentLOD = -1;
    if ((GFXTextureObject*)sgShadowTexture)
        sgShadowTextureCache::sgRelease(sgShadowTexture);
}

//-----------------------------------------------

GFXTexHandle* gTexture = NULL;

sgShadowProjector::sgShadowProjector(SceneObject* parentobject,
    LightInfo* light, TSShapeInstance* shapeinstance)
    : sgObjectBasedProjector(parentobject)
{
    sgLight = light;

    for (U32 i = 0; i < sgspMaxLOD; i++)
        sgShadowLODSize[i] = Point2I(0, 0);

    sgShadowBuilderShader = NULL;
    sgShadowShader = NULL;
    Sim::findObject("ShadowBuilderShader_3_0", sgShadowBuilderShader_3_0);
    Sim::findObject("ShadowShader_3_0", sgShadowShader_3_0);
    Sim::findObject("ShadowBuilderShader_2_0", sgShadowBuilderShader_2_0);
    Sim::findObject("ShadowShader_2_0", sgShadowShader_2_0);
    Sim::findObject("ShadowBuilderShader_1_1", sgShadowBuilderShader_1_1);
    Sim::findObject("ShadowShader_1_1", sgShadowShader_1_1);
    sgShapeInstance = shapeinstance;

    sgEnable = false;
    sgCanMove = false;
    sgCanRTT = false;
    sgCanSelfShadow = false;

    sgRequestedShadowSize = 128;
    sgFrameSkip = 5;
    sgMaxVisibleDistance = 50.0f;
    sgProjectionDistance = 14.0f;
    sgSphereAdjust = 1.0;

    sgFirstMove = true;
    sgFirstRTT = true;
    sgShadowTypeDirty = true;
    sgLastFrame = 0;
    sgCachedTextureDetailSize = 0;
    sgCachedParentTransformHash = 0;
    sgPreviousShadowTime = 0;
    sgPreviousShadowLightingVector = VectorF(0, 0, -1);

    sgLastRenderTime = 0;

    // testing...
    //gTexture = &sgLastShadow;
}

void sgShadowProjector::sgClear()
{
    // free resources...
    sgShadowLODObject.sgRelease();

    for (U32 i = 0; i < sgspMaxLOD; i++)
        sgShadowLODSize[i] = Point2I(0, 0);

    sgShadowPolys.clear();
    sgShadowPoints.clear();
    sgShadowBuffer.set(GFX, 0, GFXBufferTypeVolatile);

    // setup for recalc on enable...
    sgShadowTypeDirty = true;
    sgFirstMove = true;
    sgFirstRTT = true;
}

void sgShadowProjector::sgGetVariables()
{
    ShapeBase* shape = dynamic_cast<ShapeBase*>(sgParentObject);
    if (shape)
    {
        ShapeBaseData* datablock = dynamic_cast<ShapeBaseData*>(shape->getDataBlock());
        if (datablock)
        {
            sgEnable = datablock->shadowEnable;
            sgCanMove = datablock->shadowCanMove;
            sgCanRTT = datablock->shadowCanAnimate;
            sgFrameSkip = datablock->shadowAnimationFrameSkip;
            sgMaxVisibleDistance = datablock->shadowMaxVisibleDistance;
            sgBias = datablock->shadowBias;

            sgDirtySync(sgCanSelfShadow, datablock->shadowSelfShadow);
            sgDirtySync(sgRequestedShadowSize, datablock->shadowSize);
            sgDirtySync(sgProjectionDistance, datablock->shadowProjectionDistance);
            sgDirtySync(sgSphereAdjust, datablock->shadowSphereAdjust);
        }
    }
}

void sgShadowProjector::sgSetupShadowType()
{
    if (!sgShadowTypeDirty && (sgCalculateShaderModel() == sgCachedShaderModel) &&
        (LightManager::sgDynamicShadowDetailSize == sgCachedTextureDetailSize))
        return;

    sgClear();

    sgCachedShaderModel = sgCalculateShaderModel();
    sgCachedTextureDetailSize = LightManager::sgDynamicShadowDetailSize;

    U32 size = sgGetShadowSize();
    if (sgCachedShaderModel != sgsm_1_1)
    {
        sgShadowLODObject.sgShadowTextureFormat = sgFormatManager::sgShadowTextureFormat_2_0;
        for (U32 i = 0; i < sgspMaxLOD; i++)
        {
            sgShadowLODSize[i] = Point2I(size, size);
            size = getMax((size >> 1), U32(4));
        }
        if (sgCachedShaderModel != sgsm_2_0)
        {
            sgShadowBuilderShader = sgShadowBuilderShader_3_0;
            sgShadowShader = sgShadowShader_3_0;
        }
        else
        {
            sgShadowBuilderShader = sgShadowBuilderShader_2_0;
            sgShadowShader = sgShadowShader_2_0;
        }
    }
    else
    {
        sgShadowLODObject.sgShadowTextureFormat = sgFormatManager::sgShadowTextureFormat_1_1;
        for (U32 i = 0; i < sgspMaxLOD; i++)
        {
            sgShadowLODSize[i] = Point2I(size, size);
            size = getMax((size >> 1), U32(4));
        }
        sgShadowBuilderShader = sgShadowBuilderShader_1_1;
        sgShadowShader = sgShadowShader_1_1;
    }

    sgShadowSharedZBuffer::sgPrepZBuffer(Point2I(sgGetShadowSize(), sgGetShadowSize()));

    sgShadowTypeDirty = false;
}

sgShadowProjector::sgShaderModel sgShadowProjector::sgCalculateShaderModel()
{
    U32 quality = LightManager::sgGetDynamicShadowQuality();

    if ((!sgCanSelfShadow) || (quality >= 2))
        return sgsm_1_1;
    if (quality >= 1)
        return sgsm_2_0;
    return sgsm_3_0;
}

void sgShadowProjector::sgGetLightSpaceBasedOnY()
{
    Point3F s, t;
    if (mFabs(sgLightVector.x) > mFabs(sgLightVector.z))
    {
        mCross(sgLightVector, Point3F(0.0, 0.0, 1.0), t);
        t.normalizeSafe();
        mCross(sgLightVector, t, s);
        s.normalizeSafe();
    }
    else
    {
        mCross(sgLightVector, Point3F(1.0, 0.0, 0.0), t);
        t.normalizeSafe();
        mCross(sgLightVector, t, s);
        s.normalizeSafe();
    }

    sgLightSpaceY.identity();
    sgLightSpaceY.setRow(0, s);
    sgLightSpaceY.setRow(1, sgLightVector);
    sgLightSpaceY.setRow(2, t);
    sgLightSpaceY.transpose();

    MatrixF world;
    world.identity();
    world.setPosition(sgParentObject->getRenderPosition());
    sgLightToWorldY.mul(world, sgLightSpaceY);
    sgWorldToLightY = sgLightToWorldY;
    sgWorldToLightY.inverse();

    MatrixF lighttoworldproj;
    world.setPosition(sgCachedParentPos);
    lighttoworldproj.mul(world, sgLightSpaceY);
    sgLightProjToLightY = sgWorldToLightY;
    sgLightProjToLightY.mul(lighttoworldproj);
    sgWorldToLightProjY = lighttoworldproj;
    sgWorldToLightProjY.inverse();
}

void sgShadowProjector::sgCalculateBoundingBox()
{
    PROFILE_START(sgShadowProjector_sgCalculateBoundingBox);

    Point3F vect = sgLight->mDirection;
    Point3F pos = sgParentObject->getRenderTransform().getPosition();

    if (LightManager::sgMultipleDynamicShadows)
    {
        if (sgLight->mType != LightInfo::Vector)
        {
            vect = (pos - sgLight->mPos);
            vect.normalizeSafe();
        }
    }
    else
    {
        vect = sgGetCompositeShadowLightDirection();
    }

    SphereF sphere = sgParentObject->getWorldSphere();
    // help for objects with boxes that are too small...
    sphere.radius *= 1.2 * sgSphereAdjust;
    pos -= vect * sphere.radius * 0.25;

    bool move = ((vect != sgLightVector) || (pos != sgCachedParentPos)) && sgCanMove;
    if (!move && !sgFirstMove)
    {
        PROFILE_END();
        return;
    }

    sgFirstMove = false;
    sgFirstRTT = true;
    sgCachedParentPos = pos;
    sgLightVector = vect;

    testRenderPoints[0] = Point3F(sphere.radius, sphere.radius + sgProjectionDistance, sphere.radius);
    testRenderPoints[1] = Point3F(-sphere.radius, sphere.radius + sgProjectionDistance, sphere.radius);
    testRenderPoints[2] = Point3F(sphere.radius, -sphere.radius, sphere.radius);
    testRenderPoints[3] = Point3F(-sphere.radius, -sphere.radius, sphere.radius);
    testRenderPoints[4] = Point3F(sphere.radius, sphere.radius + sgProjectionDistance, -sphere.radius);
    testRenderPoints[5] = Point3F(-sphere.radius, sphere.radius + sgProjectionDistance, -sphere.radius);
    testRenderPoints[6] = Point3F(sphere.radius, -sphere.radius, -sphere.radius);
    testRenderPoints[7] = Point3F(-sphere.radius, -sphere.radius, -sphere.radius);

    sgProjectionScale = 1.0 / sphere.radius;
    //sgProjectionInfo.x = (sphere.radius * 2.0) + shadowdist;
    sgProjectionInfo.x = sphere.radius + sgProjectionDistance;
    sgProjectionInfo.y = sphere.radius;
    sgProjectionInfo.z = sgProjectionScale;

    dMemcpy(((float*)testRenderPointsWorld[0]), ((float*)testRenderPoints[0]), sizeof(testRenderPoints));

    sgGetLightSpaceBasedOnY();

    for (U32 i = 0; i < 8; i++)
        sgLightToWorldY.mulP(testRenderPoints[i], &testRenderPointsWorld[i]);

    Point3F extent((2.0f * sphere.radius), sgProjectionDistance, (2.0f * sphere.radius));
    sgPolyGrinder.clear();
    sgPolyGrinder.set(sgWorldToLightProjY, extent);
    sgPolyGrinder.setInterestNormal(sgLightVector);

    // build world space box and sphere around shadow
    sgShadowBox.max.set(testRenderPointsWorld[0]);
    sgShadowBox.min.set(testRenderPointsWorld[0]);
    for (U32 i = 1; i < 8; i++)
    {
        sgShadowBox.max.setMax(testRenderPointsWorld[i]);
        sgShadowBox.min.setMin(testRenderPointsWorld[i]);
    }

    sgShadowBox.getCenter(&sgShadowSphere.center);
    sgShadowSphere.radius = Point3F(sgShadowBox.max - sgShadowSphere.center).len();


    PROFILE_START(sgShadowProjector_sgCalculateBoundingBox_find);

    getCurrentClientContainer()->findObjects((AtlasObjectType | TerrainObjectType | InteriorObjectType), sgShadowProjector::collisionCallback, this);

    sgShadowPoly[0].set(-sphere.radius, 0, -sphere.radius);
    sgShadowPoly[1].set(-sphere.radius, 0, sphere.radius);
    sgShadowPoly[2].set(sphere.radius, 0, sphere.radius);
    sgShadowPoly[3].set(sphere.radius, 0, -sphere.radius);

    PROFILE_END();
    PROFILE_START(sgShadowProjector_sgCalculateBoundingBox_Part);

    sgShadowPolys.clear();
    sgShadowPoints.clear();
    sgPolyGrinder.depthPartition(sgShadowPoly, 4, sgShadowPolys, sgShadowPoints);

    // done with this...
    sgPolyGrinder.clear();


    if (sgShadowPoints.size() < 1)
    {
        PROFILE_END();
        PROFILE_END();
        return;
    }

    sgShadowBuffer.set(GFX, sgShadowPoints.size(), GFXBufferTypeStatic);
    sgShadowBuffer.lock();

    for (U32 p = 0; p < sgShadowPolys.size(); p++)
    {
        DepthSortList::Poly& poly = sgShadowPolys[p];
        AssertFatal((poly.vertexCount >= 3), "Bad poly.");

        U32 a, b, c;
        a = poly.vertexStart;
        b = a + 1;
        c = a + 2;

        poly.plane.set(sgShadowPoints[a], sgShadowPoints[b], sgShadowPoints[c]);

        for (U32 v = 0; v < poly.vertexCount; v++)
        {
            GFXVertexPN& vb = sgShadowBuffer[a + v];
            vb.point = sgShadowPoints[a + v];
            vb.normal = poly.plane;

            // adjust vert...
            sgLightProjToLightY.mulP(vb.point);
        }
    }

    sgShadowBuffer.unlock();

    PROFILE_END();

    PROFILE_END();
}

void sgShadowProjector::collisionCallback(SceneObject* obj, void* shadow)
{
    sgShadowProjector* shadowobj = reinterpret_cast<sgShadowProjector*>(shadow);
    if (obj->getWorldBox().isOverlapped(shadowobj->sgShadowBox))
    {
        // only interiors clip...
        ClippedPolyList::allowClipping = (dynamic_cast<InteriorInstance*>(obj) != NULL);
        obj->buildPolyList(&shadowobj->sgPolyGrinder, shadowobj->sgShadowBox, shadowobj->sgShadowSphere);
        ClippedPolyList::allowClipping = true;
    }
}

void sgShadowProjector::sgRenderShape(TSShapeInstance* shapeinst, const MatrixF& trans1,
    S32 vertconstindex1, const MatrixF& trans2, S32 vertconstindex2)
{
    U32 detaillevel = shapeinst->getCurrentDetail();
    if (detaillevel == -1)
        return;

    TSShape* shape = shapeinst->getShape();
    AssertFatal((detaillevel >= 0) && (detaillevel < shape->details.size()), "TSShapeInstance::renderShadow");
    const TSDetail* detail = &shape->details[detaillevel];
    AssertFatal((detail->subShapeNum >= 0), "TSShapeInstance::renderShadow: not with a billboard detail level");

    shapeinst->setStatics(detaillevel);

    S32 s = shape->subShapeFirstObject[detail->subShapeNum];
    S32 e = s + shape->subShapeNumObjects[detail->subShapeNum];

    for (U32 i = s; i < e; i++)
    {
        TSMesh* mesh = shapeinst->mMeshObjects[i].getMesh(detail->objectDetailNum);
        if (mesh)
        {
            MatrixF* meshtrans = shapeinst->mMeshObjects[i].getTransform();
            MatrixF mat;

            // one of those hidden issues with torque...
            TSShapeInstance::smRenderData.currentObjectInstance = &shapeinst->mMeshObjects[i];

            if (meshtrans)
                mat.mul(trans1, (*meshtrans));
            else
                mat = trans1;
            mat.transpose();
            GFX->setVertexShaderConstF(vertconstindex1, mat, 4);

            if (vertconstindex2 > -1)
            {
                if (meshtrans)
                    mat.mul(trans2, (*meshtrans));
                else
                    mat = trans2;
                mat.transpose();
                GFX->setVertexShaderConstF(vertconstindex2, mat, 4);
            }

            mesh->render();
        }
    }

    shapeinst->clearStatics();
}

void sgShadowProjector::sgRenderShadowBuffer()
{
    RectI originalview = GFX->getViewport();

    GFX->pushActiveRenderTarget();
    if (mShadowBufferTarget.isNull())
    {
        mShadowBufferTarget = GFX->allocRenderToTextureTarget();
    }
    mShadowBufferTarget->attachTexture(GFXTextureTarget::Color0, sgShadowLODObject.sgShadowTexture );
    mShadowBufferTarget->attachTexture(GFXTextureTarget::DepthStencil, sgShadowSharedZBuffer::sgGetZBuffer() );
    GFX->setActiveRenderTarget( mShadowBufferTarget );

    if (sgAllowSelfShadowing())
        GFX->clear(GFXClearTarget | GFXClearZBuffer, ColorI(255, 255, 255, 255), 1.0f, 0);
    else
        GFX->clear(GFXClearTarget | GFXClearZBuffer, ColorI(0, 0, 0, 255), 1.0f, 0);

    GFX->pushWorldMatrix();
    GFX->setCullMode(GFXCullCCW);
    GFX->setLightingEnable(false);
    GFX->setAlphaBlendEnable(true);
    GFX->setZEnable(true);
    GFX->setZWriteEnable(true);
    GFX->setSrcBlend(GFXBlendOne);
    GFX->setDestBlend(GFXBlendZero);

    GFX->setTextureStageColorOp(0, GFXTOPDisable);

    sgShadowBuilderShader->getShader()->process();

    const MatrixF& world = GFX->getWorldMatrix();

    MatrixF shapeworld;
    shapeworld = sgParentObject->getRenderTransform();
    shapeworld.scale(sgParentObject->getScale());

    MatrixF lightspace;
    lightspace.mul(sgWorldToLightY, shapeworld);
    lightspace.scale(Point3F(sgProjectionScale, sgProjectionScale, sgProjectionScale));

    GFX->setVertexShaderConstF(4, sgProjectionInfo, 1);
    GFX->setPixelShaderConstF(1, Point4F(1.0, 0.0, 0.0, 0.0), 1);

    MatrixF proj;
    proj.identity();
    proj.scale(Point3F(1.0, (1.0 / sgProjectionScale), 1.0));

    MatrixF newmat;
    newmat.mul(proj, lightspace);
    sgRenderShape(sgShapeInstance, newmat, 0, newmat, -1);

    ShapeBase* shapebase = dynamic_cast<ShapeBase*>(sgParentObject);
    for (U32 i = 0; i < ShapeBase::MaxMountedImages; i++)
    {
        TSShapeInstance* mountinst = shapebase->getImageShapeInstance(i);
        if (!mountinst)
            continue;

        MatrixF shapeworld;
        shapebase->getRenderImageTransform(i, &shapeworld);
        shapeworld.scale(sgParentObject->getScale());

        lightspace.mul(sgWorldToLightY, shapeworld);
        lightspace.scale(Point3F(sgProjectionScale, sgProjectionScale, sgProjectionScale));

        Point3F pos = lightspace.getPosition();
        pos.convolve(Point3F(sgProjectionScale, sgProjectionScale, sgProjectionScale));
        lightspace.setPosition(pos);

        newmat.mul(proj, lightspace);
        sgRenderShape(mountinst, newmat, 0, newmat, -1);
    }

    GFX->popWorldMatrix();
    GFX->popActiveRenderTarget();

    GFX->setViewport(originalview);
}

bool sgShadowProjector::shouldRender(F32 camDist)
{
    sgGetVariables();

    if (camDist > sgMaxVisibleDistance)
        return false;

    if (!sgEnable || !LightManager::sgAllowDynamicShadows())
    {
        sgClear();
        return false;
    }

    sgSetupShadowType();
    sgCalculateBoundingBox();

    if (!sgShadowBuilderShader || !sgShadowBuilderShader->getShader() ||
        !sgShadowShader || !sgShadowShader->getShader())
        return false;

    F32 attn = 2.0;
    if (sgLight->mType != LightInfo::Vector)
    {
        sgLightingModel& model = sgLightingModelManager::sgGetLightingModel(sgLight->sgLightingModelName);
        model.sgSetState(sgLight);
        F32 maxrad = model.sgGetMaxRadius(true);
        model.sgResetState();

        if (maxrad <= 0.0f)
            return false;

        Point3F distvect = sgParentObject->getRenderPosition() - sgLight->mPos;
        attn = distvect.len();
        attn = 1.0 - (attn / maxrad);
        attn *= 2.0;
        if (attn <= SG_MIN_LEXEL_INTENSITY)
            return false;
    }
    return true;
}

void sgShadowProjector::render(F32 camDist)
{
    sgRender(camDist);
}

void sgShadowProjector::sgRender(F32 camdist)
{
    F32 attn = 2.0;
    if (sgLight->mType != LightInfo::Vector)
    {
        sgLightingModel& model = sgLightingModelManager::sgGetLightingModel(sgLight->sgLightingModelName);
        model.sgSetState(sgLight);
        F32 maxrad = model.sgGetMaxRadius(true);
        model.sgResetState();

        if (maxrad <= 0.0f)
            return;

        Point3F distvect = sgParentObject->getRenderPosition() - sgLight->mPos;
        attn = distvect.len();
        attn = 1.0 - (attn / maxrad);
        attn *= 2.0;
        if (attn <= SG_MIN_LEXEL_INTENSITY)
            return;
    }

    // lod info...
    F32 scaledcamdist = camdist * 0.25f;
    U32 lod = mClamp(U32(scaledcamdist), U32(0), U32(sgspMaxLOD - 1));

    if (sgMaxVisibleDistance != 0.0f)
    {
        F32 distattn = camdist / sgMaxVisibleDistance;

        // intensity...
        F32 distattnadjusted = 1.0f - distattn;
        distattnadjusted *= 3.0f;
        distattnadjusted = mClampF(distattnadjusted, 0.0f, 1.0f);
        attn *= distattnadjusted;

        // size...
        distattnadjusted = distattn * F32(sgspMaxLOD - 1);
        U32 distlod = mClamp(U32(distattnadjusted), U32(0), U32(sgspMaxLOD - 1));
        // get the best of both lod types...
        //   -dist lod sucks for short visible distances...
        //   -fixed lod falls off to quick for long distances...
        lod = getMin(lod, distlod);
    }

    bool allowlodselfshadowing = lod <= sgspLastSelfShadowLOD;


    // ok, we're going to render...
    sgLastRenderTime = Platform::getRealMilliseconds();

    PROFILE_START(sgShadowProjector_sgRender);

    U32 hash = calculateCRC((float*)sgParentObject->getRenderTransform(), (sizeof(float) * 16));
    bool frame = ((sgLastFrame >= sgFrameSkip) || (sgAllowSelfShadowing() && allowlodselfshadowing)) &&
        (sgShapeInstance->shadowDirty || (sgCachedParentTransformHash != hash));
    sgLastFrame++;
    if ((sgCanRTT && (frame || (sgShadowLODObject.sgGetLOD() != lod))) || sgFirstRTT)
    {
        sgFirstRTT = false;
        sgLastFrame = 0;
        // this breaks multiple shadows - needs to be in the ObjectShadows class...
        //sgShapeInstance->shadowDirty = false;
        sgCachedParentTransformHash = hash;

        // can't rtt, means no lod switch so use highest texture...
        if (sgCanRTT)
            sgShadowLODObject.sgSetLOD(lod, sgShadowLODSize[lod]);
        else
            sgShadowLODObject.sgSetLOD(0, sgShadowLODSize[0]);

        // testing...
        //gTexture = &sgLastShadow;

        sgRenderShadowBuffer();
    }


    const MatrixF& worldmat = GFX->getWorldMatrix();
    const MatrixF& viewmat = GFX->getViewMatrix();
    const MatrixF& projmat = GFX->getProjectionMatrix();
    const MatrixF worldtoscreen = projmat * viewmat * worldmat;
    const MatrixF lighttoscreen = worldtoscreen * sgLightToWorldY;
    MatrixF objecttolight;
    MatrixF objecttoscreen;
    MatrixF transposetemp;

    GFX->pushWorldMatrix();
    GFX->setCullMode(GFXCullNone);
    GFX->setLightingEnable(false);
    GFX->setAlphaBlendEnable(true);
    GFX->setZEnable(true);
    GFX->setZWriteEnable(false);
    GFX->setSrcBlend(GFXBlendDestColor);
    //GFX->setSrcBlend(GFXBlendOne);
    GFX->setDestBlend(GFXBlendZero);

    GFX->setAlphaTestEnable(true);
    GFX->setAlphaFunc(GFXCmpGreater);
    GFX->setAlphaRef(0);


    if (sgShadowShader == sgShadowShader_1_1)
    {
        GFX->setTextureStageColorOp(0, GFXTOPModulate);
        GFX->setTexture(0, sgShadowLODObject.sgShadowTexture);
        GFX->setTextureStageColorOp(1, GFXTOPModulate);
        GFX->setTexture(1, sgShadowLODObject.sgShadowTexture);
        GFX->setTextureStageColorOp(2, GFXTOPModulate);
        GFX->setTexture(2, sgShadowLODObject.sgShadowTexture);
        GFX->setTextureStageColorOp(3, GFXTOPModulate);
        GFX->setTexture(3, sgShadowLODObject.sgShadowTexture);
        GFX->setTextureStageColorOp(4, GFXTOPDisable);
    }
    else
    {
        GFX->setTextureStageColorOp(0, GFXTOPModulate);
        GFX->setTextureStageMagFilter(0, GFXTextureFilterPoint);
        GFX->setTextureStageMinFilter(0, GFXTextureFilterPoint);
        GFX->setTexture(0, sgShadowLODObject.sgShadowTexture);
        GFX->setTextureStageColorOp(1, GFXTOPDisable);
    }

    sgShadowShader->getShader()->process();

    F32 size = (1.0 / (sgShadowLODObject.sgShadowTexture.getWidth() - 1)) * 1.25;
    Point4F stride(size, size, sgShadowLODObject.sgShadowTexture.getWidth(),
        (1.0f / sgShadowLODObject.sgShadowTexture.getWidth()));

    GFX->setPixelShaderConstF(0, stride, 1);
    GFX->setVertexShaderConstF(12, Point4F(0.0, -1.0, 0.0, 0.0), 1);

    F32 adjustment = 0.8;
    attn = mClampF(attn, 0.0, 1.0);
    Point4F color(sgLight->mColor.red, sgLight->mColor.green, sgLight->mColor.blue, attn);
    color.x = (color.x + ((1.0 - color.x) * adjustment)) * attn;
    color.y = (color.y + ((1.0 - color.y) * adjustment)) * attn;
    color.z = (color.z + ((1.0 - color.z) * adjustment)) * attn;
    //color.x = (1.0 - ((1.0 - color.x) * adjustment)) * attn;
    //color.y = (1.0 - ((1.0 - color.y) * adjustment)) * attn;
    //color.z = (1.0 - ((1.0 - color.z) * adjustment)) * attn;
    color.x = mClampF(color.x, 0.0, 1.0);
    color.y = mClampF(color.y, 0.0, 1.0);
    color.z = mClampF(color.z, 0.0, 1.0);
    GFX->setPixelShaderConstF(3, color, 1);

    lighttoscreen.transposeTo(transposetemp);
    GFX->setVertexShaderConstF(0, transposetemp, 4);
    objecttolight.identity();
    objecttolight.transposeTo(transposetemp);
    GFX->setVertexShaderConstF(4, transposetemp, 4);
    GFX->setVertexShaderConstF(8, sgProjectionInfo, 1);
    GFX->setVertexShaderConstF(9, stride, 1);

    Point4F bias(sgBias, 0.0005, 0.0, 0.0);
    Point3F center;
    sgParentObject->getWorldBox().getCenter(&center);
    center -= sgParentObject->getWorldBox().max;
    F32 rad = center.lenSquared();
    bias.x *= rad * sgSphereAdjust * sgSphereAdjust;
    GFX->setVertexShaderConstF(10, bias, 1);

    GFX->setVertexBuffer(sgShadowBuffer);

    for (U32 p = 0; p < sgShadowPolys.size(); p++)
        GFX->drawPrimitive(GFXTriangleFan, sgShadowPolys[p].vertexStart, (sgShadowPolys[p].vertexCount - 2));

    GFX->setVertexBuffer(NULL);


    if (!sgAllowSelfShadowing() || (!allowlodselfshadowing))
    {
        GFX->setTexture(0, NULL);
        GFX->setTexture(1, NULL);
        GFX->setTexture(2, NULL);
        GFX->setTexture(3, NULL);
        GFX->setTextureStageColorOp(4, GFXTOPDisable);
        GFX->setTextureStageColorOp(3, GFXTOPDisable);
        GFX->setTextureStageColorOp(2, GFXTOPDisable);
        GFX->setTextureStageColorOp(1, GFXTOPDisable);
        GFX->setTextureStageMagFilter(0, GFXTextureFilterLinear);
        GFX->setTextureStageMinFilter(0, GFXTextureFilterLinear);
        GFX->setTextureStageColorOp(0, GFXTOPDisable);
        GFX->setZEnable(true);
        GFX->setZWriteEnable(true);
        GFX->setSrcBlend(GFXBlendOne);
        GFX->setDestBlend(GFXBlendZero);
        GFX->setAlphaTestEnable(false);
        GFX->popWorldMatrix();
        GFX->disableShaders();

        PROFILE_END();
        return;
    }


    GFX->setCullMode(GFXCullCCW);

    F32 lodattn = scaledcamdist / F32(sgspLastSelfShadowLOD + 1);
    lodattn = (1.0f - (lodattn * lodattn)) * 1.5f;
    attn *= mClampF(lodattn, 0.0f, 1.0f);

    color = Point4F(sgLight->mColor.red, sgLight->mColor.green, sgLight->mColor.blue, attn);
    color.x = (color.x + ((1.0 - color.x) * adjustment)) * attn;
    color.y = (color.y + ((1.0 - color.y) * adjustment)) * attn;
    color.z = (color.z + ((1.0 - color.z) * adjustment)) * attn;
    //color.x = (1.0 - ((1.0 - color.x) * adjustment)) * attn;
    //color.y = (1.0 - ((1.0 - color.y) * adjustment)) * attn;
    //color.z = (1.0 - ((1.0 - color.z) * adjustment)) * attn;
    color.x = mClampF(color.x, 0.0, 1.0);
    color.y = mClampF(color.y, 0.0, 1.0);
    color.z = mClampF(color.z, 0.0, 1.0);
    GFX->setPixelShaderConstF(3, color, 1);

    MatrixF shapeworld;
    shapeworld = sgParentObject->getRenderTransform();
    shapeworld.scale(sgParentObject->getScale());
    objecttolight.mul(sgWorldToLightY, shapeworld);
    objecttoscreen.mul(worldtoscreen, shapeworld);
    sgRenderShape(sgShapeInstance, objecttolight, 4, objecttoscreen, 0);

    ShapeBase* shapebase = dynamic_cast<ShapeBase*>(sgParentObject);
    for (U32 i = 0; i < ShapeBase::MaxMountedImages; i++)
    {
        TSShapeInstance* mountinst = shapebase->getImageShapeInstance(i);
        if (!mountinst)
            continue;

        shapebase->getRenderImageTransform(i, &shapeworld);
        shapeworld.scale(sgParentObject->getScale());
        objecttolight.mul(sgWorldToLightY, shapeworld);
        objecttoscreen.mul(worldtoscreen, shapeworld);
        sgRenderShape(mountinst, objecttolight, 4, objecttoscreen, 0);
    }


    GFX->setTexture(0, NULL);
    GFX->setTexture(1, NULL);
    GFX->setTexture(2, NULL);
    GFX->setTexture(3, NULL);
    GFX->setTextureStageColorOp(4, GFXTOPDisable);
    GFX->setTextureStageColorOp(3, GFXTOPDisable);
    GFX->setTextureStageColorOp(2, GFXTOPDisable);
    GFX->setTextureStageColorOp(1, GFXTOPDisable);
    GFX->setTextureStageMagFilter(0, GFXTextureFilterLinear);
    GFX->setTextureStageMinFilter(0, GFXTextureFilterLinear);
    GFX->setTextureStageColorOp(0, GFXTOPDisable);
    GFX->setZEnable(true);
    GFX->setZWriteEnable(true);
    GFX->setSrcBlend(GFXBlendOne);
    GFX->setDestBlend(GFXBlendZero);
    GFX->setAlphaTestEnable(false);
    GFX->popWorldMatrix();
    GFX->disableShaders();

    PROFILE_END();
}

void sgShadowProjector::sgDebugRenderProjectionVolume()
{
    GFX->setSrcBlend(GFXBlendOne);
    //GFX->setDestBlend(GFXBlendOne);
    GFX->setDestBlend(GFXBlendZero);
    GFX->setTextureStageColorOp(0, GFXTOPDisable);

    GFX->disableShaders();

    PrimBuild::color(ColorF(1.0, 1.0, 0.0));
    PrimBuild::begin(GFXLineStrip, 20);
    PrimBuild::vertex3fv(testRenderPoints[0]);
    PrimBuild::vertex3fv(testRenderPoints[1]);
    PrimBuild::vertex3fv(testRenderPoints[3]);
    PrimBuild::vertex3fv(testRenderPoints[2]);
    PrimBuild::vertex3fv(testRenderPoints[0]);
    PrimBuild::vertex3fv(testRenderPoints[4]);
    PrimBuild::vertex3fv(testRenderPoints[5]);
    PrimBuild::vertex3fv(testRenderPoints[7]);
    PrimBuild::vertex3fv(testRenderPoints[6]);
    PrimBuild::vertex3fv(testRenderPoints[4]);
    PrimBuild::end();

    PrimBuild::begin(GFXLineStrip, 20);
    PrimBuild::vertex3fv(testRenderPoints[1]);
    PrimBuild::vertex3fv(testRenderPoints[5]);
    PrimBuild::end();

    PrimBuild::begin(GFXLineStrip, 20);
    PrimBuild::vertex3fv(testRenderPoints[2]);
    PrimBuild::vertex3fv(testRenderPoints[6]);
    PrimBuild::end();

    PrimBuild::begin(GFXLineStrip, 20);
    PrimBuild::vertex3fv(testRenderPoints[3]);
    PrimBuild::vertex3fv(testRenderPoints[7]);
    PrimBuild::end();
}

Point3F sgShadowProjector::sgGetCompositeShadowLightDirection()
{
    // don't want this to run too fast on newer systems (otherwise shadows snap into place)...
    U32 time = Platform::getRealMilliseconds();

    LightInfoList bestlights;
    getCurrentClientSceneGraph()->getLightManager()->sgGetBestLights(bestlights);

    if ((time - sgPreviousShadowTime) < SG_DYNAMIC_SHADOW_TIME)
        return sgPreviousShadowLightingVector;

    sgPreviousShadowTime = time;

    // ok get started...
    U32 zone = sgParentObject->getCurrZone(0);

    U32 score;
    U32 maxscore[2] = { 0, 0 };
    LightInfo* light[2] = { NULL, NULL };
    VectorF vector[2] = { VectorF(0, 0, 0), VectorF(0, 0, 0) };

    for (U32 i = 0; i < bestlights.size(); i++)
    {
        LightInfo* l = bestlights[i];

        if ((l->mType == LightInfo::Ambient) || (l->mType == LightInfo::Vector))
            score = l->mScore / SG_LIGHTMANAGER_SUN_PRIORITY;
        else if ((l->mType == LightInfo::SGStaticPoint) || (l->mType == LightInfo::SGStaticSpot))
            score = l->mScore / SG_LIGHTMANAGER_STATIC_PRIORITY;
        else
            score = l->mScore / SG_LIGHTMANAGER_DYNAMIC_PRIORITY;

        if (score > maxscore[0])
        {
            light[1] = light[0];
            maxscore[1] = maxscore[0];

            light[0] = l;
            maxscore[0] = score;
        }
        else if (score > maxscore[1])
        {
            light[1] = l;
            maxscore[1] = score;
        }
    }

    for (U32 i = 0; i < 2; i++)
    {
        if (!light[i])
            break;

        if ((light[i]->mType == LightInfo::Ambient) || (light[i]->mType == LightInfo::Vector))
        {
            if (zone == 0)
                vector[i] = light[i]->mDirection;
            else
                vector[i] = Point3F(0, 0, -1.0);
        }
        else
        {
            VectorF vect = sgParentObject->getPosition() - light[i]->mPos;
            vect.normalize();
            vector[i] = vect;
        }
    }

    VectorF vectcomposite = VectorF(0, 0, -1.0f);
    if (light[0])
    {
        if (light[1])
        {
            F32 ratio = F32(maxscore[0]) / F32(maxscore[0] + maxscore[1]);
            vectcomposite = (vector[0] * ratio) + (vector[1] * (1.0f - ratio));
        }
        else
            vectcomposite = vector[0];
    }

    VectorF step = (vectcomposite - sgPreviousShadowLightingVector) / SG_DYNAMIC_SHADOW_STEPS;
    sgPreviousShadowLightingVector += step;
    sgPreviousShadowLightingVector.normalize();

    return sgPreviousShadowLightingVector;
}

