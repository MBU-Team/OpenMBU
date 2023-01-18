//-----------------------------------------------------------------------------
// Torque Game Engine Advanced
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------
#include "gfx/D3D9/platformD3D.h"
#include "gfx/D3D9/gfxD3D9VertexBuffer.h"

GFXD3D9VertexBuffer::~GFXD3D9VertexBuffer() 
{
#ifdef TORQUE_DEBUG
   SAFE_DELETE( name );
#endif

   if (getOwningDevice() != NULL)
   {
      if (mBufferType == GFXBufferTypeDynamic)
         static_cast<GFXD3D9Device *>(getOwningDevice())->deallocVertexBuffer( this );
      else if (mBufferType != GFXBufferTypeVolatile)
         static_cast<GFXD3D9Device *>(getOwningDevice())->destroyD3DResource( vb );
   }
}

//-----------------------------------------------------------------------------
// Lock
//-----------------------------------------------------------------------------
void GFXD3D9VertexBuffer::lock(U32 vertexStart, U32 vertexEnd, void **vertexPtr)
{
   AssertFatal(lockedVertexStart == 0 && lockedVertexEnd == 0, "Cannot lock a buffer more than once!");

   U32 flags = 0;

   GFXD3D9Device *d = static_cast<GFXD3D9Device *>( mDevice );

   switch( mBufferType )
   {
   case GFXBufferTypeStatic:
      break;

   case GFXBufferTypeDynamic:
#ifndef TORQUE_OS_XENON
      flags |= D3DLOCK_DISCARD;
#endif
      break;

   case GFXBufferTypeVolatile:

      // Get or create the volatile buffer...
      mVolatileBuffer = d->findVBPool( mVertexType, vertexEnd );

      if( !mVolatileBuffer )
         mVolatileBuffer = d->createVBPool( mVertexType, mVertexSize );

      vb = mVolatileBuffer->vb;

      // Get our range now...
      AssertFatal(vertexStart == 0,              "Cannot get a subrange on a volatile buffer.");
      AssertFatal(vertexEnd < MAX_DYNAMIC_VERTS, "Cannot get more than MAX_DYNAMIC_VERTS in a volatile buffer. Up the constant!");
      AssertFatal(mVolatileBuffer->lockedVertexStart == 0 && mVolatileBuffer->lockedVertexEnd == 0, "Got more than one lock on the volatile pool.");

      // We created the pool when we requested this volatile buffer, so assume it exists...
      if( mVolatileBuffer->mNumVerts + vertexEnd > MAX_DYNAMIC_VERTS ) 
      {
#ifdef TORQUE_OS_XENON
         AssertFatal( false, "This should never, ever happen. findVBPool should have returned NULL" );
#else
         flags |= D3DLOCK_DISCARD;
#endif
         mVolatileStart = vertexStart  = 0;
         vertexEnd      = vertexEnd;
      }
      else 
      {
         flags |= D3DLOCK_NOOVERWRITE;
         mVolatileStart = vertexStart  = mVolatileBuffer->mNumVerts;
         vertexEnd                    += mVolatileBuffer->mNumVerts;
      }

      mVolatileBuffer->mNumVerts = vertexEnd+1;

      mVolatileBuffer->lockedVertexStart = vertexStart;
      mVolatileBuffer->lockedVertexEnd   = vertexEnd;
      break;
   }

   lockedVertexStart = vertexStart;
   lockedVertexEnd   = vertexEnd;

   //      Con::printf("%x: Locking %s range (%d, %d)", this, (mBufferType == GFXBufferTypeVolatile ? "volatile" : "static"), lockedVertexStart, lockedVertexEnd);

#ifdef TORQUE_OS_XENON
   // If the vertex buffer which we are trying to lock is held by the D3D device
   // on Xenon it will bomb. So if that is the case, then SetStreamSource to NULL
   // and also call setVertexBuffer because otherwise the state-cache will be hosed
   if( d->mCurrentVB != NULL && d->mCurrentVB->vb == vb )
   {
      d->setVertexBuffer( NULL );
      d->mD3DDevice->SetStreamSource( 0, NULL, 0, 0 );
   }
   
   // As of October 2006 XDK, range locking is no longer supported. Lock the whole buffer
   // and then manually offset the pointer to simulate the subrange. -patw
   D3D9Assert( vb->Lock( 0, 0, vertexPtr, flags),
      "Unable to lock vertex buffer.");

   U8 *tmp = (U8 *)(*vertexPtr);
   tmp += ( vertexStart * mVertexSize );
   *vertexPtr = tmp;
#else
   D3D9Assert( vb->Lock(vertexStart * mVertexSize, (vertexEnd - vertexStart) * mVertexSize, vertexPtr, flags),
      "Unable to lock vertex buffer.");
#endif
}

//-----------------------------------------------------------------------------
// Unlock
//-----------------------------------------------------------------------------
void GFXD3D9VertexBuffer::unlock()
{
   D3D9Assert( vb->Unlock(),
      "Unable to unlock vertex buffer.");
   mIsFirstLock = false;

   //      Con::printf("%x: Unlocking %s range (%d, %d)", this, (mBufferType == GFXBufferTypeVolatile ? "volatile" : "static"), lockedVertexStart, lockedVertexEnd);

   lockedVertexEnd = lockedVertexStart = 0;

   if(mVolatileBuffer.isValid())
   {
      mVolatileBuffer->lockedVertexStart = 0;
      mVolatileBuffer->lockedVertexEnd   = 0;
      mVolatileBuffer = NULL;
      //vb->Release();
      //vb = NULL;
   }
}

//-----------------------------------------------------------------------------
// Prepare
//-----------------------------------------------------------------------------
void GFXD3D9VertexBuffer::prepare()
{
   ((GFXD3D9Device*)mDevice)->setVB(this);
}

void GFXD3D9VertexBuffer::zombify()
{
   AssertFatal(lockedVertexStart == 0 && lockedVertexEnd == 0, "GFXD3D9VertexBuffer::zombify - Cannot zombify a locked buffer!");
   // Static buffers are managed by D3D9 so we don't deal with them.
   if(mBufferType == GFXBufferTypeDynamic)
   {
      SAFE_RELEASE(vb);
   }
}

void GFXD3D9VertexBuffer::resurrect()
{
   // Static buffers are managed by D3D9 so we don't deal with them.
   if(mBufferType == GFXBufferTypeDynamic)
   {
      D3D9Assert(static_cast<GFXD3D9Device*>(mDevice)->mD3DDevice->CreateVertexBuffer( mVertexSize * mNumVerts,
#ifdef TORQUE_OS_XENON
         D3DUSAGE_WRITEONLY,
#else
         D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY, 
#endif
         mVertexType, 
         D3DPOOL_DEFAULT,
         &vb,
         NULL ),
         "GFXD3D9VertexBuffer::resurrect - Failed to allocate VB" );
   }
}