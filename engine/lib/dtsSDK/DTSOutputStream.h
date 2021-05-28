#ifndef __DTSOUTPUTSTREAM_H
#define __DTSOUTPUTSTREAM_H

#include <cassert>
#include <vector>
#include <fstream>

#include "DTSShape.h"
#include "DTSQuaternion.h"
#include "DTSMatrix.h"

namespace DTS
{
   // -------------------------------------------------------------------------
   // class OutputStream
   // --------------------------------------------------------------------------

   /*!   The DTS file format stores in separate buffers values of 8, 16 or 32
         bits of length, in order to speed up loading in non-Intel platforms.
         In order to create a DTS file you should create the three buffers in
         memory and then begin writting the data using helper functions. This
         mimics the way the data is retrieved in the engine.
     
         This class provides the needed functionality. Simply create a
         OutputStream object and write the data using the "write" method,
         that stores the values in the correct buffer. The "flush" method
         creates finally the file in the format expected by the V12 engine.
     
         The class knowns about all DTS namespace simple objects and can
         save them (operator <<) in the expected format.
     
      *** WARNING ***   THIS CODE IS NOT PORTABLE
     
         This code assumes a little-endian 32 bits machine
   */

   class OutputStream
   {
   public:
      
      //! Initialize the stream. The version number is written in the header
      OutputStream (std::ostream &, int DTSVersion = 24) ;
      ~OutputStream () ;
      
      //! Wite "count" dwords (= count x 4 bytes)
      void write (const int   *, int count) ;

      //! Write "count" words (= count x 2 bytes)
      void write (const short *, int count) ; 

      //! Write "count" bytes 
      void write (const char  *, int count) ;
      
      //! Write one dword 
      OutputStream & operator << (const int   value) { write(&value, 1) ; return *this ; }

      //! Write one word
      OutputStream & operator << (const short value) { write(&value, 1) ; return *this ; }

      //! Write one byte
      OutputStream & operator << (const char  value) { write(&value, 1) ; return *this ; }
      
      //! Write one dword (1 float == 32 bits)
      OutputStream & operator << (const float value) 
      { 
         write((int *)&value, 1) ; 
         return *this ; 
      }

      //! Write one byte (provided for convenience)
      OutputStream & operator << (const unsigned char value) 
      { 
         write((char *)&value, 1) ; 
         return *this ; 
      }

      //! Write a string (1 byte len first, then n byte-sized characters)
      OutputStream & operator << (const std::string &s) ;

      // Operators to write some usual structs
      
      OutputStream & operator << (const Point &) ;     
      OutputStream & operator << (const Point2D &) ;     
      OutputStream & operator << (const Box &) ;
      OutputStream & operator << (const Quaternion &) ;
      OutputStream & operator << (const Node &) ;
      OutputStream & operator << (const Object &) ;
      OutputStream & operator << (const Decal &) ;
      OutputStream & operator << (const IFLMaterial &) ;
      OutputStream & operator << (const DecalState &) ;
      OutputStream & operator << (const DetailLevel &) ;
      OutputStream & operator << (const ObjectState &) ;
      OutputStream & operator << (const Subshape &) ;
      OutputStream & operator << (const Trigger &) ;
      OutputStream & operator << (const Primitive &) ;
      OutputStream & operator << (const Mesh &) ;
      OutputStream & operator << (const Matrix<4,4> &) ;
      
      //! Stores checkpoint values in the streams
      void storeCheck (int checkPoint = -1) ;

      //! When you're finished, call this method to create the file
      //! You'll still need to write the sequence data to the stream, next
      void flush () ;
      
   private:
      
      std::ostream & out ;
      int DTSVersion ;
      int checkCount ;
      
      // Pointers to the 3 memory buffers
      
      int   * Buffer32 ;
      short * Buffer16 ;
      char  * Buffer8  ;
      
      // Values allocated in each buffer
      
      unsigned long Allocated32 ;
      unsigned long Allocated16 ;
      unsigned long Allocated8 ;
      
      // Values actually used in each buffer
      
      unsigned long Used32 ;
      unsigned long Used16 ;
      unsigned long Used8 ;
   };
   
   // --------------------------------------------------------------------------
   //    Construction and destruction
   // --------------------------------------------------------------------------
   
