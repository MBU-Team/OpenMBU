//-----------------------------------------------------------------------------
// Torque Game Engine Advanced
// 
// Copyright (c) 2003 GarageGames.Com
//-----------------------------------------------------------------------------
#ifndef _GFXD3D9TextureObject_H_
#define _GFXD3D9TextureObject_H_

#include "gfx/gfxTextureManager.h"
#include "gfx/gfxTextureHandle.h"

class GFXD3D9TextureObject : public GFXTextureObject
{
private:
   static U32 mTexCount;
   GFXTexHandle   mLockTex;
   D3DLOCKED_RECT mLockRect;
   bool           mLocked;

   IDirect3DBaseTexture9 *mD3DTexture;

   // used for z buffers...
   IDirect3DSurface9 *mD3DSurface;

public:

   GFXD3D9TextureObject( GFXDevice * d, GFXTextureProfile *profile);
   ~GFXD3D9TextureObject();

   IDirect3DBaseTexture9 *    getTex(){ return mD3DTexture; }
   IDirect3DTexture9 *        get2DTex(){ return (LPDIRECT3DTEXTURE9) mD3DTexture; }
   IDirect3DTexture9 **       get2DTexPtr(){ return (LPDIRECT3DTEXTURE9*) &mD3DTexture; }
   IDirect3DVolumeTexture9 *  get3DTex(){ return (LPDIRECT3DVOLUMETEXTURE9) mD3DTexture; }
   IDirect3DVolumeTexture9 ** get3DTexPtr(){ return (LPDIRECT3DVOLUMETEXTURE9*) &mD3DTexture; }

   void release();

   bool isManaged;

   virtual GFXLockedRect * lock(U32 mipLevel = 0, RectI *inRect = NULL);
   virtual void unlock(U32 mipLevel = 0 );

   virtual bool copyToBmp(GBitmap* bmp);
   virtual bool readBackBuffer( Point2I &upperLeft );
   IDirect3DSurface9 *getSurface() {return mD3DSurface;}
   IDirect3DSurface9 **getSurfacePtr() {return &mD3DSurface;}

   // GFXResource
   void zombify();
   void resurrect();

#ifdef TORQUE_DEBUG
   virtual void pureVirtualCrash() {};
#endif
};


#endif
