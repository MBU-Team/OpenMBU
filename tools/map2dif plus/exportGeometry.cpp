//-----------------------------------------------------------------------------
// Torque Shader Engine
// 
// Copyright (c) 2001 GarageGames.Com
//-----------------------------------------------------------------------------

// this define required for VC++ due to conflict with placement new in STL
//#define __PLACEMENT_NEW_INLINE
#include <algorithm>

#include "map2dif plus/editGeometry.h"
#include "map2dif plus/entityTypes.h"
#include "core/tokenizer.h"
#include "map2dif plus/csgBrush.h"
#include "map2dif plus/editInteriorRes.h"

#include "interior/interior.h"
#include "gfx/gBitmap.h"
#include "core/resManager.h"
#include "math/mMath.h"
#include "materials/materialList.h"
#include "core/bitMatrix.h"

//#include "interior/mirrorSubObject.h"
#include "interior/interiorResObjects.h"
#include "interior/forceField.h"


//------------------------------------------------------------------------------
enum PointClassification {
   LEFT,
   RIGHT,
   BEYOND,
   BEHIND,
   BETWEEN,
   ORIGIN,
   DEST
};

PointClassification classify(const Point3D& start, const Point3D& end, const Point3D& normal, const Point3D& query)
{
   Point3D edge = end   - start;
   Point3D test = query - start;
   if (query == end)
      return DEST;
   if (query == start)
      return ORIGIN;

   Point3D edgeCopy = edge;
   Point3D testCopy = test;
   edgeCopy.normalize();
   testCopy.normalize();
   Point3D cross;
   mCross(edgeCopy, testCopy, &cross);
   if (cross.len() < 0.000001) {
      // Colinear
      F64 dot = mDot(edgeCopy, test);
      if (dot < 0.0) {
         return BEHIND;
      } else {
         F64 len0 = edge.len();
         F64 len1 = test.len();
         if (len0 > len1)
            return BETWEEN;
         else
            return BEYOND;
      }
   } else {
      // Not colinear, test against the normal to find out which side it's on...
      F64 dot = mDot(cross, normal);
      if (dot > 0.0)
         return LEFT;
      else
         return RIGHT;
   }
}


void EditGeometry::giftWrapPortal(Winding& winding, Portal* portal)
{
   if (portal->windings.size() == 1) {
      winding = portal->windings[0];
      return;
   }

   Point3D newPoints[4];   

   F64 minOrthox =  1e12;
   F64 maxOrthox = -1e12;
   F64 minOrthoy =  1e12;
   F64 maxOrthoy = -1e12;

   for (U32 i = 0; i < portal->windings.size(); i++) {
      for (U32 j = 0; j < portal->windings[i].numIndices; j++) {
         const Point3D& rPoint = getPoint(portal->windings[i].indices[j]);
         F64 dotx = mDot(portal->x, rPoint);
         F64 doty = mDot(portal->y, rPoint);

         if (dotx < minOrthox)
            minOrthox = dotx;
         if (dotx > maxOrthox)
            maxOrthox = dotx;
         if (doty < minOrthoy)
            minOrthoy = doty;
         if (doty > maxOrthoy)
            maxOrthoy = doty;
      }
   }
   AssertFatal(minOrthox !=  1e12, "Screwed!");
   AssertFatal(maxOrthox != -1e12, "Screwed!");
   AssertFatal(minOrthoy !=  1e12, "Screwed!");
   AssertFatal(maxOrthoy != -1e12, "Screwed!");

   const Point3D& ref = getPoint(portal->windings[0].indices[0]);
   F64 refx = mDot(ref, portal->x);
   F64 refy = mDot(ref, portal->y);

   newPoints[0] = ref + (portal->x * (minOrthox - refx)) + (portal->y * (minOrthoy - refy));
   newPoints[1] = ref + (portal->x * (minOrthox - refx)) + (portal->y * (maxOrthoy - refy));
   newPoints[2] = ref + (portal->x * (maxOrthox - refx)) + (portal->y * (minOrthoy - refy));
   newPoints[3] = ref + (portal->x * (maxOrthox - refx)) + (portal->y * (maxOrthoy - refy));

   UniqueVector points;
   points.pushBackUnique(insertPoint(newPoints[0]));
   points.pushBackUnique(insertPoint(newPoints[1]));
   points.pushBackUnique(insertPoint(newPoints[2]));
   points.pushBackUnique(insertPoint(newPoints[3]));

   Point3D cross;
   const PlaneEQ& rPlane = getPlaneEQ(portal->planeEQIndex);
   if (mFabs(rPlane.normal.z) < 0.99)
      mCross(rPlane.normal, Point3D(0, 0, 1), &cross);
   else
      mCross(rPlane.normal, Point3D(0, 1, 0), &cross);

   F64 minValue = 1e9;
   S32 a = -1;
   for (U32 i = 0; i < points.size(); i++) {
      F64 dot = mDot(getPoint(points[i]), cross);
      if (dot < minValue) {
         minValue = dot;
         a = i;
      }
   }

   AssertFatal(a != -1, "Wtf!");

   U32 n = points.size();
   U32 storA = a;

   points.push_back(points[a]);
   winding.numIndices = 0;
   for (U32 m = 0; m < n; m++) {
      // Swap (p[a], p[m])
      U32 temp  = points[a];
      points[a] = points[m];
      points[m] = temp;

      winding.indices[winding.numIndices++] = points[m];
      AssertFatal(winding.numIndices <= 32, "Error, too many points in this winding!");

      a = m + 1;
      for (U32 i = m + 2; i <= n; i++) {
         PointClassification c = classify(getPoint(points[m]), getPoint(points[a]), rPlane.normal, getPoint(points[i]));
         if (c == LEFT || c == BEYOND)
            a = i;
      }

      if (a == n)
         break;
   }

   AssertFatal(winding.numIndices >= 3, "Error, bad portal generated!");
}



inline U16 createLeafIndex(U16 baseIndex,
                           bool   isSolid)
{
   AssertFatal(baseIndex <= 0x3fff, "out of range base index.  Uh, oh... Talk to DMoore");

   U16 baseRet;
   if (isSolid)
      baseRet = 0xC000;
   else
      baseRet = 0x8000;

   return (baseRet | baseIndex);
}

inline U16 EditGeometry::remapPlaneIndex(S32 inPlaneIndex)
{
   AssertFatal(inPlaneIndex >= 0 && inPlaneIndex < mPlaneEQs.size(), "Error, out of bounds plane index!");
   AssertFatal(mPlaneRemaps[inPlaneIndex] != -1, "Error, this plane should have been exported, tell DMoore");
   

   U16 ret = mPlaneRemaps[inPlaneIndex];
   AssertFatal(ret < 32768, avar("Error, there are still to many dang planes!  Tell DMoore: %d", ret));
   ret    |= (inPlaneIndex & 1) ? 0x8000 : 0;

   return ret;
}

inline U16 EditGeometry::remapVehiclePlaneIndex(S32 inPlaneIndex, Interior* pRuntime)
{
   const PlaneEQ& rPlane = getPlaneEQ(inPlaneIndex);

   PlaneF newPlane;
   newPlane.x = F32(rPlane.normal.x);
   newPlane.y = F32(rPlane.normal.y);
   newPlane.z = F32(rPlane.normal.z);
   newPlane.d = F32(rPlane.dist) / mWorldEntity->mGeometryScale;

   for (U32 i = 0; i < pRuntime->mVehiclePlanes.size(); i++)
   {
      if (pRuntime->mVehiclePlanes[i].x == newPlane.x &&
          pRuntime->mVehiclePlanes[i].y == newPlane.y &&
          pRuntime->mVehiclePlanes[i].z == newPlane.z &&
          pRuntime->mVehiclePlanes[i].d == newPlane.d)
         return i;
   }
   
   pRuntime->mVehiclePlanes.push_back(newPlane);

   return (pRuntime->mVehiclePlanes.size() - 1);
}

inline U32 makeNullSurfaceIndex(U32 idx)
{
   return idx | 0x80000000;
}

inline U32 makeVehicleNullSurfaceIndex(U32 idx)
{
   return (idx | 0x80000000 | 0x40000000);
}

U16 EditGeometry::exportBSPToRuntime(Interior* pRuntime, EditBSPNode* pNode)
{
   if (pNode->planeEQIndex == -1) {
      if (pNode->isSolid == true) {
         // solid node
         pRuntime->mBSPSolidLeaves.increment();
         Interior::IBSPLeafSolid& rRTLeaf = pRuntime->mBSPSolidLeaves.last();

         // DMM Need to include a list of surfaces affected by this leaf
         rRTLeaf.surfaceIndex = pRuntime->mSolidLeafSurfaces.size();
         rRTLeaf.surfaceCount = 0;
         U32 i;
         for (i = 0; i < mSurfaces.size(); i++) {
            Surface& rSurface = mSurfaces[i];

            bool found = false;
            for (U32 j = 0; j < rSurface.winding.numNodes; j++) {
               if (rSurface.winding.solidNodes[j] == pNode->nodeId) {
                  found = true;
                  break;
               }
            }
            if (found == true) {
               pRuntime->mSolidLeafSurfaces.push_back(i);
               rRTLeaf.surfaceCount++;
            }
         }
         for (i = 0; i < mNullSurfaces.size(); i++) {
            NullSurface& rNullSurface = mNullSurfaces[i];

            bool found = false;
            for (U32 j = 0; j < rNullSurface.winding.numNodes; j++) {
               if (rNullSurface.winding.solidNodes[j] == pNode->nodeId) {
                  found = true;
                  break;
               }
            }
            if (found == true) {
               pRuntime->mSolidLeafSurfaces.push_back(makeNullSurfaceIndex(i));
               rRTLeaf.surfaceCount++;
            }
         }

         return createLeafIndex(pRuntime->mBSPSolidLeaves.size() - 1, true);
      }
      else {
         if (mZones[pNode->zoneId]->active == false) {
            return createLeafIndex(0x0FFF, false);
         } else {
            U32 actives = 0;
            for (U32 i = 0; i < pNode->zoneId; i++)
               if (mZones[i]->active)
                  actives++;
            return createLeafIndex(actives, false);
         }
//
//         AssertFatal(pNode->zoneId < (0xFFFF & ~0xC000), "Error, this index is going to cause problems!");
//         return createLeafIndex(pNode->zoneId, false);
      }
   }
   else {
      // Node
      U16 retIndex = pRuntime->mBSPNodes.size();
      pRuntime->mBSPNodes.increment();
      Interior::IBSPNode& rRTNode = pRuntime->mBSPNodes.last();

      rRTNode.planeIndex = remapPlaneIndex(pNode->planeEQIndex);

      rRTNode.frontIndex = exportBSPToRuntime(pRuntime, pNode->pFront);
      rRTNode.backIndex  = exportBSPToRuntime(pRuntime, pNode->pBack);

      return retIndex;
   }
}

void EditGeometry::exportDMLToRuntime(Interior* pRuntime, Vector<char*>& textureNames)
{
   AssertFatal(pRuntime->mMaterialList == NULL, "Error, material list already exists!");

   Vector<char*> textureNamesCopy;
   for (U32 i = 0; i < textureNames.size(); i++) {
      U32 len = dStrlen(textureNames[i]) + dStrlen(mWorldEntity->mWadPrefix) + 1;
      char* copyBuffer = new char[len];

      dStrcpy(copyBuffer, mWorldEntity->mWadPrefix);
      dStrcat(copyBuffer, textureNames[i]);
      textureNamesCopy.push_back(copyBuffer);
   }

   pRuntime->mMaterialList = new MaterialList(textureNamesCopy.size(), (const char**)textureNamesCopy.address());

   for (U32 i = 0; i < textureNamesCopy.size(); i++)
      delete [] textureNamesCopy[i];
}

U32 EditGeometry::exportPointToRuntime(Interior* pRuntime, const U32 pointIndex)
{
   for (U32 i = 0; i < mExportPointMap.size(); i++) {
      if (mExportPointMap[i].originalIndex == pointIndex)
         return mExportPointMap[i].runtimeIndex;
   }

   mExportPointMap.increment();
   mExportPointMap.last().originalIndex = pointIndex;
   mExportPointMap.last().runtimeIndex  = pRuntime->mPoints.size();
   
   const Point3D& rPoint = getPoint(pointIndex);
   pRuntime->mPoints.increment();
   pRuntime->mPoints.last().point.set(F32(rPoint.x) / mWorldEntity->mGeometryScale,
                                      F32(rPoint.y) / mWorldEntity->mGeometryScale,
                                      F32(rPoint.z) / mWorldEntity->mGeometryScale);

   return mExportPointMap.last().runtimeIndex;
}


U32 EditGeometry::exportVehiclePointToRuntime(Interior* pRuntime, const U32 pointIndex)
{
   for (U32 i = 0; i < mVehicleExportPointMap.size(); i++) {
      if (mVehicleExportPointMap[i].originalIndex == pointIndex)
         return mVehicleExportPointMap[i].runtimeIndex;
   }

   mVehicleExportPointMap.increment();
   mVehicleExportPointMap.last().originalIndex = pointIndex;
   mVehicleExportPointMap.last().runtimeIndex  = pRuntime->mVehiclePoints.size();
   
   const Point3D& rPoint = getPoint(pointIndex);
   pRuntime->mVehiclePoints.increment();
   pRuntime->mVehiclePoints.last().point.set(F32(rPoint.x) / mWorldEntity->mGeometryScale,
                                             F32(rPoint.y) / mWorldEntity->mGeometryScale,
                                             F32(rPoint.z) / mWorldEntity->mGeometryScale);

   return mExportPointMap.last().runtimeIndex;
}

