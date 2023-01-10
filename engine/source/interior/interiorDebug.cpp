//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------
// Conversion to TSE by Brian Richardson (bzzt@knowhere.net)

#ifdef TORQUE_DEBUG

#include "interior/interior.h"
#include "interior/interiorInstance.h"
#include "console/console.h"
#include "core/color.h"
#include "math/mMatrix.h"
#include "gfx/gBitmap.h"
#include "gfx/primBuilder.h"
#include "gfx/gfxShader.h"
#include "gfx/gBitmap.h"
#include "materials/matInstance.h"
#include "materials/materialList.h"
#include "materials/shaderData.h"
// Needed for PC_USERDEF1, is there a better spot to get this?
#include "../../game/shaders/shdrConsts.h"
#include "renderInstance/renderInstMgr.h"

static U8 interiorDebugColors[14][3] = {
   { 0xFF, 0xFF, 0xFF },
   { 0x00, 0x00, 0xFF },
   { 0x00, 0xFF, 0x00 },
   { 0xFF, 0x00, 0x00 },
   { 0xFF, 0xFF, 0x00 },
   { 0xFF, 0x00, 0xFF },
   { 0x00, 0xFF, 0xFF },
   { 0x80, 0x80, 0x80 },
   { 0xFF, 0x80, 0x80 },
   { 0x80, 0xFF, 0x80 },
   { 0x80, 0x80, 0xFF },
   { 0x80, 0xFF, 0xFF },
   { 0xFF, 0x80, 0xFF },
   { 0xFF, 0x80, 0x80 }
};


namespace {

    void lineLoopFromStrip(Vector<ItrPaddedPoint>& points,
        Vector<U32>& windings,
        U32                     windingStart,
        U32                     windingCount)
    {
        PrimBuild::begin(GFXLineStrip, windingCount + 1);
        PrimBuild::vertex3fv(points[windings[windingStart]].point);
        S32 skip = windingStart + 1;
        while (skip < (windingStart + windingCount)) {
            PrimBuild::vertex3fv(points[windings[skip]].point);
            skip += 2;
        }

        skip -= 1;
        while (skip > windingStart) {
            if (skip < (windingStart + windingCount)) {
                PrimBuild::vertex3fv(points[windings[skip]].point);
            }
            skip -= 2;
        }
        PrimBuild::vertex3fv(points[windings[windingStart]].point);
        PrimBuild::end();
    }

    void lineStrip(Vector<ItrPaddedPoint>& points,
        Vector<U32>& windings,
        U32                     windingStart,
        U32                     windingCount)
    {
        U32 end = 2;

        while (end < windingCount) {
            // Even
            PrimBuild::begin(GFXLineStrip, 4);
            PrimBuild::vertex3fv(points[windings[windingStart + end - 2]].point);
            PrimBuild::vertex3fv(points[windings[windingStart + end - 1]].point);
            PrimBuild::vertex3fv(points[windings[windingStart + end - 0]].point);
            PrimBuild::vertex3fv(points[windings[windingStart + end - 2]].point);
            PrimBuild::end();

            end++;
            if (end >= windingCount)
                break;

            // Odd
            PrimBuild::begin(GFXLineStrip, 4);
            PrimBuild::vertex3fv(points[windings[windingStart + end - 1]].point);
            PrimBuild::vertex3fv(points[windings[windingStart + end - 2]].point);
            PrimBuild::vertex3fv(points[windings[windingStart + end - 0]].point);
            PrimBuild::vertex3fv(points[windings[windingStart + end - 1]].point);
            PrimBuild::end();

            end++;
        }
    }

} // namespace {}

