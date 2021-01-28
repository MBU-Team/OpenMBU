#ifndef GFXQUAT_INCLUDED // -*- C++ -*-
#define GFXQUAT_INCLUDED
#if !defined(__GNUC__)
#  pragma once
#endif

/************************************************************************

  Quaternion class
  
  $Id: quat.h,v 1.2 2006/09/07 00:05:52 bramage Exp $

 ************************************************************************/

#include "mat4.h"

class Quat
{
private:
    Vec3 v;			// Vector component
    double s;			// Scalar component

public:
    Quat()                                       { v=0.0; s=1.0; }
    Quat(double x, double y, double z, double w) { v[0]=x;v[1]=y;v[2]=z; s=w; }
    Quat(const Vec3& a, double b)                { v=a; s=b; }
    Quat(const Quat& q)                          { *this=q; }

    // Access methods
    const Vec3& vector() const { return v; }
    Vec3&       vector()       { return v; }
    double      scalar() const { return s; }
    double&     scalar()       { return s; }

    // Assignment and in-place arithmetic methods
    Quat& operator=(const Quat& q);
    Quat& operator+=(const Quat& q);
    Quat& operator-=(const Quat& q);
    Quat& operator=(double d);
    Quat& operator*=(double d);
    Quat& operator/=(double d);

    // Construction of standard quaternions
    static Quat ident();
};

////////////////////////////////////////////////////////////////////////
//
// Implementation of Quat methods
//

inline Quat& Quat::operator=(const Quat& q) { v=q.v; s=q.s; return *this; }
inline Quat& Quat::operator+=(const Quat& q) { v+=q.v; s+=q.s; return *this; }
inline Quat& Quat::operator-=(const Quat& q) { v-=q.v; s-=q.s; return *this; }

inline Quat& Quat::operator=(double d)  { v=d;  s=d;  return *this; }
inline Quat& Quat::operator*=(double d) { v*=d; s*=d; return *this; }
inline Quat& Quat::operator/=(double d) { v/=d; s/=d; return *this; }

inline Quat Quat::ident() { return Quat(0, 0, 0, 1); }

////////////////////////////////////////////////////////////////////////
//
// Standard arithmetic operators on quaternions
//

inline Quat operator+(const Quat& q, const Quat& r)
	{ return Quat(q.vector()+r.vector(), q.scalar()+r.scalar()); }

inline Quat operator*(const Quat& q, const Quat& r)
{
    return Quat(cross(q.vector(),r.vector()) +
		r.scalar()*q.vector() +
		q.scalar()*r.vector(),
		q.scalar()*r.scalar() - q.vector()*r.vector());
}

inline Quat operator*(const Quat& q, double s)
	{ return Quat(q.vector()*s, q.scalar()*s); }
inline Quat operator*(double s, const Quat& q)
	{ return Quat(q.vector()*s, q.scalar()*s); }

inline Quat operator/(const Quat& q, double s)
	{ return Quat(q.vector()/s, q.scalar()/s); }

/*inline std::ostream &operator<<(std::ostream &out, const Quat& q)
	{ return out << q.vector() << " " << q.scalar(); }

inline std::istream &operator>>(std::istream &in, Quat& q)
	{ return in >> q.vector() >> q.scalar(); } */


////////////////////////////////////////////////////////////////////////
//
// Standard functions on quaternions
//

inline double norm(const Quat& q)
	{ return q.scalar()*q.scalar() + norm2(q.vector()); }

inline Quat conjugate(const Quat& q) { return Quat(-q.vector(), q.scalar()); }
inline Quat inverse(const Quat& q) { return conjugate(q)/norm(q); }
inline void unitize(Quat& q)  { q /= norm(q); }

extern Quat exp(const Quat& q);
extern Quat log(const Quat& q);
extern Quat axis_to_quat(const Vec3& a, double phi);
extern Mat4 quat_to_matrix(const Quat& q);
extern Mat4 unit_quat_to_matrix(const Quat& q);
extern Quat slerp(const Quat& from, const Quat& to, double t);

// GFXQUAT_INCLUDED
#endif
