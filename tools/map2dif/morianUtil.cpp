//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "map2dif/morianUtil.h"
#include "map2dif/editGeometry.h"
#include "platform/platformAssert.h"
#include "math/mMath.h"
#include "map2dif/csgBrush.h"

// Global brush arena...
BrushArena gBrushArena(1024);



S32 euclidGCD(S32 one, S32 two)
{
   AssertFatal(one > 0 && two > 0, "Error");

   S32 m = one >= two ? one : two;
   S32 n = one >= two ? two : one;

   while (true) {
      S32 test = m % n;
      if (test == 0)
         return n;

      m = n;
      n = test;
   }
}

S32 planeGCD(S32 x, S32 y, S32 z, S32 d)
{
   S32* pNonZero = NULL;
   if (x != 0)      pNonZero = &x;
   else if (y != 0) pNonZero = &y;
   else if (z != 0) pNonZero = &z;
   else if (d != 0) pNonZero = &d;
   else {
      AssertFatal(false, "Cannot pass all zero to GCD!");
      return 1;
   }

   if (x == 0) return planeGCD(*pNonZero, y, z, d);
   if (y == 0) return planeGCD(x, *pNonZero, z, d);
   if (z == 0) return planeGCD(x, y, *pNonZero, d);
   if (d == 0) return planeGCD(x, y, z, *pNonZero);

   if (x < 0) x = -x;
   if (y < 0) y = -y;
   if (z < 0) z = -z;
   if (d < 0) d = -d;

   // Do a Euclid on these...
   return euclidGCD(euclidGCD(euclidGCD(x, y), z), d);
}

void dumpWinding(const Winding& winding, const char* prefixString)
{
   for (U32 i = 0; i < winding.numIndices; i++) {
      const Point3D& rPoint = gWorkingGeometry->getPoint(winding.indices[i]);
      dPrintf("%s(%d) %lg, %lg, %lg\n", prefixString, winding.indices[i], rPoint.x, rPoint.y, rPoint.z);
   }
}

//------------------------------------------------------------------------------
F64 getWindingSurfaceArea(const Winding& rWinding, U32 planeEQIndex)
{
   // DMMTODO poly area...
   Point3D areaNormal(0, 0, 0);
   for (U32 i = 0; i < rWinding.numIndices; i++) {
      U32 j = (i + 1) % rWinding.numIndices;

      Point3D temp;
      mCross(gWorkingGeometry->getPoint(rWinding.indices[i]),
             gWorkingGeometry->getPoint(rWinding.indices[j]), &temp);

      areaNormal += temp;
   }

   F64 area = mDot(gWorkingGeometry->getPlaneEQ(planeEQIndex).normal, areaNormal);
   if (area < 0.0)
      area *= -0.5;
   else
      area *= 0.5;

   return area;
}

