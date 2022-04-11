//-----------------------------------------------------------------------------
// Torque Game Engine Advanced
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _GFX_FENCE_H_
#define _GFX_FENCE_H_

#include "gfx/gfxDevice.h"
#include "gfx/gfxResource.h"

class GFXFence : public GFXResource
{
protected:
   GFXDevice *mDevice;

public:
   /// The states returned by getStatus()
   enum FenceStatus
   {
      Unset,         ///< The fence has not been set
      Pending,       ///< The fence has been set and has not been hit
      Processed,     ///< The fence has been processed
      Unsupported    ///< A non-blocking query of fence status is not supported by the implementation
   };

public:
   GFXFence( GFXDevice *device ) : mDevice( device ) {};
   virtual ~GFXFence(){};

   /// This method inserts the fence into the command buffer
   virtual void issue() = 0;

   // CodeReview: Do we need a remove() [5/10/2007 Pat]

   /// This is a non-blocking call to get the status of the fence
   /// @see GFXFence::FenceStatus
   virtual FenceStatus getStatus() const = 0;

   /// This method will not return until the fence has been processed by the GPU
   virtual void block() = 0;
};

class GFXGeneralFence : public GFXFence
{
private:
   bool mInitialized;
   S32 mCallbackHandle;
   GFXTextureTargetRef mRenderTarget;
   GFXTexHandle mRTTexHandle;

   void _init();

public:
   GFXGeneralFence( GFXDevice *device ) : GFXFence( device ), mInitialized( false ), mCallbackHandle( -1 ) {};
   virtual ~GFXGeneralFence();

   virtual void issue();
   virtual FenceStatus getStatus() const { return GFXFence::Unsupported; };
   virtual void block();

   // To re-create render targets
   static void texManagerCallback( GFXTexCallbackCode code, void *userData );

   // GFXResource interface
   virtual void zombify();
   virtual void resurrect();
   virtual void describeSelf(char* buffer, U32 sizeOfBuffer);
};

#endif