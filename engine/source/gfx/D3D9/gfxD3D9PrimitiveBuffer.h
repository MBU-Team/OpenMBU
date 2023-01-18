//-----------------------------------------------------------------------------
// Torque Game Engine Advanced
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _GFX_D3D9_GFXD3D9PRIMITIVEBUFFER_H_
#define _GFX_D3D9_GFXD3D9PRIMITIVEBUFFER_H_

#include "gfx/gfxStructs.h"

struct IDirect3DIndexBuffer9;

class GFXD3D9PrimitiveBuffer : public GFXPrimitiveBuffer
{
   public:
      IDirect3DIndexBuffer9 *ib;
      RefPtr<GFXD3D9PrimitiveBuffer> mVolatileBuffer;
      U32 mVolatileStart;

      bool mLocked;
      bool                  mIsFirstLock;

      GFXD3D9PrimitiveBuffer(GFXDevice *device, U32 indexCount, U32 primitiveCount, GFXBufferType bufferType);
      ~GFXD3D9PrimitiveBuffer();

      virtual void lock(U16 indexStart, U16 indexEnd, U16 **indexPtr);
      virtual void unlock();

      virtual void prepare();      

#ifdef TORQUE_DEBUG
   //GFXD3D9PrimitiveBuffer *next;
#endif

      // GFXResource interface
      virtual void zombify();
      virtual void resurrect();
};

inline GFXD3D9PrimitiveBuffer::GFXD3D9PrimitiveBuffer(GFXDevice *device, U32 indexCount, U32 primitiveCount, GFXBufferType bufferType) 
   : GFXPrimitiveBuffer(device, indexCount, primitiveCount, bufferType)
{
   mVolatileStart = 0;
   ib             = NULL;
   mIsFirstLock   = true;
   mLocked = false;
}

#endif
