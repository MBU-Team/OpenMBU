//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "collision/abstractPolyList.h"


//----------------------------------------------------------------------------

AbstractPolyList::~AbstractPolyList()
{
   mInterestNormalRegistered = false;
}

static U32 PolyFace[6][4] = {
   { 3, 2, 1, 0 },
   { 7, 4, 5, 6 },
   { 0, 5, 4, 3 },
   { 6, 5, 0, 1 },
   { 7, 6, 1, 2 },
   { 4, 7, 2, 3 },
};

void AbstractPolyList::addBox(const Box3F &box)
{
   Point3F pos = box.min;
   F32 dx = box.max.x - box.min.x;
   F32 dy = box.max.y - box.min.y;
   F32 dz = box.max.z - box.min.z;

   U32 base = addPoint(pos);
   pos.y += dy; addPoint(pos);
   pos.x += dx; addPoint(pos);
   pos.y -= dy; addPoint(pos);
   pos.z += dz; addPoint(pos);
   pos.x -= dx; addPoint(pos);
   pos.y += dy; addPoint(pos);
   pos.x += dx; addPoint(pos);

   for (S32 i = 0; i < 6; i++) {
      begin(0,i);
      S32 v1 = base + PolyFace[i][0];
      S32 v2 = base + PolyFace[i][1];
      S32 v3 = base + PolyFace[i][2];
      S32 v4 = base + PolyFace[i][3];
      vertex(v1);
      vertex(v2);
      vertex(v3);
      vertex(v4);
      plane(v1,v2,v3);
      end();
   }
}

bool AbstractPolyList::getMapping(MatrixF *, Box3F *)
{
   // return list transform and bounds in list space...optional
   return false;
}


bool AbstractPolyList::isInterestedInPlane(const PlaneF& plane)
{
   if (mInterestNormalRegistered == false) {
      return true;
   }
   else {
      PlaneF xformed;
      mPlaneTransformer.transform(plane, xformed);
      if (mDot(xformed, mInterestNormal) >= 0.0f)
         return false;
      else
         return true;
   }
}

bool AbstractPolyList::isInterestedInPlane(const U32 index)
{
   if (mInterestNormalRegistered == false) {
      return true;
   }
   else {
      const PlaneF& rPlane = getIndexedPlane(index);
      if (mDot(rPlane, mInterestNormal) >= 0.0f)
         return false;
      else
         return true;
   }
}

void AbstractPolyList::setInterestNormal(const Point3F& normal)
{
   mInterestNormalRegistered = true;
   mInterestNormal           = normal;
}

