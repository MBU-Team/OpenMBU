//-----------------------------------------------------------------------------
// Torque Game Engine Advanced
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "gfx/gfxPrimitiveBuffer.h"
#include "gfx/gfxDevice.h"

//-----------------------------------------------------------------------------
#ifdef TORQUE_DEBUG
GFXPrimitiveBuffer *GFXPrimitiveBuffer::smHead = NULL;
U32 GFXPrimitiveBuffer::smActivePBCount = 0;

void GFXPrimitiveBuffer::dumpActivePBs()
{
   if(!smActivePBCount)
   {
      Con::printf("GFXPrimitiveBuffer::dumpActivePBs - no currently active PBs to dump. You are A-OK!");
      return;
   }

   Con::printf("GFXPrimitiveBuffer Usage Report - %d active PBs", smActivePBCount);
   Con::printf("---------------------------------------------------------------");
   Con::printf(" Addr  #idx #prims Profiler Path     RefCount");
   for(GFXPrimitiveBuffer *walk = smHead; walk; walk=walk->mDebugNext)
   {
#if defined(TORQUE_ENABLE_PROFILER)
      Con::printf(" %x  %6d %6d %s %d", walk, walk->mIndexCount, walk->mPrimitiveCount, walk->mDebugCreationPath, walk->mRefCount);
#else
      Con::printf(" %x  %6d %6d %s %d", walk, walk->mIndexCount, walk->mPrimitiveCount, "", walk->mRefCount);
#endif
   }
   Con::printf("----- dump complete -------------------------------------------");
   AssertFatal(false, "There is a primitive buffer leak, check the log for more details.");
}

#endif

void GFXPrimitiveBuffer::describeSelf( char* buffer, U32 sizeOfBuffer )
{
#if defined(TORQUE_DEBUG) && defined(TORQUE_ENABLE_PROFILER)
    dSprintf(buffer, sizeOfBuffer, "indexCount: %6d primCount: %6d refCount: %d path: %s",
      mIndexCount, mPrimitiveCount, mRefCount, mDebugCreationPath);
#else
    dSprintf(buffer, sizeOfBuffer, "indexCount: %6d primCount: %6d refCount: %d path: %s",
             mIndexCount, mPrimitiveCount, mRefCount, "");
#endif
}

//-----------------------------------------------------------------------------
// Set
//-----------------------------------------------------------------------------
void GFXPrimitiveBufferHandle::set(GFXDevice *theDevice, U32 indexCount, U32 primitiveCount, GFXBufferType bufferType)
{
    RefPtr<GFXPrimitiveBuffer>::operator=( theDevice->allocPrimitiveBuffer(indexCount, primitiveCount, bufferType) );
}
