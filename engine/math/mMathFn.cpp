//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "math/mMathFn.h"
#include "math/mPlane.h"
#include "math/mMatrix.h"


void mTransformPlane(const MatrixF& mat,
                     const Point3F& scale,
                     const PlaneF&  plane,
                     PlaneF*        result)
{
   m_matF_x_scale_x_planeF(mat, &scale.x, &plane.x, &result->x);
}


