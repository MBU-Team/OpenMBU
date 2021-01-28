//-----------------------------------------------------------------------------
// Torque Shader Engine
// 
// Copyright (c) 2003 GarageGames.Com
//-----------------------------------------------------------------------------

#ifndef _GFXD3DCUBEMAP_H_
#define _GFXD3DCUBEMAP_H_

#include "gfx/gfxCubemap.h"

//**************************************************************************
// Cube map
//**************************************************************************
class GFXD3DCubemap : public GFXCubemap
{
   friend class GFXD3DDevice;

   LPDIRECT3DCUBETEXTURE9  mCubeTex;
   LPDIRECT3DSURFACE9      mDepthBuff;

   static _D3DCUBEMAP_FACES faceList[6];
   bool mInit;
   bool mDynamic;
   U32  mTexSize;
   S32  mCallbackHandle;
   
   void fillCubeTextures( GFXTexHandle *faces, LPDIRECT3DDEVICE9 D3DDevice );
   void releaseSurfaces();
   static void texManagerCallback( GFXTexCallbackCode code, void *userData );

public:
   virtual void initStatic( GFXTexHandle *faces );
   virtual void initDynamic( U32 texSize );
   virtual void setToTexUnit( U32 tuNum );
   virtual void updateDynamic( Point3F &pos );

   GFXD3DCubemap();
   virtual ~GFXD3DCubemap();

};




#endif // GFXD3DCUBEMAP
