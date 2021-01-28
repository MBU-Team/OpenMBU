#ifndef __DTSPOINT_H
#define __DTSPOINT_H

#include <iomanip>
#include <iostream>

#include "DTSVector.h"

namespace DTS
{
   class Point3D : public Vector<float, 3>
   {
      public:
         
         Point3D() {}
         Point3D(const Point3D &n) : Vector<float,3>(n) {}
         Point3D(const Vector<float,3> &n) : Vector<float,3>(n) {}
         Point3D(float x, float y, float z) { set (0,x), set(1,y), set(2,z); }
         
         float x() const  { return get(0); }
         float y() const  { return get(1); }
         float z() const  { return get(2); }
         void  x(float n) { set (0, n) ;   }
         void  y(float n) { set (1, n) ;   }
         void  z(float n) { set (2, n) ;   }
   }; 

   typedef Point3D Point ;

   class Point2D : public Vector<float, 2>
   {
      public:

         Point2D() {}
         Point2D(const Point2D &n) : Vector<float,2>(n) {}
         Point2D(const Vector<float,2> &n) : Vector<float,2>(n) {}
         Point2D(float x, float y) { set(0,x), set(1,y); }
         
         float x() const  { return get(0); }
         float y() const  { return get(1); }
         void  x(float n) { set (0, n) ;   }
         void  y(float n) { set (1, n) ;   }
   };

   //! Defines a box in space. Used for bounding boxes.

   struct Box
   {
      Point min, max ;

      Box() {}

      //! Create a box from two size in space. Note that no check is done,
      //! the two size need to define a correct box (min < max)
      Box(Point MIN, Point MAX) { min = MIN; max = MAX; }
   };

   inline std::ostream & operator << (std::ostream &out, const Point3D &q)
   {
      out << std::setprecision(2) << std::setiosflags(std::ios::fixed) ;
      return out << "< " << q.x() << ", " << q.y() << ", " << q.z() << " >" ;
   }
   inline std::ostream & operator << (std::ostream &out, const Point2D &q)
   {
      out << std::setprecision(2) << std::setiosflags(std::ios::fixed) ;
      return out << "< " << q.x() << ", " << q.y() << " >" ;
   }
   inline std::ostream & operator << (std::ostream &out, const Box &q)
   {
      out << std::setprecision(2) << std::setiosflags(std::ios::fixed) ;
      return out << "( " << q.min << " - " ;
   }
}

#endif