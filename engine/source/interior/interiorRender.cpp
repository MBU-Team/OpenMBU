//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "interior/interior.h"
#include "sceneGraph/sceneState.h"
#include "sceneGraph/sceneGraph.h"
#include "sceneGraph/lightManager.h"

#include "gfx/gBitmap.h"
#include "math/mMatrix.h"
#include "math/mRect.h"
#include "core/bitVector.h"
#include "platform/profiler.h"
#include "gfx/gfxDevice.h"
#include "interior/interiorInstance.h"
#include "gfx/gfxTextureHandle.h"
#include "materials/materialList.h"
#include "materials/sceneData.h"
#include "materials/matInstance.h"
#include "gfx/glowBuffer.h"
#include "materials/customMaterial.h"
#include "math/mathUtils.h"
#include "renderInstance/renderInstMgr.h"
#include "core/frameAllocator.h"
#include "lightingSystem/sgLightingModel.h"
#include "lightingSystem/sgHashMap.h"

extern bool sgFogActive;
extern U16* sgActivePolyList;
extern U32  sgActivePolyListSize;
extern U16* sgFogPolyList;
extern U32  sgFogPolyListSize;
extern U16* sgEnvironPolyList;
extern U32  sgEnvironPolyListSize;

Point3F sgOSCamPosition;


U32         sgRenderIndices[2048];
U32         csgNumAllowedPoints = 256;

extern "C" {
    F32   texGen0[8];
    F32   texGen1[8];
    Point2F* fogCoordinatePointer;
}

//------------------------------------------------------------------------------
// Set up render states for interor rendering
//------------------------------------------------------------------------------
void Interior::setupRenderStates()
{
    GFX->setBaseRenderState();

    for (U32 i = 0; i < TEXTURE_STAGE_COUNT; i++)
    {
        GFX->setTextureStageAddressModeU(i, GFXAddressWrap);
        GFX->setTextureStageAddressModeV(i, GFXAddressWrap);
        GFX->setTextureStageMagFilter(i, GFXTextureFilterLinear);
        GFX->setTextureStageMinFilter(i, GFXTextureFilterLinear);
        GFX->setTextureStageMipFilter(i, GFXTextureFilterLinear);
    }

    // set up states for fixed function materials
    GFX->setTextureStageColorOp(0, GFXTOPModulate);
    GFX->setTextureStageColorOp(1, GFXTOPModulate);
    GFX->setTextureStageColorOp(2, GFXTOPDisable);

    // set culling
    if (getCurrentClientSceneGraph()->isReflectPass())
    {
        GFX->setCullMode(GFXCullCW);
    }
    else
    {
        GFX->setCullMode(GFXCullCCW);
    }

}

//------------------------------------------------------------------------------
// Setup zone visibility
//------------------------------------------------------------------------------
ZoneVisDeterminer Interior::setupZoneVis(InteriorInstance* intInst, SceneState* state)
{

    U32 zoneOffset = intInst->getZoneRangeStart() != 0xFFFFFFFF ? intInst->getZoneRangeStart() : 0;

    U32 baseZone = 0xFFFFFFFF;

    if (intInst->getNumCurrZones() == 1)
    {
        baseZone = intInst->getCurrZone(0);
    }
    else
    {
        for (U32 i = 0; i < intInst->getNumCurrZones(); i++)
        {
            if (state->getZoneState(intInst->getCurrZone(i)).render == true)
            {
                if (baseZone == 0xFFFFFFFF) {
                    baseZone = intInst->getCurrZone(i);
                    break;
                }
            }
        }
        if (baseZone == 0xFFFFFFFF)
        {
            baseZone = intInst->getCurrZone(0);
        }
    }

    ZoneVisDeterminer zoneVis;
    zoneVis.runFromState(state, zoneOffset, baseZone);
    return zoneVis;
}

