//-----------------------------------------------------------------------------
// Torque Game Engine Advanced
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "gfx/D3D9/pc/gfxPCD3D9Device.h"
#include "gfx/D3D9/pc/gfxPCD3D9CardProfiler.h"
#include "gfx/D3D9/gfxD3D9EnumTranslate.h"
#include "platformWin32/platformWin32.h"
//#include "windowManager/win32/win32Window.h"
#include "gfx/D3D9/pc/gfxPCD3D9CardProfiler.h"
#include "gfx/d3d9/pc/gfxPCD3D9Target.h"
#include "gfx/screenshot.h"
#include "gfx/D3D/screenshotD3D.h"
#include "platformWin32/platformWin32.h"
#include "platformWin32/win32WinMgr.h"

//------------------------------------------------------------------------------

GFXDevice *GFXPCD3D9Device::createInstance( U32 adapterIndex )
{
   if( !smD3D9.isLoaded )
      initD3D9FnTable();

   return new GFXPCD3D9Device( smD3D9.Direct3DCreate9( D3D_SDK_VERSION ), adapterIndex );
}

//------------------------------------------------------------------------------

GFXPCD3D9Device::~GFXPCD3D9Device()
{
   SAFE_DELETE( mCardProfiler );
}

//------------------------------------------------------------------------------

GFXFormat GFXPCD3D9Device::selectSupportedFormat(GFXTextureProfile *profile,
		const Vector<GFXFormat> &formats, bool texture, bool mustblend)
{
	DWORD usage = 0;

	if(profile->isDynamic())
		usage |= D3DUSAGE_DYNAMIC;

	if(profile->isRenderTarget())
		usage |= D3DUSAGE_RENDERTARGET;

	if(profile->isZTarget())
		usage |= D3DUSAGE_DEPTHSTENCIL;

	if(mustblend)
		usage |= D3DUSAGE_QUERY_POSTPIXELSHADER_BLENDING;

	D3DDISPLAYMODE mode;
	D3D9Assert(mD3D->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &mode), "Unable to get adapter mode.");

	D3DRESOURCETYPE type;
	if(texture)
		type = D3DRTYPE_TEXTURE;
	else
		type = D3DRTYPE_SURFACE;

	for(U32 i=0; i<formats.size(); i++)
	{
		if(mD3D->CheckDeviceFormat(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, mode.Format,
			usage, type, GFXD3D9TextureFormat[formats[i]]) == D3D_OK)
			return formats[i];
	}

	return GFXFormatR8G8B8A8;
}

//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Setup D3D present parameters - init helper function
//-----------------------------------------------------------------------------
D3DPRESENT_PARAMETERS GFXPCD3D9Device::setupPresentParams( const GFXVideoMode &mode, const HWND &hwnd )
{
   // Create D3D Presentation params
   D3DPRESENT_PARAMETERS d3dpp; 
   dMemset( &d3dpp, 0, sizeof( d3dpp ) );

   D3DFORMAT fmt = D3DFMT_X8R8G8B8; // 32 bit

   if( mode.bitDepth == 16 )
   {
      fmt = D3DFMT_R5G6B5;
   } 

   if (mode.antialiasLevel == 0)
   {
      mMultisampleType = D3DMULTISAMPLE_NONE;
      mMultisampleLevel = 0;
   } else {
      mMultisampleType = D3DMULTISAMPLE_NONMASKABLE;
      mMultisampleLevel = mode.antialiasLevel-1;
   }
   validateMultisampleParams(fmt);

   d3dpp.BackBufferWidth  = mode.resolution.x;
   d3dpp.BackBufferHeight = mode.resolution.y;
   d3dpp.BackBufferFormat = fmt;
   d3dpp.BackBufferCount  = 1;
   d3dpp.MultiSampleType  = getMultisampleType();
   d3dpp.MultiSampleQuality = getMultisampleLevel();
   d3dpp.SwapEffect       = D3DSWAPEFFECT_DISCARD;
   d3dpp.hDeviceWindow    = hwnd;
   d3dpp.Windowed         = !mode.fullScreen;
   d3dpp.EnableAutoDepthStencil = TRUE;
   d3dpp.AutoDepthStencilFormat = D3DFMT_D24S8;
   d3dpp.Flags            = 0;
   d3dpp.FullScreen_RefreshRateInHz = (mode.refreshRate == 0 || !mode.fullScreen) ? 
                                       D3DPRESENT_RATE_DEFAULT : mode.refreshRate;

   d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;	// This does NOT wait for vsync

   if( Con::getBoolVariable( "$pref::Video::VSync", true ) )
   {
       d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_ONE;
   }

   return d3dpp;
}

void GFXPCD3D9Device::enumerateVideoModes() 
{
   Vector<D3DFORMAT> formats;
   formats.push_back( D3DFMT_R5G6B5 );    // D3DFMT_R5G6B5 - 16bit format
   formats.push_back( D3DFMT_X8R8G8B8 );  // D3DFMT_X8R8G8B8 - 32bit format

   for( S32 i = 0; i < formats.size(); i++ ) 
   {
      DWORD MaxSampleQualities;
      mD3D->CheckDeviceMultiSampleType(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, formats[i], FALSE, D3DMULTISAMPLE_NONMASKABLE, &MaxSampleQualities);

      for( U32 j = 0; j < mD3D->GetAdapterModeCount( D3DADAPTER_DEFAULT, formats[i] ); j++ ) 
      {
         D3DDISPLAYMODE mode;
         mD3D->EnumAdapterModes( D3DADAPTER_DEFAULT, formats[i], j, &mode );

         GFXVideoMode toAdd;

         toAdd.bitDepth = ( i == 0 ? 16 : 32 ); // This will need to be changed later
         toAdd.fullScreen = false;
         toAdd.refreshRate = mode.RefreshRate;
         toAdd.resolution = Point2I( mode.Width, mode.Height );
         toAdd.antialiasLevel = MaxSampleQualities;

         mVideoModes.push_back( toAdd );
      }
   }
}

