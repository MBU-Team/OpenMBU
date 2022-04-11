//-----------------------------------------------------------------------------
// Torque Game Engine Advanced
// 
// Copyright (c) 2003 GarageGames.Com
//-----------------------------------------------------------------------------

#ifdef _MSC_VER
#pragma warning(disable: 4996) 
#endif
#include "gfx/D3D9/gfxD3D9Device.h"
#include "gfx/D3D9/gfxD3D9EnumTranslate.h"
#include "gfx/D3D9/gfxD3D9TextureManager.h"
#include "gfx/gfxCardProfile.h"
#include "gfx/ddsFile.h"
#include "platform/profiler.h"
#include "core/unicode.h"
#include "util/swizzle.h"

//-----------------------------------------------------------------------------
// Utility function, valid only in this file
#ifdef D3D_TEXTURE_SPEW
U32 GFXD3D9TextureObject::mTexCount = 0;
#endif

//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
GFXD3D9TextureManager::GFXD3D9TextureManager( LPDIRECT3DDEVICE9 d3ddevice ) 
{
   mD3DDevice = d3ddevice;
   dMemset( mCurTexSet, 0, sizeof( mCurTexSet ) );   
   mD3DDevice->GetDeviceCaps(&mDeviceCaps);

}

//-----------------------------------------------------------------------------
// innerCreateTexture
//-----------------------------------------------------------------------------
void GFXD3D9TextureManager::innerCreateTexture( GFXD3D9TextureObject *retTex, 
                                               U32 height, 
                                               U32 width, 
                                               U32 depth,
                                               GFXFormat format, 
                                               GFXTextureProfile *profile, 
                                               U32 numMipLevels,
                                               bool forceMips)
{
   GFXD3D9Device* d3d = static_cast<GFXD3D9Device*>(GFX);

   // Some relevant helper information...
   bool supportsAutoMips = GFX->getCardProfiler()->queryProfile("autoMipMapLevel", true);
   bool supportsRGB24Textures = GFX->getCardProfiler()->queryProfile("allowRGB24BitTextures", false);
   
   DWORD usage = 0;   // 0, D3DUSAGE_RENDERTARGET, or D3DUSAGE_DYNAMIC
   D3DPOOL pool = D3DPOOL_DEFAULT;

   // 24 bit textures not supported by most (or any?) cards - kick to 32
   if( format == GFXFormatR8G8B8 && !supportsRGB24Textures )
      format = GFXFormatR8G8B8X8;

   retTex->mProfile = profile;
   // This is so we can resolve tiled vs linear texture formats for render targets
   D3DFORMAT d3dTextureFormat = GFXD3D9TextureFormat[format];

#ifndef TORQUE_OS_XENON
   if( retTex->mProfile->isDynamic() )
   {
      usage = D3DUSAGE_DYNAMIC;
   }
   else
   {
      pool = D3DPOOL_MANAGED;
   }

   if( retTex->mProfile->isRenderTarget() )
   {
      pool = D3DPOOL_DEFAULT;
      usage |= D3DUSAGE_RENDERTARGET;
   }

   if(retTex->mProfile->isZTarget())
   {
      usage |= D3DUSAGE_DEPTHSTENCIL;
      pool = D3DPOOL_DEFAULT;
   }

   if( retTex->mProfile->isSystemMemory() )
   {
      pool = D3DPOOL_SYSTEMMEM;
   }

   if( supportsAutoMips && 
       !forceMips &&
       !retTex->mProfile->isSystemMemory() &&
       numMipLevels == 0 &&
       !(depth > 0) )
   {
      usage |= D3DUSAGE_AUTOGENMIPMAP;
   }
#else
   if( retTex->mProfile->isRenderTarget() || retTex->mProfile->isZTarget() )
   {
      AssertFatal( GFXD3D9RenderTargetFormat[format] != (_D3DFORMAT)GFX_UNSUPPORTED_VAL, "Invalid target format" );
      d3dTextureFormat = GFXD3D9TiledTextureFormat[format];
   }
#endif

   // Set the managed flag...
   retTex->isManaged = (pool == D3DPOOL_MANAGED);
   
   if( depth > 0 )
   {
#ifdef TORQUE_OS_XENON
      D3D9Assert( mD3DDevice->CreateVolumeTexture( width, height, depth, numMipLevels, 0 /* usage ignored on the 360 */, 
         d3dTextureFormat, pool, retTex->get3DTexPtr(), NULL), "Failed to create volume texture" );
#else
      D3D9Assert(
         GFXD3DX.D3DXCreateVolumeTexture(
            mD3DDevice,
            width,
            height,
            depth,
            numMipLevels,
            usage,
            d3dTextureFormat,
            pool,
            retTex->get3DTexPtr()
         ), "GFXD3D9TextureManager::_createTexture - failed to create volume texture!"
      );
#endif

      retTex->mTextureSize.set( width, height, depth );
      retTex->mMipLevels = retTex->get3DTex()->GetLevelCount();
   }
   else
   {

#ifdef TORQUE_OS_XENON
      D3D9Assert( mD3DDevice->CreateTexture(width, height, numMipLevels, usage, d3dTextureFormat, pool, retTex->get2DTexPtr(), NULL), "Failed to create texture" );
      retTex->mMipLevels = retTex->get2DTex()->GetLevelCount();
#else
      bool fastCreate = true;
      // Check for power of 2 textures - this is a problem with FX 5xxx cards
      // with current drivers - 3/2/05
      if( !isPow2(width) || !isPow2(height) )
      {
         fastCreate = false;
      }

      if(retTex->mProfile->isZTarget())
      {
         D3D9Assert(mD3DDevice->CreateDepthStencilSurface(width, height, d3dTextureFormat,
            d3d->getMultisampleType(), d3d->getMultisampleLevel(), TRUE, retTex->getSurfacePtr(), NULL), "Failed to create Z surface" );
      }
      else
      {
         // Try to create the texture directly - should gain us a bit in high
         // performance cases where we know we're creating good stuff and we
         // don't want to bother with D3DX - slow function.
         HRESULT res = D3DERR_INVALIDCALL;
         if( fastCreate )
         {
            res = mD3DDevice->CreateTexture(width, height, numMipLevels, usage, d3dTextureFormat, pool, retTex->get2DTexPtr(), NULL);
         }

         if( !fastCreate || (res != D3D_OK) )
         {
            D3D9Assert(
               GFXD3DX.D3DXCreateTexture(
               mD3DDevice,
               width,
               height,
               numMipLevels,
               usage,
               d3dTextureFormat,
               pool,
               retTex->get2DTexPtr()
               ), "GFXD3D9TextureManager::_createTexture - failed to create texture!"
               );
         }

         // If this is a render target, and it needs z support, we're going to need to create an actual RenderTarget.  
         // Check the caps though, if we can't stretchrect between textures, use the old RT method.  (Which hopefully means
         // that they can't force AA on us as well.)
         if (retTex->mProfile->isRenderTargetZBuffer() && (mDeviceCaps.Caps2 && D3DDEVCAPS2_CAN_STRETCHRECT_FROM_TEXTURES))
         {
            D3D9Assert(mD3DDevice->CreateRenderTarget(width, height, d3dTextureFormat, 
               d3d->getMultisampleType(), d3d->getMultisampleLevel(), false, retTex->getSurfacePtr(), NULL),
               "GFXD3D9TextureManager::_createTexture - unable to create render target");
         }

         // All done!
         retTex->mMipLevels = retTex->get2DTex()->GetLevelCount();
      }
#endif

      // Get the actual size of the texture...
      D3DSURFACE_DESC probeDesc;
      ZeroMemory(&probeDesc, sizeof probeDesc);

      if( retTex->get2DTex() != NULL )
         D3D9Assert( retTex->get2DTex()->GetLevelDesc( 0, &probeDesc ), "Failed to get surface description");
      else if( retTex->getSurface() != NULL )
         D3D9Assert( retTex->getSurface()->GetDesc( &probeDesc ), "Failed to get surface description");

      retTex->mTextureSize.set(probeDesc.Width, probeDesc.Height, 0);
   }

   retTex->mFormat = format;
}

