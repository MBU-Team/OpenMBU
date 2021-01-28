//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _DEPTHSORTLIST_H_
#define _DEPTHSORTLIST_H_

#ifndef _CLIPPEDPOLYLIST_H_
#include "collision/clippedPolyList.h"
#endif


//----------------------------------------------------------------------------

class DepthSortList : public ClippedPolyList
{
   typedef ClippedPolyList Parent;
  public:
   struct PolyExtents
   {
      // extents of poly on each coordinate axis
      F32 xMin;
      F32 xMax;
      F32 yMin;
      F32 yMax;
      F32 zMin;
      F32 zMax;
   };

   typedef Vector<PolyExtents> PolyExtentsList;
   typedef Vector<U32> PolyIndexList;

   // Internal data
   PolyExtentsList mPolyExtentsList;
   PolyIndexList mPolyIndexList;
   Point3F mExtent; // dimensions of the sort area
   S32 mBase; // base position in the list...everything before this is sorted correctly
   Poly * mBasePoly; // poly currently in base position
   Point3F * mBaseNormal; // normal of poly currently in base position
   F32 mBaseDot; // dot of basePoly with baseNormal
   F32 mBaseYMax; // max y extent of base poly
   S32 mMaxTouched; // highest index swapped into thus far...y-extents may be improperly sorted before this index
   PolyExtents * mBaseExtents; // x,y,z extents of basePoly

   // set the base position -- everything before this point should be correctly sorted
   void setBase(S32);

   // some utilities used for sorting
   bool splitPoly(const Poly & sourcePoly, Point3F & normal, F32 k, Poly & front, Poly & back);
   bool overlap(Poly *, Poly *);
   void handleOverlap(Poly * testPoly, Point3F & testNormal, F32 testDot, S32 & testOffset, bool & switched);
   void sortByYExtents();
   void setExtents(Poly &, PolyExtents &);

   // one iteration of the sort routine -- finds a poly to fill current base position
   bool sortNext();

   // used by depthPartition
   void cookieCutter(Poly & cutter, Poly & cuttee,
                     Vector<Poly> & scraps,
                     Vector<Poly> & cookies, Vector<Point3F> & cookieVerts);

   void wireCube(const Point3F & size, const Point3F & pos);

  public:

   //
   DepthSortList();
   ~DepthSortList();
   void set(const MatrixF & mat, Point3F & extents);
   void clear();
   void clearSort();

   // the point of this class
   void sort();
   void depthPartition(const Point3F * sourceVerts, U32 numVerts, Vector<Poly> & partition, Vector<Point3F> & partitionVerts);

   // Virtual methods
   void end();
   // U32  addPoint(const Point3F& p);
   // bool isEmpty() const;
   // void begin(U32 material,U32 surfaceKey);
   // void plane(U32 v1,U32 v2,U32 v3);
   // void plane(const PlaneF& p);
   // void vertex(U32 vi);
   bool getMapping(MatrixF *, Box3F *);

   // access to the polys in order (note: returned pointers are volatile, may change if polys added).
   void getOrderedPoly(U32 ith, Poly ** poly, PolyExtents ** polyExtent);

   // unordered access
   PolyExtents & getExtents(U32 idx) { return mPolyExtentsList[idx]; }
   Poly & getPoly(U32 idx) { return mPolyList[idx]; }

   //
   void render();

   static bool renderWithDepth;
};

inline void DepthSortList::getOrderedPoly(U32 ith, Poly ** poly, PolyExtents ** polyExtent)
{
   *poly = &mPolyList[mPolyIndexList[ith]];
   *polyExtent = &mPolyExtentsList[mPolyIndexList[ith]];
}

#endif