   inline OutputStream::OutputStream (std::ostream & s, int version)
      : out(s), DTSVersion(version)
   {
      // Versions 17 and down don't use the buffers
      
      assert (version >= 18) ;
      
      // Create buffers with some unused space in
      
      Buffer32 = new int   [64] ;
      Buffer16 = new short [128] ;
      Buffer8  = new char  [256] ;
      
      Allocated32 = 64 ;
      Allocated16 = 128 ;
      Allocated8  = 256 ;
      
      Used32 = 0 ;
      Used16 = 0 ;
      Used8  = 0 ;

      checkCount = 0 ;
   }
   
   inline OutputStream::~OutputStream()
   {
      delete [] Buffer8 ;
      delete [] Buffer16 ;
      delete [] Buffer32 ;
   }
   
   // --------------------------------------------------------------------------
   //    Write functions
   // --------------------------------------------------------------------------

   /*! All 32 bits dwords must be written to the internal 32-bits buffer
       so they can be reversed in a single pass in big-endian machines */
   inline void OutputStream::write (const int * data, int count)
   {
      assert (count > 0) ;
      assert (data != 0) ;
      
      if (Used32 + count > Allocated32)
      {
         // Not enough space allocated. Alloc a bigger buffer,
         // with a multiple of 64 number of elements, but enough
         // to add the new data, and swap the old buffer for it
         
         Allocated32 = ((Used32 + count) & ~63) + 64 ;
         assert (Allocated32 > Used32 + count) ;
         int * NewBuffer32 = new int[Allocated32] ;
         memcpy (NewBuffer32, Buffer32, sizeof(int) * Used32) ;
         delete [] Buffer32 ;
         Buffer32 = NewBuffer32 ;
      }
      
      // Add the new data to the buffer
      
      memcpy (Buffer32 + Used32, data, sizeof(int) * count) ;
      Used32 += count ;
   }
   
   // The other write methods are exactly the same, except for the buffer used
   
   inline void OutputStream::write (const short * data, int count)
   {
      assert (count > 0) ;
      assert (data != 0) ;
      
      if (Used16 + count > Allocated16)
      {
         Allocated16 = ((Used16 + count) & ~63) + 64 ;
         assert (Allocated16 > Used16 + count) ;
         short * NewBuffer16 = new short[Allocated16] ;
         memcpy (NewBuffer16, Buffer16, sizeof(short) * Used16) ;
         delete [] Buffer16 ;
         Buffer16 = NewBuffer16 ;
      }
      
      memcpy (Buffer16 + Used16, data, sizeof(short) * count) ;
      Used16 += count ;
   }
   
   inline void OutputStream::write (const char * data, int count)
   {
      assert (count > 0) ;
      assert (data != 0) ;
      
      if (Used8 + count > Allocated8)
      {
         Allocated8 = ((Used8 + count) & ~63) + 64 ;
         assert (Allocated8 > Used8 + count) ;
         char * NewBuffer8 = new char[Allocated8] ;
         memcpy (NewBuffer8, Buffer8, sizeof(char) * Used8) ;
         delete [] Buffer8 ;
         Buffer8 = NewBuffer8 ;
      }
      
      memcpy (Buffer8 + Used8, data, sizeof(char) * count) ;
      Used8 += count ;
   }
   
   // --------------------------------------------------------------------------
   //    Write operators
   // --------------------------------------------------------------------------
   
   inline OutputStream & OutputStream::operator << (const Point &p)
   { 
      return (*this) << p.x() << p.y() << p.z() ; 
   }
   
   inline OutputStream & OutputStream::operator << (const Point2D &p)
   { 
      return (*this) << p.x() << p.y() ; 
   }
   
   inline OutputStream & OutputStream::operator << (const Box &b)
   { 
      return (*this) << b.min << b.max ; 
   }
   
   inline OutputStream & OutputStream::operator << (const Quaternion &q)
   {
      return (*this) << (short)(q.x() * 32767.0f) 
                     << (short)(q.y() * 32767.0f)
                     << (short)(q.z() * 32767.0f)
                     << (short)(q.w() * 32767.0f) ;
   }

   inline OutputStream & OutputStream::operator << (const Node &n)
   {
      return (*this) << n.name  << n.parent << n.firstObject 
                     << n.child << n.sibling ;
   }
   
   inline OutputStream & OutputStream::operator << (const Object &o)
   {
      return (*this) << o.name << o.numMeshes << o.firstMesh
                     << o.node << o.sibling   << o.firstDecal ;
   }
   
