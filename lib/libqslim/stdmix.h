#ifndef STDMIX_INCLUDED // -*- C++ -*-
#define STDMIX_INCLUDED
#if !defined(__GNUC__)
#  pragma once
#endif

/************************************************************************

  Standard base include file for the MIX library.  This defines various
  common stuff that is used elsewhere.

  Copyright (C) 1998 Michael Garland.  See "COPYING.txt" for details.

  $Id: stdmix.h,v 1.2 2006/09/07 00:05:52 bramage Exp $

 ************************************************************************/

//#include <gfx/gfx.h>
//using namespace std;
#include <string.h>

#ifdef UINT_MAX
#  define MXID_NIL UINT_MAX
#else
#  define MXID_NIL 0xffffffffU
#endif

#if !defined(HAVE_UINT) && !defined(uint)
typedef unsigned int uint;
#endif

#if !defined(HAVE_USHORT) && !defined(ushort)
typedef unsigned short ushort;
#endif

#ifdef MIN
#undef MIN
#endif
#ifdef MAX
#undef MAX
#endif

#define MIN(a,b) (((a)>(b))?(b):(a))
#define MAX(a,b) (((a)>(b))?(a):(b))

#ifndef ABS
#  define ABS(x) (((x)<0)?-(x):(x))
#endif

#ifndef MIX_NO_AXIS_NAMES
enum Axis {X=0, Y=1, Z=2, W=3};
#endif

inline bool streq(const char *a, const char *b) { return !strcmp(a,b); }

////////////////////////////////////////////////////////////////////////
//
// Optimization control, debugging, and error reporting facilities
//

//
// Safety levels:
//
//       -2 Reckless
//       -1 Optimized
//        0 Normal
//        1 Cautious
//        2 Paranoid
//
#ifndef SAFETY
// Default safety policy is to take the middle of the road
#define SAFETY 0
#endif

#include "mixmsg.h"

#define fatal_error(s) mxmsg_signal(MXMSG_FATAL, s, NULL, __FILE__, __LINE__)

#ifdef assert
#  undef assert
#endif

#if SAFETY >= 0
#  define assert(i) (i)?((void)NULL):mxmsg_signal(MXMSG_ASSERT, # i, \
						  NULL, __FILE__, __LINE__)
#  define CAREFUL(x) x
#else
#  define assert(i)
#  define CAREFUL(x)
#endif

#if SAFETY==2
#  define SanityCheck(t) assert(t)
#  define PARANOID(x) x
#else
#  define SanityCheck(t)
#  define PARANOID(x)
#endif

#if SAFETY > 0
#  define AssertBound(t)  assert(t)
#  define PRECAUTION(x) x
#else
#  define AssertBound(t)
#  define PRECAUTION(x)
#endif

// STDMIX_INCLUDED
#endif