//------------------------------------------------------------------------------
// Simple in-place clipper.  Should be all that's required...
//
bool clipWindingToPlaneFront(Winding* winding, const PlaneEQ& rClipPlane)
{
   U32 start;
   for (start = 0; start < winding->numIndices; start++) {
      const Point3D& rPoint = gWorkingGeometry->getPoint(winding->indices[start]);
      if (rClipPlane.whichSide(rPoint) == PlaneFront)
         break;
   }

   if (start == winding->numIndices) {
      winding->numIndices = 0;
      return true;
   }

   U32 finalIndices[MaxWindingPoints];
   U32 numFinalIndices = 0;

   U32 baseStart = start;
   U32 end       = (start + 1) % winding->numIndices;

   bool modified = false;
   while (end != baseStart) {
      const Point3D& rStartPoint = gWorkingGeometry->getPoint(winding->indices[start]);
      const Point3D& rEndPoint   = gWorkingGeometry->getPoint(winding->indices[end]);

      PlaneSide fSide = rClipPlane.whichSide(rStartPoint);
      PlaneSide eSide = rClipPlane.whichSide(rEndPoint);

      S32 code = fSide * 3 + eSide;
      switch (code) {
        case 4:   // f f
        case 3:   // f o
        case 1:   // o f
        case 0:   // o o
         // No Clipping required
         finalIndices[numFinalIndices++] = winding->indices[start];
         start = end;
         end   = (end + 1) % winding->numIndices;
         break;


        case 2: { // f b
            // In this case, we emit the front point, Insert the intersection,
            //  and advancing to point to first point that is in front or on...
            //
            finalIndices[numFinalIndices++] = winding->indices[start];

            Point3D vector = rEndPoint - rStartPoint;
            F64 t = -(rClipPlane.distanceToPlane(rStartPoint) / mDot(rClipPlane.normal, vector));

            Point3D intersection = rStartPoint + (vector * t);
            AssertFatal(rClipPlane.whichSide(intersection) == PlaneOn,
                        "CSGPlane::clipWindingToPlaneFront: error in computing intersection");
            finalIndices[numFinalIndices++] = gWorkingGeometry->insertPoint(intersection);

            U32 endSeek = (end + 1) % winding->numIndices;
            while (rClipPlane.whichSide(gWorkingGeometry->getPoint(winding->indices[endSeek])) == PlaneBack)
               endSeek = (endSeek + 1) % winding->numIndices;

            PlaneSide esSide = rClipPlane.whichSide(gWorkingGeometry->getPoint(winding->indices[endSeek]));
            if (esSide == PlaneFront) {
               end   = endSeek;
               start = (end + (winding->numIndices - 1)) % winding->numIndices;

               const Point3D& rNewStartPoint = gWorkingGeometry->getPoint(winding->indices[start]);
               const Point3D& rNewEndPoint   = gWorkingGeometry->getPoint(winding->indices[end]);

               vector = rNewEndPoint - rNewStartPoint;
               t = -(rClipPlane.distanceToPlane(rNewStartPoint) / mDot(rClipPlane.normal, vector));

               intersection = rNewStartPoint + (vector * t);
               AssertFatal(rClipPlane.whichSide(intersection) == PlaneOn,
                           "CSGPlane::clipWindingToPlaneFront: error in computing intersection");

               winding->indices[start] = gWorkingGeometry->insertPoint(intersection);
               AssertFatal(winding->indices[start] != winding->indices[end], "Error");
            } else {
               start = endSeek;
               end   = (endSeek + 1) % winding->numIndices;
            }
            modified = true;
         }
         break;

        case -1: {// o b
            // In this case, we emit the front point, and advance to point to first
            //  point that is in front or on...
            //
            finalIndices[numFinalIndices++] = winding->indices[start];

            U32 endSeek = (end + 1) % winding->numIndices;
            while (rClipPlane.whichSide(gWorkingGeometry->getPoint(winding->indices[endSeek])) == PlaneBack)
               endSeek = (endSeek + 1) % winding->numIndices;

            PlaneSide esSide = rClipPlane.whichSide(gWorkingGeometry->getPoint(winding->indices[endSeek]));
            if (esSide == PlaneFront) {
               end   = endSeek;
               start = (end + (winding->numIndices - 1)) % winding->numIndices;

               const Point3D& rNewStartPoint = gWorkingGeometry->getPoint(winding->indices[start]);
               const Point3D& rNewEndPoint   = gWorkingGeometry->getPoint(winding->indices[end]);

               Point3D vector = rNewEndPoint - rNewStartPoint;
               F64  t      = -(rClipPlane.distanceToPlane(rNewStartPoint) / mDot(rClipPlane.normal, vector));

               Point3D intersection = rNewStartPoint + (vector * t);
               AssertFatal(rClipPlane.whichSide(intersection) == PlaneOn,
                           "CSGPlane::clipWindingToPlaneFront: error in computing intersection");

               winding->indices[start] = gWorkingGeometry->insertPoint(intersection);
               AssertFatal(winding->indices[start] != winding->indices[end], "Error");
            } else {
               start = endSeek;
               end   = (endSeek + 1) % winding->numIndices;
            }
            modified = true;
         }
         break;

        case -2:  // b f
        case -3:  // b o
        case -4:  // b b
         // In the algorithm used here, this should never happen...
         AssertISV(false, "CSGPlane::clipWindingToPlaneFront: error in polygon clipper");
         break;

        default:
         AssertFatal(false, "CSGPlane::clipWindingToPlaneFront: bad outcode");
         break;
      }

   }

   // Emit the last point.
   AssertFatal(rClipPlane.whichSide(gWorkingGeometry->getPoint(winding->indices[start])) != PlaneBack,
               "CSGPlane::clipWindingToPlaneFront: bad final point in clipper");
   finalIndices[numFinalIndices++] = winding->indices[start];
   AssertFatal(numFinalIndices >= 3, "Error, line out of clip!");

   // Copy the new rWinding, and we're set!
   //
   dMemcpy(winding->indices, finalIndices, numFinalIndices * sizeof(U32));
   winding->numIndices = numFinalIndices;
   AssertISV(winding->numIndices <= MaxWindingPoints,
             avar("Increase maxWindingPoints.  Talk to DMoore (%d, %d)",
                  MaxWindingPoints, numFinalIndices));

   // DEBUG POINT: No colinear points in the winding
   // DEBUG POINT: all points unique in the winding

   return modified;
}