void Interior::debugRender(const ZoneVisDeterminer& zoneVis, SceneGraphData& sgData, InteriorInstance* intInst)
{
    // We use this shader to color things and re-use the primitive buffer that the 
    // interior sets up
    if (mDebugShader == NULL)
    {
        mDebugShader = static_cast<ShaderData*>(Sim::findObject("_DebugInterior_"));
        AssertFatal(mDebugShader, "Unable to find ShaderData _DebugInterior_");
    }

    // We use this to override the texture that the interior defines.
    if (mBlankTexture.isNull()) {
        // Allocate a small white bitmap
        GBitmap temp(16, 16);
        mBlankTexture.set(&temp, &GFXDefaultStaticDiffuseProfile, false);
    }

    // set buffers (this may not be needed)
    GFX->setVertexBuffer(mVertBuff);
    GFX->setPrimitiveBuffer(mPrimBuff);

    // Set our "base debug states"
    GFX->setTextureStageColorOp(0, GFXTOPDisable); // turn off texturing.
    GFX->disableShaders();

    switch (smRenderMode) {
    case NormalRenderLines:
        debugNormalRenderLines(zoneVis);
        break;

    case ShowDetail:
        debugShowDetail(zoneVis);
        break;

    case ShowAmbiguous:
        debugShowAmbiguous(zoneVis);
        break;

    case ShowLightmaps:
        debugShowLightmaps(zoneVis, sgData, intInst);
        break;

    case ShowPortalZones:
        debugShowPortalZones(zoneVis);
        break;

    case ShowCollisionFans:
        debugShowCollisionFans(zoneVis);
        break;

    case ShowOrphan:
        debugShowOrphan(zoneVis);
        break;

    case ShowStrips:
        debugShowStrips(zoneVis);
        break;

    case ShowTexturesOnly:
        debugShowTexturesOnly(zoneVis, sgData, intInst);
        break;

    case ShowNullSurfaces:
        debugShowNullSurfaces(zoneVis, sgData, intInst);
        break;

    case ShowLargeTextures:
        debugShowLargeTextures(zoneVis, sgData, intInst);
        break;

    case ShowOutsideVisible:
        debugShowOutsideVisible(zoneVis);
        break;

    case ShowHullSurfaces:
        debugShowHullSurfaces();
        break;

    case ShowVehicleHullSurfaces:
        debugShowVehicleHullSurfaces(zoneVis, sgData, intInst);
        break;

    case ShowDetailLevel:
        debugShowDetailLevel(zoneVis);
        break;

    case ShowVertexColors:
        //      debugShowVertexColors(pMaterials);
        break;

    default:
        AssertWarn(false, "Warning!  Misunderstood debug render mode.  Defaulting to ShowDetail");
        debugShowDetail(zoneVis);
        break;
    }
}

void Interior::preDebugRender()
{
    // Set up our rendering states.
    if (mDebugShader && mDebugShader->getShader())
    {
        mDebugShader->getShader()->process();
        GFX->setTexture(0, mBlankTexture);
        GFX->setTextureStageColorOp(0, GFXTOPModulate);
        GFX->setTextureStageColorOp(1, GFXTOPDisable);
    }
}

void Interior::debugNormalRenderLines(const ZoneVisDeterminer& zoneVis)
{
    for (U32 i = 0; i < mZones.size(); i++) {
        if (zoneVis.isZoneVisible(i) == false)
            continue;
        for (U32 j = 0; j < mZones[i].surfaceCount; j++)
        {
            U32 surfaceIndex = mZoneSurfaces[mZones[i].surfaceStart + j];
            Surface& rSurface = mSurfaces[surfaceIndex];
            PrimBuild::color3f(0.0f, 0.0f, 0.0f);
            lineLoopFromStrip(mPoints, mWindings, rSurface.windingStart, rSurface.windingCount);
        }
    }
}

