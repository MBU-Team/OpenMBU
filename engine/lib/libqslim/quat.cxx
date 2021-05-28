/************************************************************************

  Quaternion class
  
  $Id: quat.cxx,v 1.2 2006/09/07 00:05:52 bramage Exp $

 ************************************************************************/

#include "quat.h"

// Based on code from the Appendix of
// 	Quaternion Calculus for Computer Graphics, by Ken Shoemake
// Computes the exponential of a quaternion, assuming scalar part w=0
Quat exp(const Quat& q)
{
    double theta = norm(q.vector());
    double c = cos(theta);

    if( theta > FEQ_EPS )
    {
	double s = sin(theta) / theta;
	return Quat( s*q.vector(), c);
    }
    else
	return Quat(q.vector(), c);
}

// Based on code from the Appendix of
// 	Quaternion Calculus for Computer Graphics, by Ken Shoemake
// Computes the natural logarithm of a UNIT quaternion
Quat log(const Quat& q)
{
    double scale = norm(q.vector());
    double theta = atan2(scale, q.scalar());

    if( scale > 0.0 )  scale=theta/scale;

    return Quat(scale*q.vector(), 0.0);
}

Quat axis_to_quat(const Vec3& a, double phi)
{
    Vec3 u = a;
    unitize(u);

    double s = sin(phi/2.0);
    return Quat(u[0]*s, u[1]*s, u[2]*s, cos(phi/2.0));
}

Mat4 quat_to_matrix(const Quat& q)
{
    Mat4 M;

    const double x = q.vector()[0];
    const double y = q.vector()[1];
    const double z = q.vector()[2];
    const double w = q.scalar();
    const double s = 2/norm(q);

    M(0,0)=1-s*(y*y+z*z); M(0,1)=s*(x*y-w*z);   M(0,2)=s*(x*z+w*y);   M(0,3)=0;
    M(1,0)=s*(x*y+w*z);   M(1,1)=1-s*(x*x+z*z); M(1,2)=s*(y*z-w*x);   M(1,3)=0;
    M(2,0)=s*(x*z-w*y);   M(2,1)=s*(y*z+w*x);   M(2,2)=1-s*(x*x+y*y); M(2,3)=0;
    M(3,0)=0;             M(3,1)=0;             M(3,2)=0;             M(3,3)=1;

    return M;
}

Mat4 unit_quat_to_matrix(const Quat& q)
{
    Mat4 M;

    const double x = q.vector()[0];
    const double y = q.vector()[1];
    const double z = q.vector()[2];
    const double w = q.scalar();

    M(0,0)=1-2*(y*y+z*z); M(0,1)=2*(x*y-w*z);   M(0,2)=2*(x*z+w*y);   M(0,3)=0;
    M(1,0)=2*(x*y+w*z);   M(1,1)=1-2*(x*x+z*z); M(1,2)=2*(y*z-w*x);   M(1,3)=0;
    M(2,0)=2*(x*z-w*y);   M(2,1)=2*(y*z+w*x);   M(2,2)=1-2*(x*x+y*y); M(2,3)=0;
    M(3,0)=0;             M(3,1)=0;             M(3,2)=0;             M(3,3)=1;

    return M;
}

Quat slerp(const Quat& from, const Quat& to, double t)
{
    const Vec3& v_from = from.vector();
    const Vec3& v_to = to.vector();
    const double s_from = from.scalar();
    const double s_to = to.scalar();

    double cosine = v_from*v_to + s_from*s_to;
    double sine = sqrt(1 - cosine*cosine);

    if( (1+cosine) < FEQ_EPS )
    {
	// The quaternions are (nearly) diametrically opposed.  We
	// treat this specially (based on suggestion in Watt & Watt).
	//
	double A = sin( (1-t)*M_PI/2.0 );
	double B = sin( t*M_PI/2.0 );

	return Quat( A*v_from[0] + B*(-v_from[1]),
		     A*v_from[1] + B*(v_from[0]),
		     A*v_from[2] + B*(-s_from),
		     A*v_from[3] + B*(v_from[2]) );
    }

    double A, B;
    if( (1-cosine) < FEQ_EPS )
    {
	// The quaternions are very close.  Approximate with normal
	// linear interpolation.  This is cheaper and avoids division
	// by very small numbers.
	//
	A = 1.0 - t;
	B = t;
    }
    else
    {
	// This is the normal case.  Perform SLERP.
	//
	double theta = acos(cosine);
	double sine = sin(theta);

	A = sin( (1-t)*theta ) / sine;
	B = sin( t*theta ) / sine;

    }

    return Quat( A*v_from + B*v_to,  A*s_from + B*s_to);
}