/// Sets the video mode for the device
void GFXPCD3D9Device::setVideoMode( const GFXVideoMode &mode )
{
    // set this before the reset - some modules like the GlowBuffer need to
    // resize screen buffers to the new video mode ( during reset() )
    mVideoMode = mode;

    D3DPRESENT_PARAMETERS d3dpp = setupPresentParams(mode, winState.appWindow);
    reset(d3dpp);

    Platform::setWindowSize(mode.resolution.x, mode.resolution.y, mode.fullScreen, mode.borderless);

    // Setup default states
    initStates();

    GFX->updateStates(true);
}

//-----------------------------------------------------------------------------
// Initialize - create window, device, etc
//-----------------------------------------------------------------------------
void GFXPCD3D9Device::init( const GFXVideoMode &mode/*, PlatformWindow *window*/ /* = NULL */ )
{
#ifdef TORQUE_SHIPPING
   // Check DX Version here, bomb if we don't have the minimum. 
   // Check platformWin32/dxVersionChecker.cpp for more information/configuration.
   AssertISV( checkDXVersion(), "Minimum DirectX version required to run this application not found. Quitting." );
#endif

   //AssertFatal(window, "GFXPCD3D9Device::init - must specify a window!");

   initD3DXFnTable();

   mVideoMode = mode;

   //Win32Window *win = dynamic_cast<Win32Window *>( window );
   //AssertISV( win, "GFXD3D9Device::init - got a non Win32Window window passed in! Did DX go crossplatform?" );

   //HWND winHwnd = win->getHWND();
    HWND winHwnd = winState.appWindow;

   // Create D3D Presentation params
   D3DPRESENT_PARAMETERS d3dpp = setupPresentParams( mode, winHwnd );
   
   bool useNVPerfHud = false;
#ifndef TORQUE_SHIPPING
   useNVPerfHud = Con::getBoolVariable("$Video::useNVPerfHud", false);
#endif

HRESULT hres = E_FAIL;
   if (useNVPerfHud)
   {   
      hres = mD3D->CreateDevice( mD3D->GetAdapterCount() - 1, D3DDEVTYPE_REF, 
              winHwnd,
              D3DCREATE_MIXED_VERTEXPROCESSING | D3DCREATE_MULTITHREADED,
              &d3dpp, &mD3DDevice ); 
   }
   else
   {
      // Vertex processing was changed from MIXED to HARDWARE because of the switch to a pure D3D device.

      // Set up device flags from our compile flags.
      U32 deviceFlags = 0;
      deviceFlags = D3DCREATE_HARDWARE_VERTEXPROCESSING;

      // Add the multithread flag if we're a multithread build.
   #ifdef TORQUE_MULTITHREAD
      deviceFlags |= D3DCREATE_MULTITHREADED;
   #endif

      // Try to do pure, unless we're doing debug (and thus doing more paranoid checking).
   #ifndef TORQUE_DEBUG_RENDER
      deviceFlags |= D3DCREATE_PUREDEVICE;
   #endif

      hres = mD3D->CreateDevice( D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, winHwnd, deviceFlags, &d3dpp, &mD3DDevice );

      if (FAILED(hres) && hres != D3DERR_OUTOFVIDEOMEMORY)
      {
         Con::errorf("   Failed to create hardware device, trying mixed device");
         // turn off pure
         deviceFlags &= (~D3DCREATE_PUREDEVICE);

         // try mixed mode
         deviceFlags &= (~D3DCREATE_HARDWARE_VERTEXPROCESSING);
         deviceFlags |= D3DCREATE_MIXED_VERTEXPROCESSING;
         hres = mD3D->CreateDevice( D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL,
               winHwnd, deviceFlags,
               &d3dpp, &mD3DDevice );

         // try software 
         if (FAILED(hres) && hres != D3DERR_OUTOFVIDEOMEMORY)
         {
            Con::errorf("   Failed to create mixed mode device, trying software device");
            deviceFlags &= (~D3DCREATE_MIXED_VERTEXPROCESSING);
            deviceFlags |= D3DCREATE_SOFTWARE_VERTEXPROCESSING;
            hres = mD3D->CreateDevice( D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL,
               winHwnd, deviceFlags,
               &d3dpp, &mD3DDevice );

            if (FAILED(hres) && hres != D3DERR_OUTOFVIDEOMEMORY)
               Con::errorf("   Failed to create software device, giving up");
            D3D9Assert(hres, "GFXPCD3D9Device::init - CreateDevice failed!");
         }
      }
   }
   

   // Gracefully die if they can't give us a device.
   if(!mD3DDevice)
   {
      if (hres == D3DERR_OUTOFVIDEOMEMORY)
      {
         char errorMsg[4096];
         dSprintf(errorMsg, sizeof(errorMsg),
            "Out of video memory. Close other windows, reboot, and/or upgrade your video card drivers. Your video card is: %s", getAdapter().getName());
         Platform::AlertOK("DirectX Error", errorMsg);
      }
      else if (hres == D3DERR_DEVICELOST)
      {
         char errorMsg[4096];
         dSprintf(errorMsg, sizeof(errorMsg),
            "Device lost before call to CreateDevice. Your video card is: %s", getAdapter().getName());
         Platform::AlertOK("DirectX Error", errorMsg);
      }
      else if (hres == D3DERR_INVALIDCALL)
      {
         char errorMsg[4096];
         dSprintf(errorMsg, sizeof(errorMsg),
            "Invalid call to CreateDevice. Your video card is: %s", getAdapter().getName());
         Platform::AlertOK("DirectX Error", errorMsg);
      }
      else if (hres == D3DERR_NOTAVAILABLE)
      {
         char errorMsg[4096];
         dSprintf(errorMsg, sizeof(errorMsg),
            "D3D Device is not available. Your video card is: %s", getAdapter().getName());
         Platform::AlertOK("DirectX Error", errorMsg);
      }
      else
      {
         Platform::AlertOK("DirectX Error!", "Failed to initialize Direct3D! Make sure you have DirectX 9 installed, and "
            "are running a graphics card that supports Pixel Shader 1.1.");
      }
      Platform::forceShutdown(1);
   }

   // Check up on things
   Con::printf("   D3DDevice ref count is %d", mD3DDevice->AddRef() - 1);
   mD3DDevice->Release();
   
   mTextureManager = new GFXD3D9TextureManager( mD3DDevice );
      
   // Setup default states
   initStates();

   // Now reacquire all the resources we trashed earlier
   reacquireDefaultPoolResources();

   //-------- Output init info ---------   
   D3DCAPS9 caps;
   mD3DDevice->GetDeviceCaps( &caps );
   
   U8 *pxPtr = (U8*) &caps.PixelShaderVersion;
   mPixVersion = pxPtr[1] + pxPtr[0] * 0.1;
   Con::printf( "   Pix version detected: %f", mPixVersion );

   U8 *vertPtr = (U8*) &caps.VertexShaderVersion;
   F32 vertVersion = vertPtr[1] + vertPtr[0] * 0.1;
   Con::printf( "   Vert version detected: %f", vertVersion );

   // Profile number of samplers, store and clamp to TEXTURE_STAGE_COUNT
   mNumSamplers = caps.MaxSimultaneousTextures;

   if( mNumSamplers > TEXTURE_STAGE_COUNT )
      mNumSamplers = TEXTURE_STAGE_COUNT;
   Con::printf( "   Maximum number of simultaneous samplers: %d", mNumSamplers );

   bool forcePixVersion = Con::getBoolVariable( "$pref::Video::forcePixVersion", false );
   if( forcePixVersion )
   {
      float forcedPixVersion = Con::getFloatVariable( "$pref::Video::forcedPixVersion", mPixVersion );
      if( forcedPixVersion < mPixVersion )
      {
         mPixVersion = forcedPixVersion;
         Con::errorf( "   Forced pix version: %f", mPixVersion );
         if (forcedPixVersion < 1.4)
         {
            Con::errorf("   Forcing max samplers to 4");
            mNumSamplers = 4;
         }
      }
   }

   mCardProfiler = new GFXPCD3D9CardProfiler();
   mCardProfiler->init();

   gScreenShot = new ScreenShotD3D;

   // Grab the depth-stencil...
   SAFE_RELEASE(mDeviceDepthStencil);
   D3D9Assert(mD3DDevice->GetDepthStencilSurface(&mDeviceDepthStencil), "GFXD3D9Device::init - couldn't grab reference to device's depth-stencil surface.");  

   mInitialized = true;
   deviceInited();
}