bool clipWindingToPlaneFront(Winding* winding, U32 planeEQIndex)
{
   // DEBUG POINT: winding contains no colinear points...

   const PlaneEQ& rClipPlane = gWorkingGeometry->getPlaneEQ(planeEQIndex);
   return clipWindingToPlaneFront(winding, rClipPlane);
}


U32 windingWhichSide(const Winding& rWinding, U32 windingPlaneIndex, U32 planeIndex)
{
   AssertFatal(rWinding.numIndices > 2, "Error, winding with invalid number of indices...");

   if (gWorkingGeometry->isCoplanar(windingPlaneIndex, planeIndex))
      return PlaneSpecialOn;

   const PlaneEQ& rPlane = gWorkingGeometry->getPlaneEQ(planeIndex);

   U32 retSide = 0;
   for (U32 i = 0; i < rWinding.numIndices; i++) {
      switch (rPlane.whichSide(gWorkingGeometry->getPoint(rWinding.indices[i]))) {
        case PlaneOn:
         retSide |= PlaneSpecialOn;
         break;
        case PlaneFront:
         retSide |= PlaneSpecialFront;
         break;
        case PlaneBack:
         retSide |= PlaneSpecialBack;
         break;
      }
   }

   return retSide;
}

struct EdgeConnection {
   U32 start;
   U32 end;
} boxEdges[] = {
   { 0, 1 },
   { 1, 2 },
   { 2, 3 },
   { 3, 0 },
   { 4, 5 },
   { 5, 6 },
   { 6, 7 },
   { 7, 4 },
   { 3, 7 },
   { 0, 4 },
   { 1, 5 },
   { 2, 6 }
};

void createBoundedWinding(const Point3D& minBound,
                          const Point3D& maxBound,
                          U32         planeEQIndex,
                          Winding*       finalWinding)
{
   Point3D boxPoints[8];
   boxPoints[0].set(minBound.x, minBound.y, minBound.z);
   boxPoints[1].set(maxBound.x, minBound.y, minBound.z);
   boxPoints[2].set(maxBound.x, minBound.y, maxBound.z);
   boxPoints[3].set(minBound.x, minBound.y, maxBound.z);
   boxPoints[4].set(minBound.x, maxBound.y, minBound.z);
   boxPoints[5].set(maxBound.x, maxBound.y, minBound.z);
   boxPoints[6].set(maxBound.x, maxBound.y, maxBound.z);
   boxPoints[7].set(minBound.x, maxBound.y, maxBound.z);

   const PlaneEQ& rPlaneEQ = gWorkingGeometry->getPlaneEQ(planeEQIndex);

   PlaneSide sides[8];
   for (U32 i = 0; i < 8; i++)
      sides[i] = rPlaneEQ.whichSide(boxPoints[i]);

   UniqueVector unorderedWinding;
   for (U32 i = 0; i < 8; i++)
      if (sides[i] == PlaneOn)
         unorderedWinding.pushBackUnique(gWorkingGeometry->insertPoint(boxPoints[i]));

   for (U32 i = 0; i < sizeof(boxEdges) / sizeof(EdgeConnection); i++) {
      if ((sides[boxEdges[i].start] == PlaneBack && sides[boxEdges[i].end]   == PlaneFront) ||
          (sides[boxEdges[i].end]   == PlaneBack && sides[boxEdges[i].start] == PlaneFront)) {
         // Emit the intersection...
         //
         Point3D vector = boxPoints[boxEdges[i].end] - boxPoints[boxEdges[i].start];
         F64 t = -(rPlaneEQ.distanceToPlane(boxPoints[boxEdges[i].start]) / mDot(rPlaneEQ.normal, vector));

         Point3D intersection = boxPoints[boxEdges[i].start] + (vector * t);
         unorderedWinding.pushBackUnique(gWorkingGeometry->insertPoint(intersection));
      }
   }

   if (unorderedWinding.size() <= 2) {
      AssertISV(false, "createBoundedWinding: Bad face on brush.  < 3 points.  DMMNOTE: Handle better?");
      finalWinding->numIndices = 0;
      return;
   }

   Vector<U32> modPoints = unorderedWinding;

   // Create a centroid first...
   Point3D centroid(0, 0, 0);
   for (U32 i = 0; i < modPoints.size(); i++)
      centroid += gWorkingGeometry->getPoint(modPoints[i]);
   centroid /= F64(modPoints.size());

   // Ok, we have a centroid.  We (arbitrarily) take the last point, and start from
   //  there...
   finalWinding->indices[0] = modPoints[modPoints.size() - 1];
   finalWinding->numIndices = 1;
   modPoints.erase(modPoints.size() - 1);

   while (modPoints.size() != 0) {
      // Find the largest dot product of a point with a positive facing
      //  cross product.
      //
      F64 maxDot  = -1e10;
      S32 maxIndex = -1;

      const Point3D& currPoint = gWorkingGeometry->getPoint(finalWinding->indices[finalWinding->numIndices - 1]);
      Point3D currVector       = currPoint - centroid;
      currVector.normalize();

      for (U32 i = 0; i < modPoints.size(); i++) {
         const Point3D& testPoint = gWorkingGeometry->getPoint(modPoints[i]);
         Point3D testVector       = testPoint - centroid;
         testVector.normalize();

         Point3D crossVector;
         mCross(currVector, testVector, &crossVector);

         if (mDot(crossVector, rPlaneEQ.normal) < 0.0) {
            // Candidate
            F32 val = mDot(testVector, currVector);
            if (val > maxDot) {
               maxDot   = val;
               maxIndex = i;
            }
         }
      }
      AssertISV(maxIndex != -1, "createBoundedWinding: no point found for winding?");

      finalWinding->indices[finalWinding->numIndices++] = modPoints[maxIndex];
      modPoints.erase(maxIndex);
   }
}


