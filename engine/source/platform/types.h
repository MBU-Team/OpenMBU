//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _TORQUE_TYPES_H_
#define _TORQUE_TYPES_H_


//------------------------------------------------------------------------------
//-------------------------------------- Basic Types...

typedef signed char        S8;      ///< Compiler independent Signed Char
typedef unsigned char      U8;      ///< Compiler independent Unsigned Char

typedef signed short       S16;     ///< Compiler independent Signed 16-bit short
typedef unsigned short     U16;     ///< Compiler independent Unsigned 16-bit short

typedef signed int         S32;     ///< Compiler independent Signed 32-bit integer
typedef unsigned int       U32;     ///< Compiler independent Unsigned 32-bit integer

typedef float              F32;     ///< Compiler independent 32-bit float
typedef double             F64;     ///< Compiler independent 64-bit float

struct EmptyType {};             ///< "Null" type used by templates


//------------------------------------------------------------------------------
//------------------------------------- String Types

typedef char           UTF8;        ///< Compiler independent 8  bit Unicode encoded character
#if defined(_MSC_VER)// && defined(__clang__)
typedef wchar_t        UTF16;       ///< Compiler independent 16 bit Unicode encoded character
#else
typedef unsigned short UTF16;       ///< Compiler independent 16 bit Unicode encoded character
#endif
typedef unsigned int   UTF32;       ///< Compiler independent 32 bit Unicode encoded character

typedef const char* StringTableEntry;

//------------------------------------------------------------------------------
//-------------------------------------- Type constants...
#define __EQUAL_CONST_F F32(0.000001)                             ///< Constant float epsilon used for F32 comparisons

static const F32 Float_One = F32(1.0);                           ///< Constant float 1.0
static const F32 Float_Half = F32(0.5);                           ///< Constant float 0.5
static const F32 Float_Zero = F32(0.0);                           ///< Constant float 0.0
static const F32 Float_Pi = F32(3.14159265358979323846);        ///< Constant float PI
static const F32 Float_2Pi = F32(2.0 * 3.14159265358979323846);  ///< Constant float 2*PI

static const S8  S8_MIN = S8(-128);                              ///< Constant Min Limit S8
static const S8  S8_MAX = S8(127);                               ///< Constant Max Limit S8
static const U8  U8_MAX = U8(255);                               ///< Constant Max Limit U8

static const S16 S16_MIN = S16(-32768);                           ///< Constant Min Limit S16
static const S16 S16_MAX = S16(32767);                            ///< Constant Max Limit S16
static const U16 U16_MAX = U16(65535);                            ///< Constant Max Limit U16

static const S32 S32_MIN = S32(-2147483647 - 1);                  ///< Constant Min Limit S32
static const S32 S32_MAX = S32(2147483647);                       ///< Constant Max Limit S32
static const U32 U32_MAX = U32(0xffffffff);                       ///< Constant Max Limit U32

static const F32 F32_MIN = F32(1.175494351e-38F);                 ///< Constant Min Limit F32
static const F32 F32_MAX = F32(3.402823466e+38F);                 ///< Constant Max Limit F32

//--------------------------------------
// Identify the compiler being used

// Metrowerks CodeWarrior
#if defined(__MWERKS__)
#  include "platform/types.codewarrior.h"
// Microsoft Visual C++/Visual.NET
#elif defined(_MSC_VER)
#  include "platform/types.visualc.h"
// GNU GCC
#elif defined(__GNUC__)
#  include "platform/types.gcc.h"
#else
#  error "Unknown Compiler"
#endif

//--------------------------------------
// Enable Asserts in all debug builds -- AFTER compiler types include.
//#if defined(TORQUE_DEBUG)
//#define TORQUE_ENABLE_ASSERTS
//#endif


//-------------------------------------- A couple of all-around useful inlines and
//                                        globals
//
U32 getNextPow2(U32 io_num);
U32 getBinLog2(U32 io_num);

/**
   Determines if the given U32 is some 2^n
   @param in_num Any U32
   @returns true if in_num is a power of two, otherwise false
 */
inline bool isPow2(const U32 in_num)
{
    return (in_num == getNextPow2(in_num));
}

inline U32 getFastBinLog2(const U32 value)
{
    const F32 floatValue = (const F32)value;
    return (*((U32*)&floatValue) >> 23) - 127;
}

inline U32 getFastBinLog2(const F32 value)
{
    return (*((U32*)&value) >> 23) - 127;
}

/**
   Convert the byte ordering on the U16 to and from big/little endian format.
   @param in_swap Any U16
   @returns swaped U16.
 */
inline U16 endianSwap(const U16 in_swap)
{
    return U16(((in_swap >> 8) & 0x00ff) |
        ((in_swap << 8) & 0xff00));
}

/**
   Convert the byte ordering on the U32 to and from big/little endian format.
   @param in_swap Any U32
   @returns swaped U32.
 */
inline U32 endianSwap(const U32 in_swap)
{
    return U32(((in_swap >> 24) & 0x000000ff) |
        ((in_swap >> 8) & 0x0000ff00) |
        ((in_swap << 8) & 0x00ff0000) |
        ((in_swap << 24) & 0xff000000));
}

