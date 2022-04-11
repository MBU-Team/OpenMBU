//-----------------------------------------------------------------------------
// Torque Game Engine Advanced
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _SWIZZLESPEC_H_
#define _SWIZZLESPEC_H_

//------------------------------------------------------------------------------
// <U8, 4> (most common) Specialization
//------------------------------------------------------------------------------
#include "util/byteswap.h"

template<>
inline void Swizzle<U8, 4>::InPlace( void *memory, const dsize_t size ) const
{
   AssertFatal( size % 4 == 0, "Bad buffer size for swizzle, see docs." );

   U8 *dest = reinterpret_cast<U8 *>( memory );
   U8 *src = reinterpret_cast<U8 *>( memory );

   // Fast divide by 4 since we are assured a proper size
   for( int i = 0; i < size >> 2; i++ )
   {
      BYTESWAP( *dest++, src[mMap[0]] );
      BYTESWAP( *dest++, src[mMap[1]] );
      BYTESWAP( *dest++, src[mMap[2]] );
      BYTESWAP( *dest++, src[mMap[3]] );
      
      src += 4;
   }
}

template<>
inline void Swizzle<U8, 4>::ToBuffer( void *destination, const void *source, const dsize_t size ) const
{
   AssertFatal( size % 4 == 0, "Bad buffer size for swizzle, see docs." );

   U8 *dest = reinterpret_cast<U8 *>( destination );
   const U8 *src = reinterpret_cast<const U8 *>( source );

   // Fast divide by 4 since we are assured a proper size
   for( int i = 0; i < size >> 2; i++ )
   {
      *dest++ = src[mMap[0]];
      *dest++ = src[mMap[1]];
      *dest++ = src[mMap[2]];
      *dest++ = src[mMap[3]];
      
      src += 4;
   }
}

//------------------------------------------------------------------------------
// <U8, 3> Specialization
//------------------------------------------------------------------------------

template<>
inline void Swizzle<U8, 3>::InPlace( void *memory, const dsize_t size ) const
{
   AssertFatal( size % 3 == 0, "Bad buffer size for swizzle, see docs." );

   U8 *dest = reinterpret_cast<U8 *>( memory );
   U8 *src = reinterpret_cast<U8 *>( memory );

   for( int i = 0; i < size /3; i++ )
   {
      BYTESWAP( *dest++, src[mMap[0]] );
      BYTESWAP( *dest++, src[mMap[1]] );
      BYTESWAP( *dest++, src[mMap[2]] );
      
      src += 3;
   }
}

template<>
inline void Swizzle<U8, 3>::ToBuffer( void *destination, const void *source, const dsize_t size ) const
{
   AssertFatal( size % 3 == 0, "Bad buffer size for swizzle, see docs." );

   U8 *dest = reinterpret_cast<U8 *>( destination );
   const U8 *src = reinterpret_cast<const U8 *>( source );

   for( int i = 0; i < size / 3; i++ )
   {
      *dest++ = src[mMap[0]];
      *dest++ = src[mMap[1]];
      *dest++ = src[mMap[2]];
      
      src += 3;
   }
}


#endif