#ifndef MIXVOPS_INCLUDED // -*- C++ -*-
#define MIXVOPS_INCLUDED //
#endif                   // WARNING:  Multiple inclusions allowed

/************************************************************************

  Low-level vector math operations.

  This header file is intended for internal library use only.  You should
  understand what's going on in here before trying to use it yourself.

  Using the magic __LINKAGE macro, this file can be compiled either as a
  set of templated functions, or as a series of inline functions whose
  argument type is determined by the __T macro.  My rationale for this is
  that some compilers either don't support templated functions or are
  incapable of inlining them.

  PUBLIC APOLOGY: I really do apologize for the ugliness of the code in
  this header.  In order to accommodate various types of compilation and
  to avoid lots of typing, I've made pretty heavy use of macros.

  Copyright (C) 1998 Michael Garland.  See "COPYING.txt" for details.
  
  $Id: mixvops.h,v 1.2 2006/09/07 00:05:52 bramage Exp $

 ************************************************************************/

#ifdef MIXVOPS_DEFAULT_DIM
#  define __DIM const int N=MIXVOPS_DEFAULT_DIM
#else
#  define __DIM const int N
#endif

#if !defined(MIXVOPS_USE_TEMPLATES)
#  ifndef __T
#    define __T double
#  endif
#  define __LINKAGE inline
#else
#  ifdef __T
#    undef __T
#  endif
#  define __LINKAGE template<class __T>inline
#endif

#define __OP __LINKAGE __T *

#define forall(i, N) for(unsigned int i=0; i<N; i++)

#define def3(name, op) __OP name(__T *r, const __T *u, const __T *v, __DIM) { forall(i,N) op; return r; }

#define def2(name, op) __OP name(__T *r, const __T *u, __DIM) { forall(i,N) op; return r; }

#define def1(name, op) __OP name(__T *r, __DIM) { forall(i,N) op; return r; }

#define def2s(name, op) __OP name(__T *r, const __T *u, __T d, __DIM) { forall(i,N) op; return r; }

#define def1s(name, op) __OP name(__T *r, __T d, __DIM) { forall(i,N) op; return r; }

def3(mxv_add, r[i] = u[i] + v[i])
def3(mxv_sub, r[i] = u[i] - v[i])
def3(mxv_mul, r[i] = u[i] * v[i])
def3(mxv_div, r[i] = u[i] / v[i])
def3(mxv_min, r[i] = u[i]<v[i]?u[i]:v[i];)
def3(mxv_max, r[i] = u[i]>v[i]?u[i]:v[i];)

def2(mxv_addinto, r[i] += u[i])
def2(mxv_subfrom, r[i] -= u[i])
def2(mxv_mulby, r[i] *= u[i])
def2(mxv_divby, r[i] /= u[i])

def2(mxv_set, r[i] = u[i])
def1(mxv_neg, r[i] = -r[i])
def2(mxv_neg, r[i] = -u[i])


def1s(mxv_set, r[i]=d)
def1s(mxv_scale, r[i] *= d)
def1s(mxv_invscale, r[i] /= d)
def2s(mxv_scale, r[i] = u[i]*d)
def2s(mxv_invscale, r[i] = u[i]/d)


__LINKAGE __T mxv_dot(const __T *u, const __T *v, __DIM)
{
    __T dot=0.0;  forall(i, N) dot+=u[i]*v[i];  return dot;
}

__OP mxv_cross3(__T *r, const __T *u, const __T *v)
{
    r[0] = u[1]*v[2] - v[1]*u[2]; r[1] = -u[0]*v[2] + v[0]*u[2];
    r[2] = u[0]*v[1] - v[0]*u[1];  return r;
}

__OP mxv_lerp(__T *r, const __T *u, const __T *v, __T t, __DIM)
{
    forall(i, N) { r[i] = t*u[i] + (1-t)*v[i]; }  return r;
}

__LINKAGE __T mxv_norm(const __T *v, __DIM) { return sqrt(mxv_dot(v,v,N)); }
__LINKAGE __T mxv_norm2(const __T *v, __DIM) { return mxv_dot(v,v,N); }

__LINKAGE __T mxv_unitize(__T *v, __DIM)
{
    __T l = mxv_norm2(v, N);
    if( l!=1.0 && l!=0.0 )  { l = sqrt(l);  mxv_invscale(v, l, N); }
    return l;
}

__LINKAGE __T mxv_Linf(const __T *u, const __T *v, __DIM)
{
    __T d=0.0;  forall(i, N) { __T p=ABS(u[i]-v[i]); d=MIN(d, p); }  return d;
}

__LINKAGE __T mxv_L1(const __T *u, const __T *v, __DIM)
{
    __T d = 0.0;  forall(i, N) d+=ABS(u[i]-v[i]);  return d;
}

__LINKAGE __T mxv_L2(const __T *u, const __T *v, __DIM)
{
    __T d = 0.0;  forall(i, N) { __T p=u[i]-v[i];  d+=p*p; }  return d;
}

__LINKAGE bool mxv_eql(const __T *u, const __T *v, __DIM)
{
    bool e=true;
    for(uint i=0; e && i<N; i++) e = e && (u[i]==v[i]);
    return e;
}

__LINKAGE bool mxv_equal(const __T *u, const __T *v, __DIM)
{
    return mxv_L2(u, v, N) < FEQ_EPS2;
}

__OP mxv_basis(__T *r, uint b, __DIM)
{
    forall(i, N)  r[i] = (i==b)?1.0:0.0;  return r;
}

#ifndef MIXVOPS_NO_IOSTREAMS
__LINKAGE ostream& mxv_write(ostream& out, const __T *v, __DIM)
{
    out << v[0];
    for(unsigned int i=1; i<N; i++)   out << " " << v[i];
    return out;
}
__LINKAGE ostream& mxv_write(const __T *v, __DIM)
	{return mxv_write(cout, v, N);}

__LINKAGE istream& mxv_read(istream& in, __T *v, __DIM)
{
    forall(i,N) in >> v[i];  return in;
}
__LINKAGE istream& mxv_read(__T *v, __DIM) { return mxv_read(cin, v, N); }
#endif

#ifndef mxv_local_block
#ifdef __GNUC__
#  define mxv_local_block(a,T,N)  T a[N]
#  define mxv_free_local(a)
#else
#  ifdef HAVE_ALLOCA
#    include <alloca.h>
#    define mxv_local_block(a,T,N)  T *a = (T *)alloca(sizeof(T)*N)
#    define mxv_free_local(a)
#  else
#    define mxv_local_block(a,T,N)  T *a = new T[N]
#    define mxv_free_local(a)       delete[] a
#  endif
#endif
#endif

#undef __T
#undef __DIM
#undef __LINKAGE
#undef __OP
#undef forall
#undef def3
#undef def2
#undef def1
#undef def2s
#undef def1s

// MIXVOPS_INCLUDED
