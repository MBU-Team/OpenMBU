//-----------------------------------------------------------------------------
// Torque Shader Engine
// 
// Copyright (c) 2003 GarageGames.Com
//-----------------------------------------------------------------------------

#include "gfx/gfxDevice.h"
#include "console/console.h"
#include "gfx/gfxInit.h"
#include "core/frameAllocator.h"
#include "gfx/gFont.h"
#include "gfx/gfxTextureHandle.h"
#include "gfx/gfxCubemap.h"
#include "gfx/gfxFence.h"
#include "sceneGraph/sceneGraph.h"
#include "gfx/debugDraw.h"
#include "gfx/gNewFont.h"
#include "platform/profiler.h"
#include "core/unicode.h"
#include "core/fileStream.h"

Vector<GFXDevice*> GFXDevice::smGFXDevice;
S32 GFXDevice::smActiveDeviceIndex = -1;
bool GFXDevice::smUseZPass = true;
GFXDevice::DeviceEventSignal* GFXDevice::smSignalGFXDeviceEvent = NULL;
S32 GFXDevice::smSfxBackBufferSize = 512;//128;//64;


//-----------------------------------------------------------------------------

// Static method
GFXDevice* GFXDevice::get()
{
    // This triggers sometimes on exit.
    /*if (smActiveDeviceIndex >= smGFXDevice.size())
    {
        char buf[1024];
        dSprintf(buf, sizeof(buf), "GFXDevice::get() - invalid device index %d out of %d", smActiveDeviceIndex, smGFXDevice.size());
        AssertFatal(false, buf);
        return NULL;
    }*/

    AssertFatal(smActiveDeviceIndex > -1  && smActiveDeviceIndex < smGFXDevice.size()
                && smGFXDevice[smActiveDeviceIndex] != NULL,
                "Attempting to get invalid GFX device. GFX may not have been initialized!");
    return smGFXDevice[smActiveDeviceIndex];
}

GFXDevice::DeviceEventSignal& GFXDevice::getDeviceEventSignal()
{
    if (smSignalGFXDeviceEvent == NULL)
    {
        smSignalGFXDeviceEvent = new GFXDevice::DeviceEventSignal();
    }
    return *smSignalGFXDeviceEvent;
}

//-----------------------------------------------------------------------------

GFXDevice::GFXDevice()
{
    mNumDirtyStates = 0;
    mNumDirtyTextureStates = 0;
    mNumDirtySamplerStates = 0;

    mWorldMatrixDirty = false;
    mWorldStackSize = 0;
    mProjectionMatrixDirty = false;
    mViewMatrixDirty = false;
    mTextureMatrixCheckDirty = false;

    mViewMatrix.identity();
    mProjectionMatrix.identity();

    for (int i = 0; i < WORLD_STACK_MAX; i++)
        mWorldMatrix[i].identity();

    mDeviceIndex = smGFXDevice.size();
    smGFXDevice.push_back(this);

    if (smActiveDeviceIndex == -1)
        smActiveDeviceIndex = 0;

    // Vertex buffer cache
    mVertexBufferDirty = false;

    // Primitive buffer cache
    mPrimitiveBufferDirty = false;
    mTexturesDirty = false;

    // Use of TEXTURE_STAGE_COUNT in initialization is okay [7/2/2007 Pat]
    for (U32 i = 0; i < TEXTURE_STAGE_COUNT; i++)
    {
        mTextureDirty[i] = false;
        mCurrentTexture[i] = NULL;
        mNewTexture[i] = NULL;
        mCurrentCubemap[i] = NULL;
        mNewCubemap[i] = NULL;
        mTexType[i] = GFXTDT_Normal;

        mTextureMatrix[i].identity();
        mTextureMatrixDirty[i] = false;
    }

   mLightsDirty = false;
   for(U32 i = 0; i < LIGHT_STAGE_COUNT; i++)
   {
      mLightDirty[i] = false;
      mCurrentLightEnable[i] = false;
   }

   mGlobalAmbientColorDirty = false;
   mGlobalAmbientColor = ColorF(0.0f, 0.0f, 0.0f, 1.0f);

   mLightMaterialDirty = false;
   dMemset(&mCurrentLightMaterial, NULL, sizeof(GFXLightMaterial));
   
    // Initialize the state stack.
    mStateStackDepth = 0;
    mStateStack[0].init();

    // misc
    mAllowRender = true;

    mInitialized = false;

    mDeviceSwizzle32 = NULL;
    mDeviceSwizzle24 = NULL;

    mResourceListHead = NULL;
}

//-----------------------------------------------------------------------------

void GFXDevice::deviceInited()
{
    getDeviceEventSignal().trigger(deInit);
}

// Static method
void GFXDevice::create()
{
    //Con::addVariable("pref::Video::sfxBackBufferSize", TypeS32, &GFXDevice::smSfxBackBufferSize);
    Con::addVariable("pref::video::useZPass", TypeBool, &GFXDevice::smUseZPass);

    Con::addVariable("$pref::Video::ReflectionDetailLevel", TypeS32, &GFXCubemap::smReflectionDetailLevel);
}

//-----------------------------------------------------------------------------

// Static method
void GFXDevice::destroy()
{
    // Make this release its buffer.
    PrimBuild::shutdown();

    // Let people know we are shutting down
    if (smSignalGFXDeviceEvent)
    {
        smSignalGFXDeviceEvent->trigger(deDestroy);
        delete smSignalGFXDeviceEvent;
        smSignalGFXDeviceEvent = NULL;
    }

    // Destroy this way otherwise we are modifying the loop end case
    U32 arraySize = smGFXDevice.size();

    // Call preDestroy on them all
    for (U32 i = 0; i < arraySize; i++)
    {
        GFXDevice* curr = smGFXDevice[0];

        curr->preDestroy();
    }

    // Delete them...all of them
    for (U32 i = 0; i < arraySize; i++)
    {
        GFXDevice* curr = smGFXDevice[0];

        smActiveDeviceIndex = 0;
        //smGFXDevice.pop_front();

        SAFE_DELETE(curr);
    }
}

//-----------------------------------------------------------------------------

// Static method
void GFXDevice::setActiveDevice(U32 deviceIndex)
{
    AssertFatal(deviceIndex < smGFXDevice.size(), "Device non-existant");

    smActiveDeviceIndex = deviceIndex;
}

//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------

GFXDevice::~GFXDevice()
{
    if (smActiveDeviceIndex == mDeviceIndex)
        smActiveDeviceIndex = 0;

    // Loop through all the GFX devices to find ourself.  If we find ourself,
    // remove ourself from the vector and decrease the device index of all devices
    // which came after us.
    for(Vector<GFXDevice *>::iterator i = smGFXDevice.begin(); i != smGFXDevice.end(); i++)
    {
        if( (*i)->mDeviceIndex == mDeviceIndex )
        {
            for (Vector<GFXDevice *>::iterator j = i + 1; j != smGFXDevice.end(); j++)
                (*j)->mDeviceIndex--;
            smGFXDevice.erase( i );
            break;
        }
    }

    /// In the future, we may need to separate out this code block that clears out the GFXDevice refptrs, so that
    /// derived classes can clear them out before releasing the device or something. BTR
    mSfxBackBuffer = NULL;

    // Clean up our current PB, if any.
    mCurrentPrimitiveBuffer = NULL;
    mCurrentVertexBuffer = NULL;

    bool found = false;

//    for (Vector<GFXDevice*>::iterator i = smGFXDevice.begin(); i != smGFXDevice.end(); i++)
//    {
//        if ((*i)->mDeviceIndex == mDeviceIndex)
//        {
//            smGFXDevice.erase(i);
//            found = true;
//        }
//        else if (found == true)
//        {
//            (*i)->mDeviceIndex--;
//        }
//    }

    // Clear out our current texture references
    for (U32 i = 0; i < TEXTURE_STAGE_COUNT; i++)
    {
        mCurrentTexture[i] = NULL;
        mNewTexture[i] = NULL;
        mCurrentCubemap[i] = NULL;
        mNewCubemap[i] = NULL;
    }

    // Check for resource leaks
#ifdef TORQUE_DEBUG
    //GFXTextureObject::dumpActiveTOs();
    GFXPrimitiveBuffer::dumpActivePBs();
#endif

    SAFE_DELETE( mTextureManager );
    //SAFE_DELETE( mDrawer );

#ifdef TORQUE_GFX_STATE_DEBUG
    SAFE_DELETE( mDebugStateManager );
#endif

    /// End Block above BTR

    // -- Clear out resource list
    // Note: our derived class destructor will have already released resources.
    // Clearing this list saves us from having our resources (which are not deleted
    // just released) turn around and try to remove themselves from this list.
    while (mResourceListHead)
    {
        GFXResource * head = mResourceListHead;
        mResourceListHead = head->mNextResource;

        head->mPrevResource = NULL;
        head->mNextResource = NULL;
        head->mOwningDevice = NULL;
    }

    // mark with bad device index so we can detect already deleted device
    mDeviceIndex = -1;
}

//-----------------------------------------------------------------------------

F32 GFXDevice::formatByteSize(GFXFormat format)
{
    if (format < GFXFormat_16BIT)
        return 1.0f;// 8 bit...
    else if (format < GFXFormat_24BIT)
        return 2.0f;// 16 bit...
    else if (format < GFXFormat_32BIT)
        return 3.0f;// 24 bit...
    else if (format < GFXFormat_64BIT)
        return 4.0f;// 32 bit...
    else if (format < GFXFormat_128BIT)
        return 8.0f;// 64 bit...
    else if (format < GFXFormat_UNKNOWNSIZE)
        return 16.0f;// 128 bit...
    return 4.0;// default...
}

//-----------------------------------------------------------------------------