void EditGeometry::exportWindingToRuntime(Interior*      pRuntime,
                                          const Winding& rWinding)
{
   Winding finalWinding;
   finalWinding.numIndices = 0;

   for (U32 i = 0; i < rWinding.numIndices; i++) {
      finalWinding.indices[finalWinding.numIndices++] =
         exportPointToRuntime(pRuntime, rWinding.indices[i]);
   }
   AssertFatal(finalWinding.numIndices == rWinding.numIndices, "Ah, crap.  Paradox.");

   pRuntime->mWindingIndices.increment();
   pRuntime->mWindingIndices.last().windingStart = pRuntime->mWindings.size();
   pRuntime->mWindingIndices.last().windingCount = finalWinding.numIndices;

   for (U32 i = 0; i < finalWinding.numIndices; i++) {
      pRuntime->mWindings.increment();
      pRuntime->mWindings.last() = finalWinding.indices[i];
   }
}

void EditGeometry::exportVehicleWindingToRuntime(Interior*      pRuntime,
                                                 const Winding& rWinding)
{
   Winding finalWinding;
   finalWinding.numIndices = 0;

   for (U32 i = 0; i < rWinding.numIndices; i++) {
      finalWinding.indices[finalWinding.numIndices++] =
         exportVehiclePointToRuntime(pRuntime, rWinding.indices[i]);
   }
   AssertFatal(finalWinding.numIndices == rWinding.numIndices, "Ah, crap.  Paradox.");

   pRuntime->mVehicleWindingIndices.increment();
   pRuntime->mVehicleWindingIndices.last().windingStart = pRuntime->mVehicleWindings.size();
   pRuntime->mVehicleWindingIndices.last().windingCount = finalWinding.numIndices;

   for (U32 i = 0; i < finalWinding.numIndices; i++) {
      pRuntime->mVehicleWindings.increment();
      pRuntime->mVehicleWindings.last() = finalWinding.indices[i];
   }
}

U32 EditGeometry::exportEmitStringToRuntime(Interior* pRuntime,
                                            const U8* pString,
                                            const U32 stringLen)
{
   // Search for already included strings.
   if (pRuntime->mConvexHullEmitStrings.size() > stringLen) {
      for (S32 i = 0; i < pRuntime->mConvexHullEmitStrings.size() - stringLen; i++) {
         if (dMemcmp(&pRuntime->mConvexHullEmitStrings[i], pString, stringLen) == 0)
            return i;
      }
   }

   // Insert the string
   U32 insertPoint = pRuntime->mConvexHullEmitStrings.size();

   if (pRuntime->mConvexHullEmitStrings.size() + stringLen > 
       pRuntime->mConvexHullEmitStrings.capacity()) {
      if (pRuntime->mConvexHullEmitStrings.capacity() == 0)
         pRuntime->mConvexHullEmitStrings.reserve(1);

      pRuntime->mConvexHullEmitStrings.reserve(pRuntime->mConvexHullEmitStrings.capacity() * 2);
   }

   pRuntime->mConvexHullEmitStrings.setSize(pRuntime->mConvexHullEmitStrings.size() + stringLen);
   dMemcpy(&pRuntime->mConvexHullEmitStrings[insertPoint],
           pString, stringLen);
   return insertPoint;
}


U32 EditGeometry::exportVehicleEmitStringToRuntime(Interior* pRuntime,
                                                   const U8* pString,
                                                   const U32 stringLen)
{
   // Search for already included strings.
   if (pRuntime->mVehicleConvexHullEmitStrings.size() > stringLen) {
      for (S32 i = 0; i < pRuntime->mVehicleConvexHullEmitStrings.size() - stringLen; i++) {
         if (dMemcmp(&pRuntime->mVehicleConvexHullEmitStrings[i], pString, stringLen) == 0)
            return i;
      }
   }

   // Insert the string
   U32 insertPoint = pRuntime->mVehicleConvexHullEmitStrings.size();

   if (pRuntime->mVehicleConvexHullEmitStrings.size() + stringLen > 
       pRuntime->mVehicleConvexHullEmitStrings.capacity()) {
      if (pRuntime->mVehicleConvexHullEmitStrings.capacity() == 0)
         pRuntime->mVehicleConvexHullEmitStrings.reserve(1);

      pRuntime->mVehicleConvexHullEmitStrings.reserve(pRuntime->mVehicleConvexHullEmitStrings.capacity() * 2);
   }

   pRuntime->mVehicleConvexHullEmitStrings.setSize(pRuntime->mVehicleConvexHullEmitStrings.size() + stringLen);
   dMemcpy(&pRuntime->mVehicleConvexHullEmitStrings[insertPoint],
           pString, stringLen);
   return insertPoint;
}


U16 EditGeometry::getMaterialIndex(const char* texName) const
{
   for (U32 i = 0; i < mTextureNames.size(); i++) {
      if (dStricmp(mTextureNames[i], texName) == 0)
         return i;
   }

   return 0xFFFF;
}

//------------------------------------------------------------------------------
// a surface is considered inside if the winding is in a non-0 zone
static bool isSurfaceInside(EditGeometry::Surface & surface)
{
   for(U32 i = 0; i < surface.winding.numZoneIds; i++)
      if(surface.winding.zoneIds[i] != 0)
         return(true);
   return(false);
}

//------------------------------------------------------------------------------
void EditGeometry::fillInLightmapInfo(Surface& rSurface)
{
   AssertFatal(rSurface.textureIndex != 0, "Error, shouldn't get these here!");

      // set surface inside flag
   rSurface.isInside = isSurfaceInside(rSurface);
   
   F32 lumelScale = rSurface.isInside ? mWorldEntity->mInsideLumelScale : 
                    mWorldEntity->mOutsideLumelScale;


   Point3D axises[3] = {
      Point3D(1, 0, 0),
      Point3D(0, 1, 0),
      Point3D(0, 0, 1)
   };
   F64 bestDot = -1;
   S32  bestIndex = -1;
   for (U32 i = 0; i < 3; i++) {
      F64 dot = mFabs(mDot(getPlaneEQ(rSurface.planeIndex).normal, axises[i]));
      if (dot > bestDot) {
         bestDot   = dot;
         bestIndex = i;
      }
   }
   AssertFatal(bestIndex != -1 && bestDot > 0.0, "Paradox!  Talk to DMoore");

   S32 sc, tc;
   if (axises[bestIndex].x != 0.0) {
      sc = 1;
      tc = 2;
   } else if (axises[bestIndex].y != 0.0) {
      sc = 0;
      tc = 2;
   } else {
      sc = 0;
      tc = 1;
   }

   F64 coords[MaxWindingPoints * 3];
   for (U32 i = 0; i < rSurface.winding.numIndices; i++) {
      coords[i * 3 + 0] = getPoint(rSurface.winding.indices[i]).x;
      coords[i * 3 + 1] = getPoint(rSurface.winding.indices[i]).y;
      coords[i * 3 + 2] = getPoint(rSurface.winding.indices[i]).z;
   }

   F64 minS =  1e10; S32 minSIndex = -1;
   F64 maxS = -1e10; S32 maxSIndex = -1;
   F64 minT =  1e10; S32 minTIndex = -1;
   F64 maxT = -1e10; S32 maxTIndex = -1;
   for (U32 i = 0; i < rSurface.winding.numIndices; i++) {
      if (coords[i*3 + sc] < minS) {
         minS      = coords[i*3 + sc];
         minSIndex = i;
      }
      if (coords[i*3 + sc] > maxS) {
         maxS      = coords[i*3 + sc];
         maxSIndex = i;
      }
      if (coords[i*3 + tc] < minT) {
         minT      = coords[i*3 + tc];
         minTIndex = i;
      }
      if (coords[i*3 + tc] > maxT) {
         maxT      = coords[i*3 + tc];
         maxTIndex = i;
      }
   }
   AssertFatal(minSIndex != -1 && maxSIndex != -1 && minTIndex != -1 && maxTIndex != -1, "Paradox!");

   // Points in the lmap coord space that represent the bounds of the poly
   F64 virtualMin[2], virtualMax[2], desiredStart[2], desiredEnd[2];
   virtualMin[0] = coords[minSIndex*3 + sc];
   virtualMin[1] = coords[minTIndex*3 + tc];
   virtualMax[0] = coords[maxSIndex*3 + sc];
   virtualMax[1] = coords[maxTIndex*3 + tc];

   for (U32 i = 0; i < 2; i++) {
      desiredStart[i] = virtualMin[i] / lumelScale;
      if (desiredStart[i] - mFloor(desiredStart[i]) < 0.5) {
         desiredStart[i] = mFloor(desiredStart[i] - 1.0);
      } else {
         desiredStart[i] = mFloor(desiredStart[i]);
      }
   }
   for (U32 i = 0; i < 2; i++) {
      desiredEnd[i] = virtualMax[i] / lumelScale;
      if (mCeil(desiredEnd[i]) - desiredEnd[i] < 0.5) {
         desiredEnd[i] = mCeil(desiredEnd[i] + 1.0);
      } else {
         desiredEnd[i] = mCeil(desiredEnd[i]);
      }
   }
   // Make sure we don't lose precision to errant FP rounding.  We now
   //  know how large the lm section should be...
   rSurface.lMapDimX = U32(desiredEnd[0] - desiredStart[0] + 0.5);
   rSurface.lMapDimY = U32(desiredEnd[1] - desiredStart[1] + 0.5);
   AssertISV(rSurface.lMapDimX <= 256 && rSurface.lMapDimY <= 256, avar(
      "Light map too large: %d x %d (where max is 256 x 256). Your have a brush that is too large, or your light map scale value may be too small.",
      rSurface.lMapDimX, rSurface.lMapDimY));
   AssertFatal(desiredEnd[0] > desiredStart[0] && desiredEnd[1] > desiredStart[1] &&
               S32(rSurface.lMapDimX) > 0 && S32(rSurface.lMapDimY) > 0, "Aw, crap, a paradox.  Please talk to DMoore");

   desiredStart[0] *= lumelScale;
   desiredStart[1] *= lumelScale;
   desiredEnd[0]   *= lumelScale;
   desiredEnd[1]   *= lumelScale;

   for (U32 i = 0; i < 4; i++) {
      rSurface.lmapTexGenX[i] = 0.0f;
      rSurface.lmapTexGenY[i] = 0.0f;
   }

   rSurface.tempScale[0] = rSurface.tempScale[1] = 1.0 / lumelScale;

   rSurface.lmapTexGenX[sc] = F32(1.0 / lumelScale);
   rSurface.lmapTexGenX[3]  = F32(-desiredStart[0] / lumelScale);

   rSurface.lmapTexGenY[tc] = F32(1.0 / lumelScale);
   rSurface.lmapTexGenY[3]  = F32(-desiredStart[1] / lumelScale);

   if (rSurface.lMapDimX < rSurface.lMapDimY) {
      // Always orient surfaces so that they lay down the long way...
      rSurface.lmapTexGenSwapped = true;
      U32 iTemp = rSurface.lMapDimX;
      rSurface.lMapDimX = rSurface.lMapDimY;
      rSurface.lMapDimY = iTemp;

      F32 fTemp;
      for (U32 i = 0; i < 4; i++) {
         fTemp = rSurface.lmapTexGenX[i];
         rSurface.lmapTexGenX[i] = rSurface.lmapTexGenY[i];
         rSurface.lmapTexGenY[i] = fTemp;
      }
   } else {
      rSurface.lmapTexGenSwapped = false;
   }

   rSurface.pLightDirMap   = new GBitmap(rSurface.lMapDimX, rSurface.lMapDimY);
   rSurface.pNormalLMap    = new GBitmap(rSurface.lMapDimX, rSurface.lMapDimY);
   rSurface.pAlarmLMap     = new GBitmap(rSurface.lMapDimX, rSurface.lMapDimY);
}

#define SMALL_FLOAT (1e-12)

//------------------------------------------------------------------------------
// fill in texture-space matrix - for bump-mapping
//------------------------------------------------------------------------------
void EditGeometry::fillInTexSpaceMat( Surface& surface )
{
   const PlaneEQ & plane = getPlaneEQ(surface.planeIndex);

   TexGenPlanes texPlanes = mTexGenEQs[surface.texGenIndex];


   Point3F pts[3];
   Point2F uv[3];

   for( U32 i=0; i<3; i++ )
   {
      pts[i] = mPoints[ surface.winding.indices[i] ];

      uv[i].x = pts[i].x * texPlanes.planeX.x +
                pts[i].y * texPlanes.planeX.y +
                pts[i].z * texPlanes.planeX.z +
                texPlanes.planeX.d;

      uv[i].y = pts[i].x * texPlanes.planeY.x +
                pts[i].y * texPlanes.planeY.y +
                pts[i].z * texPlanes.planeY.z +
                texPlanes.planeY.d;
   }

   Point3F edge1, edge2;
   Point3F cp;
   Point3F S,T,SxT;

   // x, s, t
	edge1.set( pts[1].x - pts[0].x, uv[1].x - uv[0].x, uv[1].y - uv[0].y );
	edge2.set( pts[2].x - pts[0].x, uv[2].x - uv[0].x, uv[2].y - uv[0].y );

   mCross( edge1, edge2, &cp );
   if( fabs(cp.x) > SMALL_FLOAT )
   {
		S.x = -cp.y / cp.x;
		T.x = -cp.z / cp.x;
   }

	edge1.set( pts[1].y - pts[0].y, uv[1].x - uv[0].x, uv[1].y - uv[0].y );
	edge2.set( pts[2].y - pts[0].y, uv[2].x - uv[0].x, uv[2].y - uv[0].y );

   mCross( edge1, edge2, &cp );
   if( fabs(cp.x) > SMALL_FLOAT )
   {
		S.y = -cp.y / cp.x;
		T.y = -cp.z / cp.x;
   }

	edge1.set( pts[1].z - pts[0].z, uv[1].x - uv[0].x, uv[1].y - uv[0].y );
	edge2.set( pts[2].z - pts[0].z, uv[2].x - uv[0].x, uv[2].y - uv[0].y );

   mCross( edge1, edge2, &cp );
   if( fabs(cp.x) > SMALL_FLOAT )
   {
		S.z = -cp.y / cp.x;
		T.z = -cp.z / cp.x;
   }

   Point3F planeNormal(plane.normal.x, plane.normal.y, plane.normal.z);
   S.normalizeSafe();
   T.normalizeSafe();
   mCross( S, T, &SxT );
   if( mDot( SxT, planeNormal ) < 0.0 )
   {
      SxT = -SxT;
   }


   surface.texSpace.identity();

   surface.texSpace.setRow( 0, S );
   surface.texSpace.setRow( 1, T );
   surface.texSpace.setRow( 2, SxT );
}

