//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _GFXDEVICE_H_
#define _GFXDEVICE_H_

#include "math/mPoint.h"
#include "core/color.h"
#include "gBitmap.h"
#include "core/tVector.h"
#include "math/mRect.h"
#include "math/mMatrix.h"
#include "core/llist.h"

#include "gfx/gfxEnums.h"
#include "gfx/gfxShader.h"
#include "gfx/gfxVertexBuffer.h"
#include "gfx/gfxPrimitiveBuffer.h"
#include "gfx/gfxTarget.h"
#include "gfx/gfxStructs.h"
#include "gfx/gfxAdapter.h"
#include "gfx/gfxCubemap.h"
#include "core/refBase.h"
#include "gfx/gfxTextureObject.h"
#include "gfx/gfxTextureManager.h"
#include "gfx/gfxTextureHandle.h"
#include "gfx/gfxStateFrame.h"
#include "util/swizzle.h"

#include "core/unicode.h"
#include "core/frameAllocator.h"
#include "console/console.h"

#include "util/safeDelete.h"

#ifndef _SIGNAL_H_
#include "util/tSignal.h"
#include "sceneGraph/lightInfo.h"

#endif

class GFont;
class GFXCubemap;
class GFXCardProfiler;
class GFXFence;

// Global macro
#define GFX GFXDevice::get()

#define SFXBBCOPY_SIZE 512
#define MAX_MRT_TARGETS 4 

//-----------------------------------------------------------------------------

/// GFXDevice is the TSE graphics interface layer. This allows the TSE to
/// do many things, such as use multiple render devices for multi-head systems,
/// and allow a game to render in DirectX 9, OpenGL or any other API which has
/// a GFX implementation seamlessly. There are many concepts in GFX device which
/// may not be familiar to you, especially if you have not used DirectX.
/// @n
/// <b>Buffers</b>
/// There are three types of buffers in GFX: vertex, index and primitive. Please
/// note that index buffers are not accessable outside the GFX layer, they are wrapped
/// by primitive buffers. Primitive buffers will be explained in detail later.
/// Buffers are allocated and deallocated using their associated allocXBuffer and
/// freeXBuffer methods on the device. When a buffer is allocated you pass in a
/// pointer to, depending on the buffer, a vertex type pointer or a U16 pointer. 
/// During allocation, this pointer is set to the address of where you should
/// copy in the information for this buffer. You must the tell the GFXDevice
/// that the information is in, and it should prepare the buffer for use by calling
/// the prepare method on it. Dynamic vertex buffer example:
/// @code
/// GFXVertexP *verts;        // Making a buffer containing verticies with only position
///
/// // Allocate a dynamic vertex buffer to hold 3 vertices and use *verts as the location to copy information into
/// GFXVertexBufferHandle vb = GFX->allocVertexBuffer( 3, &verts, true ); 
///
/// // Now set the information, we're making a triangle
/// verts[0].point = Point3F( 200.f, 200.f, 0.f );
/// verts[1].point = Point3F( 200.f, 400.f, 0.f );
/// verts[2].point = Point3F( 400.f, 200.f, 0.f );
///
/// // Tell GFX that the information is in and it should be made ready for use
/// // Note that nothing is done with verts, this should not and MUST NOT be deleted
/// // stored, or otherwise used after prepare is called.
/// GFX->prepare( vb );
///
/// // Because this is a dynamic vertex buffer, it is only assured to be valid until someone 
/// // else allocates a dynamic vertex buffer, so we will render it now
/// GFX->setVertexBuffer( vb );
/// GFX->drawPrimitive( GFXTriangleStrip, 0, 1 );
///
/// // Now because this is a dynamic vertex buffer it MUST NOT BE FREED you are only
/// // given a handle to a vertex buffer which belongs to the device
/// @endcode
/// 
/// To use a static vertex buffer, it is very similar, this is an example using a
/// static primitive buffer:
/// @n
/// This takes place inside a constructor for a class which has a member variable
/// called mPB which is the primitive buffer for the class instance.
/// @code
/// U16 *idx;                          // This is going to be where to write indices
/// GFXPrimitiveInfo *primitiveInfo;   // This will be where to write primitive information
///
/// // Allocate a primitive buffer with 4 indices, and 1 primitive described for use
/// mPB = GFX->allocPrimitiveBuffer( 4, &idx, 1, &primitiveInfo );
///
/// // Write the index information, this is going to be for the outline of a triangle using
/// // a line strip
/// idx[0] = 0;
/// idx[1] = 1;
/// idx[2] = 2;
/// idx[3] = 0;
///
/// // Write the information for the primitive
/// primitiveInfo->indexStart = 0;            // Starting with index 0
/// primitiveInfo->minVertex = 0;             // The minimum vertex index is 0
/// primitiveInfo->maxVertex = 3;             // The maximum vertex index is 3
/// primitiveInfo->primitiveCount = 3;        // There are 3 lines we are drawing
/// primitiveInfo->type = GFXLineStrip;       // This primitive info describes a line strip
/// @endcode
/// The following code takes place in the destructor for the same class
/// @code
/// // Because this is a static buffer it's our responcibility to free it when we are done
/// GFX->freePrimitiveBuffer( mPB );
/// @endcode
/// This last bit takes place in the rendering function for the class
/// @code
/// // You need to set a vertex buffer as well, primitive buffers contain indexing
/// // information, not vertex information. This is so you could have, say, a static
/// // vertex buffer, and a dynamic primitive buffer.
///
/// // This sets the primitive buffer to the static buffer we allocated in the constructor
/// GFX->setPrimitiveBuffer( mPB );
/// 
/// // Draw the first primitive contained in the set primitive buffer, our primitive buffer
/// // has only one primitive, so we could also technically call GFX->drawPrimitives(); and
/// // get the same result. 
/// GFX->drawPrimitive( 0 );
/// @endcode
/// If you need any more examples on how to use these buffers please see the rest of the engine.
/// @n
/// <b>Primitive Buffers</b>
/// @n
/// Primitive buffers wrap and extend the concept of index buffers. The purpose of a primitive
/// buffer is to let objects store all information they have to render their primitives in
/// a central place. Say that a shape is made up of triangle strips and triangle fans, it would
/// still have only one primitive buffer which contained primitive information for each strip
/// and fan. It could then draw itself with one call.
///
/// TO BE FINISHED LATER
class GFXDevice
{
private:
    friend class GFXInit;
    friend class GFXPrimitiveBufferHandle;
    friend class GFXVertexBufferHandleBase;
    friend class GFXTextureObject;
    friend class GFXTexHandle;
    friend class _GFXCanonizer;
    friend class Blender;
    friend class GFXStateFrame;
    friend class sgLightingModel;
    friend class GFXResource;

    //--------------------------------------------------------------------------
    // Static GFX interface
    //--------------------------------------------------------------------------
public:
    enum GFXDeviceEventType
    {
        /// The device has been created, but not initialized
        deCreate,
        /// The device has been initialized
        deInit,
        /// The device is about to be destroyed.
        deDestroy
    };
    typedef Signal <GFXDeviceEventType> DeviceEventSignal;
    static DeviceEventSignal& getDeviceEventSignal();

private:
    /// @name Device management variables
    /// @{
    static Vector<GFXDevice*> smGFXDevice; ///< Global GFXDevice vector
    static S32 smActiveDeviceIndex;         ///< Active GFX Device index, signed so -1 can be uninitalized