void Interior::debugShowSurfaceFlag(const ZoneVisDeterminer& zoneVis, const U32 flag, const ColorF& c)
{
    preDebugRender();
    for (U32 i = 0; i < mZones.size(); i++) {
        if (zoneVis.isZoneVisible(i) == false)
            continue;
        for (U32 j = 0; j < mZones[i].surfaceCount; j++)
        {
            U32 surfaceIndex = mZoneSurfaces[mZones[i].surfaceStart + j];
            Surface& rSurface = mSurfaces[surfaceIndex];
            ColorF col(0.0f, 0.0f, 0.0f);
            if (rSurface.surfaceFlags & flag)
                col = c;
            else {
                if (smFocusedDebug == true)
                    continue;
                else
                    col.set(1.0f, 1.0f, 1.0f);
            }
            GFX->setPixelShaderConstF(PC_USERDEF1, (F32*)&col, 1);
            GFXPrimitive* info = &rSurface.surfaceInfo;
            GFX->drawIndexedPrimitive(info->type, info->minIndex, info->numVertices, info->startIndex, info->numPrimitives);
        }
    }
    GFX->disableShaders();
    debugNormalRenderLines(zoneVis);
}

void Interior::debugShowDetail(const ZoneVisDeterminer& zoneVis)
{
    debugShowSurfaceFlag(zoneVis, SurfaceDetail, ColorF(1.0f, 0.0f, 0.0f));
}

void Interior::debugShowAmbiguous(const ZoneVisDeterminer& zoneVis)
{
    debugShowSurfaceFlag(zoneVis, SurfaceAmbiguous, ColorF(0.0f, 1.0f, 0.0f));
}

void Interior::debugShowOrphan(const ZoneVisDeterminer& zoneVis)
{
    debugShowSurfaceFlag(zoneVis, SurfaceOrphan, ColorF(0.0f, 0.0f, 1.0f));
}

void Interior::debugShowOutsideVisible(const ZoneVisDeterminer& zoneVis)
{
    debugShowSurfaceFlag(zoneVis, SurfaceOutsideVisible, ColorF(1.0f, 0.0f, 0.0f));
}

void Interior::debugShowPortalZones(const ZoneVisDeterminer& zoneVis)
{
    preDebugRender();

    for (U32 i = 0; i < mZones.size(); i++) {
        U8* color;
        if (i == 0)
            color = interiorDebugColors[0];
        else
            color = interiorDebugColors[(i % 13) + 1];

        for (U32 j = mZones[i].surfaceStart; j < mZones[i].surfaceStart + mZones[i].surfaceCount; j++) {
            Surface& rSurface = mSurfaces[mZoneSurfaces[j]];
            ColorF c((F32)color[0] / 255.0f, (F32)color[1] / 255.0f, (F32)color[2] / 255.0f);
            GFX->setPixelShaderConstF(PC_USERDEF1, (F32*)&c, 1);
            GFXPrimitive* info = &rSurface.surfaceInfo;
            GFX->drawIndexedPrimitive(info->type, info->minIndex, info->numVertices, info->startIndex, info->numPrimitives);
        }
    }
    GFX->disableShaders();
    debugRenderPortals();
    debugNormalRenderLines(zoneVis);
}

// Render portals
void Interior::debugRenderPortals()
{
    GFX->setCullMode(GFXCullNone);
    for (U32 i = 0; i < mPortals.size(); i++) {
        const Portal& rPortal = mPortals[i];

        for (U16 j = 0; j < rPortal.triFanCount; j++) {
            const TriFan& rFan = mWindingIndices[rPortal.triFanStart + j];
            U32 k;

            PrimBuild::color4f(0.75, 0.5, 0.75, 0.45);
            PrimBuild::begin(GFXTriangleFan, rFan.windingCount);
            for (k = 0; k < rFan.windingCount; k++)
                PrimBuild::vertex3fv(mPoints[mWindings[rFan.windingStart + k]].point);
            PrimBuild::end();
            PrimBuild::color4f(0, 0, 1, 1);
            PrimBuild::begin(GFXLineStrip, rFan.windingCount + 1);
            for (k = 0; k < rFan.windingCount; k++)
                PrimBuild::vertex3fv(mPoints[mWindings[rFan.windingStart + k]].point);
            PrimBuild::vertex3fv(mPoints[mWindings[rFan.windingStart]].point);
            PrimBuild::end();
        }
    }
}

