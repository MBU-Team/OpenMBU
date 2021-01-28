//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _EARLYOUTPOLYLIST_H_
#define _EARLYOUTPOLYLIST_H_

#ifndef _ABSTRACTPOLYLIST_H_
#include "collision/abstractPolyList.h"
#endif


/// Early out check PolyList
///
/// This class is used primarily for triggers and similar checks. It checks to see
/// if any of the geometry you feed it is inside its area, and if it is, it stops
/// checking for any more data and returns a true value. This is good if you want
/// to know if anything is in your "trigger" area, for instance.
///
/// @see AbstractPolyList
class EarlyOutPolyList : public AbstractPolyList
{
   void memcpy(U32* d, U32* s,U32 size);

   // Internal data
   struct Vertex {
      Point3F point;
      U32 mask;
   };

   struct Poly {
      PlaneF plane;
      SceneObject* object;
      U32 material;
      U32 vertexStart;
      U32 vertexCount;
      U32 surfaceKey;
   };

  public:
   typedef Vector<PlaneF> PlaneList;
  private:
   typedef Vector<Vertex> VertexList;
   typedef Vector<Poly>   PolyList;
   typedef Vector<U32>    IndexList;

   PolyList   mPolyList;
   VertexList mVertexList;
   IndexList  mIndexList;
   bool       mEarlyOut;

   PlaneList  mPolyPlaneList;

  public:
   // Data set by caller
   PlaneList mPlaneList;
   VectorF   mNormal;

  public:
   EarlyOutPolyList();
   ~EarlyOutPolyList();
   void clear();

   // Virtual methods
   bool isEmpty() const;
   U32  addPoint(const Point3F& p);
   U32  addPlane(const PlaneF& plane);
   void begin(U32 material,U32 surfaceKey);
   void plane(U32 v1,U32 v2,U32 v3);
   void plane(const PlaneF& p);
   void plane(const U32 index);
   void vertex(U32 vi);
   void end();

  protected:
   const PlaneF& getIndexedPlane(const U32 index);
};

#endif  // _H_EARLYOUTPOLYLIST_
