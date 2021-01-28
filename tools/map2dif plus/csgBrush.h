//-----------------------------------------------------------------------------
// Torque Shader Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _CSGBRUSH_H_
#define _CSGBRUSH_H_

//Includes
#ifndef _PLATFORM_H_
#include "platform/platform.h"
#endif
#ifndef _MORIANBASICS_H_
#include "map2dif plus/morianBasics.h"
#endif
#ifndef _EDITGEOMETRY_H_
#include "map2dif plus/editGeometry.h"
#endif
#ifndef _MORIANUTIL_H_
#include "map2dif plus/morianUtil.h"
#endif
#ifndef _MMATHFN_H_
#include "math/mMathFn.h"
#endif

#include "collision/convexBrush.h"

bool parseBrush (CSGBrush& brush, Tokenizer* pToker, EditGeometry& geom);
void CopyConvexToCSG(ConvexBrush* convex, CSGBrush* csg);
void CopyCSGToConvex(CSGBrush* csg, ConvexBrush* convex);
U32 addTexGen(PlaneF tGenX, PlaneF tGenY);

class CSGPlane {
  public:
   // Plane equation given as mDot(normal, (x, y, z)) = d;
   U32         planeEQIndex;

   const char* pTextureName;
   F32         xShift;
   F32         yShift;
   F32         rotation;
   F32         xScale;
   F32         yScale;

   U32         texGenIndex;

   Winding     winding;

   U32         flags;

   EditGeometry::Entity* owningEntity;

  public:
   const Point3D& getNormal() const;
   F64         getDist() const;

   PlaneSide whichSide(const Point3D&) const;
   F64    distanceToPlane(const Point3D&) const;
   void      snapPointToPlane(Point3D&) const;
   Point3D *   sharesEdgeWith(const Winding &) const;
   void        extrude(F64 byAmount);
   PlaneSide   sideCheckEpsilon(const Point3D& testPoint, F64 E=0.000000001) const;
   const char* sideCheckInfo(const Point3D & point, const char * msg, F64 epsilon) const;

   bool createBaseWinding(const Vector<U32>&);
   bool clipWindingToPlaneFront(const U32 planeEQIndex);
   void construct(const Point3D& Point1, const Point3D& Point2, const Point3D& Point3);

   enum Flags {
      Inserted = 1 << 0
   };

   void markInserted()     { flags |= Inserted; }
   void markUninserted()   { flags &= ~Inserted; }
   bool isInserted() const { return (flags & Inserted) != 0; }
};

class CSGBrush
{
  public:
   bool intersectPlanes(U32 i, U32 j, U32 k, Point3D* pOutput);

  public:
   CSGBrush() : mIsAmbiguous(false) { }

   Vector<CSGPlane> mPlanes;
   Point3D          mMinBound;
   Point3D          mMaxBound;

   bool             mIsAmbiguous;
   BrushType        mBrushType;
   U32           brushId;
   U32           materialType;

   CSGBrush* pNext;

  public:
   CSGPlane& constructBrushPlane(const Point3I& rPoint1,
                                 const Point3I& rPoint2,
                                 const Point3I& rPoint3);
   CSGPlane& constructBrushPlane(const Point3D&, const Point3D&,const Point3D&);
   bool disambiguate();
   bool selfClip();

   U32 addPlane(CSGPlane plane);

   bool doesBBoxSersect(const CSGBrush& testBrush) const;
   bool isEquivalent(const CSGBrush& testBrush) const;
   bool noMoreInsertables() const;


  public:
   void copyBrush(const CSGBrush* pCopy);
   CSGBrush * createExtruded(const Point3D & extDir, bool bothWays) const;
};

//------------------------------------------------------------------------------
inline const Point3D& CSGPlane::getNormal() const
{
   return gWorkingGeometry->getPlaneEQ(planeEQIndex).normal;
}

inline F64 CSGPlane::getDist() const
{
   AssertFatal(gWorkingGeometry != NULL, "No working geometry?");

   return gWorkingGeometry->getPlaneEQ(planeEQIndex).dist;
}

inline PlaneSide CSGPlane::whichSide(const Point3D& testPoint) const
{
   return gWorkingGeometry->getPlaneEQ(planeEQIndex).whichSide(testPoint);
}

inline F64 CSGPlane::distanceToPlane(const Point3D& rPoint) const
{
   return gWorkingGeometry->getPlaneEQ(planeEQIndex).distanceToPlane(rPoint);
}

inline bool CSGPlane::clipWindingToPlaneFront(const U32 _planeEQIndex)
{
   return ::clipWindingToPlaneFront(&winding, _planeEQIndex);
}

inline void CSGPlane::snapPointToPlane(Point3D& rPoint) const
{
   F64 distance = mDot(rPoint, getNormal()) + getDist();
   rPoint -= getNormal() * distance;
}

inline bool CSGBrush::doesBBoxSersect(const CSGBrush& testBrush) const
{
   if (testBrush.mMinBound.x > mMaxBound.x ||
       testBrush.mMinBound.y > mMaxBound.y ||
       testBrush.mMinBound.z > mMaxBound.z)
      return false;
   if (testBrush.mMaxBound.x < mMinBound.x ||
       testBrush.mMaxBound.y < mMinBound.y ||
       testBrush.mMaxBound.z < mMinBound.z)
      return false;

   return true;
}

inline bool CSGBrush::noMoreInsertables() const
{
   for (U32 i = 0; i < mPlanes.size(); i++)
      if (mPlanes[i].isInserted() == false)
         return false;

   return true;
}

#endif //_CSGBRUSH_H_