// NOTE: I changed the ordering of state setting in this function because OpenGL
// is going to store texture filtering etc along with the texture object itself
// so if the texture got set *after* the sampler states, it would ice the sampler
// states it just set. 
//
// If this causes problems, talk to patw
void GFXDevice::updateStates(bool forceSetAll /*=false*/)
{
    PROFILE_SCOPE(GFXDevice_updateStates);

    if(forceSetAll)
    {
        bool rememberToEndScene = false;
        if(!canCurrentlyRender())
        {
            beginScene();
            rememberToEndScene = true;
        }

        setMatrix( GFXMatrixProjection, mProjectionMatrix );
        setMatrix( GFXMatrixWorld, mWorldMatrix[mWorldStackSize] );
        setMatrix( GFXMatrixView, mViewMatrix );

        if(mCurrentVertexBuffer.isValid())
            mCurrentVertexBuffer->prepare();

        if( mCurrentPrimitiveBuffer.isValid() ) // This could be NULL when the device is initalizing
            mCurrentPrimitiveBuffer->prepare();

        for(U32 i = 0; i < getNumSamplers(); i++)
        {
            switch (mTexType[i])
            {
                case GFXTDT_Normal :
                {
                    mCurrentTexture[i] = mNewTexture[i];
                    setTextureInternal(i, mCurrentTexture[i]);
                }
                    break;
                case GFXTDT_Cube :
                {
                    mCurrentCubemap[i] = mNewCubemap[i];
                    if (mCurrentCubemap[i])
                        mCurrentCubemap[i]->setToTexUnit(i);
                    else
                        setTextureInternal(i, NULL);
                }
                    break;
                default:
                AssertFatal(false, "Unknown texture type!");
                    break;
            }
        }

        for(S32 i=0; i<GFXRenderState_COUNT; i++)
        {
            setRenderState(i, mStateTracker[i].newValue);
            mStateTracker[i].currentValue = mStateTracker[i].newValue;
        }

        // Why is this 8 and not 16 like the others?
        for(S32 i=0; i<8; i++)
        {
            for(S32 j=0; j<GFXTSS_COUNT; j++)
            {
                StateTracker &st = mTextureStateTracker[i][j];
                setTextureStageState(i, j, st.newValue);
                st.currentValue = st.newValue;
            }
        }

        for(S32 i=0; i<8; i++)
        {
            for(S32 j=0; j<GFXSAMP_COUNT; j++)
            {
                StateTracker &st = mSamplerStateTracker[i][j];
                setSamplerState(i, j, st.newValue);
                st.currentValue = st.newValue;
            }
        }
        // Set our material
        setLightMaterialInternal(mCurrentLightMaterial);

        // Set our lights
        for(U32 i = 0; i < LIGHT_STAGE_COUNT; i++)
        {
            setLightInternal(i, mCurrentLight[i], mCurrentLightEnable[i]);
        }

        _updateRenderTargets();

        if(rememberToEndScene)
            endScene();

        return;
    }

    // Normal update logic begins here.
    mStateDirty = false;

    // Update Projection Matrix
    if( mProjectionMatrixDirty )
    {
        setMatrix( GFXMatrixProjection, mProjectionMatrix );
        mProjectionMatrixDirty = false;
    }

    // Update World Matrix
    if( mWorldMatrixDirty )
    {
        setMatrix( GFXMatrixWorld, mWorldMatrix[mWorldStackSize] );
        mWorldMatrixDirty = false;
    }

    // Update View Matrix
    if( mViewMatrixDirty )
    {
        setMatrix( GFXMatrixView, mViewMatrix );
        mViewMatrixDirty = false;
    }


    if( mTextureMatrixCheckDirty )
    {
        for( int i = 0; i < getNumSamplers(); i++ )
        {
            if( mTextureMatrixDirty[i] )
            {
                mTextureMatrixDirty[i] = false;
                setMatrix( (GFXMatrixType)(GFXMatrixTexture + i), mTextureMatrix[i] );
            }
        }

        mTextureMatrixCheckDirty = false;
    }

    // Update vertex buffer
    if( mVertexBufferDirty )
    {
        if(mCurrentVertexBuffer.isValid())
            mCurrentVertexBuffer->prepare();
        mVertexBufferDirty = false;
    }

    // Update primitive buffer
    //
    // NOTE: It is very important to set the primitive buffer AFTER the vertex buffer
    // because in order to draw indexed primitives in DX8, the call to SetIndicies
    // needs to include the base vertex offset, and the DX8 GFXDevice relies on
    // having mCurrentVB properly assigned before the call to setIndices -patw
    if( mPrimitiveBufferDirty )
    {
        if( mCurrentPrimitiveBuffer.isValid() ) // This could be NULL when the device is initalizing
            mCurrentPrimitiveBuffer->prepare();
        mPrimitiveBufferDirty = false;
    }

    // NOTE: it is important that textures are set before texture states since
    // some devices (e.g., OpenGL) set certain states on the texture.
    if( mTexturesDirty )
    {
        mTexturesDirty = false;
        for(U32 i = 0; i < getNumSamplers(); i++)
        {
            if(!mTextureDirty[i])
                continue;
            mTextureDirty[i] = false;

            switch (mTexType[i])
            {
                case GFXTDT_Normal :
                {
                    mCurrentTexture[i] = mNewTexture[i];
                    setTextureInternal(i, mCurrentTexture[i]);
                }
                    break;
                case GFXTDT_Cube :
                {
                    mCurrentCubemap[i] = mNewCubemap[i];
                    if (mCurrentCubemap[i])
                        mCurrentCubemap[i]->setToTexUnit(i);
                    else
                        setTextureInternal(i, NULL);
                }
                    break;
                default:
                AssertFatal(false, "Unknown texture type!");
                    break;
            }
        }
    }

    // Set render states
    while( mNumDirtyStates )
    {
        mNumDirtyStates--;
        U32 state = mTrackedState[mNumDirtyStates];

        // Added so that an unsupported state can be set to > Max render state
        // and thus be ignored, could possibly put in a call to the gfx device
        // to handle an unsupported call so it could log it, or do some workaround
        // or something? -pw
        if( state > GFXRenderState_COUNT )
            continue;

        if( mStateTracker[state].currentValue != mStateTracker[state].newValue )
        {
            setRenderState(state, mStateTracker[state].newValue);
            mStateTracker[state].currentValue = mStateTracker[state].newValue;
        }
        mStateTracker[state].dirty = false;
    }

    // Set texture states
    while( mNumDirtyTextureStates )
    {
        mNumDirtyTextureStates--;
        U32 state = mTextureTrackedState[mNumDirtyTextureStates].state;
        U32 stage = mTextureTrackedState[mNumDirtyTextureStates].stage;
        StateTracker &st = mTextureStateTracker[stage][state];
        st.dirty = false;
        if( st.currentValue != st.newValue )
        {
            setTextureStageState(stage, state, st.newValue);
            st.currentValue = st.newValue;
        }
    }

    // Set sampler states
    while( mNumDirtySamplerStates )
    {
        mNumDirtySamplerStates--;
        U32 state = mSamplerTrackedState[mNumDirtySamplerStates].state;
        U32 stage = mSamplerTrackedState[mNumDirtySamplerStates].stage;
        StateTracker &st = mSamplerStateTracker[stage][state];
        st.dirty = false;
        if( st.currentValue != st.newValue )
        {
            setSamplerState(stage, state, st.newValue);
            st.currentValue = st.newValue;
        }
    }

    // Set light material
    if(mLightMaterialDirty)
    {
        setLightMaterialInternal(mCurrentLightMaterial);
        mLightMaterialDirty = false;
    }

    // Set our lights
    if(mLightsDirty)
    {
        mLightsDirty = false;
        for(U32 i = 0; i < LIGHT_STAGE_COUNT; i++)
        {
            if(!mLightDirty[i])
                continue;

            mLightDirty[i] = false;
            setLightInternal(i, mCurrentLight[i], mCurrentLightEnable[i]);
        }
    }

    _updateRenderTargets();

#ifdef TORQUE_DEBUG_RENDER
    doParanoidStateCheck();
#endif
}

//-----------------------------------------------------------------------------

void GFXDevice::setPrimitiveBuffer(GFXPrimitiveBuffer* buffer)
{
    if (buffer == mCurrentPrimitiveBuffer)
        return;

    mCurrentPrimitiveBuffer = buffer;
    mPrimitiveBufferDirty = true;
    mStateDirty = true;
}

//-----------------------------------------------------------------------------

void GFXDevice::drawPrimitive(U32 primitiveIndex)
{
    if (mStateDirty)
        updateStates();

    AssertFatal(mCurrentPrimitiveBuffer.isValid(), "Trying to call drawPrimitive with no current primitive buffer, call setPrimitiveBuffer()");
    AssertFatal(primitiveIndex < mCurrentPrimitiveBuffer->mPrimitiveCount, "Out of range primitive index.");
    GFXPrimitive* info = &mCurrentPrimitiveBuffer->mPrimitiveArray[primitiveIndex];

    // Do NOT add index buffer offset to this call, it will be added by drawIndexedPrimitive
    drawIndexedPrimitive(info->type, info->minIndex, info->numVertices, info->startIndex, info->numPrimitives);
}

//-----------------------------------------------------------------------------

void GFXDevice::drawPrimitives()
{
    if (mStateDirty)
        updateStates();

    AssertFatal(mCurrentPrimitiveBuffer.isValid(), "Trying to call drawPrimitive with no current primitive buffer, call setPrimitiveBuffer()");

    GFXPrimitive* info = NULL;

    for (U32 i = 0; i < mCurrentPrimitiveBuffer->mPrimitiveCount; i++) {
        info = &mCurrentPrimitiveBuffer->mPrimitiveArray[i];

        // Do NOT add index buffer offset to this call, it will be added by drawIndexedPrimitive
        drawIndexedPrimitive(info->type, info->minIndex, info->numVertices, info->startIndex, info->numPrimitives);
    }
}

//-------------------------------------------------------------
// Console functions
//-------------------------------------------------------------
ConsoleFunction(getDisplayDeviceList, const char*, 1, 1, "Returns a tab-seperated string of the detected devices.")
{
    Vector<GFXAdapter*> adapters;
    GFXInit::getAdapters(&adapters);

    U32 deviceCount = adapters.size();
    if (deviceCount < 1)
        return NULL;

    U32 strLen = 0, i;
    for (i = 0; i < deviceCount; i++)
        strLen += (dStrlen(adapters[i]->mName) + 1);

    char* returnString = Con::getReturnBuffer(strLen);
    dStrcpy(returnString, adapters[0]->mName);
    for (i = 1; i < deviceCount; i++)
    {
        dStrcat(returnString, "\t");
        dStrcat(returnString, adapters[i]->mName);
    }

    return(returnString);
}

ConsoleFunction(getResolution, const char*, 1, 1, "Returns screen resolution in form \"width height bitdepth fullscreen\"")
{
    Point2I size = GFX->getVideoMode().resolution;
    S32 bitdepth = GFX->getVideoMode().bitDepth;
    bool fullscreen = GFX->getVideoMode().fullScreen;
    bool borderless = GFX->getVideoMode().borderless;

    S32 mode = 0;
    if (fullscreen)
        mode = 1;
    else if (borderless)
        mode = 2;

    char* buf = Con::getReturnBuffer(256);
    dSprintf(buf, 256, "%d %d %d %d", size.x, size.y, bitdepth, mode);
    return buf;
}

ConsoleFunction(getScreenMode, S32, 1, 1, "getScreenMode() Returns full screen mode (0 = windowed, 1 = fullscreen, 2 = borderless)")
{
    bool fullscreen = GFX->getVideoMode().fullScreen;
    bool borderless = GFX->getVideoMode().borderless;

    S32 mode = 0;
    if (fullscreen)
        mode = 1;
    else if (borderless)
        mode = 2;

    return mode;
}

ConsoleFunction(getDisplayDeviceName, const char*, 1, 1, "getDisplayDeviceName() Returns the name of the current display device")
{
    const char* deviceName = "D3D"; // Only dx9 is currently supported

    char* buf = Con::getReturnBuffer(256);
    dSprintf(buf, 256, "%s", deviceName);
    return buf;
}

