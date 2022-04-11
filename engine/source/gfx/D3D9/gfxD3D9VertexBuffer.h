//-----------------------------------------------------------------------------
// Torque Game Engine Advanced
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _GFXD3D_VERTEXBUFFER_H_
#define _GFXD3D_VERTEXBUFFER_H_

#include "gfx/gfxVertexBuffer.h"
#include "gfx/D3D9/gfxD3D9Device.h"

//*****************************************************************************
// GFXD3D9VertexBuffer 
//*****************************************************************************
class GFXD3D9VertexBuffer : public GFXVertexBuffer
{
public:
   IDirect3DVertexBuffer9 *vb;
   RefPtr<GFXD3D9VertexBuffer> mVolatileBuffer;

   bool mIsFirstLock;
   bool mClearAtFrameEnd;

   GFXD3D9VertexBuffer();
   GFXD3D9VertexBuffer(GFXDevice *device, U32 numVerts, U32 vertexType, U32 vertexSize, GFXBufferType bufferType);
   virtual ~GFXD3D9VertexBuffer();

   void lock(U32 vertexStart, U32 vertexEnd, void **vertexPtr);
   void unlock();
   void prepare();


#ifdef TORQUE_DEBUG
   char *name; 

   /// In debug compile, the verts will be chained together and the device
   /// will examine the chain when it's destructor is called, this will
   /// allow developers to see which vertex buffers are not destroyed
   GFXD3D9VertexBuffer *next;
#endif
   void setName( const char *n );

   // GFXResource interface
   virtual void zombify();
   virtual void resurrect();
};

//-----------------------------------------------------------------------------
// This is for debugging vertex buffers and trying to track down which vbs
// aren't getting free'd

inline GFXD3D9VertexBuffer::GFXD3D9VertexBuffer()
: GFXVertexBuffer(0,0,0,0,(GFXBufferType)0)
{
#ifdef TORQUE_DEBUG
   name = NULL;
#endif
   vb = NULL;
   mIsFirstLock = true;
   lockedVertexEnd = lockedVertexStart = 0;
   mClearAtFrameEnd = false;
}

inline GFXD3D9VertexBuffer::GFXD3D9VertexBuffer(GFXDevice *device, U32 numVerts, U32 vertexType, U32 vertexSize, GFXBufferType bufferType)
: GFXVertexBuffer(device, numVerts, vertexType, vertexSize, bufferType)
{
#ifdef TORQUE_DEBUG
   name = NULL;
#endif
   vb = NULL;
   mIsFirstLock = true;
   mClearAtFrameEnd = false;
   lockedVertexEnd = lockedVertexStart = 0;
}

#ifdef TORQUE_DEBUG

inline void GFXD3D9VertexBuffer::setName( const char *n ) 
{
   SAFE_DELETE( name );
   name = new char[dStrlen( n )];
   dStrcpy( name, n );
}

#else

inline void GFXD3D9VertexBuffer::setName( const char *n ) { }

#endif

#endif // _GFXD3D_VERTEXBUFFER_H_