void EditGeometry::adjustLMapTexGen(Surface& rSurface)
{
   F32 lumelScale = rSurface.isInside ? mWorldEntity->mInsideLumelScale : 
                    mWorldEntity->mOutsideLumelScale;

   F64 scaleX = 1.0 / F64(mLightmaps[rSurface.sheetIndex]->getWidth()  * lumelScale);
   F64 scaleY = 1.0 / F64(mLightmaps[rSurface.sheetIndex]->getHeight() * lumelScale);

   U32 i;
   for (i = 0; i < 4; i++) {
      rSurface.lmapTexGenX[i] /= rSurface.tempScale[0];
      rSurface.lmapTexGenY[i] /= rSurface.tempScale[1];

      rSurface.lmapTexGenX[i] *= scaleX;
      rSurface.lmapTexGenY[i] *= scaleY;
   }

   rSurface.lmapTexGenX[3] += F64(rSurface.offsetX) / F64(mLightmaps[rSurface.sheetIndex]->getWidth());
   rSurface.lmapTexGenY[3] += F64(rSurface.offsetY) / F64(mLightmaps[rSurface.sheetIndex]->getHeight());

   for (i = 0; i < 3; i++) {
      rSurface.lmapTexGenX[i] *= mWorldEntity->mGeometryScale;
      rSurface.lmapTexGenY[i] *= mWorldEntity->mGeometryScale; 
   }
}


void EditGeometry::dumpSurfaceToRuntime(Interior* pRuntime,
                                        Surface&  editSurface)
{
   AssertFatal(editSurface.sheetIndex != 0xFFFFFFFF, "Error, shouldn't get these here!");

   pRuntime->mSurfaces.increment();
   pRuntime->mLMTexGenEQs.increment();
   Interior::Surface& rSurface       = pRuntime->mSurfaces.last();
   Interior::TexGenPlanes& rLMPlanes = pRuntime->mLMTexGenEQs.last();

   rSurface.planeIndex   = remapPlaneIndex(editSurface.planeIndex);
   rSurface.textureIndex = editSurface.textureIndex;

   rLMPlanes.planeX.x = editSurface.lmapTexGenX[0];
   rLMPlanes.planeX.y = editSurface.lmapTexGenX[1];
   rLMPlanes.planeX.z = editSurface.lmapTexGenX[2];
   rLMPlanes.planeX.d = editSurface.lmapTexGenX[3];
   rLMPlanes.planeY.x = editSurface.lmapTexGenY[0];
   rLMPlanes.planeY.y = editSurface.lmapTexGenY[1];
   rLMPlanes.planeY.z = editSurface.lmapTexGenY[2];
   rLMPlanes.planeY.d = editSurface.lmapTexGenY[3];
   rSurface.texGenIndex = editSurface.texGenIndex;

   // We used only runtime flags in the surface, so this is fine...
   rSurface.surfaceFlags = editSurface.flags & Interior::SurfaceFlagMask;

   rSurface.fanMask      = editSurface.fanMask;

   // Ok, this is an abuse, but it's easy.  Export to the trifans,
   //  then steal the last one.
   exportWindingToRuntime(pRuntime, editSurface.winding);
   rSurface.windingStart = pRuntime->mWindingIndices.last().windingStart;
   rSurface.windingCount = pRuntime->mWindingIndices.last().windingCount;
   pRuntime->mWindingIndices.decrement();

   AssertFatal(editSurface.lMapDimX < 256 && editSurface.lMapDimY < 256,
               "Error, lightmap too large to fit into 8 bits!");
   AssertFatal(editSurface.offsetX < 256 && editSurface.offsetY < 256,
               "Error, lightmap too large to fit into 8 bits! (Plus, that's just plain wierd)");

   rSurface.lightCount          = editSurface.numLights;
   rSurface.lightStateInfoStart = editSurface.stateDataStart;

   rSurface.mapSizeX   = editSurface.lMapDimX;
   rSurface.mapSizeY   = editSurface.lMapDimY;
   rSurface.mapOffsetX = editSurface.offsetX;
   rSurface.mapOffsetY = editSurface.offsetY;
}

void EditGeometry::dumpNullSurfaceToRuntime(Interior* pRuntime,
                                            EditGeometry::NullSurface& editSurface)
{
   pRuntime->mNullSurfaces.increment();
   Interior::NullSurface& rSurface = pRuntime->mNullSurfaces.last();
   rSurface.planeIndex   = remapPlaneIndex(editSurface.planeIndex);
   rSurface.surfaceFlags = editSurface.flags;

   exportWindingToRuntime(pRuntime, editSurface.winding);
   rSurface.windingStart = pRuntime->mWindingIndices.last().windingStart;
   rSurface.windingCount = pRuntime->mWindingIndices.last().windingCount;
   pRuntime->mWindingIndices.decrement();
}


void EditGeometry::dumpVehicleNullSurfaceToRuntime(Interior* pRuntime,
                                                   EditGeometry::NullSurface& editSurface)
{
   pRuntime->mVehicleNullSurfaces.increment();
   Interior::NullSurface& rSurface = pRuntime->mVehicleNullSurfaces.last();
   rSurface.planeIndex   = remapVehiclePlaneIndex(editSurface.planeIndex, pRuntime);
   rSurface.surfaceFlags = editSurface.flags;

   exportVehicleWindingToRuntime(pRuntime, editSurface.winding);
   rSurface.windingStart = pRuntime->mVehicleWindingIndices.last().windingStart;
   rSurface.windingCount = pRuntime->mVehicleWindingIndices.last().windingCount;
   pRuntime->mVehicleWindingIndices.decrement();
}

void EditGeometry::dumpTriggerToRuntime(Interior*         /*pRuntime*/,
                                        InteriorResource* pResource,
                                        TriggerEntity*    pEntity)
{
   AssertISV(mWorldEntity->mDetailNumber == 0, "Error, triggers only go on the 0 detail level!");
   EditInteriorResource* pEditResource = dynamic_cast<EditInteriorResource*>(pResource);

   InteriorResTrigger* pTrigger = new InteriorResTrigger();
   dStrcpy(pTrigger->mName, pEntity->mName);
   pTrigger->mDataBlock  = StringTable->insert(pEntity->mDataBlock);
   pTrigger->mPolyhedron = pEntity->triggerPolyhedron;
   pTrigger->mOffset.x   = pEntity->mOrigin.x;
   pTrigger->mOffset.y   = pEntity->mOrigin.y;
   pTrigger->mOffset.z   = pEntity->mOrigin.z;
   pTrigger->mDictionary = pEntity->mDictionary;

   pEditResource->insertTrigger(pTrigger);
}

//void EditGeometry::dumpAISpecialToRuntime(Interior*            /*pRuntime*/,
//                                          InteriorResource*    pResource,
//                                          SpecialNodeEntity*   pEntity)
//{
//   AssertISV(mWorldEntity->mDetailNumber == 0, "Error, SpecialNodes only go on the 0 detail level!");
//   EditInteriorResource* pEditResource = dynamic_cast<EditInteriorResource*>(pResource);
//   
//   AISpecialNode *pSpecialNode = new AISpecialNode();
//   pSpecialNode->mName = StringTable->insert(pEntity->mName);
//   pSpecialNode->mPos.x   = (pEntity->mOrigin.x / mWorldEntity->mGeometryScale);
//   pSpecialNode->mPos.y   = (pEntity->mOrigin.y / mWorldEntity->mGeometryScale);
//   pSpecialNode->mPos.z   = (pEntity->mOrigin.z / mWorldEntity->mGeometryScale);
//
//   pEditResource->insertSpecialNode(pSpecialNode);
//}

void EditGeometry::dumpGameEntityToRuntime(Interior*            /*pRuntime*/,
                                          InteriorResource*    pResource,
                                          GameEntity*   pEntity)
{
   AssertISV(mWorldEntity->mDetailNumber == 0, "Error, GameEntities only go on the 0 detail level!");
   EditInteriorResource* pEditResource = dynamic_cast<EditInteriorResource*>(pResource);
   
   ItrGameEntity *pGameEntity = new ItrGameEntity();
   pGameEntity->mDataBlock = StringTable->insert(pEntity->mDataBlock);
   pGameEntity->mGameClass = StringTable->insert(pEntity->mGameClass);
   pGameEntity->mDictionary = pEntity->mDictionary;
   pGameEntity->mPos.x   = (pEntity->mOrigin.x / mWorldEntity->mGeometryScale);
   pGameEntity->mPos.y   = (pEntity->mOrigin.y / mWorldEntity->mGeometryScale);
   pGameEntity->mPos.z   = (pEntity->mOrigin.z / mWorldEntity->mGeometryScale);

   pEditResource->insertGameEntity(pGameEntity);
}

void EditGeometry::dumpDoorToRuntime(Interior*         /*pRuntime*/,
                                     InteriorResource* pResource,
                                     DoorEntity*       pEntity)
{
   AssertISV(mWorldEntity->mDetailNumber == 0, "Error, doors only are on the 0 detail level!");
   EditInteriorResource* pEditResource = dynamic_cast<EditInteriorResource*>(pResource);

   if (pEntity->mInterior == NULL)
      return;

   // We need to set the following:
   //  Resource index (name will be patched in by morian later.
   //  Parent SO and Path indices (name patched in at runtime)
   InteriorPathFollower* pChild = new InteriorPathFollower;

   pEditResource->insertPathedChild(pChild);
   pChild->mName             = StringTable->insert(pEntity->mName);
   pChild->mDataBlock        = StringTable->insert(pEntity->mDataBlock);
   pChild->mInteriorResIndex = pEditResource->insertSubObject(pEntity->mInterior);
   pChild->mOffset.x         = pEntity->mOrigin.x;
   pChild->mOffset.y         = pEntity->mOrigin.y;
   pChild->mOffset.z         = pEntity->mOrigin.z;
   pChild->mDictionary       = pEntity->mDictionary;

   for(U32 i = 0; i < pEntity->mWayPoints.size(); i++)
      pEntity->mWayPoints[i].pos /= mWorldEntity->mGeometryScale;      
   pChild->mWayPoints = pEntity->mWayPoints;

   U32 curr = 0;
   pChild->mTriggerIds = pEntity->mTriggerIds;
   pEntity->mInterior        = NULL;
}


//void EditGeometry::dumpForceFieldToRuntime(Interior*         /*pRuntime*/,
//                                           InteriorResource* pResource,
//                                           ForceFieldEntity* pEntity)
//{
//   AssertISV(mWorldEntity->mDetailNumber == 0, "Error, force fields can only be on the 0 detail level!");
//   EditInteriorResource* pEditResource = dynamic_cast<EditInteriorResource*>(pResource);
//
//   if (pEntity->mInterior == NULL)
//      return;
//
//   // The process command for this object has created a nice interior object for us, 
//   //  let's copy out only the parts we want.  Note that this is just about
//   //  as non-optimal as you can imagine, but hey, it's easy and it works.
//   ForceField* pField = new ForceField;
//
//   pField->mBoundingBox    = pEntity->mInterior->mBoundingBox;
//   pField->mBoundingSphere = pEntity->mInterior->mBoundingSphere;
//
//   pField->mPlanes = pEntity->mInterior->mPlanes;
//
//   pField->mPoints.setSize(pEntity->mInterior->mPoints.size());
//   for (U32 i = 0; i < pEntity->mInterior->mPoints.size(); i++)
//      pField->mPoints[i] = pEntity->mInterior->mPoints[i].point;
//
//   pField->mBSPNodes.setSize(pEntity->mInterior->mBSPNodes.size());
//   pField->mBSPSolidLeaves.setSize(pEntity->mInterior->mBSPSolidLeaves.size());
//   for (U32 i = 0; i < pEntity->mInterior->mBSPNodes.size(); i++) {
//      pField->mBSPNodes[i].planeIndex = pEntity->mInterior->mBSPNodes[i].planeIndex;
//      pField->mBSPNodes[i].frontIndex = pEntity->mInterior->mBSPNodes[i].frontIndex;
//      pField->mBSPNodes[i].backIndex  = pEntity->mInterior->mBSPNodes[i].backIndex;
//   }
//   for (U32 i = 0; i < pEntity->mInterior->mBSPSolidLeaves.size(); i++) {
//      pField->mBSPSolidLeaves[i].surfaceIndex = pEntity->mInterior->mBSPSolidLeaves[i].surfaceIndex;
//      pField->mBSPSolidLeaves[i].surfaceCount = pEntity->mInterior->mBSPSolidLeaves[i].surfaceCount;
//   }
//   pField->mSolidLeafSurfaces = pEntity->mInterior->mSolidLeafSurfaces;
//
//   pField->mWindings = pEntity->mInterior->mWindings;
//
//   pField->mSurfaces.setSize(pEntity->mInterior->mSurfaces.size());
//   for (U32 i = 0; i < pEntity->mInterior->mSurfaces.size(); i++) {
//      pField->mSurfaces[i].windingStart = pEntity->mInterior->mSurfaces[i].windingStart;
//      pField->mSurfaces[i].fanMask      = pEntity->mInterior->mSurfaces[i].fanMask;
//      pField->mSurfaces[i].planeIndex   = pEntity->mInterior->mSurfaces[i].planeIndex;
//      pField->mSurfaces[i].windingCount = pEntity->mInterior->mSurfaces[i].windingCount;
//      pField->mSurfaces[i].surfaceFlags = pEntity->mInterior->mSurfaces[i].surfaceFlags;
//   }
//
//   pEditResource->insertField(pField);
//   pField->mName = StringTable->insert(pEntity->mName);
//   pField->mColor = pEntity->mColor;
//
//   U32 curr = 0;
//   while (pEntity->mTrigger[curr][0] != '\0')
//      pField->mTriggers.push_back(StringTable->insert(pEntity->mTrigger[curr++]));
//}


