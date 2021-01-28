//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _MQUADPATCH_H_
#define _MQUADPATCH_H_

#ifndef _PLATFORM_H_
#include "platform/platform.h"
#endif
#ifndef _MPOINT_H_
#include "math/mPoint.h"
#endif
#ifndef _MSPLINEPATCH_H_
#include "math/mSplinePatch.h"
#endif

//------------------------------------------------------------------------------
/// Quadratic spline patch.  This is a special type of spline that only had 3 control points.
/// @see SplinePatch
class QuadPatch : public SplinePatch
{
   typedef SplinePatch Parent;

private:
   Point3F a, b, c;

   void calcABC( const Point3F *points );

public:

   QuadPatch();

   virtual void   calc( F32 t, Point3F &result );
   virtual void   calc( Point3F *points, F32 t, Point3F &result );
   virtual void   setControlPoint( Point3F &point, int index );
   virtual void   submitControlPoints( SplCtrlPts &points );


};



#endif
