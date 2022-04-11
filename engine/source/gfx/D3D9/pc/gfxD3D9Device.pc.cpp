//-----------------------------------------------------------------------------
// Torque Game Engine Advanced
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include <dsetup.h>
#include "gfx/D3D9/pc/gfxPCD3D9Device.h"
#include "gfx/D3D9/pc/gfxPCD3D9CardProfiler.h"
#include "gfx/D3D9/gfxD3D9Shader.h"
#include "gfx/D3D9/gfxD3D9VertexBuffer.h"
#include "gfx/D3D9/gfxD3D9EnumTranslate.h"

// Gross hack to let the diag utility know that we only need stubs
#define DUMMYDEF
#include "gfx/D3D/DXDiagNVUtil.h"

// These functions are static members of the GFXD3D9Device class. They are split
// out into a different file because the implementations differ on the PC and the
// 360, however since they are static members, they can't also be virtual.
//------------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Get a string indicating the installed DirectX version, revision and letter number
//-----------------------------------------------------------------------------
char *GFXD3D9Device::getDXVersion()
{
   DWORD dwVersion = 0;
   DWORD dwRevision = 0;
   TCHAR dxVersionLetter = ' ';

   NVDXDiagWrapper::DXDiagNVUtil * dxDiag = new NVDXDiagWrapper::DXDiagNVUtil();
   dxDiag->InitIDxDiagContainer();
   dxDiag->GetDirectXVersion( &dwVersion, &dwRevision, &dxVersionLetter );
   dxDiag->FreeIDxDiagContainer();
   delete dxDiag;

   if( dwVersion == 0 && dwRevision == 0 )
   {
      Con::errorf("Could not determine DirectX version.");
      return "0";
   }

   static char s_pcVersionString[256];

   dSprintf(s_pcVersionString, 256, "%d.%d.%d %c", LOWORD(dwVersion), HIWORD(dwRevision),
      LOWORD(dwRevision), dxVersionLetter); 

   return s_pcVersionString;
}

//------------------------------------------------------------------------------
// D3DX Function binding
//------------------------------------------------------------------------------

bool d3dxBindFunction( DLibrary *dll, void *&fnAddress, const char *name )
{
   fnAddress = dll->bind( name );

   if (!fnAddress)
      Con::warnf( "D3DX Loader: DLL bind failed for %s", name );

   return fnAddress != 0;
}

bool d3d9BindFunction( DLibrary *dll, void *&fnAddress, const char *name )
{
   fnAddress = dll->bind( name );

   if (!fnAddress)
      Con::warnf( "D3D9 Loader: DLL bind failed for %s", name );

   return fnAddress != 0;
}