void EditGeometry::createBoundingVolumes(Interior* pRuntime)
{
   // Really slow.  ah, well...
   Box3F   boundBox(Point3F(1e10, 1e10, 1e10), Point3F(-1e10, -1e10, -1e10), true);
   Point3F centroid(0, 0, 0);

   for (U32 i = 0; i < pRuntime->mPoints.size(); i++) {
      boundBox.min.setMin(pRuntime->mPoints[i].point);
      boundBox.max.setMax(pRuntime->mPoints[i].point);

      centroid += pRuntime->mPoints[i].point;
   }
   centroid /= F32(pRuntime->mPoints.size());

   F32 maxRSq = 0.0f;
   for (U32 i = 0; i < pRuntime->mPoints.size(); i++) {
      F32 lenSQ = (pRuntime->mPoints[i].point - centroid).lenSquared();
      if (lenSQ > maxRSq)
         maxRSq = lenSQ;
   }
   F32 r = mSqrt(maxRSq);

   SphereF boundSphere(centroid, r);

   pRuntime->mBoundingBox    = boundBox;
   pRuntime->mBoundingSphere = boundSphere;
}

void remapRecurse(EditBSPNode* node, Vector<S32>& remapVector, U32& currIndex)
{
   if (node->planeEQIndex == -1)
      return;

   S32 planeIndex = node->planeEQIndex;
   planeIndex &= ~1;

   if (remapVector[planeIndex] == -1) {
      remapVector[planeIndex]     = currIndex;
      remapVector[planeIndex | 1] = currIndex++;
   }
   
   remapRecurse(node->pFront, remapVector, currIndex);
   remapRecurse(node->pBack,  remapVector, currIndex);
}

void EditGeometry::exportPlanes(Interior* pRuntime)
{
//   AssertFatal(false, avar("foeey: %d", mPlaneEQs.size()));

   mPlaneRemaps.setSize(mPlaneEQs.size());

   for (U32 i = 0; i < mPlaneEQs.size(); i++) {
      mPlaneRemaps[i] = -1;
   }

   U32 currIndex = 0;

   // Loop through all structures that have planeIndices.
   //  We'll be keeping only those planes that are actually used.
   for (U32 i = 0; i < mSurfaces.size(); i++) {
      S32 planeIndex = mSurfaces[i].planeIndex;
      planeIndex &= ~1;

      if (mPlaneRemaps[planeIndex] == -1) {
         mPlaneRemaps[planeIndex]     = currIndex;
         mPlaneRemaps[planeIndex | 1] = currIndex++;
      }
   }
   for (U32 i = 0; i < mNullSurfaces.size(); i++) {
      S32 planeIndex = mNullSurfaces[i].planeIndex;
      planeIndex &= ~1;

      if (mPlaneRemaps[planeIndex] == -1) {
         mPlaneRemaps[planeIndex]     = currIndex;
         mPlaneRemaps[planeIndex | 1] = currIndex++;
      }
   }
   for (U32 i = 0; i < mPortals.size(); i++) {
      S32 planeIndex = mPortals[i]->planeEQIndex;
      planeIndex &= ~1;

      if (mPlaneRemaps[planeIndex] == -1) {
         mPlaneRemaps[planeIndex]     = currIndex;
         mPlaneRemaps[planeIndex | 1] = currIndex++;
      }
   }
   for (U32 i = 0; i < mStructuralBrushes.size(); i++) {
      CSGBrush* pBrush = mStructuralBrushes[i];

      for (U32 j = 0; j < pBrush->mPlanes.size(); j++) {
         S32 planeIndex = pBrush->mPlanes[j].planeEQIndex;
         planeIndex &= ~1;

         if (mPlaneRemaps[planeIndex] == -1) {
            mPlaneRemaps[planeIndex]     = currIndex;
            mPlaneRemaps[planeIndex | 1] = currIndex++;
         }
      }
   }
   for (U32 i = 0; i < mDetailBrushes.size(); i++) {
      CSGBrush* pBrush = mDetailBrushes[i];

      for (U32 j = 0; j < pBrush->mPlanes.size(); j++) {
         S32 planeIndex = pBrush->mPlanes[j].planeEQIndex;
         planeIndex &= ~1;

         if (mPlaneRemaps[planeIndex] == -1) {
            mPlaneRemaps[planeIndex]     = currIndex;
            mPlaneRemaps[planeIndex | 1] = currIndex++;
         }
      }
   }

   remapRecurse(mBSPRoot, mPlaneRemaps, currIndex);

   AssertFatal(currIndex < (1 << 16), "Error, too many unique planes, must be less than (1 << 16)");
   pRuntime->mPlanes.setSize(currIndex);
   for (U32 i = 0; i < mPlaneEQs.size(); i += 2) {
      // Export if the remap vector is not -1
      if (mPlaneRemaps[i] != -1) {
         const PlaneEQ& rPlane = mPlaneEQs[i];

         PlaneF& rRTPlane = pRuntime->mPlanes[mPlaneRemaps[i]];
         rRTPlane.x = F32(rPlane.normal.x);
         rRTPlane.y = F32(rPlane.normal.y);
         rRTPlane.z = F32(rPlane.normal.z);
         rRTPlane.d = F32(rPlane.dist) / mWorldEntity->mGeometryScale;
      }
   }
}


static S32 QSORT_CALLBACK cmpAnimatedLights(const void* a,const void* b)
{
   const Interior::AnimatedLight* pLight1 = reinterpret_cast<const Interior::AnimatedLight*>(a);
   const Interior::AnimatedLight* pLight2 = reinterpret_cast<const Interior::AnimatedLight*>(b);

   if ((pLight1->flags & Interior::AnimationAmbient) == (pLight2->flags & Interior::AnimationAmbient)) {
      return 0;
   } else {
      if (pLight1->flags & Interior::AnimationAmbient) {
         // Light1 sorts to back
         return 1;
      } else {
         return -1;
      }
   }
}


U32 EditGeometry::exportIntensityMap(Interior* pRuntime, GBitmap* bmp)
{
   if (bmp == NULL)
      return 0xFFFFFFFF;

   AssertFatal(bmp->getFormat() == GFXFormatA8, avar("Error, wrong format! (%d)", bmp->getFormat()));

   U32 retIndex = pRuntime->mStateDataBuffer.size();
   pRuntime->mStateDataBuffer.setSize(retIndex + (bmp->getWidth() * bmp->getHeight()));

   dMemcpy(&pRuntime->mStateDataBuffer[retIndex], bmp->getWritableBits(),
           bmp->getWidth() * bmp->getHeight());

   return retIndex;
}

//--------------------------------------------------------------------------
void EditGeometry::preprocessLighting()
{
   for (U32 i = 0; i < mSurfaces.size(); i++)
      mSurfaces[i].numLights = 0;

   if (mAnimatedLights.size() == 0)
      return;

   // We have to make sure of the following:
   //  Each state has at least a NULL state data for each surface that ANY state
   //   affects.
   //  Each state data has the appropriate stateDataIndex
   //  Each surface has the appropriate stateDataIndex
   //
   BitVector surfacesAffected;
   surfacesAffected.setSize(mSurfaces.size());

   BitMatrix surfaceLightPairs(mSurfaces.size(), mAnimatedLights.size());
   surfaceLightPairs.clearAllBits();

   // First, make sure all the NULL states are filled in if necessary
   for (U32 i = 0; i < mAnimatedLights.size(); i++) {
      AnimatedLight* pLight = mAnimatedLights[i];

      surfacesAffected.clear();
      for (U32 j = 0; j < pLight->states.size(); j++) {
         LightState* pState = pLight->states[j];

         for (U32 k = 0; k < pState->stateData.size(); k++) {
            if (pState->stateData[k].pLMap != NULL) {
               surfacesAffected.set(pState->stateData[k].surfaceIndex);
               surfaceLightPairs.setBit(pState->stateData[k].surfaceIndex, i);
            }
         }
      }

      // Now go back and fill in any missing states
      for (U32 j = 0; j < pLight->states.size(); j++) {
         LightState* pState = pLight->states[j];

         for (U32 k = 0; k < mSurfaces.size(); k++) {
            if (surfacesAffected.test(k)) {
               // Need to make sure there is a state data for k in this state.
               bool found = false;
               for (U32 l = 0; l < pState->stateData.size(); l++) {
                  if (pState->stateData[l].surfaceIndex == k) {
                     found = true;
                     break;
                  }
               }
               if (found == false) {
                  pState->stateData.increment();
            
                  pState->stateData.last().pLMap          = NULL;
                  pState->stateData.last().surfaceIndex   = k;
                  pState->stateData.last().stateDataIndex = 0;
               }
            }
         }
      }
   }

   // Ok, we now have complete statedata information, mark the number of lights
   //  on all surfaces.  This determines the start point for the statedata.
   for (U32 i = 0; i < mSurfaces.size(); i++) {
      for (U32 j = 0; j < mAnimatedLights.size(); j++) {
         if (surfaceLightPairs.isSet(i, j))
            mSurfaces[i].numLights++;
      }
   }
}
void EditGeometry::postProcessLighting(Interior* pRuntime)
{
   if(!mSurfaces.size())
      return;

   BitVector surfacesAffected;
   surfacesAffected.setSize(mSurfaces.size());

   mSurfaces[0].stateDataStart = 0;
   for (U32 i = 1; i < mSurfaces.size(); i++) {
      mSurfaces[i].stateDataStart = mSurfaces[i-1].stateDataStart +
         mSurfaces[i-1].numLights;
   }

   // OK, now, all surfaces are marked with the appropriate stateDataStart point,
   //  we need to zip through the lights again, and fill in the stateDataIndex field
   //  on the state data.
   //
   for (U32 i = 0; i < mAnimatedLights.size(); i++) {
      AnimatedLight* pLight = mAnimatedLights[i];

      surfacesAffected.clear();
      for (U32 j = 0; j < pLight->states.size(); j++) {
         for (U32 k = 0; k < pLight->states[j]->stateData.size(); k++) {
            pLight->states[j]->stateData[k].stateDataIndex =
               mSurfaces[pLight->states[j]->stateData[k].surfaceIndex].stateDataStart;
            surfacesAffected.set(pLight->states[j]->stateData[k].surfaceIndex);
         }
      }

      for (U32 j = 0; j < mSurfaces.size(); j++)
         if (surfacesAffected.test(j))
            mSurfaces[j].stateDataStart++;
   }

   // We clobbered the stateDataStart in the last pass, lets reset it...
   mSurfaces[0].stateDataStart = 0;
   for (U32 i = 1; i < mSurfaces.size(); i++) {
      U32 correctStart = mSurfaces[i-1].stateDataStart +
         mSurfaces[i-1].numLights;
      AssertFatal(correctStart == mSurfaces[i].stateDataStart - mSurfaces[i].numLights, "Error, miscount somewhere");
      mSurfaces[i].stateDataStart = correctStart;
   }

   // Finished!
   pRuntime->mNumLightStateEntries += mSurfaces.last().stateDataStart + mSurfaces.last().numLights;
}


