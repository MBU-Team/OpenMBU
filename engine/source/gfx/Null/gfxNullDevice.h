//-----------------------------------------------------------------------------
// Torque Game Engine Advanced
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _GFXNullDevice_H_
#define _GFXNullDevice_H_

#include "platform/platform.h"

//-----------------------------------------------------------------------------

#include "gfx/gfxDevice.h"
#include "gfx/gfxInit.h"
#include "gfx/gfxFence.h"

class GFXNullWindowTarget : public GFXWindowTarget
{
   //PlatformWindow *mWindow;

public:

   GFXNullWindowTarget()//PlatformWindow *win)
   {
      //mWindow = win;
   }

//   virtual PlatformWindow *getWindow()
//   {
//      return mWindow;
//   }

   virtual bool present()
   {
      return true;
   }

   virtual const Point2I getSize()
   {
      // Return something stupid.
      return Point2I(1,1);
   }

   virtual void resetMode()
   {

   }

   virtual void zombify() {};
   virtual void resurrect() {};

};

class GFXNullDevice : public GFXDevice
{
   typedef GFXDevice Parent;

private:
   RectI viewport;
   RectI clip;
public:
   GFXNullDevice();
   virtual ~GFXNullDevice();

   static GFXDevice *createInstance( U32 adapterIndex );

   static void enumerateAdapters( Vector<GFXAdapter*> &adapterList );

   void init( const GFXVideoMode &mode/*, PlatformWindow *window = NULL*/ ) override {};

   virtual void activate() override { };
   virtual void deactivate() override { };
   virtual GFXAdapterType getAdapterType() override { return NullDevice; };

   /// @name Debug Methods
   /// @{
   virtual void enterDebugEvent(ColorI color, const char *name) override { };
   virtual void leaveDebugEvent() override { };
   virtual void setDebugMarker(ColorI color, const char *name) override { };
   /// @}

   /// Enumerates the supported video modes of the device
   virtual void enumerateVideoModes() override { };

   /// Sets the video mode for the device
   virtual void setVideoMode( const GFXVideoMode &mode ) override { };
protected:
   /// Sets states which have to do with general rendering
   virtual void setRenderState( U32 state, U32 value) override { };

   /// Sets states which have to do with how textures are displayed
   virtual void setTextureStageState( U32 stage, U32 state, U32 value ) override { };

   /// Sets states which have to do with texture sampling and addressing
   virtual void setSamplerState( U32 stage, U32 type, U32 value ) override { };
   /// @}

   virtual void setTextureInternal(U32 textureUnit, const GFXTextureObject*texture) override { };

   virtual void setLightInternal(U32 lightStage, const LightInfo light, bool lightEnable) override;
   virtual void setLightMaterialInternal(const GFXLightMaterial mat) override { };
   virtual void setGlobalAmbientInternal(ColorF color) override { };

   /// @name State Initalization.
   /// @{

   /// State initalization. This MUST BE CALLED in setVideoMode after the device
   /// is created.
   virtual void initStates() override { };

   virtual void setMatrix( GFXMatrixType mtype, const MatrixF &mat ) override { };

   virtual GFXVertexBuffer *allocVertexBuffer( U32 numVerts, U32 vertFlags, U32 vertSize, GFXBufferType bufferType ) override;
   virtual GFXPrimitiveBuffer *allocPrimitiveBuffer( U32 numIndices, U32 numPrimitives, GFXBufferType bufferType ) override;
public:
   virtual void copyBBToSfxBuff() override { };

   virtual void zombifyTextureManager() override { };
   virtual void resurrectTextureManager() override { };

   virtual GFXCubemap * createCubemap() override;

   virtual F32 getFillConventionOffset() const override { return 0.0f; };

   ///@}

   virtual GFXTextureTarget *allocRenderToTextureTarget() override {return NULL;};
   virtual GFXTextureTarget *allocRenderToTextureTarget(Point2I size, GFXFormat format) {return NULL;};
   virtual GFXWindowTarget *allocWindowTarget(/*PlatformWindow *window*/) override
   {
      GFXNullWindowTarget* target = new GFXNullWindowTarget();//window);

      getDeviceEventSignal().trigger(deInit);

      return target;
   };

   virtual void pushActiveRenderTarget() override {};
   virtual void popActiveRenderTarget() override {};
   virtual void setActiveRenderTarget( GFXTarget *target ) override {};
   virtual GFXTarget *getActiveRenderTarget() override {return NULL;};

   virtual F32 getPixelShaderVersion() const override { return 0.0f; };
   virtual void setPixelShaderVersion( F32 version ) override { };
   virtual U32 getNumSamplers() const override { return TEXTURE_STAGE_COUNT; };

   virtual GFXShader * createShader( const char *vertFile, const char *pixFile, F32 pixVersion ) override { return NULL; };
   virtual GFXShader * createShader( GFXShaderFeatureData &featureData, GFXVertexFlags vertFlags ) override { return NULL; };

   // This is called by MatInstance::reinitInstances to cause the shaders to be regenerated.
   virtual void flushProceduralShaders() override { };


   virtual void clear( U32 flags, ColorI color, F32 z, U32 stencil ) override { };
   virtual void beginSceneInternal() override { };
   virtual void endSceneInternal() override { };

   virtual void drawPrimitive( GFXPrimitiveType primType, U32 vertexStart, U32 primitiveCount ) override { };
   virtual void drawIndexedPrimitive( GFXPrimitiveType primType, U32 minIndex, U32 numVerts, U32 startIndex, U32 primitiveCount ) override { };

   virtual void setViewport( const RectI &rect ) override { };
   virtual const RectI &getViewport() const override { return viewport; };

   virtual void setClipRect( const RectI &rect ) override { };
   virtual void setClipRectOrtho( const RectI &rect, const RectI &orthoRect ) override { };
   virtual const RectI &getClipRect() const override { return clip; };

   virtual void preDestroy() override { };

   virtual U32 getMaxDynamicVerts() override { return 16384; };
   virtual U32 getMaxDynamicIndices() override { return 16384; };

   virtual GFXFormat selectSupportedFormat( GFXTextureProfile *profile, const Vector<GFXFormat> &formats, bool texture, bool mustblend ) override { return GFXFormatR8G8B8A8; };
   GFXFence *createFence() override { return new GFXGeneralFence( this ); }
};

#endif
