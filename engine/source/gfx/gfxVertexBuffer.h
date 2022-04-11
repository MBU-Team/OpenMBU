//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (c) 2003 GarageGames.Com
//-----------------------------------------------------------------------------

#ifndef _GFXVERTEXBUFFER_H_
#define _GFXVERTEXBUFFER_H_

#ifndef _GFXSTRUCTS_H_
#include "gfx/gfxStructs.h"
#endif

#ifndef _REFBASE_H_
#include "core/refBase.h"
#endif

#include "gfx/gfxResource.h"


//*****************************************************************************
// GFXVertexBuffer - base vertex buffer class
//*****************************************************************************
class GFXVertexBuffer : public RefBase, public GFXResource
{
    friend class GFXVertexBufferHandleBase;
    friend class GFXDevice;

public:
    U32 mNumVerts;       ///< Number of vertices in this buffer
    U32 mVertexType;     ///< Vertex type as defined by OR'd vertex flags
    U32 mVertexSize;     ///< Vertex size in bytes
    GFXBufferType mBufferType; ///< GFX buffer type (static, dynamic or volatile)
    GFXDevice* mDevice;  ///< Device this vertex buffer was allocated on

    bool  isLocked;
    U32   lockedVertexStart;
    U32   lockedVertexEnd;
    void* lockedVertexPtr;
    U32   mVolatileStart;

    GFXVertexBuffer(GFXDevice* device, U32 numVerts, U32 vertexType, U32 vertexSize, GFXBufferType bufferType)
    {
        mVolatileStart = 0;
        mDevice = device;
        mNumVerts = numVerts;
        mVertexType = vertexType;
        mVertexSize = vertexSize;
        mBufferType = bufferType;
    }

    virtual void lock(U32 vertexStart, U32 vertexEnd, void** vertexPtr) = 0;
    virtual void unlock() = 0;
    virtual void prepare() = 0;
    //virtual void setSize(U32 newSize) = 0;


    // GFXResource interface
    virtual void describeSelf(char* buffer, U32 sizeOfBuffer);
};


//*****************************************************************************
// GFXVertexBufferHandleBase
//*****************************************************************************
class GFXVertexBufferHandleBase : public RefPtr<GFXVertexBuffer>
{
    friend class GFXDevice;

protected:
    void set(GFXDevice* theDevice, U32 numVerts, U32 flags, U32 vertexSize, GFXBufferType type);
    void* lock(U32 vertexStart, U32 vertexEnd)
    {
        if (vertexEnd == 0)
            vertexEnd = getPointer()->mNumVerts;
        AssertFatal(vertexEnd > vertexStart, "Can't get a lock with the end before the start.");
        AssertFatal(vertexEnd <= getPointer()->mNumVerts || getPointer()->mBufferType == GFXBufferTypeVolatile, "Tried to get vertices beyond the end of the buffer!");
        getPointer()->lock(vertexStart, vertexEnd, &getPointer()->lockedVertexPtr);
        return getPointer()->lockedVertexPtr;
    }
    void unlock() ///< unlocks the vertex data, making changes illegal.
    {
        getPointer()->unlock();
    }
};

//*****************************************************************************
// GFXVertexBufferHandle
//*****************************************************************************
template<class T> class GFXVertexBufferHandle : public GFXVertexBufferHandleBase
{
    void prepare() ///< sets this vertex buffer as the current vertex buffer for the device it was allocated on
    {
        getPointer()->prepare();
    }
public:
    GFXVertexBufferHandle() { }
    GFXVertexBufferHandle(GFXDevice* theDevice, U32 numVerts = 0, GFXBufferType bufferType = GFXBufferTypeVolatile)
    {
        set(theDevice, numVerts, bufferType);
    }
    void set(GFXDevice* theDevice, U32 numVerts = 0, GFXBufferType t = GFXBufferTypeVolatile)
    {
        GFXVertexBufferHandleBase::set(theDevice, numVerts, getGFXVertFlags((T*)NULL), sizeof(T), t);
    }
    T* lock(U32 vertexStart = 0, U32 vertexEnd = 0) ///< locks the vertex buffer range, and returns a pointer to the beginning of the vertex array
                                                    ///< also allows the array operators to work on this vertex buffer.
    {
        return (T*)GFXVertexBufferHandleBase::lock(vertexStart, vertexEnd);
    }
    void unlock()
    {
        GFXVertexBufferHandleBase::unlock();
    }
    void setSize(U32 newVertexCount); ///< resizes the vertex buffer.  This is only valid for vertex buffers allocated with
                                      ///< GFXBufferTypeVolatile as the buffer type.

    T& operator[](U32 index) ///< Array operator allows indexing into a locked vertex buffer.  The debug version of the code
                             ///< will range check the array access as well as validate the locked vertex buffer pointer.
    {
        return ((T*)getPointer()->lockedVertexPtr)[index];
    }
    const T& operator[](U32 index) const ///< Array operator allows indexing into a locked vertex buffer.  The debug version of the code
                                         ///< will range check the array access as well as validate the locked vertex buffer pointer.
    {
        index += getPointer()->mVolatileStart;
        AssertFatal(getPointer()->lockedVertexPtr != NULL, "Cannot access verts from an unlocked vertex buffer!!!");
        AssertFatal(index >= getPointer()->lockedVertexStart && index < getPointer()->lockedVertexEnd, "Out of range vertex access!");
        index -= getPointer()->mVolatileStart;
        return ((T*)getPointer()->lockedVertexPtr)[index];
    }
    T& operator[](S32 index) ///< Array operator allows indexing into a locked vertex buffer.  The debug version of the code
                             ///< will range check the array access as well as validate the locked vertex buffer pointer.
    {
        index += getPointer()->mVolatileStart;
        AssertFatal(getPointer()->lockedVertexPtr != NULL, "Cannot access verts from an unlocked vertex buffer!!!");
        AssertFatal(index >= getPointer()->lockedVertexStart && index < getPointer()->lockedVertexEnd, "Out of range vertex access!");
        index -= getPointer()->mVolatileStart;
        return ((T*)getPointer()->lockedVertexPtr)[index];
    }
    const T& operator[](S32 index) const ///< Array operator allows indexing into a locked vertex buffer.  The debug version of the code
                                         ///< will range check the array access as well as validate the locked vertex buffer pointer.
    {
        index += getPointer()->mVolatileStart;
        AssertFatal(getPointer()->lockedVertexPtr != NULL, "Cannot access verts from an unlocked vertex buffer!!!");
        AssertFatal(index >= getPointer()->lockedVertexStart && index < getPointer()->lockedVertexEnd, "Out of range vertex access!");
        index -= getPointer()->mVolatileStart;
        return ((T*)getPointer()->lockedVertexPtr)[index];
    }
    GFXVertexBufferHandle<T>& operator=(GFXVertexBuffer* ptr)
    {
        RefObjectRef::set(ptr);
        return *this;
    }

};

#endif