ConsoleFunction(getResolutionList, const char*, 1, 2, "getResolutionList([minimumRes]) Returns a tab-seperated list of possible resolutions.")
{
    const Vector<GFXVideoMode>* const modelist = GFX->getVideoModeList();

    S32 minWidth = 0;
    S32 minHeight = 0;
    if (argc > 1)
        dSscanf(argv[1], "%d %d", &minWidth, &minHeight);

    U32 k;
    U32 stringLen = 0;
    for (k = 0; k < modelist->size(); k++)
    {
        AssertFatal((*modelist)[k].resolution.x < 10000 && (*modelist)[k].resolution.y < 10000,
            "My brain hurts with resolutions above 10k pixels. :(");

        if (minWidth > 0 && (*modelist)[k].resolution.x < minWidth)
            continue;
        if (minHeight > 0 && (*modelist)[k].resolution.y < minHeight)
            continue;
        if (k > 0
            && (*modelist)[k].resolution.x == (*modelist)[k-1].resolution.x
            && (*modelist)[k].resolution.y == (*modelist)[k-1].resolution.y
            && (*modelist)[k].bitDepth == (*modelist)[k-1].bitDepth)
            continue;

        if ((*modelist)[k].resolution.x >= 1000)
            stringLen += 5;//4 digits + space
        else
            stringLen += 4;//3 digits + space
        if ((*modelist)[k].resolution.y >= 1000)
            stringLen += 5;//4 digits + space
        else
            stringLen += 4;//3 digits + space
        stringLen += 3;//2 digits of bit depth + space (or terminator)

        //need something for refresh rate
    }

    char* returnString = Con::getReturnBuffer(stringLen);
    returnString[0] = '\0';  //have to null out the first byte so that we don't concatenate onto something that's already there
    for (k = 0; k < (*modelist).size(); k++)
    {
        if (minWidth > 0 && (*modelist)[k].resolution.x < minWidth)
            continue;
        if (minHeight > 0 && (*modelist)[k].resolution.y < minHeight)
            continue;
        if (k > 0
            && (*modelist)[k].resolution.x == (*modelist)[k-1].resolution.x
            && (*modelist)[k].resolution.y == (*modelist)[k-1].resolution.y
            && (*modelist)[k].bitDepth == (*modelist)[k-1].bitDepth)
            continue;

        if (SFXBBCOPY_SIZE > (*modelist)[k].resolution.x ||
            SFXBBCOPY_SIZE > (*modelist)[k].resolution.y)
        {
            continue;
        }

        char nextString[16];
        dSprintf(nextString, 16, "%d %d %d\t", (*modelist)[k].resolution.x, (*modelist)[k].resolution.y, (*modelist)[k].bitDepth);
        dStrcat(returnString, nextString);
    }

    return returnString;
}

ConsoleFunction(setVideoMode, void, 5, 6, "setVideoMode(width, height, bit depth, fullscreen, [antialiasLevel])")
{
    GFXVideoMode vm = GFX->getVideoMode();
    vm.resolution = Point2I(dAtoi(argv[1]), dAtoi(argv[2]));
    vm.bitDepth = dAtoi(argv[3]);

    const S32 fsType = dAtoi(argv[4]);

    vm.fullScreen = fsType == 1;
    vm.borderless = fsType == 2;

    if (argc > 5)
        vm.antialiasLevel = dAtoi(argv[5]);
    else
        vm.antialiasLevel = 0;
        
    GFX->setVideoMode(vm);

    if (vm.fullScreen || vm.borderless)
    {
        char tempBuf[15];
        dSprintf(tempBuf, sizeof(tempBuf), "%d %d %d", vm.resolution.x, vm.resolution.y, vm.bitDepth);
        Con::setVariable("$pref::Video::resolution", tempBuf);
    }
    else
    {
        char tempBuf[15];
        dSprintf(tempBuf, sizeof(tempBuf), "%d %d", vm.resolution.x, vm.resolution.y);
        Con::setVariable("$pref::Video::windowedRes", tempBuf);
    }
}

ConsoleFunction(updateVideoMode, void, 1, 1, "updateVideoMode()")
{
    GFXVideoMode vm = GFX->getVideoMode();

    const S32 fsType = Con::getIntVariable("$pref::Video::fullScreen");

    U32 w = 0, h = 0, d = 0;
    if (fsType > 0)
    {
        const char* fsRes = Con::getVariable("$pref::Video::resolution");
        const S32 fsResLen = dStrlen(fsRes);

        if (fsResLen)
            dSscanf(fsRes, "%d %d %d", &w, &h, &d);
        else
        {
            w = 800;
            h = 600;
            d = 32;
        }
    } else
    {
        const char* winRes = Con::getVariable("$pref::Video::windowedRes");
        const S32 winResLen = dStrlen(winRes);
        if (winResLen)
        {
            dSscanf(winRes, "%d %d", &w, &h);
            d = 32;
        } else {
            const char* fsRes = Con::getVariable("$pref::Video::resolution");
            const S32 fsResLen = dStrlen(fsRes);

            if (fsResLen)
                dSscanf(fsRes, "%d %d %d", &w, &h, &d);
            else
            {
                w = 800;
                h = 600;
                d = 32;
            }
        }
    }

    vm.resolution.x = w;
    vm.resolution.y = h;
    vm.bitDepth = d;

    vm.fullScreen = fsType == 1;
    vm.borderless = fsType == 2;

    int antialiasLevel = Con::getIntVariable("$pref::Video::FSAALevel", 0);
    vm.antialiasLevel = antialiasLevel;

    GFX->setVideoMode(vm);
}

ConsoleFunction(toggleFullScreen, void, 1, 1, "toggles between windowed and fullscreen mode. toggleFullscreen()")
{
    GFXVideoMode vm = GFX->getVideoMode();
    vm.fullScreen = !vm.fullScreen;

    U32 w = 0, h = 0, d = 0;

    if (vm.fullScreen)
        dSscanf(Con::getVariable("$pref::Video::resolution"), "%d %d %d", &w, &h, &d);
    else
    {
        dSscanf(Con::getVariable("$pref::Video::windowedRes"), "%d %d", &w, &h);
        if (w == 0 || h == 0)
            dSscanf(Con::getVariable("$pref::Video::resolution"), "%d %d %d", &w, &h, &d);
        else
            d = 32;
    }

    if (w == 0 || h == 0 || d == 0)
    {
        w = 640;
        h = 480;
        d = 32;
    }

    vm.resolution.x = w;
    vm.resolution.y = h;
    vm.bitDepth = d;

    int antialiasLevel = Con::getIntVariable("$pref::Video::FSAALevel", 0);
    vm.antialiasLevel = antialiasLevel;

    GFX->setVideoMode(vm);
}

//-----------------------------------------------------------------------------

#define TEXT_MAG 1

struct FontRenderBatcher
{
    struct CharMarker
    {
        U32 c;
        F32 x;
        GFXVertexColor color;
        PlatformFont::CharInfo* ci;
    };

    struct SheetMarker
    {
        U32 numChars;
        U32 startVertex;
        CharMarker charIndex[0];
    };

    FrameAllocatorMarker mAllocator;

    static Vector<SheetMarker*> smSheets;
    GFont* mFont;
    U32 mLength;

    SheetMarker& getSheetMarker(U32 sheetID)
    {
        // Allocate if it doesn't exist...
        if (!smSheets[sheetID])
        {
            if (sheetID >= smSheets.size())
                smSheets.setSize(sheetID + 1);

            smSheets[sheetID] = (SheetMarker*)mAllocator.alloc(
                sizeof(SheetMarker) + mLength * sizeof(CharMarker));
            constructInPlace<SheetMarker>(smSheets[sheetID]);
            smSheets[sheetID]->numChars = 0;
        }

        return *smSheets[sheetID];
    }

    void init(GFont* font, U32 n)
    {
        // Null everything and reset our count.
        dMemset(smSheets.address(), 0, smSheets.memSize());
        smSheets.clear();

        mFont = font;
        mLength = n;
    }

    void queueChar(UTF16 c, S32& currentX, GFXVertexColor& currentColor)
    {
        const PlatformFont::CharInfo& ci = mFont->getCharInfo(c);
        U32 sidx = ci.bitmapIndex;

        if (ci.width != 0 && ci.height != 0)
        {
            SheetMarker& sm = getSheetMarker(sidx);

            CharMarker& m = sm.charIndex[sm.numChars];
            sm.numChars++;

            m.c = c;
            m.x = currentX;
            m.color = currentColor;
        }

        currentX += ci.xIncrement;
    }

