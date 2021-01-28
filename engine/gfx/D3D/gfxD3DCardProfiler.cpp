//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "d3d9.h"
#include "d3dx9math.h"
#include "gfx/D3D/gfxD3DDevice.h"
#include "core/fileStream.h"
#include "console/console.h"
#include "gfx/primBuilder.h"
#include "platform/profiler.h"
#include "gfx/D3D/gfxD3DCardProfiler.h"

#define DUMMYDEF
#include "gfx/D3D/DXDiagNVUtil.h"

GFXD3DCardProfiler::GFXD3DCardProfiler()
{
   mVersionString = NULL;
   mVendorString  = NULL;
   mCardString    = NULL;
}

GFXD3DCardProfiler::~GFXD3DCardProfiler()
{
   if(mVersionString)
      delete[] mVersionString;

   if(mVendorString)
      delete (char*)mVendorString;

   if(mCardString)
      delete (char*)mCardString;
}

void GFXD3DCardProfiler::init()
{
   mDevice = dynamic_cast<GFXD3DDevice*>(GFX);

   AssertISV(mDevice, "GFXD3DCardProfiler::init() - No GFXD3DDevice found!");

   mD3DDevice = mDevice->getDevice();


   D3DDEVICE_CREATION_PARAMETERS p;
   mD3DDevice->GetCreationParameters(&p);

   mAdapterOrdinal = p.AdapterOrdinal;

   // get adapter ID
   IDirect3D9 *d3d;
   mD3DDevice->GetDirect3D(&d3d);
   D3DADAPTER_IDENTIFIER9 info;
   d3d->GetAdapterIdentifier(mAdapterOrdinal, 0, &info);
   d3d->Release();

   // Initialize the DxDiag interface...
   NVDXDiagWrapper::DXDiagNVUtil diag;
   diag.InitIDxDiagContainer();

   char foo[512];
   diag.GetDisplayDeviceManufacturer(mAdapterOrdinal, foo);
   mVendorString = dStrdup(foo);
   diag.GetDisplayDeviceChipSet(mAdapterOrdinal, foo);
   mCardString = dStrdup(foo);

/*   if(!dStricmp(mVendorString, "NVIDIA") || !dStricmp(mVendorString, "ATI"))
   { */
      // Get the actual driver revision number then, if we can...
      mVersionString = new char[128];
      F32 version;
      diag.GetDisplayDeviceNVDriverVersion(mAdapterOrdinal, &version );
      dSprintf(mVersionString, 128, "%.2f", version);
/*   }
   else
   {
      // Stick the whole version in there
      diag.GetDisplayDeviceDriverVersionStr(mAdapterOrdinal, foo);
      mVersionString = dStrdup(foo);
   } */


   diag.FreeIDxDiagContainer();

   Parent::init();
}

const char* GFXD3DCardProfiler::getVersionString()
{
   return mVersionString;
}

const char* GFXD3DCardProfiler::getCardString()
{
   return mCardString;
}

const char* GFXD3DCardProfiler::getVendorString()
{
   return mVendorString;
}

const char* GFXD3DCardProfiler::getRendererString()
{
   return "D3D9";
}

void GFXD3DCardProfiler::setupCardCapabilities()
{
   // Get the D3D device caps
   D3DCAPS9 caps;
   mD3DDevice->GetDeviceCaps(&caps);

   setCapability( dStrdup( "autoMipMapLevel" ), ( caps.Caps2 & D3DCAPS2_CANAUTOGENMIPMAP ? 1 : 0 ) );
   setCapability( dStrdup( "maxTextureWidth" ), caps.MaxTextureWidth );
   setCapability( dStrdup( "maxTextureHeight" ), caps.MaxTextureHeight );
}

bool GFXD3DCardProfiler::_queryCardCap(const char *query, U32 &foundResult)
{
   return 0;
}
