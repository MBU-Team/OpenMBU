//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------
#include "d3dx9.h"
#include "gfxD3DCubemap.h"
#include "gfxD3DDevice.h"
#include "sceneGraph/sceneGraph.h"

#define INIT_HACK

_D3DCUBEMAP_FACES GFXD3DCubemap::faceList[6] = 
{ 
   D3DCUBEMAP_FACE_POSITIVE_X, D3DCUBEMAP_FACE_NEGATIVE_X,
   D3DCUBEMAP_FACE_POSITIVE_Y, D3DCUBEMAP_FACE_NEGATIVE_Y,
   D3DCUBEMAP_FACE_POSITIVE_Z, D3DCUBEMAP_FACE_NEGATIVE_Z
};

//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
GFXD3DCubemap::GFXD3DCubemap()
{
   mCubeTex = NULL;
   mDepthBuff = NULL;
   mDynamic = false;
   mCallbackHandle = -1;

   #ifdef INIT_HACK
   mInit = false;
   #endif
}

//-----------------------------------------------------------------------------
// Destructor
//-----------------------------------------------------------------------------
GFXD3DCubemap::~GFXD3DCubemap()
{
   releaseSurfaces();
   
   if( mDynamic )
   {
      GFX->unregisterTexCallback( mCallbackHandle );
   }
}

//-----------------------------------------------------------------------------
// Release D3D surfaces
//-----------------------------------------------------------------------------
void GFXD3DCubemap::releaseSurfaces()
{
   if( mCubeTex )
   {
      mCubeTex->Release();
      mCubeTex = NULL;
   }
   if( mDepthBuff )
   {
      mDepthBuff->Release();
      mDepthBuff = NULL;
   }
}

//-----------------------------------------------------------------------------
// Texture manager callback - for resetting textures on video change
//-----------------------------------------------------------------------------
void GFXD3DCubemap::texManagerCallback( GFXTexCallbackCode code, void *userData )
{
   GFXD3DCubemap *cubemap = (GFXD3DCubemap *) userData;
   
   if( !cubemap->mDynamic ) return;
   
   if( code == GFXZombify )
   {
      cubemap->releaseSurfaces();
      return;
   }

   if( code == GFXResurrect )
   {
      cubemap->initDynamic( cubemap->mTexSize );
      return;
   }
}

//-----------------------------------------------------------------------------
// Init Static
//-----------------------------------------------------------------------------
void GFXD3DCubemap::initStatic( GFXTexHandle *faces )
{
   if( mCubeTex )
   {
      return;
   }

   if( faces )
   {
      if( !faces[0] )
      {
         AssertFatal( false, "empty texture passed to CubeMap::create" );
      }

      LPDIRECT3DDEVICE9 D3DDevice = dynamic_cast<GFXD3DDevice *>(GFX)->getDevice();
      
      
      // NOTE - check tex sizes on all faces - they MUST be all same size
      U32 texSize = faces[0].getWidth();

      D3DAssert( D3DDevice->CreateCubeTexture( texSize, 1, 0, D3DFMT_A8R8G8B8,
                 D3DPOOL_MANAGED, &mCubeTex, NULL ), NULL );


      fillCubeTextures( faces, D3DDevice );
//      mCubeTex->GenerateMipSubLevels();
   }
   
}


//-----------------------------------------------------------------------------
// Init Dynamic
//-----------------------------------------------------------------------------
void GFXD3DCubemap::initDynamic( U32 texSize )
{
   if( mCubeTex )
   {
      return;
   }

   if( mCallbackHandle == -1 )  // make sure it hasn't already registered.
   {
      GFX->registerTexCallback( texManagerCallback, (void*) this, mCallbackHandle );
   }
   
   mInit = false;
   mDynamic = true;
   mTexSize = texSize;
   

   LPDIRECT3DDEVICE9 D3DDevice = dynamic_cast<GFXD3DDevice *>(GFX)->getDevice();

   // might want to try this as a 16 bit texture...
   D3DAssert( D3DDevice->CreateCubeTexture( texSize,
                                            1, 
                                            D3DUSAGE_RENDERTARGET, 
                                            D3DFMT_A8R8G8B8,
                                            D3DPOOL_DEFAULT, 
                                            &mCubeTex, 
                                            NULL ), NULL );
      
   D3DDevice->CreateDepthStencilSurface( texSize, 
                                         texSize, 
                                         D3DFMT_D24S8, 
                                         D3DMULTISAMPLE_NONE, 
                                         0, 
                                         false, 
                                         &mDepthBuff, 
                                         NULL );
   
}