    void render(F32 rot, F32 y)
    {
        if( mLength == 0 )
            return;

        GFX->disableShaders();
        GFX->setLightingEnable(false);
        GFX->setCullMode(GFXCullNone);
        GFX->setAlphaBlendEnable(true);
        GFX->setSrcBlend(GFXBlendSrcAlpha);
        GFX->setDestBlend(GFXBlendInvSrcAlpha);
        GFX->setTextureStageMagFilter(0, GFXTextureFilterPoint);
        GFX->setTextureStageMinFilter(0, GFXTextureFilterPoint);
        GFX->setTextureStageAddressModeU(0, GFXAddressClamp);
        GFX->setTextureStageAddressModeV(0, GFXAddressClamp);

        GFX->setTextureStageAlphaOp( 0, GFXTOPModulate );
        GFX->setTextureStageAlphaOp( 1, GFXTOPDisable );
        GFX->setTextureStageAlphaArg1( 0, GFXTATexture );
        GFX->setTextureStageAlphaArg2( 0, GFXTADiffuse );

        // This is an add operation because in D3D, when a texture of format D3DFMT_A8
        // is used, the RGB channels are all set to 0.  Therefore a modulate would 
        // result in the text always being black.  This may not be the case in OpenGL
        // so it may have to change.  -bramage
        GFX->setTextureStageColorOp(0, GFXTOPAdd);
        GFX->setTextureStageColorOp(1, GFXTOPDisable);

        MatrixF rotMatrix;

        bool doRotation = rot != 0.f;
        if (doRotation)
            rotMatrix.set(EulerF(0.0, 0.0, mDegToRad(rot)));

        // Write verts out.
        U32 currentPt = 0;
        GFXVertexBufferHandle<GFXVertexPCT> verts(GFX, mLength * 6, GFXBufferTypeVolatile);
        verts.lock();

        for (S32 i = 0; i < smSheets.size(); i++)
        {
            // Do some early outs...
            if (!smSheets[i])
                continue;

            if (!smSheets[i]->numChars)
                continue;

            smSheets[i]->startVertex = currentPt;
            const GFXTextureObject* tex = mFont->getTextureHandle(i);

            for (S32 j = 0; j < smSheets[i]->numChars; j++)
            {
                // Get some general info to proceed with...
                const CharMarker& m = smSheets[i]->charIndex[j];
                const PlatformFont::CharInfo& ci = mFont->getCharInfo(m.c);

                // Where are we drawing it?
                F32 drawY = y + mFont->getBaseline() - ci.yOrigin * TEXT_MAG;
                F32 drawX = m.x + ci.xOrigin;

                // Figure some values.
                const F32 texLeft = (F32)(ci.xOffset) / (F32)(tex->getWidth());
                const F32 texRight = (F32)(ci.xOffset + ci.width) / (F32)(tex->getWidth());
                const F32 texTop = (F32)(ci.yOffset) / (F32)(tex->getHeight());
                const F32 texBottom = (F32)(ci.yOffset + ci.height) / (F32)(tex->getHeight());

                const F32 screenLeft = drawX - GFX->getFillConventionOffset();
                const F32 screenRight = drawX - GFX->getFillConventionOffset() + ci.width * TEXT_MAG;
                const F32 screenTop = drawY - GFX->getFillConventionOffset();
                const F32 screenBottom = drawY - GFX->getFillConventionOffset() + ci.height * TEXT_MAG;

                // Build our vertices. We NEVER read back from the buffer, that's
                // incredibly slow, so for rotation do it into tmp. This code is
                // ugly as sin.
                Point3F tmp;

                tmp.set(screenLeft, screenTop, 0.f);
                if (doRotation)
                    rotMatrix.mulP(tmp, &verts[currentPt].point);
                else
                    verts[currentPt].point = tmp;
                verts[currentPt].color = m.color;
                verts[currentPt].texCoord.set(texLeft, texTop);
                currentPt++;

                tmp.set(screenLeft, screenBottom, 0.f);
                if (doRotation)
                    rotMatrix.mulP(tmp, &verts[currentPt].point);
                else
                    verts[currentPt].point = tmp;
                verts[currentPt].color = m.color;
                verts[currentPt].texCoord.set(texLeft, texBottom);
                currentPt++;

                tmp.set(screenRight, screenBottom, 0.f);
                if (doRotation)
                    rotMatrix.mulP(tmp, &verts[currentPt].point);
                else
                    verts[currentPt].point = tmp;
                verts[currentPt].color = m.color;
                verts[currentPt].texCoord.set(texRight, texBottom);
                currentPt++;

                tmp.set(screenRight, screenBottom, 0.f);
                if (doRotation)
                    rotMatrix.mulP(tmp, &verts[currentPt].point);
                else
                    verts[currentPt].point = tmp;
                verts[currentPt].color = m.color;
                verts[currentPt].texCoord.set(texRight, texBottom);
                currentPt++;

                tmp.set(screenRight, screenTop, 0.f);
                if (doRotation)
                    rotMatrix.mulP(tmp, &verts[currentPt].point);
                else
                    verts[currentPt].point = tmp;
                verts[currentPt].color = m.color;
                verts[currentPt].texCoord.set(texRight, texTop);
                currentPt++;

                tmp.set(screenLeft, screenTop, 0.f);
                if (doRotation)
                    rotMatrix.mulP(tmp, &verts[currentPt].point);
                else
                    verts[currentPt].point = tmp;
                verts[currentPt].color = m.color;
                verts[currentPt].texCoord.set(texLeft, texTop);
                currentPt++;
            }
        }

        verts->unlock();

        AssertFatal(currentPt <= mLength * 6, "FontRenderBatcher::render - too many verts for length of string!");

        GFX->setVertexBuffer(verts);

        // Now do an optimal render!
        for (S32 i = 0; i < smSheets.size(); i++)
        {
            if (!smSheets[i])
                continue;

            if (!smSheets[i]->numChars)
                continue;

            GFX->setupGenericShaders(GFXDevice::GSAddColorTexture);
            GFX->setTexture(0, mFont->getTextureHandle(i));
            GFX->drawPrimitive(GFXTriangleList, smSheets[i]->startVertex, smSheets[i]->numChars * 2);
        }

        // Cleanup!
        GFX->setVertexBuffer(NULL);
        GFX->setTexture(0, NULL);
        GFX->setAlphaBlendEnable(false);
        GFX->setTextureStageColorOp(0, GFXTOPModulate);
        GFX->setTextureStageColorOp(1, GFXTOPDisable);
    }
};

Vector<FontRenderBatcher::SheetMarker*> FontRenderBatcher::smSheets(64);

U32 GFXDevice::drawTextN(GFont* font, const Point2I& ptDraw, const UTF16* in_string,
    U32 n, const ColorI* colorTable, const U32 maxColorIndex, F32 rot)
{
    // return on zero length strings
    if (n < 1)
        return ptDraw.x;

    // If it's over about 4000 verts we want to break it up
    if (n > 666)
    {
        U32 left = drawTextN(font, ptDraw, in_string, 666, colorTable, maxColorIndex, rot);

        Point2I newDrawPt(left, ptDraw.y);
        const UTF16* str = (const UTF16*)in_string;

        return drawTextN(font, newDrawPt, &(str[666]), n - 666, colorTable, maxColorIndex, rot);
    }

    PROFILE_START(GFXDevice_drawTextN);

    U32 nCharCount = 0;
    Point2I     pt = ptDraw;
    UTF16 c;
    GFXVertexColor currentColor;
    GFXTextureObject* lastTexture = NULL;

    currentColor = mBitmapModulation;

    // Queue everything for render.
    FontRenderBatcher frb;
    frb.init(font, n);

    U32 i;
    for (i = 0, c = in_string[i];
        in_string[i] && i < n;
        i++, c = in_string[i])
    {
        // We have to do a little dance here since \t = 0x9, \n = 0xa, and \r = 0xd
        if ((c >= 1 && c <= 7) ||
            (c >= 11 && c <= 12) ||
            (c == 14))
        {
            // Color code
            if (colorTable)
            {
                static U8 remap[15] =
                {
                   0x0,
                   0x0,
                   0x1,
                   0x2,
                   0x3,
                   0x4,
                   0x5,
                   0x6,
                   0x0,
                   0x0,
                   0x0,
                   0x7,
                   0x8,
                   0x0,
                   0x9
                };

                U8 remapped = remap[c];

                // Ignore if the color is greater than the specified max index:
                if (remapped <= maxColorIndex)
                {
                    const ColorI& clr = colorTable[remapped];
                    currentColor = mBitmapModulation = clr;
                }
            }

            // And skip rendering this character.
            continue;
        }
        // reset color?
        else if (c == 15)
        {
            currentColor = mBitmapModulation = mTextAnchorColor;

            // And skip rendering this character.
            continue;
        }
        // push color:
        else if (c == 16)
        {
            mTextAnchorColor = mBitmapModulation;

            // And skip rendering this character.
            continue;
        }
        // pop color:
        else if (c == 17)
        {
            currentColor = mBitmapModulation = mTextAnchorColor;

            // And skip rendering this character.
            continue;
        }
        // Tab character
        else if (c == dT('\t'))
        {
            const PlatformFont::CharInfo& ci = font->getCharInfo(dT(' '));
            pt.x += ci.xIncrement * GFont::TabWidthInSpaces;

            // And skip rendering this character.
            continue;
        }
        // Don't draw invalid characters.
        else if (!font->isValidChar(c))
            continue;

        // Queue char for rendering..
        frb.queueChar(c, pt.x, currentColor);
    }


    frb.render(rot, ptDraw.y);

    PROFILE_END();

    return pt.x - ptDraw.x;
}

//------------------------------------------------------------------------------

void GFXDevice::drawBitmapStretchSR(GFXTextureObject* texture, const RectI& dstRect, const RectI& srcRect, const GFXBitmapFlip in_flip)
{
    setBaseRenderState();

    GFXVertexBufferHandle<GFXVertexPCT> verts(GFX, 4, GFXBufferTypeVolatile);
    verts.lock();

    F32 texLeft = F32(srcRect.point.x) / F32(texture->mTextureSize.x);
    F32 texRight = F32(srcRect.point.x + srcRect.extent.x) / F32(texture->mTextureSize.x);
    F32 texTop = F32(srcRect.point.y) / F32(texture->mTextureSize.y);
    F32 texBottom = F32(srcRect.point.y + srcRect.extent.y) / F32(texture->mTextureSize.y);

    F32 screenLeft = dstRect.point.x;
    F32 screenRight = dstRect.point.x + dstRect.extent.x;
    F32 screenTop = dstRect.point.y;
    F32 screenBottom = dstRect.point.y + dstRect.extent.y;

    if (in_flip & GFXBitmapFlip_X)
    {
        F32 temp = texLeft;
        texLeft = texRight;
        texRight = temp;
    }
    if (in_flip & GFXBitmapFlip_Y)
    {
        F32 temp = texTop;
        texTop = texBottom;
        texBottom = temp;
    }

    verts[0].point.set(screenLeft - getFillConventionOffset(), screenTop - getFillConventionOffset(), 0.f);
    verts[1].point.set(screenRight - getFillConventionOffset(), screenTop - getFillConventionOffset(), 0.f);
    verts[2].point.set(screenLeft - getFillConventionOffset(), screenBottom - getFillConventionOffset(), 0.f);
    verts[3].point.set(screenRight - getFillConventionOffset(), screenBottom - getFillConventionOffset(), 0.f);

    verts[0].color = verts[1].color = verts[2].color = verts[3].color = mBitmapModulation;

    verts[0].texCoord.set(texLeft, texTop);
    verts[1].texCoord.set(texRight, texTop);
    verts[2].texCoord.set(texLeft, texBottom);
    verts[3].texCoord.set(texRight, texBottom);

    verts.unlock();

    setVertexBuffer(verts);

    setCullMode(GFXCullNone);
    setLightingEnable(false);
    setAlphaBlendEnable(true);
    setSrcBlend(GFXBlendSrcAlpha);
    setDestBlend(GFXBlendInvSrcAlpha);

    setTextureStageColorOp(0, GFXTOPModulate);
    setTextureStageColorOp(1, GFXTOPDisable);
    setTexture(0, texture);
    setupGenericShaders(GSModColorTexture);

    drawPrimitive(GFXTriangleStrip, 0, 2);

    setAlphaBlendEnable(false);
}

void GFXDevice::drawBitmapStretchSR(GFXTextureObject* texture, const RectF& dstRect, const RectF& srcRect, const GFXBitmapFlip in_flip)
{
    setBaseRenderState();

    GFXVertexBufferHandle<GFXVertexPCT> verts(GFX, 4, GFXBufferTypeVolatile);
    verts.lock();

    F32 texLeft = (srcRect.point.x) / (texture->mTextureSize.x);
    F32 texRight = (srcRect.point.x + srcRect.extent.x) / F32(texture->mTextureSize.x);
    F32 texTop = (srcRect.point.y) / (texture->mTextureSize.y);
    F32 texBottom = (srcRect.point.y + srcRect.extent.y) / (texture->mTextureSize.y);

    F32 screenLeft = dstRect.point.x;
    F32 screenRight = dstRect.point.x + dstRect.extent.x;
    F32 screenTop = dstRect.point.y;
    F32 screenBottom = dstRect.point.y + dstRect.extent.y;

    if (in_flip & GFXBitmapFlip_X)
    {
        F32 temp = texLeft;
        texLeft = texRight;
        texRight = temp;
    }
    if (in_flip & GFXBitmapFlip_Y)
    {
        F32 temp = texTop;
        texTop = texBottom;
        texBottom = temp;
    }

    verts[0].point.set(screenLeft - getFillConventionOffset(), screenTop - getFillConventionOffset(), 0.f);
    verts[1].point.set(screenRight - getFillConventionOffset(), screenTop - getFillConventionOffset(), 0.f);
    verts[2].point.set(screenLeft - getFillConventionOffset(), screenBottom - getFillConventionOffset(), 0.f);
    verts[3].point.set(screenRight - getFillConventionOffset(), screenBottom - getFillConventionOffset(), 0.f);

    verts[0].color = verts[1].color = verts[2].color = verts[3].color = mBitmapModulation;

    verts[0].texCoord.set(texLeft, texTop);
    verts[1].texCoord.set(texRight, texTop);
    verts[2].texCoord.set(texLeft, texBottom);
    verts[3].texCoord.set(texRight, texBottom);

    verts.unlock();

    setVertexBuffer(verts);

    setCullMode(GFXCullNone);
    setLightingEnable(false);
    setAlphaBlendEnable(true);
    setSrcBlend(GFXBlendSrcAlpha);
    setDestBlend(GFXBlendInvSrcAlpha);

    setTextureStageColorOp(0, GFXTOPModulate);
    setTextureStageColorOp(1, GFXTOPDisable);
    setTexture(0, texture);
    setupGenericShaders(GSModColorTexture);

    drawPrimitive(GFXTriangleStrip, 0, 2);

    setAlphaBlendEnable(false);
}

