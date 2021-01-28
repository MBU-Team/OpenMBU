//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "ts/tsTransform.h"
#include "core/stream.h"

void Quat16::identity()
{
   x = y = z = 0;
   w = MAX_VAL;
}

QuatF & Quat16::getQuatF( QuatF * q ) const
{
   q->x = float( x ) / float(MAX_VAL);
   q->y = float( y ) / float(MAX_VAL);
   q->z = float( z ) / float(MAX_VAL);
   q->w = float( w ) / float(MAX_VAL);
   return *q;
}

void Quat16::set( const QuatF & q )
{
   x = (S16)(q.x * float(MAX_VAL));
   y = (S16)(q.y * float(MAX_VAL));
   z = (S16)(q.z * float(MAX_VAL));
   w = (S16)(q.w * float(MAX_VAL));
}

S32 Quat16::operator==( const Quat16 & q ) const
{
   return( x == q.x && y == q.y && z == q.z && w == q.w );
}

void Quat16::read(Stream * s)
{
    s->read(&x);
    s->read(&y);
    s->read(&z);
    s->read(&w);
}

void Quat16::write(Stream * s)
{
    s->write(x);
    s->write(y);
    s->write(z);
    s->write(w);
}

QuatF & TSTransform::interpolate( const QuatF & q1, const QuatF & q2, F32 interp, QuatF * q )
{
   F32 Dot;
   F32 Dist2;
   F32 OneOverL;
   F32 x1,y1,z1,w1;
   F32 x2,y2,z2,w2;

   //
   // This is a linear interpolation with a fast renormalization.
   //
   x1 = q1.x;
   y1 = q1.y;
   z1 = q1.z;
   w1 = q1.w;
   x2 = q2.x;
   y2 = q2.y;
   z2 = q2.z;
   w2 = q2.w;

   // Determine if quats are further than 90 degrees
   Dot = x1*x2 + y1*y2 + z1*z2 + w1*w2;

   // If dot is negative flip one of the quaterions
   if( Dot < 0.0f )
   {
       x1 = -x1;
       y1 = -y1;
       z1 = -z1;
       w1 = -w1;
   }

   // Compute interpolated values
   x1 = x1 + interp*(x2 - x1);
   y1 = y1 + interp*(y2 - y1);
   z1 = z1 + interp*(z2 - z1);
   w1 = w1 + interp*(w2 - w1);

   // Get squared distance of new quaternion
   Dist2 = x1*x1 + y1*y1 + z1*z1 + w1*w1;

   // Use home-baked polynomial to compute 1/sqrt(Dist2)
   // since we know the range is 0.707 >= Dist2 <= 1.0
   // we'll split in half.

   if( Dist2<0.857f )
      OneOverL = (((0.699368f)*Dist2) + -1.819985f)*Dist2 + 2.126369f;    //0.0000792
   else
      OneOverL = (((0.454012f)*Dist2) + -1.403517f)*Dist2 + 1.949542f;    //0.0000373

   // Renormalize
   q->x = x1*OneOverL;
   q->y = y1*OneOverL;
   q->z = z1*OneOverL;
   q->w = w1*OneOverL;

   return *q;
}

void TSTransform::applyScale(F32 scale, MatrixF * mat)
{
   mat->scale(Point3F(scale,scale,scale));
}

void TSTransform::applyScale(const Point3F & scale, MatrixF * mat)
{
   mat->scale(scale);
}

void TSTransform::applyScale(const TSScale & scale, MatrixF * mat)
{
   MatrixF mat2;
   TSTransform::setMatrix(scale.mRotate,&mat2);
   MatrixF mat3(mat2);
   mat3.inverse();
   mat2.scale(scale.mScale);
   mat2.mul(mat3);
   mat->mul(mat2);
}