//------------------------------------------------------------------------------
// Setup scenegraph data structure for materials
//------------------------------------------------------------------------------
SceneGraphData Interior::setupSceneGraphInfo(InteriorInstance* intInst,
    SceneState* state)
{
    SceneGraphData sgData;

    // grab the sun data from the light manager
    Vector<LightInfo*> lights;
    const LightInfo* sunlight = getCurrentClientSceneGraph()->getLightManager()->sgGetSpecialLight(LightManager::sgSunLightType);
    VectorF sunVector = sunlight->mDirection; //first light is always sun

    // set the sun data into scenegraph data
    sgData.light.mDirection.set(sunVector);
    sgData.light.mPos.set(sunVector * -10000.0);
    sgData.light.mAmbient = sunlight->mAmbient;
    sgData.light.mColor = sunlight->mColor;

    /*LightInfoList lightlist;
    getCurrentClientSceneGraph()->getLightManager()->sgGetAllUnsortedLights(lightlist);
    for(U32 i=0; i<lightlist.size(); i++)
    {
        if(lightlist[i]->mType == LightInfo::Point)
            sgData.light.mPos.set(lightlist[i]->mPos);
    }*/

    // fill in camera position relative to interior
    sgData.camPos = state->getCameraPosition();

    // fill in interior's transform
    sgData.objTrans = intInst->getTransform();

    // fog
    sgData.useFog = true;
    sgData.fogTex = getCurrentClientSceneGraph()->getFogTexture();
    sgData.fogHeightOffset = getCurrentClientSceneGraph()->getFogHeightOffset();
    sgData.fogInvHeightRange = getCurrentClientSceneGraph()->getFogInvHeightRange();
    sgData.visDist = getCurrentClientSceneGraph()->getVisibleDistanceMod();

    // misc
    sgData.backBuffTex = GFX->getSfxBackBuffer();

    return sgData;
}

//------------------------------------------------------------------------------
// Render to zbuffer - this is for fill-rate limited situations.  The idea is
// to render the interior to the zbuffer first.  Then draw the interior again
// using shaders.  The second draw will have 0 over-draw.
//------------------------------------------------------------------------------
void Interior::renderToZBuffer(ZoneVisDeterminer& zoneVis, SceneGraphData& sgData)
{
#if 0
    CustomMaterial* mat = static_cast<CustomMaterial*>(Sim::findObject("Blank"));

    if (!mat) return;

    // We reduce the fillrate used by the zpass if
    // we also disable color writes.
    GFX->enableColorWrites(false, false, false, false);

    for (U32 i = 0; i < getNumZones(); i++)
    {
        if (zoneVis.isZoneVisible(i) == false)
        {
            continue;
        }
        for (U32 j = 0; j < mZoneRNList[i].renderNodeList.size(); j++)
        {
            RenderNode& node = mZoneRNList[i].renderNodeList[j];

            // Don't pre-write z for translucent surfaces.
            if (node.matInst &&
                node.matInst->getMaterial() &&
                node.matInst->getMaterial()->translucent)
            {
                continue;
            }

            while (mat->setupPass(sgData))
            {
                GFX->drawPrimitive(node.primInfoIndex);
            }
        }
    }

    // Restore color writes before we leave.
    GFX->enableColorWrites(true, true, true, true);
#endif
}