    static bool smUseZPass;

    static DeviceEventSignal* smSignalGFXDeviceEvent;

    /// @}

public:
    static void create();
    static void destroy();

    static bool useZPass() { return smUseZPass; }

    static const Vector<GFXDevice*>* getDeviceVector() { return &smGFXDevice; };
    static GFXDevice* get();
    static void setActiveDevice(U32 deviceIndex);
    static bool devicePresent() { return smActiveDeviceIndex > -1 && smActiveDeviceIndex < smGFXDevice.size() && smGFXDevice[smActiveDeviceIndex] != NULL; }

    //--------------------------------------------------------------------------
    // Core GFX interface
    //--------------------------------------------------------------------------
private:
    /// Device ID for this device.
    U32 mDeviceIndex;

    /// Adapter for this device.
    GFXAdapter mAdapter;

protected:
    /// List of valid video modes for this device.
    Vector<GFXVideoMode> mVideoModes;

    /// Current video mode.
    GFXVideoMode mVideoMode;

    /// The CardProfiler for this device.
    GFXCardProfiler* mCardProfiler;

    /// Head of the resource list.
    ///
    /// @see GFXResource
    GFXResource *mResourceListHead;

    /// Set once the device is active.
    bool mCanCurrentlyRender;

    /// Set if we're in a mode where we want rendering to occur.
    bool mAllowRender;

    /// This will allow querying to see if a device is initialized and ready to
    /// have operations performed on it.
    bool mInitialized;

    /// This is called before this, or any other device, is deleted in the global destroy()
    /// method. It allows the device to clean up anything while everything is still valid.
    virtual void preDestroy() = 0;

    /// Set the adapter that this device is using.  For use by GFXInit::createDevice only.
    virtual void setAdapter(const GFXAdapter& adapter) { mAdapter = adapter; }

    /// Notify GFXDevice that we are initialized
    virtual void deviceInited();

public:
    GFXDevice();
    virtual ~GFXDevice();

    /// initialize - create window, device, etc
    virtual void init(const GFXVideoMode& mode/*, PlatformWindow *window = NULL*/) = 0;

    virtual void activate() = 0;
    virtual void deactivate() = 0;

    bool canCurrentlyRender();
    void setAllowRender(bool render);
    bool allowRender();

    GFXCardProfiler* getCardProfiler() const { return mCardProfiler; }
    const U32 getDeviceIndex() const { return mDeviceIndex; };

    /// Returns active graphics adapter type.
    virtual GFXAdapterType getAdapterType() = 0;

    /// Returns the Adapter that was used to create this device
    virtual const GFXAdapter& getAdapter() { return mAdapter; }

    /// @}

    /// @name Debug Methods
    /// @{

    virtual void enterDebugEvent(ColorI color, const char* name) = 0;
    virtual void leaveDebugEvent() = 0;
    virtual void setDebugMarker(ColorI color, const char* name) = 0;

    /// @}

    /// @name Resource debug methods
    /// @{

    /// Lists how many of each GFX resource (e.g. textures, texture targets, shaders, etc.) GFX is aware of
    /// @param unflaggedOnly   If true, this method only counts unflagged resources
    virtual void listResources(bool unflaggedOnly);

    /// Flags all resources GFX is currently aware of
    virtual void flagCurrentResources();

    /// Clears the flag on all resources GFX is currently aware of
    virtual void clearResourceFlags();

    /// Dumps a description of the specified resource types to the console
    /// @param resNames     A string of space separated class names (e.g. "GFXTextureObject GFXTextureTarget GFXShader")
    ///                     to describe to the console
    /// @param file         A path to the file to write the descriptions to.  If it is NULL or "", descriptions are
    ///                     written to the console.
    /// @param unflaggedOnly If true, this method only counts unflagged resources
    /// @note resNames is case sensitive because there is no dStristr function.
    virtual void describeResources(const char* resName, const char* file, bool unflaggedOnly);

    /// This method sets up the initial states for the GFXDevice. If state
    /// debugging is enabled, this method will get called at the beginning of
    /// every frame, and at the end of the frame, it will generate a report of the
    /// state activity on the states which it sets up.
    void setInitialGFXState();

protected:
    /// This is a helper method for describeResourcesToFile.  It walks through the
    /// GFXResource list and sorts it by item type, putting the resources into the proper vector.
    /// @see describeResources
    virtual void fillResourceVectors(const char* resNames, bool unflaggedOnly, Vector<GFXTextureObject*> &textureObjects,
                                     Vector<GFXTextureTarget*> &textureTargets, Vector<GFXWindowTarget*> &windowTargets,
                                     Vector<GFXVertexBuffer*> &vertexBuffers, Vector<GFXPrimitiveBuffer*> &primitiveBuffers,
                                     Vector<GFXFence*> &fences, Vector<GFXCubemap*> &cubemaps, Vector<GFXShader*> &shaders);

public:

    /// @}

    /// @name Video Mode Functions
    /// @{

    /// Returns the current video mode
    GFXVideoMode getVideoMode() const;

    /// Enumerates the supported video modes of the device
    virtual void enumerateVideoModes() = 0;

    /// Sets the video mode for the device
    virtual void setVideoMode(const GFXVideoMode& mode) = 0;

    /// Well, this function gets the video mode list!
    const Vector<GFXVideoMode>* const getVideoModeList() const;

    F32 formatByteSize(GFXFormat format);
    virtual GFXFormat selectSupportedFormat(GFXTextureProfile* profile,
        const Vector<GFXFormat>& formats, bool texture, bool mustblend) = 0;

    /// @}

    //-----------------------------------------------------------------------------

private:

    U32            mStateStackDepth;
    GFXStateFrame  mStateStack[STATE_STACK_SIZE];

    /// @name State-tracking methods
    /// These methods you should not call at all. These are what set the states
    /// for the state caching system.
    /// @{

    ///
    void trackRenderState(U32 state, U32 value);
    void trackTextureStageState(U32 stage, U32 state, U32 value);
    void trackSamplerState(U32 stage, U32 type, U32 value);
    /// @}

public:
    void pushState()
    {
        //      Con::printf("state {");
        mStateStackDepth++;
        AssertFatal(mStateStackDepth < STATE_STACK_SIZE, "Too deep a stack!");
        mStateStack[mStateStackDepth].init();
    }

    void popState()
    {
        AssertFatal(mStateStackDepth > 0, "Too shallow a stack!");
        mStateStack[mStateStackDepth].rollback();
        mStateStackDepth--;
        //      Con::printf("}");
    }

protected:

    /// @name State tracking variables
    /// @{
    bool mStateDirty;     ///< This should be true if ANY state is dirty, including matrices or primitive buffers

    StateTracker         mStateTracker[GFXRenderState_COUNT];
    U32                  mTrackedState[GFXRenderState_COUNT];
    U32                  mNumDirtyStates;

    StateTracker         mTextureStateTracker[TEXTURE_STAGE_COUNT][GFXTSS_COUNT];
    TextureDirtyTracker  mTextureTrackedState[TEXTURE_STAGE_COUNT * GFXTSS_COUNT];
    U32                  mNumDirtyTextureStates;