void EditGeometry::exportLightsToRuntime(Interior* pRuntime)
{
   postProcessLighting(pRuntime);

   for (U32 i = 0; i < mAnimatedLights.size(); i++) {
      AnimatedLight* pLight = mAnimatedLights[i];

      pRuntime->mAnimatedLights.increment();
      Interior::AnimatedLight& rILight = pRuntime->mAnimatedLights.last();

      rILight.nameIndex = pRuntime->mNameBuffer.size();

      if (pLight->name == NULL) {
         static U32 sUnnamedLight = 0;
         pLight->name = new char[128];
         dSprintf(pLight->name, 127, "UNNAMED_%d", sUnnamedLight++);

      }
      pRuntime->mNameBuffer.setSize(rILight.nameIndex + dStrlen(pLight->name) + 1);
      dStrcpy(pRuntime->mNameBuffer.address() + rILight.nameIndex, pLight->name);

      rILight.flags      = pLight->type;
      rILight.flags     |= pLight->alarm ? Interior::AlarmLight : 0;

      rILight.stateIndex = pRuntime->mLightStates.size();
      rILight.stateCount = pLight->states.size();
      pRuntime->mLightStates.setSize(rILight.stateIndex + rILight.stateCount);
      
      F32 duration = 0.0f;
      for (U32 j = 0; j < rILight.stateCount; j++) {
         LightState* pState            = pLight->states[j];
         Interior::LightState& rIState = pRuntime->mLightStates[rILight.stateIndex + j];

         rIState.red   = pState->color.red;
         rIState.green = pState->color.green;
         rIState.blue  = pState->color.blue;

         rIState.activeTime = U32((duration * 1000.0f) + 0.5f);

         // DMMFIX
         rIState.dataIndex = pRuntime->mStateData.size();
         rIState.dataCount = pState->stateData.size();
         pRuntime->mStateData.setSize(rIState.dataIndex + rIState.dataCount);

         for (U32 k = 0; k < rIState.dataCount; k++) {
            Interior::LightStateData& rIStateData = pRuntime->mStateData[rIState.dataIndex + k];
            StateData& rStateData                 = pState->stateData[k];

            rIStateData.surfaceIndex    = rStateData.surfaceIndex;
            rIStateData.mapIndex        = exportIntensityMap(pRuntime, rStateData.pLMap);
            rIStateData.lightStateIndex = rStateData.stateDataIndex;
         }

         duration += pState->duration;
      }

      rILight.duration = U32((duration * 1000.0f) + 0.5f);
   }

   // Sort the interior lights so that the triggerables are first,
   //
   dQsort(pRuntime->mAnimatedLights.address(),     // p
          pRuntime->mAnimatedLights.size(),        // nElem
          sizeof(Interior::AnimatedLight),         // w
          cmpAnimatedLights);                      // cmp
}


//------------------------------------------------------------------------------
bool EditGeometry::exportToRuntime(Interior* pRuntime, InteriorResource* pResource)
{
   mExportPointMap.clear();

   pRuntime->mHasAlarmState = mHasAlarmState;

   // The first thing to do is to export the planelist. Note that
   //  we're exporting only the forward facing planes.  Negatives
   //  are indicated by the high bit being set on the index.  From now
   //  on, we need to export planeIndices by calling convert...
   //
   exportPlanes(pRuntime);

   // Next we export the BSP Tree.  We'll need to call a recursive function
   //  for this.
   U32 totalNodes = mNodeArena.mBuffers.size() * mNodeArena.arenaSize;
   pRuntime->mBSPNodes.reserve(totalNodes);
   pRuntime->mBSPSolidLeaves.reserve(totalNodes);
   exportBSPToRuntime(pRuntime, mBSPRoot);

   exportDMLToRuntime(pRuntime, mTextureNames);

   // Now export the zones
   for (U32 i = 0; i < mZones.size(); i++) {
      if (mZones[i]->active == true) {
         pRuntime->mZones.increment();

         pRuntime->mZones.last().portalStart = pRuntime->mZonePortalList.size();
         pRuntime->mZones.last().portalCount = mZones[i]->referencingPortals.size();
         pRuntime->mZones.last().zoneId      = i;
         pRuntime->mZones.last().flags       = mZones[i]->ambientLit ? 0 : Interior::ZoneInside;

         for (U32 j = 0; j < mZones[i]->referencingPortals.size(); j++) {
            pRuntime->mZonePortalList.increment();
            pRuntime->mZonePortalList.last() = mZones[i]->referencingPortals[j];
         }
      }
   }

   // ...and the portals
   for (U32 i = 0; i < mPortals.size(); i++) {
      pRuntime->mPortals.increment();
      pRuntime->mPortals.last().planeIndex  = remapPlaneIndex(mPortals[i]->planeEQIndex);
      pRuntime->mPortals.last().triFanStart = pRuntime->mWindingIndices.size();
      if (mPortals[i]->windings.size() != 0)
         pRuntime->mPortals.last().triFanCount = 1;
      else
         pRuntime->mPortals.last().triFanCount = 0;

      pRuntime->mPortals.last().zoneFront = 0xFFFF;
      pRuntime->mPortals.last().zoneBack  = 0xFFFF;
      for (U32 j = 0; j < pRuntime->mZones.size(); j++) {
         if (pRuntime->mZones[j].zoneId == mPortals[i]->frontZone)
            pRuntime->mPortals.last().zoneFront = j;
         if (pRuntime->mZones[j].zoneId == mPortals[i]->backZone)
            pRuntime->mPortals.last().zoneBack  = j;
      }
      AssertFatal((mPortals[i]->frontZone == -1 && mPortals[i]->backZone == -1) ||
                  (pRuntime->mPortals.last().zoneFront != 0xFFFF && pRuntime->mPortals.last().zoneBack != 0xFFFF),
                  avar("Error, couldn't find the active zone corresponding to this portal! (%d, %d | %d, %d)",
                       mPortals[i]->frontZone,
                       mPortals[i]->backZone,
                       pRuntime->mPortals.last().zoneFront,
                       pRuntime->mPortals.last().zoneBack));

      if (mPortals[i]->windings.size() != 0) {
         Winding winding;
         giftWrapPortal(winding, mPortals[i]);
         exportWindingToRuntime(pRuntime, winding);
      }
   }

   // Dump the animated lights.
   exportLightsToRuntime(pRuntime);

   // Dump the texgen
   pRuntime->mTexGenEQs.setSize(mTexGenEQs.size());
   for (U32 i = 0; i < mTexGenEQs.size(); i++) {
      pRuntime->mTexGenEQs[i].planeX.x = mTexGenEQs[i].planeX.x * mWorldEntity->mGeometryScale;
      pRuntime->mTexGenEQs[i].planeX.y = mTexGenEQs[i].planeX.y * mWorldEntity->mGeometryScale;
      pRuntime->mTexGenEQs[i].planeX.z = mTexGenEQs[i].planeX.z * mWorldEntity->mGeometryScale;
      pRuntime->mTexGenEQs[i].planeX.d = mTexGenEQs[i].planeX.d;
      pRuntime->mTexGenEQs[i].planeY.x = mTexGenEQs[i].planeY.x * mWorldEntity->mGeometryScale;
      pRuntime->mTexGenEQs[i].planeY.y = mTexGenEQs[i].planeY.y * mWorldEntity->mGeometryScale;
      pRuntime->mTexGenEQs[i].planeY.z = mTexGenEQs[i].planeY.z * mWorldEntity->mGeometryScale;
      pRuntime->mTexGenEQs[i].planeY.d = mTexGenEQs[i].planeY.d;
   }

   // ...and the surfaces
   if (mSurfaces.size() != 0) {
      for (U32 i = 0; i < mSurfaces.size(); i++) {
         dumpSurfaceToRuntime(pRuntime, mSurfaces[i]);

         AssertFatal(mSurfaces[i].sheetIndex < 0xFF, "Error, sheet index won't fit into 8 bits!");
         pRuntime->mNormalLMapIndices.push_back(mSurfaces[i].sheetIndex);
         pRuntime->mAlarmLMapIndices.push_back(mSurfaces[i].alarmSheetIndex);
      }
   }

   // Dump any mirrors...
   // Dump any triggers
   // Dump any paths
   // Dump any AISpecial Nodes

   for (U32 i = 0; i < mEntities.size(); i++) {
      if (dynamic_cast<TriggerEntity*>(mEntities[i]) != NULL) {
         dumpTriggerToRuntime(pRuntime, pResource,
                              static_cast<TriggerEntity*>(mEntities[i]));
      }
      //else if (dynamic_cast<SpecialNodeEntity*>(mEntities[i]) != NULL) {
      //   dumpAISpecialToRuntime(pRuntime, pResource,
      //                          static_cast<SpecialNodeEntity*>(mEntities[i]));
      //}
      else if (dynamic_cast<GameEntity*>(mEntities[i]) != NULL) {
         dumpGameEntityToRuntime(pRuntime, pResource, 
                              static_cast<GameEntity*>(mEntities[i]));
      }
   }

   // Dump doors (has to be after paths...
   for (U32 i = 0; i < mEntities.size(); i++) {
      if (dynamic_cast<DoorEntity*>(mEntities[i])) {
         dumpDoorToRuntime(pRuntime, pResource, static_cast<DoorEntity*>(mEntities[i]));
      }
      //else if (dynamic_cast<ForceFieldEntity*>(mEntities[i])) {
      //   dumpForceFieldToRuntime(pRuntime, pResource, static_cast<ForceFieldEntity*>(mEntities[i]));
      //}
   }

   for (U32 i = 0; i < mNullSurfaces.size(); i++)
      dumpNullSurfaceToRuntime(pRuntime, mNullSurfaces[i]);

   for (U32 i = 0; i < mVehicleNullSurfaces.size(); i++)
      dumpVehicleNullSurfaceToRuntime(pRuntime, mVehicleNullSurfaces[i]);

   if (mSurfaces.size() != 0) {
      for (U32 i = 0; i < pRuntime->mZones.size(); i++) {
         pRuntime->mZones[i].surfaceStart = pRuntime->mZoneSurfaces.size();
         pRuntime->mZones[i].surfaceCount = 0;
         for (U32 j = 0; j < mSurfaces.size(); j++) {
            if (mSurfaces[j].isMemberOfZone(pRuntime->mZones[i].zoneId)) {
               pRuntime->mZoneSurfaces.push_back(j);
               pRuntime->mZones[i].surfaceCount++;
            }
         }
      }
   } else {
      for (U32 i = 0; i < pRuntime->mZones.size(); i++) {
         pRuntime->mZones[i].surfaceStart = 0;
         pRuntime->mZones[i].surfaceCount = 0;
      }
   }

   // ...and the lightmaps
   for (U32 i = 0; i < mLightmaps.size(); i++)
   {
      pRuntime->mLightmaps.push_back(new GBitmap(*mLightmaps[i]));
   }

   for (U32 i = 0; i < mLightDirMaps.size(); i++)
   {
      pRuntime->mLightDirMaps.push_back(new GBitmap(*mLightDirMaps[i]));
   }

   createBoundingVolumes(pRuntime);
   pRuntime->mDetailLevel = mWorldEntity->mDetailNumber;
   pRuntime->mMinPixels   = mWorldEntity->mMinPixels;

   // Build the interval trees and convex hulls for solid collisions...
   if (mSpecialCollisionBrushes.size() == 0)
   {
      for (U32 i = 0; i < mStructuralBrushes.size(); i++)
         exportHullToRuntime(pRuntime, mStructuralBrushes[i]);
      for (U32 i = 0; i < mDetailBrushes.size(); i++)
         exportHullToRuntime(pRuntime, mDetailBrushes[i]);
   }
   else
   {
      // Export the special hulls only...
      for (U32 i = 0; i < mSpecialCollisionBrushes.size(); i++)
         exportHullToRuntime(pRuntime, mSpecialCollisionBrushes[i]);
   }

   for (U32 i = 0; i< mVehicleCollisionBrushes.size(); i++)
   {
      exportVehicleHullToRuntime(pRuntime, mVehicleCollisionBrushes[i]);
   }
   
   exportHullBins(pRuntime);

   // Export the ambient colors...
   pRuntime->mBaseAmbient  = mWorldEntity->mAmbientColor;
   pRuntime->mAlarmAmbient = mWorldEntity->mEmergencyAmbientColor;

   // Setup the point visibilities...
   pRuntime->mPointVisibility.setSize(pRuntime->mPoints.size());
   dMemset(pRuntime->mPointVisibility.address(), 0, pRuntime->mPointVisibility.size());
   for (U32 i = 0; i < pRuntime->mSurfaces.size(); i++)
   {
      // Any point that is on an outside surface gets a 0xFF for right now...
      for (U32 j = 0; j < pRuntime->mSurfaces[i].windingCount; j++)
      {
         pRuntime->mPointVisibility[pRuntime->mWindings[pRuntime->mSurfaces[i].windingStart + j]] = 0xFF;
      }
   }

   // Setup the flags to keep the lightmaps if they have animated lights on them...
   pRuntime->mLightmapKeep.setSize(pRuntime->mLightmaps.size());
   for (U32 i = 0; i < pRuntime->mLightmapKeep.size(); i++)
      pRuntime->mLightmapKeep[i] = false;
   for (U32 i = 0; i < pRuntime->mStateData.size(); i++)
   {
      pRuntime->mLightmapKeep[pRuntime->mNormalLMapIndices[pRuntime->mStateData[i].surfaceIndex]] = true;
      if (pRuntime->mHasAlarmState)
         pRuntime->mLightmapKeep[pRuntime->mAlarmLMapIndices[pRuntime->mStateData[i].surfaceIndex]] = true;
   }

   return true;
}

struct PolySorted {
   U32  numPoints;
   S32* points;
   U16  planeIndex;
};

int QSORT_CALLBACK cmpPointIndices(const void* p1, const void* p2)
{
   const S32* pS321 = (const S32*)p1;
   const S32* pS322 = (const S32*)p2;

   return (*pS322) - (*pS321);
}

