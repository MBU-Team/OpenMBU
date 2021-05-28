//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _TYPESX86UNIX_H_
#define _TYPESX86UNIX_H_

/* eek. */
#ifndef NULL
#define NULL 0
#endif

#define PLATFORM_LITTLE_ENDIAN

#define FN_CDECL

typedef signed char     	S8;
typedef unsigned char   	U8;

typedef signed short    	S16;
typedef unsigned short  	U16;

typedef signed int      	S32;
typedef unsigned int    	U32;

typedef signed long long        S64;
typedef unsigned long long      U64;

typedef float           	F32;
typedef double          	F64;

typedef unsigned int dsize_t;

typedef const char* StringTableEntry;

typedef S32 FileTime;

#define __EQUAL_CONST_F F32(0.000001)

static const F32 Float_One  = F32(1.0);
static const F32 Float_Half = F32(0.5);
static const F32 Float_Zero = F32(0.0);
static const F32 Float_Pi   = F32(3.14159265358979323846);
static const F32 Float_2Pi  = F32(2.0 * 3.14159265358979323846);

static const S8  S8_MIN  = S8(-128);
static const S8  S8_MAX  = S8(127);
static const U8  U8_MAX  = U8(255);

static const S16 S16_MIN = S16(-32768);
static const S16 S16_MAX = S16(32767);
static const U16 U16_MAX = U16(65535);

static const S32 S32_MIN = S32(-2147483647 - 1);
static const S32 S32_MAX = S32(2147483647);
static const U32 U32_MAX = U32(0xffffffff);

static const F32 F32_MAX = F32(3.402823466e+38F);
static const F32 F32_MIN = F32(1.175494351e-38F);


#endif
