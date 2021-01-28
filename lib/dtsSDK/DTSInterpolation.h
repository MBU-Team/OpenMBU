#ifndef __DTS_INTERPOLATION_H
#define __DTS_INTERPOLATION_H

#include <cmath>

namespace DTS
{
   //! Not really a class, but a bunch of static interpolation functions
   
   struct Interpolation
   {
      template <class type>
      static inline type linear (const type &a, const type &b, float s)
      {
         return s*a + (1-s)*b ;
      }

      template <class type>
      static inline type quadratic (const type &a, const type &b, float s)
      {
         return sqrt( s*a*a + (1-s)*b*b );
      }

      template <class type>
      static inline type spherical (const type &a, const type &b, float s)
      {
         return exp( s*ln(a) + (1-s)*ln(b) );
      }

      template <class type>
      static inline type bezier (const type &a, const type &b, const type &ac, const type &bc, float s)
      {
         return (-1*s*s*s +3*s*s -3*s +1) * a
	      + (+3*s*s*s -6*s*s +3*s   ) * b
	      + (-3*s*s*s +3*s*s        ) * ac
	      + (+1*s*s*s               ) * bc ;
      }

      template <class type>
      static inline type hermite (const type &a, const type &b, const type &at, const type &bt, float s)
      {
         return (+2*s*s*s -3*s*s    +1) * a
	      + (-2*s*s*s +3*s*s      ) * b
	      + (+1*s*s*s -2*s*s +s   ) * at
	      + (+1*s*s*s -1*s*s      ) * bt ;
      }
   };
}

#endif