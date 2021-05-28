#ifndef __DTSQUATERNION_H
#define __DTSQUATERNION_H

#include "DTSVector.h"
#include "DTSMatrix.h"
#include "DTSPoint.h"

namespace DTS
{   
   //! Defines a quaternion. Used for rotation information.

   class Quaternion : public Vector <float, 4>
   {  
      public:

         Quaternion() {}
         Quaternion(const Quaternion &n) : Vector<float,4>(n) {}
         Quaternion(const Vector<float,4> &n) : Vector<float,4>(n) {}
         Quaternion(float X, float Y, float Z, float W) { x(X),y(Y),z(Z),w(W); }
         Quaternion(float angle, float latitude, float longitude) ;
         Quaternion(const Matrix<4,4> &m) ;
         Quaternion(const Point &axis, float angle) { fromAxis(axis,angle); }
         
         float x() const  { return get(0); }
         float y() const  { return get(1); }
         float z() const  { return get(2); }
         float w() const  { return get(3); }
         void  x(float n) { set (0, n) ; }
         void  y(float n) { set (1, n) ; }
         void  z(float n) { set (2, n) ; }
         void  w(float n) { set (3, n) ; }

         // -------------------------------------------------------------

         Quaternion  conjugate() const { return Quaternion (-x(),-y(),-z(),w()) ; }
         Quaternion  inverse() const ;
         Matrix<4,4> toMatrix() const ;
         void        fromMatrix (const Matrix<4,4> &m) ;
         void        fromAxis   (Point axis, float angle) ;
         float       angleBetween (const Quaternion &) const ;
         Point       apply (const Point &) const ;

         friend Quaternion operator * (const Quaternion &a, const Quaternion &b) ;
         Quaternion & operator *= (const Quaternion &a) { return (*this) = (*this) * a; }

         // -------------------------------------------------------------

         static Quaternion identity ;
   };


   inline std::ostream & operator << (std::ostream &out, const Quaternion &q)
   {
      out << std::setprecision(2) << std::setiosflags(std::ios::fixed) ;
      return out << "[ " << q.x() << ' ' << q.y() << ' ' << q.z() << ' ' << q.w() << " ]" ;
   }
}

#endif