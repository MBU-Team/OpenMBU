//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _POLYHEDRON_H_
#define _POLYHEDRON_H_

#ifndef _MMATH_H_
#include "math/mMath.h"
#endif
#ifndef _TVECTOR_H_
#include "core/tVector.h"
#endif

//----------------------------------------------------------------------------

struct Polyhedron
{
   struct Edge
   {
      // Edge vertices must be oriented clockwise
      // for face[0]
      U32 face[2];
      U32 vertex[2];
   };

   Vector<Point3F> pointList;
   Vector<PlaneF> planeList;
   Vector<Edge> edgeList;

   // Misc support methods
   Polyhedron() {
      VECTOR_SET_ASSOCIATION(pointList);
      VECTOR_SET_ASSOCIATION(planeList);
      VECTOR_SET_ASSOCIATION(edgeList);
   }

   void buildBox(const MatrixF& mat, const Box3F& box);
   void render();
   void render(const VectorF& vec,F32 time);
};


#endif
