//-----------------------------------------------------------------------------
// Torque Game Engine Advanced
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------
#ifndef _GFXD3D9Device_H_
#define _GFXD3D9Device_H_

#ifdef TORQUE_OS_XENON
#  include "platformXbox/platformXbox.h"
#else
#  include <d3dx9.h>
#  include "platformWin32/platformWin32.h"
#endif

#include "gfx/gfxDevice.h"
#include "gfx/D3D9/gfxD3D9TextureManager.h"
#include "gfx/D3D9/gfxD3D9ShaderMgr.h"
#include "gfx/D3D9/gfxD3D9EffectMgr.h"
#include "gfx/D3D9/gfxD3D9Cubemap.h"
#include "gfx/D3D9/gfxD3D9PrimitiveBuffer.h"
#include "gfx/gfxInit.h"
#include "platform/platformDlibrary.h"
#include "util/safeRelease.h"

//-----------------------------------------------------------------------------
#ifdef TORQUE_DEBUG
   #include "dxerr.h"
#endif

inline void D3D9Assert( HRESULT hr, const char *info ) 
{
#if defined( TORQUE_DEBUG )
   if( FAILED( hr ) ) 
   {
      char buf[256];
      dSprintf( buf, 256, "%s\n%s\n%s", DXGetErrorStringA( hr ), DXGetErrorDescriptionA( hr ), info );
      AssertFatal( false, buf ); 
//      DXTrace( __FILE__, __LINE__, hr, info, true );
   }
#endif
}