//-----------------------------------------------------------------------------
// createTexture
//-----------------------------------------------------------------------------
GFXTextureObject *GFXD3D9TextureManager::_createTexture( U32 height, 
                                                        U32 width,
                                                        U32 depth,
                                                        GFXFormat format, 
                                                        GFXTextureProfile *profile, 
                                                        U32 numMipLevels,
                                                        bool forceMips)
{
   GFXD3D9TextureObject *retTex = new GFXD3D9TextureObject( GFX, profile );
   retTex->registerResourceWithDevice(GFX);

   innerCreateTexture(retTex, height, width, depth, format, profile, numMipLevels, forceMips);

   return retTex;
}

//-----------------------------------------------------------------------------
// loadTexture - GBitmap
//-----------------------------------------------------------------------------
bool GFXD3D9TextureManager::_loadTexture(GFXTextureObject *aTexture, GBitmap *pDL)
{
   GFXD3D9TextureObject *texture = static_cast<GFXD3D9TextureObject*>(aTexture);

   // This is so lame, todo: track samplers GFXTextures are assigned to
#ifdef TORQUE_OS_XENON
   if( texture->getTex()->IsSet( mD3DDevice ) )
   {
      mD3DDevice->SetTexture( 0, NULL );
      mD3DDevice->SetTexture( 1, NULL );
      mD3DDevice->SetTexture( 2, NULL );
      mD3DDevice->SetTexture( 3, NULL );
      mD3DDevice->SetTexture( 4, NULL );
      mD3DDevice->SetTexture( 5, NULL );
      mD3DDevice->SetTexture( 6, NULL );
      mD3DDevice->SetTexture( 7, NULL );
   }
#endif

   // Check with profiler to see if we can do automatic mipmap generation.
   bool supportsAutoMips = GFX->getCardProfiler()->queryProfile("autoMipMapLevel", true);

   // Don't do this for fonts!
   if( aTexture->mMipLevels > 1 && ( pDL->getNumMipLevels() == 1 ) && ( pDL->getFormat() != GFXFormatA8 ) &&
      isPow2( pDL->getHeight() ) && isPow2( pDL->getWidth() ) )
      pDL->extrudeMipLevels(false);

   // Settings for mipmap generation
   U32 maxDownloadMip = pDL->getNumMipLevels();
   U32 nbMipMapLevel  = pDL->getNumMipLevels();

   if(supportsAutoMips)
   {
      maxDownloadMip = 1;
      nbMipMapLevel  = aTexture->mMipLevels;
   }

   // Fill the texture...
   U32 i = 0;
   while( i < maxDownloadMip )
   {

      RECT rect;
      rect.left    = 0;
      rect.right   = pDL->getWidth(i);
      rect.top     = 0;
      rect.bottom  = pDL->getHeight(i);

      LPDIRECT3DSURFACE9 surf = NULL;
      D3D9Assert(texture->get2DTex()->GetSurfaceLevel( i, &surf ), "Failed to get surface");

//------------------------------------------------------------------------------
// This code chunk  is used to assign different colors to different mipmap levels
// for debugging purposes.

      //U32 bmpSize = pDL->getWidth(i) * pDL->getHeight(i);
      //FrameAllocatorMarker foo;
      //U8 *bar = (U8 *)foo.alloc( bmpSize * pDL->bytesPerPixel );

      //U8 color[4] = { 255, 255, 255, 255 };
      //switch( i )
      //{
      //case 1:
      //   // Red
      //   color[1] = color[2] = 0;
      //   break;

      //case 2:
      //   // Green
      //   color[0] = color[2] = 0;
      //   break;

      //case 3:
      //   // Blue
      //   color[0] = color[1] = 0;
      //   break;

      //case 4:
      //   // Pink
      //   color[1] = 0;
      //   break;
      //}

      //if( i == 0 )
      //   bar = pDL->getWritableBits(i);
      //else
      //{
      //   for( int i = 0; i < bmpSize; i++ )
      //      dMemcpy( &bar[i * pDL->bytesPerPixel], color, pDL->bytesPerPixel );
      //}
//------------------------------------------------------------------------------

      // NOTE: This code is slow. If it turns out that re-uploading textures occurs frequently
      // this should be changed.

      U8 *bPtr = new U8[pDL->bytesPerPixel * pDL->getWidth(i) * pDL->getHeight( i )];
      U8 *svBPtr = bPtr;

      U32 bufferPitch = pDL->getWidth(i) * pDL->bytesPerPixel;

      if( pDL->getFormat() == GFXFormatR8G8B8 && texture->mFormat == GFXFormatR8G8B8 ) // 24-24 bit, just needs swizzle
         GFX->getDeviceSwizzle24()->ToBuffer( bPtr, pDL->getBits(i), pDL->getWidth(i) * pDL->getHeight(i) * pDL->bytesPerPixel );
      else if( pDL->getFormat() == GFXFormatR8G8B8 && texture->mFormat == GFXFormatR8G8B8X8 )
      {
         // The card wants a 32-bit texture, we are providing a 24-bit texture. 
         AssertFatal( !GFX->getCardProfiler()->queryProfile( "allowRGB24BitTextures", false ), 
            "D3D9 texture is RGBX, GBitmap is RGB, card profiler thinks 24-bit RGB textures are allowed. Something is wrong." );

         U32 pixels = pDL->getHeight( i ) * pDL->getWidth( i );

         // This is terrible. Copy in the initial data.
         dMemcpy( bPtr, pDL->getBits(i), pDL->bytesPerPixel * pDL->getWidth(i) * pDL->getHeight(i) );

         // Pad to 32 bits. This will cause a re-allocation so...
         bitmapConvertRGB_to_RGBX( &bPtr, pixels );

         // ...update svBPtr so it will delete the correct memory
         svBPtr = bPtr;

         // And update the buffer pitch so the copy works properly
         bufferPitch = pDL->getWidth( i ) * 4; // 32-bit format, so 4 bpp

         // Now, a third copy...swizzle in place
         GFX->getDeviceSwizzle32()->InPlace( bPtr, bufferPitch * pDL->getHeight(i) );
      }
      else if( pDL->getFormat() == GFXFormatR8G8B8A8 || pDL->getFormat() == GFXFormatR8G8B8X8 )
         GFX->getDeviceSwizzle32()->ToBuffer( bPtr, pDL->getBits(i), pDL->getWidth(i) * pDL->getHeight(i) * pDL->bytesPerPixel );
      else 
         bPtr = const_cast<U8*>(pDL->getBits(i)); 

      D3D9Assert( GFXD3DX.D3DXLoadSurfaceFromMemory( surf, NULL, NULL /*&rect*/ ,
         bPtr, GFXD3D9TextureFormat[texture->mFormat],
         bufferPitch,
         NULL, &rect, 
#ifdef TORQUE_OS_XENON
         false, 0, 0, // Unique to Xenon -pw
#endif
         D3DX_FILTER_TRIANGLE|D3DX_FILTER_DITHER, 0 ), "Failed to load surface" );

      surf->Release();
      ++i;

      delete [] svBPtr;
   }

   return true;          
}