void GFXDevice::drawRectFill(const Point2I& a, const Point2I& b, const ColorI& color)
{
    setBaseRenderState();

    //
    // Convert Box   a----------x
    //               |          |
     //               x----------b
    // Into Quad
     //               v0---------v1
    //               | a       x |
     //					  |           |
     //               | x       b |
    //               v2---------v3
    //

     // NorthWest and NorthEast facing offset vectors
    Point2F nw(-0.5f, -0.5f); /*  \  */
    Point2F ne(+0.5f, -0.5f); /*  /  */

    GFXVertexBufferHandle<GFXVertexPC> verts(GFX, 4, GFXBufferTypeVolatile);
    verts.lock();

    verts[0].point.set(a.x + nw.x, a.y + nw.y, 0.0f);
    verts[1].point.set(b.x + ne.x, a.y + ne.y, 0.0f);
    verts[2].point.set(a.x - ne.x, b.y - ne.y, 0.0f);
    verts[3].point.set(b.x - nw.x, b.y - nw.y, 0.0f);

    for (int i = 0; i < 4; i++)
        verts[i].color = color;

    verts.unlock();

    setCullMode(GFXCullNone);
    setAlphaBlendEnable(true);
    setLightingEnable(false);
    setSrcBlend(GFXBlendSrcAlpha);
    setDestBlend(GFXBlendInvSrcAlpha);
    setTextureStageColorOp(0, GFXTOPDisable);

    setVertexBuffer(verts);
    setupGenericShaders();

    drawPrimitive(GFXTriangleStrip, 0, 2);

    setTextureStageColorOp(0, GFXTOPModulate);
    setAlphaBlendEnable(false);
}

void GFXDevice::drawRect(const Point2I& a, const Point2I& b, const ColorI& color)
{
    setBaseRenderState();

    //
    // Convert Box   a----------x
    //               |          |
     //               x----------b
     //
    // Into tri-Strip Outline
     //               v1-----------v2
    //               | a         x |
     //					  |  v0-----v3  |
     //               |   |     |   |
     //               |  v7-----v5  |
     //               | x         b |
    //               v6-----------v4
    //

     // NorthWest and NorthEast facing offset vectors
    Point2F nw(-0.5f, -0.5f); /*  \  */
    Point2F ne(+0.5f, -0.5f); /*  /  */

    GFXVertexBufferHandle<GFXVertexPC> verts(GFX, 10, GFXBufferTypeVolatile);
    verts.lock();

    verts[0].point.set(a.x - nw.x, a.y - nw.y, 0.0f);
    verts[1].point.set(a.x + nw.x, a.y + nw.y, 0.0f);
    verts[2].point.set(b.x + ne.x, a.y + ne.y, 0.0f);
    verts[3].point.set(b.x - ne.x, a.y - ne.y, 0.0f);
    verts[4].point.set(b.x - nw.x, b.y - nw.y, 0.0f);
    verts[5].point.set(b.x + nw.x, b.y + nw.y, 0.0f);
    verts[6].point.set(a.x - ne.x, b.y - ne.y, 0.0f);
    verts[7].point.set(a.x + ne.x, b.y + ne.y, 0.0f);
    verts[8].point.set(a.x + nw.x, a.y + nw.y, 0.0f); // same as 1
    verts[9].point.set(a.x - nw.x, a.y - nw.y, 0.0f); // same as 0

    for (int i = 0; i < 10; i++)
        verts[i].color = color;

    verts.unlock();
    setVertexBuffer(verts);

    setCullMode(GFXCullNone);
    setAlphaBlendEnable(true);
    setLightingEnable(false);
    setSrcBlend(GFXBlendSrcAlpha);
    setDestBlend(GFXBlendInvSrcAlpha);
    setTextureStageColorOp(0, GFXTOPDisable);
    setupGenericShaders();

    drawPrimitive(GFXTriangleStrip, 0, 8);

    setTextureStageColorOp(0, GFXTOPModulate);
    setAlphaBlendEnable(false);
}

void GFXDevice::draw2DSquare(const Point2F& screenPoint, F32 width, F32 spinAngle)
{
    setBaseRenderState();

    width *= 0.5;

    MatrixF rotMatrix(EulerF(0.0, 0.0, spinAngle));

    Point3F offset(screenPoint.x, screenPoint.y, 0.0);

    GFXVertexBufferHandle<GFXVertexPC> verts(GFX, 4, GFXBufferTypeVolatile);
    verts.lock();

    verts[0].point.set(-width, -width, 0.0f);
    verts[1].point.set(-width, width, 0.0f);
    verts[2].point.set(width, width, 0.0f);
    verts[3].point.set(width, -width, 0.0f);

    verts[0].color = verts[1].color = verts[2].color = verts[3].color = mBitmapModulation;

    //verts[0].texCoord.set( 0.f, 0.f );
    //verts[1].texCoord.set( 0.f, 1.f );
    //verts[2].texCoord.set( 1.f, 1.f );
    //verts[3].texCoord.set( 1.f, 0.f );

    for (int i = 0; i < 4; i++)
    {
        rotMatrix.mulP(verts[i].point);
        verts[i].point += offset;
    }

    verts.unlock();
    setVertexBuffer(verts);

    setCullMode(GFXCullNone);
    setAlphaBlendEnable(true);
    setLightingEnable(false);
    setSrcBlend(GFXBlendSrcAlpha);
    setDestBlend(GFXBlendInvSrcAlpha);
    setTextureStageColorOp(0, GFXTOPDisable);
    setupGenericShaders();

    drawPrimitive(GFXTriangleStrip, 0, 2);

    setTextureStageColorOp(0, GFXTOPModulate);
    setAlphaBlendEnable(false);
}


void GFXDevice::drawLine(S32 x1, S32 y1, S32 x2, S32 y2, const ColorI& color)
{
    setBaseRenderState();

    //
    // Convert Line   a----------b
    //
    // Into Quad      v0---------v1
    //                 a         b
    //                v2---------v3
    //

    Point2F start(x1, y1);
    Point2F end(x2, y2);
    Point2F perp, lineVec;

    // handle degenerate case where point a = b
    if (x1 == x2 && y1 == y2)
    {
        perp.set(0, .5);
        lineVec.set(0.1, 0);
    }
    else
    {
        perp.set(start.y - end.y, end.x - start.x);
        lineVec.set(end.x - start.x, end.y - start.y);
        perp.normalize(0.5);
        lineVec.normalize(0.1);
    }
    start -= lineVec;
    end += lineVec;

    GFXVertexBufferHandle<GFXVertexPC> verts(GFX, 4, GFXBufferTypeVolatile);
    verts.lock();

    verts[0].point.set(start.x + perp.x, start.y + perp.y, 0.0f);
    verts[1].point.set(end.x + perp.x, end.y + perp.y, 0.0f);
    verts[2].point.set(start.x - perp.x, start.y - perp.y, 0.0f);
    verts[3].point.set(end.x - perp.x, end.y - perp.y, 0.0f);

    verts[0].color = color;
    verts[1].color = color;
    verts[2].color = color;
    verts[3].color = color;

    verts.unlock();
    setVertexBuffer(verts);

    setCullMode(GFXCullNone);
    setAlphaBlendEnable(true);
    setLightingEnable(false);
    setSrcBlend(GFXBlendSrcAlpha);
    setDestBlend(GFXBlendInvSrcAlpha);
    setTextureStageColorOp(0, GFXTOPDisable);
    setupGenericShaders();

    drawPrimitive(GFXTriangleStrip, 0, 2);

    setTextureStageColorOp(0, GFXTOPModulate);
    setAlphaBlendEnable(false);
}


//-----------------------------------------------------------------------------
// Draw wire cube - use dynamic buffer until refcounting in.
//-----------------------------------------------------------------------------
static Point3F cubePoints[8] = {
   Point3F(-1, -1, -1), Point3F(-1, -1,  1), Point3F(-1,  1, -1), Point3F(-1,  1,  1),
   Point3F(1, -1, -1), Point3F(1, -1,  1), Point3F(1,  1, -1), Point3F(1,  1,  1)
};

static U32 cubeFaces[6][4] = {
   { 0, 2, 6, 4 }, { 0, 2, 3, 1 }, { 0, 1, 5, 4 },
   { 3, 2, 6, 7 }, { 7, 6, 4, 5 }, { 3, 7, 5, 1 }
};

void GFXDevice::drawWireCube(const Point3F& size, const Point3F& pos, const ColorI& color)
{
    setBaseRenderState();

    GFXVertexBufferHandle<GFXVertexPC> verts(GFX, 30, GFXBufferTypeVolatile);
    verts.lock();

    // setup 6 line loops
    U32 vertexIndex = 0;
    for (int i = 0; i < 6; i++)
    {
        for (int j = 0; j < 5; j++)
        {
            int idx = cubeFaces[i][j % 4];

            verts[vertexIndex].point = cubePoints[idx] * size + pos;
            verts[vertexIndex].color = color;
            vertexIndex++;
        }
    }

    verts.unlock();

    setCullMode(GFXCullNone);
    setAlphaBlendEnable(false);
    setTextureStageColorOp(0, GFXTOPDisable);

    setVertexBuffer(verts);
    setupGenericShaders();

    for (U32 i = 0; i < 6; i++)
    {
        drawPrimitive(GFXLineStrip, i * 5, 4);
    }

}

//-----------------------------------------------------------------------------
// Frustum values
//-----------------------------------------------------------------------------
F32 fLeft = 0.0, fRight = 0.0;
F32 fBottom = 0.0, fTop = 0.0;
F32 fNear = 0.0, fFar = 0.0;
bool fOrtho = false;

