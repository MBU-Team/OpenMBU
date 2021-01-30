//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _GFXD3DTypes_H_
#define _GFXD3DTypes_H_

#include "gfx/D3D/gfxD3DDevice.h"



//-----------------------------------------------------------------------------

class GFXD3DPrimitiveBuffer : public GFXPrimitiveBuffer
{
public:
    IDirect3DIndexBuffer9* ib;
    RefPtr<GFXD3DPrimitiveBuffer> mVolatileBuffer;
    U32 mVolatileStart;

    bool                  mIsFirstLock;

    GFXD3DPrimitiveBuffer(GFXDevice* device, U32 indexCount, U32 primitiveCount, GFXBufferType bufferType);
    ~GFXD3DPrimitiveBuffer();

    virtual void lock(U16 indexStart, U16 indexEnd, U16** indexPtr);
    virtual void unlock();

    virtual void prepare();

#ifdef TORQUE_DEBUG
    //GFXD3DPrimitiveBuffer *next;
#endif
};

inline GFXD3DPrimitiveBuffer::GFXD3DPrimitiveBuffer(GFXDevice* device, U32 indexCount, U32 primitiveCount, GFXBufferType bufferType)
    : GFXPrimitiveBuffer(device, indexCount, primitiveCount, bufferType)
{
    mVolatileStart = 0;
    ib = NULL;
    mIsFirstLock = true;
}

inline GFXD3DPrimitiveBuffer::~GFXD3DPrimitiveBuffer()
{
    if (mBufferType != GFXBufferTypeVolatile)
        SAFE_RELEASE(ib);
}

#endif
