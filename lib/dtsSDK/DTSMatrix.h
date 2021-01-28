#ifndef __DTSMATRIX_H
#define __DTSMATRIX_H

#include <iomanip>

#include <vector>
#include <string>
#include <ostream>
#include <cmath>
#include <cassert>

#include "DTSVector.h"
#include "DTSPoint.h"

namespace DTS
{
    //! Defines matrices

   template <int rows=4, int cols=4, class type=float>
   class Matrix : public Vector<Vector<type,cols>, rows>
   {
      public:

         typedef Vector<type,cols> Row ;
         typedef type Cell ;

         Matrix() { /* No initialization */ }

         Vector<type,rows> col (int n) {
            assert (n >= 0 && n < rows) ;

            Vector<type,rows> result ;
            for (int r = 0 ; r < rows ; r++)
               result[r] = (*this)[r][n] ;
            return result ;
         }

         void setCol (int n,Vector<type,rows> col) {
            assert (n >= 0 && n < rows) ;

            for (int r = 0 ; r < rows ; r++)
               (*this)[r][n] = col[r];
         }

         Matrix(const type *p) {
            for (int r = 0 ; r < rows ; r++)
               for (int c = 0 ; c < cols ; c++)
                  (*this)[r][c] = *p++ ;
         }

         inline static Matrix<rows,cols,type> identity() {            
            Matrix <rows,cols,type> result ;
            for (int r = 0 ; r < rows ; r++)
               for (int c = 0 ; c < cols ; c++)
                  result[r][c] = (r == c ? 1:0) ;
            return result ;
         }

         Matrix <rows, cols, type> transpose() const
         {
            Matrix <rows,cols,type> result ;
            for (int r = 0 ; r < rows ; r++)
               for (int c = 0 ; c < cols ; c++)
                  result[c][r] = (*this)[r][c] ;
            return result ;
         }

         type determinant() const ;

         Matrix <rows, cols, type> inverse() const ;

         //!{ Matrix operators

         Matrix<rows,cols,type> & operator = (const Matrix<rows,cols,type> &a) {
            for (int r = 0 ; r < rows ; r++)
               for (int c = 0 ; c < cols ; c++)
                  (*this)[r][c] = a[r][c] ;
            return *this ;
         }

         template <int rows2, int cols2>
            Matrix<rows,cols2,type>
            operator * (const Matrix<rows2,cols2,type> &b)
         {
            Matrix<rows,cols2,type> result ;
            
            assert (rows2 == cols) ;
            for (int r = 0 ; r < rows ; r++)
            {
               for (int c = 0 ; c < cols2 ; c++)
               {
                  type cell = (*this)[r][0] * b[0][c]  ;
                  for (int e = 1 ; e < cols ; e++)
                     cell += (*this)[r][e] * b[e][c] ;
                  result[r][c] = cell ;
               }
            }
            return result ;
         }

         Vector<type,rows> operator * (const Vector<type,rows> &v)
         {
            Vector<type,rows> result ;
            for (int r = 0 ; r < rows ; r++)
            {
               type cell = (*this)[r][0] * v[0]  ;
               for (int e = 1 ; e < cols ; e++)
                  cell += (*this)[r][e] * v[e] ;
               result[r] = cell ;
            }
            return result ;
         }

         Matrix<rows,cols,type> operator *= (const Matrix<rows,cols,type> &a)
         { 
            (*this) = (*this) * a ;
         }

         Matrix<rows,cols,type> operator + (const Matrix<rows,cols,type> &b)
         {
            Matrix<rows,cols2,type> result ;
            for (int r = 0 ; r < rows ; r++)
               for (int c = 0 ; c < cols2 ; c++)
                  result[r][c] = (*this)[r][c] + b[r][c] ;
            return result ;
         }

         Matrix<rows,cols,type> operator += (const Matrix<rows,cols,type> &b)
         {
            Matrix<rows,cols2,type> result ;
            for (int r = 0 ; r < rows ; r++)
               for (int c = 0 ; c < cols2 ; c++)
                  (*this)[r][c] += b[r][c] ;
            return result ;
         }

         Matrix<rows,cols,type> operator - (const Matrix<rows,cols,type> &b)
         {
            Matrix<rows,cols2,type> result ;
            for (int r = 0 ; r < rows ; r++)
               for (int c = 0 ; c < cols2 ; c++)
                  result[r][c] = (*this)[r][c] - b[r][c] ;
            return result ;
         }

         Matrix<rows,cols,type> operator -= (const Matrix<rows,cols,type> &b)
         {
            Matrix<rows,cols2,type> result ;
            for (int r = 0 ; r < rows ; r++)
               for (int c = 0 ; c < cols2 ; c++)
                  (*this)[r][c] -= b[r][c] ;
            return result ;
         }

         //!}
         
         inline static Matrix<4,4> rotationX(float angle)
         {
            float cells[] = {
                  1,           0,            0,                 0,
                  0,           cosf(angle), -sinf(angle),       0,
                  0,           sinf(angle),  cosf(angle),       0,
                  0,           0,            0,                 1
            } ;
            return Matrix<4,4> (cells) ;
         }
         
         inline static Matrix<4,4> rotationZ(float angle)
         {
            float cells[] = {
                  cosf(angle), -sinf(angle),  0,                0,
                  sinf(angle),  cosf(angle),  0,                0,
                  0,            0,            1,                0,
                  0,            0,            0,                1
            } ;
            return Matrix<4,4> (cells) ;
         }
         
         inline static Matrix<4,4> rotationY(float angle)
         {
            float cells[] = {
                  cosf(angle),  0,          -sinf(angle),       0,
                  0,            1,           0,                 0,
                  sinf(angle),  0,           cosf(angle),       0,
                  0,            0,           0,                 1
            } ;
            return Matrix<4,4> (cells) ;
         }
   };

