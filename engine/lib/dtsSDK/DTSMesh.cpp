
#pragma warning ( disable: 4786 )
#include <vector>
#include <cmath>

#include "DTSMesh.h"
#include "DTSOutputStream.h"
#include "DTSInputStream.h"

namespace DTS
{

   // -----------------------------------------------------------------------
   //  Normal encoding transformation
   // -----------------------------------------------------------------------

#include "DTSNormalTable.cpp"

   char Mesh::encodeNormal(const Point & normal)
   {
      unsigned char bestIndex = 0;
      float         bestDot   = -10E30F ;

      for (int i = 0 ; i < 256 ; i++)
      {
         float dot = 
            ( normal.x() * normalTable[i][0]
            + normal.y() * normalTable[i][1]
            + normal.z() * normalTable[i][2] ) ;

         if (dot > bestDot)
         {
            bestIndex = i;
            bestDot = dot;
         }
      }

      return * (char*)&bestIndex;
   }

   // -----------------------------------------------------------------------
   //  Searchs for minor and maximum vertex coordinates
   // -----------------------------------------------------------------------

   void Mesh::calculateBounds()
   {
      std::vector<Point>::iterator vertex ;

      bounds.max = Point(-10E30F, -10E30F, -10E30F) ;
      bounds.min = Point( 10E30F,  10E30F,  10E30F) ;
      for (vertex = verts.begin() ; vertex != verts.end() ; vertex++)
      {
         if (vertex->x() < bounds.min.x())
            bounds.min.x(vertex->x()) ;
         if (vertex->y() < bounds.min.y())
            bounds.min.y(vertex->y()) ;
         if (vertex->z() < bounds.min.z())
            bounds.min.z(vertex->z()) ;
         if (vertex->x() > bounds.max.x())
            bounds.max.x(vertex->x()) ;
         if (vertex->y() > bounds.max.y())
            bounds.max.y(vertex->y()) ;
         if (vertex->z() > bounds.max.z())
            bounds.max.z(vertex->z()) ;
      }
   }

   Box Mesh::getBounds(Point trans,Quaternion rot) const
   {
      // Compute the bounding box using the given transform.
      Box bounds2;
      bounds2.max = Point(-10E30F, -10E30F, -10E30F) ;
      bounds2.min = Point( 10E30F,  10E30F,  10E30F) ;

      std::vector<Point>::const_iterator vertex ;
      for (vertex = verts.begin() ; vertex != verts.end() ; vertex++)
      {
         Point tv = rot.apply(*vertex) + trans;

         if (tv.x() < bounds2.min.x())
            bounds2.min.x(tv.x()) ;
         if (tv.y() < bounds2.min.y())
            bounds2.min.y(tv.y()) ;
         if (tv.z() < bounds2.min.z())
            bounds2.min.z(tv.z()) ;
         if (tv.x() > bounds2.max.x())
            bounds2.max.x(tv.x()) ;
         if (tv.y() > bounds2.max.y())
            bounds2.max.y(tv.y()) ;
         if (tv.z() > bounds2.max.z())
            bounds2.max.z(tv.z()) ;
      }
      return bounds2;
   }


   // -----------------------------------------------------------------------
   //  Calculates center of bounding box
   // -----------------------------------------------------------------------

   void Mesh::calculateCenter()
   {
      center = bounds.max.midpoint(bounds.min) ;
   }

   // -----------------------------------------------------------------------
   //  Search for the maximum vertex distance from center 
   // -----------------------------------------------------------------------

   void Mesh::calculateRadius()
   {
       radius = getRadiusFrom(Point(0,0,0),Quaternion::identity,center) ;
   }

   float Mesh::getRadiusFrom(Point trans, Quaternion rot, Point center) const
   {
      std::vector<Point>::const_iterator vertex ;

      float radius = 0.0F ;
      for (vertex = verts.begin() ; vertex != verts.end() ; vertex++)
      {
         Point tv = rot.apply(*vertex) + trans;
         float distance = (tv - center).length() ;
         if (distance > radius)
            radius = distance ;
      }
      return radius ;
   }

