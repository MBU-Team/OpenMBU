//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "sceneGraph/windingClipper.h"

void sgUtil_clipToPlane(Point3F* points, U32& rNumPoints, const PlaneF& rPlane)
{
   S32 start = -1;
   for (U32 i = 0; i < rNumPoints; i++) {
      if (rPlane.whichSide(points[i]) == PlaneF::Front) {
         start = i;
         break;
      }
   }

   // Nothing was in front of the plane...
   if (start == -1) {
      rNumPoints = 0;
      return;
   }

   Point3F finalPoints[128];
   U32  numFinalPoints = 0;

   U32 baseStart = start;
   U32 end       = (start + 1) % rNumPoints;

   while (end != baseStart) {
      const Point3F& rStartPoint = points[start];
      const Point3F& rEndPoint   = points[end];

      PlaneF::Side fSide = rPlane.whichSide(rStartPoint);
      PlaneF::Side eSide = rPlane.whichSide(rEndPoint);

      S32 code = fSide * 3 + eSide;
      switch (code) {
        case 4:   // f f
        case 3:   // f o
        case 1:   // o f
        case 0:   // o o
         // No Clipping required
         finalPoints[numFinalPoints++] = points[start];
         start = end;
         end   = (end + 1) % rNumPoints;
         break;


        case 2: { // f b
            // In this case, we emit the front point, Insert the intersection,
            //  and advancing to point to first point that is in front or on...
            //
            finalPoints[numFinalPoints++] = points[start];

            Point3F vector = rEndPoint - rStartPoint;
            F32 t        = -(rPlane.distToPlane(rStartPoint) / mDot(rPlane, vector));

            Point3F intersection = rStartPoint + (vector * t);
            finalPoints[numFinalPoints++] = intersection;

            U32 endSeek = (end + 1) % rNumPoints;
            while (rPlane.whichSide(points[endSeek]) == PlaneF::Back)
               endSeek = (endSeek + 1) % rNumPoints;

            end   = endSeek;
            start = (end + (rNumPoints - 1)) % rNumPoints;

            const Point3F& rNewStartPoint = points[start];
            const Point3F& rNewEndPoint   = points[end];

            vector = rNewEndPoint - rNewStartPoint;
            t = -(rPlane.distToPlane(rNewStartPoint) / mDot(rPlane, vector));

            intersection = rNewStartPoint + (vector * t);
            points[start] = intersection;
         }
         break;

        case -1: {// o b
            // In this case, we emit the front point, and advance to point to first
            //  point that is in front or on...
            //
            finalPoints[numFinalPoints++] = points[start];

            U32 endSeek = (end + 1) % rNumPoints;
            while (rPlane.whichSide(points[endSeek]) == PlaneF::Back)
               endSeek = (endSeek + 1) % rNumPoints;

            end   = endSeek;
            start = (end + (rNumPoints - 1)) % rNumPoints;

            const Point3F& rNewStartPoint = points[start];
            const Point3F& rNewEndPoint   = points[end];

            Point3F vector = rNewEndPoint - rNewStartPoint;
            F32 t        = -(rPlane.distToPlane(rNewStartPoint) / mDot(rPlane, vector));

            Point3F intersection = rNewStartPoint + (vector * t);
            points[start] = intersection;
         }
         break;

        case -2:  // b f
        case -3:  // b o
        case -4:  // b b
         // In the algorithm used here, this should never happen...
         AssertISV(false, "SGUtil::clipToPlane: error in polygon clipper");
         break;

        default:
         AssertFatal(false, "SGUtil::clipToPlane: bad outcode");
         break;
      }

   }

   // Emit the last point.
   finalPoints[numFinalPoints++] = points[start];
   AssertFatal(numFinalPoints >= 3, avar("Error, this shouldn't happen!  Invalid winding in clipToPlane: %d", numFinalPoints));

   // Copy the new rWinding, and we're set!
   //
   dMemcpy(points, finalPoints, numFinalPoints * sizeof(Point3F));
   rNumPoints = numFinalPoints;
   AssertISV(rNumPoints <= 128, "MaxWindingPoints exceeded in scenegraph.  Fatal error.");
}
