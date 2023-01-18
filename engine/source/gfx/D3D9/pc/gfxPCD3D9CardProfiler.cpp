//-----------------------------------------------------------------------------
// Torque Game Engine Advanced
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "gfx/D3D9/gfxD3D9Device.h"
#include "core/fileStream.h"
#include "console/console.h"
#include "gfx/primBuilder.h"
#include "platform/profiler.h"
#include "gfx/D3D9/pc/gfxPCD3D9CardProfiler.h"
#include "util/safeDelete.h"
#include "gfx/D3D9/gfxD3D9EnumTranslate.h"

// Gross hack to let the diag utility know that we only need stubs
#define DUMMYDEF
#include "gfx/D3D/DXDiagNVUtil.h"

GFXPCD3D9CardProfiler::GFXPCD3D9CardProfiler()
{
   mVersionString = NULL;
   mVendorString  = NULL;
   mCardString    = NULL;
}

GFXPCD3D9CardProfiler::~GFXPCD3D9CardProfiler()
{
   SAFE_DELETE(mVersionString);
   SAFE_DELETE(mVendorString);
   SAFE_DELETE(mCardString);
}

void GFXPCD3D9CardProfiler::init()
{
   mD3DDevice = dynamic_cast<GFXD3D9Device *>(GFX)->getDevice();
   AssertISV( mD3DDevice, "GFXPCD3D9CardProfiler::init() - No D3D9 Device found!");

   // Grab the caps so we can get our adapter ordinal and look up our name.
   D3DCAPS9 caps;
   D3D9Assert(mD3DDevice->GetDeviceCaps(&caps), "GFXPCD3D9CardProfiler::init - failed to get device caps!");

   char buffManufacturer[1024], buffDesc[1024];
   F32 devVersion;
   NVDXDiagWrapper::DXDiagNVUtil * dxDiag = new NVDXDiagWrapper::DXDiagNVUtil();
   dxDiag->InitIDxDiagContainer();
   dxDiag->GetDisplayDeviceManufacturer   (caps.AdapterOrdinal, buffManufacturer);
   dxDiag->GetDisplayDeviceNVDriverVersion(caps.AdapterOrdinal, &devVersion);
   dxDiag->GetDisplayDeviceDescription    (caps.AdapterOrdinal, buffDesc);
   dxDiag->FreeIDxDiagContainer();
   delete dxDiag;

   mVendorString  = dStrdup( buffManufacturer );
   mCardString    = dStrdup( buffDesc );
   mVersionString = dStrdup( avar("%.2f", devVersion ) );

   Parent::init();
}

const char* GFXPCD3D9CardProfiler::getVersionString()
{
   return mVersionString;
}

const char* GFXPCD3D9CardProfiler::getCardString()
{
   return mCardString;
}

const char* GFXPCD3D9CardProfiler::getVendorString()
{
   return mVendorString;
}

const char* GFXPCD3D9CardProfiler::getRendererString()
{
   return "D3D9";
}

void GFXPCD3D9CardProfiler::setupCardCapabilities()
{
   // Get the D3D device caps
   D3DCAPS9 caps;
   mD3DDevice->GetDeviceCaps(&caps);

   setCapability( dStrdup( "autoMipMapLevel" ), ( caps.Caps2 & D3DCAPS2_CANAUTOGENMIPMAP ? 1 : 0 ) );

   setCapability( dStrdup( "maxTextureWidth" ), caps.MaxTextureWidth );
   setCapability( dStrdup( "maxTextureHeight" ), caps.MaxTextureHeight );
   setCapability( dStrdup( "maxTextureSize" ), getMin( (U32)caps.MaxTextureWidth, (U32)caps.MaxTextureHeight) );

   bool canDoLERPDetailBlend = ( caps.TextureOpCaps & D3DTEXOPCAPS_LERP ) && ( caps.MaxTextureBlendStages > 1 );

   bool canDoFourStageDetailBlend = ( caps.TextureOpCaps & D3DTEXOPCAPS_SUBTRACT ) &&
                                    ( caps.PrimitiveMiscCaps & D3DPMISCCAPS_TSSARGTEMP ) &&
                                    ( caps.MaxTextureBlendStages > 3 );

   setCapability( dStrdup( "lerpDetailBlend" ), canDoLERPDetailBlend );
   setCapability( dStrdup( "fourStageDetailBlend" ), canDoFourStageDetailBlend );

   // Ask the device about R8G8B8 textures. Some devices will not like 24-bit RGB
   // and will insist on R8G8B8X8, 32-bit, RGB texture format.
   IDirect3D9 *pD3D = NULL;
   D3DDISPLAYMODE displayMode;
   mD3DDevice->GetDirect3D( &pD3D );
   mD3DDevice->GetDisplayMode( 0, &displayMode );
   HRESULT hr = pD3D->CheckDeviceFormat( D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, 
                                    displayMode.Format, 0 /* usage flags = 0 */, 
                                    D3DRTYPE_TEXTURE, GFXD3D9TextureFormat[GFXFormatR8G8B8] );
   setCapability( dStrdup( "allowRGB24BitTextures" ), SUCCEEDED( hr ) ); 

   SAFE_RELEASE( pD3D );
}

bool GFXPCD3D9CardProfiler::_queryCardCap(const char *query, U32 &foundResult)
{
   return 0;
}
