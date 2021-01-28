//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "game/sphere.h"

Sphere::Sphere(U32 baseType)
{
   VECTOR_SET_ASSOCIATION(mDetails);

   switch(baseType)
   {
      case Tetrahedron:
         mDetails.push_back(createTetrahedron());
         break;

      case Octahedron:
         mDetails.push_back(createOctahedron());
         break;

      case Icosahedron:
         mDetails.push_back(createIcosahedron());
         break;
   }
   calcNormals(mDetails[0]);
}

//------------------------------------------------------------------------------

Sphere::TriangleMesh * Sphere::createTetrahedron()
{
   const F32 sqrt3 = 0.5773502692;

   static Point3F spherePnts[] = {
      Point3F( sqrt3, sqrt3, sqrt3 ),
      Point3F(-sqrt3,-sqrt3, sqrt3 ),
      Point3F(-sqrt3, sqrt3,-sqrt3 ),
      Point3F( sqrt3,-sqrt3,-sqrt3 )
   };

   static Triangle tetrahedron[] = {
      Triangle(spherePnts[0], spherePnts[1], spherePnts[2]),
      Triangle(spherePnts[0], spherePnts[3], spherePnts[1]),
      Triangle(spherePnts[2], spherePnts[1], spherePnts[3]),
      Triangle(spherePnts[3], spherePnts[0], spherePnts[2]),
   };

   static TriangleMesh tetrahedronMesh = {
      Tetrahedron,
      &tetrahedron[0]
   };

   return(&tetrahedronMesh);
}

//------------------------------------------------------------------------------

Sphere::TriangleMesh * Sphere::createOctahedron()
{
   //
   static Point3F spherePnts[] = {
      Point3F( 1, 0, 0),
      Point3F(-1, 0, 0),
      Point3F( 0, 1, 0),
      Point3F( 0,-1, 0),
      Point3F( 0, 0, 1),
      Point3F( 0, 0,-1)
   };

   //
   static Triangle octahedron[] = {
      Triangle(spherePnts[0], spherePnts[4], spherePnts[2]),
      Triangle(spherePnts[2], spherePnts[4], spherePnts[1]),
      Triangle(spherePnts[1], spherePnts[4], spherePnts[3]),
      Triangle(spherePnts[3], spherePnts[4], spherePnts[0]),
      Triangle(spherePnts[0], spherePnts[2], spherePnts[5]),
      Triangle(spherePnts[2], spherePnts[1], spherePnts[5]),
      Triangle(spherePnts[1], spherePnts[3], spherePnts[5]),
      Triangle(spherePnts[3], spherePnts[0], spherePnts[5])
   };

   //
   static TriangleMesh octahedronMesh = {
      Octahedron,
      &octahedron[0]
   };

   return(&octahedronMesh);
}