// Typedefs
#define D3DX_FUNCTION(fn_name, fn_return, fn_args) \
   typedef fn_return (WINAPI *D3DXFNPTR##fn_name)fn_args;
#include "gfx/D3D9/d3dx9Functions.h"
#undef D3DX_FUNCTION

// Function table
struct D3DXFNTable
{
   D3DXFNTable() : isLoaded( false ){};
   bool isLoaded;
   DLibraryRef dllRef;
#define D3DX_FUNCTION(fn_name, fn_return, fn_args) \
   D3DXFNPTR##fn_name fn_name;
#include "gfx/D3D9/d3dx9Functions.h"
#undef D3DX_FUNCTION
};

#define D3D9_FUNCTION(fn_name, fn_return, fn_args) \
   typedef fn_return (WINAPI *D3D9FNPTR##fn_name)fn_args;
#include "gfx/D3D9/d3d9Functions.h"
#undef D3D9_FUNCTION

struct D3D9FNTable
{
   D3D9FNTable() : isLoaded( false ) {};
   bool isLoaded;
   DLibraryRef dllRef;
#define D3D9_FUNCTION(fn_name, fn_return, fn_args) \
   D3D9FNPTR##fn_name fn_name;
#include "gfx/D3D9/d3d9Functions.h"
#undef D3D9_FUNCTION
};

#define GFXD3DX static_cast<GFXD3D9Device *>(GFX)->smD3DX
#define GFXD3D9 static_cast<GFXD3D9Device *>(GFX)->smD3D9 

class GFXResource;

//------------------------------------------------------------------------------

class GFXD3D9Device : public GFXDevice
{
   friend class GFXResource;
   friend class GFXD3D9PrimitiveBuffer;
   friend class GFXD3D9VertexBuffer;
   friend class GFXD3D9TextureObject;
   friend class GFXPCD3D9TextureTarget;
   
   typedef GFXDevice Parent;

   protected:
      MatrixF mTempMatrix;    ///< Temporary matrix, no assurances on value at all
      D3DVIEWPORT9 mViewport; ///< Because setViewport gets called a lot, don't want to allocate/unallocate a lot
      RectI mViewportRect;
      RectI mClipRect;
      
      typedef RefPtr<GFXD3D9VertexBuffer> RPGDVB;
      Vector<RPGDVB> mVolatileVBList;

      IDirect3DSurface9 *mDeviceBackbuffer;
      IDirect3DSurface9 *mDeviceDepthStencil;
      IDirect3DSurface9 *mDeviceColor;

      GFXD3D9VertexBuffer *mCurrentOpenAllocVB;
      GFXD3D9VertexBuffer *mCurrentVB;
      void *mCurrentOpenAllocVertexData;

      static void initD3DXFnTable();
      static void initD3D9FnTable();
      //-----------------------------------------------------------------------
      RefPtr<GFXD3D9PrimitiveBuffer> mDynamicPB;                       ///< Dynamic index buffer
      GFXD3D9PrimitiveBuffer *mCurrentOpenAllocPB;
      GFXD3D9PrimitiveBuffer *mCurrentPB;

      IDirect3DVertexShader9 *mLastVertShader;
      IDirect3DPixelShader9 *mLastPixShader;

      S32 mCreateFenceType;

      LPDIRECT3D9       mD3D;        ///< D3D Handle
      LPDIRECT3DDEVICE9 mD3DDevice;  ///< Handle for D3DDevice

      U32  mAdapterIndex;            ///< Adapter index because D3D supports multiple adapters

      GFXD3D9ShaderMgr mShaderMgr;    ///< D3D Shader Manager
      GFXD3D9EffectMgr mEffectMgr;    ///< D3D Effect Manager
      F32 mPixVersion;
      U32 mNumSamplers;               ///< Profiled (via caps)

      D3DMULTISAMPLE_TYPE mMultisampleType;
      DWORD mMultisampleLevel;

      /// To manage creating and re-creating of these when device is aquired
      void reacquireDefaultPoolResources();

      /// To release all resources we control from D3DPOOL_DEFAULT
      void releaseDefaultPoolResources();

      /// This you will probably never, ever use, but it is used to generate the code for
      /// the initStates() function
      void regenStates();

      virtual GFXD3D9VertexBuffer *findVBPool( U32 vertFlags, U32 numVertsNeeded );
      virtual GFXD3D9VertexBuffer *createVBPool( U32 vertFlags, U32 vertSize );

#ifdef TORQUE_DEBUG
      /// @name Debug Vertex Buffer information/management
      /// @{

      ///
      U32 mNumAllocatedVertexBuffers; ///< To keep track of how many are allocated and freed
      GFXD3D9VertexBuffer *mVBListHead;
      void addVertexBuffer( GFXD3D9VertexBuffer *buffer );
      void removeVertexBuffer( GFXD3D9VertexBuffer *buffer );
      void logVertexBuffers();
      /// @}
#endif

      // State overrides
      // {

      ///
      virtual void setRenderState( U32 state, U32 value) override;
      virtual void setSamplerState( U32 stage, U32 type, U32 value ) override;
      virtual void setTextureInternal(U32 textureUnit, const GFXTextureObject* texture) override;

      // CodeReview - How exactly do we want to deal with this on the Xenon?
      // Right now it's just in an #ifndef in gfxD3D9Device.cpp - AlexS 4/11/07
      virtual void setLightInternal(U32 lightStage, const LightInfo light, bool lightEnable) override;
      virtual void setLightMaterialInternal(const GFXLightMaterial mat) override;
      virtual void setGlobalAmbientInternal(ColorF color) override;

      virtual void initStates() override =0;
      // }

      // Index buffer management
      // {
      virtual void _setPrimitiveBuffer( GFXPrimitiveBuffer *buffer );
      virtual void drawIndexedPrimitive( GFXPrimitiveType primType, U32 minIndex, U32 numVerts, U32 startIndex, U32 primitiveCount ) override;
      // }

      virtual GFXShader * createShader( const char *vertFile, const char *pixFile, F32 pixVersion) override;
      virtual GFXShader * createShader( GFXShaderFeatureData &featureData, GFXVertexFlags vertFlags ) override;
      virtual void destroyShader( GFXShader *shader ) override;

      // This is called by MatInstance::reinitInstances to cause the shaders to be regenerated.
      virtual void flushProceduralShaders() override;

      /// Device helper function
      virtual D3DPRESENT_PARAMETERS setupPresentParams( const GFXVideoMode &mode, const HWND &hwnd ) = 0;

   public:
      static D3DXFNTable smD3DX;
      static D3D9FNTable smD3D9;

      static GFXDevice *createInstance( U32 adapterIndex );

      GFXTextureObject* createRenderSurface( U32 width, U32 height, GFXFormat format, U32 mipLevel );
      
      /// Constructor
      /// @param   d3d   Direct3D object to instantiate this device with
      /// @param   index   Adapter index since D3D can use multiple graphics adapters
      GFXD3D9Device( LPDIRECT3D9 d3d, U32 index );
      virtual ~GFXD3D9Device();

      /// Get a string indicating the installed DirectX version, revision and letter number
      static char *getDXVersion();

      // Activate/deactivate
      // {
      virtual void init( const GFXVideoMode &mode/*, PlatformWindow *window = NULL*/ ) override = 0;
      virtual void activate() override { };
      virtual void deactivate() override { };

      virtual void preDestroy() override { if(mTextureManager) mTextureManager->kill(); }

      GFXAdapterType getAdapterType() override{ return Direct3D9; }
      
      virtual GFXCubemap *createCubemap() override;
      
      virtual F32  getPixelShaderVersion() const override { return mPixVersion; }
      virtual void setPixelShaderVersion( F32 version ) override{ mPixVersion = version; }
      virtual void disableShaders() override;
      virtual void setShader( GFXShader *shader ) override;
      virtual void setVertexShaderConstF( U32 reg, const float *data, U32 size ) override;
      virtual void setPixelShaderConstF( U32 reg, const float *data, U32 size ) override;
      virtual U32  getNumSamplers() const override { return mNumSamplers; }

      virtual GFXEffectMgr* getEffectManager() { return &mEffectMgr; }
      
      static void enumerateAdapters( Vector<GFXAdapter*> &adapterList );

      // }

      // Misc rendering control
      // {
      virtual void clear( U32 flags, ColorI color, F32 z, U32 stencil ) override;

      virtual void setViewport( const RectI &rect ) override;
      virtual const RectI &getViewport() const override;

      virtual void setClipRect( const RectI &rect ) override;
      virtual void setClipRectOrtho( const RectI &rect, const RectI &orthoRect ) override;
      virtual const RectI &getClipRect() const override;
      // }

      /// @name Render Targets
      /// @{
      virtual GFXTextureTarget *allocRenderToTextureTarget(Point2I size, GFXFormat format);
      /// @}

      // Vertex/Index buffer management
      // {
      void setVB( GFXVertexBuffer *buffer );

      virtual GFXVertexBuffer    *allocVertexBuffer   ( U32 numVerts, U32 vertFlags, U32 vertSize, GFXBufferType bufferType ) override;
      virtual GFXPrimitiveBuffer *allocPrimitiveBuffer( U32 numIndices, U32 numPrimitives, GFXBufferType bufferType ) override;
      virtual void deallocVertexBuffer( GFXD3D9VertexBuffer *vertBuff );
      // }

      // Not implemented, what is the story? -patw
      //virtual GFXTextureObject* allocTexture( GBitmap *bitmap, GFXTextureType type = GFXTextureType_Normal, bool extrudeMips = false );
      //virtual GFXTextureObject* allocTexture( StringTableEntry fileName, GFXTextureType type = GFXTextureType_Normal, bool extrudeMips = false, bool preserveSize = false );

      virtual void zombifyTextureManager() override;
      virtual void resurrectTextureManager() override;

      virtual U32 getMaxDynamicVerts() override { return MAX_DYNAMIC_VERTS; }
      virtual U32 getMaxDynamicIndices() override { return MAX_DYNAMIC_INDICES; }

      // Rendering
      // {
      virtual void drawPrimitive( GFXPrimitiveType primType, U32 vertexStart, U32 primitiveCount ) override;
      // }

      virtual LPDIRECT3DDEVICE9 getDevice(){ return mD3DDevice; }

      /// Reset
      virtual void reset( D3DPRESENT_PARAMETERS &d3dpp ) = 0;

      GFXShader *mGenericShader[GS_COUNT];
      virtual void setupGenericShaders( GenericShaderType type  = GSColor ) override;

      // Function only really used on the, however a centralized function for
      // destroying resources is probably a good thing -patw
      virtual void destroyD3DResource( IDirect3DResource9 *d3dResource ) { SAFE_RELEASE( d3dResource ); };

      inline virtual F32 getFillConventionOffset() const override { return 0.5f; }
      virtual void doParanoidStateCheck() override;

   GFXFence *createFence() override;

   // Default multisample parameters
   D3DMULTISAMPLE_TYPE getMultisampleType() { return mMultisampleType; }
   U32 getMultisampleLevel() { return mMultisampleLevel; }
};

//------------------------------------------------------------------------------

inline const RectI &GFXD3D9Device::getClipRect() const
{
   return mClipRect;
}

inline const RectI &GFXD3D9Device::getViewport() const
{
   return mViewportRect;
}

#endif