struct FooeyDebug {
   Point3D p;
   F64     d;
};

void splitBrush(CSGBrush*& inBrush,
                U32        planeEQIndex,
                CSGBrush*& outFront,
                CSGBrush*& outBack)
{
   AssertFatal(outFront == NULL && outBack == NULL, "Must be null on entry");

   CSGBrush* pFront = gBrushArena.allocateBrush();
   CSGBrush* pBack  = gBrushArena.allocateBrush();

   pFront->copyBrush(inBrush);
   pBack->copyBrush(inBrush);

   bool regen = false;
   for (S32 i = S32(pFront->mPlanes.size()) - 1; i >= 0; i--) {
      CSGPlane& rPlane = pFront->mPlanes[i];

      if (rPlane.planeEQIndex == planeEQIndex ||
          rPlane.planeEQIndex == gWorkingGeometry->getPlaneInverse(planeEQIndex))
         continue;

      if (rPlane.clipWindingToPlaneFront(planeEQIndex) == true) {
         // Winding was modified
         regen = true;

         if (rPlane.winding.numIndices == 0) {
            // Plane is completely gone.  nuke it.
            pFront->mPlanes.erase(i);
         }
      }
   }
   if (pFront->mPlanes.size() <= 1) {
      // Nuke it.
      gBrushArena.freeBrush(pFront);
      pFront = NULL;
   } else {
      // Otherwise, we have a good brush still.  Check to see if we need to add
      //  a plane, and regen, or can just return it...
      if (regen == true) {
         pFront->mPlanes.increment();
         CSGPlane& rPlane = pFront->mPlanes.last();

         rPlane.planeEQIndex = gWorkingGeometry->getPlaneInverse(planeEQIndex);
         rPlane.pTextureName = NULL;
         rPlane.flags        = 0;

         for (U32 i = 0; i < pFront->mPlanes.size(); i++)
            pFront->mPlanes[i].winding.numIndices = 0;

         pFront->selfClip();
      }

      outFront = pFront;
   }

   //-------------------------------------- now do the back...
   regen = false;
   for (S32 i = S32(pBack->mPlanes.size()) - 1; i >= 0; i--) {
      CSGPlane& rPlane = pBack->mPlanes[i];

      if (rPlane.planeEQIndex == planeEQIndex ||
          rPlane.planeEQIndex == gWorkingGeometry->getPlaneInverse(planeEQIndex))
         continue;

      if (rPlane.clipWindingToPlaneFront(gWorkingGeometry->getPlaneInverse(planeEQIndex)) == true) {
         // Winding was modified
         regen = true;

         if (rPlane.winding.numIndices == 0) {
            // Plane is completely gone.  nuke it.
            pBack->mPlanes.erase(i);
         }
      }
   }
   if (pBack->mPlanes.size() <= 1) {
      // Nuke it.
      gBrushArena.freeBrush(pBack);
      pBack = NULL;
   } else {
      // Otherwise, we have a good brush still.  Check to see if we need to add
      //  a plane, and regen, or can just return it...
      if (regen == true) {
         pBack->mPlanes.increment();
         CSGPlane& rPlane = pBack->mPlanes.last();

         rPlane.planeEQIndex = planeEQIndex;
         rPlane.pTextureName = NULL;
         rPlane.flags        = 0;

         for (U32 i = 0; i < pBack->mPlanes.size(); i++)
            pBack->mPlanes[i].winding.numIndices = 0;

         pBack->selfClip();
      }

      outBack = pBack;
   }
}


