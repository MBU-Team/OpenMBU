//-----------------------------------------------------------------------------
// Torque Shader Engine
// 
// Copyright (c) 2003 GarageGames.Com
//-----------------------------------------------------------------------------

#ifndef _GFX_Vertex_Color_H_
#define _GFX_Vertex_Color_H_

#include "core/color.h"

#define PACK_COLOR(c, outPack) 

/// The purpose of this class is to get around the crazy stupid way that Direct3D/Windows
/// does color. Since they decided to be different than the rest of the world, who use
/// RGBA, and use BGRA, we must do this...
class GFXVertexColor 
{
   
   private:
      U32 packedColorData;

   public:
      enum ColorOrdering 
      {
         RGBA,
         BGRA,
      };

      static ColorOrdering sVertexColorOrdering;

      GFXVertexColor()
      {
         packedColorData = 255 << 24; // Black with full alpha
      }

      GFXVertexColor( const ColorI &color );
      GFXVertexColor( const ColorF &color );

      void set( U8 red, U8 green, U8 blue, U8 alpha = 255 );
      void set( const ColorI &color ) { set( color.red, color.green, color.blue, color.alpha ); }
      void set( const ColorF &color ) { set( U8( color.red * 255 ), U8( color.green * 255 ), U8( color.blue * 255 ), U8( color.alpha * 255 ) ); }

      GFXVertexColor &operator=( const ColorI &color ) { set( color ); return *this; }
      GFXVertexColor &operator=( const ColorF &color ) { set( color ); return *this; }

      operator U32*() { return &packedColorData; }

      void getColor( ColorI *color ) const;

      const U32& getPackedColorData() const { return packedColorData; }
};

//-----------------------------------------------------------------------------



//-----------------------------------------------------------------------------

#endif
