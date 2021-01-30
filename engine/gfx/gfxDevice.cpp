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
#include "scenegraph/sceneGraph.h"
#include "gfx/debugDraw.h"
#include "gfx/gNewFont.h"
#include "platform/profiler.h"
#include "core/unicode.h"

Vector<GFXDevice*> GFXDevice::smGFXDevice;
S32 GFXDevice::smActiveDeviceIndex = -1;


//-----------------------------------------------------------------------------

// Static method
GFXDevice* GFXDevice::get()
{
    AssertFatal(smActiveDeviceIndex > -1 && smGFXDevice[smActiveDeviceIndex] != NULL, "Attempting to get invalid GFX device.");
    return smGFXDevice[smActiveDeviceIndex];
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

    for (U32 i = 0; i < TEXTURE_STAGE_COUNT; i++)
    {
        mTextureDirty[i] = false;
        mCurrentTexture[i] = NULL;
        mTexType[i] = GFXTDT_Normal;
    }

    // Initialize the state stack.
    mStateStackDepth = 0;
    mStateStack[0].init();

    // misc
    mAllowRender = true;
}

//-----------------------------------------------------------------------------

// Static method
void GFXDevice::create()
{
    // nothing right now
}

//-----------------------------------------------------------------------------

// Static method
void GFXDevice::destroy()
{
    // Make this release its buffer.
    PrimBuild::shutdown();

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
        smGFXDevice.pop_front();

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

    // Clean up our current PB, if any.
    mCurrentPrimitiveBuffer = NULL;
    mCurrentVertexBuffer = NULL;

    bool found = false;

    for (Vector<GFXDevice*>::iterator i = smGFXDevice.begin(); i != smGFXDevice.end(); i++)
    {
        if ((*i)->mDeviceIndex == mDeviceIndex)
        {
            smGFXDevice.erase(i);
            found = true;
        }
        else if (found == true)
        {
            (*i)->mDeviceIndex--;
        }
    }

    if (mTextureManager)
    {
        delete mTextureManager;
        mTextureManager = NULL;
    }
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
    if (forceSetAll)
    {
        bool rememberToEndScene = false;
        if (!GFX->canCurrentlyRender())
        {
            GFX->beginScene();
            rememberToEndScene = true;
        }

        setMatrix(GFXMatrixProjection, mProjectionMatrix);
        setMatrix(GFXMatrixWorld, mWorldMatrix[mWorldStackSize]);
        setMatrix(GFXMatrixView, mViewMatrix);

        if (mCurrentVertexBuffer.isValid())
            mCurrentVertexBuffer->prepare();

        if (mCurrentPrimitiveBuffer.isValid()) // This could be NULL when the device is initalizing
            mCurrentPrimitiveBuffer->prepare();

        for (U32 i = 0; i < TEXTURE_STAGE_COUNT; i++)
        {
            if (mCurrentTexture[i] == NULL)
            {
                setTextureInternal(i, NULL);
                continue;
            }

            if (mTexType[i] == GFXTDT_Normal)
            {
                setTextureInternal(i, (GFXTextureObject*)mCurrentTexture[i]);
            }
            else
            {
                ((GFXCubemap*)mCurrentTexture[i])->setToTexUnit(i);
            }
        }

        for (S32 i = 0; i < GFXRenderState_COUNT; i++)
        {
            setRenderState(i, mStateTracker[i].newValue);
            mStateTracker[i].currentValue = mStateTracker[i].newValue;
        }

        // Why is this 8 and not 16 like the others?
        for (S32 i = 0; i < 8; i++)
        {
            for (S32 j = 0; j < GFXTSS_COUNT; j++)
            {
                StateTracker& st = mTextureStateTracker[i][j];
                setTextureStageState(i, j, st.newValue);
                st.currentValue = st.newValue;
            }
        }

        for (S32 i = 0; i < 8; i++)
        {
            for (S32 j = 0; j < GFXSAMP_COUNT; j++)
            {
                StateTracker& st = mSamplerStateTracker[i][j];
                setSamplerState(i, j, st.newValue);
                st.currentValue = st.newValue;
            }
        }

        if (rememberToEndScene)
            GFX->endScene();

        return;
    }

    // Normal update logic begins here.
    mStateDirty = false;

    // Update Projection Matrix
    if (mProjectionMatrixDirty)
    {
        setMatrix(GFXMatrixProjection, mProjectionMatrix);
        mProjectionMatrixDirty = false;
    }

    // Update World Matrix
    if (mWorldMatrixDirty)
    {
        setMatrix(GFXMatrixWorld, mWorldMatrix[mWorldStackSize]);
        mWorldMatrixDirty = false;
    }

    // Update View Matrix
    if (mViewMatrixDirty)
    {
        setMatrix(GFXMatrixView, mViewMatrix);
        mViewMatrixDirty = false;
    }

    // Update vertex buffer
    if (mVertexBufferDirty)
    {
        if (mCurrentVertexBuffer.isValid())
            mCurrentVertexBuffer->prepare();
        mVertexBufferDirty = false;
    }

    // Update primitive buffer
    if (mPrimitiveBufferDirty)
    {
        if (mCurrentPrimitiveBuffer.isValid()) // This could be NULL when the device is initalizing
            mCurrentPrimitiveBuffer->prepare();
        mPrimitiveBufferDirty = false;
    }

    if (mTexturesDirty)
    {
        mTexturesDirty = false;
        for (U32 i = 0; i < TEXTURE_STAGE_COUNT; i++)
        {
            if (!mTextureDirty[i])
                continue;

            mTextureDirty[i] = false;

            if (mCurrentTexture[i] == NULL)
            {
                setTextureInternal(i, NULL);
                continue;
            }

            if (mTexType[i] == GFXTDT_Normal)
            {
                setTextureInternal(i, (GFXTextureObject*)mCurrentTexture[i]);
            }
            else
            {
                ((GFXCubemap*)mCurrentTexture[i])->setToTexUnit(i);
            }
        }
    }

    // Set render states
    while (mNumDirtyStates)
    {
        mNumDirtyStates--;
        U32 state = mTrackedState[mNumDirtyStates];

        // Added so that an unsupported state can be set to > Max render state
        // and thus be ignored, could possibly put in a call to the gfx device
        // to handle an unsupported call so it could log it, or do some workaround
        // or something? -pw
        if (state > GFXRenderState_COUNT)
            continue;

        if (mStateTracker[state].currentValue != mStateTracker[state].newValue)
        {
            setRenderState(state, mStateTracker[state].newValue);
            mStateTracker[state].currentValue = mStateTracker[state].newValue;
        }
        mStateTracker[state].dirty = false;
    }

    // Set texture states
    while (mNumDirtyTextureStates)
    {
        mNumDirtyTextureStates--;
        U32 state = mTextureTrackedState[mNumDirtyTextureStates].state;
        U32 stage = mTextureTrackedState[mNumDirtyTextureStates].stage;
        StateTracker& st = mTextureStateTracker[stage][state];
        st.dirty = false;
        if (st.currentValue != st.newValue)
        {
            setTextureStageState(stage, state, st.newValue);
            st.currentValue = st.newValue;
        }
    }

    // Set sampler states
    while (mNumDirtySamplerStates)
    {
        mNumDirtySamplerStates--;
        U32 state = mSamplerTrackedState[mNumDirtySamplerStates].state;
        U32 stage = mSamplerTrackedState[mNumDirtySamplerStates].stage;
        StateTracker& st = mSamplerStateTracker[stage][state];
        st.dirty = false;
        if (st.currentValue != st.newValue)
        {
            setSamplerState(stage, state, st.newValue);
            st.currentValue = st.newValue;
        }
    }

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
    Vector<GFXAdapter> adapters;
    GFXInit::getAdapters(&adapters);

    U32 deviceCount = adapters.size();
    if (deviceCount < 1)
        return NULL;

    U32 strLen = 0, i;
    for (i = 0; i < deviceCount; i++)
        strLen += (dStrlen(adapters[i].name) + 1);

    char* returnString = Con::getReturnBuffer(strLen);
    dStrcpy(returnString, adapters[0].name);
    for (i = 1; i < deviceCount; i++)
    {
        dStrcat(returnString, "\t");
        dStrcat(returnString, adapters[i].name);
    }

    return(returnString);
}