void assessPlane(const U32    testPlane,
                 const CSGBrush& rTestBrush,
                 S32*          numCoplanar,
                 S32*          numTinyWindings,
                 S32*          numSplits,
                 S32*          numFront,
                 S32*          numBack)
{
   for (U32 i = 0; i < rTestBrush.mPlanes.size(); i++) {
      if (gWorkingGeometry->isCoplanar(testPlane, rTestBrush.mPlanes[i].planeEQIndex)) {
         // Easy.  Brush abuts the plane.
         (*numCoplanar)++;
         if (testPlane == rTestBrush.mPlanes[i].planeEQIndex)
            (*numBack)++;
         else
            (*numFront)++;
         return;
      }
   }

   // Ah well, more work...
   const PlaneEQ& rPlane = gWorkingGeometry->getPlaneEQ(testPlane);

   static UniqueVector uniquePoints(64);
   for (U32 i = 0; i < rTestBrush.mPlanes.size(); i++) {
      for (U32 j = 0; j < rTestBrush.mPlanes[i].winding.numIndices; j++)
         uniquePoints.pushBackUnique(rTestBrush.mPlanes[i].winding.indices[j]);
   }

   F64 maxFront = 0.0;
   F64 minBack  = 0.0;

   for (U32 i = 0; i < uniquePoints.size(); i++) {
      const Point3D& rPoint = gWorkingGeometry->getPoint(uniquePoints[i]);

      F64 dist = rPlane.distanceToPlane(rPoint);
      if (dist > maxFront)
         maxFront = dist;
      if (dist < minBack)
         minBack = dist;
   }

   //AssertFatal(maxFront > gcPlaneDistanceEpsilon || minBack < -gcPlaneDistanceEpsilon,
   //            "This should only happen for coplanar windings...");

   if (maxFront >  gcPlaneDistanceEpsilon)
      (*numFront)++;

   if (minBack  < -gcPlaneDistanceEpsilon)
      (*numBack)++;

   if (maxFront >  gcPlaneDistanceEpsilon &&
       minBack  < -gcPlaneDistanceEpsilon)
      (*numSplits)++;


   if ((maxFront > 0.0 && maxFront < 1.0) ||
       (minBack  < 0.0 && minBack  > -1.0))
      (*numTinyWindings)++;

   // done.
   uniquePoints.clear();
}


//------------------------------------------------------------------------------
bool windingsEquivalent(const Winding& winding1, const Winding& winding2)
{
   if (winding1.numIndices != winding2.numIndices)
      return false;

   U32 oneCurr = 0;
   S32 twoCurr  = -1;
   for (U32 i = 0; i < winding2.numIndices; i++) {
      if (winding2.indices[i] == winding1.indices[0]) {
         twoCurr = i;
         break;
      }
   }
   if (twoCurr == -1)
      return false;

   for (U32 i = 0; i < winding1.numIndices; i++) {
      if (winding1.indices[oneCurr] != winding2.indices[twoCurr])
         return false;

      oneCurr = (oneCurr + 1) % winding1.numIndices;
      twoCurr = (twoCurr + 1) % winding2.numIndices;
   }

   return true;
}


//------------------------------------------------------------------------------
bool canCollapseWinding(const Winding& winding1, const Winding& winding2)
{
   // We are looking for an edge that is backwards in winding2 that is present
   //  in winding1.  Note that this is not a general purpose algorithm, it's
   //  very tailored to the break*Winding function from the BSP.
   //
   AssertFatal(winding1.numIndices >= 3 && winding2.numIndices >= 3, "improper windings");

   // First, lets check the ambient light condition...
   if (winding1.numZoneIds == 0 || winding2.numZoneIds == 0)
      return false;

   if (gWorkingGeometry->mZones[winding1.zoneIds[0]]->ambientLit !=
       gWorkingGeometry->mZones[winding2.zoneIds[0]]->ambientLit)
      return false;

   for (U32 i = 0; i < winding1.numIndices; i++) {
      U32 edgeStart = winding1.indices[(i + 1) % winding1.numIndices];
      U32 edgeEnd   = winding1.indices[i];

      for (U32 j = 0; j < winding2.numIndices; j++) {
         if (winding2.indices[j] == edgeStart &&
             winding2.indices[(j + 1) % winding2.numIndices] == edgeEnd) {
            // Yay!
            return true;
         }
      }
   }

   return false;
}

