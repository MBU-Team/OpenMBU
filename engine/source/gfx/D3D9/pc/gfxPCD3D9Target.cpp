//-----------------------------------------------------------------------------
// Torque Game Engine Advanced
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifdef _MSC_VER
#pragma warning(disable: 4996) // turn off "deprecation" warnings
#endif

#include "gfx/D3D9/gfxD3D9Device.h"
#include "gfx/D3D9/pc/gfxPCD3D9Target.h"
#include "gfx/D3D9/gfxD3D9TextureObject.h"
#include "gfx/D3D9/gfxD3D9Cubemap.h"
#include "platformWin32/win32WinMgr.h"

GFXPCD3D9WindowTarget::GFXPCD3D9WindowTarget()
{
   mSwapChain    = NULL;
   mDepthStencil = NULL;
   //mWindow       = NULL;
   mDevice       = NULL;
   mBackbuffer   = NULL;
}

GFXPCD3D9WindowTarget::~GFXPCD3D9WindowTarget()
{
   SAFE_RELEASE(mSwapChain);
   SAFE_RELEASE(mDepthStencil);
   SAFE_RELEASE(mBackbuffer);
}

void GFXPCD3D9WindowTarget::initPresentationParams()
{
   // Clear everything to safe values.
   dMemset( &mPresentationParams, 0, sizeof( mPresentationParams ) );

   // Get some video mode related info.
    AssertFatal(winState.videoMode, "GFXPCD3D9WindowTarget::initPresentationParams - No video mode set!");
   GFXVideoMode vm = *winState.videoMode;//mWindow->getCurrentMode();

   // Do some validation...

	   // The logic in the Assert below is faulty due to how we currently start up the client
	   // What we seem to want here is an actual identifier of primary or secondary window, but instead
	   // we are relying on if we are creating a second window with a previously created device (see
	   // gfxPCDD9Device.cpp/GFXPCD3D9Device::allocWindowTarget, which is the only place in the code
	   // we currently set a value for mImplicit.
	   //
	   // Until we have a better way to identify the status of a window (primary or secondary), the following
	   // "validation" should be commented out. SRZ 1/25/08
       //
       // NOTE: The following applies to the D38 device as well.
#if 0
   if(vm.fullScreen == true && mImplicit == false)
   {

      AssertISV(false, 
         "GFXPCD3D9WindowTarget::initPresentationParams - Cannot go fullscreen with secondary window!");
   }
#endif

   // Figure our backbuffer format.
   D3DFORMAT fmt = D3DFMT_UNKNOWN; 
   
   if(vm.fullScreen)
   {
      // We can't default to the desktop bitdepth if we're fullscreen - actually
      // set something.

      if( vm.bitDepth == 16 )
         fmt = D3DFMT_R5G6B5; // 16 bit
      else
         fmt = D3DFMT_X8R8G8B8; // 32 bit
   }

   // And fill out our presentation params.
   mPresentationParams.BackBufferWidth  = vm.resolution.x;
   mPresentationParams.BackBufferHeight = vm.resolution.y;
   mPresentationParams.BackBufferFormat = fmt;
   mPresentationParams.BackBufferCount  = 1;
   if (vm.antialiasLevel == 0)
   {
      mPresentationParams.MultiSampleType = D3DMULTISAMPLE_NONE;
      mPresentationParams.MultiSampleQuality = 0;
   } else {
      mPresentationParams.MultiSampleType = D3DMULTISAMPLE_NONMASKABLE;
      mPresentationParams.MultiSampleQuality = vm.antialiasLevel-1;
   }
   mPresentationParams.SwapEffect       = D3DSWAPEFFECT_DISCARD;
   mPresentationParams.hDeviceWindow    =  winState.appWindow;//mWindow->getHWND();
   mPresentationParams.Windowed         = (vm.fullScreen ? FALSE : TRUE);
   mPresentationParams.EnableAutoDepthStencil = TRUE;
   mPresentationParams.AutoDepthStencilFormat = D3DFMT_D24S8;
   mPresentationParams.Flags            = 0;
   mPresentationParams.FullScreen_RefreshRateInHz = (vm.refreshRate == 0 || !vm.fullScreen) ? 
                                                     D3DPRESENT_RATE_DEFAULT : vm.refreshRate;

    mPresentationParams.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;	// This does NOT wait for vsync

    if( Con::getBoolVariable( "$pref::Video::VSync", true ) )
    {
        mPresentationParams.PresentationInterval = D3DPRESENT_INTERVAL_ONE;
    }
}

/*PlatformWindow * GFXPCD3D9WindowTarget::getWindow()
{
   return mWindow;
}*/

const Point2I GFXPCD3D9WindowTarget::getSize()
{
   return Point2I(winState.windowManager->mActualWidth, winState.windowManager->mActualHeight);//mWindow->getBounds().extent;
}

