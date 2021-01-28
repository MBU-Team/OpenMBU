//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "core/stringBuffer.h"
#include "core/frameAllocator.h"
#include "core/unicode.h"

StringBuffer::StringBuffer(const StringBuffer *in)
: mBuffer()
{
   set(in);
}

/*StringBuffer::StringBuffer(const char *in)
: mBuffer()
{
   set(in);
}*/

StringBuffer::StringBuffer(const UTF8 *in)
: mBuffer()
{
   set(in);
}

StringBuffer::StringBuffer(const UTF16 *in)
: mBuffer()
{
   set(in);
}

//-------------------------------------------------------------------------

StringBuffer::~StringBuffer()
{
   // Everything will get cleared up nicely cuz it's a vector. Sweet.
}

//-------------------------------------------------------------------------

void StringBuffer::set(const StringBuffer *in)
{
   // Copy the vector.
   mBuffer.setSize(in->length()+1);
   dMemcpy(mBuffer.address(), in->mBuffer.address(), sizeof(UTF16)*in->mBuffer.size());
}

/*void StringBuffer::set(const char *in)
{
   // Same rules apply for ANSI strings as for UTF8.
   set((UTF8*)in);
}*/

void StringBuffer::set(const UTF8 *in)
{
   // Convert and store. Note that a UTF16 version of the string cannot be longer.
   FrameTemp<UTF16> tmpBuff(dStrlen(in)+1);
   if(!in || in[0] == 0 || !convertUTF8toUTF16(in, tmpBuff, dStrlen(in)+1))
   {
      // Easy out, it's a blank string, or a bad string.
      mBuffer.clear();
      mBuffer.push_back(0);
      AssertFatal(mBuffer.last() == 0, "StringBuffer::set UTF8 - not a null terminated string!");
      return;
   }
   
   // Otherwise, we've a copy to do. (This might not be strictly necessary.)
   mBuffer.setSize(dStrlen(tmpBuff)+1);
   dMemcpy(mBuffer.address(), tmpBuff, sizeof(UTF16) * mBuffer.size());
   mBuffer.compact();
   AssertFatal(mBuffer.last() == 0, "StringBuffer::set UTF8 - not a null terminated string!");
}

void StringBuffer::set(const UTF16 *in)
{
   // Just copy, it's already UTF16.
   mBuffer.setSize(dStrlen(in)+1);
   dMemcpy(mBuffer.address(), in, sizeof(UTF16) * mBuffer.size());
   mBuffer.compact();
   AssertFatal(mBuffer.last() == 0, "StringBuffer::set UTF16 - not a null terminated string!");
}

//-------------------------------------------------------------------------

void StringBuffer::append(const StringBuffer &in)
{
   // Stick in onto the end of us - first make space.
   U32 oldSize = length();
   mBuffer.increment(in.length());
 
   // Copy it in, ignoring both our terminator and theirs.
   dMemcpy(&mBuffer[oldSize], in.mBuffer.address(), sizeof(UTF16)*in.length());

   // Terminate the string.
   mBuffer.last() = 0;
}

void StringBuffer::insert(const U32 charOffset, const StringBuffer &in)
{
   // Deal with append case.
   if(charOffset >= length())
   {
      append(in);
      return;
   }

   // Append was easy, now we have to do some work.

   // Make some space in our buffer. We only need in.length() more as we
   // will be dropping one of the two terminators present in this operation.
   mBuffer.increment(in.length());
   
   // Copy everything we have that comes after charOffset past where the new
   // string data will be.

   // Figure the address to start copying at. We know this is ok as otherwise
   // we'd be in the append case.
   const UTF16 *copyStart = &mBuffer[charOffset];

   // Figure the number of UTF16's to copy, taking into account the possibility
   // that we may be inserting a long string into a short string.
   
   // We want to copy in.length() chars up, but there may not be that many available.
   // So we want to really do either in.length() or length()-charOffset, whichever
   // is lower.
   const U32 copyCharLength = (S32)(length() - charOffset);

   // Don't copy unless we have to.
   if(copyCharLength)
   {
      for(S32 i=copyCharLength-1; i>=0; i--)
         mBuffer[charOffset+i+in.length()] = mBuffer[charOffset+i];

      //  Can't copy directly, it messes up sometimes, esp. in release mode builds.
      //dMemcpy(&mBuffer[charOffset+in.length()], &mBuffer[charOffset], sizeof(UTF16) * copyCharLength);
   }

   // And finally copy the new data in, not including its terminator.
   dMemcpy(&mBuffer[charOffset], in.mBuffer.address(), sizeof(UTF16) * in.length());

   // All done!
   AssertFatal(mBuffer.last() == 0, "StringBuffer::insert - not a null terminated string!");
}

StringBuffer StringBuffer::substring(const U32 start, const U32 len) const
{
   // Deal with bonehead user input.
   if(start >= length() || len == 0)
   {
      // Either they asked beyond the end of the string or we're null. Either
      // way, let's give them a null string.
      return StringBuffer("");
   }

   AssertFatal(start < length(), "StringBuffer::substring - invalid start!");
   AssertFatal(start+len <= length(), "StringBuffer::substring - invalid len!");
   AssertFatal(len > 0, "StringBuffer::substring - len must be >= 1.");

   StringBuffer tmp;
   tmp.mBuffer.clear();
   for(S32 i=0; i<len; i++)
      tmp.mBuffer.push_back(mBuffer[start+i]);
   if(tmp.mBuffer.last() != 0) tmp.mBuffer.push_back(0);
   
   // Make sure this shit is terminated; we might get a totally empty string.
   if(!tmp.mBuffer.size()) 
      tmp.mBuffer.push_back(0);

   return tmp;
}

StringBuffer StringBuffer::cut(const U32 start, const U32 len)
{
   AssertFatal(start < length(), "StringBuffer::cut - invalid start!");
   AssertFatal(start+len <= length(), "StringBuffer::cut - invalid len!");
   AssertFatal(len > 0, "StringBuffer::cut - len >= 1.");

   // Get the substring for later use.
   StringBuffer tmp;
   tmp.mBuffer.clear();
   tmp.mBuffer.reserve(len);
   for(S32 i=0; i<len; i++)
      tmp.mBuffer.push_back(mBuffer[start+i]);
   if(tmp.mBuffer.last() != 0) 
      tmp.mBuffer.push_back(0);

   AssertFatal(mBuffer.last() == 0, "StringBuffer::cut - not a null terminated string! (pre)");

   // Now snip things.
   for(S32 i=start; i<mBuffer.size()-len; i++)
      mBuffer[i] = mBuffer[i+len];
   mBuffer.decrement(len);
   mBuffer.compact();

   AssertFatal(mBuffer.last() == 0, "StringBuffer::cut - not a null terminated string! (post)");

   return tmp;
}

const UTF16 StringBuffer::getChar(const U32 offset) const
{
   // Allow them to grab the null terminator if they want.
   AssertFatal(offset<mBuffer.size(), "StringBuffer::getChar - outside of range.");

   return mBuffer[offset];
}

//-------------------------------------------------------------------------

void StringBuffer::get(UTF8 *buff, const U32 buffSize) const
{
   AssertFatal(mBuffer.last() == 0, "StringBuffer::get UTF8 - not a null terminated string!");
   convertUTF16toUTF8(mBuffer.address(), buff, buffSize);
}

void StringBuffer::get(UTF16 *buff, const U32 buffSize) const
{
   // Just copy it out.
   AssertFatal(mBuffer.last() == 0, "StringBuffer::get UTF8 - not a null terminated string!");
   dMemcpy(buff, mBuffer.address(), mBuffer.memSize());
}