void Interior::debugShowCollisionFans(const ZoneVisDeterminer& zoneVis)
{
    for (U32 i = 0; i < mZones.size(); i++) {
        if (zoneVis.isZoneVisible(i) == false)
            continue;
        for (U32 j = 0; j < mZones[i].surfaceCount; j++)
        {
            U32 surfaceIndex = mZoneSurfaces[mZones[i].surfaceStart + j];
            Surface& rSurface = mSurfaces[surfaceIndex];
            U32 numIndices;
            U32 fanIndices[32];
            collisionFanFromSurface(rSurface, fanIndices, &numIndices);

            // Filled brush
            PrimBuild::color3f(1.0f, 1.0f, 1.0f);
            PrimBuild::begin(GFXTriangleFan, numIndices);
            for (U32 i = 0; i < numIndices; i++)
                PrimBuild::vertex3fv(mPoints[fanIndices[i]].point);
            PrimBuild::end();

            // Outline
            PrimBuild::color3f(0.0f, 0.0f, 0.0f);
            PrimBuild::begin(GFXLineList, numIndices + 1);
            for (U32 i = 0; i < numIndices; i++)
                PrimBuild::vertex3fv(mPoints[fanIndices[i]].point);
            if (numIndices > 0)
                PrimBuild::vertex3fv(mPoints[fanIndices[0]].point);
            PrimBuild::end();

            // Normal
            PrimBuild::color3f(1, 0, 0);
            PrimBuild::begin(GFXLineList, numIndices * 2);
            for (U32 j = 0; j < numIndices; j++) {
                Point3F up = mPoints[fanIndices[j]].point;
                Point3F norm = getPlane(rSurface.planeIndex);
                if (planeIsFlipped(rSurface.planeIndex))
                    up -= norm * 0.4;
                else
                    up += norm * 0.4;

                PrimBuild::vertex3fv(mPoints[fanIndices[j]].point);
                PrimBuild::vertex3fv(up);
            }
            PrimBuild::end();
        }
    }
}

// This doesn't show strip (they don't go to the card that way)
// But it does show the batches of primitives we send.
void Interior::debugShowStrips(const ZoneVisDeterminer& zoneVis)
{
    // Set up our rendering states.
    preDebugRender();
    for (U32 i = 0; i < mZones.size(); i++) {
        if (zoneVis.isZoneVisible(i) == false)
            continue;

        for (U32 j = 0; j < mZoneRNList[i].renderNodeList.size(); j++)
        {
            RenderNode& node = mZoneRNList[i].renderNodeList[j];
            U32 index = (i + j) % 14;
            ColorF col((F32)interiorDebugColors[index][0] / 255.0f, (F32)interiorDebugColors[index][1] / 255.0f,
                (F32)interiorDebugColors[index][2] / 255.0f);

            GFX->setPixelShaderConstF(PC_USERDEF1, (F32*)&col, 1);
            GFX->drawPrimitive(node.primInfoIndex);
        }
    }
    GFX->disableShaders();
    debugNormalRenderLines(zoneVis);
}

void Interior::debugShowDetailLevel(const ZoneVisDeterminer& zoneVis)
{
    // Set up our rendering states.
    preDebugRender();
    U32 index = getDetailLevel();
    ColorF col((F32)interiorDebugColors[index][0] / 255.0f, (F32)interiorDebugColors[index][1] / 255.0f,
        (F32)interiorDebugColors[index][2] / 255.0f);
    GFX->setPixelShaderConstF(PC_USERDEF1, (F32*)&col, 1);

    for (U32 i = 0; i < mZones.size(); i++) {
        if (zoneVis.isZoneVisible(i) == false)
            continue;

        for (U32 j = 0; j < mZoneRNList[i].renderNodeList.size(); j++)
        {
            RenderNode& node = mZoneRNList[i].renderNodeList[j];
            GFX->drawPrimitive(node.primInfoIndex);
        }
    }
    GFX->disableShaders();
    debugNormalRenderLines(zoneVis);
}

