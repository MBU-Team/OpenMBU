//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _EXTRUDEDPOLYLIST_H_
#define _EXTRUDEDPOLYLIST_H_

#ifndef _MMATH_H_
#include "math/mMath.h"
#endif
#ifndef _TVECTOR_H_
#include "core/tVector.h"
#endif
#ifndef _ABSTRACTPOLYLIST_H_
#include "collision/abstractPolyList.h"
#endif

struct Polyhedron;
struct CollisionList;

//----------------------------------------------------------------------------
/// Extruded Polytope PolyList
///
/// This class is used primarily for collision detection, by objects which need
/// to check for obstructions along their path. You feed it a polytope to
/// extrude along the direction of movement, and it gives you a list of collisions.
///
/// @see AbstractPolyList
class ExtrudedPolyList: public AbstractPolyList
{
public:
   struct Vertex {
      Point3F point;
      U32 mask;
   };

   struct Poly {
      PlaneF plane;
      SceneObject* object;
      U32 material;
   };

   struct ExtrudedFace {
      bool active;
      PlaneF plane;
      F32 maxDistance;
      U32 planeMask;
      F32 faceDot;
      F32 faceShift;
      F32 time;
      Point3F point;
      F32 height;
   };

   typedef Vector<ExtrudedFace> ExtrudedList;
   typedef Vector<PlaneF> PlaneList;
   typedef Vector<Vertex> VertexList;
   typedef Vector<U32> IndexList;

   static F32 EqualEpsilon;
   static F32 FaceEpsilon;

   // Internal data
   VertexList   mVertexList;
   IndexList    mIndexList;
   ExtrudedList mExtrudedList;
   PlaneList    mPlaneList;
   VectorF      mVelocity;
   VectorF      mNormalVelocity;
   F32          mFaceShift;
   Poly         mPoly;

   // Returned info
   CollisionList* mCollisionList;

   PlaneList mPolyPlaneList;

   //
private:
   bool testPoly(ExtrudedFace&);

public:
   ExtrudedPolyList();
   ~ExtrudedPolyList();
   void extrude(const Polyhedron&, const VectorF& vec);
   void setVelocity(const VectorF& velocity);
   void setCollisionList(CollisionList*);
   void adjustCollisionTime();
   void render();

   // Virtual methods
   bool isEmpty() const;
   U32  addPoint(const Point3F& p);
   U32  addPlane(const PlaneF& plane);
   void begin(U32 material, U32 surfaceKey);
   void plane(U32 v1,U32 v2,U32 v3);
   void plane(const PlaneF& p);
   void plane(const U32 index);
   void vertex(U32 vi);
   void end();

  protected:
   const PlaneF& getIndexedPlane(const U32 index);
};

#endif