   inline OutputStream & OutputStream::operator << (const Decal &d)
   {
      return (*this) << d.name   << d.numMeshes << d.firstMesh 
                     << d.object << d.sibling ;
   }

   inline OutputStream & OutputStream::operator << (const IFLMaterial &m)
   {
      return (*this) << m.name << m.slot << m.firstFrame 
                     << m.time << m.numFrames ;
   }

   inline OutputStream & OutputStream::operator << (const Subshape &s)
   {
      assert (0 && "Subshapes should be written in block") ;
   }

   inline OutputStream & OutputStream::operator << (const Trigger &d)
   {
      return (*this) << d.state << d.pos ;
   }

   inline OutputStream & OutputStream::operator << (const DecalState &d)
   {
      return (*this) << d.frame ;
   }

   inline OutputStream & OutputStream::operator << (const ObjectState &o)
   {
      return (*this) << o.vis << o.frame << o.matFrame ;
   }

   inline OutputStream & OutputStream::operator << (const DetailLevel &d)
   {
      return (*this) << d.name << d.subshape << d.objectDetail << d.size
                     << d.avgError << d.maxError << d.polyCount ;
   }

   inline OutputStream & OutputStream::operator << (const Primitive &p)
   {
      return (*this) << p.firstElement << p.numElements << p.type ;
   }

   inline OutputStream & OutputStream::operator << (const Mesh &m)
   {
      m.save(*this) ;
      return *this ;
   }

   inline OutputStream & OutputStream::operator << (const std::string &s) 
   {
      std::string::const_iterator pos = s.begin() ;
      while (pos != s.end()) (*this) << *pos++ ;
      (*this) << '\x00' ;
      return *this ;
   }

   template <class type>
   inline OutputStream & operator << (OutputStream & out, const std::vector<type> &v) 
   {
      std::vector<type>::const_iterator pos = v.begin() ;
      while (pos != v.end())
      {
         out << *pos++ ;
      }
      return out ;
   }

   inline OutputStream & OutputStream::operator << (const Matrix<4,4> &m)
   { 
     for (int r = 0 ; r < 4 ; r++)
        for (int c = 0 ; c < 4 ; c++)
            (*this) << m[r][c];
      return *this;
   }
   
   // --------------------------------------------------------------------------
   //  Checkpoints and alignment
   // --------------------------------------------------------------------------
   
   /*! Sometimes during the storing process, a checkpoint value is
       stored to all the buffers at the same time. This check helps
       to determine if the files are not corrupted. */

   inline void OutputStream::storeCheck(int checkPoint)
   {
      if (checkPoint >= 0)
         assert (checkPoint == checkCount) ;

      (*this) << (int)   checkCount ;
      (*this) << (short) checkCount ;
      (*this) << (char)  checkCount ;

      checkCount++ ;
   }

   // --------------------------------------------------------------------------
   //    Flush method
   // --------------------------------------------------------------------------
   
   /*! The file format wants a header with the version number, total size
       of all three buffers (in dwords), and the offsets of the 16-bits
       and 8-bits buffers, followed with the buffer data itself.

       There is additional information that is stored after this data,
       but we don't know about it here */

   inline void OutputStream::flush ()
   {
      int totalSize ;
      int offset16 ;
      int offset8 ;
      
      // Force all buffers to have a size multiple of 4 bytes
      
      if    (Used16 & 0x0001) (*this) << (short)0 ;
      while (Used8  & 0x0003) (*this) << (char) 0 ;
      
      // Compute the header values (in dwords)
      
      offset16  = Used32 ;
      offset8   = Used32 + Used16/2 ;
      totalSize = Used32 + Used16/2 + Used8/4 ;

      // Write the resulting data to the file

      std::streampos pos = out.tellp() ;
      out.write ((char *) &DTSVersion, 4) ;
      out.write ((char *) &totalSize, 4) ;
      out.write ((char *) &offset16, 4) ;
      out.write ((char *) &offset8, 4) ;
      out.write ((char *) Buffer32, 4 * Used32) ;
      out.write ((char *) Buffer16, 2 * Used16) ;
      out.write (Buffer8, Used8) ;
      std::streampos endpos = out.tellp() ;

      assert ((endpos - pos) == 4*Used32 + 2*Used16 + 1*Used8 + 16) ;

   }
   
}

#endif