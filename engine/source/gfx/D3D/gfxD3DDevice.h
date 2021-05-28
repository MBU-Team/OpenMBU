//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------
#ifndef _GFXD3DDevice_H_
#define _GFXD3DDevice_H_

#include "platform/platform.h"

//-----------------------------------------------------------------------------
#ifdef TORQUE_DEBUG
#include "dxerr.h"
#endif

inline void D3DAssert(HRESULT hr, const char* info)
{
#if defined( TORQUE_DEBUG )
    if (FAILED(hr))
    {
        char buf[256];
        dSprintf(buf, 256, "%s\n%s\n%s", DXGetErrorString(hr), DXGetErrorDescription(hr), info);
        AssertFatal(false, buf);
        //      DXTrace( __FILE__, __LINE__, hr, info, true );
    }
#endif

}

// Adjust these pools to your app's needs.  Be aware dynamic vertices are much more
// expensive than static vertices.
#define MAX_DYNAMIC_VERTS   (8192*2)
#define MAX_DYNAMIC_INDICES (8192*4)


#include "gfx/gfxDevice.h"
#include "platformWin32/platformWin32.h"
#include "gfx/D3D/gfxD3DTextureManager.h"
#include "gfx/D3D/gfxD3DShaderMgr.h"
#include "gfx/D3D/gfxD3DCubemap.h"
#include "gfx/D3D/gfxD3DTypes.h"
#include "gfx/gfxInit.h"
#include "util/safeRelease.h"

class GFXD3DDevice : public GFXDevice
{
    friend class GFXD3DPrimitiveBuffer;
    friend class GFXD3DVertexBuffer;
    typedef GFXDevice Parent;

private:
    MatrixF mTempMatrix;    ///< Temporary matrix, no assurances on value at all
    D3DVIEWPORT9 mViewport; ///< Because setViewport gets called a lot, don't want to allocate/unallocate a lot
    RectI mViewportRect;
    RectI mClipRect;

    typedef RefPtr<GFXD3DVertexBuffer> RPGDVB;
    Vector<RPGDVB> mVolatileVBList;
    Vector<GFXD3DVertexBuffer*> mDynamicVBList;

    GFXD3DVertexBuffer* mCurrentOpenAllocVB;
    GFXD3DVertexBuffer* mCurrentVB;
    void* mCurrentOpenAllocVertexData;
    //-----------------------------------------------------------------------
    RefPtr<GFXD3DPrimitiveBuffer> mDynamicPB;                       ///< Dynamic index buffer
    GFXD3DPrimitiveBuffer* mCurrentOpenAllocPB;
    GFXD3DPrimitiveBuffer* mCurrentPB;

    LPDIRECT3D9       mD3D;        ///< D3D Handle
    LPDIRECT3DDEVICE9 mD3DDevice;  ///< Handle for D3DDevice

    U32  mAdapterIndex;            ///< Adapter index because D3D supports multiple adapters

    GFXD3DShaderMgr mShaderMgr;    ///< D3D Shader Manager
    F32 mPixVersion;

    Vector<IDirect3DSurface9*> activeZStack;

    /// Special effects back buffer - for refraction and other effects
    virtual void copyBBToSfxBuff();

    /// To manage creating and re-creating of these when device is aquired
    void reacquireDefaultPoolResources();

    /// To release all resources we control from D3DPOOL_DEFAULT
    void releaseDefaultPoolResources();

    /// This you will probably never, ever use, but it is used to generate the code for
    /// the initStates() function
    void regenStates();

    GFXD3DVertexBuffer* findVBPool(U32 vertFlags);
    GFXD3DVertexBuffer* createVBPool(U32 vertFlags, U32 vertSize);

#ifdef TORQUE_DEBUG
    /// @name Debug Vertex Buffer information/management
    /// @{
    U32 mNumAllocatedVertexBuffers; ///< To keep track of how many are allocated and freed
    GFXD3DVertexBuffer* mVBListHead;
    void addVertexBuffer(GFXD3DVertexBuffer* buffer);
    void removeVertexBuffer(GFXD3DVertexBuffer* buffer);
    void logVertexBuffers();
    /// @}
#endif

protected:

    // State overrides
    // {
    void setRenderState(U32 state, U32 value);
    void setTextureStageState(U32 stage, U32 state, U32 value);
    void setSamplerState(U32 stage, U32 type, U32 value);

    void setTextureInternal(U32 textureUnit, const GFXTextureObject* texture);

    void setMatrix(GFXMatrixType mtype, const MatrixF& mat);

    void initStates();
    // }

    // Buffer allocation
    // {  
    GFXVertexBuffer* allocVB(U32 numVerts, void** vertexPtr, U32 vertFlags, U32 vertSize);
    GFXVertexBuffer* allocPooledVB(U32 numVerts, void** vertexPtr, U32 vertFlags, U32 vertSize);

    GFXPrimitiveBuffer* allocPB(U32 numIndices, void** indexPtr);
    GFXPrimitiveBuffer* allocPooledPB(U32 numIndices, void** indexPtr);
    // } 

    // Index buffer management
    // {
    void setPrimitiveBuffer(GFXPrimitiveBuffer* buffer);
    void drawIndexedPrimitive(GFXPrimitiveType primType, U32 minIndex, U32 numVerts, U32 startIndex, U32 primitiveCount);
    // }

