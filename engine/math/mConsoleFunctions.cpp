//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "platform/platform.h"
#include "console/console.h"
#include "math/mMathFn.h"
#include "math/mRandom.h"

ConsoleFunctionGroupBegin( GeneralMath, "General math functions. Use these whenever possible, as they'll run much faster than script equivalents.");

ConsoleFunction( mSolveQuadratic, const char *, 4, 4, "(float a, float b, float c)"
              "Solve a quadratic equation of form a*x^2 + b*x + c = 0.\n\n"
              "@returns A triple, contanining: sol x0 x1. sol is the number of"
              " solutions (being 0, 1, or 2), and x0 and x1 are the solutions, if any."
              " Unused x's are undefined.")
{
   char * retBuffer = Con::getReturnBuffer(256);
   F32 x[2];
   U32 sol = mSolveQuadratic(dAtof(argv[1]), dAtof(argv[2]), dAtof(argv[3]), x);
   dSprintf(retBuffer, 256, "%d %g %g", sol, x[0], x[1]);
   return retBuffer;
}

ConsoleFunction( mSolveCubic, const char *, 5, 5, "(float a, float b, float c, float d)"
              "Solve a cubic equation of form a*x^3 + b*x^2 + c*x + d = 0.\n\n"
              "@returns A 4-tuple, contanining: sol x0 x1 x2. sol is the number of"
              " solutions (being 0, 1, 2, or 3), and x0, x1, x2 are the solutions, if any."
              " Unused x's are undefined.")
{
   char * retBuffer = Con::getReturnBuffer(256);
   F32 x[3];
   U32 sol = mSolveCubic(dAtof(argv[1]), dAtof(argv[2]), dAtof(argv[3]), dAtof(argv[4]), x);
   dSprintf(retBuffer, 256, "%d %g %g %g", sol, x[0], x[1], x[2]);
   return retBuffer;
}

ConsoleFunction( mSolveQuartic, const char *, 6, 6, "(float a, float b, float c, float d, float e)"
              "Solve a quartic equation of form a*x^4 + b*x^3 + c*x^2 + d*x + e = 0.\n\n"
              "@returns A 5-tuple, contanining: sol x0 x1 x2 x3. sol is the number of"
              " solutions (ranging from 0-4), and x0, x1, x2 and x3 are the solutions, if any."
              " Unused x's are undefined.")
{
   char * retBuffer = Con::getReturnBuffer(256);
   F32 x[4];
   U32 sol = mSolveQuartic(dAtof(argv[1]), dAtof(argv[2]), dAtof(argv[3]), dAtof(argv[4]), dAtof(argv[5]), x);
   dSprintf(retBuffer, 256, "%d %g %g %g %g", sol, x[0], x[1], x[2], x[3]);
   return retBuffer;
}

ConsoleFunction( mFloor, S32, 2, 2, "(float v) Round v down to the nearest whole number.")
{
   return (S32)mFloor(dAtof(argv[1]));
}

ConsoleFunction( mCeil, S32, 2, 2, "(float v) Round v up to the nearest whole number.")
{
   return (S32)mCeil(dAtof(argv[1]));
}

ConsoleFunction( mFloatLength, const char *, 3, 3, "(float v, int numDecimals)"
              "Return a string containing v formatted with the specified number of decimal places.")
{
   char * outBuffer = Con::getReturnBuffer(256);
   char fmtString[8] = "%.0f";
   U32 precision = dAtoi(argv[2]);
   if (precision > 9)
      precision = 9;
   fmtString[2] = '0' + precision;

   dSprintf(outBuffer, 255, fmtString, dAtof(argv[1]));
   return outBuffer;
}

//------------------------------------------------------------------------------
ConsoleFunction( mAbs, F32, 2, 2, "(float v) Returns the absolute value of the argument.")
{
   return(mFabs(dAtof(argv[1])));
}

ConsoleFunction( mSqrt, F32, 2, 2, "(float v) Returns the square root of the argument.")
{
   return(mSqrt(dAtof(argv[1])));
}

ConsoleFunction( mPow, F32, 3, 3, "(float b, float p) Returns the b raised to the pth power.")
{
   return(mPow(dAtof(argv[1]), dAtof(argv[2])));
}

ConsoleFunction( mLog, F32, 2, 2, "(float v) Returns the natural logarithm of the argument.")
{
   return(mLog(dAtof(argv[1])));
}

ConsoleFunction( mSin, F32, 2, 2, "(float th) Returns the sine of th, which is in radians.")
{
   return(mSin(dAtof(argv[1])));
}

ConsoleFunction( mCos, F32, 2, 2, "(float th) Returns the cosine of th, which is in radians.")
{
   return(mCos(dAtof(argv[1])));
}

ConsoleFunction( mTan, F32, 2, 2, "(float th) Returns the tangent of th, which is in radians.")
{
   return(mTan(dAtof(argv[1])));
}

ConsoleFunction( mAsin, F32, 2, 2, "(float th) Returns the arc-sine of th, which is in radians.")
{
   return(mAsin(dAtof(argv[1])));
}

ConsoleFunction( mAcos, F32, 2, 2, "(float th) Returns the arc-cosine of th, which is in radians.")
{
   return(mAcos(dAtof(argv[1])));
}

ConsoleFunction( mAtan, F32, 3, 3, "(float rise, float run) Returns the slope in radians (the arc-tangent) of a line with the given rise and run.")
{
   return(mAtan(dAtof(argv[1]), dAtof(argv[2])));
}

ConsoleFunction( mRadToDeg, F32, 2, 2, "(float radians) Converts a measure in radians to degrees.")
{
   return(mRadToDeg(dAtof(argv[1])));
}

ConsoleFunction( mDegToRad, F32, 2, 2, "(float degrees) Convert a measure in degrees to radians.")
{
   return(mDegToRad(dAtof(argv[1])));
}

ConsoleFunctionGroupEnd( GeneralMath );
