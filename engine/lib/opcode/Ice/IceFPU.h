///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Contains FPU related code.
 *	\file		IceFPU.h
 *	\author		Pierre Terdiman
 *	\date		April, 4, 2000
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Include Guard
#ifndef __ICEFPU_H__
#define __ICEFPU_H__

#if defined( PLATFORM_XENON )
#  include <PPCIntrinsics.h>
#endif
#include <math.h>
#include "IcePreprocessor.h"
#include "IceTypes.h"

	#define	SIGN_BITMASK			0x80000000

	//! Integer representation of a floating-point value.
	#define IR(x)					((udword&)(x))

	//! Signed integer representation of a floating-point value.
	#define SIR(x)					((sdword&)(x))

	//! Absolute integer representation of a floating-point value
	#define AIR(x)					(IR(x)&0x7fffffff)

	//! Floating-point representation of an integer value.
	#define FR(x)					((float&)(x))

	//! Integer-based comparison of a floating point value.
	//! Don't use it blindly, it can be faster or slower than the FPU comparison, depends on the context.
	#define IS_NEGATIVE_FLOAT(x)	(IR(x)&0x80000000)

	//! Fast fabs for floating-point values. It just clears the sign bit.
	//! Don't use it blindy, it can be faster or slower than the FPU comparison, depends on the context.
	inline float FastFabs(float x)
	{
		udword FloatBits = IR(x)&0x7fffffff;
		return FR(FloatBits);
	}

	//! Fast square root for floating-point values.
	inline float FastSqrt(float square)
	{
		return( sqrtf( square ) );
	}

	//! Saturates positive to zero.
	float fsat(float f)
	{
		udword y = (udword&)f & ~((sdword&)f >>31);
		return (float&)y;
	}

	//! Computes 1.0f / sqrtf(x).
	float frsqrt(float f)
	{
		float x = f * 0.5f;
		udword y = 0x5f3759df - ((udword&)f >> 1);
		// Iteration...
		(float&)y  = (float&)y * ( 1.5f - ( x * (float&)y * (float&)y ) );
		// Result
		return (float&)y;
	}

	//! Computes 1.0f / sqrtf(x). Comes from NVIDIA.
	float InvSqrt(const float& x)
	{
		udword tmp = (udword(IEEE_1_0 << 1) + IEEE_1_0 - *(udword*)&x) >> 1;   
		float y = *(float*)&tmp;                                             
		return y * (1.47f - 0.47f * x * y * y);
	}

	//! Computes 1.0f / sqrtf(x). Comes from Quake3. Looks like the first one I had above.
	//! See http://www.magic-software.com/3DGEDInvSqrt.html
	float RSqrt(float number)
	{
		long i;
		float x2, y;
		const float threehalfs = 1.5f;

		x2 = number * 0.5f;
		y  = number;
		i  = * (long *) &y;
		i  = 0x5f3759df - (i >> 1);
		y  = * (float *) &i;
		y  = y * (threehalfs - (x2 * y * y));

		return y;
	}

	//! TO BE DOCUMENTED
	float fsqrt(float f)
	{
		udword y = ( ( (sdword&)f - 0x3f800000 ) >> 1 ) + 0x3f800000;
		// Iteration...?
		// (float&)y = (3.0f - ((float&)y * (float&)y) / f) * (float&)y * 0.5f;
		// Result
		return (float&)y;
	}

	//! Returns the float ranged espilon value.
	float fepsilon(float f)
	{
		udword b = (udword&)f & 0xff800000;
		udword a = b | 0x00000001;
		(float&)a -= (float&)b;
		// Result
		return (float&)a;
	}

	//! Is the float valid ?
	bool IsNAN(float value)				{ return (IR(value)&0x7f800000) == 0x7f800000;	}
	bool IsIndeterminate(float value)	{ return IR(value) == 0xffc00000;				}
	bool IsPlusInf(float value)			{ return IR(value) == 0x7f800000;				}
	bool IsMinusInf(float value)		{ return IR(value) == 0xff800000;				}

	bool IsValidFloat(float value)
	{
		if(IsNAN(value))			return false;
		if(IsIndeterminate(value))	return false;
		if(IsPlusInf(value))		return false;
		if(IsMinusInf(value))		return false;
		return true;
	}

	#define CHECK_VALID_FLOAT(x)	ASSERT(IsValidFloat(x));

