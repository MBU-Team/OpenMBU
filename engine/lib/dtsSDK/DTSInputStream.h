#ifndef __DTSINPUTSTREAM_H
#define __DTSINPUTSTREAM_H

#include <cassert>
#include <vector>
#include <fstream>

#include "DTSShape.h"

namespace DTS
{
   // -------------------------------------------------------------------------
   // class InputStream
   // --------------------------------------------------------------------------

   /*!   This is more or less a quick hack to provide future loading 
         features to the library. Refere to the OutputStream for info,
         this is almost a copy of that.
   */

   class InputStream
   {
   public:
      
      //! Load the stream.
      InputStream (std::istream &) ;
      ~InputStream () ;

      int version() { return DTSVersion ; }
      
      //! Read "count" dwords (= count x 4 bytes). A null pointer is OK
      void read (int   *, int count) ;

      //! Read "count" words (= count x 2 bytes). A null pointer is OK
      void read (short *, int count) ; 

      //! Read "count" bytes . A null pointer is OK
      void read (char  *, int count) ;
      
      //! Read one dword . A null pointer is used to discard the value
      InputStream & operator >> (int   &value) { read(&value, 1) ; return *this ; }

      //! Read one word
      InputStream & operator >> (short &value) { read(&value, 1) ; return *this ; }

      //! Read one byte
      InputStream & operator >> (char  &value) { read(&value, 1) ; return *this ; }
      
      //! Read one dword (1 float == 32 bits)
      InputStream & operator >> (float &value) 
      { 
         read((int *)&value, 1) ; 
         return *this ; 
      }

      //! Read one byte (provided for convenience)
      InputStream & operator >> (unsigned char &value) 
      { 
         read((char *)&value, 1) ; 
         return *this ; 
      }

      //! Read a string (1 byte len first, then n byte-sized characters)
      InputStream & operator >> (std::string &s) ;

      // Operators to read some usual structs
      
      InputStream & operator >> (Point &) ;     
      InputStream & operator >> (Point2D &) ;     
      InputStream & operator >> (Box &) ;
      InputStream & operator >> (Quaternion &) ;
      InputStream & operator >> (Node &) ;
      InputStream & operator >> (Object &) ;
      InputStream & operator >> (Decal &) ;
      InputStream & operator >> (IFLMaterial &) ;
      InputStream & operator >> (DecalState &) ;
      InputStream & operator >> (DetailLevel &) ;
      InputStream & operator >> (ObjectState &) ;
      InputStream & operator >> (Subshape &) ;
      InputStream & operator >> (Trigger &) ;
      InputStream & operator >> (Primitive &) ;
      InputStream & operator >> (Mesh &) ;
      
      //! Stores checkpoint values in the streams
      void readCheck (int checkPoint = -1) ;

   private:
      
      std::istream & in ;
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
   
   inline InputStream::InputStream (std::istream & s) : in(s)
   {
      int totalSize ;
      int offset16 ;
      int offset8 ;
      
      // Read the header

      in.read ((char *) &DTSVersion, 4) ;
      in.read ((char *) &totalSize, 4) ;
      in.read ((char *) &offset16, 4) ;
      in.read ((char *) &offset8, 4) ;

      // Create buffers 
      
      Allocated32 = offset16 ;
      Allocated16 = (offset8-offset16)  * 2 ;
      Allocated8  = (totalSize-offset8) * 4 ;

      Buffer32 = new int   [Allocated32] ;
      Buffer16 = new short [Allocated16] ;
      Buffer8  = new char  [Allocated8] ;
      
      // Read the data

      in.read ((char *) Buffer32, 4 * Allocated32) ;
      in.read ((char *) Buffer16, 2 * Allocated16) ;
      in.read (Buffer8, Allocated8) ;

      checkCount = 0 ;
      Used32     = 0 ;
      Used16     = 0 ;
      Used8      = 0 ;
   }
   
   inline InputStream::~InputStream()
   {
      delete [] Buffer8 ;
      delete [] Buffer16 ;
      delete [] Buffer32 ;
   }
   
   // --------------------------------------------------------------------------
   //    Read functions
   // --------------------------------------------------------------------------

   inline void InputStream::read (int * data, int count)
   {
      assert (count > 0) ;
      assert (Used32+count < Allocated32) ;
      if (data != 0) 
         memcpy (data, Buffer32 + Used32, sizeof(int) * count) ;
      Used32 += count ;
   }
   
   // The other read methods are exactly the same, except for the buffer used
   
   inline void InputStream::read (short * data, int count)
   {
      assert (count > 0) ;
      assert (Used16 < Allocated16) ;
      if (data != 0) 
         memcpy (data, Buffer16 + Used16, sizeof(short) * count) ;
      Used16 += count ;
   }
   