bool GFXPCD3D9WindowTarget::present()
{
   AssertFatal(mSwapChain, "GFXPCD3D9WindowTarget::present - no swap chain present to present!");
   HRESULT res = mSwapChain->Present(NULL, NULL, NULL, NULL, NULL);

   return (res == S_OK);
}

void GFXPCD3D9WindowTarget::resetMode()
{
    // TODO: What is this?
   //mWindow->mSuppressReset = true;

   // Setup our presentation params.
   initPresentationParams();

   // So, the video mode has changed - if we're an additional swap chain
   // just kill the swapchain and reallocate to match new vid mode.
   if(mImplicit == false)
   {
      // Since we're not going to do a device reset for an additional swap
      // chain, we can just release our resources and regrab them.
      SAFE_RELEASE(mSwapChain);
      SAFE_RELEASE(mDepthStencil);
      SAFE_RELEASE(mBackbuffer);

      D3D9Assert(mDevice->getDevice()->CreateAdditionalSwapChain(&mPresentationParams, &mSwapChain),
         "GFXPCD3D9WindowTarget::resetMode - couldn't reallocate additional swap chain!");
      D3D9Assert(mDevice->getDevice()->CreateDepthStencilSurface(mPresentationParams.BackBufferWidth, mPresentationParams.BackBufferHeight,
         D3DFMT_D24S8, mPresentationParams.MultiSampleType, mPresentationParams.MultiSampleQuality, false, &mDepthStencil, NULL),          
         "Unable to create stencil/depth surface");
      mSwapChain->GetBackBuffer(0, D3DBACKBUFFER_TYPE_MONO, &mBackbuffer);
   }
   else
   {
      // Otherwise, we have to reset the device, if we're the implicit swapchain.
      mDevice->reset(mPresentationParams);
   }

   // Update our size, too.
   mSize = Point2I(mPresentationParams.BackBufferWidth, mPresentationParams.BackBufferHeight);

   // TODO: What is this?
   //mWindow->mSuppressReset = false;
}

void GFXPCD3D9WindowTarget::zombify()
{
   // Release our resources
   SAFE_RELEASE(mSwapChain);
   SAFE_RELEASE(mDepthStencil);
   SAFE_RELEASE(mBackbuffer);
}

void GFXPCD3D9WindowTarget::resurrect()
{
   if(mImplicit)
   {
      // Reacquire our swapchain & DS
      if(!mSwapChain)
         mDevice->getDevice()->GetSwapChain(0, &mSwapChain);

      if(!mDepthStencil)
         mDevice->getDevice()->GetDepthStencilSurface(&mDepthStencil);

      if (!mBackbuffer)      
         mSwapChain->GetBackBuffer(0, D3DBACKBUFFER_TYPE_MONO, &mBackbuffer);         

      // Moved this from D3D device into target where it belongs
      D3DSURFACE_DESC desc;
      D3D9Assert( mBackbuffer->GetDesc( &desc ), "GFXPCD3D9WindowTarget::resurrect - Couldn't get backbuffer description" );

      //Con::errorf("setting window video mode to be %dx%d", desc.Width, desc.Height);
      //static_cast<Win32Window *>( getWindow() )->mVideoMode.resolution.x = desc.Width;
      //static_cast<Win32Window *>( getWindow() )->mVideoMode.resolution.y = desc.Height;
       AssertFatal(winState.videoMode, "GFXPCD3D9WindowTarget::resurrect - No video mode set!");
      winState.videoMode->resolution.x = desc.Width;
       winState.videoMode->resolution.y = desc.Width;
   }
   else
   {
      if(!mSwapChain)
      {
         initPresentationParams();         
         D3D9Assert(mDevice->getDevice()->CreateAdditionalSwapChain(&mPresentationParams, &mSwapChain), 
            "Unable to create additional swap chain!");         
         D3D9Assert(mDevice->getDevice()->CreateDepthStencilSurface(mPresentationParams.BackBufferWidth, mPresentationParams.BackBufferHeight, 
            D3DFMT_D24S8, mPresentationParams.MultiSampleType, mPresentationParams.MultiSampleQuality, false, &mDepthStencil, NULL),             
            "Unable to create z/stencil for additional swap chain!");
         mSwapChain->GetBackBuffer(0, D3DBACKBUFFER_TYPE_MONO, &mBackbuffer);         
      }
   }
}

// ----------------------------------------------------------------------------

GFXPCD3D9TextureTarget::GFXPCD3D9TextureTarget()
{
   for(S32 i=0; i<MaxRenderSlotId; i++)
   {
      mTargets[i] = NULL;
      mTextures[i] = NULL;
   }    
}

GFXPCD3D9TextureTarget::~GFXPCD3D9TextureTarget()
{
   // Release anything we might be holding.
   for(S32 i=0; i<MaxRenderSlotId; i++)
   {
      if( GFXDevice::devicePresent() )
      {
         static_cast<GFXD3D9Device *>( GFX )->destroyD3DResource( mTargets[i] );
         mTargets[i] = NULL;
      }
      else
         SAFE_RELEASE( mTargets[i] );
   }
}