//------------------------------------------------------------------------------
void GFXPCD3D9Device::enterDebugEvent(ColorI color, const char *name)
{
   // We aren't really Unicode compliant (boo hiss BJGFIX) so we get the
   // dubious pleasure of runtime string conversion! :)
   WCHAR  eventName[260];
   MultiByteToWideChar( CP_ACP, 0, name, -1, eventName, 260 );

   smD3D9.D3DPERF_BeginEvent(D3DCOLOR_ARGB(color.alpha, color.red, color.green, color.blue),
      (LPCWSTR)&eventName);
}

//------------------------------------------------------------------------------
void GFXPCD3D9Device::leaveDebugEvent()
{
   smD3D9.D3DPERF_EndEvent();
}

//------------------------------------------------------------------------------
void GFXPCD3D9Device::setDebugMarker(ColorI color, const char *name)
{
   // We aren't really Unicode compliant (boo hiss BJGFIX) so we get the
   // dubious pleasure of runtime string conversion! :)
   WCHAR  eventName[260];
   MultiByteToWideChar( CP_ACP, 0, name, -1, eventName, 260 );

   smD3D9.D3DPERF_SetMarker(D3DCOLOR_ARGB(color.alpha, color.red, color.green, color.blue), 
      (LPCWSTR)&eventName);
}

//-----------------------------------------------------------------------------
// Copy back buffer to Sfx Back Buffer
//-----------------------------------------------------------------------------
void GFXPCD3D9Device::copyBBToSfxBuff()
{
   if( !mSfxBackBuffer || mSfxBackBuffer.getHeight() != smSfxBackBufferSize)
   {
      mSfxBackBuffer.set( smSfxBackBufferSize, smSfxBackBufferSize, GFXFormatR8G8B8, &GFXDefaultRenderTargetProfile );
   }

   IDirect3DSurface9 *surf;
   GFXD3D9TextureObject *texObj = (GFXD3D9TextureObject*)(GFXTextureObject*)mSfxBackBuffer;
   texObj->get2DTex()->GetSurfaceLevel( 0, &surf );
   //mD3DDevice->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &surf);
   if (!mCurrentRT.isNull()) {
       auto& target = *mCurrentRT;
       if (GFXPCD3D9TextureTarget* gdtt = dynamic_cast<GFXPCD3D9TextureTarget*>(&target))
       {
           IDirect3DSurface9* ss = gdtt->mTargets[GFXTextureTarget::Color0];
           mD3DDevice->StretchRect(ss, NULL, surf, NULL, D3DTEXF_NONE);
       }
   }
   // mD3DDevice->StretchRect( mDeviceBackbuffer, NULL, surf, NULL, D3DTEXF_NONE );
   
   surf->Release();
}

//-----------------------------------------------------------------------------

void GFXPCD3D9Device::setMatrix( GFXMatrixType mtype, const MatrixF &mat ) 
{
   mat.transposeTo( mTempMatrix );

   mD3DDevice->SetTransform( (_D3DTRANSFORMSTATETYPE)mtype, (D3DMATRIX *)&mTempMatrix );
}

//-----------------------------------------------------------------------------

