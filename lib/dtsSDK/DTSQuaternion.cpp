
#pragma warning ( disable: 4786 )
#include <vector>
#include <cmath>

#include "DTSQuaternion.h"

namespace DTS
{
   Quaternion Quaternion::identity (0,0,0,1) ;

   /*! Create a quaternion from three spherical rotation angles
       (XY rotation angle, latitude and longitude) in radians */

   Quaternion::Quaternion (float angle, float latitude, float longitude)
   {
      Quaternion x (Point( 1, 0, 0), angle) ;
      Quaternion y (Point( 0, 1, 0), latitude) ;
      Quaternion z (Point( 0, 0, 1), longitude) ;

      // This order is a little wierd, but we flip the x & y 
      // axis when importing from MilkShape.
      (*this) = x * z * y;
   }

   /*! Calculate the angle in radians between this and another quaternion */

   float Quaternion::angleBetween (const Quaternion &b) const 
   {
      return acos (x()*b.x() + y()*b.y() + z()*b.z() + w()*b.w()) ;
   }

   /*! Calculate the rotation matrix */

   Matrix<4,4> Quaternion::toMatrix () const
   {
      float xx = x() * x() ;
      float xy = x() * y(), yy = y() * y() ;
      float xz = x() * z(), yz = y() * z(), zz = z() * z() ;
      float xw = x() * w(), yw = y() * w(), zw = z() * w(), ww = w() * w()  ;

      Matrix<4,4>::Cell data[] = {
         1 - 2 * (yy+zz),     2 * (xy-zw),     2 * (xz+yw),     0,
             2 * (xy+zw), 1 - 2 * (xx+zz),     2 * (yz-xw),     0,
             2 * (xz-yw),     2 * (yz+xw), 1 - 2 * (xx+yy),     0,
             0,               0,               0,               1
      } ;
      return Matrix<4,4>(data) ;
   }

   /*! Create a quaternion from a rotation matrix */

   Quaternion::Quaternion (const Matrix<> &m)
   {
      fromMatrix(m) ;
   }

   void Quaternion::fromAxis (Point axis, float angle)
   {
      axis.normalize() ;
      float s = sin(angle / 2.0f) ;
      x(axis.x() * s) ;
      y(axis.y() * s) ;
      z(axis.z() * s) ;
      w(cos(angle / 2.0f)) ;
   }

   void Quaternion::fromMatrix (const Matrix<> &m)
   {
      float qw2 = (1+ m[0][0] + m[1][1] + m[2][2]) * 0.25f ;
      float qx2 = qw2 - 0.5f * (m[1][1] + m[2][2]) ;
      float qy2 = qw2 - 0.5f * (m[0][0] + m[2][2]) ;
      float qz2 = qw2 - 0.5f * (m[0][0] + m[1][1]) ;
      float s ;

      if (qw2 > qx2 && qw2 > qy2 && qw2 > qz2)
      {
         w( sqrtf(qw2) ); s = 0.25f / w() ;
         x( (m[2][1] - m[1][2]) * s );
         y( (m[0][2] - m[2][0]) * s );
         z( (m[1][0] - m[0][1]) * s );
      }
      else if (qx2 > qy2 && qx2 > qz2)
      {
         x( sqrtf(qx2) ); s = 0.25f / x() ;
         w( (m[2][1] - m[1][2]) * s );
         y( (m[0][1] - m[1][0]) * s );
         z( (m[2][0] - m[0][2]) * s );
      }
      else if (qy2 > qz2)
      {
         y( sqrtf(qy2) ); s = 0.25f / y() ;
         w( (m[0][2] - m[2][0]) * s );
         x( (m[0][1] - m[1][0]) * s );
         z( (m[1][2] - m[2][1]) * s );
      }
      else
      {
         z( sqrtf(qz2) ); s = 0.25f / z() ;
         w( (m[1][0] - m[0][1]) * s );
         x( (m[0][2] - m[2][0]) * s );
         y( (m[1][2] - m[2][1]) * s );
      }

      //normalize() ;
   }

   /*! Return the inverse of a quaternion */

   Quaternion Quaternion::inverse() const
   {
      float norm = x()*x() + y()*y() + z()*z() + w()*w() ;
      
      return conjugate() * (1.0f / norm) ;
   }

   Quaternion operator * (const Quaternion &a, const Quaternion &b) 
   {
      return Quaternion (
         +a[0]*b[3] +a[1]*b[2] -a[2]*b[1] +a[3]*b[0],
         -a[0]*b[2] +a[1]*b[3] +a[2]*b[0] +a[3]*b[1],
         +a[0]*b[1] -a[1]*b[0] +a[2]*b[3] +a[3]*b[2],
         -a[0]*b[0] -a[1]*b[1] -a[2]*b[2] +a[3]*b[3]
         ) ;
   }

   Point Quaternion::apply (const Point &v) const
   {
      // Torque uses column vectors, which means quaternions
      // rotate backwards from what you might normally expect.
      Quaternion q(v.x(), v.y(), v.z(), 0) ;
      Quaternion r =  conjugate() * q * (*this);
      return Point(r.x(), r.y(), r.z()) ;

      // The following should give the same result:
      //Matrix<> m = toMatrix() ;
      //return m * v ;
   }

}