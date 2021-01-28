//-----------------------------------------------------------------------------
// Torque Shader Engine
// 
// Copyright (c) 2003 GarageGames.Com
//-----------------------------------------------------------------------------

#ifdef _MSC_VER
#pragma warning(disable: 4996) 
#endif

#include <d3d9.h>
#include <d3dx9core.h>
#include <d3dx9tex.h>
#include "gfx/D3D/gfxD3DTextureManager.h"
#include "gfx/gfxDevice.h"
#include "gfx/gfxCardProfile.h"
#include "gfx/ddsFile.h"
#include "platform/profiler.h"
#include "math/mathUtils.h"
#include "gfx/d3d/gfxD3DEnumTranslate.h"
#include "core/unicode.h"

// Gross hack to let the diag utility know that we only need stubs
#define DUMMYDEF
#include "gfx/D3D/DXDiagNVUtil.h"

//-----------------------------------------------------------------------------
// Utility function, valid only in this file
#ifdef D3D_TEXTURE_SPEW
U32 GFXD3DTextureObject::mTexCount = 0;
#endif

#ifdef TORQUE_DEBUG
#include "dxerr9.h"
#endif

inline void D3DAssert( HRESULT hr, const char *info ) 
{
#if defined( TORQUE_DEBUG )
   if( FAILED( hr ) ) 
   {
#ifdef UNICODE
      UTF16 tehInfo[256];
      convertUTF8toUTF16( (const UTF8 *)info, tehInfo, sizeof( tehInfo ) );
#else
      const char *tehInfo = info;
#endif
      DXTrace( __FILE__, __LINE__, hr, tehInfo, true );
   }
#endif
}

//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
GFXD3DTextureManager::GFXD3DTextureManager( LPDIRECT3DDEVICE9 d3ddevice ) 
{
   mD3DDevice = d3ddevice;
   dMemset( mCurTexSet, 0, sizeof( mCurTexSet ) );
}