    virtual GFXShader* createShader(char* vertFile,
        char* pixFile,
        F32 pixVersion);

    virtual GFXShader* createShader(GFXShaderFeatureData& featureData, GFXVertexFlags vertFlags);
    virtual void destroyShader(GFXShader* shader);

public:

    GFXTextureObject* createRenderSurface(U32 width, U32 height, GFXFormat format, U32 mipLevel);

    /// Constructor
    /// @param   d3d   Direct3D object to instantiate this device with
    /// @param   index   Adapter index since D3D can use multiple graphics adapters
    GFXD3DDevice(LPDIRECT3D9 d3d, U32 index);
    ~GFXD3DDevice();

    /// Get a string indicating the installed DirectX version, revision and letter number
    static char* getDXVersion();

    // Activate/deactivate
    // {
    virtual void init(const GFXVideoMode& mode);

    virtual void activate();
    virtual void deactivate() {}                            // <- TODO!!
    virtual void preDestroy() { mTextureManager->kill(); }

    GFXAdapterType getAdapterType() { return Direct3D9; }

    virtual GFXCubemap* createCubemap() { return new GFXD3DCubemap(); }

    virtual F32  getPixelShaderVersion() { return mPixVersion; }
    virtual void setPixelShaderVersion(F32 version) { mPixVersion = version; }
    virtual void disableShaders();
    virtual void setShader(GFXShader* shader);
    virtual void setVertexShaderConstF(U32 reg, const float* data, U32 size);
    virtual void setPixelShaderConstF(U32 reg, const float* data, U32 size);
    virtual U32  getNumSamplers();

    virtual void enterDebugEvent(ColorI color, const char* name);
    virtual void leaveDebugEvent();
    virtual void setDebugMarker(ColorI color, const char* name);

    void enumerateVideoModes();
    static void enumerateAdapters(Vector<GFXAdapter>& adapterList);
    void setVideoMode(const GFXVideoMode& mode);

    virtual GFXFormat selectSupportedFormat(GFXTextureProfile* profile,
        const Vector<GFXFormat>& formats, bool texture, bool mustblend);
    // }

    // Misc rendering control
    // {
    void clear(U32 flags, ColorI color, F32 z, U32 stencil);
    void beginScene();
    void endScene();
    void swapBuffers();

    void setViewport(const RectI& rect);
    const RectI& getViewport() const;

    void setClipRect(const RectI& rect);
    const RectI& getClipRect() const;
    // }

    // Render Targets
    // {
    void setRenderTarget(GFXTextureObject* surface, U32 renderTargetIndex, U32 mipLevel);
    void setRTBackBuffer();
    void pushActiveRenderSurfaces();
    void pushActiveZSurface();
    void popActiveRenderSurfaces();
    void popActiveZSurface();
    void setActiveRenderSurface(GFXTextureObject* surface, U32 renderTargetIndex = 0, U32 mipLevel = 0);
    void setActiveZSurface(GFXTextureObject* surface);

    // }

    // Vertex/Index buffer management
    // {
    void setVB(GFXVertexBuffer* buffer);

    GFXVertexBuffer* allocVertexBuffer(U32 numVerts, U32 vertFlags, U32 vertSize, GFXBufferType bufferType);
    GFXPrimitiveBuffer* allocPrimitiveBuffer(U32 numIndices, U32 numPrimitives, GFXBufferType bufferType);
    void  deallocVertexBuffer(GFXD3DVertexBuffer* vertBuff);
    // }

    GFXTextureObject* allocTexture(GBitmap* bitmap, GFXTextureType type = GFXTextureType_Normal, bool extrudeMips = false);
    GFXTextureObject* allocTexture(StringTableEntry fileName, GFXTextureType type = GFXTextureType_Normal, bool extrudeMips = false, bool preserveSize = false);

    void zombifyTextureManager();
    void resurrectTextureManager();

    U32 getMaxDynamicVerts() { return MAX_DYNAMIC_VERTS; }
    U32 getMaxDynamicIndices() { return MAX_DYNAMIC_INDICES; }

    // Rendering
    // {
    void drawPrimitive(GFXPrimitiveType primType, U32 vertexStart, U32 primitiveCount);
    // }

    virtual void project(Point3F& outPoint,
        Point3F& inPoint,
        MatrixF& modelview,
        MatrixF& projection,
        RectI& viewport);

    virtual void unProject(Point3F& outPoint,
        Point3F& inPoint,
        MatrixF& modelview,
        MatrixF& projection,
        RectI& viewport);

    LPDIRECT3DDEVICE9 getDevice() { return mD3DDevice; }

    /// Device helper function
    D3DPRESENT_PARAMETERS setupPresentParams(const GFXVideoMode& mode);

    /// Reset
    void reset(D3DPRESENT_PARAMETERS& d3dpp);

    GFXShader* mGenericShader[GS_COUNT];
    virtual void setupGenericShaders(GenericShaderType type = GSColor);

    inline F32 getFillConventionOffset() const { return 0.5f; }

    virtual void doParanoidStateCheck();
};


inline const RectI& GFXD3DDevice::getClipRect() const
{
    return mClipRect;
}

inline const RectI& GFXD3DDevice::getViewport() const
{
    return mViewportRect;
}

#endif
