//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _REALCOMP_H_
#define _REALCOMP_H_

//Includes
#ifndef _MMATHFN_H_
#include "math/mMathFn.h"
#endif
#ifndef _PLATFORM_H_
#include "platform/platform.h"
#endif

inline bool isEqual(F32 a, F32 b)
{
   return mFabs(a - b) < __EQUAL_CONST_F;
}

inline bool isZero(F32 a)
{
   return mFabs(a) < __EQUAL_CONST_F;
}

#endif //_REALCOMP_H_
