//-----------------------------------------------------------------------------
// Torque Game Engine Advanced
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------
#include "gfx/D3D9/gfxD3D9Device.h"
#include "gfx/D3D9/gfxD3D9Cubemap.h"
#include "sceneGraph/sceneGraph.h"

// Set to make dynamic cubemaps only update once.
//#define INIT_HACK

_D3DCUBEMAP_FACES GFXD3D9Cubemap::faceList[6] = 
{ 
   D3DCUBEMAP_FACE_POSITIVE_X, D3DCUBEMAP_FACE_NEGATIVE_X,
   D3DCUBEMAP_FACE_POSITIVE_Y, D3DCUBEMAP_FACE_NEGATIVE_Y,
   D3DCUBEMAP_FACE_POSITIVE_Z, D3DCUBEMAP_FACE_NEGATIVE_Z
};

//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
GFXD3D9Cubemap::GFXD3D9Cubemap()
{
   mRenderTarget = NULL;
   mCubeTex = NULL;
   mDepthBuff = NULL;
   mDynamic = false;
   mCallbackHandle = -1;
#ifdef MB_CUBEMAP_FAST
   mNumFacesPerUpdate = 2;
#else
   mNumFacesPerUpdate = 6;
#endif
   mCurrentFace = 0;

   #ifdef INIT_HACK
   mInit = false;
   #endif
}

//-----------------------------------------------------------------------------
// Destructor
//-----------------------------------------------------------------------------
GFXD3D9Cubemap::~GFXD3D9Cubemap()
{
   releaseSurfaces();
   
   if( mDynamic )
   {
       if (GFXDevice::devicePresent())
       {
           GFX->unregisterTexCallback(mCallbackHandle);
       }
   }
}

//-----------------------------------------------------------------------------
// Release D3D surfaces
//-----------------------------------------------------------------------------
void GFXD3D9Cubemap::releaseSurfaces()
{
   if( mCubeTex )
   {
      mCubeTex->Release();
      mCubeTex = NULL;
   }
}

//-----------------------------------------------------------------------------
// Texture manager callback - for resetting textures on video change
//-----------------------------------------------------------------------------
void GFXD3D9Cubemap::texManagerCallback( GFXTexCallbackCode code, void *userData )
{
   GFXD3D9Cubemap *cubemap = reinterpret_cast<GFXD3D9Cubemap *>( userData );
   
   if( !cubemap->mDynamic ) 
      return;
   
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
void GFXD3D9Cubemap::initStatic( GFXTexHandle *faces )
{
   if( mCubeTex )
   {
      return;
   }

   if( faces )
   {
      AssertFatal( faces[0], "empty texture passed to CubeMap::create" );


      LPDIRECT3DDEVICE9 D3D9Device = static_cast<GFXD3D9Device *>(GFX)->getDevice();     
      
      // NOTE - check tex sizes on all faces - they MUST be all same size
      U32 texSize = faces[0].getWidth();

      D3D9Assert( D3D9Device->CreateCubeTexture( texSize, 1, 0, D3DFMT_A8R8G8B8,
                 D3DPOOL_MANAGED, &mCubeTex, NULL ), NULL );


      fillCubeTextures( faces, D3D9Device );
//      mCubeTex->GenerateMipSubLevels();
   }
}


//-----------------------------------------------------------------------------
// Init Dynamic
//-----------------------------------------------------------------------------
void GFXD3D9Cubemap::initDynamic( U32 texSize )
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
   
   LPDIRECT3DDEVICE9 D3D9Device = reinterpret_cast<GFXD3D9Device *>(GFX)->getDevice();

   // might want to try this as a 16 bit texture...
   D3D9Assert( D3D9Device->CreateCubeTexture( texSize,
                                            1, 
#ifdef TORQUE_OS_XENON
                                            0,
#else
                                            D3DUSAGE_RENDERTARGET, 
#endif
                                            D3DFMT_A8R8G8B8,
                                            D3DPOOL_DEFAULT, 
                                            &mCubeTex, 
                                            NULL ), NULL );

   mDepthBuff = GFXTexHandle( texSize, texSize, GFXFormatD24S8, &GFXDefaultZTargetProfile);

   mRenderTarget = GFX->allocRenderToTextureTarget();
}

