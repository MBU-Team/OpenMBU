//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _WINDINGCLIPPER_H_
#define _WINDINGCLIPPER_H_

#ifndef _MMATH_H_
#include "math/mMath.h"
#endif

   void sgUtil_clipToPlane(Point3F* points, U32& rNumPoints, const PlaneF& rPlane);

#endif