//------------------------------------------------------------------------------
// Render zone RenderNode
//------------------------------------------------------------------------------
void Interior::renderZoneNode(RenderNode& node,
    InteriorInstance* intInst,
    SceneGraphData& sgData,
    RenderInst* coreRi)
{
    //static U16 curBaseTexIndex = 0;

    RenderInst* ri = gRenderInstManager.allocInst();
    *ri = *coreRi;

    ri->lightmap = NULL;


    // setup lightmap
    //if (node.lightMapIndex != U8(-1))
    //{
    //    ri->lightmap = &gInteriorLMManager.getHandle(mLMHandle, intInst->getLMHandle(), node.lightMapIndex);

    //    /*if( node.exterior )
    //    {
    //       sgData.normLightmap = NULL;
    //       sgData.useLightDir = true;
    //    }
    //    else
    //    {*/
    //    ri->normLightmap = &gInteriorLMManager.getNormalHandle(mLMHandle, intInst->getLMHandle(), node.lightMapIndex);
    //    //ri->useLightDir = false;
    // //}
    //}

    // setup base map
    //if( node.baseTexIndex )
    //{
    //   curBaseTexIndex = node.baseTexIndex;
    //}

    MatInstance* mat = node.matInst;
    if (mat)// && GFX->getPixelShaderVersion() > 0.0)
    {

        ri->matInst = mat;
        ri->primBuffIndex = node.primInfoIndex;
        gRenderInstManager.addInst(ri);

    }
}

//------------------------------------------------------------------------------
// Render zone RenderNode
//------------------------------------------------------------------------------
void Interior::renderReflectNode(ReflectRenderNode& node,
    InteriorInstance* intInst,
    SceneGraphData& sgData,
    RenderInst* coreRi)
{
    RenderInst* ri = gRenderInstManager.allocInst();
    *ri = *coreRi;

    ri->vertBuff = &mReflectVertBuff;
    ri->primBuff = &mReflectPrimBuff;

    static U16 curBaseTexIndex = 0;

    // use sgData.backBuffer to transfer the reflect texture to the materials
    ReflectPlane* rp = &intInst->mReflectPlanes[node.reflectPlaneIndex];
    ri->backBuffTex = &rp->getTex();
    ri->reflective = true;

    // setup lightmap
    /*if (node.lightMapIndex != U8(-1))
    {
        ri->lightmap = &gInteriorLMManager.getHandle(mLMHandle, intInst->getLMHandle(), node.lightMapIndex);
        ri->normLightmap = &gInteriorLMManager.getNormalHandle(mLMHandle, intInst->getLMHandle(), node.lightMapIndex);
    }*/

    MatInstance* mat = node.matInst;
    if (mat)// && GFX->getPixelShaderVersion() > 0.0)
    {
        ri->matInst = mat;
        ri->primBuffIndex = node.primInfoIndex;
        gRenderInstManager.addInst(ri);

    }
}


//------------------------------------------------------------------------------
// Setup the rendering
//------------------------------------------------------------------------------
void Interior::setupRender(InteriorInstance* intInst,
    SceneState* state,
    RenderInst* coreRi)
{
    // setup world matrix - for fixed function
    MatrixF world = GFX->getWorldMatrix();
    world.mul(intInst->getRenderTransform());
    world.scale(intInst->getScale());
    GFX->setWorldMatrix(world);

    // setup world matrix - for shaders
    MatrixF proj = GFX->getProjectionMatrix();
    proj.mul(world);
    proj.transpose();
    GFX->setVertexShaderConstF(0, (float*)&proj, 4);

    // set buffers
    GFX->setVertexBuffer(mVertBuff);
    GFX->setPrimitiveBuffer(mPrimBuff);


    // setup renderInst
    coreRi->worldXform = gRenderInstManager.allocXform();
    *coreRi->worldXform = proj;

    coreRi->vertBuff = &mVertBuff;
    coreRi->primBuff = &mPrimBuff;

    coreRi->objXform = gRenderInstManager.allocXform();
    *coreRi->objXform = intInst->getRenderTransform();

    // grab the sun data from the light manager
    const LightInfo* sunlight = getCurrentClientSceneGraph()->getLightManager()->sgGetSpecialLight(LightManager::sgSunLightType);
    VectorF sunVector = sunlight->mDirection;

    coreRi->light.mDirection.set(sunVector);
    coreRi->light.mPos.set(sunVector * -10000.0);
    coreRi->light.mAmbient = sunlight->mAmbient;
    coreRi->light.mColor = sunlight->mColor;

    coreRi->type = RenderInstManager::RIT_Interior;
    coreRi->backBuffTex = &GFX->getSfxBackBuffer();
}


