/**********************************************************************
 *<
	FILE: gfloat

	DESCRIPTION: Single Precision Floating Point Routines

	CREATED BY: Don Brittain & Dan Silva

	HISTORY:

 *>	Copyright (c) 1994, All Rights Reserved.
 **********************************************************************/

#ifndef _GFLOAT_H 

#define _GFLOAT_H


//-------------------------------------------------------------------------------
// Single precision floating point stuff...
//
inline void SinCos (float angle, float *sine, float *cosine) 
{
#ifdef _M_IX86	// if on Intel, use assembly language
	__asm {
		push		ecx
        fld         dword ptr angle
        fsincos
		mov 		ecx, dword ptr[cosine]
        fstp        dword ptr [ecx]
		mov 		ecx, dword ptr[sine]
        fstp        dword ptr [ecx]
		pop			ecx
    }
#else
    *sine   = (float)sin (angle);
    *cosine = (float)cos (angle);
#endif
}

inline float Sin(float angle)
{
#ifdef _M_IX86
	float s, c;
	SinCos(angle, &s, &c);
	return s;
#else
	return (float)sin((double)angle);
#endif
}

inline float Cos(float angle)
{
#ifdef _M_IX86
	float s, c;
	SinCos(angle, &s, &c);
	return c;
#else
	return (float)cos((double)angle);
#endif
}

inline float Sqrt(float arg)
{
#ifdef _M_IX86
	float ans;
    __asm {
        fld         dword ptr arg
        fsqrt
        fstp        dword ptr [ans]
    	}
	return ans;
#else
	return (float)sqrt((double)arg);
#endif
}


#endif