   inline void InputStream::read (char * data, int count)
   {
      assert (count > 0) ;
      assert (Used8 < Allocated8) ;
      if (data == 0) 
         memcpy (data, Buffer8 + Used8, sizeof(char) * count) ;
      Used8 += count ;
   }
   
   // --------------------------------------------------------------------------
   //    Read operators
   // --------------------------------------------------------------------------
   
   inline InputStream & InputStream::operator >> (Point &p)
   { 
      float x, y, z ;
      (*this) >> x >> y >> z ;
      p.x(x) ;
      p.y(y) ;
      p.z(z) ;
      return *this ;
   }
   
   inline InputStream & InputStream::operator >> (Point2D &p)
   { 
      float x, y ;
      (*this) >> x >> y ;
      p.x(x) ;
      p.y(y) ;
      return *this ;
   }
   
   inline InputStream & InputStream::operator >> (Box &b)
   { 
      return (*this) >> b.min >> b.max ; 
   }
   
   inline InputStream & InputStream::operator >> (Quaternion &q)
   {
      short x, y, z, w ;
      (*this) >> x >> y >> z >> w ;
      q.x(x / 32767.0f) ;
      q.y(y / 32767.0f) ;
      q.z(z / 32767.0f) ;
      q.w(w / 32767.0f) ;
      return (*this) ;
   }

   inline InputStream & InputStream::operator >> (Node &n)
   {
      return (*this) >> n.name  >> n.parent >> n.firstObject 
                     >> n.child >> n.sibling ;
   }
   
   inline InputStream & InputStream::operator >> (Object &o)
   {
      return (*this) >> o.name >> o.numMeshes >> o.firstMesh
                     >> o.node >> o.sibling   >> o.firstDecal ;
   }
   
   inline InputStream & InputStream::operator >> (Decal &d)
   {
      return (*this) >> d.name   >> d.numMeshes >> d.firstMesh 
                     >> d.object >> d.sibling ;
   }

   inline InputStream & InputStream::operator >> (IFLMaterial &m)
   {
      return (*this) >> m.name >> m.slot >> m.firstFrame 
                     >> m.time >> m.numFrames ;
   }

   inline InputStream & InputStream::operator >> (Subshape &s)
   {
      assert (0 && "Subshapes should be written in block") ;
   }

   inline InputStream & InputStream::operator >> (Trigger &d)
   {
      return (*this) >> d.state >> d.pos ;
   }

   inline InputStream & InputStream::operator >> (DecalState &d)
   {
      return (*this) >> d.frame ;
   }

   inline InputStream & InputStream::operator >> (ObjectState &o)
   {
      return (*this) >> o.vis >> o.frame >> o.matFrame ;
   }

   inline InputStream & InputStream::operator >> (DetailLevel &d)
   {
      return (*this) >> d.name >> d.subshape >> d.objectDetail >> d.size
                     >> d.avgError >> d.maxError >> d.polyCount ;
   }

   inline InputStream & InputStream::operator >> (Primitive &p)
   {
      return (*this) >> p.firstElement >> p.numElements >> p.type ;
   }

   inline InputStream & InputStream::operator >> (Mesh &m)
   {
      m.read(*this) ;
      return *this ;
   }

   inline InputStream & InputStream::operator >> (std::string &s)
   {
      char c ;

      for (s = "" ;;)
      {
         (*this) >> c ;
         if (!c) break ;
         s += c ;
      }
      return *this ;
   }

   template <class type>
   inline InputStream & operator >> (InputStream & in, std::vector<type> &v)
   {
      std::vector<type>::iterator pos = v.begin() ;
      while (pos != v.end())
      {
         in >> *pos++ ;
      }
      return in ;
   }
   
   // --------------------------------------------------------------------------
   //  Checkpoints and alignment
   // --------------------------------------------------------------------------
   
   /*! Sometimes during the storing process, a checkpoint value is
       stored to all the buffers at the same time. This check helps
       to determine if the files are not corrupted. */

   inline void InputStream::readCheck(int checkPoint)
   {
      if (checkPoint >= 0)
         assert (checkPoint == checkCount) ;

      int   checkCount_int ;
      short checkCount_short ;
      char  checkCount_char  ;

      (*this) >> checkCount_int ;
      (*this) >> checkCount_short ;
      (*this) >> checkCount_char ;

      assert (checkCount_char  == (char) checkCount) ;
      assert (checkCount_short == (short)checkCount) ;
      assert (checkCount_int   == (int)  checkCount) ;

      checkCount++ ;
   }

}

#endif