void GFXPCD3D9Device::setTextureStageState( U32 stage, U32 state, U32 value ) 
{
   switch( state )
   {
      case GFXTSSColorOp:
      case GFXTSSAlphaOp:
         mD3DDevice->SetTextureStageState( stage, GFXD3D9TextureStageState[state], GFXD3D9TextureOp[value] );
         break;

      default:
         mD3DDevice->SetTextureStageState( stage, GFXD3D9TextureStageState[state], value );
         break;
   }
}

//------------------------------------------------------------------------------

void GFXPCD3D9Device::initStates() 
{
   //-------------------------------------
   // Auto-generated default states, see regenStates() for details
   //

   // Render states
   initRenderState( 0, 1 );
   initRenderState( 1, 3 );
   initRenderState( 2, 2 );
   initRenderState( 3, 1 );
   initRenderState( 4, 0 );
   initRenderState( 5, 1 );
   initRenderState( 6, 1 );
   initRenderState( 7, 0 );
   initRenderState( 8, 2 );
   initRenderState( 9, 3 );
   initRenderState( 10, 0 );
   initRenderState( 11, 7 );
   initRenderState( 12, 0 );
   initRenderState( 13, 0 );
   initRenderState( 14, 0 );
   initRenderState( 15, 0 );
   initRenderState( 16, 0 );
   initRenderState( 17, 0 );
   initRenderState( 18, 0 );
   initRenderState( 19, 1065353216 );
   initRenderState( 20, 1065353216 );
   initRenderState( 21, 0 );
   initRenderState( 22, 0 );
   initRenderState( 23, 0 );
   initRenderState( 24, 0 );
   initRenderState( 25, 0 );
   initRenderState( 26, 7 );
   initRenderState( 27, 0 );
   initRenderState( 28, -1 );
   initRenderState( 29, -1 );
   initRenderState( 30, -1 );
   initRenderState( 31, 0 );
   initRenderState( 32, 0 );
   initRenderState( 33, 0 );
   initRenderState( 34, 0 );
   initRenderState( 35, 0 );
   initRenderState( 36, 0 );
   initRenderState( 37, 0 );
   initRenderState( 38, 0 );
   initRenderState( 39, 1 );
   initRenderState( 40, 1 );
   initRenderState( 41, 0 );
   initRenderState( 42, 0 );
   initRenderState( 43, 1 );
   initRenderState( 44, 1 );
   initRenderState( 45, 0 );
   initRenderState( 46, 1 );
   initRenderState( 47, 2 );
   initRenderState( 48, 0 );
   initRenderState( 49, 0 );
   initRenderState( 50, 0 );
   initRenderState( 51, 0 );
   initRenderState( 52, 1065353216 );
   initRenderState( 53, 1065353216 );
   initRenderState( 54, 0 );
   initRenderState( 55, 0 );
   initRenderState( 56, 1065353216 );
   initRenderState( 57, 0 );
   initRenderState( 58, 0 );
   initRenderState( 59, 1 );
   initRenderState( 60, -1 );
   initRenderState( 61, 0 );
   initRenderState( 62, -1163015426 );
   initRenderState( 63, 1132462080 );
   initRenderState( 64, 0 );
   initRenderState( 65, 15 );
   initRenderState( 66, 0 );
   initRenderState( 67, 0 );
   initRenderState( 68, 3 );
   initRenderState( 69, 1 );
   initRenderState( 70, 0 );
   initRenderState( 71, 0 );
   initRenderState( 72, 0 );
   initRenderState( 73, 1065353216 );
   initRenderState( 74, 1065353216 );
   initRenderState( 75, 0 );
   initRenderState( 76, 0 );
   initRenderState( 77, 1065353216 );
   initRenderState( 78, 0 );
   initRenderState( 79, 0 );
   initRenderState( 80, 0 );
   initRenderState( 81, 0 );
   initRenderState( 82, 0 );
   initRenderState( 83, 0 );
   initRenderState( 84, 7 );
   initRenderState( 85, 15 );
   initRenderState( 86, 15 );
   initRenderState( 87, 15 );
   initRenderState( 88, -1 );
   initRenderState( 89, 0 );
   initRenderState( 90, 0 );
   initRenderState( 91, 0 );
   initRenderState( 92, 0 );
   initRenderState( 93, 0 );
   initRenderState( 94, 0 );
   initRenderState( 95, 0 );
   initRenderState( 96, 0 );
   initRenderState( 97, 0 );
   initRenderState( 98, 0 );
   initRenderState( 99, 0 );
   initRenderState( 100, 1 );
   initRenderState( 101, 0 );
   initRenderState( 102, 0 );

   // Texture Stage states
   initTextureState( 0, 0, 3 );
   initTextureState( 0, 1, 2 );
   initTextureState( 0, 2, 1 );
   initTextureState( 0, 3, 1 );
   initTextureState( 0, 4, 2 );
   initTextureState( 0, 5, 1 );
   initTextureState( 0, 6, 0 );
   initTextureState( 0, 7, 0 );
   initTextureState( 0, 8, 0 );
   initTextureState( 0, 9, 0 );
   initTextureState( 0, 10, 0 );
   initTextureState( 0, 11, 0 );
   initTextureState( 0, 12, 0 );
   initTextureState( 0, 13, 0 );
   initTextureState( 0, 14, 1 );
   initTextureState( 0, 15, 1 );
   initTextureState( 0, 16, 1 );
   initTextureState( 0, 17, 0 );
   initTextureState( 1, 0, 0 );
   initTextureState( 1, 1, 2 );
   initTextureState( 1, 2, 1 );
   initTextureState( 1, 3, 0 );
   initTextureState( 1, 4, 2 );
   initTextureState( 1, 5, 1 );
   initTextureState( 1, 6, 0 );
   initTextureState( 1, 7, 0 );
   initTextureState( 1, 8, 0 );
   initTextureState( 1, 9, 0 );
   initTextureState( 1, 10, 1 );
   initTextureState( 1, 11, 0 );
   initTextureState( 1, 12, 0 );
   initTextureState( 1, 13, 0 );
   initTextureState( 1, 14, 1 );
   initTextureState( 1, 15, 1 );
   initTextureState( 1, 16, 1 );
   initTextureState( 1, 17, 0 );
   initTextureState( 2, 0, 0 );
   initTextureState( 2, 1, 2 );
   initTextureState( 2, 2, 1 );
   initTextureState( 2, 3, 0 );
   initTextureState( 2, 4, 2 );
   initTextureState( 2, 5, 1 );
   initTextureState( 2, 6, 0 );
   initTextureState( 2, 7, 0 );
   initTextureState( 2, 8, 0 );
   initTextureState( 2, 9, 0 );
   initTextureState( 2, 10, 2 );
   initTextureState( 2, 11, 0 );
   initTextureState( 2, 12, 0 );
   initTextureState( 2, 13, 0 );
   initTextureState( 2, 14, 1 );
   initTextureState( 2, 15, 1 );
   initTextureState( 2, 16, 1 );
   initTextureState( 2, 17, 0 );
   initTextureState( 3, 0, 0 );
   initTextureState( 3, 1, 2 );
   initTextureState( 3, 2, 1 );
   initTextureState( 3, 3, 0 );
   initTextureState( 3, 4, 2 );
   initTextureState( 3, 5, 1 );
   initTextureState( 3, 6, 0 );
   initTextureState( 3, 7, 0 );
   initTextureState( 3, 8, 0 );
   initTextureState( 3, 9, 0 );
   initTextureState( 3, 10, 3 );
   initTextureState( 3, 11, 0 );
   initTextureState( 3, 12, 0 );
   initTextureState( 3, 13, 0 );
   initTextureState( 3, 14, 1 );
   initTextureState( 3, 15, 1 );
   initTextureState( 3, 16, 1 );
   initTextureState( 3, 17, 0 );
   initTextureState( 4, 0, 0 );
   initTextureState( 4, 1, 2 );
   initTextureState( 4, 2, 1 );
   initTextureState( 4, 3, 0 );
   initTextureState( 4, 4, 2 );
   initTextureState( 4, 5, 1 );
   initTextureState( 4, 6, 0 );
   initTextureState( 4, 7, 0 );
   initTextureState( 4, 8, 0 );
   initTextureState( 4, 9, 0 );
   initTextureState( 4, 10, 4 );
   initTextureState( 4, 11, 0 );
   initTextureState( 4, 12, 0 );
   initTextureState( 4, 13, 0 );
   initTextureState( 4, 14, 1 );
   initTextureState( 4, 15, 1 );
   initTextureState( 4, 16, 1 );
   initTextureState( 4, 17, 0 );
   initTextureState( 5, 0, 0 );
   initTextureState( 5, 1, 2 );
   initTextureState( 5, 2, 1 );
   initTextureState( 5, 3, 0 );
   initTextureState( 5, 4, 2 );
   initTextureState( 5, 5, 1 );
   initTextureState( 5, 6, 0 );
   initTextureState( 5, 7, 0 );
   initTextureState( 5, 8, 0 );
   initTextureState( 5, 9, 0 );
   initTextureState( 5, 10, 5 );
   initTextureState( 5, 11, 0 );
   initTextureState( 5, 12, 0 );
   initTextureState( 5, 13, 0 );
   initTextureState( 5, 14, 1 );
   initTextureState( 5, 15, 1 );
   initTextureState( 5, 16, 1 );
   initTextureState( 5, 17, 0 );
   initTextureState( 6, 0, 0 );
   initTextureState( 6, 1, 2 );
   initTextureState( 6, 2, 1 );
   initTextureState( 6, 3, 0 );
   initTextureState( 6, 4, 2 );
   initTextureState( 6, 5, 1 );
   initTextureState( 6, 6, 0 );
   initTextureState( 6, 7, 0 );
   initTextureState( 6, 8, 0 );
   initTextureState( 6, 9, 0 );
   initTextureState( 6, 10, 6 );
   initTextureState( 6, 11, 0 );
   initTextureState( 6, 12, 0 );
   initTextureState( 6, 13, 0 );
   initTextureState( 6, 14, 1 );
   initTextureState( 6, 15, 1 );
   initTextureState( 6, 16, 1 );
   initTextureState( 6, 17, 0 );
   initTextureState( 7, 0, 0 );
   initTextureState( 7, 1, 2 );
   initTextureState( 7, 2, 1 );
   initTextureState( 7, 3, 0 );
   initTextureState( 7, 4, 2 );
   initTextureState( 7, 5, 1 );
   initTextureState( 7, 6, 0 );
   initTextureState( 7, 7, 0 );
   initTextureState( 7, 8, 0 );
   initTextureState( 7, 9, 0 );
   initTextureState( 7, 10, 7 );
   initTextureState( 7, 11, 0 );
   initTextureState( 7, 12, 0 );
   initTextureState( 7, 13, 0 );
   initTextureState( 7, 14, 1 );
   initTextureState( 7, 15, 1 );
   initTextureState( 7, 16, 1 );
   initTextureState( 7, 17, 0 );

   // Sampler states
   initSamplerState( 0, 0, 0 );
   initSamplerState( 0, 1, 0 );
   initSamplerState( 0, 2, 0 );
   initSamplerState( 0, 3, 0 );
   initSamplerState( 0, 4, 1 );
   initSamplerState( 0, 5, 1 );
   initSamplerState( 0, 6, 0 );
   initSamplerState( 0, 7, 0 );
   initSamplerState( 0, 8, 0 );
   initSamplerState( 0, 9, 1 );
   initSamplerState( 0, 10, 0 );
   initSamplerState( 0, 11, 0 );
   initSamplerState( 0, 12, 0 );
   initSamplerState( 1, 0, 0 );
   initSamplerState( 1, 1, 0 );
   initSamplerState( 1, 2, 0 );
   initSamplerState( 1, 3, 0 );
   initSamplerState( 1, 4, 1 );
   initSamplerState( 1, 5, 1 );
   initSamplerState( 1, 6, 0 );
   initSamplerState( 1, 7, 0 );
   initSamplerState( 1, 8, 0 );
   initSamplerState( 1, 9, 1 );
   initSamplerState( 1, 10, 0 );
   initSamplerState( 1, 11, 0 );
   initSamplerState( 1, 12, 0 );
   initSamplerState( 2, 0, 0 );
   initSamplerState( 2, 1, 0 );
   initSamplerState( 2, 2, 0 );
   initSamplerState( 2, 3, 0 );
   initSamplerState( 2, 4, 1 );
   initSamplerState( 2, 5, 1 );
   initSamplerState( 2, 6, 0 );
   initSamplerState( 2, 7, 0 );
   initSamplerState( 2, 8, 0 );
   initSamplerState( 2, 9, 1 );
   initSamplerState( 2, 10, 0 );
   initSamplerState( 2, 11, 0 );
   initSamplerState( 2, 12, 0 );
   initSamplerState( 3, 0, 0 );
   initSamplerState( 3, 1, 0 );
   initSamplerState( 3, 2, 0 );
   initSamplerState( 3, 3, 0 );
   initSamplerState( 3, 4, 1 );
   initSamplerState( 3, 5, 1 );
   initSamplerState( 3, 6, 0 );
   initSamplerState( 3, 7, 0 );
   initSamplerState( 3, 8, 0 );
   initSamplerState( 3, 9, 1 );
   initSamplerState( 3, 10, 0 );
   initSamplerState( 3, 11, 0 );
   initSamplerState( 3, 12, 0 );
   initSamplerState( 4, 0, 0 );
   initSamplerState( 4, 1, 0 );
   initSamplerState( 4, 2, 0 );
   initSamplerState( 4, 3, 0 );
   initSamplerState( 4, 4, 1 );
   initSamplerState( 4, 5, 1 );
   initSamplerState( 4, 6, 0 );
   initSamplerState( 4, 7, 0 );
   initSamplerState( 4, 8, 0 );
   initSamplerState( 4, 9, 1 );
   initSamplerState( 4, 10, 0 );
   initSamplerState( 4, 11, 0 );
   initSamplerState( 4, 12, 0 );
   initSamplerState( 5, 0, 0 );
   initSamplerState( 5, 1, 0 );
   initSamplerState( 5, 2, 0 );
   initSamplerState( 5, 3, 0 );
   initSamplerState( 5, 4, 1 );
   initSamplerState( 5, 5, 1 );
   initSamplerState( 5, 6, 0 );
   initSamplerState( 5, 7, 0 );
   initSamplerState( 5, 8, 0 );
   initSamplerState( 5, 9, 1 );
   initSamplerState( 5, 10, 0 );
   initSamplerState( 5, 11, 0 );
   initSamplerState( 5, 12, 0 );
   initSamplerState( 6, 0, 0 );
   initSamplerState( 6, 1, 0 );
   initSamplerState( 6, 2, 0 );
   initSamplerState( 6, 3, 0 );
   initSamplerState( 6, 4, 1 );
   initSamplerState( 6, 5, 1 );
   initSamplerState( 6, 6, 0 );
   initSamplerState( 6, 7, 0 );
   initSamplerState( 6, 8, 0 );
   initSamplerState( 6, 9, 1 );
   initSamplerState( 6, 10, 0 );
   initSamplerState( 6, 11, 0 );
   initSamplerState( 6, 12, 0 );
   initSamplerState( 7, 0, 0 );
   initSamplerState( 7, 1, 0 );
   initSamplerState( 7, 2, 0 );
   initSamplerState( 7, 3, 0 );
   initSamplerState( 7, 4, 1 );
   initSamplerState( 7, 5, 1 );
   initSamplerState( 7, 6, 0 );
   initSamplerState( 7, 7, 0 );
   initSamplerState( 7, 8, 0 );
   initSamplerState( 7, 9, 1 );
   initSamplerState( 7, 10, 0 );
   initSamplerState( 7, 11, 0 );
   initSamplerState( 7, 12, 0 );
}