//------------------------------------------------------------------------------
bool pointsAreColinear(U32 p1, U32 p2, U32 p3)
{
   const Point3D& rP1 = gWorkingGeometry->getPoint(p1);
   const Point3D& rP2 = gWorkingGeometry->getPoint(p2);
   const Point3D& rP3 = gWorkingGeometry->getPoint(p3);

   // We are testing the distance from p3 to the line defined
   //  by p1 and p2.
   //
   F64 f = rP2.x - rP1.x;
   F64 g = rP2.y - rP1.y;
   F64 h = rP2.z - rP1.z;

   // P0 in our case is rP1.  Note the the following is taken from
   //  "A Programmer's Geometry"
   //
   F64 denom = f*f + g*g + h*h;
   if (denom < gcPlaneDistanceEpsilon * gcPlaneDistanceEpsilon) {
      // If two points are the same, then all three are colinear...
      return true;
   }

   F64 xj0 = rP3.x - rP1.x;
   F64 yj0 = rP3.y - rP1.y;
   F64 zj0 = rP3.z - rP1.z;

   F64 fygx = f * yj0 - g * xj0;
   F64 fzhx = f * zj0 - h * xj0;
   F64 gzhy = g * zj0 - h * yj0;

   F64 v1 =  g * fygx + h * fzhx;
   F64 v2 =  h * gzhy - f * fygx;
   F64 v3 = -f * fzhx - g * gzhy;

   F64 dist = mSqrt(v1*v1 + v2*v2 + v3*v3) / denom;

   if (dist < gcPlaneDistanceEpsilon)
      return true;
   else
      return false;
}


//------------------------------------------------------------------------------
bool collapseWindings(Winding& into, const Winding& from)
{
   if (canCollapseWinding(into, from) == false)
      return false;

   // DEBUG POINT: Check that into contains no colinear
   // DEBUG POINT: Check that from contains no colinear

   U32 oneStart;
   U32 twoStart;

   bool found = false;
   for (U32 i = 0; i < into.numIndices && !found; i++) {
      U32 edgeStart = into.indices[(i + 1) % into.numIndices];
      U32 edgeEnd   = into.indices[i];

      for (U32 j = 0; j < from.numIndices && !found; j++) {
         if (from.indices[j] == edgeStart &&
             from.indices[(j + 1) % from.numIndices] == edgeEnd) {
            found = true;
            oneStart = i;
            twoStart = j;
         }
      }
   }
   AssertFatal(found == true, "error, something missing in dodge city, no common edge!");

   AssertFatal((into.numIndices + from.numIndices - 2) < MaxWindingPoints,
               "Uh, need to increase MaxWindingPoints.  Talk to DMoore");
   U32 newIndices[64];
   U32 newCount = 0;

   for (U32 i = 0; i <= oneStart; i++) {
      newIndices[newCount++] = into.indices[i];
   }
   for (U32 i = (twoStart + 2) % from.numIndices; i != twoStart;) {
      newIndices[newCount++] = from.indices[i];

      i = (i+1) % from.numIndices;
   }
   for (U32 i = (oneStart + 1) % into.numIndices; i > 0;) {
      newIndices[newCount++] = into.indices[i];

      i = (i+1) % into.numIndices;
   }

   // Remove any colinear edges...
   for (U32 i = 0; i < newCount; i++) {
      U32 index0 = i;
      U32 index1 = (i + 1) % newCount;
      U32 index2 = (i + 2) % newCount;

      if (pointsAreColinear(newIndices[index0], newIndices[index1], newIndices[index2])) {
         // Remove the edge...
         U32 size = (newCount - index1 - 1) * sizeof(U32);
         dMemmove(&newIndices[index1], &newIndices[index1+1], size);
         newCount--;
      }
   }
   AssertFatal(newCount >= 3 && newCount <= MaxWindingPoints, "Ah, crap, something goofed in collapseWindings.  Talk to DMoore");

   // See if the new winding is convex
   Point3D reference;
   for (U32 i = 0; i < newCount; i++) {
      U32 j = (i + 1) % newCount;
      U32 k = (i + 2) % newCount;

      Point3D vec1 = gWorkingGeometry->getPoint(newIndices[i]) - gWorkingGeometry->getPoint(newIndices[j]);
      Point3D vec2 = gWorkingGeometry->getPoint(newIndices[k]) - gWorkingGeometry->getPoint(newIndices[j]);

      Point3D result;
      mCross(vec1, vec2, &result);
      if (i == 0) {
         reference = result;
      } else {
         F64 dot = mDot(reference, result);
         if (dot < 0.0) {
            return false;
         }
      }
   }

   // Copy the new rWinding, and we're set!
   //
   dMemcpy(into.indices, newIndices, newCount * sizeof(U32));
   into.numIndices = newCount;

   // Make sure the nodes are merged
   for (U32 i = 0; i < from.numNodes; i++) {
      bool found = false;
      for (U32 j = 0; j < into.numNodes; j++) {
         if (into.solidNodes[j] == from.solidNodes[i]) {
            found = true;
            break;
         }
      }
      if (found == false) {
         AssertFatal(into.numNodes < MaxWindingNodes, "Error, too many solid nodes affecting poly.  Plase ask DMoore to increase the allowed number!");
         into.solidNodes[into.numNodes++] = from.solidNodes[i];
      }
   }
   for (U32 i = 0; i < from.numZoneIds; i++)
      into.insertZone(from.zoneIds[i]);

   // DEBUG POINT: into must contain no colinear

   return true;
}


