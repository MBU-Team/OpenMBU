//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _TSSORTEDMESH_H_
#define _TSSORTEDMESH_H_

#ifndef _TSMESH_H_
#include "ts/tsMesh.h"
#endif

/// TSSortedMesh is for meshes that need sorting (obviously).  Such meshes
/// are usually partially or completely composed of translucent/parent polygons.
class TSSortedMesh : public TSMesh
{
public:
   typedef TSMesh Parent;

   /// This is a group of primitives that belong "together" in the rendering sequence.
   /// For example, if a player model had a helmet with a translucent visor, the visor
   /// would be a Cluster.
   struct Cluster
   {
      S32 startPrimitive;
      S32 endPrimitive;
      Point3F normal;
      F32 k;
      S32 frontCluster; ///< go to this cluster if in front of plane, if frontCluster<0, no cluster
      S32 backCluster;  ///< go to this cluster if in back of plane, if backCluster<0, no cluster
                        ///< if frontCluster==backCluster, no plane to test against...
   };

   ToolVector<Cluster> clusters; ///< All of the clusters of primitives to be drawn
   ToolVector<S32> startCluster; ///< indexed by frame number
   ToolVector<S32> firstVerts;   ///< indexed by frame number
   ToolVector<S32> numVerts;     ///< indexed by frame number
   ToolVector<S32> firstTVerts;  ///< indexed by frame number or matFrame number, depending on which one animates (never both)

   /// sometimes, we want to write the depth value to the frame buffer even when object is translucent
   bool alwaysWriteDepth;

   // render methods..
   void render(S32 frame, S32 matFrame, TSMaterialList *);
   void renderFog(S32 frame);

   bool buildPolyList(S32 frame, AbstractPolyList * polyList, U32 & surfaceKey);
   bool castRay(S32 frame, const Point3F & start, const Point3F & end, RayInfo * rayInfo);
   bool buildConvexHull(); ///< does nothing, skins don't use this
                           ///
                           ///  @returns false ALWAYS
   S32 getNumPolys();

   void assemble(bool skip);
   void disassemble();

   TSSortedMesh() {
      meshType = SortedMeshType;
   }
};

#endif // _TS_SORTED_MESH


