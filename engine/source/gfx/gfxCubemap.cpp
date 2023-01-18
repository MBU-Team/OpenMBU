#include "gfxCubemap.h"
#include "console/consoleTypes.h"

// Added as console variable in gfxDevice.cpp - GFXDevice::create()
S32 GFXCubemap::smReflectionDetailLevel = 2;

void GFXCubemap::initNormalize(U32 size)
{
    Point3F axis[6] =
        {Point3F(1.0, 0.0, 0.0), Point3F(-1.0,  0.0,  0.0),
         Point3F(0.0, 1.0, 0.0), Point3F( 0.0, -1.0,  0.0),
         Point3F(0.0, 0.0, 1.0), Point3F( 0.0,  0.0, -1.0),};
    Point3F s[6] =
        {Point3F(0.0, 0.0, -1.0), Point3F( 0.0, 0.0, 1.0),
         Point3F(1.0, 0.0,  0.0), Point3F( 1.0, 0.0, 0.0),
         Point3F(1.0, 0.0,  0.0), Point3F(-1.0, 0.0, 0.0),};
    Point3F t[6] =
        {Point3F(0.0, -1.0, 0.0), Point3F(0.0, -1.0,  0.0),
         Point3F(0.0,  0.0, 1.0), Point3F(0.0,  0.0, -1.0),
         Point3F(0.0, -1.0, 0.0), Point3F(0.0, -1.0,  0.0),};

    F32 span = 2.0;
    F32 start = -1.0;

    F32 stride = span / F32(size - 1);
    GFXTexHandle faces[6];

    for(U32 i=0; i<6; i++)
    {
        GFXTexHandle &tex = faces[i];
        GBitmap *bitmap = new GBitmap(size, size);

        // fill in...
        for(U32 v=0; v<size; v++)
        {
            for(U32 u=0; u<size; u++)
            {
                Point3F vector;
                vector = axis[i] +
                         ((F32(u) * stride) + start) * s[i] +
                         ((F32(v) * stride) + start) * t[i];
                vector.normalizeSafe();
                vector = ((vector * 0.5) + Point3F(0.5, 0.5, 0.5)) * 255.0;
                vector.x = mClampF(vector.x, 0.0f, 255.0f);
                vector.y = mClampF(vector.y, 0.0f, 255.0f);
                vector.z = mClampF(vector.z, 0.0f, 255.0f);
                // easy way to avoid knowledge of the format (RGB, RGBA, RGBX, ...)...
                U8 *bits = bitmap->getAddress(u, v);
                bits[0] = U8(vector.x);
                bits[1] = U8(vector.y);
                bits[2] = U8(vector.z);
            }
        }

        tex.set(bitmap, &GFXDefaultStaticDiffuseProfile, true);
    }

    initStatic(faces);
}