void GFXPCD3D9Device::validateMultisampleParams(D3DFORMAT format)
{
   if (mMultisampleType != D3DMULTISAMPLE_NONE)
   {
      DWORD MaxSampleQualities;      
      HRESULT hres = mD3D->CheckDeviceMultiSampleType(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, format, FALSE, D3DMULTISAMPLE_NONMASKABLE, &MaxSampleQualities);
      if (hres == D3DERR_NOTAVAILABLE || getPixelShaderVersion() == 0.0)
      {
         mMultisampleType = D3DMULTISAMPLE_NONE;
         mMultisampleLevel = 0;
      }
      else
      {
         mMultisampleType = D3DMULTISAMPLE_NONMASKABLE;
         if (mMultisampleLevel > MaxSampleQualities-1)
            mMultisampleLevel = MaxSampleQualities-1;  
      }
   }
}

//-----------------------------------------------------------------------------
// Reset D3D device
//-----------------------------------------------------------------------------
void GFXPCD3D9Device::reset( D3DPRESENT_PARAMETERS &d3dpp )
{
   if(!mD3DDevice)
      return;

   mInitialized = false;

   mMultisampleType = d3dpp.MultiSampleType;
   mMultisampleLevel = d3dpp.MultiSampleQuality;
   validateMultisampleParams(d3dpp.BackBufferFormat);

   d3dpp.MultiSampleType = mMultisampleType;
   d3dpp.MultiSampleQuality = mMultisampleLevel;

   // Clean up some commonly dangling state. This helps prevents issues with
   // items that are destroyed by the texture manager callbacks and recreated
   // later, but still left bound.
   setVertexBuffer(NULL);
   setPrimitiveBuffer(NULL);
   for(S32 i=0; i<getNumSamplers(); i++)
      setTexture(i, NULL);

   // Deal with the depth/stencil buffer.
   if(mDeviceDepthStencil)
   {
      Con::printf("GFXPCD3D9Device::reset - depthstencil %x has %d ref's", mDeviceDepthStencil, mDeviceDepthStencil->AddRef()-1);
      mDeviceDepthStencil->Release();
   }

   // First release all the stuff we allocated from D3DPOOL_DEFAULT
   releaseDefaultPoolResources();

   // reset device
   Con::printf( "--- Resetting D3D Device ---" );
   HRESULT hres = S_OK;

   hres = mD3DDevice->Reset( &d3dpp ); 

   if( FAILED( hres ) )
   {
      U32 retries = 0;
      while( mD3DDevice->TestCooperativeLevel() == D3DERR_DEVICELOST && retries < 2 )
      {
         Sleep( 100 );
         retries++;
      }

      // Ok...something failed when we tried to create\reset our device so try scaling back
      if (retries < 2)
      {
         hres = mD3DDevice->Reset( &d3dpp );

         if ( FAILED( hres ) )
         {
            // Try disabling the MSAA
            Con::errorf("GFXD3D9Device::reset - Unable to create device with (%d %d %d %d %d %d) - attempting with FSAA disabled",
               d3dpp.BackBufferWidth, d3dpp.BackBufferHeight, d3dpp.Windowed, d3dpp.BackBufferFormat, d3dpp.FullScreen_RefreshRateInHz, d3dpp.MultiSampleQuality);

            d3dpp.MultiSampleType = D3DMULTISAMPLE_NONE;
            d3dpp.MultiSampleQuality = 0;

            hres = mD3DDevice->Reset( &d3dpp );

            if ( FAILED( hres ) )
            {
               // We probably have a "bad" resolution...default to 800x600
               Con::errorf("GFXD3D9Device::reset - Unable to create device with (%d %d %d %d %d %d) - falling back to 800x600 resolution",
               d3dpp.BackBufferWidth, d3dpp.BackBufferHeight, d3dpp.Windowed, d3dpp.BackBufferFormat, d3dpp.FullScreen_RefreshRateInHz, d3dpp.MultiSampleQuality);

               d3dpp.BackBufferWidth  = 800;
               d3dpp.BackBufferHeight = 600;

               hres = mD3DDevice->Reset( &d3dpp );

               if ( FAILED( hres ) )
               {
                  // Try windowed
                  Con::errorf("GFXD3D9Device::reset - Unable to create device with (%d %d %d %d %d %d) - trying windowed",
                  d3dpp.BackBufferWidth, d3dpp.BackBufferHeight, d3dpp.Windowed, d3dpp.BackBufferFormat, d3dpp.FullScreen_RefreshRateInHz, d3dpp.MultiSampleQuality);

                  d3dpp.Windowed = true;

                  hres = mD3DDevice->Reset( &d3dpp );

                  if ( FAILED( hres ) )
                  {
                     // last ditch - switch bit depth
                     Con::errorf("GFXD3D9Device::reset - Unable to create device with (%d %d %d %d %d %d) - switching bitdepth",
                     d3dpp.BackBufferWidth, d3dpp.BackBufferHeight, d3dpp.Windowed, d3dpp.BackBufferFormat, d3dpp.FullScreen_RefreshRateInHz, d3dpp.MultiSampleQuality);

                     if(d3dpp.BackBufferFormat == D3DFMT_X8R8G8B8)
                        d3dpp.BackBufferFormat = D3DFMT_R5G6B5;
                     else
                        d3dpp.BackBufferFormat = D3DFMT_X8R8G8B8;

                     hres = mD3DDevice->Reset( &d3dpp );
                  }
               }
            }
         }
      }
   }

   D3D9Assert( hres, "GFXD3D9Device::reset - Failed to create D3D Device!" );
   mInitialized = true;

   // Setup default states
   initStates();

   // Mark everything dirty and flush to card, for sanity.
   updateStates(true);

   // Now re aquire all the resources we trashed earlier
   reacquireDefaultPoolResources();
}
//------------------------------------------------------------------------------

