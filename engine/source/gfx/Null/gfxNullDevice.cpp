//-----------------------------------------------------------------------------
// Torque Game Engine Advanced
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "gfx/Null/gfxNullDevice.h"
#include "gfx/gfxCubemap.h"
#include "gfx/screenshot.h"

class GFXNullTextureObject : public GFXTextureObject 
{
public:
   GFXNullTextureObject(GFXDevice * aDevice, GFXTextureProfile *profile); 
   ~GFXNullTextureObject() { kill(); };

   virtual void pureVirtualCrash() { };

   virtual GFXLockedRect * lock( U32 mipLevel = 0, RectI *inRect = NULL ) { return NULL; };
   virtual void unlock( U32 mipLevel = 0) {};
   virtual bool copyToBmp(GBitmap *) { return false; };
   virtual bool readBackBuffer(Point2I &) { return false; };

   virtual void zombify() {}
   virtual void resurrect() {}
};

GFXNullTextureObject::GFXNullTextureObject(GFXDevice * aDevice, GFXTextureProfile *profile) :
   GFXTextureObject(aDevice, profile) 
{
   mProfile = profile;
   mTextureSize.set( 0, 0, 0 );
}

class GFXNullTextureManager : public GFXTextureManager
{
protected:
      virtual GFXTextureObject *_createTexture(U32 height, U32 width, U32 depth, GFXFormat format, GFXTextureProfile *profile, U32 numMipLevels, bool forceMips = false) override
      { 
         GFXNullTextureObject* to = new GFXNullTextureObject(GFX, profile);
         to->mBitmap = new GBitmap(width, height);
         return to;
      };

      /// Load a texture from a proper DDSFile instance.
      virtual bool _loadTexture(GFXTextureObject *texture, DDSFile *dds) override { return true; };

      /// Load data into a texture from a GBitmap using the internal API.
      virtual bool _loadTexture(GFXTextureObject *texture, GBitmap *bmp) override { return true; };

      /// Load data into a texture from a raw buffer using the internal API.
      ///
      /// Note that the size of the buffer is assumed from the parameters used
      /// for this GFXTextureObject's _createTexture call.
      virtual bool _loadTexture(GFXTextureObject *texture, void *raw) override { return true; };

      /// Refresh a texture using the internal API.
      virtual bool _refreshTexture(GFXTextureObject *texture) override { return true; };

      /// Free a texture (but do not delete the GFXTextureObject) using the internal
      /// API.
      ///
      /// This is only called during zombification for textures which need it, so you
      /// don't need to do any internal safety checks.
      virtual bool _freeTexture(GFXTextureObject *texture, bool zombify=false) override { return true; };

      /// Returns available VRAM in bytes.
      virtual U32 _getTotalVideoMemory() override { return 0; };
};

class GFXNullCubemap : public GFXCubemap
{
   friend class GFXDevice;
private:
   // should only be called by GFXDevice
   virtual void setToTexUnit( U32 tuNum ) override { };

public:
   virtual void initStatic( GFXTexHandle *faces ) override { };
   virtual void initDynamic( U32 texSize ) override { };
   

   // pos is the position to generate cubemap from
   virtual void updateDynamic( const Point3F &pos ) override { };
   
   virtual ~GFXNullCubemap(){};

   virtual void zombify() {}
   virtual void resurrect() {}
};

class GFXNullVertexBuffer : public GFXVertexBuffer 
{
   unsigned char* tempBuf;
public:
   GFXNullVertexBuffer(GFXDevice *device, U32 numVerts, U32 vertexType, U32 vertexSize, GFXBufferType bufferType) :
      GFXVertexBuffer(device, numVerts, vertexType, vertexSize, bufferType) { };
   virtual void lock(U32 vertexStart, U32 vertexEnd, void **vertexPtr) override;
   virtual void unlock() override;
   virtual void prepare() override;

   virtual void zombify() {}
   virtual void resurrect() {}
};

void GFXNullVertexBuffer::lock(U32 vertexStart, U32 vertexEnd, void **vertexPtr) 
{
   tempBuf = new unsigned char[(vertexEnd - vertexStart) * mVertexSize];
   *vertexPtr = (void*) tempBuf;
   lockedVertexStart = vertexStart;
   lockedVertexEnd   = vertexEnd;
}

void GFXNullVertexBuffer::unlock() 
{
   delete[] tempBuf;
   tempBuf = NULL;
}

void GFXNullVertexBuffer::prepare() 
{
}

class GFXNullPrimitiveBuffer : public GFXPrimitiveBuffer
{
private:
   U16* temp;
public:
   GFXNullPrimitiveBuffer(GFXDevice *device, U32 indexCount, U32 primitiveCount, GFXBufferType bufferType) :
      GFXPrimitiveBuffer(device, indexCount, primitiveCount, bufferType) {};

   virtual void lock(U16 indexStart, U16 indexEnd, U16 **indexPtr); ///< locks this primitive buffer for writing into
   virtual void unlock(); ///< unlocks this primitive buffer.
   virtual void prepare() { };  ///< prepares this primitive buffer for use on the device it was allocated on

   virtual void zombify() {}
   virtual void resurrect() {}
};

void GFXNullPrimitiveBuffer::lock(U16 indexStart, U16 indexEnd, U16 **indexPtr)
{
   temp = new U16[indexEnd - indexStart];
   *indexPtr = temp;
}

void GFXNullPrimitiveBuffer::unlock() 
{
   delete[] temp;
   temp = NULL;
}

GFXDevice *GFXNullDevice::createInstance( U32 adapterIndex )
{
   return new GFXNullDevice();
}

GFXNullDevice::GFXNullDevice()
{
   viewport.set(0, 0, 800, 600);
   clip.set(0, 0, 800, 800);

   mTextureManager = new GFXNullTextureManager();
   gScreenShot = new ScreenShot();
}

GFXNullDevice::~GFXNullDevice()
{
   delete mTextureManager;
   mTextureManager = NULL;
}

GFXVertexBuffer *GFXNullDevice::allocVertexBuffer( U32 numVerts, U32 vertFlags, U32 vertSize, GFXBufferType bufferType ) 
{
   return new GFXNullVertexBuffer(GFX, numVerts, 0, vertSize, bufferType);
}

GFXPrimitiveBuffer *GFXNullDevice::allocPrimitiveBuffer( U32 numIndices, U32 numPrimitives, GFXBufferType bufferType ) 
{
   return new GFXNullPrimitiveBuffer(GFX, numIndices, numPrimitives, bufferType);
}

GFXCubemap* GFXNullDevice::createCubemap()
{ 
   return new GFXNullCubemap(); 
};

void GFXNullDevice::enumerateAdapters( Vector<GFXAdapter*> &adapterList )
{
   // Add the NULL renderer
   GFXAdapter *toAdd = new GFXAdapter();

   toAdd->mIndex = 0;
   toAdd->mType  = NullDevice;

   GFXVideoMode vm;
   vm.bitDepth = 32;
   vm.resolution.set(800,600);
   toAdd->mAvailableModes.push_back(vm);

   dStrcpy(toAdd->mName, "GFX Null Device");

   adapterList.push_back(toAdd);
}

void GFXNullDevice::setLightInternal(U32 lightStage, const LightInfo light, bool lightEnable)
{

}