    StateTracker         mSamplerStateTracker[TEXTURE_STAGE_COUNT][GFXSAMP_COUNT];
    TextureDirtyTracker  mSamplerTrackedState[TEXTURE_STAGE_COUNT * GFXSAMP_COUNT];
    U32                  mNumDirtySamplerStates;


    enum TexDirtyType
    {
        GFXTDT_Normal,
        GFXTDT_Cube
    };

    GFXTexHandle mCurrentTexture[TEXTURE_STAGE_COUNT];
    GFXTexHandle mNewTexture[TEXTURE_STAGE_COUNT];
    GFXCubemapHandle mCurrentCubemap[TEXTURE_STAGE_COUNT];
    GFXCubemapHandle mNewCubemap[TEXTURE_STAGE_COUNT];

    TexDirtyType   mTexType[TEXTURE_STAGE_COUNT];
    bool           mTextureDirty[TEXTURE_STAGE_COUNT];
    bool           mTexturesDirty;

   /// @name Light Tracking
   /// @{

   LightInfo  mCurrentLight[LIGHT_STAGE_COUNT];
   bool          mCurrentLightEnable[LIGHT_STAGE_COUNT];
   bool          mLightDirty[LIGHT_STAGE_COUNT];
   bool          mLightsDirty;

   ColorF        mGlobalAmbientColor;
   bool          mGlobalAmbientColorDirty;

   /// @}

   /// @name Fixed function material tracking
   /// @{

   GFXLightMaterial mCurrentLightMaterial;
   bool mLightMaterialDirty;
    /// @}

    /// @name Bitmap modulation and color stack
    /// @{

    ///
    GFXVertexColor mBitmapModulation;
    GFXVertexColor mTextAnchorColor;
    GFXVertexColor mColorStackValue;
    /// @}

    /// @see getDeviceSwizzle32
    Swizzle<U8, 4> *mDeviceSwizzle32;

    /// @see getDeviceSwizzle24
    Swizzle<U8, 3> *mDeviceSwizzle24;

    //-----------------------------------------------------------------------------

    /// @name Matrix managing variables
    /// @{

    ///
    MatrixF mWorldMatrix[WORLD_STACK_MAX];
    bool    mWorldMatrixDirty;
    S32     mWorldStackSize;

    MatrixF mProjectionMatrix;
    bool    mProjectionMatrixDirty;

    MatrixF mViewMatrix;
    bool    mViewMatrixDirty;

    MatrixF mTextureMatrix[TEXTURE_STAGE_COUNT];
    bool    mTextureMatrixDirty[TEXTURE_STAGE_COUNT];
    bool    mTextureMatrixCheckDirty;
    /// @}

    //-----------------------------------------------------------------------------

    /// @name State setting functions
    /// These functions should set the state when called, not
    /// use the state-caching things. The state-tracking
    /// methods will call these functions.
    ///
    /// See their associated enums for more information
    /// @{

    /// Sets states which have to do with general rendering
    virtual void setRenderState(U32 state, U32 value) = 0;

    /// Sets states which have to do with how textures are displayed
    virtual void setTextureStageState(U32 stage, U32 state, U32 value) = 0;

    /// Sets states which have to do with texture sampling and addressing
    virtual void setSamplerState(U32 stage, U32 type, U32 value) = 0;
    /// @}

    virtual void setTextureInternal(U32 textureUnit, const GFXTextureObject* texture) = 0;

    virtual void setLightInternal(U32 lightStage, const LightInfo light, bool lightEnable) = 0;
    virtual void setGlobalAmbientInternal(ColorF color) = 0;
    virtual void setLightMaterialInternal(const GFXLightMaterial mat) = 0;

    virtual void beginSceneInternal() = 0;
    virtual void endSceneInternal() = 0;

    /// @name State Initalization.
    /// @{

    /// State initalization. This MUST BE CALLED in setVideoMode after the device
    /// is created.
    virtual void initStates() = 0;

    /// Helper to init render state
    void initRenderState(U32 state, U32 value);

    /// Helper to init texture state
    void initTextureState(U32 stage, U32 state, U32 value);

    /// Helper to init sampler state
    void initSamplerState(U32 stage, U32 state, U32 value);
    /// @}

    //-----------------------------------------------------------------------------

    /// This function must be implemented differently per
    /// API and it should set ONLY the current matrix.
    /// For example, in OpenGL, there should be NO matrix stack
    /// activity, all the stack stuff is managed in the GFX layer.
    ///
    /// OpenGL does not have seperate world and
    /// view matricies. It has Modelview which is world * view.
    /// You must take this into consideration.
    ///
    /// @param   mtype   Which matrix to set, world/view/projection
    /// @param   mat   Matrix to assign
    virtual void setMatrix(GFXMatrixType mtype, const MatrixF& mat) = 0;

    //-----------------------------------------------------------------------------
protected:


    /// @name Buffer Allocation 
    /// These methods are implemented per-device and are called by the GFX layer
    /// when a user calls an alloc
    ///
    /// @note Primitive Buffers are NOT implemented per device, they wrap index buffers
    /// @{

    /// This allocates a vertex buffer and returns a pointer to the allocated buffer.
    /// This function should not be called directly - rather it should be used by
    /// the GFXVertexBufferHandle class.

    /// @param   numVerts   Number of vertices to allocate
    /// @param   vertexPtr   A pointer to the location where the user can copy in vertex data (out)
    /// @param   vertType   Vertex type
    virtual GFXVertexBuffer* allocVertexBuffer(U32 numVerts, U32 vertFlags, U32 vertSize, GFXBufferType bufferType) = 0;

    RefPtr<GFXVertexBuffer> mCurrentVertexBuffer;
    bool mVertexBufferDirty;

    RefPtr<GFXPrimitiveBuffer> mCurrentPrimitiveBuffer;
    bool mPrimitiveBufferDirty;

    /// This allocates a primitive buffer and returns a pointer to the allocated buffer.
    /// A primitive buffer's type argument refers to the index data - the primitive data will
    /// always be preserved from call to call.
    ///
    /// @note All index buffers use 16-bit indices.
    /// @param   numIndices   Number of indices to allocate
    /// @param   indexPtr   Pointer to the location where the user can copy in index data (out)
    virtual GFXPrimitiveBuffer* allocPrimitiveBuffer(U32 numIndices, U32 numPrimitives, GFXBufferType bufferType) = 0;
    /// @}

    //---------------------------------------
    // SFX buffer
    //---------------------------------------
protected:
    GFXTexHandle mSfxBackBuffer;
    bool         mUseSfxBackBuffer;
    static S32   smSfxBackBufferSize;



    //---------------------------------------
    // Render target related
    //---------------------------------------
    Vector<GFXTargetRef> mRTStack;
    GFXTargetRef mCurrentRT;

    virtual void _updateRenderTargets();

public:

    /// @name Special FX Back Buffer functions
    /// @{

    ///
    virtual GFXTexHandle& getSfxBackBuffer() { return mSfxBackBuffer; }
    virtual void copyBBToSfxBuff() = 0;

    /// @}

    /// @name Texture functions
    /// @{
protected:
    GFXTextureManager* mTextureManager;

public:
    virtual void zombifyTextureManager() = 0;
    virtual void resurrectTextureManager() = 0;

    void registerTexCallback(GFXTexEventCallback callback, void* userData, S32& handle);
    void unregisterTexCallback(S32 handle);

    void reloadTextureResource(const char* filename) { mTextureManager->reloadTextureResource(filename); }
    virtual GFXCubemap* createCubemap() = 0;