// God this sucks.
int QSORT_CALLBACK cmpPolySorted(const void* p1, const void* p2)
{
   const PolySorted* pSorted1 = (const PolySorted*)p1;
   const PolySorted* pSorted2 = (const PolySorted*)p2;

   static S32 temp1[128];
   static S32 temp2[128];
   dMemcpy(temp1, pSorted1->points, sizeof(S32) * pSorted1->numPoints);
   dMemcpy(temp2, pSorted2->points, sizeof(S32) * pSorted2->numPoints);

   dQsort(temp1, pSorted1->numPoints, sizeof(S32), cmpPointIndices);
   dQsort(temp2, pSorted2->numPoints, sizeof(S32), cmpPointIndices);

   for (U32 i = 0; i < pSorted1->numPoints && i < pSorted2->numPoints; i++) {
      if (temp1[i] != temp2[i])
         return temp2[i] - temp1[i];
   }

   if (pSorted1->numPoints > pSorted2->numPoints)
      return 1;
   else if (pSorted2->numPoints > pSorted1->numPoints)
      return -1;
   else
      return 0;
}

void EditGeometry::exportHullToRuntime(Interior* pRuntime, CSGBrush* brush)
{
   S32** pWindingRemap = new S32*[brush->mPlanes.size()];
   for (U32 i = 0; i < brush->mPlanes.size(); i++) {
      pWindingRemap[i] = new S32[brush->mPlanes[i].winding.numIndices];
      for (U32 j = 0; j < brush->mPlanes[i].winding.numIndices; j++)
         pWindingRemap[i][j] = -1;
   }

   U32 numRemaps = 0;
   // First, we remap the first winding
   for (U32 i = 0; i < brush->mPlanes[0].winding.numIndices; i++)
      pWindingRemap[0][i] = numRemaps++;

   // Now, replace all of those indices in the other windings with the index of the
   //  remapped point
   for (U32 i = 0; i < brush->mPlanes[0].winding.numIndices; i++) {
      S32 remapIndex = brush->mPlanes[0].winding.indices[i];
      S32 remapTo    = i;

      for (U32 j = 1; j < brush->mPlanes.size(); j++) {
         for (U32 k = 0; k < brush->mPlanes[j].winding.numIndices; k++) {
            if (brush->mPlanes[j].winding.indices[k] == remapIndex) {
               pWindingRemap[j][k] = remapTo;
            }
         }
      }
   }

   // Now, push the edge signature of the first winding onto our stack
   Vector<U32> edgeSignatureStack;
   for (U32 i = 0; i < brush->mPlanes[0].winding.numIndices; i++) {
      S32 firstPoint = pWindingRemap[0][i];
      S32 nextPoint = pWindingRemap[0][(i+1) % brush->mPlanes[0].winding.numIndices];
      U32 edgeSignature = ((getMin(firstPoint, nextPoint) << 16) |
                           (getMax(firstPoint, nextPoint) << 0));
      edgeSignatureStack.push_back(edgeSignature);
   }

   Vector<U32> polysLeft;
   for (U32 i = 1; i < brush->mPlanes.size(); i++)
      polysLeft.push_back(i);

   U32 numFinds = 0;
   while (edgeSignatureStack.empty() == false) {
      // Find the winding with an edge signature that matches the first edge on the stack.
      //  we're guaranteed that this exists.
      S32 polyIndex = -1;
      for (U32 i = 0; i < polysLeft.size() && polyIndex == -1; i++) {
         for (U32 j = 0; j < brush->mPlanes[polysLeft[i]].winding.numIndices && polyIndex == -1; j++) {
            S32 firstPoint = pWindingRemap[polysLeft[i]][j];
            S32 nextPoint  = pWindingRemap[polysLeft[i]][(j+1) % brush->mPlanes[polysLeft[i]].winding.numIndices];
            if (firstPoint == -1 || nextPoint == -1)
               continue;

            U32 edgeSignature = ((getMin(firstPoint, nextPoint) << 16) |
                                 (getMax(firstPoint, nextPoint) << 0));
            if (edgeSignature == edgeSignatureStack[0]) {
               polyIndex = polysLeft[i];
               polysLeft.erase(i);
               break;
            }
         }
      }
      AssertFatal(polyIndex != -1, "Error, should have found that poly!");

      if (polyIndex == -1)
         break;
      

      // Ok, so now we know which poly is next, let's rotate it's winding until the edge
      //  we want is the first two indices in this poly
      U32 warnCount = 0;
      while (true) {
         bool rotate = false;
         S32 firstPoint = pWindingRemap[polyIndex][0];
         S32 nextPoint  = pWindingRemap[polyIndex][1];
         if (firstPoint == -1 || nextPoint == -1) {
            rotate = true;
         } else {
            U32 edgeSignature = ((getMin(firstPoint, nextPoint) << 16) |
                                 (getMax(firstPoint, nextPoint) << 0));
            if (edgeSignature != edgeSignatureStack[0])
               rotate = true;
         }

         // OK, if we need to rotate, let's do that, otherwise, let's move on.
         if (rotate == true) {
            S32 stor = pWindingRemap[polyIndex][0];
            dMemmove(&pWindingRemap[polyIndex][0], &pWindingRemap[polyIndex][1],
                     sizeof(S32) * (brush->mPlanes[polyIndex].winding.numIndices - 1));
            pWindingRemap[polyIndex][brush->mPlanes[polyIndex].winding.numIndices - 1] = stor;

            U32 storU = brush->mPlanes[polyIndex].winding.indices[0];
            dMemmove(&brush->mPlanes[polyIndex].winding.indices[0],
                     &brush->mPlanes[polyIndex].winding.indices[1],
                     sizeof(U32) * (brush->mPlanes[polyIndex].winding.numIndices - 1));
            brush->mPlanes[polyIndex].winding.indices[brush->mPlanes[polyIndex].winding.numIndices - 1] = storU;
         } else {
            break;
         }
         warnCount++;
         AssertFatal(warnCount < 500, "Error, more rotations performed than possible");
      }

      // Ok.  Now let's remap all the points necessary on this one
      for (U32 i = 0; i < brush->mPlanes[polyIndex].winding.numIndices; i++) {
         if (pWindingRemap[polyIndex][i] == -1) {
            pWindingRemap[polyIndex][i] = numRemaps++;

            // Now remap all the polys that contain that vertex as well (note that the
            //  zero poly is always fully remapped, so we can ignore it
            S32 remapIndex = brush->mPlanes[polyIndex].winding.indices[i];
            S32 remapTo    = pWindingRemap[polyIndex][i];

            for (U32 j = 1; j < brush->mPlanes.size(); j++) {
               for (U32 k = 0; k < brush->mPlanes[j].winding.numIndices; k++) {
                  if (brush->mPlanes[j].winding.indices[k] == remapIndex) {
                     pWindingRemap[j][k] = remapTo;
                  }
               }
            }
         }
      }

      // And push back the edge signatures for this winding, deleting any duplicates
      for (U32 i = 0; i < brush->mPlanes[polyIndex].winding.numIndices; i++) {
         S32 firstPoint = pWindingRemap[polyIndex][i];
         S32 nextPoint = pWindingRemap[polyIndex][(i+1) % brush->mPlanes[polyIndex].winding.numIndices];
         AssertFatal(firstPoint != -1 && nextPoint != -1, "Error, that's impossible!");
         U32 edgeSignature = ((getMin(firstPoint, nextPoint) << 16) |
                              (getMax(firstPoint, nextPoint) << 0));

         bool insert = true;
         for (U32 j = 0; j < edgeSignatureStack.size(); j++) {
            if (edgeSignatureStack[j] == edgeSignature) {
               insert = false;
               edgeSignatureStack.erase(j);
               break;
            }
         }
         if (insert == true)
            edgeSignatureStack.push_back(edgeSignature);
      }


      numFinds++;
      AssertFatal(numFinds <= brush->mPlanes.size(), avar("Error, too many damn finds (%d)!", brush->brushId));
   }

   // OK!  At this point, none of the points should be uninserted, and everything is
   //  in canonical order.  Let's whip through the remap arrays just to make sure.  At
   //  the same time, we'll create the points array, which is a remap of indices in the
   //  remaps to the hullIndices.
   Vector<U32> points;
   points.setSize(numRemaps);
   for (U32 i = 0; i < points.size(); i++)
      points[i] = 0xFFFFFFFF;

   for (U32 i = 0; i < brush->mPlanes.size(); i++) {
      for (U32 j = 0; j < brush->mPlanes[i].winding.numIndices; j++) {
         if (pWindingRemap[i][j] == -1)
            continue;
         AssertFatal(pWindingRemap[i][j] != -1, "Error, there's an unremapped point here!");

         if (points[pWindingRemap[i][j]] != 0xFFFFFFFF) {
            AssertFatal(points[pWindingRemap[i][j]] == brush->mPlanes[i].winding.indices[j], "Error, bad remapping!");
         } else {
            points[pWindingRemap[i][j]] = brush->mPlanes[i].winding.indices[j];
         }
      }
   }

   for (U32 i = 0; i < points.size(); i++)
      points[i] = exportPointToRuntime(pRuntime, points[i]);

   pRuntime->mConvexHulls.increment();
   pRuntime->mConvexHulls.last().hullStart = pRuntime->mHullIndices.size();

   F32 minx = 1e8;
   F32 miny = 1e8;
   F32 minz = 1e8;
   F32 maxx = -1e8;
   F32 maxy = -1e8;
   F32 maxz = -1e8;
   for (U32 i = 0; i < points.size(); i++) {
      pRuntime->mHullIndices.push_back(points[i]);

      Point3F& rPoint = pRuntime->mPoints[points[i]].point;
      if (rPoint.x < minx) minx = rPoint.x;
      if (rPoint.x > maxx) maxx = rPoint.x;
      if (rPoint.y < miny) miny = rPoint.y;
      if (rPoint.y > maxy) maxy = rPoint.y;
      if (rPoint.z < minz) minz = rPoint.z;
      if (rPoint.z > maxz) maxz = rPoint.z;
   }

   pRuntime->mConvexHulls.last().hullCount = (pRuntime->mHullIndices.size() -
                                              pRuntime->mConvexHulls.last().hullStart);
   pRuntime->mConvexHulls.last().minX = minx;
   pRuntime->mConvexHulls.last().maxX = maxx;
   pRuntime->mConvexHulls.last().minY = miny;
   pRuntime->mConvexHulls.last().maxY = maxy;
   pRuntime->mConvexHulls.last().minZ = minz;
   pRuntime->mConvexHulls.last().maxZ = maxz;


   // We need a sorted array of poly objects in order to make these plane normals come
   //  out right.
   PolySorted* pSortedPolys = new PolySorted[brush->mPlanes.size()];
   for (U32 i = 0; i < brush->mPlanes.size(); i++) {
      pSortedPolys[i].numPoints  = brush->mPlanes[i].winding.numIndices;
      pSortedPolys[i].points     = pWindingRemap[i];
      pSortedPolys[i].planeIndex = remapPlaneIndex(brush->mPlanes[i].planeEQIndex);
   }
   dQsort(pSortedPolys, brush->mPlanes.size(), sizeof(PolySorted), cmpPolySorted);

   // Ok, now we have to construct an emit string for each vertex.  This should be fairly
   //  straightforward, the procedure is: for each point:
   //   - find all polys that contain that point
   //   - find all points in those polys
   //   - find all edges  "  "     "
   //   - enter the string
   //  The tricky bit is that we have to set up the emit indices to be relative to the
   //   hullindices.
   for (U32 i = 0; i < points.size(); i++) {
      static Vector<U32> emitPoints(128);
      static Vector<U16> emitEdges(128);
      static Vector<U32> emitPolys;
      emitPoints.setSize(0);
      emitEdges.setSize(0);
      emitPolys.setSize(0);

      for (U32 j = 0; j < brush->mPlanes.size(); j++) {
         bool found = false;
         for (U32 k = 0; k < pSortedPolys[j].numPoints; k++) {
            if (pSortedPolys[j].points[k] == i) {
               found = true;
               break;
            }
         }
         if (found) {
            // Have to emit this poly...
            for (U32 k = 0; k < pSortedPolys[j].numPoints; k++) {
               emitPoints.push_back(pSortedPolys[j].points[k]);

               S32 firstPoint = pSortedPolys[j].points[k];
               S32 nextPoint  = pSortedPolys[j].points[(k+1) % pSortedPolys[j].numPoints];
               AssertFatal(firstPoint != -1 && nextPoint != -1, "Error, that's impossible!");
               AssertFatal(firstPoint < 255 && nextPoint < 255, "Error, that's impossible!");
               U16 edgeSignature = ((getMin(firstPoint, nextPoint) << 8) |
                                    (getMax(firstPoint, nextPoint) << 0));
               emitEdges.push_back(edgeSignature);
            }
            emitPolys.push_back(j);
         }
      }

      // We also have to emit any polys that share the plane, but not necessarily the
      //  support point
      for (U32 j = 0; j < brush->mPlanes.size(); j++) {
         for (U32 k = 0; k < emitPolys.size(); k++) {
            if (emitPolys[k] == j)
               continue;

            if (pSortedPolys[emitPolys[k]].planeIndex == pSortedPolys[j].planeIndex) {
               // We may have to emit this as well...
               bool found = false;
               for (U32 l = 0; l < emitPolys.size(); l++) {
                  if (emitPolys[l] == j)
                     found = true;
               }
               if (!found) {
                  // Have to emit this poly...
                  for (U32 l = 0; l < pSortedPolys[j].numPoints; l++) {
                     emitPoints.push_back(pSortedPolys[j].points[l]);

                     S32 firstPoint = pSortedPolys[j].points[l];
                     S32 nextPoint  = pSortedPolys[j].points[(l+1) % pSortedPolys[j].numPoints];
                     AssertFatal(firstPoint != -1 && nextPoint != -1, "Error, that's impossible!");
                     AssertFatal(firstPoint < 255 && nextPoint < 255, "Error, that's impossible!");
                     U16 edgeSignature = ((getMin(firstPoint, nextPoint) << 8) |
                                          (getMax(firstPoint, nextPoint) << 0));
                     emitEdges.push_back(edgeSignature);
                  }
                  emitPolys.push_back(j);
               }
            }
         }
      }

      AssertFatal(emitPoints.size() != 0 && emitEdges.size() != 0 && emitPolys.size() != 0,
                  "Error, should always emit something!");

      std::sort(emitPoints.begin(), emitPoints.end());
      U32* newend = std::unique(emitPoints.begin(), emitPoints.end());
      emitPoints.setSize(newend - emitPoints.begin());
      std::sort(emitPoints.begin(), emitPoints.end());
      
      std::sort(emitEdges.begin(), emitEdges.end());
      U16* newend16 = std::unique(emitEdges.begin(), emitEdges.end());
      emitEdges.setSize(newend16 - emitEdges.begin());
      std::sort(emitEdges.begin(), emitEdges.end());

      // We need to transmute the edges to be relative to the points in the order of
      //  emission
      for (U32 j = 0; j < emitEdges.size(); j++) {
         U16 firstIndex = emitEdges[j] >> 8;
         U16 nextIndex  = emitEdges[j] & 0xFF;

         U16 newFirst = 0xFFFF;
         U16 newNext  = 0xFFFF;
         for (U32 k = 0; k < emitPoints.size(); k++) {
            if (emitPoints[k] == firstIndex)
               newFirst = k;
            if (emitPoints[k] == nextIndex)
               newNext = k;
         }
         AssertFatal(newFirst != 0xFFFF && newNext != 0xFFFF, "Error, bad edge!");

         U16 newSignature = ((getMin(newFirst, newNext) << 8) |
                             (getMax(newFirst, newNext) << 0));
         emitEdges[j] = newSignature;
      }

      // Emit string length is:
      //  1 + NumPoints +
      //  1 + (NumEdges * 2) +
      //  1 + Sum_polys(1 + 1 + numVerts(poly))

      U32 emitStringLen = 0;
      emitStringLen += 1 + emitPoints.size();
      emitStringLen += 1 + (emitEdges.size() * 2);
      emitStringLen += 1;
      for (U32 j = 0; j < emitPolys.size(); j++)
         emitStringLen += 1 + 1 + pSortedPolys[emitPolys[j]].numPoints;

      U8* emitString = new U8[emitStringLen];
      U32 currPos = 0;
      dMemset(emitString, 0, emitStringLen);

      // First, dump the points
      emitString[currPos++] = emitPoints.size();
      for (U32 j = 0; j < emitPoints.size(); j++)
         emitString[currPos++] = emitPoints[j];

      // Now the edges
      emitString[currPos++] = emitEdges.size();
      for (U32 j = 0; j < emitEdges.size(); j++) {
         U16 edgeP1 = emitEdges[j] >> 8;
         U16 edgeP2 = emitEdges[j] & 0xFF;

         emitString[currPos++] = edgeP1;
         emitString[currPos++] = edgeP2;
      }

      // And the polys.
      emitString[currPos++] = emitPolys.size();
      for (U32 j = 0; j < emitPolys.size(); j++) {
         // A poly is:
         //  vertex count
         //  plane index
         //  verts (relative)
         emitString[currPos++] = pSortedPolys[emitPolys[j]].numPoints;
         emitString[currPos++] = emitPolys[j];
         for (U32 k = 0; k < pSortedPolys[emitPolys[j]].numPoints; k++) {
            bool found = false;
            for (U32 l = 0; l < emitPoints.size(); l++) {
               if (emitPoints[l] == pSortedPolys[emitPolys[j]].points[k]) {
                  emitString[currPos++] = l;
                  found = true;
                  break;
               }
            }
            AssertFatal(found, "Error, missed a point!");
         }
      }
      AssertFatal(currPos == emitStringLen, "Error, miscalculation of string len!");

      U32 emitStringExportedIndex = exportEmitStringToRuntime(pRuntime, emitString, emitStringLen);
      pRuntime->mHullEmitStringIndices.push_back(emitStringExportedIndex);
      AssertFatal(dMemcmp(&pRuntime->mConvexHullEmitStrings[emitStringExportedIndex], emitString, emitStringLen) == 0, "Bad exported string!");
      delete [] emitString;
   }
   AssertFatal(pRuntime->mHullEmitStringIndices.size() == pRuntime->mHullIndices.size(), "Point/emitstring mismatch!");

   
   pRuntime->mConvexHulls.last().planeStart = pRuntime->mHullPlaneIndices.size();
   for (U32 i = 0; i < brush->mPlanes.size(); i++)
      pRuntime->mHullPlaneIndices.push_back(pSortedPolys[i].planeIndex);

   for (U32 i = 0; i < brush->mPlanes.size(); i++)
      delete [] pWindingRemap[i];
   delete [] pWindingRemap;
   delete [] pSortedPolys;

   // Now dump out the surfaces that were created by this hull that are still in
   //  the interior...
   pRuntime->mConvexHulls.last().surfaceStart = pRuntime->mHullSurfaceIndices.size();
   for (U32 i = 0; i < mSurfaces.size(); i++) {
      if (mSurfaces[i].winding.brushId == brush->brushId) {
         // Dump this surface
         pRuntime->mHullSurfaceIndices.push_back(i);
      }
   }
   for (U32 i = 0; i < mNullSurfaces.size(); i++) {
      if (mNullSurfaces[i].winding.brushId == brush->brushId) {
         // Dump this surface
         pRuntime->mHullSurfaceIndices.push_back(makeNullSurfaceIndex(i));
      }
   }
   pRuntime->mConvexHulls.last().surfaceCount = (pRuntime->mHullSurfaceIndices.size() -
                                                 pRuntime->mConvexHulls.last().surfaceStart);
}


