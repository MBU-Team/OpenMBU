//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _SPHERE_H_
#define _SPHERE_H_

#ifndef _MMATH_H_
#include "math/mMath.h"
#endif
#ifndef _TVECTOR_H_
#include "core/tVector.h"
#endif

//------------------------------------------------------------------------------
// Class: Sphere
//------------------------------------------------------------------------------
// * ctor takes type of base polyhedron that is subdivided to create sphere
// * getMesh(...) will subdivide the current mesh to the desired level where
//   (each level has 4 times the polys of the previous level)

class Sphere
{
   public:

      // regular polyhedra with triangle face polygons (num of faces)
      enum {
         Tetrahedron = 4,
         Octahedron = 8,
         Icosahedron = 20,

         MaxLevel       = 5
      };

      struct Triangle {
         Triangle() {}
         Triangle(Point3F a, Point3F b, Point3F c) {pnt[0] = a; pnt[1] = b; pnt[2] = c;}
         Point3F     pnt[3];
         Point3F     normal;
      };

      struct TriangleMesh {
         U32            numPoly;
         Triangle *     poly;
      };

      Sphere(U32 baseType = Octahedron);
      ~Sphere();

      const TriangleMesh * getMesh(U32 level = 0);

   private:

      TriangleMesh * createTetrahedron();
      TriangleMesh * createOctahedron();
      TriangleMesh * createIcosahedron();

      Vector<TriangleMesh*>      mDetails;

      void calcNormals(TriangleMesh *);
      TriangleMesh * subdivideMesh(TriangleMesh*);
};

#endif