    inline GFXTextureManager* getTextureManager()
    {
        return mTextureManager;
    }

    ///@}

    /// Swizzle to convert 32bpp bitmaps from RGBA to the native device format.
    const Swizzle<U8, 4> *getDeviceSwizzle32() const
    {
        return mDeviceSwizzle32;
    }

    /// Swizzle to convert 24bpp bitmaps from RGB to the native device format.
    const Swizzle<U8, 3> *getDeviceSwizzle24() const
    {
        return mDeviceSwizzle24;
    }

    /// @name Render Target functions
    /// @{

    /// Allocate a target for doing render to texture operations, with no
    /// depth/stencil buffer.
    virtual GFXTextureTarget *allocRenderToTextureTarget()=0;

    /// Allocate a target for a given window.
    virtual GFXWindowTarget *allocWindowTarget(/*PlatformWindow *window*/)=0;

    /// Save current render target states - note this works with MRT's
    virtual void pushActiveRenderTarget();

    /// Restore all render targets - supports MRT's
    virtual void popActiveRenderTarget();

    /// Start rendering to to a specified render target.
    virtual void setActiveRenderTarget( GFXTarget *target )=0;

    /// Return a pointer to the current active render target.
    virtual GFXTarget *getActiveRenderTarget();

    ///@}

    /// @name Shader functions
    /// @{
    virtual F32 getPixelShaderVersion() const = 0;
    virtual void  setPixelShaderVersion(F32 version) = 0;

    /// Returns the number of texture samplers that can be used in a shader rendering pass
    virtual U32 getNumSamplers() const = 0;

    virtual void setShader(GFXShader* shader) {};
    virtual void setVertexShaderConstF(U32 reg, const float* data, U32 size) {};
    virtual void setPixelShaderConstF(U32 reg, const float* data, U32 size) {};
    virtual void disableShaders() {};

    /// Creates a shader
    ///
    /// @param  vertFile       Vertex shader filename
    /// @param  pixFile        Pixel shader filename
    /// @param  pixVersion     Pixel shader version
    virtual GFXShader* createShader(const char* vertFile, const char* pixFile, F32 pixVersion) = 0;

    /// Generates a shader based on the passed in features
    virtual GFXShader* createShader(GFXShaderFeatureData& featureData, GFXVertexFlags vertFlags) = 0;

    /// Destroys shader
    virtual void destroyShader(GFXShader* shader) {};

    // This is called by MatInstance::reinitInstances to cause the shaders to be regenerated.
    virtual void flushProceduralShaders() = 0;

    /// @}

    //-----------------------------------------------------------------------------

    /// @name Rendering methods
    /// @{
    virtual void clear(U32 flags, ColorI color, F32 z, U32 stencil) = 0;
    virtual void beginScene();
    virtual void endScene();
    //virtual void swapBuffers() = 0;

    void setPrimitiveBuffer(GFXPrimitiveBuffer* buffer);
    void setVertexBuffer(GFXVertexBuffer* buffer);

    virtual void drawPrimitive(GFXPrimitiveType primType, U32 vertexStart, U32 primitiveCount) = 0;
    virtual void drawIndexedPrimitive(GFXPrimitiveType primType, U32 minIndex, U32 numVerts, U32 startIndex, U32 primitiveCount) = 0;

    void drawPrimitive(U32 primitiveIndex);
    void drawPrimitives();
    void drawPrimitiveBuffer(GFXPrimitiveBuffer* buffer);
    /// @}

    //-----------------------------------------------------------------------------

    /// Allocate a fence. The API specific implementation of GFXDevice is responsible
    /// to make sure that the proper type is used. GFXGeneralFence should work in
    /// all cases.
    virtual GFXFence *createFence() = 0;

    /// This resets a number of basic rendering states.  Insures that a forgotten
    /// state like a disable Z buffer doesn't affect other bits of rendering code.
    void setBaseRenderState();

    /// @name Light Settings
    /// NONE of these should be overridden by API implementations
    /// because of the state caching stuff.
    /// @{
    void setLight(U32 stage, LightInfo* light);
    void setLightMaterial(GFXLightMaterial mat);
    void setGlobalAmbientColor(ColorF color);

    /// @}

    /// @name Texture State Settings
    /// NONE of these should be overridden by API implementations
    /// because of the state caching stuff.
    /// @{

    ///
    void setTexture(U32 stage, GFXTextureObject* texture);
    void setCubeTexture(U32 stage, GFXCubemap* cubemap);

    void setTextureStageColorOp(U32 stage, GFXTextureOp op);
    void setTextureStageColorArg1(U32 stage, U32 argFlags);
    void setTextureStageColorArg2(U32 stage, U32 argFlags);
    void setTextureStageColorArg3( U32 stage, U32 argFlags );
    void setTextureStageResultArg( U32 stage, GFXTextureArgument arg );
    void setTextureStageAlphaOp(U32 stage, GFXTextureOp op);
    void setTextureStageAlphaArg1(U32 stage, U32 argFlags);
    void setTextureStageAlphaArg2(U32 stage, U32 argFlags);
    void setTextureStageBumpEnvMat(U32 stage, F32 mat[2][2]);
    void setTextureStageTransform(U32 stage, U32 texTransFlags);
    void setTextureMatrix(U32 stage, const MatrixF &texMat);
    /// @}

    //-----------------------------------------------------------------------------

    /// @name Sampler State Settings
    /// NONE of these should be overridden by API implementations
    /// because of the state caching stuff.
    /// @{

    /// 
    void setTextureStageAddressModeU(U32 stage, GFXTextureAddressMode mode);
    void setTextureStageAddressModeV(U32 stage, GFXTextureAddressMode mode);
    void setTextureStageAddressModeW(U32 stage, GFXTextureAddressMode mode);
    void setTextureStageBorderColor(U32 stage, ColorI color);
    void setTextureStageMagFilter(U32 stage, GFXTextureFilterType filter);
    void setTextureStageMinFilter(U32 stage, GFXTextureFilterType filter);
    void setTextureStageMipFilter(U32 stage, GFXTextureFilterType filter);
    void setTextureStageLODBias(U32 stage, F32 bias);
    void setTextureStageMaxMipLevel(U32 stage, U32 maxMiplevel);
    void setTextureStageMaxAnisotropy(U32 stage, F32 maxAnisotropy);
    /// @}

    //-----------------------------------------------------------------------------

    /// @name Render State Settings
    ///
    /// NONE of these should be overridden by API implementations
    /// because of the state caching stuff.
    ///
    /// @note Might be a good idea to have a render state that sets
    /// texturing on or off (openGL style).  Right now the proper way
    /// turn off texturing is to call "setTextureStageColorOp( 0, GFXTOPDisable )"
    /// which is not intuitive.  Even the interns are calling setTexture( 0, NULL)
    /// which is wrong. -bramage
    ///
    /// @{

    ///
    void setCullMode(GFXCullMode mode);
    GFXCullMode getCullMode();

