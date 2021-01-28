//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _TSTRANSFORM_H_
#define _TSTRANSFORM_H_

#ifndef _MMATH_H_
#include "math/mMath.h"
#endif

class Stream;

/// compressed quaternion class
struct Quat16
{
   enum { MAX_VAL = 0x7fff };

   S16 x, y, z, w;

   void read(Stream *);
   void write(Stream *);

   void identity();
   QuatF &getQuatF( QuatF * q ) const;
   void set( const QuatF & q );
   S32 operator==( const Quat16 & q ) const;
};

/// class to handle general scaling case
///
/// transform = rot * scale * inv(rot)
struct TSScale
{
   QuatF            mRotate;
   Point3F          mScale;

   void identity() { mRotate.identity(); mScale.set( 1.0f,1.0f,1.0f ); }
   S32 operator==( const TSScale & other ) const { return mRotate==other.mRotate && mScale==other.mScale; }
};

/// struct for encapsulating ts transform related static functions
struct TSTransform
{
   static Point3F & interpolate(const Point3F & p1, const Point3F & p2, F32 t, Point3F *);
   static F32       interpolate(F32             p1, F32             p2, F32 t);
   static QuatF   & interpolate(const QuatF   & q1, const QuatF   & q2, F32 t, QuatF   *);
   static TSScale & interpolate(const TSScale & s1, const TSScale & s2, F32 t, TSScale *);

   static void      setMatrix(const QuatF     &  q, MatrixF *);
   static void      setMatrix(const QuatF     &  q, const Point3F & p, MatrixF *);

   static void      applyScale(F32 scale, MatrixF *);
   static void      applyScale(const Point3F & scale, MatrixF *);
   static void      applyScale(const TSScale & scale, MatrixF *);
};

inline Point3F & TSTransform::interpolate(const Point3F & p1, const Point3F & p2, F32 t, Point3F * p)
{
   p->x = p1.x + t * (p2.x-p1.x);
   p->y = p1.y + t * (p2.y-p1.y);
   p->z = p1.z + t * (p2.z-p1.z);
   return *p;
}

inline F32 TSTransform::interpolate(F32 p1, F32 p2, F32 t)
{
   return p1 + t*(p2-p1);
}

inline TSScale & TSTransform::interpolate(const TSScale & s1, const TSScale & s2, F32 t, TSScale * s)
{
   TSTransform::interpolate(s1.mRotate,s2.mRotate,t,&s->mRotate);
   TSTransform::interpolate(s1.mScale,s2.mScale,t,&s->mScale);
   return *s;
}

inline void TSTransform::setMatrix( const QuatF & q, const Point3F & p, MatrixF * pDest )
{
   q.setMatrix(pDest);
   pDest->setColumn(3,p);
}

inline void TSTransform::setMatrix( const QuatF & q, MatrixF * pDest )
{
   q.setMatrix(pDest);
}

#endif
