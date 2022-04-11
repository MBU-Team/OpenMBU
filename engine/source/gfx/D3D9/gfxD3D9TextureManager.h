//-----------------------------------------------------------------------------
// Torque Game Engine Advanced
// 
// Copyright (c) 2003 GarageGames.Com
//-----------------------------------------------------------------------------

#ifndef _GFX_D3D_Texture_Manager_H_
#define _GFX_D3D_Texture_Manager_H_

#include "console/console.h"

#include "gfx/gfxTextureManager.h"
#include "gfx/D3D9/gfxD3D9TextureObject.h"
#include "util/safeRelease.h"

// #define D3D_TEXTURE_SPEW


//*****************************************************************************
// GFX D3D Texture Manager
//*****************************************************************************
class GFXD3D9TextureManager : public GFXTextureManager 
{
   friend class GFXD3D9TextureObject;

   U32 mCurTexSet[TEXTURE_STAGE_COUNT];

private:
   LPDIRECT3DDEVICE9 mD3DDevice;
   D3DCAPS9 mDeviceCaps;

   void innerCreateTexture(GFXD3D9TextureObject *obj, U32 height, U32 width, 
      U32 depth, GFXFormat format, GFXTextureProfile *profile, U32 numMipLevels, 
      bool forceMips = false);

protected:
   GFXTextureObject *_createTexture(U32 height, U32 width, U32 depth, GFXFormat format, 
      GFXTextureProfile *profile, U32 numMipLevels, bool forceMips = false);
   bool _loadTexture(GFXTextureObject *texture, DDSFile *dds);
   bool _loadTexture(GFXTextureObject *texture, GBitmap *bmp);
   bool _loadTexture(GFXTextureObject *texture, void *raw);
   bool _refreshTexture(GFXTextureObject *texture);
   bool _freeTexture(GFXTextureObject *texture, bool zombify = false);
   U32 _getTotalVideoMemory();

public:
   GFXD3D9TextureManager( LPDIRECT3DDEVICE9 d3ddevice );

};

#endif