   template <int rows, int cols, class type>
   type Matrix<rows,cols,type>::determinant() const
   {
      int    r, c, f ;
      type   result = 0 ;
      const Matrix<rows,cols,type> &m = *this ;

      assert (rows == cols) ;
      switch (rows)
      {
         case 1:
            return m[0][0] ;
         case 2:
            return m[0][0]*m[1][1] - m[0][1]*m[1][0] ;
         case 3:
            return m[0][0] * ( m[1][1]*m[2][2] - m[1][2]*m[2][1] )
                 - m[0][1] * ( m[1][0]*m[2][2] - m[1][2]*m[2][0] )
                 + m[0][2] * ( m[1][0]*m[2][1] - m[1][1]*m[2][0] ) ;

#define GREATER_ORDER(o) \
         case o:                                         \
            for (f = 0 ; f < o ; f++)                    \
            {                                            \
               type data[(o-1)*(o-1)], *ptr = data ;     \
               for (r = 0 ; r < o ; r++)                 \
               {                                         \
                  if (r == f) continue ;                 \
                  for (c = 0 ; c < o ; c++)              \
                  {                                      \
                     if (c == f) continue ;              \
                     *ptr++ = m[r][c] ;                  \
                  }                                      \
               }                                         \
               result += Matrix<o-1,o-1,type>(data)      \
                  .determinant() * ((f&1) ? -1:1) ;      \
            }                                            \
            return result ;                              

         GREATER_ORDER(4)
         GREATER_ORDER(5)
         GREATER_ORDER(6)
         GREATER_ORDER(7)
         GREATER_ORDER(8)
         GREATER_ORDER(9)

#undef GREATER_ORDER

         default:
            return 0 ;
      }
   }


   template <int rows, int cols, class type>
   Matrix<rows,cols,type> Matrix<rows,cols,type>::inverse() const
   {
      int  r, c, r2, c2 ;
      type det = determinant() ;
      type p[rows][cols] ;

      const Matrix<rows,cols,type> &m = *this ;

      assert (rows == cols) ;
      assert (det  != 0) ;

      switch (rows)
      {
         case 1:
            p[0][0] = m[0][0]/det ;
            break ;
         case 2:
            p[0][1] = -m[0][1] / det ;
            p[1][0] = -m[1][0] / det ;
            p[1][1] =  m[0][0] / det ;
            break ;
         case 3:
            p[0][0] =  ( m[1][1]*m[2][2] - m[1][2]*m[2][1] ) / det ;
            p[0][1] =  ( m[1][0]*m[2][2] - m[1][2]*m[2][0] ) / det ;
            p[0][2] =  ( m[1][0]*m[2][1] - m[1][1]*m[2][0] ) / det ;
            p[1][0] =  ( m[0][1]*m[2][2] - m[0][2]*m[2][1] ) / det ;
            p[1][1] =  ( m[0][0]*m[2][2] - m[0][2]*m[2][0] ) / det ;
            p[1][2] =  ( m[0][0]*m[2][1] - m[0][1]*m[2][0] ) / det ;
            p[2][0] =  ( m[0][1]*m[1][2] - m[0][2]*m[1][1] ) / det ;
            p[2][1] =  ( m[0][0]*m[1][2] - m[0][2]*m[1][0] ) / det ;
            p[2][2] =  ( m[0][0]*m[1][1] - m[0][1]*m[1][0] ) / det ;
            break ;

#define GREATER_ORDER(o)                                 \
         case o:                                         \
            for (r = 0 ; r < o ; r++)                    \
            {                                            \
               for (c = 0 ; c < o ; c++)                 \
               {                                         \
                  type data[o*o], *ptr = data ;          \
                  for (r2 = 0 ; r2 < o ; r2++)           \
                  {                                      \
                     if (r2 == r) continue ;             \
                     for (c2 = 0 ; c < o ; c2++)         \
                     {                                   \
                        if (c2 == c) continue ;          \
                        *ptr++ = m[r2][c2] ;             \
                     }                                   \
                  }                                      \
                  type sign = 1-((r+c)%2)*2 ;            \
                  p[r][c] = Matrix<o-1,o-1,type>(data)   \
                     .determinant() * sign / det ;       \
               }                                         \
            }                                            \
            break ;

         GREATER_ORDER(4)
         GREATER_ORDER(5)
         GREATER_ORDER(6)
         GREATER_ORDER(7)
         GREATER_ORDER(8)
         GREATER_ORDER(9)

#undef GREATER_ORDER

         default:
            return 0 ;
      }

      return Matrix<rows,cols,type> (&p[0][0]) ;
   }

   template <int rows, int cols, class type>
   std::ostream & operator << (std::ostream &out, const Matrix<rows,cols,type> &m) 
   {
      out << std::setprecision(2) << std::setiosflags(std::ios::fixed) ;
      for (int r = 0 ; r < rows ; r++)
      {
         out << "| " ;
         for (int c = 0 ; c < cols ; c++)
            out << std::setw(7) << m[r][c] << ' ' ;
         out << "|\n" ;
      }
      return out ;
   }

   inline Point operator * (const Point &p, const Matrix<> &m)
   {
      Point result ;
      result.x( p.x()*m[0][0] + p.y()*m[0][1] + p.z()*m[0][2] + m[0][3] ) ;
      result.y( p.x()*m[1][0] + p.y()*m[1][1] + p.z()*m[1][2] + m[1][3] ) ;
      result.z( p.x()*m[2][0] + p.y()*m[2][1] + p.z()*m[2][2] + m[2][3] ) ;
      return result ;
   }

   inline Point operator * (const Matrix<> &m, const Point &p)
   {
      return p * m ;
   }
}

#endif