//-----------------------------------------------------------------------------
// loadTexture - raw
//-----------------------------------------------------------------------------
bool GFXD3D9TextureManager::_loadTexture( GFXTextureObject *inTex, void *raw )
{
   GFXD3D9TextureObject *texture = (GFXD3D9TextureObject *) inTex;

   // currently only for volume textures...
   if( texture->getDepth() < 1 ) return false;

   
   U32 bytesPerPix = 1;

   switch( texture->mFormat )
   {
      case GFXFormatR8G8B8:
         bytesPerPix = 3;
         break;
      case GFXFormatR8G8B8A8:
      case GFXFormatR8G8B8X8:
         bytesPerPix = 4;
         break;
   }

   U32 rowPitch = texture->getWidth() * bytesPerPix;
   U32 slicePitch = texture->getWidth() * texture->getHeight() * bytesPerPix;

   D3DBOX box;
   box.Left    = 0;
   box.Right   = texture->getWidth();
   box.Front   = 0;
   box.Back    = texture->getDepth();
   box.Top     = 0;
   box.Bottom  = texture->getHeight();


   LPDIRECT3DVOLUME9 volume = NULL;
   D3D9Assert( texture->get3DTex()->GetVolumeLevel( 0, &volume ), "Failed to load volume" );

#ifdef TORQUE_OS_XENON
   D3DLOCKED_BOX lockedBox;
   volume->LockBox( &lockedBox, &box, 0 );
   
   dMemcpy( lockedBox.pBits, raw, slicePitch * texture->getDepth() );

   volume->UnlockBox();
#else
   D3D9Assert(
      GFXD3DX.D3DXLoadVolumeFromMemory(
         volume,
         NULL,
         NULL,
         raw,
         GFXD3D9TextureFormat[texture->mFormat],
         rowPitch,
         slicePitch,
         NULL,
         &box,
#ifdef TORQUE_OS_XENON
         false, 0, 0, 0, // Unique to Xenon -pw
#endif
         D3DX_FILTER_NONE,
         0
      ),
      "Failed to load volume texture" 
   );
#endif

   volume->Release();


   return true;
}

