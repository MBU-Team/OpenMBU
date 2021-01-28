//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _BITSET_H_
#define _BITSET_H_

#ifndef _PLATFORM_H_
#include "platform/platform.h"
#endif

/// A convenience class to manipulate a set of bits.
///
/// Notice that bits are accessed directly, ie, by passing
/// a variable with the relevant bit set or not, instead of
/// passing the index of the relevant bit.
class BitSet32
{
private:
   /// Internal representation of bitset.
   U32 mbits;

public:
   BitSet32()                         { mbits = 0; }
   BitSet32(const BitSet32& in_rCopy) { mbits = in_rCopy.mbits; }
   BitSet32(const U32 in_mask)        { mbits = in_mask; }

   operator U32() const               { return mbits; }
   U32 getMask() const                { return mbits; }

   /// Set all bits to true.
   void set()                         { mbits  = 0xFFFFFFFFUL; }

   /// Set the specified bit(s) to true.
   void set(const U32 m)              { mbits |= m; }

   /// Masked-set the bitset; ie, using s as the mask and then setting the masked bits
   /// to b.
   void set(BitSet32 s, bool b)       { mbits = (mbits&~(s.mbits))|(b?s.mbits:0); }

   /// Clear all bits.
   void clear()                       { mbits  = 0; }

   /// Clear the specified bit(s).
   void clear(const U32 m)            { mbits &= ~m; }

   /// Toggle the specified bit(s).
   void toggle(const U32 m)           { mbits ^= m; }

   /// Are any of the specified bit(s) set?
   bool test(const U32 m) const       { return (mbits & m) != 0; }

   /// Are ALL the specified bit(s) set?
   bool testStrict(const U32 m) const { return (mbits & m) == m; }

   /// @name Operator Overloads
   /// @{
   BitSet32& operator =(const U32 m)  { mbits  = m;  return *this; }
   BitSet32& operator|=(const U32 m)  { mbits |= m; return *this; }
   BitSet32& operator&=(const U32 m)  { mbits &= m; return *this; }
   BitSet32& operator^=(const U32 m)  { mbits ^= m; return *this; }

   BitSet32 operator|(const U32 m) const { return BitSet32(mbits | m); }
   BitSet32 operator&(const U32 m) const { return BitSet32(mbits & m); }
   BitSet32 operator^(const U32 m) const { return BitSet32(mbits ^ m); }
   /// @}
};


#endif //_NBITSET_H_