//-----------------------------------------------------------------------------
// Set projection frustum
//-----------------------------------------------------------------------------
void GFXDevice::setFrustum(F32 left,
    F32 right,
    F32 bottom,
    F32 top,
    F32 nearPlane,
    F32 farPlane,
    bool bRotate)
{
    // store values
    fLeft = left;
    fRight = right;
    fBottom = bottom;
    fTop = top;
    fNear = nearPlane;
    fFar = farPlane;
    fOrtho = false;

    // compute matrix
    MatrixF projection;

    Point4F row;
    row.x = 2.0 * nearPlane / (right - left);
    row.y = 0.0;
    row.z = 0.0;
    row.w = 0.0;
    projection.setRow(0, row);

    row.x = 0.0;
    row.y = 2.0 * nearPlane / (top - bottom);
    row.z = 0.0;
    row.w = 0.0;
    projection.setRow(1, row);

    row.x = (left + right) / (right - left);
    row.y = (top + bottom) / (top - bottom);
    row.z = farPlane / (nearPlane - farPlane);
    row.w = -1.0;
    projection.setRow(2, row);

    row.x = 0.0;
    row.y = 0.0;
    row.z = nearPlane * farPlane / (nearPlane - farPlane);
    row.w = 0.0;
    projection.setRow(3, row);

    projection.transpose();

    if (bRotate)
    {
        static MatrixF rotMat(EulerF((M_PI / 2.0), 0.0, 0.0));
        projection.mul(rotMat);
    }
    setProjectionMatrix(projection);
}


//-----------------------------------------------------------------------------
// Get projection frustum
//-----------------------------------------------------------------------------
void GFXDevice::getFrustum(F32* left, F32* right, F32* bottom, F32* top, F32* nearPlane, F32* farPlane)
{
    *left = fLeft;
    *right = fRight;
    *bottom = fBottom;
    *top = fTop;
    *nearPlane = fNear;
    *farPlane = fFar;
}

//-----------------------------------------------------------------------------
// Set frustum using FOV (Field of view) in degrees along the horizontal axis
//-----------------------------------------------------------------------------
void GFXDevice::setFrustum(F32 FOVx, F32 aspectRatio, F32 nearPlane, F32 farPlane)
{
    // Figure our planes and pass it up.

    //b = a tan D
    F32 left = -nearPlane * mTan(mDegToRad(FOVx) / 2.0);
    F32 right = -left;
    F32 top = left / aspectRatio;
    F32 bottom = -top;

    setFrustum(left, right, top, bottom, nearPlane, farPlane);

    return;
}

//-----------------------------------------------------------------------------
// Set projection matrix to ortho transform
//-----------------------------------------------------------------------------
void GFXDevice::setOrtho(F32 left,
    F32 right,
    F32 bottom,
    F32 top,
    F32 nearPlane,
    F32 farPlane,
    bool doRotate)
{
    // store values
    fLeft = left;
    fRight = right;
    fBottom = bottom;
    fTop = top;
    fNear = nearPlane;
    fFar = farPlane;
    fOrtho = true;

    // compute matrix
    MatrixF projection;

    Point4F row;

    row.x = 2.0f / (right - left);
    row.y = 0.0;
    row.z = 0.0;
    row.w = 0.0;
    projection.setRow(0, row);

    row.x = 0.0;
    row.y = 2.0f / (top - bottom);
    row.z = 0.0;
    row.w = 0.0;
    projection.setRow(1, row);

    row.x = 0.0;
    row.y = 0.0;
    // This may need be modified to work with OpenGL (d3d has 0..1 projection for z, vs -1..1 in OpenGL)
    row.z = 1.0f / (nearPlane - farPlane);
    row.w = 0.0;
    projection.setRow(2, row);

    row.x = (left + right) / (left - right);
    row.y = (top + bottom) / (bottom - top);
    row.z = nearPlane / (nearPlane - farPlane);
    row.w = 1;
    projection.setRow(3, row);

    projection.transpose();

    static MatrixF rotMat(EulerF((M_PI / 2.0), 0.0, 0.0));

    if (doRotate)
        projection.mul(rotMat);

    setProjectionMatrix(projection);
}


//-----------------------------------------------------------------------------
// Project radius
//-----------------------------------------------------------------------------
F32 GFXDevice::projectRadius(F32 dist, F32 radius)
{
    RectI viewPort = getViewport();
    F32 worldToScreenScale;
    if (!fOrtho)
        worldToScreenScale = (fNear * viewPort.extent.x) / (fRight - fLeft);
    else
        worldToScreenScale = viewPort.extent.x / (fRight - fLeft);
    return (radius / dist) * worldToScreenScale;
}

//-----------------------------------------------------------------------------
// Set base state
//-----------------------------------------------------------------------------
void GFXDevice::setBaseRenderState()
{
    setAlphaBlendEnable(false);
    setCullMode(GFXCullNone);
    setLightingEnable(false);
    setZWriteEnable(true);
    setTextureStageColorOp(0, GFXTOPDisable);
    disableShaders();
}

//-----------------------------------------------------------------------------
// Set Light
//-----------------------------------------------------------------------------
void GFXDevice::setLight(U32 stage, LightInfo* light)
{
   AssertFatal(stage < LIGHT_STAGE_COUNT, "GFXDevice::setLight - out of range stage!");

   if(!mLightDirty[stage])
   {
      mStateDirty = true;
      mLightsDirty = true;
      mLightDirty[stage] = true;
   }
   mCurrentLightEnable[stage] = (light != NULL);
   if(mCurrentLightEnable[stage])
      mCurrentLight[stage] = *light;
}

//-----------------------------------------------------------------------------
// Set Light Material
//-----------------------------------------------------------------------------
void GFXDevice::setLightMaterial(GFXLightMaterial mat)
{
   mCurrentLightMaterial = mat;
   mLightMaterialDirty = true;
   mStateDirty = true;
}

void GFXDevice::setGlobalAmbientColor(ColorF color)
{
   if(mGlobalAmbientColor != color)
   {
      mGlobalAmbientColor = color;
      mGlobalAmbientColorDirty = true;
   }
}

//-----------------------------------------------------------------------------
// Set texture
//-----------------------------------------------------------------------------
void GFXDevice::setTexture(U32 stage, GFXTextureObject* texture)
{
    AssertFatal(stage < getNumSamplers(), "GFXDevice::setTexture - out of range stage!");

    if( mCurrentTexture[stage].getPointer() == texture )
    {
        mTextureDirty[stage] = false;
        return;
    }

    if( !mTextureDirty[stage] )
    {
        mStateDirty = true;
        mTexturesDirty = true;
        mTextureDirty[stage] = true;
    }
    mNewTexture[stage] = texture;
    mTexType[stage] = GFXTDT_Normal;

    // Clear out the cubemaps
    mNewCubemap[stage] = NULL;
    mCurrentCubemap[stage] = NULL;
}

//-----------------------------------------------------------------------------
// Set cube texture
//-----------------------------------------------------------------------------
void GFXDevice::setCubeTexture( U32 stage, GFXCubemap *texture )
{
    AssertFatal(stage < getNumSamplers(), "GFXDevice::setTexture - out of range stage!");

    if( mCurrentCubemap[stage].getPointer() == texture )
    {
        mTextureDirty[stage] = false;
        return;
    }

    if( !mTextureDirty[stage] )
    {
        mStateDirty = true;
        mTexturesDirty = true;
        mTextureDirty[stage] = true;
    }
    mNewCubemap[stage] = texture;
    mTexType[stage] = GFXTDT_Cube;

    // Clear out the normal textures
    mNewTexture[stage] = NULL;
    mCurrentTexture[stage] = NULL;
}

//-----------------------------------------------------------------------------
// Register texture callback
//-----------------------------------------------------------------------------
void GFXDevice::registerTexCallback(GFXTexEventCallback callback,
    void* userData,
    S32& handle)
{
    mTextureManager->registerTexCallback(callback, userData, handle);
}

//-----------------------------------------------------------------------------
// unregister texture callback
//-----------------------------------------------------------------------------
void GFXDevice::unregisterTexCallback(S32 handle)
{
    mTextureManager->unregisterTexCallback(handle);
}
//------------------------------------------------------------------------------

