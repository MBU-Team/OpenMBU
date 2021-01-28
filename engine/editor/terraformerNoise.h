//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _TERRAFORMER_NOISE_H_
#define _TERRAFORMER_NOISE_H_


#ifndef _PLATFORM_H_
#include "platform/platform.h"
#endif
#ifndef _MMATH_H_
#include "math/mMath.h"
#endif
#ifndef _MRANDOM_H_
#include "math/mRandom.h"
#endif

struct Heightfield;

class Noise2D
{
private:
   enum Constants {
      SIZE      = 0x100,
      SIZE_MASK = 0x0ff
   };
   S32 mPermutation[SIZE + SIZE + 2];
   F32 mGradient[SIZE + SIZE + 2][2];

   U32 mSeed;

   MRandom mRandom;

   F32  lerp(F32 t, F32 a, F32 b);
   F32  curve(F32 t);
   void setup(F32 t, S32 &b0, S32 &b1, F32 &r0, F32 &r1);
   F32  dot(const F32 *q, F32 rx, F32 ry);
   void normalize(F32 v[2]);


public:
   Noise2D();
   ~Noise2D();

   void setSeed(U32 seed);
   U32  getSeed();

   F32  getValue(F32 u, F32 v, S32 interval);
   void fBm(Heightfield *dst, U32 size, U32 interval, F32 h, F32 octave=5.0f);
   void rigidMultiFractal(Heightfield *dst, Heightfield *signal, U32 size, U32 interval, F32 h, F32 octave=5.0f);
   F32  turbulence(F32 x, F32 y, F32 freq);
};



#endif  // _H_TERRAFORMER_NOISE_