//-----------------------------------------------------------------------------
// Fills in face textures of cube map from existing textures
//-----------------------------------------------------------------------------
void GFXD3DCubemap::fillCubeTextures( GFXTexHandle *faces, LPDIRECT3DDEVICE9 D3DDevice )
{
   for( U32 i=0; i<6; i++ )
   {
      // get cube face surface
      IDirect3DSurface9 *cubeSurf = NULL;
      D3DAssert( mCubeTex->GetCubeMapSurface( faceList[i], 0, &cubeSurf ), NULL );

      // get incoming texture surface
      GFXD3DTextureObject *texObj = dynamic_cast<GFXD3DTextureObject*>( (GFXTextureObject*)faces[i] );
      IDirect3DSurface9 *inSurf;
      D3DAssert( texObj->get2DTex()->GetSurfaceLevel( 0, &inSurf ), NULL );
      
      // copy incoming texture into cube face
      D3DAssert( D3DXLoadSurfaceFromSurface( cubeSurf, NULL, NULL, inSurf, NULL, 
                                  NULL, D3DX_FILTER_NONE, 0 ), NULL );
      cubeSurf->Release();
      inSurf->Release();
   }
}

//-----------------------------------------------------------------------------
// Set the cubemap to the specified texture unit num
//-----------------------------------------------------------------------------
void GFXD3DCubemap::setToTexUnit( U32 tuNum )
{
   LPDIRECT3DDEVICE9 D3DDevice = static_cast<GFXD3DDevice *>(GFX)->getDevice();
   D3DDevice->SetTexture( tuNum, mCubeTex );
}

//-----------------------------------------------------------------------------
// Update dynamic cubemap
//-----------------------------------------------------------------------------
void GFXD3DCubemap::updateDynamic( Point3F &pos )
{
   if( !mCubeTex ) return;

#ifdef INIT_HACK
   if( mInit ) return;
   mInit = true;
#endif

   LPDIRECT3DDEVICE9 D3DDevice = dynamic_cast<GFXD3DDevice *>(GFX)->getDevice();

   GFX->pushActiveRenderSurfaces();

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
   
   gClientSceneGraph->setReflectPass( true );
   
   // Loop through the six faces of the cube map.
   for( DWORD i=0; i<6; i++ )
   {
      // Standard view that will be overridden below.
      VectorF vEnvEyePt = VectorF( 0.0f, 0.0f, 0.0f );
      VectorF vLookatPt, vUpVec, vRight;

      switch( i )
      {
         case D3DCUBEMAP_FACE_POSITIVE_X:
             vLookatPt = VectorF( 1.0f, 0.0f, 0.0f );
             vUpVec    = VectorF( 0.0f, 1.0f, 0.0f );
             break;
         case D3DCUBEMAP_FACE_NEGATIVE_X:
             vLookatPt = VectorF( -1.0f, 0.0f, 0.0f );
             vUpVec    = VectorF( 0.0f, 1.0f, 0.0f );
             break;
         case D3DCUBEMAP_FACE_POSITIVE_Y:
             vLookatPt = VectorF( 0.0f, 1.0f, 0.0f );
             vUpVec    = VectorF( 0.0f, 0.0f,-1.0f );
             break;
         case D3DCUBEMAP_FACE_NEGATIVE_Y:
             vLookatPt = VectorF( 0.0f, -1.0f, 0.0f );
             vUpVec    = VectorF( 0.0f, 0.0f, 1.0f );
             break;
         case D3DCUBEMAP_FACE_POSITIVE_Z:
             vLookatPt = VectorF( 0.0f, 0.0f, 1.0f );
             vUpVec    = VectorF( 0.0f, 1.0f, 0.0f );
             break;
         case D3DCUBEMAP_FACE_NEGATIVE_Z:
             vLookatPt = VectorF( 0.0f, 0.0f, -1.0f );
             vUpVec    = VectorF( 0.0f, 1.0f, 0.0f );
             break;
      }

      // create camera matrix
      VectorF cross = mCross( vUpVec, vLookatPt );
      cross.normalizeSafe();

      MatrixF matView(true);
      matView.setColumn( 0, cross );
      matView.setColumn( 1, vLookatPt );
      matView.setColumn( 2, vUpVec );
      matView.setPosition( pos );
      matView.inverse();

      GFX->setWorldMatrix(matView);


      // set new render target / zbuffer , clear buffers
      LPDIRECT3DSURFACE9 pFace;
      mCubeTex->GetCubeMapSurface( (D3DCUBEMAP_FACES)i, 0, &pFace );
      D3DDevice->SetRenderTarget( 0, pFace );
      D3DDevice->SetDepthStencilSurface( mDepthBuff );
      pFace->Release();

      D3DDevice->Clear( 0, NULL, D3DCLEAR_STENCIL | D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_XRGB(0,0,0), 1.0f, 0 );

      // render scene
      gClientSceneGraph->renderScene( InteriorObjectType );
   }
   
   gClientSceneGraph->setReflectPass( false );
   
   // restore render surface and depth buffer
   GFX->popActiveRenderSurfaces();
   D3DDevice->SetDepthStencilSurface( prevDepthSurface );
   prevDepthSurface->Release();
   
   // restore matrices
   GFX->popWorldMatrix();
   GFX->setProjectionMatrix( proj );
}

