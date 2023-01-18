//-----------------------------------------------------------------------------
// Torque Game Engine Advanced
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _GFX_PC_D3D9DEVICE_H_
#define _GFX_PC_D3D9DEVICE_H_

#include "gfx/D3D9/gfxD3D9Device.h"

class GFXPCD3D9Device : public GFXD3D9Device
{
public:
   GFXPCD3D9Device( LPDIRECT3D9 d3d, U32 index ) : GFXD3D9Device( d3d, index ) {};
   ~GFXPCD3D9Device();

   static GFXDevice *createInstance( U32 adapterIndex );

   virtual GFXFormat selectSupportedFormat(GFXTextureProfile *profile,
	   const Vector<GFXFormat> &formats, bool texture, bool mustblend) override;
   
   virtual void enumerateVideoModes() override;

   /// Sets the video mode for the device
   virtual void setVideoMode( const GFXVideoMode &mode ) override;


   virtual void beginSceneInternal() override;
   virtual void endSceneInternal() override;

   virtual void setActiveRenderTarget( GFXTarget *target ) override;
   virtual GFXWindowTarget *allocWindowTarget(/*PlatformWindow *window*/) override;
   virtual GFXTextureTarget *allocRenderToTextureTarget() override;

   virtual void init( const GFXVideoMode &mode/*, PlatformWindow *window = NULL*/ ) override;

   virtual void enterDebugEvent(ColorI color, const char *name) override;
   virtual void leaveDebugEvent() override;
   virtual void setDebugMarker(ColorI color, const char *name) override;

   virtual void copyBBToSfxBuff() override;

   virtual void setMatrix( GFXMatrixType mtype, const MatrixF &mat ) override;

   virtual void initStates() override;
   virtual void reset( D3DPRESENT_PARAMETERS &d3dpp ) override;
protected:
   virtual D3DPRESENT_PARAMETERS setupPresentParams( const GFXVideoMode &mode, const HWND &hwnd ) override;
   virtual void setTextureStageState( U32 stage, U32 state, U32 value ) override;
   
   void validateMultisampleParams(D3DFORMAT format);
};

#endif