//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _BITVECTOR_H_
#define _BITVECTOR_H_

#ifndef _PLATFORM_H_
#include "platform/platform.h"
#endif

/// Manage a vector of bits of arbitrary size.
class BitVector
{
   U8*   mBits;
   U32   mByteSize;

   U32   mSize;

   static U32 calcByteSize(const U32 numBits);

  public:
   BitVector();
   BitVector(const U32 _size);
   ~BitVector();

   /// @name Size Management
   /// @{
   void setSize(const U32 _size);
   U32  getSize() const;
   U32  getByteSize() const;
   U32  getAllocatedByteSize() const { return mByteSize; }

   const U8* getBits() const { return mBits; }
   U8*       getNCBits()     { return mBits; }

   /// @}

   bool copy(const BitVector& from);

   /// @name Mutators
   /// Note that bits are specified by index, unlike BitSet32.
   /// @{

   /// Clear all the bits.
   void clear();

   /// Set all the bits.
   void set();

   /// Set the specified bit.
   void set(U32 bit);
   /// Clear the specified bit.
   void clear(U32 bit);
   /// Test that the specified bit is set.
   bool test(U32 bit) const;

   /// @}
};

inline BitVector::BitVector()
{
   mBits     = NULL;
   mByteSize = 0;

   mSize = 0;
}


inline BitVector::BitVector(const U32 _size)
{
   mBits     = NULL;
   mByteSize = 0;

   mSize = 0;

   setSize(_size);
}

inline BitVector::~BitVector()
{
   delete [] mBits;
   mBits = NULL;
   mByteSize = 0;

   mSize = 0;
}

inline U32 BitVector::calcByteSize(const U32 numBits)
{
   // Make sure that we are 32 bit aligned
   //
   return (((numBits + 0x7) >> 3) + 0x3) & ~0x3;
}

inline void BitVector::setSize(const U32 _size)
{
   if (_size != 0) {
      U32 newSize = calcByteSize(_size);
      if (mByteSize < newSize) {
         delete [] mBits;
         mBits     = new U8[newSize];
         mByteSize = newSize;
      }
   } else {
      delete [] mBits;
      mBits     = NULL;
      mByteSize = 0;
   }

   mSize = _size;
}

inline U32 BitVector::getSize() const
{
   return mSize;
}

inline U32 BitVector::getByteSize() const
{
   return calcByteSize(mSize);
}

inline void BitVector::clear()
{
   if (mSize != 0)
      dMemset(mBits, 0x00, calcByteSize(mSize));
}

inline bool BitVector::copy(const BitVector& from)
{
   U32   sourceSize = from.getSize();
   if (sourceSize) {
      setSize(sourceSize);
      dMemcpy(mBits, from.getBits(), getByteSize());
      return true;
   }
   return false;
}

inline void BitVector::set()
{
   if (mSize != 0)
      dMemset(mBits, 0xFF, calcByteSize(mSize));
}

inline void BitVector::set(U32 bit)
{
   AssertFatal(bit < mSize, "Error, out of range bit");

   mBits[bit >> 3] |= U8(1 << (bit & 0x7));
}

inline void BitVector::clear(U32 bit)
{
   AssertFatal(bit < mSize, "Error, out of range bit");

   mBits[bit >> 3] &= U8(~(1 << (bit & 0x7)));
}

inline bool BitVector::test(U32 bit) const
{
   AssertFatal(bit < mSize, "Error, out of range bit");

   return (mBits[bit >> 3] & U8(1 << (bit & 0x7))) != 0;
}

#endif //_BITVECTOR_H_