void GFXDevice::setInitialGFXState()
{
    // Implementation plan:
    // Create macro which respects debug state #define, sets the proper render state
    // and creates a no-break watch on that state so that a report may be run at
    // end of frame. Repeat process for sampler/texture stage states.

    // Future work:
    // Some simple "red flag" logic should be pretty trivial to implement here that
    // could warn about a file-name setting a state and not restoring it's value

    // CodeReview: Should there be some way to reset value of a state back to its
    // 'initial' value? Not like a dglSetCanonicalState, but more like a:
    //    GFX->setSomeRenderState(); // No args means default value
    //
    // When we switch back to state blocks, does a canonical state actually make
    // *more* sense now?
    // [6/5/2007 Pat]
    //
    // Another approach is to be able to "mark" the beginning of a new render setup,
    // and from that point until a draw primitive track anything which was dirty
    // before the mark  and not touched after the mark.  That way we can rely on a
    // clean rendering slate without slamming all the states.  Something like this:
    //    Default value of state A=a.  Assume previous render left A=a'.
    //    If after calling markInitState() we never touch state A, it will get set to value a on first draw.
    //    If we touch it, it will not get set back to default.
    //    If, OTOH, the previous draw had left it in default state, it would not get reset.
    // So algo would be that when markInitState() is called all the dirty states would get
    // added to a linked list.  As states are touched they would be removed from this list.  When draw
    // call is made, all remaining states would be slammed.  This works out better than the dglCannonical
    // approach because we don't end up setting A=a' then A=a (on dglCannonical), then A=a' because
    // next render wants the same state.  Particularly important for things like shader constants which
    // our state cacher isn't cacheing.
    // Note: markInitState() not meant to be actual name of method, just for illustration purposes.
    // [6/30/2007 Clark]

    // This macro will help this function out, because we don't actually want to
    // use the GFX device methods to set the states. This is a forced-state set
    // that, while it will notify the state-cache, it occurs on the graphics device
    // immediately. If state debugging is enabled, then the macro will also set up
    // a watch on each state and provide state history at end of frame. This will
    // be very useful for figuring out what code is not cleaning up states.
#ifdef TORQUE_GFX_STATE_DEBUG
    #  define SET_INITIAL_RENDER_STATE( state, val ) \
   initRenderState( state, val ); \
   setRenderState( state, val ); \
   mDebugStateManager->WatchState( GFXDebugState::GFXRenderStateEvent, state )
#else
#  define SET_INITIAL_RENDER_STATE( state, val ) \
   initRenderState( state, val ); \
   setRenderState( state, val )
#endif

    // Set up all render states with the macro
    SET_INITIAL_RENDER_STATE( GFXRSZEnable,          false );
    SET_INITIAL_RENDER_STATE( GFXRSZWriteEnable,     true );
    SET_INITIAL_RENDER_STATE( GFXRSLighting,         false );
    SET_INITIAL_RENDER_STATE( GFXRSAlphaBlendEnable, false );
    SET_INITIAL_RENDER_STATE( GFXRSAlphaTestEnable,  false );
    SET_INITIAL_RENDER_STATE( GFXRSZFunc,            GFXCmpAlways );

    SET_INITIAL_RENDER_STATE( GFXRSSrcBlend,         GFXBlendOne );
    SET_INITIAL_RENDER_STATE( GFXRSDestBlend,        GFXBlendZero );

    SET_INITIAL_RENDER_STATE( GFXRSAlphaFunc,        GFXCmpAlways );
    SET_INITIAL_RENDER_STATE( GFXRSAlphaRef,         0 );

    SET_INITIAL_RENDER_STATE( GFXRSStencilFail,      GFXStencilOpKeep );
    SET_INITIAL_RENDER_STATE( GFXRSStencilZFail,     GFXStencilOpKeep );
    SET_INITIAL_RENDER_STATE( GFXRSStencilPass,      GFXStencilOpKeep );

    SET_INITIAL_RENDER_STATE( GFXRSStencilFunc,      GFXCmpAlways );
    SET_INITIAL_RENDER_STATE( GFXRSStencilRef,       0 );
    SET_INITIAL_RENDER_STATE( GFXRSStencilMask,      0xFFFFFFFF );

    SET_INITIAL_RENDER_STATE( GFXRSStencilWriteMask, 0xFFFFFFFF );

    SET_INITIAL_RENDER_STATE( GFXRSStencilEnable,    false );

    SET_INITIAL_RENDER_STATE( GFXRSCullMode,         GFXCullCCW );

    SET_INITIAL_RENDER_STATE( GFXRSColorVertex,      true );

    SET_INITIAL_RENDER_STATE( GFXRSDepthBias,        0 );

    // Add more here using the macro

#undef SET_INITIAL_RENDER_STATE

    // Do the same thing for Texture-Stage states. In this case, all color-op's
    // will get set to disable. This is actually the proper method for disabling
    // texturing on a texture-stage. Setting the texture to NULL, is not.
#ifdef TORQUE_GFX_STATE_DEBUG
    #  define SET_INITIAL_TS_STATE( state, val, stage ) \
   initTextureState( stage, state, val ); \
   setTextureStageState( stage, state, val ); \
   mDebugStateManager->WatchState( GFXDebugState::GFXTextureStageStateEvent, state, stage )
#else
#  define SET_INITIAL_TS_STATE( state, val, stage ) \
   initTextureState( stage, state, val ); \
   setTextureStageState( stage, state, val )
#endif

    // Init these states across all texture stages
    for( int i = 0; i < getNumSamplers(); i++ )
    {
        SET_INITIAL_TS_STATE( GFXTSSColorOp, GFXTOPDisable, i );

        // Add more here using the macro
    }
#undef SET_INITIAL_TS_STATE

#ifdef TORQUE_GFX_STATE_DEBUG
    #  define SET_INITIAL_SAMPLER_STATE( state, val, stage ) \
   initSamplerState( stage, state, val ); \
   setSamplerState( stage, state, val ); \
   mDebugStateManager->WatchState( GFXDebugState::GFXSamplerStateEvent, state, stage )
#else
#  define SET_INITIAL_SAMPLER_STATE( state, val, stage ) \
   initSamplerState( stage, state, val ); \
   setSamplerState( stage, state, val )
#endif

    // Init these states across all texture stages
    for( int i = 0; i < getNumSamplers(); i++ )
    {
        // Set all samplers to clamp tex coordinates outside of their range
        SET_INITIAL_SAMPLER_STATE( GFXSAMPAddressU, GFXAddressClamp, i );
        SET_INITIAL_SAMPLER_STATE( GFXSAMPAddressV, GFXAddressClamp, i );
        SET_INITIAL_SAMPLER_STATE( GFXSAMPAddressW, GFXAddressClamp, i );

        // And point sampling for everyone
        SET_INITIAL_SAMPLER_STATE( GFXSAMPMagFilter, GFXTextureFilterPoint, i );
        SET_INITIAL_SAMPLER_STATE( GFXSAMPMinFilter, GFXTextureFilterPoint, i );
        SET_INITIAL_SAMPLER_STATE( GFXSAMPMipFilter, GFXTextureFilterPoint, i );

        // Add more here using the macro
    }
#undef SET_INITIAL_SAMPLER_STATE
}

//------------------------------------------------------------------------------

#ifdef TORQUE_GFX_STATE_DEBUG
void GFXDevice::processStateWatchHistory( const GFXDebugState::GFXDebugStateWatch *watch )
{
   // This method will get called once per no-break watch, at end of frame, for
   // every frame that the watch is active. The accessors on the state watch
   // should provide all of the information needed to look at the history of
   // the state that was being tracked.
}
#endif

//------------------------------------------------------------------------------

// These two methods I modified because I may want to catch these events for state
// debugging. [6/7/2007 Pat]
inline void GFXDevice::beginScene()
{
    beginSceneInternal();
}

//------------------------------------------------------------------------------

inline void GFXDevice::endScene()
{
    endSceneInternal();
}

//------------------------------------------------------------------------------

void GFXDevice::_updateRenderTargets()
{
    // Re-set the RT if needed.
    GFXTarget *targ = getActiveRenderTarget();

    if( !targ )
        return;

    if( targ->isPendingState() )
        setActiveRenderTarget( targ );
}

void GFXDevice::pushActiveRenderTarget()
{
    // Duplicate last item on the stack.
    mRTStack.push_back(mCurrentRT);
}

void GFXDevice::popActiveRenderTarget()
{
    // Pop the last item on the stack, set next item down.
    AssertFatal(mRTStack.size() > 0, "GFXD3D9Device::popActiveRenderTarget - stack is empty!");
    setActiveRenderTarget(mRTStack.last());
    mRTStack.pop_back();
}

inline GFXTarget *GFXDevice::getActiveRenderTarget()
{
    return mCurrentRT;
}

#ifndef TORQUE_SHIPPING
void GFXDevice::_dumpStatesToFile( const char *fileName ) const
{
#ifdef TORQUE_GFX_STATE_DEBUG
    static int stateDumpNum = 0;
   static char nameBuffer[256];

   dSprintf( nameBuffer, sizeof(nameBuffer), "demo/%s_%d.gfxstate", fileName, stateDumpNum );


   Stream* stream;

   if( ResourceManager->openFileForWrite( stream, nameBuffer ) )
   {
      // Report
      Con::printf( "--Dumping GFX States to file: %s", nameBuffer );

      // Increment state dump #
      stateDumpNum++;

      // Dump render states
      stream->writeLine( (U8 *)"Render States" );
      for( U32 state = GFXRenderState_FIRST; state < GFXRenderState_COUNT; state++ )
         stream->writeLine( (U8 *)avar( "%s - %s", GFXStringRenderState[state], GFXStringRenderStateValueLookup[state]( mStateTracker[state].newValue ) ) );

      // Dump texture stage states
      for( U32 stage = 0; stage < getNumSamplers(); stage++ )
      {
         stream->writeLine( (U8 *)avar( "Texture Stage: %d", stage ) );
         for( U32 state = GFXTSS_FIRST; state < GFXTSS_COUNT; state++ )
            stream->writeLine( (U8 *)avar( "::%s - %s", GFXStringTextureStageState[state], GFXStringTextureStageStateValueLookup[state]( mTextureStateTracker[stage][state].newValue ) ) );
      }

      // Dump sampler states
      for( U32 stage = 0; stage < getNumSamplers(); stage++ )
      {
         stream->writeLine( (U8 *)avar( "Sampler Stage: %d\n", stage ) );
         for( U32 state = GFXSAMP_FIRST; state < GFXSAMP_COUNT; state++ )
            stream->writeLine( (U8 *)avar( "::%s - %s", GFXStringSamplerState[state], GFXStringSamplerStateValueLookup[state]( mSamplerStateTracker[stage][state].newValue ) ) );
      }
      delete stream;
   }
#endif
}
#endif

void GFXDevice::listResources(bool unflaggedOnly)
{
    U32 numTextures = 0, numShaders = 0, numRenderToTextureTargs = 0, numWindowTargs = 0;
    U32 numCubemaps = 0, numVertexBuffers = 0, numPrimitiveBuffers = 0, numFences = 0;
    GFXResource* walk = mResourceListHead;
    while(walk)
    {
        if(unflaggedOnly && walk->isFlagged())
        {
            walk = walk->getNextResource();
            continue;
        }

        if(dynamic_cast<GFXTextureObject*>(walk))
            numTextures++;
        else if(dynamic_cast<GFXShader*>(walk))
            numShaders++;
        else if(dynamic_cast<GFXTextureTarget*>(walk))
            numRenderToTextureTargs++;
        else if(dynamic_cast<GFXWindowTarget*>(walk))
            numWindowTargs++;
        else if(dynamic_cast<GFXCubemap*>(walk))
            numCubemaps++;
        else if(dynamic_cast<GFXVertexBuffer*>(walk))
            numVertexBuffers++;
        else if(dynamic_cast<GFXPrimitiveBuffer*>(walk))
            numPrimitiveBuffers++;
        else if(dynamic_cast<GFXFence*>(walk))
            numFences++;
        else
            Con::warnf("Unknown resource: %x", walk);

        walk = walk->getNextResource();
    }
    const char* flag = unflaggedOnly ? "unflagged" : "allocated";

    Con::printf("GFX currently has:");
    Con::printf("   %i %s textures", numTextures, flag);
    Con::printf("   %i %s shaders", numShaders, flag);
    Con::printf("   %i %s texture targets", numRenderToTextureTargs, flag);
    Con::printf("   %i %s window targets", numWindowTargs, flag);
    Con::printf("   %i %s cubemaps", numCubemaps, flag);
    Con::printf("   %i %s vertex buffers", numVertexBuffers, flag);
    Con::printf("   %i %s primitive buffers", numPrimitiveBuffers, flag);
    Con::printf("   %i %s fences", numFences, flag);
}

