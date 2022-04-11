//-----------------------------------------------------------------------------
// Torque Game Engine Advanced
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "gfx/D3D9/gfxD3D9Device.h"
#include "gfx/D3D9/gfxD3D9PrimitiveBuffer.h"
#include "util/safeRelease.h"

void GFXD3D9PrimitiveBuffer::lock(U16 indexStart, U16 indexEnd, U16 **indexPtr)
{
   AssertFatal(!mLocked, "GFXD3D9PrimitiveBuffer::lock - Can't lock a primitive buffer more than once!");
   mLocked = true;
   U32 flags=0;
   switch(mBufferType)
   {
   case GFXBufferTypeStatic:
      // flags |= D3DLOCK_DISCARD;
      break;
   case GFXBufferTypeDynamic:
      // AssertISV(false, "D3D doesn't support dynamic primitive buffers. -- BJG");
      // Does too. -- BJG
      break;
   case GFXBufferTypeVolatile:
      // Get our range now...
      AssertFatal(indexStart == 0,                "Cannot get a subrange on a volatile buffer.");
      AssertFatal(indexEnd < MAX_DYNAMIC_INDICES, "Cannot get more than MAX_DYNAMIC_INDICES in a volatile buffer. Up the constant!");

      // Get the primtive buffer
      mVolatileBuffer = ((GFXD3D9Device*)mDevice)->mDynamicPB;

      AssertFatal( mVolatileBuffer, "GFXD3D9PrimitiveBuffer::lock - No dynamic primitive buffer was available!");

      // We created the pool when we requested this volatile buffer, so assume it exists...
      if( mVolatileBuffer->mIndexCount + indexEnd > MAX_DYNAMIC_INDICES ) 
      {
         flags |= D3DLOCK_DISCARD;
         mVolatileStart = indexStart  = 0;
         indexEnd       = indexEnd;
      }
      else 
      {
         flags |= D3DLOCK_NOOVERWRITE;
         mVolatileStart = indexStart  = mVolatileBuffer->mIndexCount;
         indexEnd                    += mVolatileBuffer->mIndexCount;
      }

      mVolatileBuffer->mIndexCount = indexEnd + 1;
      ib = mVolatileBuffer->ib;

      break;
   }

   D3D9Assert( ib->Lock(indexStart * sizeof(U16), (indexEnd - indexStart) * sizeof(U16), (void**)indexPtr, flags),
      "GFXD3D9PrimitiveBuffer::lock - Could not lock primitive buffer.");

}