//------------------------------------------------------------------------------
void extendName(char* pName, const char* pExt)
{
   AssertFatal(pName != NULL && dStrlen(pName) != 0 && pExt != NULL && pExt[0] == '.',
               "Error, bad inputs to reextendName");

   dStrcat(pName, pExt);
}


void reextendName(char* pName, const char* pExt)
{
   AssertFatal(pName != NULL && dStrlen(pName) != 0 && pExt != NULL && pExt[0] == '.',
               "Error, bad inputs to reextendName");

   char* pDot = dStrrchr(pName, '.');
   if (pDot != NULL) {
      if (dStricmp(pDot, pExt) != 0) {
         dStrcpy(pDot, pExt);
      }
   } else {
      dStrcat(pName, pExt);
   }
}

bool createBaseWinding(const Vector<U32>& rPoints, const Point3D& normal, Winding* pWinding)
{
   if (rPoints.size() <= 2) {
      dPrintf("::createBaseWinding: Bad face on brush.  < 3 points.  DMMNOTE: Handle better?\n");
      return false;
   }
   if (rPoints.size() > 32) {
      dPrintf("::createBaseWinding: Bad face on brush.  > 32 points.  DMMNOTE: Increase max winding points?\n");
      return false;
   }


   Vector<U32> modPoints = rPoints;

   // Create a centroid first...
   Point3D centroid(0, 0, 0);
   for (U32 i = 0; i < modPoints.size(); i++)
      centroid += gWorkingGeometry->getPoint(modPoints[i]);
   centroid /= F64(modPoints.size());

   // Ok, we have a centroid.  We (arbitrarily) take the last point, and start from
   //  there...
   pWinding->indices[0] = modPoints[modPoints.size() - 1];
   pWinding->numIndices = 1;
   modPoints.erase(modPoints.size() - 1);

   while (modPoints.size() != 0) {
      // Find the largest dot product of a point with a positive facing
      //  cross product.
      //
      F64 maxDot  = -1e10;
      S32 maxIndex = -1;

      const Point3D& currPoint = gWorkingGeometry->getPoint(pWinding->indices[pWinding->numIndices - 1]);
      Point3D currVector       = currPoint - centroid;
      currVector.normalize();

      for (U32 i = 0; i < modPoints.size(); i++) {

         const Point3D& testPoint = gWorkingGeometry->getPoint(modPoints[i]);
         Point3D testVector       = testPoint - centroid;
         testVector.normalize();

         Point3D crossVector;
         mCross(currVector, testVector, &crossVector);
         crossVector.normalize();

         if (mDot(crossVector, normal) < 0.0) {
            // Candidate
            F64 val  = mDot(testVector, currVector);
            F64 dVal = mDot(testVector, currVector);

            if (val > maxDot) {
               maxDot   = val;
               maxIndex = i;
            }
         }
      }
      //AssertISV(maxIndex != -1, "::createBaseWinding: no point found for winding?");
      if(maxIndex == -1)
         return false;

      pWinding->indices[pWinding->numIndices++] = modPoints[maxIndex];
      modPoints.erase(maxIndex);
   }

   pWinding->numNodes = 0;
   pWinding->numZoneIds = 0;

   return true;
}


