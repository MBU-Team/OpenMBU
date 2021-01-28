
#pragma warning ( disable: 4786 )
#include <vector>
#include <cmath>

#include "DTSBrushMesh.h"

#ifndef M_PI
#define M_PI 3.1415926535f
#endif

namespace DTS
{

   void BrushMesh::addVertex(float x, float y, float z)
   {
      verts.push_back(Point(x, y, z));
      tverts.push_back(Point2D(x+z, y+z));
      Point normal = Point(x, y, z) - getCenter() ;
      normal.normalize() ;
      normals.push_back(normal) ;
      enormals.push_back(encodeNormal(normal)) ;
   }

   // -----------------------------------------------------------------------
   //  Creates a vertical cylinder
   // -----------------------------------------------------------------------

   CylinderMesh::CylinderMesh (Box &_bounds, float _radius, float _complexity)
   {
      setRadius (_radius) ;
      setBounds (_bounds) ;
      setCenter (_bounds.min.midpoint(_bounds.max)) ;

      complexity = _complexity ;

      construct() ;
   }

   void CylinderMesh::construct()
   {
      int i, firstTop ;
      int steps = (unsigned int)(complexity * 64.0f) ;
      float angle ;
      Point normal ;
      Box   bounds = getBounds() ;
      Point center = getCenter() ;
      float radius = getRadius() ;

      Primitive p ;
      p.type = Primitive::NoMaterial | Primitive::Strip | Primitive::Indexed ;

      if (steps < 4)  steps = 4 ;
      if (steps > 64) steps = 64 ;
      steps &= ~1 ;

      // Bottom
      addVertex (center.x(), center.y(), bounds.min.z()) ;
      for (angle = 0.0f, i = 0 ; i < steps ; i++, angle += 2.0f*M_PI/(float)steps)
      {
         addVertex (cosf(angle)*radius + center.x(), sinf(angle)*radius + center.y(), 
                    bounds.min.z()) ;
      }

      for (i = 1 ; i <= steps ; i++)
      {
         p.firstElement = indices.size() ;
         p.numElements  = 3 ;
         indices.push_back (0) ;
         indices.push_back (i) ;
         indices.push_back (i == steps ? 1 : i+1) ;
         primitives.push_back(p) ;
      }

      // Top
      firstTop = verts.size() ;
      addVertex (center.x(), center.y(), bounds.max.z()) ;
      for (angle = 0.0f, i = 0 ; i < steps ; i++, angle += 2.0f*M_PI/(float)steps)
      {
         addVertex (cosf(angle)*radius + center.x(), sinf(angle)*radius + center.y(), 
                    bounds.max.z()) ;
      }

      for (i = 1 ; i <= steps ; i++)
      {
         p.firstElement = indices.size() ;
         p.numElements  = 3 ;
         indices.push_back (firstTop) ;
         indices.push_back (firstTop+(i == steps ? 1 : i+1)) ;
         indices.push_back (firstTop+i) ;
         primitives.push_back(p) ;
      }

      // Walls
      int pos ;
      for (pos = indices.size(), i = 0 ; i < steps-1 ; i++, pos += 4)
      {
         indices.push_back (i+1) ;
         indices.push_back (firstTop+i+1) ;
         indices.push_back (i+2) ;
         indices.push_back (firstTop+i+2) ;
         p.firstElement = pos ;
         p.numElements  = 4 ;
         primitives.push_back(p) ;
      }
      indices.push_back (i+1) ;
      indices.push_back (firstTop+i+1) ;
      indices.push_back (1) ;
      indices.push_back (firstTop+1) ;
      p.firstElement = pos ;
      p.numElements  = 4 ;
      primitives.push_back(p) ;

      // Other stuff

      setFrames (1) ;
      setParent (-1) ;
      calculateBounds() ;
      calculateCenter() ;
      calculateRadius() ;
   }

   // -----------------------------------------------------------------------
   //  Creates a box
   // -----------------------------------------------------------------------

   BoxMesh::BoxMesh (Box & _bounds)
   {
      setBounds(_bounds) ;
      construct() ;
   }

   void BoxMesh::construct() 
   {
      int   i ;
      Box   bounds = getBounds() ;
      Point center = (bounds.max - bounds.min) / 2 ;

      // Vertex
      verts.push_back(Point(bounds.min.x(), bounds.min.y(), bounds.min.z()));
      verts.push_back(Point(bounds.max.x(), bounds.min.y(), bounds.min.z()));
      verts.push_back(Point(bounds.min.x(), bounds.max.y(), bounds.min.z()));
      verts.push_back(Point(bounds.max.x(), bounds.max.y(), bounds.min.z()));
      verts.push_back(Point(bounds.min.x(), bounds.min.y(), bounds.max.z()));
      verts.push_back(Point(bounds.max.x(), bounds.min.y(), bounds.max.z()));
      verts.push_back(Point(bounds.min.x(), bounds.max.y(), bounds.max.z()));
      verts.push_back(Point(bounds.max.x(), bounds.max.y(), bounds.max.z()));

      // Texture coordinates
      for (i = 0 ; i < 8 ; i++)
         tverts.push_back(Point2D(0,0));

      // Indices
      int inds[] = { 0, 4, 1, 5, 3, 7, 2, 6, 0, 4, 6, 7, 4, 5, 0, 1, 2, 3 } ;
      for (i = 0 ; i < sizeof(inds)/sizeof(inds[0]) ; i++)
         indices.push_back(inds[i]);

      // Primitives
      Primitive p ;
      p.firstElement = 0 ;
      p.numElements  = 10 ;
      p.type         = Primitive::NoMaterial | Primitive::Strip | Primitive::Indexed ;
      primitives.push_back(p) ;
      p.firstElement = 10 ;
      p.numElements  = 4 ;
      primitives.push_back(p) ;
      p.firstElement = 14 ;
      primitives.push_back(p) ;

      // Normals
      std::vector<Point>::iterator ptr ;
      for (ptr = verts.begin() ; ptr != verts.end() ; ptr++)
      {
         Point normal ;
         normal = *ptr - center ;
         normal.normalize() ;
         normals.push_back (normal) ;
         enormals.push_back (encodeNormal(normal)) ;
      }

      // Other stuff
      setFrames (1) ;
      setParent (-1) ;
      calculateCenter() ;
      calculateRadius() ;
   }

}