ConsoleFunction(getResolution, const char*, 1, 1, "Returns screen resolution in form \"width height bitdepth fullscreen\"")
{
    Point2I size = GFX->getVideoMode().resolution;
    S32 bitdepth = GFX->getVideoMode().bitDepth;
    S32 fullscreen = GFX->getVideoMode().fullScreen;

    char* buf = Con::getReturnBuffer(256);
    dSprintf(buf, 256, "%d %d %d %d", size.x, size.y, bitdepth, fullscreen);
    return buf;
}

ConsoleFunction(getResolutionList, const char*, 1, 1, "Returns a tab-seperated list of possible resolutions.")
{
    const Vector<GFXVideoMode>* const modelist = GFX->getVideoModeList();

    U32 k;
    U32 stringLen = 0;
    for (k = 0; k < modelist->size(); k++)
    {
        AssertFatal((*modelist)[k].resolution.x < 10000 && (*modelist)[k].resolution.y < 10000,
            "My brain hurts with resolutions above 10k pixels. :(");

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

ConsoleFunction(setVideoMode, void, 5, 5, "setVideoMode(width, height, bit depth, fullscreen)")
{
    GFXVideoMode vm = GFX->getVideoMode();
    vm.resolution = Point2I(dAtoi(argv[1]), dAtoi(argv[2]));
    vm.bitDepth = dAtoi(argv[3]);
    vm.fullScreen = dAtoi(argv[4]);
    GFX->setVideoMode(vm);

    if (vm.fullScreen)
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
        GFX->setLightingEnable(false);
        GFX->setCullMode(GFXCullNone);
        GFX->setAlphaBlendEnable(true);
        GFX->setSrcBlend(GFXBlendSrcAlpha);
        GFX->setDestBlend(GFXBlendInvSrcAlpha);
        GFX->setTextureStageMagFilter(0, GFXTextureFilterPoint);
        GFX->setTextureStageMinFilter(0, GFXTextureFilterPoint);
        GFX->setTextureStageAddressModeU(0, GFXAddressClamp);
        GFX->setTextureStageAddressModeV(0, GFXAddressClamp);

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
    F32 farPlane)
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

    MatrixF rotMat(EulerF((M_PI / 2.0), 0.0, 0.0));

    projection.mul(rotMat);

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
    F32 farPlane)
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

    MatrixF rotMat(EulerF((M_PI / 2.0), 0.0, 0.0));

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
// Set texture
//-----------------------------------------------------------------------------
void GFXDevice::setTexture(U32 stage, GFXTextureObject* texture)
{
    AssertFatal(stage < TEXTURE_STAGE_COUNT, "GFXDevice::setTexture - out of range stage!");

    if (!mTextureDirty[stage])
    {
        mStateDirty = true;
        mTexturesDirty = true;
        mTextureDirty[stage] = true;
    }
    mCurrentTexture[stage] = texture;
    mTexType[stage] = GFXTDT_Normal;

}

//-----------------------------------------------------------------------------
// Set cube texture
//-----------------------------------------------------------------------------
void GFXDevice::setCubeTexture(U32 stage, GFXCubemap* texture)
{
    AssertFatal(stage < TEXTURE_STAGE_COUNT, "GFXDevice::setTexture - out of range stage!");

    if (!mTextureDirty[stage])
    {
        mStateDirty = true;
        mTexturesDirty = true;
        mTextureDirty[stage] = true;
    }
    mCurrentTexture[stage] = texture;
    mTexType[stage] = GFXTDT_Cube;
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


//-----------------------------------------------------------------------------
// Get pixel shader version - for script
//-----------------------------------------------------------------------------
ConsoleFunction(getPixelShaderVersion, F32, 1, 1, "Get pixel shader version.\n\n")
{
    return GFX->getPixelShaderVersion();
}

