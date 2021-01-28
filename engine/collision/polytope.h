//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _POLYTOPE_H_
#define _POLYTOPE_H_

#ifndef _TVECTOR_H_
#include "core/tVector.h"
#endif


//----------------------------------------------------------------------------

class SimObject;


//----------------------------------------------------------------------------

class Polytope
{
   // Convex Polyhedron
public:
   struct Vertex {
      Point3F point;
      /// Temp BSP clip info
      S32 side;
   };
   struct Edge {
      S32 vertex[2];
      S32 face[2];
      S32 next;
   };
   struct Face {
      PlaneF plane;
      S32 original;
      /// Temp BSP clip info
      S32 vertex;
   };
   struct Volume
   {
      S32 edgeList;
      S32 material;
      SimObject* object;
   };
   struct StackElement
   {
      S32 edgeList;
      const BSPNode *node;
   };
   struct Collision {
      SimObject* object;
      S32 material;
      PlaneF plane;
      Point3F point;
      F32 distance;

      Collision()
      {
         object = NULL;
         distance = 0.0;
      }
   };

   typedef Vector<Edge> EdgeList;
   typedef Vector<Face> FaceList;
   typedef Vector<Vertex> VertexList;
   typedef Vector<Volume> VolumeList;
   typedef Vector<StackElement> VolumeStack;

   //
   S32 sideCount;
   EdgeList mEdgeList;
   FaceList mFaceList;
   VertexList mVertexList;
   VolumeList mVolumeList;

private:
   bool intersect(const PlaneF& plane,const Point3F& sp,const Point3F& ep);

public:
   //
   Polytope();
   void buildBox(const MatrixF& transform,const Box3F& box);
   void intersect(SimObject*, const BSPNode* node);
   inline bool didIntersect()  { return mVolumeList.size() > 1; }
   void extrudeFace(int fi,const VectorF& vec,Polytope* out);
   bool findCollision(const VectorF& vec,Polytope::Collision *best);
   void render();
};




#endif
