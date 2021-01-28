#ifndef __DTSVECTOR_H
#define __DTSVECTOR_H

#include <vector>
#include <string>
#include <ostream>
#include <cmath>

namespace DTS
{

   //! Defines a generic vector class

   template <class type, int size> class Vector
   {
      public:
      
         //! Create a null vector
         Vector () {
            // No initialization. Derivated classes have no overhead.
         }

         //! Create a vector from a data array
         Vector (const type *d) {
            for (int n = 0 ; n < size ; n++) members[n] = d[n] ;
         }
         
         //! Create a point with given space coordinates
         Vector<type, size> (const Vector<type, size> &v) {
            for (int n = 0 ; n < size ; n++) members[n] = v.members[n] ;
         }
         
         //! Member access
         const type & get (int n) const         { return members[n] ; }
         type & operator[] (int n)              { return members[n] ; }
         const type & operator[] (int n) const  { return members[n] ; }
         void set (int n, type value)           { members[n] = value ; }

         //!{ Member operators *************************************************

         Vector<type, size> operator - () const {
            Vector<type, size> r ;
            for (int n = 0 ; n < size ; n++) r.members[n] = -members[n]; 
            return r ;
         }
         Vector<type, size> & operator = (const Vector<type, size> &v) {
            for (int n = 0 ; n < size ; n++) members[n] = v.members[n] ;
            return *this ;
         }
         Vector<type, size> & operator += (const Vector<type, size> &r) {
            for (int n = 0 ; n < size ; n++) members[n] += r.members[n]; 
            return *this ;
         }
         Vector<type, size> & operator -= (const Vector<type, size> &r) {
            for (int n = 0 ; n < size ; n++) members[n] -= r.members[n]; 
            return *this ;
         }
         Vector<type, size> & operator *= (const type r) {
            for (int n = 0 ; n < size ; n++) members[n] *= r; 
            return *this ;
         }
         Vector<type, size> & operator /= (const type r) {
            for (int n = 0 ; n < size ; n++) members[n] /= r; 
            return *this ;
         }
         //!}
         //!{ Non-member operators *********************************************

         friend bool operator <  (const Vector<type,size> &a, const Vector<type,size> &b) {
            for (int n = 0 ; n < size ; n++) if (a.members[n] < b.members[n]) return true ;
            return false ;
         }
         friend bool operator >  (const Vector<type,size> &a, const Vector<type,size> &b) {
            for (int n = 0 ; n < size ; n++) if (a.members[n] < b.members[n]) return false ;
            return a[n] != b[n] ;
         }
         friend bool operator == (const Vector<type,size> &a, const Vector<type,size> &b) {
            for (int n = 0 ; n < size ; n++) if (a.members[n] != b.members[n]) return false ;
            return true ; 
         }
         friend bool operator != (const Vector<type,size> &a, const Vector<type,size> &b) {
            for (int n = 0 ; n < size ; n++) if (a.members[n] != b.members[n]) return true ;
            return false ; 
         }
         //!}
         //!{ Vector utilities *************************************************

         //! Normalize the vector (reduce it to 1 unit in length)
         void normalize() ;
         
         //! Length of the vector
         type length(int nc = size) const ;
         
         //! Dot product
         type dot (const Vector<type, size> &r) const ;
         
         //! Angle in radians between two vectors
         type angle (const Vector<type, size> &r) const ;
         
         //! Middle point between two points/vectors
         Vector<type, size> midpoint (const Vector<type, size> & p) const ;

         //! Some sintactic sugar stuff
         bool isPerpendicular (Vector<type, size> &r) const { return dot(r) == 0.0f ; }

         //! Returns true if all members are 0 (do not use operator (bool))
         bool isNull() const ;
         

      private:
         
         type members[size] ;
   };
   
   // -----------------------------------------------------------------------
   //  Implementation
   // -----------------------------------------------------------------------

   template <class type, int size>
   inline Vector<type, size> operator + (const Vector<type, size> &a, const Vector<type, size> &b) 
   { 
      Vector<type, size> result ;
      for (int n = 0 ; n < size ; n++)
         result.set (n, a.get(n) + b.get(n)) ;
      return result ; 
   }
   
   template <class type, int size>
   inline Vector<type, size> operator - (const Vector<type, size> &a, const Vector<type, size> &b) 
   { 
      Vector<type, size> result ;
      for (int n = 0 ; n < size ; n++)
         result.set (n, a.get(n) - b.get(n)) ;
      return result ; 
   }
   
   template <class type, int size>
   inline bool operator == (const Vector<type, size> &a, const Vector<type, size> &b) 
   { 
      for (int n = 0 ; n < size ; n++)
         if (a.members[n] != b.members[n]) return false ;
      return true ; 
   }
   
   template <class type, int size>
   inline bool operator != (const Vector<type, size> &a, const Vector<type, size> &b) 
   { 
      for (int n = 0 ; n < size ; n++)
         if (a.members[n] != b.members[n]) return true ;
      return false ; 
   }
   
   //! Multiply by constant

   template <class type, int size>
   inline Vector<type, size> operator * (const Vector<type, size> &p, float c)
   {
      Vector<type, size> result ;
      for (int n = 0 ; n < size ; n++)
         result.set (n, p.get(n) * c) ;
      return result ;
   }

   //! Divide by constant

   template <class type, int size>
   inline Vector<type, size> operator / (const Vector<type, size> &p, float c)
   {
      Vector<type, size>  result ;
      for (int n = 0 ; n < size ; n++)
         result.set (n, p.get(n) / c) ;
      return result ;
   }

   //! Dot product

   template <class type, int size>
   inline type Vector<type, size>::dot (const Vector<type, size> &r) const
   {
      type result = r.members[0] * members[0] ; ;
      for (int n = 1 ; n < size ; n++)
         result += r.members[n] * members[n] ;
      return result ;
   }

   //! Length

   template <class type, int size>
   inline type Vector<type, size>::length(int nc) const
   {
      type result = members[0] * members[0] ; ;
      for (int n = 1 ; n < nc ; n++)
         result += members[n] * members[n] ;
      return sqrt(result) ;
   }

   //! Angle between two vectors

   template <class type, int size>
   inline type Vector<type, size>::angle (const Vector<type, size> &r) const
   {
      return acos (dot(r) / length() / r.length()) ;
   }

   //! Angle between two vectors

   template <class type, int size>
   inline Vector<type,size> Vector<type, size>::midpoint (const Vector<type, size> &r) const
   {
      Vector<type,size> result ;
      for (int n = 0 ; n < size ; n++)
         result.set (n, (r.members[n] - members[n])/2 + members[n]) ;
      return result ;
   }

   //! Normalices a vector. If it is a null vector (0, 0, 0) then it is left untouched

   template <class type, int size>
   inline void Vector<type, size>::normalize()
   {
      float dist = length() ; ;

      if (dist != 0.0f)
      {
         for (int n = 0 ; n < size ; n++)
            members[n] /= dist ;
      }
   }

   template <class type, int size>
   inline bool Vector<type,size>::isNull() const
   {
      for (int n = 0 ; n < size ; n++)
         if (members[n] != 0) return false ;
      return true ;
   }
}

#endif