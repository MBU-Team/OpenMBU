//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _STRINGBUFFER_H_
#define _STRINGBUFFER_H_

//Includes
#ifndef _PLATFORM_H_
#include "platform/platform.h"
#endif

#ifndef _TVECTOR_H_
#include "core/tVector.h"
#endif

/// Utility class to wrap string manipulation in a representation
/// independent way.
///
/// Length does NOT include the null terminator.
class StringBuffer
{
   Vector<UTF16>  mBuffer;

public:
   StringBuffer() : mBuffer() 
   {
      mBuffer.push_back(0);
   };

   /// Copy constructor. Very important.
   StringBuffer(const StringBuffer &copy) : mBuffer()
   {
      set(&copy);
   };

   StringBuffer(const StringBuffer *in);
//   StringBuffer(const char *in);
   StringBuffer(const UTF8 *in);
   StringBuffer(const UTF16 *in);

   ~StringBuffer();

   void append(const StringBuffer &in);
   void insert(const U32 charOffset, const StringBuffer &in);
   StringBuffer substring(const U32 start, const U32 len) const;
   StringBuffer cut(const U32 start, const U32 len);

   const UTF16 getChar(const U32 offset) const;

   // No setChar because even a UTF16 char might potentially involve 
   // surrogate pairs, so rather than allow broken behavior, we'll just 
   // ONLY accept substrings in mutable operations.

   void set(const StringBuffer *in);
//   void set(const char *in);
   void set(const UTF8 *in);
   void set(const UTF16 *in);

   const U32 length() const 
   { 
      return mBuffer.size() - 1; // Don't count the NULL of course.
   }

   void get(UTF8 *buff, const U32 buffSize) const;
   void get(UTF16 *buff, const U32 buffSize) const;
};

#endif