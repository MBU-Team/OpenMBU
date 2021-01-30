//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------
#include "gfx/gfxDevice.h"

#ifndef _PRIMBUILDER_H_
#define _PRIMBUILDER_H_

//**************************************************************************
//
//**************************************************************************

/// Primitive Builder.
///
/// A simple interface to put together lines and polygons
/// quickly and easily - OpenGL style. This is basically
/// a convenient way to fill a vertex buffer, then draw it.
///
/// There are two ways to use it. You can use the begin()
/// and end() calls to have it draw immediately after calling
/// end(). This is the "OpenGL" or "immediate" style of usage.
///
/// The other way to use this is to use the beginToBuffer()
/// and endToBuffer() calls, which let you store the
/// results of your intermediate calls for later use.
/// This is much more efficient than using the immediate style.
///
namespace PrimBuild
{
    void beginToBuffer(GFXPrimitiveType type, U32 maxVerts);
    GFXVertexBuffer* endToBuffer(U32& numPrims);

    void begin(GFXPrimitiveType type, U32 maxVerts);
    void end();

    void vertex2f(F32 x, F32 y);
    void vertex3f(F32 x, F32 y, F32 z);

    void vertex2fv(F32* data);
    inline void vertex2fv(Point2F& pnt) { vertex2fv((F32*)&pnt); };

    void vertex3fv(F32* data);
    inline void vertex3fv(Point3F& pnt) { vertex3fv((F32*)&pnt); };


    void color(ColorI);
    void color(ColorF);
    void color3i(U8 red, U8 green, U8 blue);
    void color4i(U8 red, U8 green, U8 blue, U8 alpha);
    void color3f(F32 red, F32 green, F32 blue);
    void color4f(F32 red, F32 green, F32 blue, F32 alpha);

    void texCoord2f(F32 x, F32 y);

    void shutdown();
}

#endif