    void setNormalizeNormals(bool doNormalize);
    void setZEnable(bool enable);
    void setZWriteEnable(bool enable);
    void setZFunc(GFXCmpFunc func);
    void setZBias(U32 zBias);
    void setSlopeScaleDepthBias(U32 slopebias)
    {
        trackRenderState(GFXRSSlopeScaleDepthBias, slopebias);
    }
    void enableColorWrites(bool red, bool green, bool blue, bool alpha);
    void setFillMode(GFXFillMode mode);
    void setShadeMode(GFXShadeMode mode);
    void setSpecularEnable(bool enable);
    void setAmbientLightColor(ColorI color);
    void setVertexColorEnable(bool enable);
    void setDiffuseMaterialSource(GFXMaterialColorSource src);
    void setSpecularMaterialSource(GFXMaterialColorSource src);
    void setAmbientMaterialSource(GFXMaterialColorSource src);
    void setEmissiveMaterialSource(GFXMaterialColorSource src);
    void setLightingEnable(bool enable);
    void setAlphaTestEnable(bool enable);
    void setSrcBlend(GFXBlend blend);
    void setDestBlend(GFXBlend blend);
    void setBlendOp(GFXBlendOp blendOp);
    void setAlphaRef(U8 alphaVal);
    void setAlphaFunc(GFXCmpFunc func);
    void setAlphaBlendEnable(bool enable);
    void setStencilEnable(bool enable);
    void setStencilFailOp(GFXStencilOp op);
    void setStencilZFailOp(GFXStencilOp op);
    void setStencilPassOp(GFXStencilOp op);
    void setStencilFunc(GFXCmpFunc func);
    void setStencilRef(U32 ref);
    void setStencilMask(U32 mask);
    void setStencilWriteMask(U32 mask);
    void setTextureFactor(ColorI color);
    /// @}

    //-----------------------------------------------------------------------------

    /// @name State-tracker interface
    /// @{

    /// Sets the dirty Render/Texture/Sampler states from the caching system
    void updateStates(bool forceSetAll = false);

    /// Returns the setting for a particular render state
    /// @param   state   Render state to get status of
    U32 getRenderState(U32 state) const;

    /// Returns the texture stage state
    /// @param   stage   Texture unit to query
    /// @param   state   State to get status of
    U32 getTextureStageState(U32 stage, U32 state) const;

    /// Returns the sampler state
    /// @param   stage   Texture unit to query
    /// @param   state   State to get status of  
    U32 getSamplerState(U32 stage, U32 type) const;
    /// @}

    //-----------------------------------------------------------------------------

    /// @name Matrix interface
    /// @{

    /// Sets the top of the world matrix stack
    /// @param   newWorld   New world matrix to set
    void setWorldMatrix(const MatrixF& newWorld);

    /// Gets the matrix on the top of the world matrix stack
    /// @param   out   Where to store the matrix
    const MatrixF& getWorldMatrix() const;

    /// Pushes the world matrix stack and copies the current top
    /// matrix to the new top of the stack
    void pushWorldMatrix();

    /// Pops the world matrix stack
    void popWorldMatrix();

    /// Sets the projection matrix
    /// @param   newProj   New projection matrix to set
    void setProjectionMatrix(const MatrixF& newProj);

    /// Gets the projection matrix
    /// @param   out   Where to store the projection matrix
    const MatrixF& getProjectionMatrix() const;

    /// Sets the view matrix
    /// @param   newView   New view matrix to set
    void setViewMatrix(const MatrixF& newView);

    /// Gets the view matrix
    /// @param   out   Where to store the view matrix
    const MatrixF& getViewMatrix() const;

    /// Multiplies the matrix at the top of the world matrix stack by a matrix
    /// and replaces the top of the matrix stack with the result
    /// @param   mat   Matrix to multiply
    void multWorld(const MatrixF& mat);

    virtual void setViewport(const RectI& rect) = 0;
    virtual const RectI& getViewport() const = 0;

    virtual void setClipRect(const RectI& rect) = 0;
    virtual void setClipRectOrtho( const RectI &rect, const RectI &orthoRect ) = 0;
    virtual const RectI& getClipRect() const = 0;

    void setFrustum(F32 left, F32 right, F32 bottom, F32 top, F32 nearPlane, F32 farPlane, bool bRotate = true);
    void getFrustum(F32* left, F32* right, F32* bottom, F32* top, F32* nearPlane, F32* farPlane);

    void setFrustum(F32 FOV, F32 aspectRatio, F32 nearPlane, F32 farPlane);
    void setOrtho(F32 left, F32 right, F32 bottom, F32 top, F32 nearPlane, F32 farPlane, bool doRotate = false);

    /// should this even be in GFX layer? -bramage
    F32 projectRadius(F32 dist, F32 radius);

    /// project 3D point onto 2D screen.  Returns X, Y, and depth in the outPoint vector.
//    virtual void project(Point3F& outPoint,
//        Point3F& inPoint,
//        MatrixF& modelview,
//        MatrixF& projection,
//        RectI& viewport) = 0;
//
//    virtual void unProject(Point3F& outPoint,
//        Point3F& inPoint,
//        MatrixF& modelview,
//        MatrixF& projection,
//        RectI& viewport) = 0;

    /// @}

    /// @name Immediate Mode
    ///
    /// A variety of helper functions you can use for immediate drawing. Useful
    /// for GUI controls.
    ///
    /// @{

    void drawRect(const Point2I& upperL, const Point2I& lowerR, const ColorI& color);
    void drawRect(const RectI& rect, const ColorI& color);
    void drawRectFill(const Point2I& upperL, const Point2I& lowerR, const ColorI& color);
    void drawRectFill(const RectI& rect, const ColorI& color);
    void drawLine(const Point2I& startPt, const Point2I& endPt, const ColorI& color);
    virtual void drawLine(S32 x1, S32 y1, S32 x2, S32 y2, const ColorI& color);
    void drawWireCube(const Point3F& size, const Point3F& pos, const ColorI& color);
    void draw2DSquare(const Point2F& screenPoint, F32 width, F32 spinAngle);

    U32 drawText(GFont* font, const Point2I& ptDraw, const UTF8* in_string,
        const ColorI* colorTable = NULL, const U32 maxColorIndex = 9, F32 rot = 0.f);

    U32 drawTextN(GFont* font, const Point2I& ptDraw, const UTF8* in_string,
        U32 n, const ColorI* colorTable = NULL, const U32 maxColorIndex = 9, F32 rot = 0.f);

    U32 drawText(GFont* font, const Point2I& ptDraw, const UTF16* in_string,
        const ColorI* colorTable = NULL, const U32 maxColorIndex = 9, F32 rot = 0.f);

    U32 drawTextN(GFont* font, const Point2I& ptDraw, const UTF16* in_string,
        U32 n, const ColorI* colorTable = NULL, const U32 maxColorIndex = 9, F32 rot = 0.f);

    void setBitmapModulation(const ColorI& modColor);
    void setTextAnchorColor(const ColorI& ancColor);
    void clearBitmapModulation();
    void getBitmapModulation(ColorI* color);

    void drawBitmap(GFXTextureObject* texture, const Point2I& in_rAt, const GFXBitmapFlip in_flip = GFXBitmapFlip_None);
    void drawBitmapSR(GFXTextureObject* texture, const Point2I& in_rAt, const RectI& srcRect, const GFXBitmapFlip in_flip = GFXBitmapFlip_None);
    void drawBitmapSR(GFXTextureObject* texture, const Point2F& in_rAt, const RectF& srcRect, const GFXBitmapFlip in_flip = GFXBitmapFlip_None);
    void drawBitmapStretch(GFXTextureObject* texture, const RectI& dstRect, const GFXBitmapFlip in_flip = GFXBitmapFlip_None);
    void drawBitmapStretchSR(GFXTextureObject* texture, const RectI& dstRect, const RectI& srcRect, const GFXBitmapFlip in_flip = GFXBitmapFlip_None);
    void drawBitmapStretchSR(GFXTextureObject* texture, const RectF& dstRect, const RectF& srcRect, const GFXBitmapFlip in_flip = GFXBitmapFlip_None);

