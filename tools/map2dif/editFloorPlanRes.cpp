//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "map2dif/editGeometry.h"
#include "map2dif/editFloorPlanRes.h"

void EditFloorPlanResource::pushArea(const Winding & winding, U32 planeEQ)
{
   mWindings.push_back(winding);
   mPlaneEQs.push_back(planeEQ);
}

void EditFloorPlanResource::clearFloorPlan()
{
   mPlaneTable.clear();
   mPointTable.clear();
   mPointLists.clear();
   mAreas.clear();
}

// Build data to persist.
void EditFloorPlanResource::constructFloorPlan()
{
   UniqueVector   planes, points;
   U32            i, j, maxPointIndex=0, maxPlaneIndex=0, totalPoints=0;

   clearFloorPlan();

   // Get the lists of unique points and planes, figure out max for remap tables.
   for( i = 0; i < mWindings.size(); i++ )
   {
      const Winding & W = mWindings[i];
      for( j = 0; j < W.numIndices; j++, totalPoints++ )
      {
         if( W.indices[j] > maxPointIndex )
            maxPointIndex = W.indices[j];
         points.pushBackUnique( W.indices[j] );
      }
      if( mPlaneEQs[i] > maxPlaneIndex )
         maxPlaneIndex = mPlaneEQs[i];
      planes.pushBackUnique( mPlaneEQs[i] );
   }

   // Allocate index remap tables
   Vector<U32> remapPoints;   remapPoints.setSize(maxPointIndex + 1);
   Vector<U32> remapPlanes;   remapPlanes.setSize(maxPlaneIndex + 1);

   // Build the index remap tables while copying over the point and plane
   //    vector data into the FloorPlanResource.
   for( i = 0, mPointTable.reserve(points.size()); i < points.size(); i++ )
   {
      Point3D  point = gWorkingGeometry->getPoint(points[i]) / 32.0;
      mPointTable.push_back( Point3F(point.x,point.y,point.z) );
      remapPoints[ points[i] ] = i;
   }
   for( i = 0, mPlaneTable.reserve(planes.size()); i < planes.size(); i++ )
   {
      PlaneEQ  pl64 = gWorkingGeometry->getPlaneEQ(planes[i]);
      Point3F  norm ( pl64.normal.x, pl64.normal.y, pl64.normal.z );
      F64      dist ( pl64.dist / 32.0 );
      mPlaneTable.push_back( PlaneF(norm.x, norm.y, norm.z, dist) );
      remapPlanes[ planes[i] ] = i;
   }

   // Construct the areas
   for( i = 0, mPointLists.reserve(totalPoints); i < mWindings.size(); i++ )
   {
      const Winding & W = mWindings[i];
      Area  area(W.numIndices, mPointLists.size(), remapPlanes[mPlaneEQs[i]] );
      mAreas.push_back( area );
      for( j = 0; j < W.numIndices; j++ )
         mPointLists.push_back( remapPoints[ W.indices[j] ] );
   }
}