//-----------------------------------------------------------------------------
// refreshTexture
//-----------------------------------------------------------------------------
bool GFXD3D9TextureManager::_refreshTexture(GFXTextureObject *texture)
{
   U32 usedStrategies = 0;
   GFXD3D9TextureObject *realTex = static_cast<GFXD3D9TextureObject *>( texture );

   if(texture->mProfile->doStoreBitmap())
   {
//      SAFE_RELEASE(realTex->mD3DTexture);
//      innerCreateTexture(realTex, texture->mTextureSize.x, texture->mTextureSize.y, texture->mFormat, texture->mProfile, texture->mMipLevels);

      if(texture->mBitmap)
         _loadTexture(texture, texture->mBitmap);

      if(texture->mDDS)
         _loadTexture(texture, texture->mDDS);

      usedStrategies++;
   }

   if(texture->mProfile->isRenderTarget() || texture->mProfile->isDynamic() ||
	   texture->mProfile->isZTarget())
   {
      realTex->release();
      innerCreateTexture(realTex, texture->getHeight(), texture->getWidth(), texture->getDepth(), texture->mFormat, texture->mProfile, texture->mMipLevels);
      usedStrategies++;
   }

   AssertFatal(usedStrategies < 2, "GFXD3D9TextureManager::_refreshTexture - Inconsistent profile flags!");

   return true;
}