void GFXPCD3D9Device::beginSceneInternal() 
{
   // Make sure we have a device
   HRESULT res = mD3DDevice->TestCooperativeLevel();

   while(res == D3DERR_DEVICELOST)
   {
      // Lost device! Just keep querying
      res = mD3DDevice->TestCooperativeLevel();

      Con::warnf("GFXD3D9Device::beginScene - Device needs to be reset, waiting on device...");

      Sleep(50);
   }

   // Trigger a reset if we can't get a good result from TestCooperativeLevel.
   if(res == D3DERR_DEVICENOTRESET)
   {
      Con::warnf("GFXD3D9Device::beginScene - Device needs to be reset, resetting device...");

      // Reset the device!
      GFXResource *walk = mResourceListHead;
      while(walk)
      {
         // Find the window target with implicit flag set and reset the device with its presentation params.
         if(GFXPCD3D9WindowTarget *gdwt = dynamic_cast<GFXPCD3D9WindowTarget*>(walk))
         {
            if(gdwt->mImplicit)
            {
               reset(gdwt->mPresentationParams);
               break;
            }
         }

         walk = walk->getNextResource();
      }
   }

   D3D9Assert( mD3DDevice->BeginScene(), "GFXD3D9Device::beginSceneInternal - failed to BeginScene");
   mCanCurrentlyRender = true;
}