BrushArena::BrushArena(U32 _arenaSize)
{
   arenaValid    = true;
   arenaSize     = _arenaSize;
   mFreeListHead = NULL;
}

BrushArena::~BrushArena()
{
   arenaValid = false;
   for (U32 i = 0; i < mBuffers.size(); i++) {
      delete [] mBuffers[i];
      mBuffers[i] = NULL;
   }
   mFreeListHead = NULL;
   arenaSize     = 0;
}

void BrushArena::newBuffer()
{
   // Otherwise, we set up some more free windings...
   CSGBrush* pNewBuffer = new CSGBrush[arenaSize];
   mBuffers.push_back(pNewBuffer);
   for (U32 i = 0; i < arenaSize - 1; i++) {
      pNewBuffer[i].pNext = &pNewBuffer[i+1];
   }
   pNewBuffer[arenaSize - 1].pNext = NULL;

   mFreeListHead = pNewBuffer;
}

CSGBrush* BrushArena::allocateBrush()
{
   AssertFatal(arenaValid == true, "Arena invalid!");

   if (mFreeListHead == NULL)
      newBuffer();
   AssertFatal(mFreeListHead != NULL, "error, that shouldn't happen!");

   CSGBrush* pRet     = mFreeListHead;
   mFreeListHead      = pRet->pNext;
   pRet->pNext        = NULL;
   pRet->mIsAmbiguous = false;
   pRet->mPlanes.clear();
   return pRet;
}

void BrushArena::freeBrush(CSGBrush* junk)
{
   AssertFatal(arenaValid == true, "Arena invalid!");
   AssertFatal(junk->pNext == NULL, "Error, this is still on the free list!");

   junk->pNext   = mFreeListHead;
   mFreeListHead = junk;
}


bool intersectWGPlanes(U32 i, U32 j, U32 k,
                       Point3D* pOutput)
{
   const PlaneEQ& rPlaneEQ1 = gWorkingGeometry->getPlaneEQ(i);
   const PlaneEQ& rPlaneEQ2 = gWorkingGeometry->getPlaneEQ(j);
   const PlaneEQ& rPlaneEQ3 = gWorkingGeometry->getPlaneEQ(k);

   F64 bc  = (rPlaneEQ2.normal.y * rPlaneEQ3.normal.z) - (rPlaneEQ3.normal.y * rPlaneEQ2.normal.z);
   F64 ac  = (rPlaneEQ2.normal.x * rPlaneEQ3.normal.z) - (rPlaneEQ3.normal.x * rPlaneEQ2.normal.z);
   F64 ab  = (rPlaneEQ2.normal.x * rPlaneEQ3.normal.y) - (rPlaneEQ3.normal.x * rPlaneEQ2.normal.y);
   F64 det = (rPlaneEQ1.normal.x * bc) - (rPlaneEQ1.normal.y * ac) + (rPlaneEQ1.normal.z * ab);

   if (mFabs(det) < 1e-9) {
      // Parallel planes
      return false;
   }

   F64 dc     = (rPlaneEQ2.dist * rPlaneEQ3.normal.z) - (rPlaneEQ3.dist * rPlaneEQ2.normal.z);
   F64 db     = (rPlaneEQ2.dist * rPlaneEQ3.normal.y) - (rPlaneEQ3.dist * rPlaneEQ2.normal.y);
   F64 ad     = (rPlaneEQ3.dist * rPlaneEQ2.normal.x) - (rPlaneEQ2.dist * rPlaneEQ3.normal.x);
   F64 detInv = 1.0 / det;

   pOutput->x = ((rPlaneEQ1.normal.y * dc) - (rPlaneEQ1.dist     * bc) - (rPlaneEQ1.normal.z * db)) * detInv;
   pOutput->y = ((rPlaneEQ1.dist     * ac) - (rPlaneEQ1.normal.x * dc) - (rPlaneEQ1.normal.z * ad)) * detInv;
   pOutput->z = ((rPlaneEQ1.normal.y * ad) + (rPlaneEQ1.normal.x * db) - (rPlaneEQ1.dist     * ab)) * detInv;

   return true;
}
