//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _GFXD3D_VERTEXBUFFER_H_
#define _GFXD3D_VERTEXBUFFER_H_

#include "gfx/gfxVertexBuffer.h"
#include "gfx/D3D/gfxD3DDevice.h"

//*****************************************************************************
// GFXD3DVertexBuffer 
//*****************************************************************************
class GFXD3DVertexBuffer : public GFXVertexBuffer
{
public:
    IDirect3DVertexBuffer9* vb;
    RefPtr<GFXD3DVertexBuffer> mVolatileBuffer;

    bool                       mIsFirstLock;

    GFXD3DVertexBuffer();
    GFXD3DVertexBuffer(GFXDevice* device, U32 numVerts, U32 vertexType, U32 vertexSize, GFXBufferType bufferType);
    ~GFXD3DVertexBuffer();

    void lock(U32 vertexStart, U32 vertexEnd, void** vertexPtr);
    void unlock();
    void prepare();

    void setSize(U32 newSize)
    {
        AssertISV(false, "Implement this -- BJG");
    }

#ifdef TORQUE_DEBUG
    char* name;

    /// In debug compile, the verts will be chained together and the device
    /// will examine the chain when it's destructor is called, this will
    /// allow developers to see which vertex buffers are not destroyed
    GFXD3DVertexBuffer* next;
#endif
    void setName(const char* n);
};

//-----------------------------------------------------------------------------
// This is for debugging vertex buffers and trying to track down which vbs
// aren't getting free'd

inline GFXD3DVertexBuffer::GFXD3DVertexBuffer()
    : GFXVertexBuffer(0, 0, 0, 0, (GFXBufferType)0)
{
#ifdef TORQUE_DEBUG
    name = NULL;
#endif
    vb = NULL;
    mIsFirstLock = true;
    lockedVertexEnd = lockedVertexStart = 0;
}

inline GFXD3DVertexBuffer::GFXD3DVertexBuffer(GFXDevice* device, U32 numVerts, U32 vertexType, U32 vertexSize, GFXBufferType bufferType)
    : GFXVertexBuffer(device, numVerts, vertexType, vertexSize, bufferType)
{
#ifdef TORQUE_DEBUG
    name = NULL;
#endif
    vb = NULL;
    mIsFirstLock = true;
    lockedVertexEnd = lockedVertexStart = 0;
}

inline GFXD3DVertexBuffer::~GFXD3DVertexBuffer()
{

#ifdef TORQUE_DEBUG
    SAFE_DELETE(name);
#endif

    if (mBufferType == GFXBufferTypeDynamic)
    {
        ((GFXD3DDevice*)mDevice)->deallocVertexBuffer(this);
    }

    // don't want to kill entire volatile pool  
    if (mBufferType != GFXBufferTypeVolatile)
    {
        SAFE_RELEASE(vb);
    }
}

#ifdef TORQUE_DEBUG

inline void GFXD3DVertexBuffer::setName(const char* n)
{
    SAFE_DELETE(name);
    name = new char[dStrlen(n)];
    dStrcpy(name, n);
}

#else

inline void GFXD3DVertexBuffer::setName(const char* n) { }

#endif

#endif // _GFXD3D_VERTEXBUFFER_H_