//-----------------------------------------------------------------------------

void GFXPCD3D9Device::endSceneInternal() 
{
   mD3DDevice->EndScene();
   mCanCurrentlyRender = false;
}
//-----------------------------------------------------------------------------

void GFXPCD3D9Device::setActiveRenderTarget(GFXTarget *target )
{
#ifdef TORQUE_DEBUG
   AssertFatal(target, 
      "GFXD3D9Device::setActiveRenderTarget - must specify a render target!");

   // Let's do a little validation here...
   bool isValid = dynamic_cast<GFXPCD3D9WindowTarget*>(target) || dynamic_cast<GFXPCD3D9TextureTarget*>(target);

   AssertFatal( isValid, 
      "GFXD3D9Device::setActiveRenderTarget - invalid target subclass passed!");
#endif
   // Update our current RT.
   if (mCurrentRT)
      mCurrentRT->deactivate();
   mCurrentRT = target;
   mCurrentRT->activate();

   // Deal with window case.
   if(GFXPCD3D9WindowTarget *gdwt = dynamic_cast<GFXPCD3D9WindowTarget*>(target))
   {
      mD3DDevice->SetRenderTarget(0, gdwt->mBackbuffer);
      mD3DDevice->SetDepthStencilSurface(gdwt->mDepthStencil);

      D3DPRESENT_PARAMETERS pp;
      gdwt->mSwapChain->GetPresentParameters(&pp);

      // TODO: May need changes for resizing window
      // Update our video mode here, too.
      GFXVideoMode vm;
      //vm = gdwt->mWindow->getCurrentMode();
      AssertFatal(winState.videoMode, "GFXD3D9Device::setActiveRenderTarget - no video mode!");
      vm = *winState.videoMode;
      vm.resolution.x = pp.BackBufferWidth;
      vm.resolution.y = pp.BackBufferHeight;
      vm.fullScreen = !pp.Windowed;

      //gdwt->mWindow->mVideoMode.resolution = vm.resolution;
      //gdwt->mWindow->mVideoMode.fullScreen = vm.fullScreen;
      gdwt->mSize = vm.resolution;

      // Exit!
      return;
   }

   // Deal with texture target case.
   if(GFXPCD3D9TextureTarget *gdtt = dynamic_cast<GFXPCD3D9TextureTarget*>(target))
   {
      // Clear the state indicator.
      gdtt->stateApplied();

      // Set all the surfaces into the appropriate slots.
      D3D9Assert(mD3DDevice->SetRenderTarget(0, gdtt->mTargets[GFXTextureTarget::Color0]), 
         "GFXD3D9Device::setActiveRenderTarget - failed to set slot 0 for texture target!" );

      D3D9Assert(mD3DDevice->SetDepthStencilSurface(gdtt->mTargets[GFXTextureTarget::DepthStencil]), 
         "GFXD3D9Device::SetDepthStencilSurface - failed to set depthstencil target!" );

      // Reset the viewport.
      D3DSURFACE_DESC desc;
      gdtt->mTargets[GFXTextureTarget::Color0]->GetDesc( &desc );
      RectI vp( 0,0, desc.Width, desc.Height );
      setViewport( vp );

      // Exit!
      return;
   }
}

