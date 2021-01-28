//-----------------------------------------------------------------------------
// Torque Shader Engine
// 
// Copyright (c) 2003 GarageGames.Com
//-----------------------------------------------------------------------------

#include "d3d9.h"
#include "d3dx9math.h"
#include "gfx/D3D/gfxD3DDevice.h"



void GFXD3DPrimitiveBuffer::lock(U16 indexStart, U16 indexEnd, U16 **indexPtr)
{
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

      // Get the primitive buffer
      mVolatileBuffer = ((GFXD3DDevice*)mDevice)->mDynamicPB;

      AssertFatal( mVolatileBuffer, "No dynamic primitive buffer was available!");

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

   D3DAssert( ib->Lock(indexStart * sizeof(U16), (indexEnd - indexStart) * sizeof(U16), (void**)indexPtr, flags),
               "Could not lock primitive buffer.");

}

void GFXD3DPrimitiveBuffer::unlock()
{
   D3DAssert( ib->Unlock(),
               "Failed to unlock primitive buffer.");

   mIsFirstLock = false;
}

void GFXD3DPrimitiveBuffer::prepare()
{
	((GFXD3DDevice*)mDevice)->setPrimitiveBuffer(this);
}