void GFXD3D9Device::initD3DXFnTable()
{
   if( !smD3DX.isLoaded )
   {
      // Total hack for the win!
      U32 d3dxver = D3DX_SDK_VERSION;
      FrameTemp<char> d3dxdllString(16);

      do
      {
         // Just look for the normal binary - D3D will substitute the debug runtime
         // if that option is selected.
         dSprintf( d3dxdllString, 16, "d3dx9_%d.dll", d3dxver-- );
         smD3DX.dllRef = OsLoadLibrary( d3dxdllString );
      } while( smD3DX.dllRef == NULL && d3dxver > 23 ); // This was when we started using d3d9, yes? And headers for used fxns haven't changed AFAIK. -patw

      AssertISV(smD3DX.dllRef, "GFXD3D9Devce::initD3DXFnTable - failed to locate a D3DX dll!");

      smD3DX.isLoaded = true;

#define D3DX_FUNCTION(fn_name, fn_return, fn_args) \
   smD3DX.isLoaded &= d3dxBindFunction(smD3DX.dllRef, *(void**)&smD3DX.fn_name, #fn_name);
#     include "gfx/D3D9/d3dx9Functions.h"
#undef D3DX_FUNCTION

      AssertISV( smD3DX.isLoaded, "D3DX Failed to load all functions." );
      Con::printf( "GFXD3D9Device - using '%s' for dynamic linking.", ~d3dxdllString );
   }
}

//------------------------------------------------------------------------------

void GFXD3D9Device::initD3D9FnTable()
{
   if( !smD3D9.isLoaded )
   {
      smD3D9.dllRef = OsLoadLibrary( "d3d9.dll" ); // TODO: Check for debug ver

      if (smD3D9.dllRef == NULL)
      {
         Con::printf("Failed to load D3D9");
         smD3D9.isLoaded = false;
         return;
      }
      else
      {
         smD3D9.isLoaded = true;
      }

#define D3D9_FUNCTION(fn_name, fn_return, fn_args) \
   smD3D9.isLoaded &= d3d9BindFunction(smD3D9.dllRef, *(void**)&smD3D9.fn_name, #fn_name);
#     include "gfx/D3D9/d3d9Functions.h"
#undef D3D9_FUNCTION

      AssertISV( smD3D9.isLoaded, "D3D9 Failed to load all functions." );
   }
}

//-----------------------------------------------------------------------------
// Enumerate D3D adapters
//-----------------------------------------------------------------------------
void GFXD3D9Device::enumerateAdapters( Vector<GFXAdapter*> &adapterList )
{
   // If the dynamic D3D9 loader isn't ready, init it
   if( !smD3D9.isLoaded )
      initD3D9FnTable();

   // If it's still not loaded, that means the system doesn't have DX9, so bail
   if( !smD3D9.isLoaded )
      return;

   // Grab a D3D9 handle here to first get the D3D9 devices
   LPDIRECT3D9 d3d9 = smD3D9.Direct3DCreate9( D3D_SDK_VERSION );

   AssertISV( d3d9, "Could not create D3D object, make sure you have the latest version of DirectX installed" );

   // Print DX version ASAP - this is not at top of function because it has not
   // been tested with DX 8, so it may crash and the user would not get a dialog box...
   Con::printf( "DirectX 9 version - %s", getDXVersion() );

   for( U32 adapterIndex = 0; adapterIndex < d3d9->GetAdapterCount(); adapterIndex++ ) 
   {
      GFXAdapter *toAdd = new GFXAdapter;
      toAdd->mType  = Direct3D9;
      toAdd->mIndex = adapterIndex;

      // Grab the shader model.
      D3DCAPS9 caps;
      d3d9->GetDeviceCaps(adapterIndex, D3DDEVTYPE_HAL, &caps);
      U8 *pxPtr = (U8*) &caps.PixelShaderVersion;
      toAdd->mShaderModel = pxPtr[1] + pxPtr[0] * 0.1;

      // Get the device description string.
      D3DADAPTER_IDENTIFIER9 temp;
      d3d9->GetAdapterIdentifier( adapterIndex, NULL, &temp ); // The NULL is the flags which deal with WHQL

      dStrcpy( toAdd->mName, temp.Description );
      dStrncat(toAdd->mName, " (D3D9)\0", GFXAdapter::MaxAdapterNameLen);

      // Video mode enumeration.
      Vector<D3DFORMAT> formats;
      formats.push_back( D3DFMT_R5G6B5 );    // D3DFMT_R5G6B5 - 16bit format
      formats.push_back( D3DFMT_X8R8G8B8 );  // D3DFMT_X8R8G8B8 - 32bit format

      for( S32 i = 0; i < formats.size(); i++ ) 
      {
         DWORD MaxSampleQualities;
         d3d9->CheckDeviceMultiSampleType(adapterIndex, D3DDEVTYPE_HAL, formats[i], FALSE, D3DMULTISAMPLE_NONMASKABLE, &MaxSampleQualities);

         for( U32 j = 0; j < d3d9->GetAdapterModeCount( adapterIndex, formats[i] ); j++ ) 
         {
            D3DDISPLAYMODE mode;
            d3d9->EnumAdapterModes( D3DADAPTER_DEFAULT, formats[i], j, &mode );

            GFXVideoMode vmAdd;

            vmAdd.bitDepth    = ( i == 0 ? 16 : 32 ); // This will need to be changed later
            vmAdd.fullScreen  = true;
            vmAdd.refreshRate = mode.RefreshRate;
            vmAdd.resolution  = Point2I( mode.Width, mode.Height );
            vmAdd.antialiasLevel = MaxSampleQualities;

            toAdd->mAvailableModes.push_back( vmAdd );
         }
      }

      adapterList.push_back( toAdd );
   }

   d3d9->Release();
}