void Interior::debugShowHullSurfaces()
{
    for (U32 i = 0; i < mConvexHulls.size(); i++) {
        const ConvexHull& rHull = mConvexHulls[i];
        for (U32 j = rHull.surfaceStart; j < rHull.surfaceCount + rHull.surfaceStart; j++) {
            U32 index = mHullSurfaceIndices[j];
            if (!isNullSurfaceIndex(index)) {
                const Interior::Surface& rSurface = mSurfaces[index];
                U32 fanVerts[32];
                U32 numVerts;
                collisionFanFromSurface(rSurface, fanVerts, &numVerts);

                PrimBuild::color3i(interiorDebugColors[(i % 13) + 1][0], interiorDebugColors[(i % 13) + 1][1],
                    interiorDebugColors[(i % 13) + 1][2]);
                Point3F center(0, 0, 0);
                PrimBuild::begin(GFXTriangleFan, numVerts);
                for (U32 k = 0; k < numVerts; k++) {
                    PrimBuild::vertex3fv(mPoints[fanVerts[k]].point);
                    center += mPoints[fanVerts[k]].point;
                }
                PrimBuild::end();
                center /= F32(numVerts);
                PrimBuild::color3f(0, 0, 0);
                lineLoopFromStrip(mPoints, mWindings, rSurface.windingStart, rSurface.windingCount);

                PlaneF plane;
                plane.set(mPoints[fanVerts[0]].point, mPoints[fanVerts[1]].point, mPoints[fanVerts[2]].point);
                PrimBuild::begin(GFXLineList, 2);
                PrimBuild::vertex3fv(center);
                PrimBuild::vertex3fv(center + (plane * 0.25));
                PrimBuild::end();
            }
        }
    }
}

void Interior::debugDefaultRender(const ZoneVisDeterminer& zoneVis, SceneGraphData& sgData, InteriorInstance* intInst)
{
    for (U32 i = 0; i < getNumZones(); i++)
    {
        if (zoneVis.isZoneVisible(i) == false)
        {
            continue;
        }
        for (U32 j = 0; j < mZoneRNList[i].renderNodeList.size(); j++)
        {
            RenderNode& node = mZoneRNList[i].renderNodeList[j];
            renderZoneNode(node, intInst, sgData, NULL);
        }
    }
}

void Interior::debugShowNullSurfaces(const ZoneVisDeterminer& zoneVis, SceneGraphData& sgData, InteriorInstance* intInst)
{
    debugDefaultRender(zoneVis, sgData, intInst);
    PrimBuild::color3f(1, 0, 0);
    for (U32 i = 0; i < mNullSurfaces.size(); i++) {
        const NullSurface& rSurface = mNullSurfaces[i];
        PrimBuild::begin(GFXTriangleFan, rSurface.windingCount);
        for (U32 k = 0; k < rSurface.windingCount; k++) {
            PrimBuild::vertex3fv(mPoints[mWindings[rSurface.windingStart + k]].point);
        }
        PrimBuild::end();
    }
}

void Interior::debugShowVehicleHullSurfaces(const ZoneVisDeterminer& zoneVis, SceneGraphData& sgData,
    InteriorInstance* intInst)
{
    debugDefaultRender(zoneVis, sgData, intInst);
    PrimBuild::color3f(1, 0, 0);
    for (U32 i = 0; i < mVehicleNullSurfaces.size(); i++) {
        const NullSurface& rSurface = mNullSurfaces[i];
        PrimBuild::begin(GFXTriangleFan, rSurface.windingCount);
        for (U32 k = 0; k < rSurface.windingCount; k++) {
            PrimBuild::vertex3fv(mPoints[mWindings[rSurface.windingStart + k]].point);
        }
    }
}

