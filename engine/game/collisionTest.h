//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _COLLISIONTEST_H_
#define _COLLISIONTEST_H_

#ifndef _SCENEOBJECT_H_
#include "sim/sceneObject.h"
#endif

#ifndef _POLYTOPE_H_
#include "collision/polytope.h"
#endif
#ifndef _CLIPPEDPOLYLIST_H_
#include "collision/clippedPolyList.h"
#endif
#ifndef _EXTRUDEDPOLYLIST_H_
#include "collision/extrudedPolyList.h"
#endif
#ifndef _DEPTHSORTLIST_H_
#include "collision/depthSortList.h"
#endif
#ifndef _POLYHEDRON_H_
#include "collision/polyhedron.h"
#endif

/// Helper class for collision detection.
struct CollisionTest
{
   /// @name Basic Settings
   /// @{
   Box3F boundingBox;
   SphereF boundingSphere;
   static bool renderAlways;

   Point3F testPos;
   /// @}

   /// @name Depth Sort List
   /// Use a slightly different box and sphere for depthSortList.
   /// @{
   Box3F mDepthBox;
   SphereF mDepthSphere;
   Point3F mDepthSortExtent;
   /// @}

   /// @name Polytope/BSP test
   /// @{
   static bool testPolytope;
   BSPTree tree;
   Polytope volume;
   /// @}

   /// @name Clipped polylists
   /// @{
   static bool testClippedPolyList;
   ClippedPolyList polyList;
   /// @}

   /// @name Depth sorted polylists
   /// @{
   static bool testDepthSortList;
   static bool depthSort;
   static bool depthRender;
   DepthSortList depthSortList;
   /// @}

   /// @name Extruded
   /// @{
   CollisionList collisionList;
   static bool testExtrudedPolyList;
   Polyhedron polyhedron;
   VectorF extrudeVector;
   ExtrudedPolyList extrudedList;
   /// @}

   /// @name Implementation
   /// @{
   CollisionTest();
   ~CollisionTest();
   static void consoleInit();
   static void callback(SceneObject*, void *thisPtr);
   void collide(const MatrixF& transform);
   void render();
   /// @}
};


#endif
