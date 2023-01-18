//-----------------------------------------------------------------------------
// Torque Shader Engine
// 
// Copyright (c) 2003 GarageGames.Com
//-----------------------------------------------------------------------------

#ifndef _GFXCUBEMAP_H_
#define _GFXCUBEMAP_H_

#include "gfx/gfxTextureHandle.h"
//#include "gfx/gfxDevice.h"

//**************************************************************************
// Cube map
//**************************************************************************
class GFXCubemap : public RefBase, public GFXResource
{
    friend class GFXDevice;
private:
    // should only be called by GFXDevice
    virtual void setToTexUnit(U32 tuNum) = 0;

protected:
    static S32 smReflectionDetailLevel;

public:
    virtual void initStatic(GFXTexHandle* faces) = 0;
    virtual void initDynamic(U32 texSize) = 0;

    void initNormalize(U32 size);

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
