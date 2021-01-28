//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "math/mPlaneTransformer.h"
#include "math/mMathFn.h"

void PlaneTransformer::set(const MatrixF& xform, const Point3F& scale)
{
   mTransform = xform;
   mScale     = scale;

   MatrixF scaleMat(true);
   F32* m = scaleMat;
   m[MatrixF::idx(0, 0)] = scale.x;
   m[MatrixF::idx(1, 1)] = scale.y;
   m[MatrixF::idx(2, 2)] = scale.z;

   mTransposeInverse = xform;
   mTransposeInverse.mul(scaleMat);
   mTransposeInverse.transpose();
   mTransposeInverse.inverse();
}

void PlaneTransformer::transform(const PlaneF& plane, PlaneF& result)
{
   Point3F point = plane;
   point *= -plane.d;
   point.convolve(mScale);
   mTransform.mulP(point);

   Point3F normal = plane;
   mTransposeInverse.mulV(normal);

   result.set(point, normal);
//   mTransformPlane(mTransform, mScale, plane, &result);
}

void PlaneTransformer::setIdentity()
{
   static struct MakeIdentity {
      PlaneTransformer  transformer;
      MakeIdentity() {
         MatrixF  defMat(true);
         Point3F  defScale(1, 1, 1);
         transformer.set(defMat, defScale);
      }
   } sMakeIdentity;

   * this = sMakeIdentity.transformer;
}
