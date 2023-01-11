//-----------------------------------------------------------------------------
// Torque Game Engine Advanced
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _UTIL_RETURNTYPE_H_
#define _UTIL_RETURNTYPE_H_

/// @file
///
/// Helper templates to determine the return type of functions.

template <class R> struct ReturnType { typedef void ValueType; };

template <class R,class A,class B,class C,class D,class E,class F,class G>
struct ReturnType<R (*)(A,B,C,D,E,F,G)> { typedef R ValueType; };
template <class R,class A,class B,class C,class D,class E,class F>
struct ReturnType<R (*)(A,B,C,D,E,F)> { typedef R ValueType; };
template <class R,class A,class B,class C,class D,class E>
struct ReturnType<R (*)(A,B,C,D,E)> { typedef R ValueType; };
template <class R,class A,class B,class C,class D>
struct ReturnType<R (*)(A,B,C,D)> { typedef R ValueType; };
template <class R,class A,class B,class C>
struct ReturnType<R (*)(A,B,C)> { typedef R ValueType; };
template <class R,class A,class B>
struct ReturnType<R (*)(A,B)> { typedef R ValueType; };
template <class R,class A>
struct ReturnType<R (*)(A)> { typedef R ValueType; };
template <class R>
struct ReturnType<R (*)()> { typedef R ValueType; };

template <class R,class O,class A,class B,class C,class D,class E,class F,class G>
struct ReturnType<R (O::*)(A,B,C,D,E,F,G)> { typedef R ValueType; };
template <class R,class O,class A,class B,class C,class D,class E,class F>
struct ReturnType<R (O::*)(A,B,C,D,E,F)> { typedef R ValueType; };
template <class R,class O,class A,class B,class C,class D,class E>
struct ReturnType<R (O::*)(A,B,C,D,E)> { typedef R ValueType; };
template <class R,class O,class A,class B,class C,class D>
struct ReturnType<R (O::*)(A,B,C,D)> { typedef R ValueType; };
template <class R,class O,class A,class B,class C>
struct ReturnType<R (O::*)(A,B,C)> { typedef R ValueType; };
template <class R,class O,class A,class B>
struct ReturnType<R (O::*)(A,B)> { typedef R ValueType; };
template <class R,class O,class A>
struct ReturnType<R (O::*)(A)> { typedef R ValueType; };
template <class R,class O>
struct ReturnType<R (O::*)()> { typedef R ValueType; };

#endif