    /// @}

    enum GenericShaderType
    {
        GSColor = 0,
        GSTexture,
        GSModColorTexture,
        GSAddColorTexture,
        GS_COUNT
    };
    /// This is a helper function to set a default shader for rendering GUI elements
    /// on systems which do not support fixed-function operations as well as for
    /// things which need just generic position/texture/color shaders
    ///
    /// @param  type  Type of generic shader, add your own if you need
    virtual void setupGenericShaders(GenericShaderType type = GSColor) {};

    /// Get the fill convention for this device
    virtual F32 getFillConventionOffset() const = 0;

    virtual U32 getMaxDynamicVerts() = 0;
    virtual U32 getMaxDynamicIndices() = 0;

    virtual void doParanoidStateCheck() {};

#ifndef TORQUE_SHIPPING
    /// This is a method designed for debugging. It will allow you to dump the states
    /// in the render manager out to a file so that it can be diffed and examined.
    void _dumpStatesToFile( const char *fileName ) const;
#else
    void _dumpStatesToFile( const char *fileName ) const {};
#endif
};

//-----------------------------------------------------------------------------
// Texture Stage States

inline void GFXDevice::setTextureStageColorOp(U32 stage, GFXTextureOp op)
{
    trackTextureStageState(stage, GFXTSSColorOp, op);
}

inline void GFXDevice::setTextureStageColorArg1(U32 stage, U32 argFlags)
{
    trackTextureStageState(stage, GFXTSSColorArg1, argFlags);
}

inline void GFXDevice::setTextureStageColorArg2(U32 stage, U32 argFlags)
{
    trackTextureStageState(stage, GFXTSSColorArg2, argFlags);
}

inline void GFXDevice::setTextureStageColorArg3( U32 stage, U32 argFlags )
{
   trackTextureStageState( stage, GFXTSSColorArg0, argFlags );
}

inline void GFXDevice::setTextureStageAlphaOp(U32 stage, GFXTextureOp op)
{
    trackTextureStageState(stage, GFXTSSAlphaOp, op);
}

inline void GFXDevice::setTextureStageResultArg( U32 stage, GFXTextureArgument arg )
{
   trackTextureStageState( stage, GFXTSSResultArg, arg );
}

inline void GFXDevice::setTextureStageAlphaArg1(U32 stage, U32 argFlags)
{
    trackTextureStageState(stage, GFXTSSAlphaArg1, argFlags);
}

inline void GFXDevice::setTextureStageAlphaArg2(U32 stage, U32 argFlags)
{
    trackTextureStageState(stage, GFXTSSAlphaArg2, argFlags);
}

inline void GFXDevice::setTextureStageTransform(U32 stage, U32 flags)
{
    trackTextureStageState(stage, GFXTSSTextureTransformFlags, flags);
}

inline void GFXDevice::setTextureMatrix(U32 stage, const MatrixF &texMat)
{
    mStateDirty = true;
    mTextureMatrixDirty[stage] = true;
    mTextureMatrix[stage] = texMat;
    mTextureMatrixCheckDirty = true;
}

// Utility function
inline U32 F32toU32(F32 f) { return *((U32*)&f); }

inline void GFXDevice::setTextureStageBumpEnvMat(U32 stage, F32 mat[2][2])
{
    trackTextureStageState(stage, GFXTSSBumpEnvMat00, F32toU32(mat[0][0]));
    trackTextureStageState(stage, GFXTSSBumpEnvMat01, F32toU32(mat[0][1]));
    trackTextureStageState(stage, GFXTSSBumpEnvMat10, F32toU32(mat[1][0]));
    trackTextureStageState(stage, GFXTSSBumpEnvMat11, F32toU32(mat[1][1]));
}

//-----------------------------------------------------------------------------
// Sampler States

inline void GFXDevice::setTextureStageAddressModeU(U32 stage, GFXTextureAddressMode mode)
{
    trackSamplerState(stage, GFXSAMPAddressU, mode);
}

inline void GFXDevice::setTextureStageAddressModeV(U32 stage, GFXTextureAddressMode mode)
{
    trackSamplerState(stage, GFXSAMPAddressV, mode);
}

inline void GFXDevice::setTextureStageAddressModeW(U32 stage, GFXTextureAddressMode mode)
{
    trackSamplerState(stage, GFXSAMPAddressW, mode);
}

inline void GFXDevice::setTextureStageBorderColor(U32 stage, ColorI color)
{
    trackSamplerState(stage, GFXSAMPBorderColor, 42/*color*/); // ToDo: Needs proper cast
}

inline void GFXDevice::setTextureStageMagFilter(U32 stage, GFXTextureFilterType filter)
{
    trackSamplerState(stage, GFXSAMPMagFilter, filter);
}

inline void GFXDevice::setTextureStageMinFilter(U32 stage, GFXTextureFilterType filter)
{
    trackSamplerState(stage, GFXSAMPMinFilter, filter);
}

inline void GFXDevice::setTextureStageMipFilter(U32 stage, GFXTextureFilterType filter)
{
    trackSamplerState(stage, GFXSAMPMipFilter, filter);
}

inline void GFXDevice::setTextureStageLODBias(U32 stage, F32 bias)
{
    trackSamplerState(stage, GFXSAMPMipMapLODBias, F32toU32(bias));
}

inline void GFXDevice::setTextureStageMaxMipLevel(U32 stage, U32 maxMiplevel)
{
    trackSamplerState(stage, GFXSAMPMaxMipLevel, maxMiplevel);
}

inline void GFXDevice::setTextureStageMaxAnisotropy(U32 stage, F32 maxAnisotropy)
{
    trackSamplerState(stage, GFXSAMPMaxAnisotropy, (U32)maxAnisotropy);
}

//-----------------------------------------------------------------------------
// Render States

inline void GFXDevice::enableColorWrites(bool red, bool green, bool blue, bool alpha)
{
    U32 mask = 0;

    mask |= (red ? GFXCOLORWRITEENABLE_RED : 0);
    mask |= (green ? GFXCOLORWRITEENABLE_GREEN : 0);
    mask |= (blue ? GFXCOLORWRITEENABLE_BLUE : 0);
    mask |= (alpha ? GFXCOLORWRITEENABLE_ALPHA : 0);

    trackRenderState(GFXRSColorWriteEnable, mask);
}

inline void GFXDevice::setAlphaBlendEnable(bool enable)
{
    trackRenderState(GFXRSAlphaBlendEnable, enable);
}

inline void GFXDevice::setAlphaFunc(GFXCmpFunc func)
{
    trackRenderState(GFXRSAlphaFunc, func);
}

inline void GFXDevice::setAlphaRef(U8 alphaVal)
{
    trackRenderState(GFXRSAlphaRef, alphaVal);
}

inline void GFXDevice::setAlphaTestEnable(bool enable)
{
    trackRenderState(GFXRSAlphaTestEnable, enable);
}

