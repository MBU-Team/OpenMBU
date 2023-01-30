//-----------------------------------------------------------------------------
// Torque Game Engine Advanced
// 
// Copyright (c) 2003 GarageGames.Com
//-----------------------------------------------------------------------------

#ifndef _GFXD3D9CUBEMAP_H_
#define _GFXD3D9CUBEMAP_H_

#include "gfx/gfxCubemap.h"
#include "gfx/gfxResource.h"
#include "gfx/gfxTarget.h"

//**************************************************************************
// Cube map
//**************************************************************************
class GFXD3D9Cubemap : public GFXCubemap
{
   friend class GFXPCD3D9TextureTarget;
   friend class GFX360TextureTarget;
   friend class GFXD3D9Device;

   LPDIRECT3DCUBETEXTURE9  mCubeTex;
   GFXTexHandle mDepthBuff;
   GFXTextureTargetRef mRenderTarget;

   static _D3DCUBEMAP_FACES faceList[6];
   bool mInit;
   bool mDynamic;
   U32  mTexSize;
   S32  mCallbackHandle;
   S32  mNumFacesPerUpdate;
   S32  mCurrentFace;
   
   void fillCubeTextures( GFXTexHandle *faces, LPDIRECT3DDEVICE9 D3DDevice );
   void releaseSurfaces();
   static void texManagerCallback( GFXTexCallbackCode code, void *userData );

public:
   virtual void initStatic( GFXTexHandle *faces );
   virtual void initDynamic( U32 texSize );
   virtual void setToTexUnit( U32 tuNum );
   virtual void updateDynamic( const Point3F &pos );

   GFXD3D9Cubemap();
   virtual ~GFXD3D9Cubemap();

   // GFXResource interface
   virtual void zombify();
   virtual void resurrect();
};

#endif // GFXD3D9CUBEMAP
