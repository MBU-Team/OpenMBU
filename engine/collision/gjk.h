//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _GJK_H_
#define _GJK_H_

#ifndef _MMATH_H_
#include "math/mMath.h"
#endif
#ifndef _TVECTOR_H_
#include "core/tVector.h"
#endif
#ifndef _CONVEX_H_
#include "collision/convex.h"
#endif

class Convex;
struct CollisionStateList;
struct Collision;

//----------------------------------------------------------------------------

struct GjkCollisionState: public CollisionState
{
   /// @name Temporary values
   /// @{
   Point3F p[4];     ///< support points of object A in local coordinates
   Point3F q[4];     ///< support points of object B in local coordinates
   VectorF y[4];     ///< support points of A - B in world coordinates

   S32 bits;         ///< identifies current simplex
   S32 all_bits;     ///< all_bits = bits | last_bit
   F32 det[16][4];   ///< cached sub-determinants
   F32 dp[4][4];     ///< cached dot products

   S32 last;         ///< identifies last found support point
   S32 last_bit;     ///< last_bit = 1<<last
   /// @}

   ///
   void compute_det();
   bool valid(int s);
   void compute_vector(int bits, VectorF& v);
   bool closest(VectorF& v);
   bool degenerate(const VectorF& w);
   void nextBit();
   void swap();
   void reset(const MatrixF& a2w, const MatrixF& b2w);

   GjkCollisionState();
   ~GjkCollisionState();

   void set(Convex* a,Convex* b,const MatrixF& a2w, const MatrixF& b2w);

   void getCollisionInfo(const MatrixF& mat, Collision* info);
   void getClosestPoints(Point3F& p1, Point3F& p2);
   bool intersect(const MatrixF& a2w, const MatrixF& b2w);
   F32 distance(const MatrixF& a2w, const MatrixF& b2w, const F32 dontCareDist,
                       const MatrixF* w2a = NULL, const MatrixF* _w2b = NULL);
   void render();
};


#endif
