#pragma warning ( disable: 4786 )
#include <vector>
#include <map>
#include <cmath>

#include "DTSMilkshapeMesh.h"

namespace DTS
{

   // -----------------------------------------------------------------------
   //  Imports a Milkshape Mesh
   // -----------------------------------------------------------------------

   MilkshapeMesh::MilkshapeMesh (msMesh * mesh, int rootBone, float scaleFactor, bool WithMaterials) : Mesh (Mesh::T_Standard)
   {
      int v, n ;
      int materialIndex = msMesh_GetMaterialIndex(mesh) ;

      // Import vertex position and texture data

      for (v = 0 ; v < msMesh_GetVertexCount(mesh) ; v++)
      {
         msVertex * vertex = msMesh_GetVertexAt(mesh, v) ;
         msVec3     vector ;
         
         msVertex_GetVertex (vertex, vector) ;

         vector[0] *= scaleFactor ;
         vector[1] *= scaleFactor ;
         vector[2] *= scaleFactor ;
         
         // Vertex weights, MS only supports one bone per vertex.
         // All vertices in a skin mesh must be assigned to a bone,
         // so if MS doesn't have one we'll assign it to the root.
         vindex.push_back(verts.size());
         int bone = msVertex_GetBoneIndex (vertex);
         vbone.push_back(getVertexBone((bone >= 0)? bone: rootBone));
         vweight.push_back(1.0);

         // The MilkshapePoint class takes care of axis conversion
         verts.push_back (MilkshapePoint(vector)) ;

         // Texture coordinates
         msVertex_GetTexCoords (vertex, vector) ;
         tverts.push_back (Point2D(vector[0], vector[1])) ;
      }

      // Adjust our type. We default to a rigid mesh, but if the mesh
      // is attached to more than one bone, we need to output as a skin
      // mesh. Node index is managed by the getVertexBone method.
      if (nodeIndex.size() > 1)
         setType(T_Skin);
      
      // Import normals (into a temporary vector)
      
      std::vector<Point> msNormals ;

      for (v = 0 ; v < msMesh_GetVertexNormalCount(mesh) ; v++)
      {
         msVec3 vector ;         
         msMesh_GetVertexNormalAt (mesh, v, vector) ;
         msNormals.push_back (MilkshapePoint(vector)) ;
      }

      // Import primitives (as single triangles for now)
      // While we're at it, use the triangle data to get per-vertex normals
      
      std::vector<bool> vertexNormalFound (verts.size(), false) ;    
      normals.assign  (verts.size(), Point()) ;
      enormals.assign (verts.size(), '\0') ;

      for (v = 0 ; v < msMesh_GetTriangleCount(mesh) ; v++)
      {
         msTriangle * triangle = msMesh_GetTriangleAt(mesh, v) ;
         word         ivertexes[3] ;
         word         inormals[3] ;
         Primitive    myTriangle ;
         
         msTriangle_GetVertexIndices (triangle, ivertexes) ;
         msTriangle_GetNormalIndices (triangle, inormals) ;

         // Transform milkshape per-triangle normals to V12 per-vertex normals
         // There could be many normals per vertex, interpolate them

         for (n = 0 ; n < 3 ; n++)
         {
            if (!vertexNormalFound[ivertexes[n]])
            {
               vertexNormalFound[ivertexes[n]] = true ;
               normals[ivertexes[n]]  = msNormals[inormals[n]] ;
               enormals[ivertexes[n]] = encodeNormal(msNormals[inormals[n]]) ;
            }
            else
            {
               Point midpoint = (normals[ivertexes[n]] + msNormals[inormals[n]]) / 2.0f ;
               midpoint.normalize() ;
               normals[ivertexes[n]]  = midpoint ;
               enormals[ivertexes[n]] = encodeNormal(midpoint) ;
            }
         }

         // Create a triangle primitive for this triangle

         myTriangle.firstElement = indices.size() ;
         myTriangle.numElements  = 3 ;
         myTriangle.type         = Primitive::Strip | Primitive::Indexed ;

         if (WithMaterials && materialIndex >= 0)
            myTriangle.type |= materialIndex ;
         else
            myTriangle.type |= Primitive::NoMaterial ;

         indices.push_back  (ivertexes[2]) ;
         indices.push_back  (ivertexes[1]) ;
         indices.push_back  (ivertexes[0]) ;
         
         primitives.push_back (myTriangle) ;
      }
      
      // Other stuff we don't support
      
      setFrames(1) ;
      setParent(-1) ;

      calculateBounds() ;
      calculateCenter() ;
      calculateRadius() ;
   }
}