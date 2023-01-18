//-----------------------------------------------------------------------------
// Torque Game Engine Advanced
//
// Copyright (c) 2003 GarageGames.Com
//-----------------------------------------------------------------------------

#ifndef _GFX_Vertex_Color_H_
#define _GFX_Vertex_Color_H_

#include "core/color.h"
#include "util/swizzle.h"

class GFXVertexColor
{

private:
    U32 packedColorData;
    static Swizzle<U8, 4> *mDeviceSwizzle;

public:
    static void setSwizzle( Swizzle<U8, 4> *val ) { mDeviceSwizzle = val; }

    GFXVertexColor() : packedColorData( 0xFFFFFFFF ) {} // White with full alpha
    GFXVertexColor( const ColorI &color ) { set( color ); }

    void set( U8 red, U8 green, U8 blue, U8 alpha = 255 )
    {
        packedColorData = red << 0 | green << 8 | blue << 16 | alpha << 24;
        mDeviceSwizzle->InPlace( &packedColorData, sizeof( packedColorData ) );
    }

    void set( const ColorI &color )
    {
        mDeviceSwizzle->ToBuffer( &packedColorData, (U8 *)&color, sizeof( packedColorData ) );
    }

    GFXVertexColor &operator=( const ColorI &color ) { set( color ); return *this; }
    operator const U32 *() const { return &packedColorData; }
    const U32& getPackedColorData() const { return packedColorData; }

    void getColor( ColorI *color ) const
    {
        mDeviceSwizzle->ToBuffer( color, &packedColorData, sizeof( packedColorData ) );
    }
};

#endif