void EditGeometry::exportVehicleHullToRuntime(Interior* pRuntime, CSGBrush* brush)
{
   S32** pWindingRemap = new S32*[brush->mPlanes.size()];
   for (U32 i = 0; i < brush->mPlanes.size(); i++) {
      pWindingRemap[i] = new S32[brush->mPlanes[i].winding.numIndices];
      for (U32 j = 0; j < brush->mPlanes[i].winding.numIndices; j++)
         pWindingRemap[i][j] = -1;
   }

   U32 numRemaps = 0;
   // First, we remap the first winding
   for (U32 i = 0; i < brush->mPlanes[0].winding.numIndices; i++)
      pWindingRemap[0][i] = numRemaps++;

   // Now, replace all of those indices in the other windings with the index of the
   //  remapped point
   for (U32 i = 0; i < brush->mPlanes[0].winding.numIndices; i++) {
      S32 remapIndex = brush->mPlanes[0].winding.indices[i];
      S32 remapTo    = i;

      for (U32 j = 1; j < brush->mPlanes.size(); j++) {
         for (U32 k = 0; k < brush->mPlanes[j].winding.numIndices; k++) {
            if (brush->mPlanes[j].winding.indices[k] == remapIndex) {
               pWindingRemap[j][k] = remapTo;
            }
         }
      }
   }

   // Now, push the edge signature of the first winding onto our stack
   Vector<U32> edgeSignatureStack;
   for (U32 i = 0; i < brush->mPlanes[0].winding.numIndices; i++) {
      S32 firstPoint = pWindingRemap[0][i];
      S32 nextPoint  = pWindingRemap[0][(i+1) % brush->mPlanes[0].winding.numIndices];
      U32 edgeSignature = (getMin(firstPoint, nextPoint) << 16) |
         (getMax(firstPoint, nextPoint) << 0);
      edgeSignatureStack.push_back(edgeSignature);
   }

   Vector<U32> polysLeft;
   for (U32 i = 1; i < brush->mPlanes.size(); i++)
      polysLeft.push_back(i);

   U32 numFinds = 0;
   while (edgeSignatureStack.empty() == false) {
      // Find the winding with an edge signature that matches the first edge on the stack.
      //  we're guaranteed that this exists.
      S32 polyIndex = -1;
      for (U32 i = 0; i < polysLeft.size() && polyIndex == -1; i++) {
         for (U32 j = 0; j < brush->mPlanes[polysLeft[i]].winding.numIndices && polyIndex == -1; j++) {
            S32 firstPoint = pWindingRemap[polysLeft[i]][j];
            S32 nextPoint  = pWindingRemap[polysLeft[i]][(j+1) % brush->mPlanes[polysLeft[i]].winding.numIndices];
            if (firstPoint == -1 || nextPoint == -1)
               continue;

            U32 edgeSignature = (getMin(firstPoint, nextPoint) << 16) |
               (getMax(firstPoint, nextPoint) << 0);
            if (edgeSignature == edgeSignatureStack[0]) {
               polyIndex = polysLeft[i];
               polysLeft.erase(i);
               break;
            }
         }
      }
      AssertFatal(polyIndex != -1, "Error, should have found that poly!");
      

      // Ok, so now we know which poly is next, let's rotate it's winding until the edge
      //  we want is the first two indices in this poly
      U32 warnCount = 0;
      while (true) {
         bool rotate = false;
         S32 firstPoint = pWindingRemap[polyIndex][0];
         S32 nextPoint  = pWindingRemap[polyIndex][1];
         if (firstPoint == -1 || nextPoint == -1) {
            rotate = true;
         } else {
            U32 edgeSignature = (getMin(firstPoint, nextPoint) << 16) |
               (getMax(firstPoint, nextPoint) << 0);
            if (edgeSignature != edgeSignatureStack[0])
               rotate = true;
         }

         // OK, if we need to rotate, let's do that, otherwise, let's move on.
         if (rotate == true) {
            S32 stor = pWindingRemap[polyIndex][0];
            dMemmove(&pWindingRemap[polyIndex][0], &pWindingRemap[polyIndex][1],
                     sizeof(S32) * (brush->mPlanes[polyIndex].winding.numIndices - 1));
            pWindingRemap[polyIndex][brush->mPlanes[polyIndex].winding.numIndices - 1] = stor;

            U32 storU = brush->mPlanes[polyIndex].winding.indices[0];
            dMemmove(&brush->mPlanes[polyIndex].winding.indices[0],
                     &brush->mPlanes[polyIndex].winding.indices[1],
                     sizeof(U32) * (brush->mPlanes[polyIndex].winding.numIndices - 1));
            brush->mPlanes[polyIndex].winding.indices[brush->mPlanes[polyIndex].winding.numIndices - 1] = storU;
         } else {
            break;
         }
         warnCount++;
         AssertFatal(warnCount < 500, "Error, more rotations performed than possible");
      }

      // Ok.  Now let's remap all the points necessary on this one
      for (U32 i = 0; i < brush->mPlanes[polyIndex].winding.numIndices; i++) {
         if (pWindingRemap[polyIndex][i] == -1) {
            pWindingRemap[polyIndex][i] = numRemaps++;

            // Now remap all the polys that contain that vertex as well (note that the
            //  zero poly is always fully remapped, so we can ignore it
            S32 remapIndex = brush->mPlanes[polyIndex].winding.indices[i];
            S32 remapTo    = pWindingRemap[polyIndex][i];

            for (U32 j = 1; j < brush->mPlanes.size(); j++) {
               for (U32 k = 0; k < brush->mPlanes[j].winding.numIndices; k++) {
                  if (brush->mPlanes[j].winding.indices[k] == remapIndex) {
                     pWindingRemap[j][k] = remapTo;
                  }
               }
            }
         }
      }

      // And push back the edge signatures for this winding, deleting any duplicates
      for (U32 i = 0; i < brush->mPlanes[polyIndex].winding.numIndices; i++) {
         S32 firstPoint = pWindingRemap[polyIndex][i];
         S32 nextPoint = pWindingRemap[polyIndex][(i+1) % brush->mPlanes[polyIndex].winding.numIndices];
         AssertFatal(firstPoint != -1 && nextPoint != -1, "Error, that's impossible!");
         U32 edgeSignature = (getMin(firstPoint, nextPoint) << 16) |
            (getMax(firstPoint, nextPoint) << 0);

         bool insert = true;
         for (U32 j = 0; j < edgeSignatureStack.size(); j++) {
            if (edgeSignatureStack[j] == edgeSignature) {
               insert = false;
               edgeSignatureStack.erase(j);
               break;
            }
         }
         if (insert == true)
            edgeSignatureStack.push_back(edgeSignature);
      }


      numFinds++;
      AssertFatal(numFinds <= brush->mPlanes.size(), avar("Error, too many damn finds (%d)!", brush->brushId));
   }

   // OK!  At this point, none of the points should be uninserted, and everything is
   //  in canonical order.  Let's whip through the remap arrays just to make sure.  At
   //  the same time, we'll create the points array, which is a remap of indices in the
   //  remaps to the hullIndices.
   Vector<U32> points;
   points.setSize(numRemaps);
   for (U32 i = 0; i < points.size(); i++)
      points[i] = 0xFFFFFFFF;

   for (U32 i = 0; i < brush->mPlanes.size(); i++) {
      for (U32 j = 0; j < brush->mPlanes[i].winding.numIndices; j++) {
         AssertFatal(pWindingRemap[i][j] != -1, "Error, there's an unremapped point here!");

         if (points[pWindingRemap[i][j]] != 0xFFFFFFFF) {
            AssertFatal(points[pWindingRemap[i][j]] == brush->mPlanes[i].winding.indices[j], "Error, bad remapping!");
         } else {
            points[pWindingRemap[i][j]] = brush->mPlanes[i].winding.indices[j];
         }
      }
   }

   for (U32 i = 0; i < points.size(); i++)
      points[i] = exportVehiclePointToRuntime(pRuntime, points[i]);

   pRuntime->mVehicleConvexHulls.increment();
   pRuntime->mVehicleConvexHulls.last().hullStart = pRuntime->mVehicleHullIndices.size();

   F32 minx = 1e8;
   F32 miny = 1e8;
   F32 minz = 1e8;
   F32 maxx = -1e8;
   F32 maxy = -1e8;
   F32 maxz = -1e8;
   for (U32 i = 0; i < points.size(); i++) {
      pRuntime->mVehicleHullIndices.push_back(points[i]);

      Point3F& rPoint = pRuntime->mVehiclePoints[points[i]].point;
      if (rPoint.x < minx) minx = rPoint.x;
      if (rPoint.x > maxx) maxx = rPoint.x;
      if (rPoint.y < miny) miny = rPoint.y;
      if (rPoint.y > maxy) maxy = rPoint.y;
      if (rPoint.z < minz) minz = rPoint.z;
      if (rPoint.z > maxz) maxz = rPoint.z;
   }

   pRuntime->mVehicleConvexHulls.last().hullCount = (pRuntime->mVehicleHullIndices.size() -
                                                     pRuntime->mVehicleConvexHulls.last().hullStart);
   pRuntime->mVehicleConvexHulls.last().minX = minx;
   pRuntime->mVehicleConvexHulls.last().maxX = maxx;
   pRuntime->mVehicleConvexHulls.last().minY = miny;
   pRuntime->mVehicleConvexHulls.last().maxY = maxy;
   pRuntime->mVehicleConvexHulls.last().minZ = minz;
   pRuntime->mVehicleConvexHulls.last().maxZ = maxz;


   // We need a sorted array of poly objects in order to make these plane normals come
   //  out right.
   PolySorted* pSortedPolys = new PolySorted[brush->mPlanes.size()];
   for (U32 i = 0; i < brush->mPlanes.size(); i++) {
      pSortedPolys[i].numPoints  = brush->mPlanes[i].winding.numIndices;
      pSortedPolys[i].points     = pWindingRemap[i];
      pSortedPolys[i].planeIndex = remapVehiclePlaneIndex(brush->mPlanes[i].planeEQIndex, pRuntime);
   }
   dQsort(pSortedPolys, brush->mPlanes.size(), sizeof(PolySorted), cmpPolySorted);

   // Ok, now we have to construct an emit string for each vertex.  This should be fairly
   //  straightforward, the procedure is: for each point:
   //   - find all polys that contain that point
   //   - find all points in those polys
   //   - find all edges  "  "     "
   //   - enter the string
   //  The tricky bit is that we have to set up the emit indices to be relative to the
   //   hullindices.
   for (U32 i = 0; i < points.size(); i++) {
      static Vector<U32> emitPoints(128);
      static Vector<U16> emitEdges(128);
      static Vector<U32> emitPolys;
      emitPoints.setSize(0);
      emitEdges.setSize(0);
      emitPolys.setSize(0);

      for (U32 j = 0; j < brush->mPlanes.size(); j++) {
         bool found = false;
         for (U32 k = 0; k < pSortedPolys[j].numPoints; k++) {
            if (pSortedPolys[j].points[k] == i) {
               found = true;
               break;
            }
         }
         if (found) {
            // Have to emit this poly...
            for (U32 k = 0; k < pSortedPolys[j].numPoints; k++) {
               emitPoints.push_back(pSortedPolys[j].points[k]);

               S32 firstPoint = pSortedPolys[j].points[k];
               S32 nextPoint  = pSortedPolys[j].points[(k+1) % pSortedPolys[j].numPoints];
               AssertFatal(firstPoint != -1 && nextPoint != -1, "Error, that's impossible!");
               AssertFatal(firstPoint < 255 && nextPoint < 255, "Error, that's impossible!");
               U16 edgeSignature = (getMin(firstPoint, nextPoint) << 8) |
                  (getMax(firstPoint, nextPoint) << 0);
               emitEdges.push_back(edgeSignature);
            }
            emitPolys.push_back(j);
         }
      }

      // We also have to emit any polys that share the plane, but not necessarily the
      //  support point
      for (U32 j = 0; j < brush->mPlanes.size(); j++) {
         for (U32 k = 0; k < emitPolys.size(); k++) {
            if (emitPolys[k] == j)
               continue;

            if (pSortedPolys[emitPolys[k]].planeIndex == pSortedPolys[j].planeIndex) {
               // We may have to emit this as well...
               bool found = false;
               for (U32 l = 0; l < emitPolys.size(); l++) {
                  if (emitPolys[l] == j)
                     found = true;
               }
               if (!found) {
                  // Have to emit this poly...
                  for (U32 l = 0; l < pSortedPolys[j].numPoints; l++) {
                     emitPoints.push_back(pSortedPolys[j].points[l]);

                     S32 firstPoint = pSortedPolys[j].points[l];
                     S32 nextPoint  = pSortedPolys[j].points[(l+1) % pSortedPolys[j].numPoints];
                     AssertFatal(firstPoint != -1 && nextPoint != -1, "Error, that's impossible!");
                     AssertFatal(firstPoint < 255 && nextPoint < 255, "Error, that's impossible!");
                     U16 edgeSignature = (getMin(firstPoint, nextPoint) << 8) |
                        (getMax(firstPoint, nextPoint) << 0);
                     emitEdges.push_back(edgeSignature);
                  }
                  emitPolys.push_back(j);
               }
            }
         }
      }

      AssertFatal(emitPoints.size() != 0 && emitEdges.size() != 0 && emitPolys.size() != 0,
                  "Error, should always emit something!");

      std::sort(emitPoints.begin(), emitPoints.end());
      U32* newend = std::unique(emitPoints.begin(), emitPoints.end());
      emitPoints.setSize(newend - emitPoints.begin());
      std::sort(emitPoints.begin(), emitPoints.end());
      
      std::sort(emitEdges.begin(), emitEdges.end());
      U16* newend16 = std::unique(emitEdges.begin(), emitEdges.end());
      emitEdges.setSize(newend16 - emitEdges.begin());
      std::sort(emitEdges.begin(), emitEdges.end());

      // We need to transmute the edges to be relative to the points in the order of
      //  emission
      for (U32 j = 0; j < emitEdges.size(); j++) {
         U16 firstIndex = emitEdges[j] >> 8;
         U16 nextIndex  = emitEdges[j] & 0xFF;

         U16 newFirst = 0xFFFF;
         U16 newNext  = 0xFFFF;
         for (U32 k = 0; k < emitPoints.size(); k++) {
            if (emitPoints[k] == firstIndex)
               newFirst = k;
            if (emitPoints[k] == nextIndex)
               newNext = k;
         }
         AssertFatal(newFirst != 0xFFFF && newNext != 0xFFFF, "Error, bad edge!");

         U16 newSignature = (getMin(newFirst, newNext) << 8) |
            (getMax(newFirst, newNext) << 0);
         emitEdges[j] = newSignature;
      }

      // Emit string length is:
      //  1 + NumPoints +
      //  1 + (NumEdges * 2) +
      //  1 + Sum_polys(1 + 1 + numVerts(poly))

      U32 emitStringLen = 0;
      emitStringLen += 1 + emitPoints.size();
      emitStringLen += 1 + (emitEdges.size() * 2);
      emitStringLen += 1;
      for (U32 j = 0; j < emitPolys.size(); j++)
         emitStringLen += 1 + 1 + pSortedPolys[emitPolys[j]].numPoints;

      U8* emitString = new U8[emitStringLen];
      U32 currPos = 0;
      dMemset(emitString, 0, emitStringLen);

      // First, dump the points
      emitString[currPos++] = emitPoints.size();
      for (U32 j = 0; j < emitPoints.size(); j++)
         emitString[currPos++] = emitPoints[j];

      // Now the edges
      emitString[currPos++] = emitEdges.size();
      for (U32 j = 0; j < emitEdges.size(); j++) {
         U16 edgeP1 = emitEdges[j] >> 8;
         U16 edgeP2 = emitEdges[j] & 0xFF;

         emitString[currPos++] = edgeP1;
         emitString[currPos++] = edgeP2;
      }

      // And the polys.
      emitString[currPos++] = emitPolys.size();
      for (U32 j = 0; j < emitPolys.size(); j++) {
         // A poly is:
         //  vertex count
         //  plane index
         //  verts (relative)
         emitString[currPos++] = pSortedPolys[emitPolys[j]].numPoints;
         emitString[currPos++] = emitPolys[j];
         for (U32 k = 0; k < pSortedPolys[emitPolys[j]].numPoints; k++) {
            bool found = false;
            for (U32 l = 0; l < emitPoints.size(); l++) {
               if (emitPoints[l] == pSortedPolys[emitPolys[j]].points[k]) {
                  emitString[currPos++] = l;
                  found = true;
                  break;
               }
            }
            AssertFatal(found, "Error, missed a point!");
         }
      }
      AssertFatal(currPos == emitStringLen, "Error, miscalculation of string len!");

      U32 emitStringExportedIndex = exportVehicleEmitStringToRuntime(pRuntime, emitString, emitStringLen);
      pRuntime->mVehicleHullEmitStringIndices.push_back(emitStringExportedIndex);
      AssertFatal(dMemcmp(&pRuntime->mVehicleConvexHullEmitStrings[emitStringExportedIndex], emitString, emitStringLen) == 0, "Bad exported string!");
      delete [] emitString;
   }
   AssertFatal(pRuntime->mVehicleHullEmitStringIndices.size() == pRuntime->mVehicleHullIndices.size(),
               "Point/emitstring mismatch!");
   
   pRuntime->mVehicleConvexHulls.last().planeStart = pRuntime->mVehicleHullPlaneIndices.size();
   for (U32 i = 0; i < brush->mPlanes.size(); i++)
      pRuntime->mVehicleHullPlaneIndices.push_back(pSortedPolys[i].planeIndex);
   
   for (U32 i = 0; i < brush->mPlanes.size(); i++)
      delete [] pWindingRemap[i];
   delete [] pWindingRemap;
   delete [] pSortedPolys;

   // Now dump out the surfaces that were created by this hull that are still in
   //  the interior...
   pRuntime->mVehicleConvexHulls.last().surfaceStart = pRuntime->mVehicleHullSurfaceIndices.size();
   for (U32 i = 0; i < mVehicleNullSurfaces.size(); i++) {
      if (mVehicleNullSurfaces[i].winding.brushId == brush->brushId) {
         // Dump this surface
         pRuntime->mVehicleHullSurfaceIndices.push_back(makeVehicleNullSurfaceIndex(i));
      }
   }
   pRuntime->mVehicleConvexHulls.last().surfaceCount = (pRuntime->mVehicleHullSurfaceIndices.size() -
                                                        pRuntime->mVehicleConvexHulls.last().surfaceStart);
}


