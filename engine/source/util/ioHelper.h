//-----------------------------------------------------------------------------
// Torque Game Engine Advanced
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _UTIL_IOHELPER_H_
#define _UTIL_IOHELPER_H_

#ifndef _CORE_STREAM_H_
#include "core/stream.h"
#endif

/// Helper templates to aggregate IO operations - generally used in
/// template expansion.
namespace IOHelper
{
   template<class A,class B,class C,class D,class E,class F,class G,class H,class I,class J>
      inline bool reads(Stream *s,A& a,B& b,C& c,D& d,E& e,F& f,G& g,H& h,I& i,J& j)
   { s->read(&a); s->read(&b); s->read(&c); s->read(&d); s->read(&e); s->read(&f); s->read(&g); s->read(&h); s->read(&i); return s->read(&j); }

   template<class A,class B,class C,class D,class E,class F,class G,class H,class I>
      inline bool reads(Stream *s,A& a,B& b,C& c,D& d,E& e,F& f,G& g,H& h,I& i)
   { s->read(&a); s->read(&b); s->read(&c); s->read(&d); s->read(&e); s->read(&f); s->read(&g); s->read(&h); return s->read(&i); }

   template<class A,class B,class C,class D,class E,class F,class G,class H>
      inline bool reads(Stream *s,A& a,B& b,C& c,D& d,E& e,F& f,G& g,H& h)
   { s->read(&a); s->read(&b); s->read(&c); s->read(&d); s->read(&e); s->read(&f); s->read(&g); return s->read(&h); }

   template<class A,class B,class C,class D,class E,class F,class G>
      inline bool reads(Stream *s,A& a,B& b,C& c,D& d,E& e,F& f,G& g)
   { s->read(&a); s->read(&b); s->read(&c); s->read(&d); s->read(&e); s->read(&f); return s->read(&g); }

   template<class A,class B,class C,class D,class E,class F>
      inline bool reads(Stream *s,A& a,B& b,C& c,D& d,E& e,F& f)
   { s->read(&a); s->read(&b); s->read(&c); s->read(&d); s->read(&e); return s->read(&f); }

   template<class A,class B,class C,class D,class E>
      inline bool reads(Stream *s,A& a,B& b,C& c,D& d,E& e)
   { s->read(&a); s->read(&b); s->read(&c); s->read(&d); return s->read(&e); }

   template<class A,class B,class C,class D>
      inline bool reads(Stream *s,A& a,B& b,C& c,D& d)
   { s->read(&a); s->read(&b); s->read(&c); return s->read(&d); }

   template<class A,class B,class C>
      inline bool reads(Stream *s,A& a,B& b,C& c)
   { s->read(&a); s->read(&b); return s->read(&c); }

   template<class A,class B>
      inline bool reads(Stream *s,A& a,B& b)
   { s->read(&a); return s->read(&b); }

   template<class A>
      inline bool reads(Stream *s,A& a)
   { return s->read(&a); }

   template<class A,class B,class C,class D,class E,class F,class G,class H,class I,class J>
      inline bool writes(Stream *s,A& a,B& b,C& c,D& d,E& e,F& f,G& g,H& h,I& i,J& j)
   { s->write(a); s->write(b); s->write(c); s->write(d); s->write(e); s->write(f); s->write(g); s->write(h); s->write(i); return s->write(j); }

   template<class A,class B,class C,class D,class E,class F,class G,class H,class I>
      inline bool writes(Stream *s,A& a,B& b,C& c,D& d,E& e,F& f,G& g,H& h,I& i)
   { s->write(a); s->write(b); s->write(c); s->write(d); s->write(e); s->write(f); s->write(g); s->write(h); return s->write(i); }

   template<class A,class B,class C,class D,class E,class F,class G,class H>
      inline bool writes(Stream *s,A& a,B& b,C& c,D& d,E& e,F& f,G& g,H& h)
   { s->write(a); s->write(b); s->write(c); s->write(d); s->write(e); s->write(f); s->write(g); return s->write(h); }

   template<class A,class B,class C,class D,class E,class F,class G>
      inline bool writes(Stream *s,A& a,B& b,C& c,D& d,E& e,F& f,G& g)
   { s->write(a); s->write(b); s->write(c); s->write(d); s->write(e); s->write(f); return s->write(g); }

   template<class A,class B,class C,class D,class E,class F>
      inline bool writes(Stream *s,A& a,B& b,C& c,D& d,E& e,F& f)
   { s->write(a); s->write(b); s->write(c); s->write(d); s->write(e); return s->write(f); }

   template<class A,class B,class C,class D,class E>
      inline bool writes(Stream *s,A& a,B& b,C& c,D& d,E& e)
   { s->write(a); s->write(b); s->write(c); s->write(d); return s->write(e); }

   template<class A,class B,class C,class D>
      inline bool writes(Stream *s,A& a,B& b,C& c,D& d)
   { s->write(a); s->write(b); s->write(c); return s->write(d); }

   template<class A,class B,class C>
      inline bool writes(Stream *s,A& a,B& b,C& c)
   { s->write(a); s->write(b); return s->write(c); }

   template<class A,class B>
      inline bool writes(Stream *s,A& a,B& b)
   { s->write(a); return s->write(b); }

   template<class A>
      inline bool writes(Stream *s,A& a)
   { return s->write(a); }
}

#endif