void Interior::debugShowTexturesOnly(const ZoneVisDeterminer& zoneVis, SceneGraphData& sgData,
    InteriorInstance* intInst)
{
    GFX->setTextureStageColorOp(1, GFXTOPDisable);
    GFX->setTextureStageColorOp(0, GFXTOPModulate);
    for (U32 i = 0; i < getNumZones(); i++)
    {
        if (zoneVis.isZoneVisible(i) == false)
        {
            continue;
        }
        for (U32 j = 0; j < mZoneRNList[i].renderNodeList.size(); j++)
        {
            RenderNode& node = mZoneRNList[i].renderNodeList[j];
            static U16 curBaseTexIndex = 0;

            // setup base map
            if (node.baseTexIndex)
            {
                curBaseTexIndex = node.baseTexIndex;
            }
            GFX->setTexture(0, mMaterialList->getMaterial(curBaseTexIndex));
            GFX->drawPrimitive(node.primInfoIndex);
        }
    }
}

void Interior::debugShowLargeTextures(const ZoneVisDeterminer& zoneVis, SceneGraphData& sgData,
    InteriorInstance* intInst)
{
    preDebugRender();
    for (U32 i = 0; i < getNumZones(); i++)
    {
        if (zoneVis.isZoneVisible(i) == false)
        {
            continue;
        }
        for (U32 j = 0; j < mZoneRNList[i].renderNodeList.size(); j++)
        {
            RenderNode& node = mZoneRNList[i].renderNodeList[j];
            static U16 curBaseTexIndex = 0;

            // setup lightmap
            if (node.lightMapIndex != U8(-1))
            {
                sgData.lightmap = gInteriorLMManager.getHandle(mLMHandle, intInst->getLMHandle(), node.lightMapIndex);

                /*if( node.exterior )
                {
                    sgData.normLightmap = NULL;
                    sgData.useLightDir = true;
                }
                else
                {*/
                sgData.normLightmap = gInteriorLMManager.getNormalHandle(mLMHandle, intInst->getLMHandle(), node.lightMapIndex);
                sgData.useLightDir = false;
                //}
            }

            // setup base map
            if (node.baseTexIndex)
            {
                curBaseTexIndex = node.baseTexIndex;
            }
            GFXTexHandle t = mMaterialList->getMaterial(curBaseTexIndex);
            ColorF texSizeColor(1.0f, 1.0f, 1.0f, 1.0f);
            if (t) {
                U32 width = t.getWidth();
                U32 height = t.getHeight();
                if (width >= 256 || height >= 256) {
                    if (width == 256 && height == 256) {
                        // small large
                        texSizeColor = ColorF(0.25f, 0.25f, 1.0f);
                    }
                    else if (width != 512 || height != 512) {
                        // thin large
                        texSizeColor = ColorF(0.25f, 1.0f, 0.25f);
                    }
                    else {
                        // oh god.
                        texSizeColor = ColorF(1.0f, 0.25f, 0.25f);
                    }
                }
                else {
                    texSizeColor = ColorF(0.35f, 0.35f, 0.35f);
                }
            }

            GFX->setPixelShaderConstF(PC_USERDEF1, (F32*)&texSizeColor, 1);
            GFX->setTexture(0, t);
            GFX->drawPrimitive(node.primInfoIndex);
        }
    }
}

void Interior::debugShowLightmaps(const ZoneVisDeterminer& zoneVis, SceneGraphData& sgData, InteriorInstance* intInst)
{
    GFX->setTexture(0, mBlankTexture);
    GFX->setTextureStageColorOp(1, GFXTOPModulate);
    GFX->setTextureStageColorOp(0, GFXTOPModulate);

    for (U32 i = 0; i < getNumZones(); i++)
    {
        if (zoneVis.isZoneVisible(i) == false)
        {
            continue;
        }
        for (U32 j = 0; j < mZoneRNList[i].renderNodeList.size(); j++)
        {
            RenderNode& node = mZoneRNList[i].renderNodeList[j];
            // setup lightmap
            if (node.lightMapIndex != U8(-1))
            {
                sgData.lightmap = gInteriorLMManager.getHandle(mLMHandle, intInst->getLMHandle(), node.lightMapIndex);
            }
            GFX->setTexture(1, sgData.lightmap);
            GFX->drawPrimitive(node.primInfoIndex);
        }
    }
}

#endif