//-----------------------------------------------------------------------------
// Torque Game Engine Advanced
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _GFX_GFXTARGET_H_
#define _GFX_GFXTARGET_H_

#include "gfx/gfxResource.h"
#include "core/refBase.h"
#include "math/mPoint.h"

//class PlatformWindow;
class GFXCubemap;
class GFXTextureObject;

/// Base class for a target to which GFX can render.
///
/// Most modern graphics hardware supports selecting render targets. However,
/// there may be multiple types of render target, with wildly varying
/// device-level implementations, resource requirements, and so forth.
///
/// This base class is used to represent a render target; it might be a context
/// tied to a window, or a set of surfaces or textures.
class GFXTarget : public RefBase, public GFXResource
{
   friend class GFXPCD3D9Device;
   friend class GFX360Device;

private:
   S32 mChangeToken;
   S32 mLastAppliedChange;

protected:

   /// Called whenever a change is made to this target.
   inline void invalidateState()
   {
      mChangeToken++;
   }

   /// Called when the device has applied pending state.
   inline void stateApplied()
   {
      mLastAppliedChange = mChangeToken;
   }
public:

   /// Constructor to initialize the state tracking logic.
   GFXTarget() : mChangeToken( 0 ),
                 mLastAppliedChange( 0 )
   {
   }

   /// Called to check if we have pending state for the device to apply.
   inline const bool isPendingState() const
   {
      return (mChangeToken != mLastAppliedChange);
   }

   /// Returns the size in pixels of this rendering target.
   virtual const Point2I getSize()=0;

   // GFXResourceInterface
   virtual void describeSelf(char* buffer, U32 sizeOfBuffer);

   /// This is called when the target is not being used anymore.
   virtual void activate() { }

   /// This is called when the target is not being used anymore.
   virtual void deactivate() { }
};

/// A render target associated with an OS window.
///
/// Various API/OS combinations will implement their own GFXTargets for
/// rendering to a window. However, they are all subclasses of GFXWindowTarget.
///
/// This allows platform-neutral code to safely distinguish between various
/// types of render targets (using dynamic_cast<>), as well as letting it
/// gain access to useful things like the corresponding PlatformWindow.
class GFXWindowTarget : public GFXTarget
{
public:
   /// Returns a pointer to the window this target is bound to.
   //virtual PlatformWindow *getWindow()=0;

   /// Present latest buffer, if buffer swapping is in effect.
   virtual bool present()=0;

   /// Notify the target that the video mode on the window has changed.
   virtual void resetMode()=0;
};

/// A render target associated with one or more textures.
///
/// Although some APIs allow directly selecting any texture or surfaces, in
/// some cases it is necessary to allocate helper resources to enable RTT
/// operations.
///
/// @note A GFXTextureTarget will retain references to textures that are 
/// attached to it, so be sure to clear them out when you're done!
///
/// @note Different APIs have different restrictions on what they can support
///       here. Be aware when mixing cubemaps vs. non-cubemaps, or targets of
///       different resolutions. The devices will attempt to limit behavior
///       to things that are safely portable, but they cannot catch every
///       possible situation for all drivers and API - so make sure to
///       actually test things!
class GFXTextureTarget : public GFXTarget
{
public:
   enum RenderSlot
   {
      DepthStencil,
      Color0, Color1, Color2, Color3, Color4,
      MaxRenderSlotId,
   };

   static GFXTextureObject *sDefaultDepthStencil;

   /// Attach a surface to a given slot as part of this render target.
   ///
   /// @param slot What slot is used for multiple render target (MRT) effects.
   ///             Most of the time you'll use Color0.
   /// @param tex A texture and miplevel to bind for rendering, or else NULL/0
   ///            to clear a slot.
   /// @param mipLevel What level of this texture are we rendering to?
   /// @param zOffset  If this is a depth texture, what z level are we 
   ///                 rendering to?
   virtual void attachTexture(RenderSlot slot, GFXTextureObject *tex, U32 mipLevel=0, U32 zOffset = 0) = 0;

   /// Support binding to cubemaps.
   ///
   /// @param slot What slot is used for multiple render target (MRT) effects.
   ///             Most of the time you'll use Color0.
   /// @param tex  What cubemap will we be rendering to?
   /// @param face A face identifier.
   /// @param mipLevel What level of this texture are we rendering to?
   virtual void attachTexture(RenderSlot slot, GFXCubemap *tex, U32 face, U32 mipLevel=0) = 0;

   /// Unattach all bound textures.
   ///
   /// This is a helper function to make it easy to make sure a GFXTextureTarget
   /// is entirely empty before you move on to other rendering tasks. Because
   /// a GFXTextureTarget will keep references to bound GFXTextureObjects,
   /// not clearing can results in resource leaks and hard to track down bugs.
   virtual void clearAttachments()=0;
};

typedef RefPtr<GFXTarget> GFXTargetRef;
typedef RefPtr<GFXWindowTarget> GFXWindowTargetRef;
typedef RefPtr<GFXTextureTarget> GFXTextureTargetRef;

#endif