//-----------------------------------------------------------------------------
// Fills in face textures of cube map from existing textures
//-----------------------------------------------------------------------------
void GFXD3D9Cubemap::fillCubeTextures( GFXTexHandle *faces, LPDIRECT3DDEVICE9 D3DDevice )
{
   for( U32 i=0; i<6; i++ )
   {
      // get cube face surface
      IDirect3DSurface9 *cubeSurf = NULL;
      D3D9Assert( mCubeTex->GetCubeMapSurface( faceList[i], 0, &cubeSurf ), NULL );

      // get incoming texture surface
      GFXD3D9TextureObject *texObj = dynamic_cast<GFXD3D9TextureObject*>( (GFXTextureObject*)faces[i] );
      IDirect3DSurface9 *inSurf;
      D3D9Assert( texObj->get2DTex()->GetSurfaceLevel( 0, &inSurf ), NULL );
      
      // copy incoming texture into cube face
      D3D9Assert( GFXD3DX.D3DXLoadSurfaceFromSurface( cubeSurf, NULL, NULL, inSurf, NULL, 
                                  NULL, D3DX_FILTER_NONE, 0 ), NULL );
      cubeSurf->Release();
      inSurf->Release();
   }
}

//-----------------------------------------------------------------------------
// Set the cubemap to the specified texture unit num
//-----------------------------------------------------------------------------
void GFXD3D9Cubemap::setToTexUnit( U32 tuNum )
{
   static_cast<GFXD3D9Device *>(GFX)->getDevice()->SetTexture( tuNum, mCubeTex );
}

//-----------------------------------------------------------------------------
// Update dynamic cubemap
//-----------------------------------------------------------------------------
void GFXD3D9Cubemap::updateDynamic( const Point3F &pos )
{
#ifndef TORQUE_TGB_ONLY

   if( mCubeTex == NULL ) 
      return;

#ifdef INIT_HACK
   if( mInit ) return;
   mInit = true;
#endif

   GFX->pushActiveRenderTarget();
   mRenderTarget->attachTexture(GFXTextureTarget::DepthStencil, mDepthBuff );

   // store current matrices
   GFX->pushWorldMatrix();
   MatrixF proj = GFX->getProjectionMatrix();

   // set projection to 90 degrees vertical and horizontal
   MatrixF matProj;
   GFXD3DX.D3DXMatrixPerspectiveFovRH( (D3DXMATRIX *) &matProj, D3DX_PI/2, 1.0f, 0.1f, 1000.0f );
   matProj.transpose();

   // set projection to match Torque space
   MatrixF rotMat(EulerF( F32(M_PI / 2.0), 0.0f, 0.0f));

   matProj.mul(rotMat);

   //if (smReflectionDetailLevel < 3)
   //     GFX->setFrustum((M_PI / 2.0), 1.0f, 0.1f, 1000.0f);

   GFX->setProjectionMatrix(matProj);
   
   getCurrentClientSceneGraph()->setReflectPass( true );
   
   // Loop through the six faces of the cube map.
   for( DWORD i = 0; i < mNumFacesPerUpdate; i++ )
   {
       int faceIdx = (mCurrentFace + i) % 6;
      // Standard view that will be overridden below.
      VectorF vLookatPt, vUpVec, vRight;

      switch(faceIdx)
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

      mRenderTarget->attachTexture( GFXTextureTarget::Color0, this, faceIdx);
      GFX->setActiveRenderTarget( mRenderTarget );
      GFX->clear( GFXClearStencil | GFXClearTarget | GFXClearZBuffer, ColorI( 0, 0, 0 ), 1.f, 0 );

      // render scene
      switch (smReflectionDetailLevel)
      {
          case 1: // Sky
              getCurrentClientSceneGraph()->renderScene( EnvironmentObjectType );
              break;
          case 2: // Level
              getCurrentClientSceneGraph()->renderScene( TerrainObjectType | InteriorObjectType | EnvironmentObjectType );
              break;
          case 3: // Items
              getCurrentClientSceneGraph()->renderScene( TerrainObjectType | InteriorObjectType | EnvironmentObjectType | ItemObjectType );
              break;
          case 4: // Static Shapes like glass and pads
              getCurrentClientSceneGraph()->renderScene( TerrainObjectType | InteriorObjectType | EnvironmentObjectType | ItemObjectType | StaticShapeObjectType | StaticTSObjectType );
              break;
          default: // No Reflection
              break;
      }
   }

   mCurrentFace = (mCurrentFace + mNumFacesPerUpdate) % 6;

    getCurrentClientSceneGraph()->setReflectPass( false );
   
   // restore render surface and depth buffer
   GFX->popActiveRenderTarget();
   
   mRenderTarget->clearAttachments();

   // restore matrices
   GFX->popWorldMatrix();
   GFX->setProjectionMatrix( proj );
#endif
}

void GFXD3D9Cubemap::zombify()
{
   // Static cubemaps are handled by D3D
   if( mDynamic )
   {
      releaseSurfaces();
   }
}

void GFXD3D9Cubemap::resurrect()
{
   // Static cubemaps are handled by D3D
   if( mDynamic )
   {
      initDynamic( mTexSize );
   }
}
