//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------
#include "primBuilder.h"
#include "console/console.h"


//*****************************************************************************
// Primitive Builder
//*****************************************************************************
namespace PrimBuild
{

    GFXVertexBufferHandle<GFXVertexPCT> mVertBuff;
    GFXPrimitiveType  mType;
    U32               mCurVertIndex;
    ColorI            mCurColor(255, 255, 255);
    Point2F           mCurTexCoord;

#ifdef TORQUE_DEBUG
    U32 mMaxVerts;

#define INIT_VERTEX_SIZE(x) mMaxVerts = x;
#define VERTEX_BOUNDS_CHECK() AssertFatal( mCurVertIndex < mMaxVerts, "PrimBuilder encountered an out of bounds vertex! Break and debug!" );

    // This next check shouldn't really be used a lot unless you are tracking down
    // a specific bug. -pw
#define VERTEX_SIZE_CHECK() AssertFatal( mCurVertIndex <= mMaxVerts, "PrimBuilder allocated more verts than you used! Break and debug or rendering artifacts could occur." );

#else

#define INIT_VERTEX_SIZE(x)
#define VERTEX_BOUNDS_CHECK()
#define VERTEX_SIZE_CHECK()

#endif

    //-----------------------------------------------------------------------------
    // begin
    //-----------------------------------------------------------------------------
    void begin(GFXPrimitiveType type, U32 maxVerts)
    {
        mType = type;
        mCurVertIndex = 0;
        INIT_VERTEX_SIZE(maxVerts);
        mVertBuff.set(GFX, maxVerts, GFXBufferTypeVolatile);
        mVertBuff.lock();
    }

    void beginToBuffer(GFXPrimitiveType type, U32 maxVerts)
    {
        mType = type;
        mCurVertIndex = 0;
        INIT_VERTEX_SIZE(maxVerts);
        mVertBuff.set(GFX, maxVerts, GFXBufferTypeStatic);
        mVertBuff.lock();
    }

    //-----------------------------------------------------------------------------
    // end
    //-----------------------------------------------------------------------------
    GFXVertexBuffer* endToBuffer(U32& numPrims)
    {
        mVertBuff.unlock();

        VERTEX_SIZE_CHECK();

        switch (mType)
        {
        case GFXPointList:
        {
            numPrims = mCurVertIndex;
            break;
        }

        case GFXLineList:
        {
            numPrims = mCurVertIndex / 2;
            break;
        }

        case GFXLineStrip:
        {
            numPrims = mCurVertIndex - 1;
            break;
        }

        case GFXTriangleList:
        {
            numPrims = mCurVertIndex / 3;
            break;
        }

        case GFXTriangleStrip:
        case GFXTriangleFan:
        {
            numPrims = mCurVertIndex - 2;
            break;
        }

        }

        return mVertBuff;
    }

    void end()
    {
        mVertBuff.unlock();

        VERTEX_SIZE_CHECK();

        U32 numPrims = 0;

        switch (mType)
        {
        case GFXPointList:
        {
            numPrims = mCurVertIndex;
            break;
        }

        case GFXLineList:
        {
            numPrims = mCurVertIndex / 2;
            break;
        }

        case GFXLineStrip:
        {
            numPrims = mCurVertIndex - 1;
            break;
        }

        case GFXTriangleList:
        {
            numPrims = mCurVertIndex / 3;
            break;
        }

        case GFXTriangleStrip:
        case GFXTriangleFan:
        {
            numPrims = mCurVertIndex - 2;
            break;
        }

        }

        GFX->setVertexBuffer(mVertBuff);
        GFX->drawPrimitive(mType, 0, numPrims);
    }

    //-----------------------------------------------------------------------------
    // vertex2f
    //-----------------------------------------------------------------------------
    void vertex2f(F32 x, F32 y)
    {
        VERTEX_BOUNDS_CHECK();
        GFXVertexPCT* vert = &mVertBuff[mCurVertIndex++];

        vert->point.x = x;
        vert->point.y = y;
        vert->point.z = 0.0;
        vert->color = mCurColor;
        vert->texCoord = mCurTexCoord;
    }

    //-----------------------------------------------------------------------------
    // vertex3f
    //-----------------------------------------------------------------------------
    void vertex3f(F32 x, F32 y, F32 z)
    {
        VERTEX_BOUNDS_CHECK();
        GFXVertexPCT* vert = &mVertBuff[mCurVertIndex++];

        vert->point.x = x;
        vert->point.y = y;
        vert->point.z = z;
        vert->color = mCurColor;
        vert->texCoord = mCurTexCoord;
    }

    //-----------------------------------------------------------------------------
    // vertex3fv
    //-----------------------------------------------------------------------------
    void vertex3fv(F32* data)
    {
        VERTEX_BOUNDS_CHECK();
        GFXVertexPCT* vert = &mVertBuff[mCurVertIndex++];

        vert->point.set(data[0], data[1], data[2]);
        vert->color = mCurColor;
        vert->texCoord = mCurTexCoord;
    }

    //-----------------------------------------------------------------------------
    // vertex2fv
    //-----------------------------------------------------------------------------
    void vertex2fv(F32* data)
    {
        VERTEX_BOUNDS_CHECK();
        GFXVertexPCT* vert = &mVertBuff[mCurVertIndex++];

        vert->point.set(data[0], data[1], 0.f);
        vert->color = mCurColor;
        vert->texCoord = mCurTexCoord;
    }



    //-----------------------------------------------------------------------------
    // color
    //-----------------------------------------------------------------------------
    void color(ColorI inColor)
    {
        mCurColor = inColor;
    }

    void color(ColorF inColor)
    {
        mCurColor = inColor;
    }

    void color3i(U8 red, U8 green, U8 blue)
    {
        mCurColor.set(red, green, blue);
    }

    void color4i(U8 red, U8 green, U8 blue, U8 alpha)
    {
        mCurColor.set(red, green, blue, alpha);
    }

    void color3f(F32 red, F32 green, F32 blue)
    {
        mCurColor.set(U8(red * 255), U8(green * 255), U8(blue * 255));
    }

    void color4f(F32 red, F32 green, F32 blue, F32 alpha)
    {
        mCurColor.set(U8(red * 255), U8(green * 255), U8(blue * 255), U8(alpha * 255));
    }


    //-----------------------------------------------------------------------------
    // texCoord
    //-----------------------------------------------------------------------------
    void texCoord2f(F32 x, F32 y)
    {
        mCurTexCoord.set(x, y);
    }

    void shutdown()
    {
        mVertBuff = NULL;
    }

}  // namespace PrimBuild