//-----------------------------------------------------------------------------
// innerCreateTexture
//-----------------------------------------------------------------------------
void GFXD3DTextureManager::innerCreateTexture( GFXD3DTextureObject *retTex, 
                                               U32 height, 
                                               U32 width, 
                                               U32 depth,
                                               GFXFormat format, 
                                               GFXTextureProfile *profile, 
                                               U32 numMipLevels,
                                               bool forceMips)
{
   // Some relevant helper information...
   bool supportsAutoMips = GFX->getCardProfiler()->queryProfile("autoMipMapLevel", true);
   
   DWORD usage;   // 0, D3DUSAGE_RENDERTARGET, or D3DUSAGE_DYNAMIC
   D3DPOOL pool;  // D3DPOOL_DEFAULT or D3DPOOL_MANAGED

   // 24 bit textures not supported by most (or any?) cards - kick to 32
   if( format == GFXFormatR8G8B8 )
   {
      format = GFXFormatR8G8B8A8;
   }

   retTex->mProfile = profile;
   D3DFORMAT d3dTextureFormat = GFXD3DTextureFormat[format];

   if( retTex->mProfile->isDynamic() )
   {
      usage = D3DUSAGE_DYNAMIC;
      pool = D3DPOOL_DEFAULT;
   }
   else
   {
      usage = 0;
      pool = D3DPOOL_MANAGED;
   }

   if( retTex->mProfile->isRenderTarget() )
   {
      usage |= D3DUSAGE_RENDERTARGET;
      pool = D3DPOOL_DEFAULT;
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


   // 
   if( supportsAutoMips && 
       !forceMips &&
       !retTex->mProfile->isSystemMemory() &&
       numMipLevels == 0 &&
       !(depth > 0) )
   {
      usage          |= D3DUSAGE_AUTOGENMIPMAP;
   }

   // Ascertain how D3D feels about this texture request.
   /*{
      UINT lWidth = width, lHeight = height, lNumMips = D3DX_DEFAULT;
      D3DFORMAT lFormat = GFXD3DTextureFormat[format];
      DWORD lUsage = usage;
      D3DPOOL lPool  = pool;

      D3DXCheckTextureRequirements(mD3DDevice, &lWidth, &lHeight, &lNumMips, lUsage, &lFormat, lPool);
   }*/

   // Set the managed flag...
   retTex->isManaged = (pool == D3DPOOL_MANAGED);

   
   if( depth > 0 )
   {
      D3DAssert(
         D3DXCreateVolumeTexture(
            mD3DDevice,
            width,
            height,
            depth,
            numMipLevels,
            usage,
            d3dTextureFormat,
            pool,
            retTex->get3DTexPtr()
         ), "GFXD3DTextureManager::_createTexture - failed to create volume texture!"
      );

      retTex->mTextureSize.set( width, height, depth );
      retTex->mMipLevels = retTex->get3DTex()->GetLevelCount();
   }
   else
   {
      bool fastCreate = true;

      // Check for power of 2 textures - this is a problem with FX 5xxx cards
      // with current drivers - 3/2/05
      if( !MathUtils::isPow2(width) || !MathUtils::isPow2(height) )
      {
         fastCreate = false;
      }

	  if(retTex->mProfile->isZTarget())
	  {
		  D3DAssert(mD3DDevice->CreateDepthStencilSurface(width, height, GFXD3DTextureFormat[format],
			  D3DMULTISAMPLE_NONE, 0, TRUE, retTex->getSurfacePtr(), NULL), "Failed to create Z surface" );

		  // Get the actual size of the texture...
		  D3DSURFACE_DESC probeDesc;
		  D3DAssert(retTex->getSurface()->GetDesc(&probeDesc), "Failed to get surface description");
		  retTex->mTextureSize.set(probeDesc.Width, probeDesc.Height, 0);
	  }
	  else
	  {
		  // Try to create the texture directly - should gain us a bit in high
		  // performance cases where we know we're creating good stuff and we
		  // don't want to bother with D3DX - slow function.
		  HRESULT res;
		  if( fastCreate )
		  {
			  res= mD3DDevice->CreateTexture(width, height, numMipLevels, usage, GFXD3DTextureFormat[format], pool, retTex->get2DTexPtr(), NULL);
		  }

		  if( !fastCreate || (res != D3D_OK) )
		  {
			  D3DAssert(
				  D3DXCreateTexture(
				  mD3DDevice,
				  width,
				  height,
				  numMipLevels,
				  usage,
				  d3dTextureFormat,
				  pool,
				  retTex->get2DTexPtr()
				  ), "GFXD3DTextureManager::_createTexture - failed to create texture!"
				  );
		  }

		  // Get the actual size of the texture...
		  D3DSURFACE_DESC probeDesc;
		  D3DAssert( retTex->get2DTex()->GetLevelDesc( 0, &probeDesc ), "Failed to get surface description");
		  retTex->mTextureSize.set(probeDesc.Width, probeDesc.Height, 0);

		  // All done!
		  retTex->mMipLevels = retTex->get2DTex()->GetLevelCount();
	  }
   }

   retTex->mFormat = format;
}

//-----------------------------------------------------------------------------
// createTexture
//-----------------------------------------------------------------------------
GFXTextureObject *GFXD3DTextureManager::_createTexture( U32 height, 
                                                        U32 width,
                                                        U32 depth,
                                                        GFXFormat format, 
                                                        GFXTextureProfile *profile, 
                                                        U32 numMipLevels,
                                                        bool forceMips)
{
   GFXD3DTextureObject *retTex = new GFXD3DTextureObject( GFX, profile );

   innerCreateTexture(retTex, height, width, depth, format, profile, numMipLevels, forceMips);

   return retTex;
}

//-----------------------------------------------------------------------------
// loadTexture - GBitmap
//-----------------------------------------------------------------------------
bool GFXD3DTextureManager::_loadTexture(GFXTextureObject *aTexture, GBitmap *pDL)
{
   GFXD3DTextureObject *texture = static_cast<GFXD3DTextureObject*>(aTexture);

   // Check with profiler to see if we can do automatic mipmap generation.
   bool supportsAutoMips = GFX->getCardProfiler()->queryProfile("autoMipMapLevel", true);

   if(!supportsAutoMips && pDL->getNumMipLevels() == 1)
      pDL->extrudeMipLevels(false);

   // Convert images
   pDL->convertToBGRx(); // This does checking for format inside it, fear not

   // Settings for mipmap generation
   U32 maxDownloadMip = pDL->getNumMipLevels();
   U32 nbMipMapLevel  = pDL->getNumMipLevels();

   if(supportsAutoMips)
   {
      maxDownloadMip = 1;
      nbMipMapLevel  = aTexture->mMipLevels;
   }

   // Make sure we have a texture - we might be getting called in a resurrect


   // Fill the texture...
   U32 i = 0, firstMip = 0;
   while( i < maxDownloadMip )
   {
      U32 width = pDL->getWidth(i), height = pDL->getHeight(i);

      RECT rect;
      rect.left    = 0;
      rect.right   = pDL->getWidth(i);
      rect.top     = 0;
      rect.bottom  = pDL->getHeight(i);

      LPDIRECT3DSURFACE9 surf = NULL;
      D3DAssert(texture->get2DTex()->GetSurfaceLevel( i, &surf ), "Failed to get surface");

      D3DAssert( D3DXLoadSurfaceFromMemory( surf, NULL, NULL /*&rect*/ ,
         pDL->getBits(i), GFXD3DTextureFormat[pDL->getFormat()],
         pDL->getWidth(i) * pDL->bytesPerPixel,
         NULL, &rect, D3DX_FILTER_NONE, 0 ), "Failed to load surface" );

      surf->Release();
      ++i;
   }

   return true;          
}

//-----------------------------------------------------------------------------
// loadTexture - raw
//-----------------------------------------------------------------------------
bool GFXD3DTextureManager::_loadTexture( GFXTextureObject *inTex, void *raw )
{
   GFXD3DTextureObject *texture = (GFXD3DTextureObject *) inTex;

   // currently only for volume textures...
   if( texture->getDepth() < 1 ) return false;

   
   U32 bytesPerPix = 1;

   switch( texture->mFormat )
   {
      case GFXFormatR8G8B8:
         bytesPerPix = 3;
         break;
      case GFXFormatR8G8B8A8:
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
   D3DAssert( texture->get3DTex()->GetVolumeLevel( 0, &volume ), "Failed to load volume" );


   D3DAssert(
      D3DXLoadVolumeFromMemory(
         volume,
         NULL,
         NULL,
         raw,
         GFXD3DTextureFormat[texture->mFormat],
         rowPitch,
         slicePitch,
         NULL,
         &box,
         D3DX_FILTER_NONE,
         0
      ),
      "Failed to load volume texture" 
   );

   volume->Release();


   return true;
}

//-----------------------------------------------------------------------------
// refreshTexture
//-----------------------------------------------------------------------------
bool GFXD3DTextureManager::_refreshTexture(GFXTextureObject *texture)
{
   U32 usedStrategies = 0;
   GFXD3DTextureObject *realTex = static_cast<GFXD3DTextureObject *>( texture );

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

   AssertFatal(usedStrategies < 2, "GFXD3DTextureManager::_refreshTexture - Inconsistent profile flags!");

   return true;
}


//-----------------------------------------------------------------------------
// freeTexture
//-----------------------------------------------------------------------------
bool GFXD3DTextureManager::_freeTexture(GFXTextureObject *texture, bool zombify)
{
   AssertFatal(dynamic_cast<GFXD3DTextureObject *>(texture),"Not an actual d3d texture object!");
   GFXD3DTextureObject *tex = static_cast<GFXD3DTextureObject *>( texture );

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
U32 GFXD3DTextureManager::_getTotalVideoMemory()
{
   int mem;
   NVDXDiagWrapper::DWORD numDevices;

   // WANG == wise and nifty gnome.
   NVDXDiagWrapper::DXDiagNVUtil * wang = new NVDXDiagWrapper::DXDiagNVUtil();

   wang->InitIDxDiagContainer();
   wang->GetNumDisplayDevices(&numDevices);
   wang->GetDisplayDeviceMemoryInMB(0, &mem);
   wang->FreeIDxDiagContainer();

   delete wang;

   // It returns megs so scale up...
   return mem * 1024 * 1024;
}

/// Load a texture from a proper DDSFile instance.
bool GFXD3DTextureManager::_loadTexture(GFXTextureObject *aTexture, DDSFile *dds)
{
   PROFILE_START(GFXD3DTexMan_loadTexture);

   GFXD3DTextureObject *texture = static_cast<GFXD3DTextureObject*>(aTexture);

   // Ok, check to see what sort of mipmap strategy we have to use...
   if(!dds->mFlags.test(DDSFile::MipMapsFlag))
   {
      // Great, we have to generate our own mips.
      AssertISV(false, "Ben is lazy. No mip generation, and your image doesn't have 'em.");
   }

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
   U32 i = 0, firstMip = 0;
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
      D3DAssert(texture->get2DTex()->GetSurfaceLevel( i, &surf ), "Failed to get surface");

      D3DSURFACE_DESC desc;
      surf->GetDesc(&desc);

      D3DAssert( D3DXLoadSurfaceFromMemory( surf, NULL, NULL,
         (void*)((U8*)(dds->mSurfaces[0]->mMips[i])), 
         GFXD3DTextureFormat[dds->mFormat], dds->getPitch(i),
         NULL, &rect, D3DX_FILTER_NONE, 0 ), "Failed to load surface" );

      surf->Release();
      ++i;

      PROFILE_END();
   }

   PROFILE_END();

   return true;
}


//-----------------------------------------------------------------------------
// loadDDS - hack
//-----------------------------------------------------------------------------
GFXTextureObject *GFXD3DTextureManager::_loadDDSHack( const char *fName, GFXTextureProfile *profile )
{
#ifdef UNICODE
   UTF16 filename[256];
   convertUTF8toUTF16( (const UTF8 *)fName, filename, sizeof( filename ) );
#else
   const char *filename = fName;
#endif

   GFXD3DTextureObject *retTex = new GFXD3DTextureObject( GFX, profile );

   // get file info
   D3DXIMAGE_INFO info;
   HRESULT success = D3DXGetImageInfoFromFile( filename, &info );
   if( success != D3D_OK )
   {
      Con::errorf( "Unable to load .dds file: %s", filename );
      return NULL;
   }

   if( info.ResourceType != D3DRTYPE_VOLUMETEXTURE )
   {
      D3DAssert(D3DXCreateTextureFromFile(mD3DDevice, filename, retTex->get2DTexPtr()), 
         "GFXD3DTextureManager::_loadDDSHack - failed to create texture!");

      retTex->mProfile = profile;
      retTex->isManaged = true;
      retTex->mTextureSize.set( info.Width, info.Height, info.Depth );
      retTex->mMipLevels = info.MipLevels;
      retTex->mFormat = (GFXFormat)info.Format;
   }
   else
   {   
      D3DAssert(D3DXCreateVolumeTextureFromFile(mD3DDevice, filename, retTex->get3DTexPtr()), 
         "GFXD3DTextureManager::_loadDDSHack - failed to create texture!");

      retTex->mProfile = profile;
      retTex->isManaged = true;
      retTex->mTextureSize.set( info.Width, info.Height, info.Depth );
      retTex->mMipLevels = info.MipLevels;
      retTex->mFormat = (GFXFormat)info.Format;
   }

   return retTex;
}
