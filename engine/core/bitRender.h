//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _BITRENDER_H_
#define _BITRENDER_H_

#ifndef _MMATH_H_
#include "math/mMath.h"
#endif

/// Functions for rendering to 1bpp bitmaps.
///
/// Used primarily for fast shadow rendering.
struct BitRender
{
   /// Render a triangle to a bitmap of 1-bit per pixel and size dim X dim.
   static void render(const Point2I *, const Point2I *, const Point2I *, S32 dim, U32 * bits);

   /// Render a number of triangle strips to 1-bit per pixel bmp of size dim by dim.
   static void render_strips(const U8 * draw, S32 numDraw, S32 szDraw, const U16 * indices, const Point2I * points, S32 dim, U32 * bits);

   /// Render a number of triangles to 1-bit per pixel bmp of size dim by dim.
   static void render_tris(const U8 * draw, S32 numDraw, S32 szDraw, const U16 * indices, const Point2I * points, S32 dim, U32 * bits);

   /// @name Render Bits
   /// These are used to convert a 1bpp bitmap to an 8bpp bitmap.
   ///
   /// @see Shadow::endRenderToBitmap
   /// @{

   /// Render bits to the bitmap.
   static void bitTo8Bit(U32 * bits, U32 * eightBits, S32 dim);

   /// Render bits to the bitmap, with gaussian pass.
   static void bitTo8Bit_3(U32 * bits, U32 * eightBits, S32 dim);
   /// @}
};

#endif // _BIT_RENDER_H_