//----------------Many versions of min and max-------------
//---not using template functions because MS VC++ chokes---

/// Returns the lesser of the two parameters: a & b.
inline U32 getMin(U32 a, U32 b)
{
    return a > b ? b : a;
}

/// Returns the lesser of the two parameters: a & b.
inline U16 getMin(U16 a, U16 b)
{
    return a > b ? b : a;
}

/// Returns the lesser of the two parameters: a & b.
inline U8 getMin(U8 a, U8 b)
{
    return a > b ? b : a;
}

/// Returns the lesser of the two parameters: a & b.
inline S32 getMin(S32 a, S32 b)
{
    return a > b ? b : a;
}

/// Returns the lesser of the two parameters: a & b.
inline S16 getMin(S16 a, S16 b)
{
    return a > b ? b : a;
}

/// Returns the lesser of the two parameters: a & b.
inline S8 getMin(S8 a, S8 b)
{
    return a > b ? b : a;
}

/// Returns the lesser of the two parameters: a & b.
inline float getMin(float a, float b)
{
    return a > b ? b : a;
}

/// Returns the lesser of the two parameters: a & b.
inline double getMin(double a, double b)
{
    return a > b ? b : a;
}

/// Returns the greater of the two parameters: a & b.
inline U32 getMax(U32 a, U32 b)
{
    return a > b ? a : b;
}

/// Returns the greater of the two parameters: a & b.
inline U16 getMax(U16 a, U16 b)
{
    return a > b ? a : b;
}

/// Returns the greater of the two parameters: a & b.
inline U8 getMax(U8 a, U8 b)
{
    return a > b ? a : b;
}

/// Returns the greater of the two parameters: a & b.
inline S32 getMax(S32 a, S32 b)
{
    return a > b ? a : b;
}

/// Returns the greater of the two parameters: a & b.
inline S16 getMax(S16 a, S16 b)
{
    return a > b ? a : b;
}

/// Returns the greater of the two parameters: a & b.
inline S8 getMax(S8 a, S8 b)
{
    return a > b ? a : b;
}

/// Returns the greater of the two parameters: a & b.
inline float getMax(float a, float b)
{
    return a > b ? a : b;
}

/// Returns the greater of the two parameters: a & b.
inline double getMax(double a, double b)
{
    return a > b ? a : b;
}

//-------------------------------------- Use this instead of Win32 FOURCC()
//                                        macro...
//
#define makeFourCCTag(ch0, ch1, ch2, ch3)    \
   (((U32(ch0) & 0xFF) << 0)  |             \
    ((U32(ch1) & 0xFF) << 8)  |             \
    ((U32(ch2) & 0xFF) << 16) |             \
    ((U32(ch3) & 0xFF) << 24) )

#define makeFourCCString(ch0, ch1, ch2, ch3) { ch0, ch1, ch2, ch3 }

#define BIT(x) (1 << (x))                       ///< Returns value with bit x set (2^x)

#ifndef Offset
/// Offset macro:
/// Calculates the location in memory of a given member x of class cls from the
/// start of the class.  Need several definitions to account for various
/// flavors of GCC.

// define all the variants of Offset that we might use
#define _Offset_Normal(x, cls) ((dsize_t)((const char *)&(((cls *)0)->x)-(const char *)0))
#define _Offset_Variant_1(x, cls) ((int)(&((cls *)1)->x) - 1)
#define _Offset_Variant_2(x, cls) offsetof(cls, x) // also requires #include <stddef.h>

// now, for each compiler type, define the Offset macros that should be used.
// The Engine code usually uses the Offset macro, but OffsetNonConst is needed
// when a variable is used in the indexing of the member field (see
// TSShapeConstructor::initPersistFields for an example)

// compiler is non-GCC, or gcc < 3
#if !defined(TORQUE_COMPILER_GCC) || (__GNUC__ < 3)
#define Offset(x, cls) _Offset_Normal(x, cls)
#define OffsetNonConst(x, cls) _Offset_Normal(x, cls)

// compiler is GCC 3 with minor version less than 4
#elif defined(TORQUE_COMPILER_GCC) && (__GNUC__ == 3) && (__GNUC_MINOR__ < 4)
#define Offset(x, cls) _Offset_Variant_1(x, cls)
#define OffsetNonConst(x, cls) _Offset_Variant_1(x, cls)

// compiler is GCC 3 with minor version greater than 4
#elif defined(TORQUE_COMPILER_GCC) && (__GNUC__ == 3) && (__GNUC_MINOR__ >= 4)
#include <stddef.h>
#define Offset(x, cls) _Offset_Variant_2(x, cls)
#define OffsetNonConst(x, cls) _Offset_Variant_1(x, cls)

// compiler is GCC 4
#elif defined(TORQUE_COMPILER_GCC) && (__GNUC__ == 4)
#include <stddef.h>
#define Offset(x, cls) _Offset_Variant_2(x, cls)
#define OffsetNonConst(x, cls) _Offset_Variant_1(x, cls)

#endif
#endif

#endif //_TORQUE_TYPES_H_