void EditGeometry::exportHullBins(Interior* pRuntime)
{
   pRuntime->mCoordBinMode = Interior::BinsXY;

   for (U32 i = 0; i < Interior::NumCoordBins; i++) {
      F32 minX = pRuntime->mBoundingBox.min.x;
      F32 maxX = pRuntime->mBoundingBox.min.x;
      minX += F32(i)   * (pRuntime->mBoundingBox.len_x() / F32(Interior::NumCoordBins));
      maxX += F32(i+1) * (pRuntime->mBoundingBox.len_x() / F32(Interior::NumCoordBins));

      for (U32 j = 0; j < Interior::NumCoordBins; j++) {
         F32 minY = pRuntime->mBoundingBox.min.y;
         F32 maxY = pRuntime->mBoundingBox.min.y;
         minY += F32(j)   * (pRuntime->mBoundingBox.len_y() / F32(Interior::NumCoordBins));
         maxY += F32(j+1) * (pRuntime->mBoundingBox.len_y() / F32(Interior::NumCoordBins));

         // This isn't really the fastest way to do this, but it is convenient...
         RectF binRect(Point2F(minX, minY), Point2F(maxX - minX, maxY - minY));
         AssertFatal(binRect.isValidRect(), "Error, invalid rect for bin!");

         U32 binIndex = (i * Interior::NumCoordBins) + j;
         pRuntime->mCoordBins[binIndex].binStart = pRuntime->mCoordBinIndices.size();

         for (U32 k = 0; k < pRuntime->mConvexHulls.size(); k++) {
            const Interior::ConvexHull& rHull = pRuntime->mConvexHulls[k];
            RectF hullRect(Point2F(rHull.minX, rHull.minY), Point2F(rHull.maxX - rHull.minX, rHull.maxY - rHull.minY));
            AssertFatal(hullRect.isValidRect(), "Error, invalid rect for bin!");

            if (binRect.overlaps(hullRect)) {
               // Index k goes into bin [i][j]...
               pRuntime->mCoordBinIndices.push_back(k);
            }
         }
         pRuntime->mCoordBins[binIndex].binCount = (pRuntime->mCoordBinIndices.size() -
                                                    pRuntime->mCoordBins[binIndex].binStart);
      }                                                                                
   }
}
