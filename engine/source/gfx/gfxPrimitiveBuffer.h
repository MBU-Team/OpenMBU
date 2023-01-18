//-----------------------------------------------------------------------------
// Torque Game Engine Advanced
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _GFX_GFXPRIMITIVEBUFFER_H_
#define _GFX_GFXPRIMITIVEBUFFER_H_

//#include "core/refBase.h"
//#include "gfx/gfxResource.h"
#include "gfx/gfxStructs.h"

class GFXPrimitiveBuffer : public RefBase, public GFXResource
{
    friend class GFXPrimitiveBufferHandle;
    friend class GFXDevice;
public: //protected:
    U32 mIndexCount;
    U32 mPrimitiveCount;
    GFXBufferType mBufferType;
    GFXPrimitive *mPrimitiveArray;
    GFXDevice *mDevice;

#ifdef TORQUE_DEBUG
    // In debug builds we provide a TOC leak tracking system.
   static U32 smActivePBCount;
   static GFXPrimitiveBuffer *smHead;
   static void dumpActivePBs();

   StringTableEntry mDebugCreationPath;
   GFXPrimitiveBuffer *mDebugNext;
   GFXPrimitiveBuffer *mDebugPrev;
#endif

    GFXPrimitiveBuffer(GFXDevice *device, U32 indexCount, U32 primitiveCount, GFXBufferType bufferType)
    {
        mDevice = device;
        mIndexCount = indexCount;
        mPrimitiveCount = primitiveCount;
        mBufferType = bufferType;
        if(primitiveCount)
        {
            mPrimitiveArray = new GFXPrimitive[primitiveCount];
            dMemset((void *) mPrimitiveArray, 0, primitiveCount * sizeof(GFXPrimitive));
        }
        else
            mPrimitiveArray = NULL;

#if defined(TORQUE_DEBUG)
        // Active copy tracking.
      smActivePBCount++;
#if defined(TORQUE_ENABLE_PROFILER)
      mDebugCreationPath = gProfiler->getProfilePath();
#endif
      mDebugNext = smHead;
      mDebugPrev = NULL;

      if(smHead)
      {
         AssertFatal(smHead->mDebugPrev == NULL, "GFXPrimitiveBuffer::GFXPrimitiveBuffer - found unexpected previous in current head!");
         smHead->mDebugPrev = this;
      }

      smHead = this;
#endif
    }

    virtual ~GFXPrimitiveBuffer()
    {
        if( mPrimitiveArray != NULL )
        {
            delete [] mPrimitiveArray;
            mPrimitiveArray = NULL;
        }

#ifdef TORQUE_DEBUG
        if(smHead == this)
         smHead = this->mDebugNext;

      if(mDebugNext)
         mDebugNext->mDebugPrev = mDebugPrev;

      if(mDebugPrev)
         mDebugPrev->mDebugNext = mDebugNext;

      mDebugPrev = mDebugNext = NULL;

      smActivePBCount--;
#endif
    }

    virtual void lock(U16 indexStart, U16 indexEnd, U16 **indexPtr)=0; ///< locks this primitive buffer for writing into
    virtual void unlock()=0; ///< unlocks this primitive buffer.
    virtual void prepare()=0;  ///< prepares this primitive buffer for use on the device it was allocated on

    // GFXResource interface
    virtual void describeSelf(char* buffer, U32 sizeOfBuffer);
};

class GFXPrimitiveBufferHandle : public RefPtr<GFXPrimitiveBuffer>
{
    typedef RefPtr<GFXPrimitiveBuffer> Parent;
public:
    enum Constants {
        MaxIndexCount = 65535,
    };
    GFXPrimitiveBufferHandle() {};
    GFXPrimitiveBufferHandle(GFXDevice *theDevice, U32 indexCount, U32 primitiveCount, GFXBufferType bufferType)
    {
        set(theDevice, indexCount, primitiveCount, bufferType);
    }
    void set(GFXDevice *theDevice, U32 indexCount, U32 primitiveCount, GFXBufferType bufferType);
    void lock(U16 **indexBuffer, GFXPrimitive **primitiveBuffer = NULL, U32 indexStart = 0, U32 indexEnd = 0)
    {
        if(indexEnd == 0)
            indexEnd = getPointer()->mIndexCount;
        AssertFatal(indexStart < indexEnd && indexEnd <= getPointer()->mIndexCount, "Out of range index lock!");
        getPointer()->lock(indexStart, indexEnd, indexBuffer);
        if(primitiveBuffer)
            *primitiveBuffer = getPointer()->mPrimitiveArray;
    }
    void unlock()
    {
        getPointer()->unlock();
    }
    void prepare()
    {
        getPointer()->prepare();
    }
    bool operator==(const GFXPrimitiveBufferHandle &buffer) const {
        return getPointer() == buffer.getPointer();
    }
    GFXPrimitiveBufferHandle& operator=(GFXPrimitiveBuffer *ptr)
    {
        RefObjectRef::set(ptr);
        return *this;
    }
};

#endif