//-----------------------------------------------------------------------------
// freeTexture
//-----------------------------------------------------------------------------
bool GFXD3D9TextureManager::_freeTexture(GFXTextureObject *texture, bool zombify)
{
   AssertFatal(dynamic_cast<GFXD3D9TextureObject *>(texture),"Not an actual d3d texture object!");
   GFXD3D9TextureObject *tex = static_cast<GFXD3D9TextureObject *>( texture );

   // If it's a managed texture and we're zombifying, don't blast it, D3D allows
   // us to keep it.
   if(zombify && tex->isManaged)
      return true;

   tex->release();

   return true;
}

//-----------------------------------------------------------------------------
// getTotalVideoMemory
//-----------------------------------------------------------------------------
U32 GFXD3D9TextureManager::_getTotalVideoMemory()
{
   // CodeReview: There is no DirectX API call to query the total video memory.
   // This will get the available texture memory, but not the total.
   // Oh, Microsoft... [5/10/2007 Pat]
#ifndef TORQUE_OS_XENON
   return mD3DDevice->GetAvailableTextureMem();
#else
   return 512 * 1024;
#endif
}

/// Load a texture from a proper DDSFile instance.
bool GFXD3D9TextureManager::_loadTexture(GFXTextureObject *aTexture, DDSFile *dds)
{
   PROFILE_START(GFXD3DTexMan_loadTexture);

   GFXD3D9TextureObject *texture = static_cast<GFXD3D9TextureObject*>(aTexture);

   // Check with profiler to see if we can do automatic mipmap generation.
   bool supportsAutoMips = GFX->getCardProfiler()->queryProfile("autoMipMapLevel", true);

   // Settings for mipmap generation
   U32 maxDownloadMip = dds->mMipMapCount;
   U32 nbMipMapLevel  = dds->mMipMapCount;

   if(supportsAutoMips && false) // Off it goes!
   {
      maxDownloadMip = 1;
      nbMipMapLevel  = aTexture->mMipLevels;
   }

   // Fill the texture...
   U32 i = 0;
   while( i < maxDownloadMip)
   {
      PROFILE_START(GFXD3DTexMan_loadSurface);

      U32 width =  dds->getWidth(i);
      U32 height = dds->getHeight(i);

      RECT rect;
      rect.left    = 0;
      rect.right   = width;
      rect.top     = 0;
      rect.bottom  = height;

      LPDIRECT3DSURFACE9 surf = NULL;
      D3D9Assert(texture->get2DTex()->GetSurfaceLevel( i, &surf ), "Failed to get surface");

      D3DSURFACE_DESC desc;
      surf->GetDesc(&desc);

      D3D9Assert( GFXD3DX.D3DXLoadSurfaceFromMemory( surf, NULL, NULL,
         (void*)((U8*)(dds->mSurfaces[0]->mMips[i])), 
         GFXD3D9TextureFormat[dds->mFormat], dds->getPitch(i),
         NULL, &rect, 
#ifdef TORQUE_OS_XENON
         false, 0, 0, // Unique to Xenon -pw
#endif
         D3DX_FILTER_NONE, 0 ), "Failed to load surface" );

      surf->Release();
      ++i;

      PROFILE_END();
   }

   PROFILE_END();

   return true;
}