const Point2I GFXPCD3D9TextureTarget::getSize()
{
   if(!mTargets[Color0])
      return Point2I(0,0);

   // Query the first surface.
   D3DSURFACE_DESC sd;
   mTargets[Color0]->GetDesc(&sd);
   return Point2I(sd.Width, sd.Height);
}

void GFXPCD3D9TextureTarget::attachTexture( RenderSlot slot, GFXTextureObject *tex, U32 mipLevel/*=0*/, U32 zOffset /*= 0*/ )
{
   AssertFatal(slot < MaxRenderSlotId, "GFXPCD3D9TextureTarget::attachTexture - out of range slot.");

   // Mark state as dirty so device can know to update.
   invalidateState();

   // Release what we had, it's definitely going to change.
   static_cast<GFXD3D9Device *>( GFX )->destroyD3DResource( mTargets[slot] );
   mTargets[slot] = NULL;
   mTextures[slot] = NULL;

   // Are we clearing?
   if(!tex)
   {
      // Yup - just exit, it'll stay NULL.      
      return;
   }

   // Take care of default targets
   if( tex == GFXTextureTarget::sDefaultDepthStencil )
   {
      mTargets[slot] = static_cast<GFXD3D9Device *>(mDevice)->mDeviceDepthStencil;
      mTargets[slot]->AddRef();
   }
   else
   {
      // Cast the texture object to D3D...
      AssertFatal(dynamic_cast<GFXD3D9TextureObject*>(tex), 
         "GFXPCD3D9TextureTarget::attachTexture - invalid texture object.");

      GFXD3D9TextureObject *d3dto = static_cast<GFXD3D9TextureObject*>(tex);

      // Grab the surface level.
      if( slot == DepthStencil )
      {
         mTargets[slot] = d3dto->getSurface();
         mTargets[slot]->AddRef();
      }
      else
      {
         if (!d3dto->mProfile->isRenderTargetZBuffer() || d3dto->getSurface() == NULL)
         {
            D3D9Assert(d3dto->get2DTex()->GetSurfaceLevel(mipLevel, &mTargets[slot]), 
               "GFXD3D9TextureTarget::attachTexture - could not get surface level for the passed texture!");
         } else {
            mTargets[slot] = d3dto->getSurface();
            mTargets[slot]->AddRef();
            mTextures[slot] = d3dto;
         }                                
      }
   }
}

void GFXPCD3D9TextureTarget::attachTexture( RenderSlot slot, GFXCubemap *tex, U32 face, U32 mipLevel/*=0*/ )
{
   AssertFatal(slot < MaxRenderSlotId, "GFXPCD3D9TextureTarget::attachTexture - out of range slot.");

   // Mark state as dirty so device can know to update.
   invalidateState();

   // Release what we had, it's definitely going to change.
   static_cast<GFXD3D9Device *>( GFX )->destroyD3DResource( mTargets[slot] );
   mTargets[slot] = NULL;
   mTextures[slot] = NULL;

   // Are we clearing?
   if(!tex)
   {
      // Yup - just exit, it'll stay NULL.      
      return;
   }

   // Cast the texture object to D3D...
   AssertFatal(dynamic_cast<GFXD3D9Cubemap*>(tex), 
      "GFXD3DTextureTarget::attachTexture - invalid cubemap object.");

   GFXD3D9Cubemap *cube = static_cast<GFXD3D9Cubemap*>(tex);

   D3D9Assert(cube->mCubeTex->GetCubeMapSurface( (D3DCUBEMAP_FACES)face, mipLevel, &mTargets[slot] ),
      "GFXD3DTextureTarget::attachTexture - could not get surface level for the passed texture!");
}

void GFXPCD3D9TextureTarget::clearAttachments()
{
   for(S32 i=0; i<MaxRenderSlotId; i++)
      attachTexture((RenderSlot)i, NULL);
}

void GFXPCD3D9TextureTarget::deactivate()
{
   for (U32 i = 0; i < MaxRenderSlotId; i++)
   {
      // We use existance @ mTextures as a flag that we need to copy
      // data from the rendertarget into the texture.
      if (mTextures[i])
      {
         IDirect3DSurface9 *surf;         
         mTextures[i]->get2DTex()->GetSurfaceLevel( 0, &surf );         
         AssertFatal(dynamic_cast<GFXD3D9Device*>(getOwningDevice()), "Incorrect device type!");
         GFXD3D9Device* d = static_cast<GFXD3D9Device*>(getOwningDevice());
         d->getDevice()->StretchRect(mTargets[i], NULL, surf, NULL, D3DTEXF_NONE );         
         surf->Release();
      }
   }
}

void GFXPCD3D9TextureTarget::zombify()
{
   // Dump our attachments and damn the consequences.
   clearAttachments();
}

void GFXPCD3D9TextureTarget::resurrect()
{

}