   float Mesh::getTubeRadius() const
   {
      std::vector<Point>::const_iterator vertex ;

      float radius = 0.0F ;
      for (vertex = verts.begin() ; vertex != verts.end() ; vertex++)
      {
         float distance = (*vertex - center).length(2) ;
         if (distance > radius)
            radius = distance ;
      }
      return radius ;
   }

   float Mesh::getTubeRadiusFrom(Point trans, Quaternion rot, Point center) const
   {
      std::vector<Point>::const_iterator vertex ;

      float radius = 0.0F ;
      for (vertex = verts.begin() ; vertex != verts.end() ; vertex++)
      {
         Point tv = rot.apply(*vertex) + trans;
         float distance = (tv - center).length(2) ;
         if (distance > radius)
            radius = distance ;
      }
      return radius ;
   }

   // -----------------------------------------------------------------------
   //  Calculate the polygon count
   // -----------------------------------------------------------------------

   int Mesh::getPolyCount() const
   {
      int count = 0 ;

      std::vector<Primitive>::const_iterator p ;
      for (p = primitives.begin() ; p != primitives.end() ; p++)
      {
         if (p->type & Primitive::Strip)
            count += p->numElements - 2 ;
         else
            count += p->numElements / 3 ;
      }
      return count ;
   }

   // -----------------------------------------------------------------------
   //  Creates an empty mesh
   // -----------------------------------------------------------------------

   Mesh::Mesh(Type t)
   {
      bounds.min = Point(0,0,0) ;
      bounds.max = Point(0,0,0) ;
      center = Point(0,0,0) ;
      radius = 0.0f ;
      numFrames = 1 ;
      matFrames = 1 ;
      vertsPerFrame = 0 ;
      parent = -1 ;
      flags = 0 ;
      type = t ;
   }

   // -----------------------------------------------------------------------
   //  Joins another mesh with this one
   // -----------------------------------------------------------------------

   Mesh & Mesh::operator += (const Mesh & m) 
   {
      int   firstIndex  = indices.size();
      int   firstVertex = verts.size() ;
      int   firstPrimit = primitives.size() ;
      int   i ;

      verts.insert (verts.end(), m.verts.begin(), m.verts.end()) ;
      tverts.insert (tverts.end(), m.tverts.begin(), m.tverts.end()) ;
      indices.insert (indices.end(), m.indices.begin(), m.indices.end()) ;
      normals.insert (normals.end(), m.normals.begin(), m.normals.end()) ;
      enormals.insert (enormals.end(), m.enormals.begin(), m.enormals.end()) ;
      primitives.insert (primitives.end(), m.primitives.begin(), m.primitives.end()) ;

      for (i = firstIndex ; i < indices.size() ; i++)
         indices[i] += firstVertex ;
      for (i = firstPrimit ; i < primitives.size() ; i++)
         primitives[i].firstElement += firstIndex ;

      calculateBounds() ;
      calculateCenter() ;
      calculateRadius() ;

      vertsPerFrame = verts.size() ;

      return *this ;
   }

   
   // -----------------------------------------------------------------------
   //  Input/output
   // -----------------------------------------------------------------------

   void Mesh::read (InputStream &stream)
   {
      stream >> type ;
      if (type == T_Null) return ;

      stream.readCheck() ;

      // Header & Bounds

      stream >> numFrames >> matFrames >> parent ;
      stream >> bounds    >> center ;
      
      int radiusInt ;
      stream >> radiusInt ;
      radius = (float)radiusInt ;

      // Vertexes

      int numVertexes ;
      stream >> numVertexes ;
      verts.resize (numVertexes) ;
      stream >> verts ;

      // Texture coordinates

      int numTVerts ;
      stream >> numTVerts ;
      tverts.resize (numTVerts) ;
      stream >> tverts ;

      // Normals 

      normals.resize (numVertexes) ;
      enormals.resize (numVertexes) ;
      stream >> normals ;
      stream >> enormals ;

      // Primitives and other stuff

      int numPrimitives ;
      stream >> numPrimitives ;
      primitives.resize (numPrimitives) ;
      stream >> primitives ;

      int numIndices ;
      stream >> numIndices ;
      indices.resize (numIndices) ;
      stream >> indices ;

      int numMIndices ;
      stream >> numMIndices ;
      mindices.resize (numMIndices) ;
      stream >> mindices ;

      stream >> vertsPerFrame >> flags ;
      stream.readCheck() ;
   }

