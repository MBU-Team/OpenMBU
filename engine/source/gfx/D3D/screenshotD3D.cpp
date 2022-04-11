//-----------------------------------------------------------------------------
// Torque Game Engine Advanced 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------
#include "d3d9.h"
#include "d3dx9core.h"
#include "d3dx9tex.h"

#include "gfx/D3D9/gfxD3D9Device.h"

#include "screenshotD3D.h"

#ifndef TORQUE_TGB_ONLY
#  include "sceneGraph/sceneGraph.h"
#endif

#include "core/fileStream.h"
#include "gui/core/guiCanvas.h"
#include "gui/core/guiTSControl.h"

//-----------------------------------------------------------------------------
// Capture standard screenshot - read it from the back buffer
//    This function jumps through some hoops copying surfaces around so that
//    the screenshot will work when using a multisample back buffer.
//-----------------------------------------------------------------------------
void ScreenShotD3D::captureStandard()
{
   LPDIRECT3DDEVICE9 D3DDevice = dynamic_cast<GFXD3D9Device *>(GFX)->getDevice();

   // CodeReview - We should probably just be doing this on Canvas and getting
   // the data from the GFXWindowTarget - [bjg 5/1/07]
   // grab the back buffer
   IDirect3DSurface9 * backBuffer;
   D3DDevice->GetBackBuffer( 0, 0, D3DBACKBUFFER_TYPE_MONO, &backBuffer );

   // Figure the size we're snagging.
   D3DSURFACE_DESC desc;
   backBuffer->GetDesc(&desc);

   Point2I size;
   size.x = desc.Width;
   size.y = desc.Height;

   // set up the 2 copy surfaces
   GFXTexHandle tex[2];
   IDirect3DSurface9 *surface[2];
   
   tex[0].set( size.x, size.y, GFXFormatR8G8B8X8, &GFXDefaultRenderTargetProfile );
   tex[1].set( size.x, size.y, GFXFormatR8G8B8X8, &GFXSystemMemProfile );
   
   // grab the top level surface of tex 0
   GFXD3D9TextureObject *to = (GFXD3D9TextureObject *) &(*tex[0]);
   D3D9Assert( to->get2DTex()->GetSurfaceLevel( 0, &surface[0] ), NULL );

   // use StretchRect because it allows a copy from a multisample surface
   // to a normal rendertarget surface
   D3DDevice->StretchRect( backBuffer, NULL, surface[0], NULL, D3DTEXF_NONE );

   // grab the top level surface of tex 1
   to = (GFXD3D9TextureObject *) &(*tex[1]);
   D3D9Assert( to->get2DTex()->GetSurfaceLevel( 0, &surface[1] ), NULL );

   // copy the data from the render target to the system memory texture
   D3DDevice->GetRenderTargetData( surface[0], surface[1] );
   
   // save it off
   D3DXIMAGE_FILEFORMAT format;
   
   if( dStrstr( (const char*)mFilename, ".jpg" ) )
   {
      format = D3DXIFF_JPG;
   }
   else
   {
      format = D3DXIFF_PNG;
   }
   
   GFXD3DX.D3DXSaveSurfaceToFile( mFilename, format, surface[1], NULL, NULL );

   // release the COM pointers
   surface[0]->Release();
   surface[1]->Release();
   backBuffer->Release();

   mPending = false;
}

void saveRT_to_bitmap(GFXTexHandle &texToSave, const char *filename)
{
   LPDIRECT3DDEVICE9 D3DDevice = dynamic_cast<GFXD3D9Device *>(GFX)->getDevice();

   Point2I size(texToSave.getWidth(), texToSave.getHeight());

   GFXTexHandle sysTex;
   sysTex.set( size.x, size.y, GFXFormatR8G8B8X8, &GFXSystemMemProfile );

   IDirect3DSurface9 *surface[2];

   // grab the top level surface of tex to save
   GFXD3D9TextureObject *to = (GFXD3D9TextureObject *) &(*texToSave);
   D3D9Assert( to->get2DTex()->GetSurfaceLevel( 0, &surface[0] ), NULL );

   // grab the top level surface of tex 1
   to = (GFXD3D9TextureObject *) &(*sysTex);
   D3D9Assert( to->get2DTex()->GetSurfaceLevel( 0, &surface[1] ), NULL );

   // copy the data from the render target to the system memory texture
   D3DDevice->GetRenderTargetData( surface[0], surface[1] );

   // save it off
   D3DXIMAGE_FILEFORMAT format;

   if( dStrstr( (const char*)filename, ".jpg" ) )
   {
      format = D3DXIFF_JPG;
   }
   else
   {
      format = D3DXIFF_PNG;
   }

   GFXD3DX.D3DXSaveSurfaceToFileA( filename, format, surface[1], NULL, NULL );

   // release the COM pointers
   surface[0]->Release();
   surface[1]->Release();
}

