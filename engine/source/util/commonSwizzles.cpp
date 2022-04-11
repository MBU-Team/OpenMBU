//-----------------------------------------------------------------------------
// Torque Game Engine Advanced
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "util/swizzle.h"

namespace Swizzles
{
   dsize_t _bgra[] = { 2, 1, 0, 3 };
   dsize_t _bgr[] = { 2, 1, 0 };
   dsize_t _rgb[] = { 0, 1, 2 };
   dsize_t _argb[] = { 3, 0, 1, 2 };
   dsize_t _rgba[] = { 0, 1, 2, 3 };
   dsize_t _abgr[] = { 3, 2, 1, 0 };

   Swizzle<U8, 4> bgra( _bgra );
   Swizzle<U8, 3> bgr( _bgr );
   Swizzle<U8, 3> rgb( _rgb );
   Swizzle<U8, 4> argb( _argb );
   Swizzle<U8, 4> rgba( _rgba );
   Swizzle<U8, 4> abgr( _abgr );

   NullSwizzle<U8, 4> null;
};