//------------------------------------------------------------------------------

GFXWindowTarget * GFXPCD3D9Device::allocWindowTarget( /*PlatformWindow *window*/ )
{
   /*AssertFatal(window,"GFXD3D9Device::allocWindowTarget - no window provided!");
   AssertFatal(dynamic_cast<Win32Window*>(window), 
      "GFXD3D9Device::allocWindowTarget - only works with Win32Windows!");

   Win32Window *w32w = static_cast<Win32Window*>(window);*/

   // Set up a new window target...
   GFXPCD3D9WindowTarget *gdwt = new GFXPCD3D9WindowTarget();
   gdwt->mSize = Point2I(winState.windowManager->mActualWidth, winState.windowManager->mActualHeight);//window->getBounds().extent;
   gdwt->mDevice = this;
   //gdwt->mWindow = w32w;
   gdwt->initPresentationParams();

   // Now, we have to init & bind our device... we have basically two scenarios
   // of which the first is:
   if(mD3DDevice == NULL)
   {
      // Allocate the device.
      //init(window->getCurrentMode(), window);
       AssertFatal(winState.videoMode, "GFXD3D9Device::allocWindowTarget - no video mode!");
       init(*winState.videoMode);

      // Cool, we have the device, grab back the depthstencil buffer as well
      // as the swap chain.

      HRESULT hres = mD3DDevice->GetSwapChain(0, &gdwt->mSwapChain);
      D3D9Assert(hres, "GFXD3D9Device::allocWindowTarget - couldn't get swap chain!");

      hres = mD3DDevice->GetDepthStencilSurface(&gdwt->mDepthStencil);
      D3D9Assert(hres, "GFXD3D9Device::allocWindowTarget - couldn't get depth stencil!");

      gdwt->mSwapChain->GetBackBuffer(0, D3DBACKBUFFER_TYPE_MONO, &gdwt->mBackbuffer);         

      gdwt->mImplicit = true;
   }
   else
   {
      // And the second case:
      // Initialized device, create an additional swap chain.
      gdwt->mImplicit = false;

      HRESULT hres = mD3DDevice->CreateAdditionalSwapChain(&gdwt->mPresentationParams, &gdwt->mSwapChain);
      
      if(hres != S_OK)
      {
         Con::errorf("GFXD3D9Device::allocWindowTarget - failed to create additional swap chain!");
         SAFE_DELETE(gdwt);
         return NULL;
      }
     
      hres = mD3DDevice->CreateDepthStencilSurface(gdwt->mPresentationParams.BackBufferWidth, gdwt->mPresentationParams.BackBufferHeight, 
         D3DFMT_D24S8, getMultisampleType(), getMultisampleLevel(), false, &gdwt->mDepthStencil, NULL);

      if(hres != S_OK)
      {
         Con::errorf("GFXD3D9Device::allocWindowTarget - failed to create additional swap chain zbuffer!");
         SAFE_DELETE(gdwt);
         return NULL;
      }
   }

   gdwt->registerResourceWithDevice(this);

   return gdwt;
}

//------------------------------------------------------------------------------

GFXTextureTarget * GFXPCD3D9Device::allocRenderToTextureTarget()
{
   GFXPCD3D9TextureTarget *targ = new GFXPCD3D9TextureTarget();
   targ->mDevice = this;

   targ->registerResourceWithDevice(this);

   return targ;
}