//------------------------------------------------------------------------------
// Render
//------------------------------------------------------------------------------
void Interior::prepBatchRender(InteriorInstance* intInst, SceneState* state)
{
    // coreRi - used as basis for subsequent interior ri allocations
    RenderInst* coreRi = gRenderInstManager.allocInst();
    SceneGraphData sgData;
    setupRender(intInst, state, coreRi);
    ZoneVisDeterminer zoneVis = setupZoneVis(intInst, state);

#ifdef TORQUE_DEBUG
    if (smRenderMode != 0)
    {
        RenderInst* ri = gRenderInstManager.allocInst();
        ri->obj = intInst;
        ri->state = state;
        ri->type = RenderInstManager::RIT_Object;
        gRenderInstManager.addInst(ri);
        return;
    }
#endif

    // render zones
    for (U32 i = 0; i < mZones.size(); i++)
    {
        // No need to try to render zones without any surfaces
        if (mZones[i].surfaceCount == 0)
            continue;

        if (zoneVis.isZoneVisible(i) == false && !getCurrentClientSceneGraph()->isReflectPass())
        {
            continue;
        }

        for (U32 j = 0; j < mZoneRNList[i].renderNodeList.size(); j++)
        {
            RenderNode& node = mZoneRNList[i].renderNodeList[j];
            renderZoneNode(node, intInst, sgData, coreRi);
        }
    }


    // render reflective surfaces
    if (!getCurrentClientSceneGraph()->isReflectPass())
    {
        renderLights(intInst, sgData, coreRi, zoneVis);

        GFX->setVertexBuffer(mReflectVertBuff);
        GFX->setPrimitiveBuffer(mReflectPrimBuff);

        // hack - need to address wrapping/clamping at some point
        GFX->setTextureStageAddressModeU(2, GFXAddressWrap);
        GFX->setTextureStageAddressModeV(2, GFXAddressWrap);

        for (U32 i = 0; i < mZones.size(); i++)
        {
            if (zoneVis.isZoneVisible(i) == false) continue;

            for (U32 j = 0; j < mZoneReflectRNList[i].reflectList.size(); j++)
            {
                ReflectRenderNode& node = mZoneReflectRNList[i].reflectList[j];
                renderReflectNode(node, intInst, sgData, coreRi);
            }
        }
    }


    GFX->setBaseRenderState();
}


struct sgDynamicLightCacheData
{
    LightInfo* sgLight;

    // light/model info...
    F32 sgMaxRadius;
    GFXTexHandle sgTexture;
};

class sgDynamicLightCache : public Vector<sgDynamicLightCacheData>
{
public:
    sgDynamicLightCacheData* sgFind(LightInfo* light)
    {
        sgDynamicLightCache& list = *this;
        for (U32 i = 0; i < list.size(); i++)
        {
            if (list[i].sgLight == light)
                return &list[i];
        }
        return NULL;
    }
};