inline void GFXDevice::setAmbientLightColor(ColorI color)
{
    trackRenderState(GFXRSAmbient, color.getARGBPack());
}

inline void GFXDevice::setVertexColorEnable(bool enable)
{
    trackRenderState(GFXRSColorVertex, enable);
}

inline void GFXDevice::setAmbientMaterialSource(GFXMaterialColorSource src)
{
    trackRenderState(GFXRSAmbientMaterialSource, src);
}

inline void GFXDevice::setBlendOp(GFXBlendOp blendOp)
{
    trackRenderState(GFXRSBlendOp, blendOp);
}

inline void GFXDevice::setCullMode(GFXCullMode mode)
{
    trackRenderState(GFXRSCullMode, mode);
}

inline GFXCullMode GFXDevice::getCullMode()
{
    return (GFXCullMode)mStateTracker[GFXRSCullMode].currentValue;
}

inline void GFXDevice::setDestBlend(GFXBlend blend)
{
    trackRenderState(GFXRSDestBlend, blend);
}

inline void GFXDevice::setDiffuseMaterialSource(GFXMaterialColorSource src)
{
    trackRenderState(GFXRSDiffuseMaterialSource, src);
}

inline void GFXDevice::setEmissiveMaterialSource(GFXMaterialColorSource src)
{
    trackRenderState(GFXRSEmissiveMaterialSource, src);
}

inline void GFXDevice::setFillMode(GFXFillMode mode)
{
    trackRenderState(GFXRSFillMode, mode);
}

inline void GFXDevice::setLightingEnable(bool enable)
{
    trackRenderState(GFXRSLighting, enable);
}

inline void GFXDevice::setNormalizeNormals(bool doNormalize)
{
    trackRenderState(GFXRSNormalizeNormals, doNormalize);
}

inline void GFXDevice::setShadeMode(GFXShadeMode mode)
{
    trackRenderState(GFXRSShadeMode, mode);
}

inline void GFXDevice::setSpecularEnable(bool enable)
{
    trackRenderState(GFXRSSpecularEnable, enable);
}

inline void GFXDevice::setSpecularMaterialSource(GFXMaterialColorSource src)
{
    trackRenderState(GFXRSSpecularMaterialSource, src);
}

inline void GFXDevice::setSrcBlend(GFXBlend blend)
{
    trackRenderState(GFXRSSrcBlend, blend);
}

inline void GFXDevice::setStencilEnable(bool enable)
{
    trackRenderState(GFXRSStencilEnable, enable);
}

inline void GFXDevice::setStencilFailOp(GFXStencilOp op)
{
    trackRenderState(GFXRSStencilFail, op);
}

inline void GFXDevice::setStencilFunc(GFXCmpFunc func)
{
    trackRenderState(GFXRSStencilFunc, func);
}

inline void GFXDevice::setStencilMask(U32 mask)
{
    trackRenderState(GFXRSStencilMask, mask);
}

inline void GFXDevice::setStencilPassOp(GFXStencilOp op)
{
    trackRenderState(GFXRSStencilPass, op);
}

inline void GFXDevice::setStencilRef(U32 ref)
{
    trackRenderState(GFXRSStencilRef, ref);
}

inline void GFXDevice::setStencilWriteMask(U32 mask)
{
    trackRenderState(GFXRSStencilWriteMask, mask);
}

inline void GFXDevice::setStencilZFailOp(GFXStencilOp op)
{
    trackRenderState(GFXRSStencilZFail, op);
}

inline void GFXDevice::setTextureFactor(ColorI color)
{
    trackRenderState(GFXRSTextureFactor, color.getARGBPack());
}


inline void GFXDevice::setZBias(U32 zBias)
{
    trackRenderState(GFXRSDepthBias, zBias);
}

inline void GFXDevice::setZEnable(bool enable)
{
    trackRenderState(GFXRSZEnable, enable);
}

inline void GFXDevice::setZFunc(GFXCmpFunc func)
{
    trackRenderState(GFXRSZFunc, func);
}

inline void GFXDevice::setZWriteEnable(bool enable)
{
    trackRenderState(GFXRSZWriteEnable, enable);
}

//-----------------------------------------------------------------------------
// State caching methods

inline void GFXDevice::trackRenderState(U32 state, U32 value)
{
    if (!mStateTracker[state].dirty)
    {
        if (mStateTracker[state].currentValue == value)
            return;

        // Update our internal data.
        mStateTracker[state].dirty = true;
        mTrackedState[mNumDirtyStates] = state;
        mNumDirtyStates++;
        mStateDirty = true;
    }

    // Update the state stack.
    if (mStateStackDepth)
        mStateStack[mStateStackDepth].trackRenderState(state, mStateTracker[state].newValue);

    mStateTracker[state].newValue = value;
}

inline void GFXDevice::trackTextureStageState(U32 stage, U32 state, U32 value)
{
    if (!mTextureStateTracker[stage][state].dirty)
    {
        if (mTextureStateTracker[stage][state].currentValue == value)
            return;

        // Update our internal data.
        mTextureStateTracker[stage][state].dirty = true;
        mStateDirty = true;
        mTextureTrackedState[mNumDirtyTextureStates].state = state;
        mTextureTrackedState[mNumDirtyTextureStates].stage = stage;
        mNumDirtyTextureStates++;
    }

    // Update the state stack.
    if (mStateStackDepth)
        mStateStack[mStateStackDepth].trackTextureStageState(stage, state, mTextureStateTracker[stage][state].newValue);

    mTextureStateTracker[stage][state].newValue = value;
}


inline void GFXDevice::trackSamplerState(U32 stage, U32 type, U32 value)
{
    if (!mSamplerStateTracker[stage][type].dirty)
    {
        if (mSamplerStateTracker[stage][type].currentValue == value)
            return;

        // Update our internal data.
        mSamplerStateTracker[stage][type].dirty = true;
        mStateDirty = true;
        mSamplerTrackedState[mNumDirtySamplerStates].state = type;
        mSamplerTrackedState[mNumDirtySamplerStates].stage = stage;
        mNumDirtySamplerStates++;
    }

    // Update the state stack.
    if (mStateStackDepth)
        mStateStack[mStateStackDepth].trackSamplerState(stage, type, mSamplerStateTracker[stage][type].newValue);

    mSamplerStateTracker[stage][type].newValue = value;
}


//-----------------------------------------------------------------------------
// State tracker interface

inline U32 GFXDevice::getRenderState(U32 state) const {
    return mStateTracker[state].newValue;
}

inline U32 GFXDevice::getTextureStageState(U32 stage, U32 state) const {
    return mTextureStateTracker[stage][state].newValue;
}

inline U32 GFXDevice::getSamplerState(U32 stage, U32 type) const {
    return mSamplerStateTracker[stage][type].newValue;
}

inline void GFXDevice::initRenderState(U32 state, U32 value)
{
    mStateTracker[state].dirty = false;
    mStateTracker[state].newValue = value;
    mStateTracker[state].currentValue = value;
}

inline void GFXDevice::initTextureState(U32 stage, U32 state, U32 value)
{
    mTextureStateTracker[stage][state].dirty = false;
    mTextureStateTracker[stage][state].newValue = value;
    mTextureStateTracker[stage][state].currentValue = value;
}

