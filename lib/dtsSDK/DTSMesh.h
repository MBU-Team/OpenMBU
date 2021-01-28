#ifndef __DTSMESH_H
#define __DTSMESH_H

#include "DTSPoint.h"
#include "DTSMatrix.h"

namespace DTS
{
   // Forward reference;
   class Quaternion;

   //! Defines a primitive as stored in the DTS file

   struct Primitive
   {
      short firstElement ;       //!< Number of consecutive indices used
      short numElements ;        //!< Number of the first index
      int   type ;               //!< Type of primitive and number of material used

      enum /* type */
      {
         Triangles    = 0x00000000,
         Strip        = 0x40000000, 
         Fan          = 0x80000000,       //!< WARNING! May not be supported 
         TypeMask     = 0xC0000000,

         Indexed      = 0x20000000,       //!< WARNING! Non-indexed primitives not supported in the engine
         NoMaterial   = 0x10000000,
         MaterialMask = 0x0FFFFFFF,
      };
   };

   //! Defines a Mesh as stored in the DTS file
   //  Skin render...
   //    NodeIndex loop
   //       Vb[n] = nodeTransform[nodeIndex[n]] * InitialTransform[n]
   //    Vertex loop
   //       v[vindex[n]] += initialVerts[n] * Vb[boneIndex[n]] * weight[n]

   class Mesh
   {
   public:

      //! Mesh types
      enum Type
      {
         T_Standard  = 0,     //!< Rigid mesh
         T_Skin      = 1,     //!< TODO: Skined (mesh with bones) 
         T_Decal     = 2,     //!< DEPRECATED
         T_Sorted    = 3,     //!< BSP Sorted for correct alpha rendering
         T_Null      = 4      //!< Null mesh, used in collision objects as the visible meshes
      };

      //!< Mesh flags
      enum Flag
      {
         Billboard      = 0x80000000,     //!< Mesh always faces the camera
         HasDetail      = 0x40000000,     //!< ?
         BillboardZ     = 0x20000000,     //!< If billboard, only rotate around Z axis
         EncodedNormals = 0x10000000,     //!< Mesh uses encoded normals
      } ;


      //! Create an empty mesh of the given type
      Mesh (Type t = T_Null) ;

      //! Change object type
      Type getType()           { return (Type)type; }
      void setType(Type t)     { type = t; }

      //! Set mesh flags
      void setFlag(int f)     { flags |= f; }

      //! Save the mesh to a special DTS stream
      void save (class OutputStream &) const ;

      //! Read the mesh from a special DTS stream
      void read (class InputStream &stream) ;

      //! Calculates and returns the number of polygons
      int   getPolyCount() const ;

      //! Get the radius of the bounding sphere
      float getRadius() const  { return radius ; }

      //! Get the center point of mesh (and mesh's bounding sphere)
      Point getCenter() const  { return center ; }

      //! Get the bounding box of the mesh
      Box   getBounds() const  { return bounds ; }

      //! Get the transformed bounding box of the mesh 
      Box   getBounds(Point, Quaternion) const;

      //! Calculate the radius of a bounding sphere, if it is centered in the given point
      float getRadiusFrom(Point, Quaternion, Point center) const ;

      //! Get the tube radius (radius of bounding cylinder)
      float getTubeRadius() const ;

      //! Calculate the radius of a bounding cylinder, if it is centered in the given point
      float getTubeRadiusFrom(Point, Quaternion, Point center) const ;

      //! Joins another mesh with this one (imports all vertexes and triangles)
      Mesh & operator += (const Mesh &) ;

      //! Change the material of the mesh
      void setMaterial(int n) ;

      //! Move all the vertexes in a direction
      void translate(const Point n) ;

      //! Rotates all vertexes
      void rotate (const class Quaternion &q) ;

      //! Return a vertex bone number for the given node
      int getVertexBone(int node);

      //! Return the node for a given vertex bone
      int getNodeIndexCount() { return nodeIndex.size(); }
      int getNodeIndex(int node) {
         return (node >= 0 && node < nodeIndex.size())? nodeIndex[node]: -1;
      }
      void setNodeTransform(int node,Point t, Quaternion q);

   protected:

      void setCenter(Point c) { center = c; }
      void setBounds(Box b)   { bounds = b; }
      void setRadius(float r) { radius = r; }
      void setFrames(int n)   { numFrames = n ; vertsPerFrame = verts.size()/n ; }
      void setParent(int n)   { parent = n; }

   protected:

      void calculateBounds() ;
      void calculateCenter() ;
      void calculateRadius() ;

      static char encodeNormal(const Point &) ;

      std::vector <Point>     verts ;     //!< Contains all mesh vertexes
      std::vector <Point2D>   tverts ;    //!< Texture coordinates s, t for each vertex
      std::vector <Point>     normals ;   //!< Normals for each vertex
      std::vector <char>      enormals ;  //!< Encoded normals (see encodeNormal)
      std::vector <Primitive> primitives; //!< Mesh primitives
      std::vector <short>     indices ;   //!< Vertex index array, for primitives
      std::vector <short>     mindices ;  //!< ?

      // Extended data used by Skin Meshes

      std::vector <int>       vindex;     //!< Vertex index
      std::vector <int>       vbone;      //!< Vertex vbone
      std::vector <float>     vweight;    //!< Vertex weight
      std::vector <int>       nodeIndex;  //!< vbone -> node
      std::vector <Matrix<4,4> > nodeTransform;  //!< 

   private:

      int   type ;
      int   numFrames ;
      int   matFrames ;
      int   parent ;
      Box   bounds ;
      Point center ;
      float radius ;

      int vertsPerFrame ;
      int flags ;

   };

}

#endif