/*
	//! FPU precision setting function.
	inline_ void SetFPU()
	{
		// This function evaluates whether the floating-point
		// control word is set to single precision/round to nearest/
		// exceptions disabled. If these conditions don't hold, the
		// function changes the control word to set them and returns
		// TRUE, putting the old control word value in the passback
		// location pointed to by pwOldCW.
		{
			uword wTemp, wSave;
 
			__asm fstcw wSave
			if (wSave & 0x300 ||            // Not single mode
				0x3f != (wSave & 0x3f) ||   // Exceptions enabled
				wSave & 0xC00)              // Not round to nearest mode
			{
				__asm
				{
					mov ax, wSave
					and ax, not 300h    ;; single mode
					or  ax, 3fh         ;; disable all exceptions
					and ax, not 0xC00   ;; round to nearest mode
					mov wTemp, ax
					fldcw   wTemp
				}
			}
		}
	}
*/
	//! This function computes the slowest possible floating-point value (you can also directly use FLT_EPSILON)
	inline float ComputeFloatEpsilon()
	{
		float f = 1.0f;
		((udword&)f)^=1;
		return f - 1.0f;	// You can check it's the same as FLT_EPSILON
	}

	inline bool IsFloatZero(float x, float epsilon=1e-6f)
	{
		return x*x < epsilon;
	}

inline float FCMax2(float a, float b) { return a>b ? a : b; }
inline float FCMin2(float a, float b) { return a>b ? b : a; }
inline float FCMax3(float a, float b, float c)
{
    if (a >= b)
		return( FCMax2( a, c ) );
	
	return( FCMax2( b, c ) );
}
inline float FCMin3(float a, float b, float c)
{
    if (a >= b)
		return( FCMin2( b, c ) );
	
	return( FCMin2( a, c ) );
}

	inline int ConvertToSortable(float f)
	{
		int& Fi = (int&)f;
		int Fmask = (Fi>>31);
		Fi ^= Fmask;
		Fmask &= ~(1<<31);
		Fi -= Fmask;
		return Fi;
	}

	enum FPUMode
	{
		FPU_FLOOR		= 0,
		FPU_CEIL		= 1,
		FPU_BEST		= 2,

		FPU_FORCE_DWORD	= 0x7fffffff
	};

	FUNCTION ICECORE_API FPUMode	GetFPUMode();
	FUNCTION ICECORE_API void		SaveFPU();
	FUNCTION ICECORE_API void		RestoreFPU();
	FUNCTION ICECORE_API void		SetFPUFloorMode();
	FUNCTION ICECORE_API void		SetFPUCeilMode();
	FUNCTION ICECORE_API void		SetFPUBestMode();

	FUNCTION ICECORE_API void		SetFPUPrecision24();
	FUNCTION ICECORE_API void		SetFPUPrecision53();
	FUNCTION ICECORE_API void		SetFPUPrecision64();
	FUNCTION ICECORE_API void		SetFPURoundingChop();
	FUNCTION ICECORE_API void		SetFPURoundingUp();
	FUNCTION ICECORE_API void		SetFPURoundingDown();
	FUNCTION ICECORE_API void		SetFPURoundingNear();

	FUNCTION ICECORE_API int		intChop(const float& f);
	FUNCTION ICECORE_API int		intFloor(const float& f);
	FUNCTION ICECORE_API int		intCeil(const float& f);

#endif // __ICEFPU_H__