Sphere::TriangleMesh * Sphere::createIcosahedron()
{
   const F32 tau = 0.8506508084;
   const F32 one = 0.5257311121;

   static Point3F spherePnts[] = {
      Point3F( tau, one,   0),
      Point3F(-tau, one,   0),
      Point3F(-tau,-one,   0),
      Point3F( tau,-one,   0),
      Point3F( one,   0, tau),
      Point3F( one,   0,-tau),
      Point3F(-one,   0,-tau),
      Point3F(-one,   0, tau),
      Point3F(   0, tau, one),
      Point3F(   0,-tau, one),
      Point3F(   0,-tau,-one),
      Point3F(   0, tau,-one),
   };

   static Triangle icosahedron[] = {
      Triangle(spherePnts[4],  spherePnts[8],  spherePnts[7]),
      Triangle(spherePnts[4],  spherePnts[7],  spherePnts[9]),
      Triangle(spherePnts[5],  spherePnts[6],  spherePnts[11]),
      Triangle(spherePnts[5],  spherePnts[10], spherePnts[6]),
      Triangle(spherePnts[0],  spherePnts[4],  spherePnts[3]),
      Triangle(spherePnts[0],  spherePnts[3],  spherePnts[5]),
      Triangle(spherePnts[2],  spherePnts[7],  spherePnts[1]),
      Triangle(spherePnts[2],  spherePnts[1],  spherePnts[6]),
      Triangle(spherePnts[8],  spherePnts[0],  spherePnts[11]),
      Triangle(spherePnts[8],  spherePnts[11], spherePnts[1]),
      Triangle(spherePnts[9],  spherePnts[10], spherePnts[3]),
      Triangle(spherePnts[9],  spherePnts[2],  spherePnts[10]),
      Triangle(spherePnts[8],  spherePnts[4],  spherePnts[0]),
      Triangle(spherePnts[11], spherePnts[0],  spherePnts[5]),
      Triangle(spherePnts[4],  spherePnts[9],  spherePnts[3]),
      Triangle(spherePnts[5],  spherePnts[3],  spherePnts[10]),
      Triangle(spherePnts[7],  spherePnts[8],  spherePnts[1]),
      Triangle(spherePnts[6],  spherePnts[1],  spherePnts[11]),
      Triangle(spherePnts[7],  spherePnts[2],  spherePnts[9]),
      Triangle(spherePnts[6],  spherePnts[10], spherePnts[2]),
   };

   static TriangleMesh icosahedronMesh = {
      Icosahedron,
      &icosahedron[0]
   };

   return(&icosahedronMesh);
}

//------------------------------------------------------------------------------

void Sphere::calcNormals(TriangleMesh * mesh)
{
   for(U32 i = 0; i < mesh->numPoly; i++)
   {
      Triangle & tri = mesh->poly[i];
      mCross(tri.pnt[1] - tri.pnt[0], tri.pnt[2] - tri.pnt[0], &tri.normal);
   }
}

//------------------------------------------------------------------------------

Sphere::~Sphere()
{
   // level 0 is static data
   for(U32 i = 1; i < mDetails.size(); i++)
   {
      delete [] mDetails[i]->poly;
      delete mDetails[i];
   }
}

//------------------------------------------------------------------------------

const Sphere::TriangleMesh * Sphere::getMesh(U32 level)
{
   AssertFatal(mDetails.size(), "Sphere::getMesh: no details!");

   if(level > MaxLevel)
      level = MaxLevel;

   //
   while(mDetails.size() <= level)
      mDetails.push_back(subdivideMesh(mDetails.last()));

   return(mDetails[level]);
}

Sphere::TriangleMesh * Sphere::subdivideMesh(TriangleMesh * prevMesh)
{
   AssertFatal(prevMesh, "Sphere::subdivideMesh: invalid previous mesh level!");

   //
   TriangleMesh * mesh = new TriangleMesh;

   mesh->numPoly = prevMesh->numPoly * 4;
   mesh->poly = new Triangle [mesh->numPoly];

   //
   for(U32 i = 0; i < prevMesh->numPoly; i++)
   {
      Triangle * pt = &prevMesh->poly[i];
      Triangle * nt = &mesh->poly[i*4];

      Point3F a = (pt->pnt[0] + pt->pnt[2]) / 2;
      Point3F b = (pt->pnt[0] + pt->pnt[1]) / 2;
      Point3F c = (pt->pnt[1] + pt->pnt[2]) / 2;

      // force the point onto the unit sphere surface
      a.normalize();
      b.normalize();
      c.normalize();

      //
      nt->pnt[0] = pt->pnt[0];
      nt->pnt[1] = b;
      nt->pnt[2] = a;
      nt++;

      //
      nt->pnt[0] = b;
      nt->pnt[1] = pt->pnt[1];
      nt->pnt[2] = c;
      nt++;

      //
      nt->pnt[0] = a;
      nt->pnt[1] = b;
      nt->pnt[2] = c;
      nt++;

      //
      nt->pnt[0] = a;
      nt->pnt[1] = c;
      nt->pnt[2] = pt->pnt[2];
   }

   calcNormals(mesh);
   return(mesh);
}