void GFXDevice::fillResourceVectors(const char* resNames, bool unflaggedOnly, Vector<GFXTextureObject*> &textureObjects,
                                    Vector<GFXTextureTarget*> &textureTargets, Vector<GFXWindowTarget*> &windowTargets,
                                    Vector<GFXVertexBuffer*> &vertexBuffers, Vector<GFXPrimitiveBuffer*> &primitiveBuffers,
                                    Vector<GFXFence*> &fences, Vector<GFXCubemap*> &cubemaps, Vector<GFXShader*> &shaders)
{
    bool describeTexture = true, describeTextureTarget = true, describeWindowTarget = true, describeVertexBuffer = true,
        describePrimitiveBuffer = true, describeFence = true, describeCubemap = true, describeShader = true;

    // If we didn't specify a string of names, we'll print all of them
    if(resNames && resNames[0] != '\0')
    {
        // If we did specify a string of names, determine which names
        describeTexture =          (dStrstr(resNames, "GFXTextureObject")    != NULL);
        describeTextureTarget =    (dStrstr(resNames, "GFXTextureTarget")    != NULL);
        describeWindowTarget =     (dStrstr(resNames, "GFXWindowTarget")     != NULL);
        describeVertexBuffer =     (dStrstr(resNames, "GFXVertexBuffer")     != NULL);
        describePrimitiveBuffer =  (dStrstr(resNames, "GFXPrimitiveBuffer")   != NULL);
        describeFence =            (dStrstr(resNames, "GFXFence")            != NULL);
        describeCubemap =          (dStrstr(resNames, "GFXCubemap")          != NULL);
        describeShader =           (dStrstr(resNames, "GFXShader")           != NULL);
    }

    // Start going through the list
    GFXResource* walk = mResourceListHead;
    while(walk)
    {
        // If we only want unflagged resources, skip all flagged resources
        if(unflaggedOnly && walk->isFlagged())
        {
            walk = walk->getNextResource();
            continue;
        }

        // All of the following checks go through the same logic.
        // if(describingThisResource)
        // {
        //    ResourceType* type = dynamic_cast<ResourceType*>(walk)
        //    if(type)
        //    {
        //       typeVector.push_back(type);
        //       walk = walk->getNextResource();
        //       continue;
        //    }
        // }

        if(describeTexture)
        {
            GFXTextureObject* tex = dynamic_cast<GFXTextureObject*>(walk);
            {
                if(tex)
                {
                    textureObjects.push_back(tex);
                    walk = walk->getNextResource();
                    continue;
                }
            }
        }
        if(describeShader)
        {
            GFXShader* shd = dynamic_cast<GFXShader*>(walk);
            if(shd)
            {
                shaders.push_back(shd);
                walk = walk->getNextResource();
                continue;
            }
        }
        if(describeVertexBuffer)
        {
            GFXVertexBuffer* buf = dynamic_cast<GFXVertexBuffer*>(walk);
            if(buf)
            {
                vertexBuffers.push_back(buf);
                walk = walk->getNextResource();
                continue;
            }
        }
        if(describePrimitiveBuffer)
        {
            GFXPrimitiveBuffer* buf = dynamic_cast<GFXPrimitiveBuffer*>(walk);
            if(buf)
            {
                primitiveBuffers.push_back(buf);
                walk = walk->getNextResource();
                continue;
            }
        }
        if(describeTextureTarget)
        {
            GFXTextureTarget* targ = dynamic_cast<GFXTextureTarget*>(walk);
            if(targ)
            {
                textureTargets.push_back(targ);
                walk = walk->getNextResource();
                continue;
            }
        }
        if(describeWindowTarget)
        {
            GFXWindowTarget* targ = dynamic_cast<GFXWindowTarget*>(walk);
            if(targ)
            {
                windowTargets.push_back(targ);
                walk = walk->getNextResource();
                continue;
            }
        }
        if(describeCubemap)
        {
            GFXCubemap* cube = dynamic_cast<GFXCubemap*>(walk);
            if(cube)
            {
                cubemaps.push_back(cube);
                walk = walk->getNextResource();
                continue;
            }
        }
        if(describeFence)
        {
            GFXFence* fence = dynamic_cast<GFXFence*>(walk);
            if(fence)
            {
                fences.push_back(fence);
                walk = walk->getNextResource();
                continue;
            }
        }
        // Wasn't something we were looking for
        walk = walk->getNextResource();
    }
}

/// Helper class for GFXDevice::describeResources.
class DescriptionOutputter
{
    /// Are we writing to a file?
    bool mWriteToFile;

    /// File if we are writing to a file
    FileStream mFile;
public:
    DescriptionOutputter(const char* file)
    {
        mWriteToFile = false;
        // If we've been given what could be a valid file path, open it.
        if(file && file[0] != '\0')
        {
            mWriteToFile = mFile.open(file, FileStream::Write);

            // Note that it is safe to retry.  If this is hit, we'll just write to the console instead of to the file.
            AssertFatal(mWriteToFile, avar("DescriptionOutputter::DescriptionOutputter - could not open file %s", file));
        }
    }

    ~DescriptionOutputter()
    {
        // Close the file
        if(mWriteToFile)
            mFile.close();
    }

    /// Writes line to the file or to the console, depending on what we want.
    void write(const char* line)
    {
        if(mWriteToFile)
            mFile.writeLine((U8*)line);
        else
            Con::printf(line);
    }
};

void GFXDevice::describeResources(const char* resNames, const char* filePath, bool unflaggedOnly)
{
    // Vectors of objects.  By having these we can go through the list once.
    Vector<GFXTextureObject*> textureObjects(__FILE__, __LINE__);
    Vector<GFXTextureTarget*> textureTargets(__FILE__, __LINE__);
    Vector<GFXWindowTarget*> windowTargets(__FILE__, __LINE__);
    Vector<GFXVertexBuffer*> vertexBuffers(__FILE__, __LINE__);
    Vector<GFXPrimitiveBuffer*> primitiveBuffers(__FILE__, __LINE__);
    Vector<GFXFence*> fences(__FILE__, __LINE__);
    Vector<GFXCubemap*> cubemaps(__FILE__, __LINE__);
    Vector<GFXShader*> shaders(__FILE__, __LINE__);

    // Fill the vectors with the right resources
    fillResourceVectors(resNames, unflaggedOnly, textureObjects, textureTargets, windowTargets, vertexBuffers, primitiveBuffers,
                        fences, cubemaps, shaders);

    // Helper object
    DescriptionOutputter output(filePath);

    // Print the info to the file
    // Note that we check if we have any objects of that type.
    char descriptionBuffer[996];
    char writeBuffer[1024];
    if(textureObjects.size())
    {
        output.write("--------Dumping GFX texture descriptions...----------");
        for(U32 i = 0; i < textureObjects.size(); i++)
        {
            textureObjects[i]->describeSelf(descriptionBuffer, sizeof(descriptionBuffer));
            dSprintf(writeBuffer, sizeof(writeBuffer), "Addr: %x %s", textureObjects[i], descriptionBuffer);
            output.write(writeBuffer);
        }
        output.write("--------------------Done---------------------");
        output.write("");
    }

    if(textureTargets.size())
    {
        output.write("--------Dumping GFX texture target descriptions...----------");
        for(U32 i = 0; i < textureTargets.size(); i++)
        {
            textureTargets[i]->describeSelf(descriptionBuffer, sizeof(descriptionBuffer));
            dSprintf(writeBuffer, sizeof(writeBuffer), "Addr: %x %s", textureTargets[i], descriptionBuffer);
            output.write(writeBuffer);
        }
        output.write("--------------------Done---------------------");
        output.write("");
    }

    if(windowTargets.size())
    {
        output.write("--------Dumping GFX window target descriptions...----------");
        for(U32 i = 0; i < windowTargets.size(); i++)
        {
            windowTargets[i]->describeSelf(descriptionBuffer, sizeof(descriptionBuffer));
            dSprintf(writeBuffer, sizeof(writeBuffer), "Addr: %x %s", windowTargets[i], descriptionBuffer);
            output.write(writeBuffer);
        }
        output.write("--------------------Done---------------------");
        output.write("");
    }

    if(vertexBuffers.size())
    {
        output.write("--------Dumping GFX vertex buffer descriptions...----------");
        for(U32 i = 0; i < vertexBuffers.size(); i++)
        {
            vertexBuffers[i]->describeSelf(descriptionBuffer, sizeof(descriptionBuffer));
            dSprintf(writeBuffer, sizeof(writeBuffer), "Addr: %x %s", vertexBuffers[i], descriptionBuffer);
            output.write(writeBuffer);
        }
        output.write("--------------------Done---------------------");
        output.write("");
    }

    if(primitiveBuffers.size())
    {
        output.write("--------Dumping GFX primitive buffer descriptions...----------");
        for(U32 i = 0; i < primitiveBuffers.size(); i++)
        {
            primitiveBuffers[i]->describeSelf(descriptionBuffer, sizeof(descriptionBuffer));
            dSprintf(writeBuffer, sizeof(writeBuffer), "Addr: %x %s", primitiveBuffers[i], descriptionBuffer);
            output.write(writeBuffer);
        }
        output.write("--------------------Done---------------------");
        output.write("");
    }

    if(fences.size())
    {
        output.write("--------Dumping GFX fence descriptions...----------");
        for(U32 i = 0; i < fences.size(); i++)
        {
            fences[i]->describeSelf(descriptionBuffer, sizeof(descriptionBuffer));
            dSprintf(writeBuffer, sizeof(writeBuffer), "Addr: %x %s", fences[i], descriptionBuffer);
            output.write(writeBuffer);
        }
        output.write("--------------------Done---------------------");
        output.write("");
    }

    if(cubemaps.size())
    {
        output.write("--------Dumping GFX cubemap descriptions...----------");
        for(U32 i = 0; i < cubemaps.size(); i++)
        {
            cubemaps[i]->describeSelf(descriptionBuffer, sizeof(descriptionBuffer));
            dSprintf(writeBuffer, sizeof(writeBuffer), "Addr: %x %s", cubemaps[i], descriptionBuffer);
            output.write(writeBuffer);
        }
        output.write("--------------------Done---------------------");
        output.write("");
    }

    if(shaders.size())
    {
        output.write("--------Dumping GFX shader descriptions...----------");
        for(U32 i = 0; i < shaders.size(); i++)
        {
            shaders[i]->describeSelf(descriptionBuffer, sizeof(descriptionBuffer));
            dSprintf(writeBuffer, sizeof(writeBuffer), "Addr: %x %s", shaders[i], descriptionBuffer);
            output.write(writeBuffer);
        }
        output.write("--------------------Done---------------------");
        output.write("");
    }
}


void GFXDevice::flagCurrentResources()
{
    GFXResource* walk = mResourceListHead;
    while(walk)
    {
        walk->setFlag();
        walk = walk->getNextResource();
    }
}

void GFXDevice::clearResourceFlags()
{
    GFXResource* walk = mResourceListHead;
    while(walk)
    {
        walk->clearFlag();
        walk = walk->getNextResource();
    }
}

ConsoleFunction(listGFXResources, void, 1, 2, "(bool unflaggedOnly = false)")
{
    bool unflaggedOnly = false;
    if(argc == 2)
        unflaggedOnly = dAtob(argv[1]);
    GFX->listResources(unflaggedOnly);
}

ConsoleFunction(flagCurrentGFXResources, void, 1, 1, "")
{
    GFX->flagCurrentResources();
}

ConsoleFunction(clearGFXResourceFlags, void, 1, 1, "")
{
    GFX->clearResourceFlags();
}

ConsoleFunction(describeGFXResources, void, 3, 4, "(string resourceNames, string filePath, bool unflaggedOnly = false)\n"
                                                  " If resourceNames is "", this function describes all resources.\n"
                                                  " If filePath is "", this function writes the resource descriptions to the console")
{
    bool unflaggedOnly = false;
    if(argc == 4)
        unflaggedOnly = dAtob(argv[3]);
    GFX->describeResources(argv[1], argv[2], unflaggedOnly);
}

//-----------------------------------------------------------------------------
// Get pixel shader version - for script
//-----------------------------------------------------------------------------
ConsoleFunction(getPixelShaderVersion, F32, 1, 1, "Get pixel shader version.\n\n")
{
    return GFX->getPixelShaderVersion();
}

