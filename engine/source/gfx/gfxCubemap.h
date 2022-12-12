//-----------------------------------------------------------------------------
// Torque Shader Engine
// 
// Copyright (c) 2003 GarageGames.Com
//-----------------------------------------------------------------------------

#ifndef _GFXCUBEMAP_H_
#define _GFXCUBEMAP_H_

#include "gfx/gfxTextureHandle.h"
#include "gfx/gfxDevice.h"

//**************************************************************************
// Cube map
//**************************************************************************
class GFXCubemap : public RefBase, public GFXResource
{
    friend class GFXDevice;
private:
    // should only be called by GFXDevice
    virtual void setToTexUnit(U32 tuNum) = 0;

public:
    virtual void initStatic(GFXTexHandle* faces) = 0;
    virtual void initDynamic(U32 texSize) = 0;

    void initNormalize(U32 size)
    {
        // TODO: Move to cpp file
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

    // pos is the position to generate cubemap from
    virtual void updateDynamic(const Point3F& pos) = 0;

    virtual ~GFXCubemap() {}

    // GFXResource interface
    virtual void describeSelf(char* buffer, U32 sizeOfBuffer)
    {
        // We've got nothing
        buffer[0] = NULL;
    }
};

typedef RefPtr<GFXCubemap> GFXCubemapHandle;



#endif // GFXCUBEMAP