inline void GFXDevice::initSamplerState(U32 stage, U32 state, U32 value)
{
    mSamplerStateTracker[stage][state].dirty = false;
    mSamplerStateTracker[stage][state].newValue = value;
    mSamplerStateTracker[stage][state].currentValue = value;
}

//-----------------------------------------------------------------------------
// Matrix interface

inline void GFXDevice::setWorldMatrix(const MatrixF& newWorld)
{
    mWorldMatrixDirty = true;
    mStateDirty = true;
    mWorldMatrix[mWorldStackSize] = newWorld;
}

inline const MatrixF& GFXDevice::getWorldMatrix() const {
    return mWorldMatrix[mWorldStackSize];
}

inline void GFXDevice::pushWorldMatrix()
{
    mWorldMatrixDirty = true;
    mStateDirty = true;
    mWorldStackSize++;
    if (mWorldStackSize >= WORLD_STACK_MAX)
    {
        AssertFatal(false, "GFX: Exceeded world matrix stack size");
    }
    mWorldMatrix[mWorldStackSize] = mWorldMatrix[mWorldStackSize - 1];
}

inline void GFXDevice::popWorldMatrix()
{
    mWorldMatrixDirty = true;
    mStateDirty = true;
    mWorldStackSize--;
    if (mWorldStackSize < 0)
    {
        AssertFatal(false, "GFX: Negative WorldStackSize!");
    }
}

inline void GFXDevice::multWorld(const MatrixF& mat)
{
    mWorldMatrixDirty = true;
    mStateDirty = true;
    mWorldMatrix[mWorldStackSize].mul(mat);
}

inline void GFXDevice::setProjectionMatrix(const MatrixF& newProj)
{
    mProjectionMatrixDirty = true;
    mStateDirty = true;
    mProjectionMatrix = newProj;
}

inline const MatrixF& GFXDevice::getProjectionMatrix() const {
    return mProjectionMatrix;
}

inline void GFXDevice::setViewMatrix(const MatrixF& newView)
{
    mStateDirty = true;
    mViewMatrixDirty = true;
    mViewMatrix = newView;
}

inline const MatrixF& GFXDevice::getViewMatrix() const
{
    return mViewMatrix;
}

//-----------------------------------------------------------------------------
// Misc

inline GFXVideoMode GFXDevice::getVideoMode() const
{
    return mVideoMode;
}

inline const Vector<GFXVideoMode>* const GFXDevice::getVideoModeList() const
{
    return &mVideoModes;
}

//-----------------------------------------------------------------------------
// Buffer management

inline void GFXDevice::setVertexBuffer(GFXVertexBuffer* buffer)
{
    if (buffer == mCurrentVertexBuffer)
        return;

    mCurrentVertexBuffer = buffer;
    mVertexBufferDirty = true;
    mStateDirty = true;
}

//-----------------------------------------------------------------------------
// Imediate mode

inline void GFXDevice::drawRect(const RectI& rect, const ColorI& color)
{
    drawRect(rect.point,
        Point2I(rect.extent.x + rect.point.x - 1, rect.extent.y + rect.point.y - 1),
        color);
}

inline void GFXDevice::drawRectFill(const RectI& rect, const ColorI& color)
{
    drawRectFill(rect.point,
        Point2I(rect.extent.x + rect.point.x - 1, rect.extent.y + rect.point.y - 1),
        color);
}

inline void GFXDevice::drawLine(const Point2I& startPt, const Point2I& endPt, const ColorI& color)
{
    drawLine(startPt.x, startPt.y, endPt.x, endPt.y, color);
}

// Bitmap modulation

inline void GFXDevice::setBitmapModulation(const ColorI& modColor)
{
    mBitmapModulation = modColor;
}

inline void GFXDevice::clearBitmapModulation()
{
    mBitmapModulation.set(255, 255, 255, 255);
}

inline void GFXDevice::getBitmapModulation(ColorI* color)
{
    mBitmapModulation.getColor(color);
}

inline void GFXDevice::setTextAnchorColor(const ColorI& ancColor)
{
    mTextAnchorColor = ancColor;
}

// Draw bitmap

inline void GFXDevice::drawBitmap(GFXTextureObject* texture, const Point2I& in_rAt, const GFXBitmapFlip in_flip)
{
    AssertFatal(texture, "No texture specified for drawBitmap()");

    RectI subRegion(0, 0, texture->mBitmapSize.x + 1, texture->mBitmapSize.y + 1);
    RectI stretch(in_rAt.x, in_rAt.y, texture->mBitmapSize.x, texture->mBitmapSize.y);
    drawBitmapStretchSR(texture, stretch, subRegion, in_flip);
}

inline void GFXDevice::drawBitmapStretch(GFXTextureObject* texture, const RectI& dstRect, const GFXBitmapFlip in_flip)
{
    AssertFatal(texture, "No texture specified for drawBitmapStretch()");

    RectI subRegion(0, 0, texture->mBitmapSize.x, texture->mBitmapSize.y);
    drawBitmapStretchSR(texture, dstRect, subRegion, in_flip);
}

inline void GFXDevice::drawBitmapSR(GFXTextureObject* texture, const Point2I& in_rAt, const RectI& srcRect, const GFXBitmapFlip in_flip)
{
    AssertFatal(texture, "No texture specified for drawBitmapSR()");

    RectI stretch(in_rAt.x, in_rAt.y, srcRect.len_x(), srcRect.len_y());
    drawBitmapStretchSR(texture, stretch, srcRect, in_flip);
}

inline void GFXDevice::drawBitmapSR(GFXTextureObject* texture, const Point2F& in_rAt, const RectF& srcRect, const GFXBitmapFlip in_flip)
{
    AssertFatal(texture, "No texture specified for drawBitmapSR()");

    RectF stretch(in_rAt.x, in_rAt.y, srcRect.len_x(), srcRect.len_y());
    drawBitmapStretchSR(texture, stretch, srcRect, in_flip);
}

// Text

inline U32 GFXDevice::drawText(GFont* font, const Point2I& ptDraw, const UTF16* in_string,
    const ColorI* colorTable, const U32 maxColorIndex, F32 rot)
{
    return drawTextN(font, ptDraw, in_string, dStrlen(in_string), colorTable, maxColorIndex, rot);
}

inline U32 GFXDevice::drawText(GFont* font, const Point2I& ptDraw, const UTF8* in_string,
    const ColorI* colorTable, const U32 maxColorIndex, F32 rot)
{
    return drawTextN(font, ptDraw, in_string, dStrlen(in_string), colorTable, maxColorIndex, rot);
}

inline U32 GFXDevice::drawTextN(GFont* font, const Point2I& ptDraw, const UTF8* in_string, U32 n,
    const ColorI* colorTable, const U32 maxColorIndex, F32 rot)
{
    // Convert to UTF16 temporarily.
    FrameTemp<UTF16> ubuf((n + 1) * sizeof(UTF16));
    U32 realLen = convertUTF8toUTF16(in_string, ubuf, (n + 1));
    return drawTextN(font, ptDraw, ubuf, realLen, colorTable, maxColorIndex, rot);
}

inline bool GFXDevice::canCurrentlyRender()
{
    return mCanCurrentlyRender;
}

inline bool GFXDevice::allowRender()
{
    return mAllowRender;
}

inline void GFXDevice::setAllowRender(bool render)
{
    mAllowRender = render;
}

#endif
