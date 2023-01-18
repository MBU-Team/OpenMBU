//-----------------------------------------------------------------------------
// Torque Game Engine Advanced
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _GFX_GFXPCD3D9TARGET_H_
#define _GFX_GFXPCD3D9TARGET_H_

#include "gfx/gfxTarget.h"
#include "d3d9types.h"
//#include "windowManager/win32/win32Window.h"

struct IDirect3DSurface9;
struct IDirect3DSwapChain9;

class GFXD3D9TextureObject;

class GFXPCD3D9TextureTarget : public GFXTextureTarget
{
   friend class GFXD3D9Device;
   friend class GFXPCD3D9Device;

   // Array of target surfaces, this is given to us by attachTexture
   IDirect3DSurface9 * mTargets[MaxRenderSlotId];

   // Array of texture objects which correspond to the target surfaces above,
   // needed for copy from RenderTarget to texture situations.  Current only valid in those situations
   GFXD3D9TextureObject* mTextures[MaxRenderSlotId];

   /// Owning d3d device.
   GFXD3D9Device  *mDevice;

public:

   GFXPCD3D9TextureTarget();
   ~GFXPCD3D9TextureTarget();

   // Public interface.
   virtual const Point2I getSize();
   virtual void attachTexture(RenderSlot slot, GFXTextureObject *tex, U32 mipLevel=0, U32 zOffset = 0);
   virtual void attachTexture(RenderSlot slot, GFXCubemap *tex, U32 face, U32 mipLevel=0);
   virtual void clearAttachments();
   virtual void deactivate();

   void zombify();
   void resurrect();
};

class GFXPCD3D9WindowTarget : public GFXWindowTarget
{
   friend class GFXD3D9WindowTarget;
   friend class GFXPCD3D9Device;
   //friend class Win32Window;

   /// Our depth stencil buffer, if any.
   IDirect3DSurface9 *mDepthStencil;

   /// Our backbuffer
   IDirect3DSurface9 *mBackbuffer;

   /// Maximum size we can render to.
   Point2I mSize;

   /// Our swap chain, potentially the implicit device swap chain.
   IDirect3DSwapChain9 *mSwapChain;

   /// Reference to our window.
   //Win32Window *mWindow;

   /// D3D presentation info.
   D3DPRESENT_PARAMETERS mPresentationParams;

   /// Owning d3d device.
   GFXD3D9Device  *mDevice;

   /// Is this the implicit swap chain?
   bool mImplicit;

   /// Internal interface that notifies us we need to reset our video mode.
   void resetMode();

public:

   GFXPCD3D9WindowTarget();
   ~GFXPCD3D9WindowTarget();
 
   //virtual PlatformWindow *getWindow();
   virtual const Point2I getSize();
   virtual bool present();

   void initPresentationParams();

   void zombify();
   void resurrect();
};

#endif
