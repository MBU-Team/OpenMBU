//-----------------------------------------------------------------------------
// Torque Game Engine Advanced
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "gfx/gfxFence.h"
#include "gfx/primBuilder.h"

GFXGeneralFence::~GFXGeneralFence()
{
   // Release the ref pointers
   mRenderTarget = NULL;
   mRTTexHandle = NULL;

   // Unregister texture callback
   if( GFXDevice::devicePresent() )
      GFX->unregisterTexCallback( mCallbackHandle );

   mCallbackHandle = -1;
}

//------------------------------------------------------------------------------

void GFXGeneralFence::_init()
{
   // Set this to true for error checking
   mInitialized = true;

   // Allocate resources
   mInitialized &= mRTTexHandle.set( 2, 2, GFXFormatR8G8B8X8, &GFXDefaultRenderTargetProfile );
   mRenderTarget = GFX->allocRenderToTextureTarget();
   mInitialized &= ( mRenderTarget != NULL );

   // Register texture callback to re-create resources
   if( mCallbackHandle == -1 )
      GFX->registerTexCallback( texManagerCallback, (void*) this, mCallbackHandle );
}

//------------------------------------------------------------------------------

void GFXGeneralFence::issue()
{
   PROFILE_START(GFXGeneralFence_Issue);

   // Resource creation will be done on first call to issue()
   if( !mInitialized )
      _init();

   // If we can't init, return.  Note that GL does not support RenderTargets so init always fails on GL
   if(!mInitialized)
   {
      PROFILE_END();
      return;
   }

   AssertFatal( mInitialized, "Error occured during GFXGeneralFence::_init, sorry I can't be more specific, break and debug." );

   RectI viewport = GFX->getViewport();

   GFX->pushActiveRenderTarget();
   mRenderTarget->attachTexture( GFXTextureTarget::Color0, mRTTexHandle );
   GFX->setActiveRenderTarget( mRenderTarget );

   // Set-up states
   GFX->setTexture( 0, NULL );
   GFX->setAlphaBlendEnable( false );
   GFX->pushWorldMatrix();
   GFX->setWorldMatrix( MatrixF(true) );

   // CodeReview: We can re-do this with a static vertex buffer at some point. [5/9/2007 Pat]
   PrimBuild::begin( GFXTriangleList, 3 );
      PrimBuild::vertex2f( 0.f, 0.f );
      PrimBuild::vertex2f( 0.f, 1.f );
      PrimBuild::vertex2f( 1.f, 0.f );
   PrimBuild::end();

   GFX->popWorldMatrix();
   GFX->popActiveRenderTarget();

   GFX->setViewport(viewport);

   PROFILE_END();
}

//------------------------------------------------------------------------------

void GFXGeneralFence::block()
{
   PROFILE_SCOPE(GFXGeneralFence_block);
   if( !mInitialized )
      return;

   // We have to deal with the case where the lock fails (usually due to 
   // a device reset).
   if(mRTTexHandle.lock())
      mRTTexHandle.unlock();
}

//------------------------------------------------------------------------------

void GFXGeneralFence::texManagerCallback( GFXTexCallbackCode code, void *userData )
{
   GFXGeneralFence *fence = reinterpret_cast<GFXGeneralFence *>( userData );

   switch( code )
   {
   case GFXZombify:
      fence->mRTTexHandle = NULL;
      break;

   case GFXResurrect:
      fence->mRTTexHandle.set( 2, 2, GFXFormatR8G8B8X8, &GFXDefaultRenderTargetProfile );
      break;
   }
}

void GFXGeneralFence::zombify()
{
   mRTTexHandle = NULL;
}

void GFXGeneralFence::resurrect()
{
   mRTTexHandle.set( 2, 2, GFXFormatR8G8B8X8, &GFXDefaultRenderTargetProfile );
}

void GFXGeneralFence::describeSelf( char* buffer, U32 sizeOfBuffer )
{
   // We've got nothing
   buffer[0] = NULL;
}