bool Interior::renderLights(InteriorInstance* intInst, SceneGraphData& sgData,
    RenderInst* coreRi, const ZoneVisDeterminer& zonevis)
{
#ifdef TEMP_REMOVE_RENDER_LIGHTS
    sgDynamicLightCache lightdatacache;
    LightInfoList lights;
    LightManager* lm = getCurrentClientSceneGraph()->getLightManager();

    // get all lights...
    lm->sgGetAllUnsortedLights(lights);

    // filter down to needed lights...
    for (U32 i = 0; i < lights.size(); i++)
    {
        LightInfo* light = lights[i];
        if ((light->mType != ::LightInfo::Point) && (light->mType != ::LightInfo::Spot))
            continue;

        // get info...
        sgLightingModel& lightingmodel = sgLightingModelManager::sgGetLightingModel(light->sgLightingModelName);
        lightingmodel.sgSetState(light);

        F32 maxrad = lightingmodel.sgGetMaxRadius(true);
        light->sgTempModelInfo[0] = 0.5f / maxrad;

        GFXTexHandle tex;
        if (light->mType == LightInfo::Spot)
            tex = lightingmodel.sgGetDynamicLightingTextureSpot();
        else
            tex = lightingmodel.sgGetDynamicLightingTextureOmni();

        lightingmodel.sgResetState();

        Point3F offset = Point3F(maxrad, maxrad, maxrad);
        Point3F lightpos = light->mPos;
        intInst->getRenderWorldTransform().mulP(lightpos);
        lightpos.convolveInverse(intInst->getScale());
        Box3F box;
        box.min = lightpos;
        box.max = lightpos;
        box.min -= offset;
        box.max += offset;

        // test visible...
        // TODO!!! - nm, using zone vis instead...

        // test illuminate interior...
        if (intInst->getObjBox().isOverlapped(box) == false)
            continue;

        // add to the list...
        lightdatacache.increment();
        lightdatacache.last().sgLight = light;
        lightdatacache.last().sgMaxRadius = maxrad;
        lightdatacache.last().sgTexture = tex;
    }

    // build the render instances...
    for (U32 z = 0; z < mZones.size(); z++)
    {
        if (!zonevis.isZoneVisible(z))
            continue;

        Zone& zone = mZones[z];
        S32 zoneid = zone.zoneId - 1;
        if (zoneid > -1)
            zoneid += intInst->getZoneRangeStart();// only zone managers...
        else
            zoneid = intInst->getCurrZone(0);// if not what zone is it in...

        // compare the lights and build list...
        lights.clear();
        for (U32 i = 0; i < lightdatacache.size(); i++)
        {
            if (!lightdatacache[i].sgLight->sgAllowDiffuseZoneLighting(zoneid))
                continue;

            // am I in the zone?
            // need to verify - this is the only thing stopping
            // the entire interior from being rerendered!!!
            if (!lightdatacache[i].sgLight->sgIsInZone(zoneid))
                continue;

            lights.push_back(lightdatacache[i].sgLight);
        }

        if (lights.size() <= 0)
            continue;

        // get the dual sorted list...
        LightInfoDualList duallist;
        lm->sgBuildDualLightLists(lights, duallist);

        for (U32 d = 0; d < duallist.size(); d++)
        {
            sgDynamicLightCacheData* lightpri = lightdatacache.sgFind(duallist[d].sgLightPrimary);
            sgDynamicLightCacheData* lightsec = lightdatacache.sgFind(duallist[d].sgLightSecondary);

            if (!lightpri)
                continue;

            // avoid dual on single groups...
            bool allowdual = (lightsec != NULL);

            for (U32 j = 0; j < mZoneRNList[z].renderNodeList.size(); j++)
            {
                RenderNode& node = mZoneRNList[z].renderNodeList[j];

                // find the primary material (secondary lights all render the same)...
                MatInstance* dmat = MatInstance::getDynamicLightingMaterial(
                    node.matInst, lightpri->sgLight, allowdual);
                if (!node.matInst || !dmat)
                    continue;

                RenderInst* ri = gRenderInstManager.allocInst();
                *ri = *coreRi;
                ri->type = RenderInstManager::RIT_InteriorDynamicLighting;
                ri->matInst = dmat;
                ri->primBuffIndex = node.primInfoIndex;
                ri->dynamicLight = lightpri->sgTexture;
                dMemcpy(&ri->light, lightpri->sgLight, sizeof(ri->light));

                if (dmat->isDynamicLightingMaterial_Dual())
                {
                    ri->dynamicLightSecondary = lightsec->sgTexture;
                    dMemcpy(&ri->lightSecondary, lightsec->sgLight, sizeof(ri->lightSecondary));
                    gRenderInstManager.addInst(ri);
                    continue;
                }

                gRenderInstManager.addInst(ri);

                // could be the material is only single...
                if (lightsec)
                {
                    // own pass needs own material...
                    dmat = MatInstance::getDynamicLightingMaterial(
                        node.matInst, lightsec->sgLight, false);
                    if (!node.matInst || !dmat)
                        continue;

                    RenderInst* ri = gRenderInstManager.allocInst();
                    *ri = *coreRi;
                    ri->type = RenderInstManager::RIT_InteriorDynamicLighting;
                    ri->matInst = dmat;
                    ri->primBuffIndex = node.primInfoIndex;
                    ri->dynamicLight = lightsec->sgTexture;
                    dMemcpy(&ri->light, lightsec->sgLight, sizeof(ri->light));
                    gRenderInstManager.addInst(ri);
                }
            }
        }
    }

    //LightInfoDualList duallist;
    //lm->sgBuildDualLightLists(lights, duallist);

    /*if(lights.size() <= 0)
        return true;

    for(U32 i=0; i<lights.size(); i++)
    {
        LightInfo *light = lights[i];
        if((light->mType != ::LightInfo::Point) && (light->mType != ::LightInfo::Spot))
            continue;

        Point3F lightPoint = light->mPos;
        bool spot = (light->mType == LightInfo::Spot);

        // get the model...
        sgLightingModel &lightingmodel = sgLightingModelManager::sgGetLightingModel(
            light->sgLightingModelName);
        lightingmodel.sgSetState(light);
        // get the info...
        F32 maxrad = lightingmodel.sgGetMaxRadius(true);
        light->sgTempModelInfo[0] = 0.5f / maxrad;

        // get the dynamic lighting texture...
        if((light->mType == LightInfo::Spot) || (light->mType == LightInfo::SGStaticSpot))
            coreRi->dynamicLight = lightingmodel.sgGetDynamicLightingTextureSpot();
        else
            coreRi->dynamicLight = lightingmodel.sgGetDynamicLightingTextureOmni();
        // reset the model...
        lightingmodel.sgResetState();

        if(maxrad <= 0.0f)
            continue;

        coreRi->lightingTransform = light->sgLightingTransform;
        dMemcpy(&coreRi->light, light, sizeof(coreRi->light));

        intInst->getRenderWorldTransform().mulP(lightPoint);
        lightPoint.convolveInverse(intInst->getScale());

        // get the offset and scale...
        Point3F offset = Point3F(maxrad, maxrad, maxrad);

        // overlap with interior?
        Box3F box;
        box.min = lightPoint;
        box.max = lightPoint;
        box.min -= offset;
        box.max += offset;
        if (intInst->getObjBox().isOverlapped(box) == false)
            continue;

        for(U32 z=0; z<mZones.size(); z++)
        {
            Zone &zone = mZones[z];
            S32 zoneid = zone.zoneId - 1;
            if(zoneid > -1)
                zoneid += intInst->getZoneRangeStart();// only zone managers...
            else
                zoneid = intInst->getCurrZone(0);// if not what zone is it in...

            if(!light->sgAllowDiffuseZoneLighting(zoneid))
                continue;

            // am I in the zone?
            // need to verify - this is the only thing stopping
            // the entire interior from being rerendered!!!
            if(!light->sgIsInZone(zoneid))
                continue;

                for(U32 j=0; j<mZoneRNList[z].renderNodeList.size(); j++)
                {
                    RenderNode &node = mZoneRNList[z].renderNodeList[j];

                    MatInstance *dmat = MatInstance::getDynamicLightingMaterial(node.matInst, light, true);
                    if(!node.matInst || !dmat)
                        continue;

                    RenderInst *ri = gRenderInstManager.allocInst();
                    *ri = *coreRi;
                    ri->type = RenderInstManager::RIT_InteriorDynamicLighting;
                    ri->matInst = dmat;
                    ri->primBuffIndex = node.primInfoIndex;
                    gRenderInstManager.addInst(ri);
                }
        }
    }*/
#endif

    return true;
}

