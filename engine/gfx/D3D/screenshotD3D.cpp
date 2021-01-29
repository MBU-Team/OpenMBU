//-----------------------------------------------------------------------------
// Torque Shader Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------
#include "d3d9.h"
#include "d3dx9core.h"
#include "d3dx9tex.h"

#include "gfxD3DDevice.h"

#include "screenshotD3D.h"
#include "sceneGraph/sceneGraph.h"
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
   LPDIRECT3DDEVICE9 D3DDevice = dynamic_cast<GFXD3DDevice *>(GFX)->getDevice();

   Point2I size = GFX->getVideoMode().resolution;

   // set up the 2 copy surfaces
   GFXTexHandle tex[2];
   IDirect3DSurface9 *surface[2];
   
   tex[0].set( size.x, size.y, GFXFormatR8G8B8X8, &GFXDefaultRenderTargetProfile );
   tex[1].set( size.x, size.y, GFXFormatR8G8B8X8, &GFXSystemMemProfile );
   
   // grab the back buffer
   IDirect3DSurface9 * backBuffer;
   D3DDevice->GetBackBuffer( 0, 0, D3DBACKBUFFER_TYPE_MONO, &backBuffer );

   // grab the top level surface of tex 0
   GFXD3DTextureObject *to = (GFXD3DTextureObject *) &(*tex[0]);
   D3DAssert( to->get2DTex()->GetSurfaceLevel( 0, &surface[0] ), NULL );

   // use StretchRect because it allows a copy from a multisample surface
   // to a normal rendertarget surface
   D3DDevice->StretchRect( backBuffer, NULL, surface[0], NULL, D3DTEXF_NONE );

   // grab the top level surface of tex 1
   to = (GFXD3DTextureObject *) &(*tex[1]);
   D3DAssert( to->get2DTex()->GetSurfaceLevel( 0, &surface[1] ), NULL );

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
   
   D3DXSaveSurfaceToFile( mFilename, format, surface[1], NULL, NULL );

   // release the COM pointers
   surface[0]->Release();
   surface[1]->Release();
   backBuffer->Release();

   mPending = false;
}

//-----------------------------------------------------------------------------
// Capture custom screenshot - useful for high resolution screen captures
//
// This is a test function - I left the code in so I can pick up where I left
// off later.  Right now there are issues with:
// - Capturing the Guis
// - Other rendering modules overriding render-to-texture (Glowbuffer, terrain, etc)
//   This can probably be fixed by creating a render target stack in GFXDevice
// - In order to work for captures larger than the current screen size, this
//   function needs to create its own stencil buffer - this isn't hard, just
//   didn't get to it

//
// Bramage
//-----------------------------------------------------------------------------
void ScreenShotD3D::captureCustom()
{
   LPDIRECT3DDEVICE9 D3DDevice = dynamic_cast<GFXD3DDevice *>(GFX)->getDevice();

   const F32 width = 512;
   const F32 height = 512;

   
   // set up render target surface
   GFXTexHandle tex;
   IDirect3DSurface9 *surface;
   
   tex.set( width, height, GFXFormatR8G8B8X8, &GFXDefaultRenderTargetProfile );
   GFXD3DTextureObject *to = (GFXD3DTextureObject *) &(*tex);
   D3DAssert( to->get2DTex()->GetSurfaceLevel( 0, &surface ), NULL );
   
   // store current matrices
   GFX->pushWorldMatrix();
   MatrixF proj = GFX->getProjectionMatrix();

   // store previous depth buffer
   IDirect3DSurface9 *prevDepthSurface;
   D3DDevice->GetDepthStencilSurface( &prevDepthSurface );

   // set projection to 90 degrees vertical and horizontal
   MatrixF matProj;
   D3DXMatrixPerspectiveFovRH( (D3DXMATRIX *) &matProj, D3DX_PI/2, 1.0f, 0.1f, 1000.0f );
   matProj.transpose();

   // set projection to match Torque space
   MatrixF rotMat(EulerF( (M_PI / 2.0), 0.0, 0.0));

   matProj.mul(rotMat);
   GFX->setProjectionMatrix(matProj);
   
   // grab camera transform from tsCtrl
   GuiTSCtrl *tsCtrl;
   tsCtrl = dynamic_cast<GuiTSCtrl*>( Sim::findObject("PlayGui") );

   CameraQuery query;
   tsCtrl->processCameraQuery( &query );
   MatrixF camMatrix = query.cameraMatrix.inverse();

   GFX->setWorldMatrix( camMatrix );

   // setup viewport
   RectI oldVp = GFX->getViewport();
   RectI vp( Point2I(0,0), Point2I( width, height ) );
   GFX->setViewport( vp );

   // render a frame
   GFX->pushActiveRenderSurfaces();
   GFX->setActiveRenderSurface( tex );
   GFX->setZEnable( true );
   GFX->beginScene();
   GFX->clear( GFXClearZBuffer | GFXClearStencil | GFXClearTarget, ColorI( 255, 0, 255 ), 1.0f, 0 );

   getCurrentClientSceneGraph()->renderScene( InteriorObjectType );

   GFX->endScene();
   GFX->popActiveRenderSurfaces();

   // save the screenshot
   D3DXSaveSurfaceToFile( dT( "testScreen.png" ), D3DXIFF_PNG, surface, NULL, NULL );

   // cleanup
   surface->Release();
   GFX->popWorldMatrix();
   GFX->setProjectionMatrix( proj );
}

void saveRT_to_bitmap(GFXTexHandle &texToSave, const char *filename)
{
   LPDIRECT3DDEVICE9 D3DDevice = dynamic_cast<GFXD3DDevice *>(GFX)->getDevice();

   Point2I size(texToSave.getWidth(), texToSave.getHeight());

   GFXTexHandle sysTex;
   sysTex.set( size.x, size.y, GFXFormatR8G8B8X8, &GFXSystemMemProfile );

   IDirect3DSurface9 *surface[2];

   // grab the top level surface of tex to save
   GFXD3DTextureObject *to = (GFXD3DTextureObject *) &(*texToSave);
   D3DAssert( to->get2DTex()->GetSurfaceLevel( 0, &surface[0] ), NULL );

   // grab the top level surface of tex 1
   to = (GFXD3DTextureObject *) &(*sysTex);
   D3DAssert( to->get2DTex()->GetSurfaceLevel( 0, &surface[1] ), NULL );

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

   D3DXSaveSurfaceToFileA( filename, format, surface[1], NULL, NULL );

   // release the COM pointers
   surface[0]->Release();
   surface[1]->Release();
}

