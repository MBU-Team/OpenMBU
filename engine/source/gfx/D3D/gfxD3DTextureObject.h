//-----------------------------------------------------------------------------
// Torque Shader Engine
// 
// Copyright (c) 2003 GarageGames.Com
//-----------------------------------------------------------------------------
#ifndef _GFXD3DTEXTUREOBJECT_H_
#define _GFXD3DTEXTUREOBJECT_H_

#include "gfx/gfxTextureManager.h"
#include "gfx/gfxTextureHandle.h"

//*****************************************************************************
// GFX D3D Texture Object
//*****************************************************************************
class GFXD3DTextureObject : public GFXTextureObject
{
private:
    static U32 mTexCount;
    GFXTexHandle   mLockTex;     // used to copy render target data to
    D3DLOCKED_RECT mLockRect;
    bool           mLocked;

    IDirect3DBaseTexture9* mD3DTexture;  // could be 2D or volume tex (possibly cube tex in future)

    // used for z buffers...
    IDirect3DSurface9* mD3DSurface;

public:

    GFXD3DTextureObject(GFXDevice* d, GFXTextureProfile* profile);
    ~GFXD3DTextureObject();

    IDirect3DBaseTexture9* getTex() { return mD3DTexture; }
    IDirect3DTexture9* get2DTex() { return (LPDIRECT3DTEXTURE9)mD3DTexture; }
    IDirect3DTexture9** get2DTexPtr() { return (LPDIRECT3DTEXTURE9*)&mD3DTexture; }
    IDirect3DVolumeTexture9* get3DTex() { return (LPDIRECT3DVOLUMETEXTURE9)mD3DTexture; }
    IDirect3DVolumeTexture9** get3DTexPtr() { return (LPDIRECT3DVOLUMETEXTURE9*)&mD3DTexture; }

    void release();

    bool isManaged;

    virtual GFXLockedRect* lock(U32 mipLevel = 0, RectI* inRect = NULL);
    virtual void unlock(U32 mipLevel = 0);

    bool copyToBmp(GBitmap* bmp);

    IDirect3DSurface9* getSurface() { return mD3DSurface; }
    IDirect3DSurface9** getSurfacePtr() { return &mD3DSurface; }

#ifdef TORQUE_DEBUG
    virtual void pureVirtualCrash() {};
#endif
};


#endif
