//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _MSPHERE_H_
#define _MSPHERE_H_

//Includes
#ifndef _MPOINT_H_
#include "math/mPoint.h"
#endif

class SphereF
{
  public:
   Point3F center;
   F32     radius;

  public:
   SphereF() { }
   SphereF(const Point3F& in_rPosition,
           const F32    in_rRadius)
    : center(in_rPosition),
      radius(in_rRadius)
   {
      if (radius < 0.0f)
         radius = 0.0f;
   }

   bool isContained(const Point3F& in_rContain) const;
   bool isContained(const SphereF& in_rContain) const;
   bool isIntersecting(const SphereF& in_rIntersect) const;
};

//-------------------------------------- INLINES
//
inline bool
SphereF::isContained(const Point3F& in_rContain) const
{
   F32 distSq = (center - in_rContain).lenSquared();

   return (distSq <= (radius * radius));
}

inline bool
SphereF::isContained(const SphereF& in_rContain) const
{
   if (radius < in_rContain.radius)
      return false;

   // Since our radius is guaranteed to be >= other's, we
   //  can dodge the sqrt() here.
   //
   F32 dist = (in_rContain.center - center).lenSquared();

   return (dist <= ((radius - in_rContain.radius) *
                    (radius - in_rContain.radius)));
}

inline bool
SphereF::isIntersecting(const SphereF& in_rIntersect) const
{
   F32 distSq = (in_rIntersect.center - center).lenSquared();

   return (distSq <= ((in_rIntersect.radius + radius) *
                      (in_rIntersect.radius + radius)));
}

#endif //_SPHERE_H_
