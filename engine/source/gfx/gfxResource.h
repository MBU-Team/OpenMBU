//-----------------------------------------------------------------------------
// Torque Game Engine Advanced
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _GFX_GFXRESOURCE_H_
#define _GFX_GFXRESOURCE_H_

#include "platform/types.h"

class GFXDevice;

/// Mixin for the purpose of tracking GFX resources owned by a GFXDevice.
///
/// There are many types of resource that are allocated from a GFXDevice that
/// must be participatory in device resets. For instance, all default pool
/// DirectX resources have to be involved when the device resets. Render
/// targets in all APIs need to unbind themselves when resets happen.
///
/// This system is also handy for accounting purposes. For instance, we may
/// want to traverse all registered VBs, IBs, Textures, or RTs in order to
/// determine what, if any, items are still allocated. This can be used in
/// leak reports, memory usage reports, etc.
class GFXResource
{
private:
   friend class GFXDevice;

   GFXResource *mPrevResource;
   GFXResource *mNextResource;
   GFXDevice   *mOwningDevice;

   /// Helper flag to check new resource allocations
   bool mFlagged;

public:
   GFXResource();
   ~GFXResource();

   /// Registers this resource with the given device
   void registerResourceWithDevice(GFXDevice *device);

   /// When called the resource should destroy all device sensitive information (e.g. D3D resources in D3DPOOL_DEFAULT
   virtual void zombify()=0;

   /// When called the resource should restore all device sensitive information destroyed by zombify()
   virtual void resurrect()=0;

   /// The resource should put a description of itself (number of vertices, size/width of texture, etc.) in buffer
   virtual void describeSelf(char* buffer, U32 sizeOfBuffer) = 0;

   inline GFXResource *getNextResource() const
   {
      return mNextResource;
   }

   inline GFXResource *getPrevResource() const
   {
      return mPrevResource;
   }

   inline GFXDevice *getOwningDevice() const
   {
      return mOwningDevice;
   }

   inline bool isFlagged()
   {
      return mFlagged;
   }

   inline void setFlag()
   {
      mFlagged = true;
   }

   inline void clearFlag()
   {
      mFlagged = false;
   }
};

#endif