   void Mesh::save (OutputStream &stream) const
   {
      // Mesh Type

      stream << type ;
      if (type == T_Null) return ;     // Null mesh has no data

      stream.storeCheck() ;

      // Header

      stream << numFrames  << matFrames << parent ;      // Header
      stream << bounds     << center    << (int)radius ; // Bounds

      // Vertexes 

      stream << (int) verts.size() ;
      stream << verts ;

      // Texture coordinates

      assert (tverts.size() == verts.size()) ;

      stream << (int) tverts.size() ;
      stream << tverts ;

      // Normals

      assert (normals.size()  == verts.size()) ;
      assert (enormals.size() == verts.size()) ;

      stream << normals ;
      stream << enormals ;

      // Primitives

      stream << (int) primitives.size() ;
      stream << primitives ;
      stream << (int) indices.size() ;
      stream << indices ;
      stream << (int) mindices.size() ;
      stream << mindices ;

      // Other small stuff

      stream << vertsPerFrame << flags ;

      stream.storeCheck() ;

      // Skin Mesh support
      if (type == T_Skin) {

         // Initial skin vertices & normals of affected vertices.
         // Yes, we did just write these out, but this is what it wants.
         stream << (int) verts.size() ;
         stream << verts;
         stream << normals;
         stream << enormals;

         // Inverse world transforms for bones used in vertex weighting
         assert(nodeTransform.size() == nodeIndex.size());

         stream << (int) nodeIndex.size();
         for (int n = 0; n < nodeIndex.size(); n++)
            stream << nodeTransform[n];

         // Vertex tuples.  If this more than one entry per vertex, then
         // these tables should be sorted by vindex.
         assert (vweight.size()  == vindex.size());
         assert (vbone.size() == vindex.size());

         stream << (int) vindex.size();
         stream << vindex;
         stream << vbone;
         stream << vweight;

         // Vertex bone to node table
         stream << (int) nodeIndex.size();
         stream << nodeIndex;

         stream.storeCheck();
      }
   }

   // -----------------------------------------------------------------------
   //  Utility stuff
   // -----------------------------------------------------------------------

   void Mesh::setMaterial (int n)
   {
      std::vector<Primitive>::iterator p ;
      for (p = primitives.begin() ; p != primitives.end() ; p++)
      {
         p->type = (p->type & ~Primitive::MaterialMask) | 
                   (n       &  Primitive::MaterialMask) ;

         // Just in case
         p->type &= ~Primitive::NoMaterial ;
      }
   }

   void Mesh::translate(const Point n)
   {
      std::vector<Point>::iterator v ;
      for (v = verts.begin() ; v != verts.end() ; v++)
         *v += n ;

      calculateBounds() ;
      calculateCenter() ;
      calculateRadius() ;
   }

   void Mesh::rotate (const Quaternion &q)
   {
      std::vector<Point>::iterator v ;
      for (v = verts.begin() ; v != verts.end() ; v++)
         *v = q.apply(*v);

      calculateBounds() ;
      calculateCenter() ;
      calculateRadius() ;
   }

   int Mesh::getVertexBone(int node)
   {
      // Finds the bone index in the table, or adds it if it's
      // not there.  The vertex bone & nodeIndex list are here to
      // track which bones are used by this mesh.
      int b = 0;
      for (; b < nodeIndex.size(); b++)
         if (nodeIndex[b] == node)
            return b;
      nodeIndex.push_back(node);
      nodeTransform.push_back(Matrix<4,4>::identity());
      return b;
   }

   void Mesh::setNodeTransform(int node,Point t, Quaternion q)
   {
      // Build inverse transform, the mesh wants to be able to
      // transform the vertices into node space.
      assert(node < nodeTransform.size());
      t = q.inverse().apply(-t);
      Matrix<4,4>::Row row;
      row[0] = t.x(); row[1] = t.y(); row[2] = t.z(); row[3] = 1;

      // The toMatrix builds a transposed transform from what we
      // want, so we need to pass the original quaternion to get
      // the inverse.
      nodeTransform[node] = q.toMatrix();
      nodeTransform[node].setCol(3,row);
   }

};
