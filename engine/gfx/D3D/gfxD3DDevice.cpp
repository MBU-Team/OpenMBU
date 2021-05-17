//-----------------------------------------------------------------------------
// Torque Shader Engine
// 
// Copyright (c) 2003 GarageGames.Com
//-----------------------------------------------------------------------------

#ifdef _MSC_VER
#pragma warning(disable: 4996) // turn off "deprecation" warnings
#endif

#include <d3d9.h>
#include <d3dx9math.h>
#include "gfx/D3D/gfxD3DDevice.h"
#include "core/fileStream.h"
#include "console/console.h"
#include "gfx/primBuilder.h"
#include "platform/profiler.h"
#include "gfx/D3D/gfxD3DCardProfiler.h"
#include "gfx/D3D/gfxD3DVertexBuffer.h"
#include "gfx/D3D/screenShotD3D.h"
#include "gfx/d3d/gfxD3DEnumTranslate.h"
#include "../../example/shaders/shdrConsts.h"
#include "core/unicode.h"
#include "gfx/d3d/gfxD3DShader.h"

// Include this after the 
#include "platformWin32/dxVersionChecker.h"

// Gross hack to let the diag utility know that we only need stubs
#define DUMMYDEF
#include "gfx/D3D/DXDiagNVUtil.h"

GFXD3DDevice::GFXD3DDevice(LPDIRECT3D9 d3d, U32 index)
{
    GFXVertexColor::sVertexColorOrdering = GFXVertexColor::ColorOrdering::BGRA;

    mD3D = d3d;
    mAdapterIndex = index;
    mD3DDevice = NULL;
    mCurrentOpenAllocVB = NULL;
    mCurrentVB = NULL;

    mCurrentOpenAllocPB = NULL;
    mCurrentPB = NULL;
    mDynamicPB = NULL;

    mCanCurrentlyRender = false;
    mTextureManager = NULL;

#ifdef TORQUE_DEBUG
    mVBListHead = NULL;
    mNumAllocatedVertexBuffers = 0;
#endif

    mGenericShader[GSColor] = NULL;

    mCurrentOpenAllocVertexData = NULL;
    mPixVersion = 0.0;
}

//-----------------------------------------------------------------------------

GFXD3DDevice::~GFXD3DDevice()
{
    mShaderMgr.shutdown();

    releaseDefaultPoolResources();

    // Check up on things
    Con::printf("Cur. D3DDevice ref count=%d", mD3DDevice->AddRef() - 1);
    mD3DDevice->Release();

    // Forcibly clean up the pools
    mVolatileVBList.setSize(0);
    mDynamicVBList.setSize(0);
    mDynamicPB = NULL;

    // And release our D3D resources.
    SAFE_RELEASE(mD3D);
    SAFE_RELEASE(mD3DDevice);

#ifdef TORQUE_DEBUG
    logVertexBuffers();

    // Free them
    if (mNumAllocatedVertexBuffers != 0)
    {
        Platform::AlertOK("Debug Vertex Tracker", "You have unallocated vertex buffers, check vertexbuffer.log");

        GFXD3DVertexBuffer* walk = mVBListHead;
        GFXD3DVertexBuffer* temp = NULL;

        //while( walk != NULL ) 
        //{
        //   temp = walk;
        //   walk = walk->next;
        //   freeVertexBuffer( temp );
        //}
    }
#endif

    if (mCardProfiler)
    {
        delete mCardProfiler;
        mCardProfiler = NULL;
    }

    if (gScreenShot)
    {
        delete gScreenShot;
        gScreenShot = NULL;
    }

#ifdef TORQUE_DEBUG
    GFXTextureObject::dumpExtantTOs();
    GFXPrimitiveBuffer::dumpExtantPBs();
#endif
}

//-----------------------------------------------------------------------------

void GFXD3DDevice::enumerateVideoModes()
{

    Vector<D3DFORMAT> formats;
    formats.push_back(D3DFMT_R5G6B5);    // D3DFMT_R5G6B5 - 16bit format
    formats.push_back(D3DFMT_X8R8G8B8);  // D3DFMT_X8R8G8B8 - 32bit format

    for (S32 i = 0; i < formats.size(); i++)
    {
        for (U32 j = 0; j < mD3D->GetAdapterModeCount(D3DADAPTER_DEFAULT, formats[i]); j++)
        {
            D3DDISPLAYMODE mode;
            mD3D->EnumAdapterModes(D3DADAPTER_DEFAULT, formats[i], j, &mode);

            GFXVideoMode toAdd;

            toAdd.bitDepth = (i == 0 ? 16 : 32); // This will need to be changed later
            toAdd.fullScreen = false;
            toAdd.refreshRate = mode.RefreshRate;
            toAdd.resolution = Point2I(mode.Width, mode.Height);
            toAdd.borderless = false;

            bool skipRes = false;
            for (U32 k = 0; k < mVideoModes.size(); k++)
            {
                if (toAdd.bitDepth == mVideoModes[k].bitDepth && toAdd.resolution == mVideoModes[k].resolution)
                    skipRes = true;
            }

            if (skipRes)
                continue;

            mVideoModes.push_back(toAdd);
        }
    }
}

//-----------------------------------------------------------------------------

void GFXD3DDevice::setVideoMode(const GFXVideoMode& mode)
{
    // set this before the reset - some modules like the GlowBuffer need to 
    // resize screen buffers to the new video mode ( during reset() )
    mVideoMode = mode;

    D3DPRESENT_PARAMETERS d3dpp = setupPresentParams(mode);
    reset(d3dpp);

    Platform::setWindowSize(mode.resolution.x, mode.resolution.y, mode.fullScreen, mode.borderless);

    // Setup default states
    initStates();

    GFX->updateStates(true);
}

//-----------------------------------------------------------------------------

GFXFormat GFXD3DDevice::selectSupportedFormat(GFXTextureProfile* profile,
    const Vector<GFXFormat>& formats, bool texture, bool mustblend)
{
    DWORD usage = 0;

    if (profile->isDynamic())
        usage |= D3DUSAGE_DYNAMIC;

    if (profile->isRenderTarget())
        usage |= D3DUSAGE_RENDERTARGET;

    if (profile->isZTarget())
        usage |= D3DUSAGE_DEPTHSTENCIL;

    if (mustblend)
        usage |= D3DUSAGE_QUERY_POSTPIXELSHADER_BLENDING;

    D3DDISPLAYMODE mode;
    D3DAssert(mD3D->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &mode), "Unable to get adapter mode.");

    D3DRESOURCETYPE type;
    if (texture)
        type = D3DRTYPE_TEXTURE;
    else
        type = D3DRTYPE_SURFACE;

    for (U32 i = 0; i < formats.size(); i++)
    {
        if (mD3D->CheckDeviceFormat(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, mode.Format,
            usage, type, GFXD3DTextureFormat[formats[i]]) == D3D_OK)
            return formats[i];
    }

    return GFXFormatR8G8B8A8;
}

//-----------------------------------------------------------------------------

void GFXD3DDevice::initStates()
{
    //-------------------------------------
    // Auto-generated default states, see regenStates() for details
    //

    // Render states
    initRenderState(0, 1);
    initRenderState(1, 3);
    initRenderState(2, 2);
    initRenderState(3, 1);
    initRenderState(4, 0);
    initRenderState(5, 1);
    initRenderState(6, 1);
    initRenderState(7, 0);
    initRenderState(8, 2);
    initRenderState(9, 3);
    initRenderState(10, 0);
    initRenderState(11, 7);
    initRenderState(12, 0);
    initRenderState(13, 0);
    initRenderState(14, 0);
    initRenderState(15, 0);
    initRenderState(16, 0);
    initRenderState(17, 0);
    initRenderState(18, 0);
    initRenderState(19, 1065353216);
    initRenderState(20, 1065353216);
    initRenderState(21, 0);
    initRenderState(22, 0);
    initRenderState(23, 0);
    initRenderState(24, 0);
    initRenderState(25, 0);
    initRenderState(26, 7);
    initRenderState(27, 0);
    initRenderState(28, -1);
    initRenderState(29, -1);
    initRenderState(30, -1);
    initRenderState(31, 0);
    initRenderState(32, 0);
    initRenderState(33, 0);
    initRenderState(34, 0);
    initRenderState(35, 0);
    initRenderState(36, 0);
    initRenderState(37, 0);
    initRenderState(38, 0);
    initRenderState(39, 1);
    initRenderState(40, 1);
    initRenderState(41, 0);
    initRenderState(42, 0);
    initRenderState(43, 1);
    initRenderState(44, 1);
    initRenderState(45, 0);
    initRenderState(46, 1);
    initRenderState(47, 2);
    initRenderState(48, 0);
    initRenderState(49, 0);
    initRenderState(50, 0);
    initRenderState(51, 0);
    initRenderState(52, 1065353216);
    initRenderState(53, 1065353216);
    initRenderState(54, 0);
    initRenderState(55, 0);
    initRenderState(56, 1065353216);
    initRenderState(57, 0);
    initRenderState(58, 0);
    initRenderState(59, 1);
    initRenderState(60, -1);
    initRenderState(61, 0);
    initRenderState(62, -1163015426);
    initRenderState(63, 1132462080);
    initRenderState(64, 0);
    initRenderState(65, 15);
    initRenderState(66, 0);
    initRenderState(67, 0);
    initRenderState(68, 3);
    initRenderState(69, 1);
    initRenderState(70, 0);
    initRenderState(71, 0);
    initRenderState(72, 0);
    initRenderState(73, 1065353216);
    initRenderState(74, 1065353216);
    initRenderState(75, 0);
    initRenderState(76, 0);
    initRenderState(77, 1065353216);
    initRenderState(78, 0);
    initRenderState(79, 0);
    initRenderState(80, 0);
    initRenderState(81, 0);
    initRenderState(82, 0);
    initRenderState(83, 0);
    initRenderState(84, 7);
    initRenderState(85, 15);
    initRenderState(86, 15);
    initRenderState(87, 15);
    initRenderState(88, -1);
    initRenderState(89, 0);
    initRenderState(90, 0);
    initRenderState(91, 0);
    initRenderState(92, 0);
    initRenderState(93, 0);
    initRenderState(94, 0);
    initRenderState(95, 0);
    initRenderState(96, 0);
    initRenderState(97, 0);
    initRenderState(98, 0);
    initRenderState(99, 0);
    initRenderState(100, 1);
    initRenderState(101, 0);
    initRenderState(102, 0);

    // Texture Stage states
    initTextureState(0, 0, 3);
    initTextureState(0, 1, 2);
    initTextureState(0, 2, 1);
    initTextureState(0, 3, 1);
    initTextureState(0, 4, 2);
    initTextureState(0, 5, 1);
    initTextureState(0, 6, 0);
    initTextureState(0, 7, 0);
    initTextureState(0, 8, 0);
    initTextureState(0, 9, 0);
    initTextureState(0, 10, 0);
    initTextureState(0, 11, 0);
    initTextureState(0, 12, 0);
    initTextureState(0, 13, 0);
    initTextureState(0, 14, 1);
    initTextureState(0, 15, 1);
    initTextureState(0, 16, 1);
    initTextureState(0, 17, 0);
    initTextureState(1, 0, 0);
    initTextureState(1, 1, 2);
    initTextureState(1, 2, 1);
    initTextureState(1, 3, 0);
    initTextureState(1, 4, 2);
    initTextureState(1, 5, 1);
    initTextureState(1, 6, 0);
    initTextureState(1, 7, 0);
    initTextureState(1, 8, 0);
    initTextureState(1, 9, 0);
    initTextureState(1, 10, 1);
    initTextureState(1, 11, 0);
    initTextureState(1, 12, 0);
    initTextureState(1, 13, 0);
    initTextureState(1, 14, 1);
    initTextureState(1, 15, 1);
    initTextureState(1, 16, 1);
    initTextureState(1, 17, 0);
    initTextureState(2, 0, 0);
    initTextureState(2, 1, 2);
    initTextureState(2, 2, 1);
    initTextureState(2, 3, 0);
    initTextureState(2, 4, 2);
    initTextureState(2, 5, 1);
    initTextureState(2, 6, 0);
    initTextureState(2, 7, 0);
    initTextureState(2, 8, 0);
    initTextureState(2, 9, 0);
    initTextureState(2, 10, 2);
    initTextureState(2, 11, 0);
    initTextureState(2, 12, 0);
    initTextureState(2, 13, 0);
    initTextureState(2, 14, 1);
    initTextureState(2, 15, 1);
    initTextureState(2, 16, 1);
    initTextureState(2, 17, 0);
    initTextureState(3, 0, 0);
    initTextureState(3, 1, 2);
    initTextureState(3, 2, 1);
    initTextureState(3, 3, 0);
    initTextureState(3, 4, 2);
    initTextureState(3, 5, 1);
    initTextureState(3, 6, 0);
    initTextureState(3, 7, 0);
    initTextureState(3, 8, 0);
    initTextureState(3, 9, 0);
    initTextureState(3, 10, 3);
    initTextureState(3, 11, 0);
    initTextureState(3, 12, 0);
    initTextureState(3, 13, 0);
    initTextureState(3, 14, 1);
    initTextureState(3, 15, 1);
    initTextureState(3, 16, 1);
    initTextureState(3, 17, 0);
    initTextureState(4, 0, 0);
    initTextureState(4, 1, 2);
    initTextureState(4, 2, 1);
    initTextureState(4, 3, 0);
    initTextureState(4, 4, 2);
    initTextureState(4, 5, 1);
    initTextureState(4, 6, 0);
    initTextureState(4, 7, 0);
    initTextureState(4, 8, 0);
    initTextureState(4, 9, 0);
    initTextureState(4, 10, 4);
    initTextureState(4, 11, 0);
    initTextureState(4, 12, 0);
    initTextureState(4, 13, 0);
    initTextureState(4, 14, 1);
    initTextureState(4, 15, 1);
    initTextureState(4, 16, 1);
    initTextureState(4, 17, 0);
    initTextureState(5, 0, 0);
    initTextureState(5, 1, 2);
    initTextureState(5, 2, 1);
    initTextureState(5, 3, 0);
    initTextureState(5, 4, 2);
    initTextureState(5, 5, 1);
    initTextureState(5, 6, 0);
    initTextureState(5, 7, 0);
    initTextureState(5, 8, 0);
    initTextureState(5, 9, 0);
    initTextureState(5, 10, 5);
    initTextureState(5, 11, 0);
    initTextureState(5, 12, 0);
    initTextureState(5, 13, 0);
    initTextureState(5, 14, 1);
    initTextureState(5, 15, 1);
    initTextureState(5, 16, 1);
    initTextureState(5, 17, 0);
    initTextureState(6, 0, 0);
    initTextureState(6, 1, 2);
    initTextureState(6, 2, 1);
    initTextureState(6, 3, 0);
    initTextureState(6, 4, 2);
    initTextureState(6, 5, 1);
    initTextureState(6, 6, 0);
    initTextureState(6, 7, 0);
    initTextureState(6, 8, 0);
    initTextureState(6, 9, 0);
    initTextureState(6, 10, 6);
    initTextureState(6, 11, 0);
    initTextureState(6, 12, 0);
    initTextureState(6, 13, 0);
    initTextureState(6, 14, 1);
    initTextureState(6, 15, 1);
    initTextureState(6, 16, 1);
    initTextureState(6, 17, 0);
    initTextureState(7, 0, 0);
    initTextureState(7, 1, 2);
    initTextureState(7, 2, 1);
    initTextureState(7, 3, 0);
    initTextureState(7, 4, 2);
    initTextureState(7, 5, 1);
    initTextureState(7, 6, 0);
    initTextureState(7, 7, 0);
    initTextureState(7, 8, 0);
    initTextureState(7, 9, 0);
    initTextureState(7, 10, 7);
    initTextureState(7, 11, 0);
    initTextureState(7, 12, 0);
    initTextureState(7, 13, 0);
    initTextureState(7, 14, 1);
    initTextureState(7, 15, 1);
    initTextureState(7, 16, 1);
    initTextureState(7, 17, 0);

    // Sampler states
    initSamplerState(0, 0, 0);
    initSamplerState(0, 1, 0);
    initSamplerState(0, 2, 0);
    initSamplerState(0, 3, 0);
    initSamplerState(0, 4, 1);
    initSamplerState(0, 5, 1);
    initSamplerState(0, 6, 0);
    initSamplerState(0, 7, 0);
    initSamplerState(0, 8, 0);
    initSamplerState(0, 9, 1);
    initSamplerState(0, 10, 0);
    initSamplerState(0, 11, 0);
    initSamplerState(0, 12, 0);
    initSamplerState(1, 0, 0);
    initSamplerState(1, 1, 0);
    initSamplerState(1, 2, 0);
    initSamplerState(1, 3, 0);
    initSamplerState(1, 4, 1);
    initSamplerState(1, 5, 1);
    initSamplerState(1, 6, 0);
    initSamplerState(1, 7, 0);
    initSamplerState(1, 8, 0);
    initSamplerState(1, 9, 1);
    initSamplerState(1, 10, 0);
    initSamplerState(1, 11, 0);
    initSamplerState(1, 12, 0);
    initSamplerState(2, 0, 0);
    initSamplerState(2, 1, 0);
    initSamplerState(2, 2, 0);
    initSamplerState(2, 3, 0);
    initSamplerState(2, 4, 1);
    initSamplerState(2, 5, 1);
    initSamplerState(2, 6, 0);
    initSamplerState(2, 7, 0);
    initSamplerState(2, 8, 0);
    initSamplerState(2, 9, 1);
    initSamplerState(2, 10, 0);
    initSamplerState(2, 11, 0);
    initSamplerState(2, 12, 0);
    initSamplerState(3, 0, 0);
    initSamplerState(3, 1, 0);
    initSamplerState(3, 2, 0);
    initSamplerState(3, 3, 0);
    initSamplerState(3, 4, 1);
    initSamplerState(3, 5, 1);
    initSamplerState(3, 6, 0);
    initSamplerState(3, 7, 0);
    initSamplerState(3, 8, 0);
    initSamplerState(3, 9, 1);
    initSamplerState(3, 10, 0);
    initSamplerState(3, 11, 0);
    initSamplerState(3, 12, 0);
    initSamplerState(4, 0, 0);
    initSamplerState(4, 1, 0);
    initSamplerState(4, 2, 0);
    initSamplerState(4, 3, 0);
    initSamplerState(4, 4, 1);
    initSamplerState(4, 5, 1);
    initSamplerState(4, 6, 0);
    initSamplerState(4, 7, 0);
    initSamplerState(4, 8, 0);
    initSamplerState(4, 9, 1);
    initSamplerState(4, 10, 0);
    initSamplerState(4, 11, 0);
    initSamplerState(4, 12, 0);
    initSamplerState(5, 0, 0);
    initSamplerState(5, 1, 0);
    initSamplerState(5, 2, 0);
    initSamplerState(5, 3, 0);
    initSamplerState(5, 4, 1);
    initSamplerState(5, 5, 1);
    initSamplerState(5, 6, 0);
    initSamplerState(5, 7, 0);
    initSamplerState(5, 8, 0);
    initSamplerState(5, 9, 1);
    initSamplerState(5, 10, 0);
    initSamplerState(5, 11, 0);
    initSamplerState(5, 12, 0);
    initSamplerState(6, 0, 0);
    initSamplerState(6, 1, 0);
    initSamplerState(6, 2, 0);
    initSamplerState(6, 3, 0);
    initSamplerState(6, 4, 1);
    initSamplerState(6, 5, 1);
    initSamplerState(6, 6, 0);
    initSamplerState(6, 7, 0);
    initSamplerState(6, 8, 0);
    initSamplerState(6, 9, 1);
    initSamplerState(6, 10, 0);
    initSamplerState(6, 11, 0);
    initSamplerState(6, 12, 0);
    initSamplerState(7, 0, 0);
    initSamplerState(7, 1, 0);
    initSamplerState(7, 2, 0);
    initSamplerState(7, 3, 0);
    initSamplerState(7, 4, 1);
    initSamplerState(7, 5, 1);
    initSamplerState(7, 6, 0);
    initSamplerState(7, 7, 0);
    initSamplerState(7, 8, 0);
    initSamplerState(7, 9, 1);
    initSamplerState(7, 10, 0);
    initSamplerState(7, 11, 0);
    initSamplerState(7, 12, 0);
}

//-----------------------------------------------------------------------------

void GFXD3DDevice::setMatrix(GFXMatrixType mtype, const MatrixF& mat)
{
    mat.transposeTo(mTempMatrix);

    mD3DDevice->SetTransform((_D3DTRANSFORMSTATETYPE)mtype, (D3DMATRIX*)&mTempMatrix);
}

//------------------------------------------------------------------------------
// setupGenericShaders - This function is totally redundant on PC
//------------------------------------------------------------------------------
inline void GFXD3DDevice::setupGenericShaders(GenericShaderType type /* = GSColor */)
{
#ifdef TORQUE_OS_XENON
    if (mGenericShader[GSColor] == NULL)
    {
        mGenericShader[GSColor] = createShader("shaders/genericColorV.hlsl", "shaders/genericColorP.hlsl", 2.f);
        mGenericShader[GSModColorTexture] = createShader("shaders/genericModColorTextureV.hlsl", "shaders/genericModColorTextureP.hlsl", 2.f);
        mGenericShader[GSAddColorTexture] = createShader("shaders/genericAddColorTextureV.hlsl", "shaders/genericAddColorTextureP.hlsl", 2.f);
    }

    mGenericShader[type]->process();

    MatrixF world, view, proj;
    mWorldMatrix[mWorldStackSize].transposeTo(world);
    mViewMatrix.transposeTo(view);
    mProjectionMatrix.transposeTo(proj);

    mTempMatrix = world * view * proj;

    setVertexShaderConstF(VC_WORLD_PROJ, (F32*)&mTempMatrix, 4);
#endif
}

//-----------------------------------------------------------------------------

void GFXD3DDevice::setSamplerState(U32 stage, U32 type, U32 value)
{
    switch (type)
    {
    case GFXSAMPMagFilter:
    case GFXSAMPMinFilter:
    case GFXSAMPMipFilter:
        mD3DDevice->SetSamplerState(stage, GFXD3DSamplerState[type], GFXD3DTextureFilter[value]);
        break;

    case GFXSAMPAddressU:
    case GFXSAMPAddressV:
    case GFXSAMPAddressW:
        mD3DDevice->SetSamplerState(stage, GFXD3DSamplerState[type], GFXD3DTextureAddress[value]);
        break;

    default:
        mD3DDevice->SetSamplerState(stage, GFXD3DSamplerState[type], value);
        break;
    }
}

//-----------------------------------------------------------------------------

void GFXD3DDevice::setTextureStageState(U32 stage, U32 state, U32 value)
{
    switch (state)
    {
    case GFXTSSColorOp:
    case GFXTSSAlphaOp:
        mD3DDevice->SetTextureStageState(stage, GFXD3DTextureStageState[state], GFXD3DTextureOp[value]);
        break;

    default:
        mD3DDevice->SetTextureStageState(stage, GFXD3DTextureStageState[state], value);
        break;
    }
}

//-----------------------------------------------------------------------------

void GFXD3DDevice::setRenderState(U32 state, U32 value)
{
    switch (state)
    {
    case GFXRSSrcBlend:
    case GFXRSDestBlend:
    case GFXRSSrcBlendAlpha:
    case GFXRSDestBlendAlpha:
        mD3DDevice->SetRenderState(GFXD3DRenderState[state], GFXD3DBlend[value]);
        break;

    case GFXRSBlendOp:
    case GFXRSBlendOpAlpha:
        mD3DDevice->SetRenderState(GFXD3DRenderState[state], GFXD3DBlendOp[value]);
        break;

    case GFXRSStencilFail:
    case GFXRSStencilZFail:
    case GFXRSStencilPass:
    case GFXRSCCWStencilFail:
    case GFXRSCCWStencilZFail:
    case GFXRSCCWStencilPass:
        mD3DDevice->SetRenderState(GFXD3DRenderState[state], GFXD3DStencilOp[value]);
        break;

    case GFXRSZFunc:
    case GFXRSAlphaFunc:
    case GFXRSStencilFunc:
    case GFXRSCCWStencilFunc:
        mD3DDevice->SetRenderState(GFXD3DRenderState[state], GFXD3DCmpFunc[value]);
        break;

    case GFXRSCullMode:
        mD3DDevice->SetRenderState(GFXD3DRenderState[state], GFXD3DCullMode[value]);
        break;

    default:
        mD3DDevice->SetRenderState(GFXD3DRenderState[state], value);
        break;
    }
}

//-----------------------------------------------------------------------------

void GFXD3DDevice::clear(U32 flags, ColorI color, F32 z, U32 stencil)
{
    // Kind of a bummer we have to do this, there should be a better way made
    DWORD realflags = 0;

    if (flags & GFXClearTarget)
        realflags |= D3DCLEAR_TARGET;

    if (flags & GFXClearZBuffer)
        realflags |= D3DCLEAR_ZBUFFER;

    if (flags & GFXClearStencil)
        realflags |= D3DCLEAR_STENCIL;

    mD3DDevice->Clear(0, NULL, realflags, D3DCOLOR_ARGB(color.alpha, color.red, color.green, color.blue), z, stencil);
}

//-----------------------------------------------------------------------------

void GFXD3DDevice::beginScene()
{
    mD3DDevice->BeginScene();
    mCanCurrentlyRender = true;
}

//-----------------------------------------------------------------------------

void GFXD3DDevice::endScene()
{
    mD3DDevice->EndScene();
    mCanCurrentlyRender = false;
}

//-----------------------------------------------------------------------------
void GFXD3DDevice::setRenderTarget(GFXTextureObject* surf,
    U32 targetIndex,
    U32 mipLevel)

{
    if (surf == NULL)
    {
        mD3DDevice->SetRenderTarget(targetIndex, NULL);
        return;
    }

    GFXD3DTextureObject* renderTarget;
    renderTarget = static_cast<GFXD3DTextureObject*>(surf);
    LPDIRECT3DSURFACE9 tempSurface = NULL;
    D3DAssert(renderTarget->get2DTex()->GetSurfaceLevel(mipLevel, &tempSurface),
        "Failed to grab surface from texture");
    D3DAssert(mD3DDevice->SetRenderTarget(targetIndex, tempSurface), "Failed to set render target");

    // reset viewport data
    D3DSURFACE_DESC desc;
    tempSurface->GetDesc(&desc);
    SAFE_RELEASE(tempSurface);
    RectI vp(0, 0, desc.Width, desc.Height);
    setViewport(vp);
}

//-----------------------------------------------------------------------------
// setRTBackBuffer() - set the render target to be the back buffer
//-----------------------------------------------------------------------------
void GFXD3DDevice::setRTBackBuffer()
{
    IDirect3DSurface9* surf;
    mD3DDevice->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &surf);
    mD3DDevice->SetRenderTarget(0, surf);

    // reset viewport data
    D3DSURFACE_DESC desc;
    surf->GetDesc(&desc);
    surf->Release();
    RectI vp(0, 0, desc.Width, desc.Height);
    setViewport(vp);
}

//-----------------------------------------------------------------------------
// pushActiveRenderSurfaces()
//-----------------------------------------------------------------------------
void GFXD3DDevice::pushActiveRenderSurfaces()
{
    mRTStack.link(mCurrentRTData, mRTStack.first());
    mCurrentRTData.clear();
}

//-----------------------------------------------------------------------------
// popActiveRenderSurfaces()
//-----------------------------------------------------------------------------
void GFXD3DDevice::popActiveRenderSurfaces()
{
    RTStackElement RTData = *mRTStack.first();
    mRTStack.free(mRTStack.first());

    // Set render target to back buffer if stack is clear
    if (mRTStack.size() == 0 && RTData.renderTarget[0] == NULL)
    {
        mCurrentRTData = RTData;
        setRTBackBuffer();
        return;
    }

    // Otherwise, restore all targets that aren't already set
    for (U32 i = 0; i < MAX_MRT_TARGETS; i++)
    {
        if (RTData.renderTarget[i] != mCurrentRTData.renderTarget[i])
        {
            setRenderTarget(RTData.renderTarget[i], i, RTData.mipLevel[i]);
        }
    }

    mCurrentRTData = RTData;
}


//-----------------------------------------------------------------------------
// setActiveRenderSurface()
//-----------------------------------------------------------------------------
void GFXD3DDevice::setActiveRenderSurface(GFXTextureObject* surface, U32 renderTargetIndex, U32 mipLevel)
{
    if (surface == NULL)
    {
        // In debug, iterate over everything and make sure this isn't bound to a
        // texture sampler yet.
        for (S32 i = 0; i < TEXTURE_STAGE_COUNT; i++)
        {
            AssertFatal(mCurrentTexture[i] != mCurrentRTData.renderTarget[renderTargetIndex],
                "GFXD3DDevice::setActiveRenderSurface - Cannot have an active render target bound to a texture sampler!");
        }

        mCurrentRTData.renderTarget[renderTargetIndex] = NULL;

        // if ending render surface 0, then switch it back to the back buffer
        if (renderTargetIndex == 0)
        {
            setRTBackBuffer();
        }
        else
        {
            mD3DDevice->SetRenderTarget(renderTargetIndex, NULL);
        }
        return;
    }

    // In debug, iterate over everything and make sure this isn't bound to a
    // texture sampler yet.
    for (S32 i = 0; i < TEXTURE_STAGE_COUNT; i++)
    {
        AssertFatal(mCurrentTexture[i] != surface,
            "GFXD3DDevice::setActiveRenderSurface - Cannot have an active render target bound to a texture sampler!");
    }

    mCurrentRTData.renderTarget[renderTargetIndex] = surface;
    mCurrentRTData.mipLevel[renderTargetIndex] = mipLevel;

    setRenderTarget(surface, renderTargetIndex, mipLevel);
}


void GFXD3DDevice::pushActiveZSurface()
{
    IDirect3DSurface9* surface = NULL;
    D3DAssert(mD3DDevice->GetDepthStencilSurface(&surface), "Failed to get Z target");
    activeZStack.push_back(surface);
}

void GFXD3DDevice::popActiveZSurface()
{
    AssertFatal((activeZStack.size() > 0), "Popping empty stack!");

    D3DAssert(mD3DDevice->SetDepthStencilSurface(activeZStack.last()), "Failed to set Z target");
    activeZStack.last()->Release();
    activeZStack.pop_back();
}

void GFXD3DDevice::setActiveZSurface(GFXTextureObject* surface)
{
    if (!surface)
    {
        D3DAssert(mD3DDevice->SetDepthStencilSurface(NULL), "Failed to set null Z target");
        return;
    }

    GFXD3DTextureObject* renderTarget = static_cast<GFXD3DTextureObject*>(surface);

    AssertFatal((renderTarget->getSurface()), "Bad Z texture");

    D3DAssert(mD3DDevice->SetDepthStencilSurface(renderTarget->getSurface()), "Failed to set Z target");
}


//-----------------------------------------------------------------------------

// TODO: add a swapBufferRegions that does dirty rect stuff
void GFXD3DDevice::swapBuffers()
{
    AssertFatal(mRTStack.size() == 0, "Render target stack not balanced - probably missing an endActiveRenderSurface() call.");

    mD3DDevice->Present(NULL, NULL, NULL, NULL);

    bool killFramesAhead = Con::getBoolVariable("$pref::Video::killFramesAhead", false);
    if (killFramesAhead)
    {
        LPDIRECT3DSURFACE9 backBuff;
        mD3DDevice->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &backBuff);

        D3DLOCKED_RECT rect;
        backBuff->LockRect(&rect, NULL, 0);
        backBuff->UnlockRect();
        backBuff->Release();
    }


}

//-----------------------------------------------------------------------------

void GFXD3DDevice::setViewport(const RectI& rect)
{
    mViewportRect = rect;

    mViewport.X = mViewportRect.point.x;
    mViewport.Y = mViewportRect.point.y;
    mViewport.Width = mViewportRect.extent.x;
    mViewport.Height = mViewportRect.extent.y;
    mViewport.MinZ = 0.0;
    mViewport.MaxZ = 1.0;

    D3DAssert(mD3DDevice->SetViewport(&mViewport), "Error setting viewport");
}

//-----------------------------------------------------------------------------
#ifdef TORQUE_DEBUG

void GFXD3DDevice::logVertexBuffers()
{

    // NOTE: This function should be called on the destructor of this class and ONLY then
    // otherwise it'll produce the wrong output
    if (mNumAllocatedVertexBuffers == 0)
        return;

    FileStream fs;

    fs.open("vertexbuffer.log", FileStream::Write);

    char buff[256];

    fs.writeLine((U8*)avar("-- Vertex buffer memory leak report -- time = %d", Platform::getRealMilliseconds()));
    dSprintf((char*)&buff, sizeof(buff), "%d un-freed vertex buffers", mNumAllocatedVertexBuffers);
    fs.writeLine((U8*)buff);

    GFXD3DVertexBuffer* walk = mVBListHead;

    while (walk != NULL)
    {
        dSprintf((char*)&buff, sizeof(buff), "[Name: %s] Size: %d", walk->name, walk->mNumVerts);
        fs.writeLine((U8*)buff);

        walk = walk->next;
    }

    fs.writeLine((U8*)"-- End report --");

    fs.close();
}

//-----------------------------------------------------------------------------

void GFXD3DDevice::addVertexBuffer(GFXD3DVertexBuffer* buffer)
{
    mNumAllocatedVertexBuffers++;

    if (mVBListHead == NULL)
    {
        mVBListHead = buffer;
    }
    else
    {
        GFXD3DVertexBuffer* walk = mVBListHead;

        while (walk->next != NULL)
        {
            walk = walk->next;
        }

        walk->next = buffer;
    }

    buffer->next = NULL;
}

//-----------------------------------------------------------------------------

void GFXD3DDevice::removeVertexBuffer(GFXD3DVertexBuffer* buffer)
{
    mNumAllocatedVertexBuffers--;

    // Quick check to see if this is head of list
    if (mVBListHead == buffer)
    {
        mVBListHead = mVBListHead->next;
        return;
    }

    GFXD3DVertexBuffer* walk = mVBListHead;

    while (walk->next != NULL)
    {
        if (walk->next == buffer)
        {
            walk->next = walk->next->next;
            return;
        }

        walk = walk->next;
    }

    AssertFatal(false, "Vertex buffer not found in list.");
}

#endif

//-----------------------------------------------------------------------------

void GFXD3DDevice::releaseDefaultPoolResources()
{
    // Release all the dynamic vertex buffer arrays
    // Forcibly clean up the pools
    for (U32 i = 0; i < mVolatileVBList.size(); i++)
    {
        // Con::printf("Trying to release vb with COM refcount of %d and internal refcount of %d", mVolatileVBList[i]->vb->AddRef() - 1, mVolatileVBList[i]->mRefCount);  
        // mVolatileVBList[i]->vb->Release();

        mVolatileVBList[i]->vb->Release();
        mVolatileVBList[i]->vb = NULL;
        mVolatileVBList[i] = NULL;
    }
    mVolatileVBList.setSize(0);

    // free dynamic vertex buffers
    for (U32 i = 0; i < mDynamicVBList.size(); i++)
    {
        GFXD3DVertexBuffer* vertBuff = mDynamicVBList[i];
        vertBuff->vb->Release();
        vertBuff->vb = NULL;
    }

    // Set current VB to NULL and set state dirty
    mCurrentVertexBuffer = NULL;
    mVertexBufferDirty = true;

    // Release dynamic index buffer
    if (mDynamicPB != NULL)
    {
        SAFE_RELEASE(mDynamicPB->ib);
    }

    // Set current PB/IB to NULL and set state dirty
    mCurrentPrimitiveBuffer = NULL;
    mCurrentPB = NULL;
    mPrimitiveBufferDirty = true;

    // Zombify texture manager (for D3D this only modifies default pool textures)
    if (mTextureManager)
        mTextureManager->zombify();

    // Set global dirty state so the IB/PB and VB get reset
    mStateDirty = true;
}

//-----------------------------------------------------------------------------

void GFXD3DDevice::reacquireDefaultPoolResources()
{

    // Now do the dynamic index buffers
    if (mDynamicPB == NULL)
    {
        mDynamicPB = new GFXD3DPrimitiveBuffer(this, 0, 0, GFXBufferTypeDynamic);
    }

    // automatically rellocate dynamic vertex buffers
    for (U32 i = 0; i < mDynamicVBList.size(); i++)
    {
        GFXD3DVertexBuffer* vertBuff = mDynamicVBList[i];

        D3DAssert(mD3DDevice->CreateVertexBuffer(vertBuff->mVertexSize * vertBuff->mNumVerts,
            D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY,
            vertBuff->mVertexType,
            D3DPOOL_DEFAULT,
            &vertBuff->vb,
            NULL),
            "Failed to allocate VB");
    }



    D3DAssert(mD3DDevice->CreateIndexBuffer(sizeof(U16) * MAX_DYNAMIC_INDICES, D3DUSAGE_WRITEONLY | D3DUSAGE_DYNAMIC,
        GFXD3DIndexFormat[GFXIndexFormat16], D3DPOOL_DEFAULT, &mDynamicPB->ib, NULL), "Failed to allocate dynamic IB");

    // Ressurect texture manager (for D3D this only modifies default pool textures)
    mTextureManager->resurrect();
}

//-----------------------------------------------------------------------------
GFXD3DVertexBuffer* GFXD3DDevice::findVBPool(U32 vertFlags)
{
    for (U32 i = 0; i < mVolatileVBList.size(); i++)
    {
        if (mVolatileVBList[i]->mVertexType == vertFlags)
        {
            return mVolatileVBList[i];
        }
    }

    return NULL;
}

//-----------------------------------------------------------------------------
GFXD3DVertexBuffer* GFXD3DDevice::createVBPool(U32 vertFlags, U32 vertSize)
{
    // this is a bit funky, but it will avoid problems with (lack of) copy constructors
    //    with a push_back() situation
    mVolatileVBList.increment();
    RefPtr<GFXD3DVertexBuffer> newBuff;
    mVolatileVBList.last() = new GFXD3DVertexBuffer();
    newBuff = mVolatileVBList.last();

    newBuff->mNumVerts = 0;
    newBuff->mBufferType = GFXBufferTypeVolatile;
    newBuff->mVertexType = vertFlags;
    newBuff->mVertexSize = vertSize;
    newBuff->mDevice = this;

    //   Con::printf("Created buff with type %x", vertFlags);

    D3DAssert(mD3DDevice->CreateVertexBuffer(vertSize * MAX_DYNAMIC_VERTS,
        D3DUSAGE_WRITEONLY | D3DUSAGE_DYNAMIC,
        vertFlags,
        D3DPOOL_DEFAULT,
        &newBuff->vb,
        NULL),
        "Failed to allocate dynamic VB");
    return newBuff;
}

//-----------------------------------------------------------------------------

void GFXD3DDevice::setClipRect(const RectI& rect)
{
    mClipRect = rect;

    F32 left = mClipRect.point.x;
    F32 right = mClipRect.point.x + mClipRect.extent.x;
    F32 bottom = mClipRect.point.y + mClipRect.extent.y;
    F32 top = mClipRect.point.y;

    // Set up projection matrix
    D3DXMatrixOrthoOffCenterLH((D3DXMATRIX*)&mTempMatrix, left, right, bottom, top, 0.f, 1.f);
    mTempMatrix.transpose();

    setProjectionMatrix(mTempMatrix);

    // Set up world/view matrix
    mTempMatrix.identity();
    setViewMatrix(mTempMatrix);
    setWorldMatrix(mTempMatrix);

    setViewport(mClipRect);
}

//-----------------------------------------------------------------------------

void GFXD3DDevice::setVB(GFXVertexBuffer* buffer)
{
    AssertFatal(mCurrentOpenAllocVB == NULL, "Calling setVertexBuffer() when a vertex buffer is still open for editing");

    mCurrentVB = static_cast<GFXD3DVertexBuffer*>(buffer);

    D3DAssert(mD3DDevice->SetStreamSource(0, mCurrentVB->vb, 0, mCurrentVB->mVertexSize), "Failed to set stream source");
    D3DAssert(mD3DDevice->SetFVF(mCurrentVB->mVertexType), "Failed to set FVF");
}

//-----------------------------------------------------------------------------

void GFXD3DDevice::setPrimitiveBuffer(GFXPrimitiveBuffer* buffer)
{
    AssertFatal(mCurrentOpenAllocPB == NULL, "Calling setIndexBuffer() when a index buffer is still open for editing");

    mCurrentPB = static_cast<GFXD3DPrimitiveBuffer*>(buffer);

    D3DAssert(mD3DDevice->SetIndices(mCurrentPB->ib), "Failed to set indices");
}

//-----------------------------------------------------------------------------

void GFXD3DDevice::drawPrimitive(GFXPrimitiveType primType, U32 vertexStart, U32 primitiveCount)
{
    // This is d one to avoid the function call overhead if possible
    if (mStateDirty)
        updateStates();

    AssertFatal(mCurrentOpenAllocVB == NULL, "Calling drawPrimitive() when a vertex buffer is still open for editing");
    AssertFatal(mCurrentVB != NULL, "Trying to call draw primitive with no current vertex buffer, call setVertexBuffer()");

    D3DAssert(mD3DDevice->DrawPrimitive(GFXD3DPrimType[primType], mCurrentVB->mVolatileStart + vertexStart, primitiveCount), "Failed to draw primitives");
}

//-----------------------------------------------------------------------------

void GFXD3DDevice::drawIndexedPrimitive(GFXPrimitiveType primType, U32 minIndex, U32 numVerts, U32 startIndex, U32 primitiveCount)
{
    // This is done to avoid the function call overhead if possible
    if (mStateDirty)
        updateStates();

    AssertFatal(mCurrentOpenAllocVB == NULL, "Calling drawIndexedPrimitive() when a vertex buffer is still open for editing");
    AssertFatal(mCurrentVB != NULL, "Trying to call drawIndexedPrimitive with no current vertex buffer, call setVertexBuffer()");

    AssertFatal(mCurrentOpenAllocPB == NULL, "Calling drawIndexedPrimitive() when a index buffer is still open for editing");
    AssertFatal(mCurrentPB != NULL, "Trying to call drawIndexedPrimitive with no current index buffer, call setIndexBuffer()");

    D3DAssert(mD3DDevice->DrawIndexedPrimitive(GFXD3DPrimType[primType], mCurrentVB->mVolatileStart, /* mCurrentPB->mVolatileStart + */ minIndex, numVerts, mCurrentPB->mVolatileStart + startIndex, primitiveCount), "Failed to draw indexed primitive");
}


//-----------------------------------------------------------------------------
// Project point - as in gluProject()
//-----------------------------------------------------------------------------
void GFXD3DDevice::project(Point3F& outPoint,
    Point3F& inPoint,
    MatrixF& modelview,
    MatrixF& projection,
    RectI& viewport)
{
    D3DVIEWPORT9 D3Dviewport;
    D3Dviewport.X = viewport.point.x;
    D3Dviewport.Y = viewport.point.y;
    D3Dviewport.Width = viewport.extent.x - viewport.point.x;
    D3Dviewport.Height = viewport.extent.y - viewport.point.y;
    D3Dviewport.MinZ = 0.0;
    D3Dviewport.MaxZ = 1.0;

    MatrixF p, m;
    p = projection;
    p.transpose();

    m = modelview;
    m.transpose();

    MatrixF view(true);

    D3DXVec3Project((D3DXVECTOR3*)&outPoint, (D3DXVECTOR3*)&inPoint, &D3Dviewport,
        (D3DXMATRIX*)&p, (D3DXMATRIX*)&view, (D3DXMATRIX*)&m);

}

//-----------------------------------------------------------------------------
// Project point - as in gluProject()
//-----------------------------------------------------------------------------
void GFXD3DDevice::unProject(Point3F& outPoint,
    Point3F& inPoint,
    MatrixF& modelview,
    MatrixF& projection,
    RectI& viewport)
{

    D3DVIEWPORT9 D3Dviewport;
    D3Dviewport.X = viewport.point.x;
    D3Dviewport.Y = viewport.point.y;
    D3Dviewport.Width = viewport.extent.x - viewport.point.x;
    D3Dviewport.Height = viewport.extent.y - viewport.point.y;
    D3Dviewport.MinZ = 0.0;
    D3Dviewport.MaxZ = 1.0;

    MatrixF p, m;
    p = projection;
    p.transpose();

    m = modelview;
    m.transpose();

    MatrixF view(true);

    D3DXVec3Unproject((D3DXVECTOR3*)&outPoint, (D3DXVECTOR3*)&inPoint, &D3Dviewport,
        (D3DXMATRIX*)&p, (D3DXMATRIX*)&view, (D3DXMATRIX*)&m);

}

//-----------------------------------------------------------------------------
// Create shader - for D3D.  Returns NULL if cannot create.
//-----------------------------------------------------------------------------
GFXShader* GFXD3DDevice::createShader(char* vertFile,
    char* pixFile,
    F32 pixVersion)
{
    return (GFXShader*)mShaderMgr.createShader(vertFile, pixFile, pixVersion);
}

//-----------------------------------------------------------------------------
GFXShader* GFXD3DDevice::createShader(GFXShaderFeatureData& featureData,
    GFXVertexFlags vertFlags)
{
    return mShaderMgr.getShader(featureData, vertFlags);
}

//-----------------------------------------------------------------------------
void GFXD3DDevice::destroyShader(GFXShader* shader)
{
    mShaderMgr.destroyShader(shader);
}

//-----------------------------------------------------------------------------
// Disable shaders
//-----------------------------------------------------------------------------
void GFXD3DDevice::disableShaders()
{
    //   mD3DDevice->SetVertexShader( NULL );
    //   mD3DDevice->SetPixelShader( NULL );

    setShader(NULL);
}

IDirect3DVertexShader9* lastVertShader = NULL;
IDirect3DPixelShader9* lastPixShader = NULL;

//-----------------------------------------------------------------------------
// Set shader - this function exists to make sure this is done in one place,
//              and to make sure redundant shader states are not being
//              sent to the card.
//-----------------------------------------------------------------------------
void GFXD3DDevice::setShader(GFXShader* shader)
{
    GFXD3DShader* d3dShader = static_cast<GFXD3DShader*>(shader);

    IDirect3DPixelShader9* pixShader = d3dShader ? d3dShader->pixShader : NULL;
    IDirect3DVertexShader9* vertShader = d3dShader ? d3dShader->vertShader : NULL;

    if (pixShader != lastPixShader)
    {
        mD3DDevice->SetPixelShader(pixShader);
        lastPixShader = pixShader;
    }

    if (vertShader != lastVertShader)
    {
        mD3DDevice->SetVertexShader(vertShader);
        lastVertShader = vertShader;
    }

}

//-----------------------------------------------------------------------------
// Set vertex shader constant
//-----------------------------------------------------------------------------
void GFXD3DDevice::setVertexShaderConstF(U32 reg, const float* data, U32 size)
{
    mD3DDevice->SetVertexShaderConstantF(reg, data, size);
}

//-----------------------------------------------------------------------------
// Set pixel shader constant
//-----------------------------------------------------------------------------
void GFXD3DDevice::setPixelShaderConstF(U32 reg, const float* data, U32 size)
{
    mD3DDevice->SetPixelShaderConstantF(reg, data, size);
}

//-----------------------------------------------------------------------------
// Returns the number of texture samplers that can be used in a shader
// rendering pass.
//
// The numbers come from the table in the D3D Docs on the page titled:
// "Confirming Pixel Shader Support"
//-----------------------------------------------------------------------------
U32 GFXD3DDevice::getNumSamplers()
{
    F32 shaderVersion = getPixelShaderVersion();

    if (shaderVersion > 0.0)
    {
        if (shaderVersion < 2.0)
            return 4;
        else
            return 16;
    }
    else
    {
        D3DCAPS9 caps;
        mD3DDevice->GetDeviceCaps(&caps);
        return caps.MaxTextureBlendStages / caps.MaxSimultaneousTextures;
    }
}

//-----------------------------------------------------------------------------
// allocPrimitiveBuffer
//-----------------------------------------------------------------------------
GFXPrimitiveBuffer* GFXD3DDevice::allocPrimitiveBuffer(U32 numIndices, U32 numPrimitives, GFXBufferType bufferType)
{
    // Allocate a buffer to return
    GFXD3DPrimitiveBuffer* res = new GFXD3DPrimitiveBuffer(this, numIndices, numPrimitives, bufferType);

    // Determine usage flags
    U32 usage = 0;
    D3DPOOL pool; //  = 0;

    // Assumptions:
    //    - static buffers are write once, use many
    //    - dynamic buffers are write many, use many
    //    - volatile buffers are write once, use once
    // You may never read from a buffer.
    switch (bufferType)
    {
    case GFXBufferTypeStatic:
        pool = D3DPOOL_MANAGED;
        break;

    case GFXBufferTypeDynamic:
        AssertISV(false, "D3D does not support dynamic primitive buffers. -- BJG");
        usage |= D3DUSAGE_DYNAMIC;
        break;

    case GFXBufferTypeVolatile:
        pool = D3DPOOL_DEFAULT;
        break;
    }

    // We never allow reading from a vert buffer.
    usage |= D3DUSAGE_WRITEONLY;

    // Create d3d index buffer
    if (bufferType == GFXBufferTypeVolatile)
    {
        // Get it from the pool if it's a volatile...
        AssertFatal(numIndices < MAX_DYNAMIC_INDICES, "Cannot allocate that many indices in a volatile buffer, increase MAX_DYNAMIC_INDICES.");

        res->ib = mDynamicPB->ib;
        // mDynamicPB->ib->AddRef();
        res->mVolatileBuffer = mDynamicPB;
    }
    else
    {
        // Otherwise, get it as a seperate buffer...
        D3DAssert(mD3DDevice->CreateIndexBuffer(sizeof(U16) * numIndices, usage, GFXD3DIndexFormat[GFXIndexFormat16], pool, &res->ib, 0),
            "Failed to allocate an index buffer.");
    }

    // Return buffer
    return res;
}

//-----------------------------------------------------------------------------
// allocVertexBuffer
//-----------------------------------------------------------------------------
GFXVertexBuffer* GFXD3DDevice::allocVertexBuffer(U32 numVerts, U32 vertFlags, U32 vertSize, GFXBufferType bufferType)
{
    GFXD3DVertexBuffer* res = new GFXD3DVertexBuffer(this, numVerts, vertFlags, vertSize, bufferType);

    // Determine usage flags
    U32 usage = 0;
    D3DPOOL pool; //  = 0;

    res->mNumVerts = 0;

    // Assumptions:
    //    - static buffers are write once, use many
    //    - dynamic buffers are write many, use many
    //    - volatile buffers are write once, use once
    // You may never read from a buffer.
    switch (bufferType)
    {
    case GFXBufferTypeStatic:
        pool = D3DPOOL_MANAGED;
        break;

    case GFXBufferTypeDynamic:
    case GFXBufferTypeVolatile:
        pool = D3DPOOL_DEFAULT;
        usage |= D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY;
        break;
    }

    // Create vertex buffer
    if (bufferType == GFXBufferTypeVolatile)
    {
        // Get volatile stuff from a pool...
        AssertFatal(numVerts < MAX_DYNAMIC_VERTS, "Cannot allocate that many verts in a volatile vertex buffer, increase MAX_DYNAMIC_VERTS! -- BJG");

        // This is all we need here, everything else lives in the lock method on the 
        // buffer... -- BJG

    }
    else
    {
        // Get a new buffer...
        D3DAssert(mD3DDevice->CreateVertexBuffer(vertSize * numVerts, usage, vertFlags, pool, &res->vb, NULL),
            "Failed to allocate VB");

        if (bufferType == GFXBufferTypeDynamic)
        {
            mDynamicVBList.push_back(res);
        }

    }


    res->mNumVerts = numVerts;
    return res;
}

//-----------------------------------------------------------------------------
// deallocate vertex buffer
//-----------------------------------------------------------------------------
void GFXD3DDevice::deallocVertexBuffer(GFXD3DVertexBuffer* vertBuff)
{
    for (U32 i = 0; i < mDynamicVBList.size(); i++)
    {
        if (mDynamicVBList[i] == vertBuff)
        {
            mDynamicVBList[i] = mDynamicVBList.last();
            mDynamicVBList.decrement();
        }
    }
}

void GFXD3DDevice::resurrectTextureManager()
{
    mTextureManager->resurrect();
}

void GFXD3DDevice::zombifyTextureManager()
{
    mTextureManager->zombify();
}

//-----------------------------------------------------------------------------
// This function should ONLY be called from GFXDevice::updateStates() !!!
//-----------------------------------------------------------------------------
void GFXD3DDevice::setTextureInternal(U32 textureUnit, const GFXTextureObject* texture)
{
    if (texture == NULL)
    {
        D3DAssert(mD3DDevice->SetTexture(textureUnit, NULL), "Failed to set texture to null!");
        return;
    }

    GFXD3DTextureObject* tex = (GFXD3DTextureObject*)texture;
    D3DAssert(mD3DDevice->SetTexture(textureUnit, tex->getTex()), "Failed to set texture to valid value!");
}


//-----------------------------------------------------------------------------
// Copy back buffer to Sfx Back Buffer
//-----------------------------------------------------------------------------
void GFXD3DDevice::copyBBToSfxBuff()
{
    if (!mSfxBackBuffer)
    {
        mSfxBackBuffer.set(SFXBBCOPY_SIZE, SFXBBCOPY_SIZE, GFXFormatR8G8B8, &GFXDefaultRenderTargetProfile);
    }

    IDirect3DSurface9* backBuffer;
    if ((mRTStack.size() < 1) || (!mCurrentRTData.renderTarget[0]))
        mD3DDevice->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &backBuffer);
    else
    {
        GFXD3DTextureObject* rt = (GFXD3DTextureObject*)(GFXTextureObject*)mCurrentRTData.renderTarget[0];
        AssertFatal((rt), "Invalid render target.");
        rt->get2DTex()->GetSurfaceLevel(0, &backBuffer);
    }

    IDirect3DSurface9* surf;
    GFXD3DTextureObject* texObj = (GFXD3DTextureObject*)(GFXTextureObject*)mSfxBackBuffer;
    texObj->get2DTex()->GetSurfaceLevel(0, &surf);
    mD3DDevice->StretchRect(backBuffer, NULL, surf, NULL, D3DTEXF_NONE);

    surf->Release();
    backBuffer->Release();
}

//-----------------------------------------------------------------------------
// Initialize - create window, device, etc
//-----------------------------------------------------------------------------
void GFXD3DDevice::init(const GFXVideoMode& mode)
{
#ifdef TORQUE_SHIPPING
    // Check DX Version here, bomb if we don't have the minimum. 
    // Check platformWin32/dxVersionChecker.cpp for more information/configuration.
    AssertISV(checkDXVersion(), "Minimum DirectX version required to run this application not found. Quitting.");
#endif

    // Set up the Enum translation tables
    GFXD3DEnumTranslate::init();

    mVideoMode = mode;

    // Create D3D Presentation params
    D3DPRESENT_PARAMETERS d3dpp = setupPresentParams(mode);


#ifdef TORQUE_NVPERFHUD
    HRESULT hres = mD3D->CreateDevice(mD3D->GetAdapterCount() - 1, D3DDEVTYPE_REF, winState.appWindow,
        D3DCREATE_MIXED_VERTEXPROCESSING | D3DCREATE_MULTITHREADED,
        &d3dpp, &mD3DDevice);
#else

    // Vertex processing was changed from MIXED to HARDWARE because of the switch to a pure D3D device.
    // If this causes problems, please let me know: patw@garagegames.com

    // Set up device flags from our compile flags.
    U32 deviceFlags = 0;
    deviceFlags = D3DCREATE_HARDWARE_VERTEXPROCESSING;

    // Add the multithread flag if we're a multithread build.
#ifdef TORQUE_MULTITHREAD
    deviceFlags |= D3DCREATE_MULTITHREADED;
#endif

    // Try to do pure, unless we're doing debug (and thus doing more paranoid checking).
#ifndef TORQUE_DEBUG_RENDER
    deviceFlags |= D3DCREATE_PUREDEVICE;
#endif

    HRESULT hres = mD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL,
        winState.appWindow, deviceFlags,
        &d3dpp, &mD3DDevice);
#endif

    //regenStates();

    // Gracefully die if they can't give us a device.
    if (!mD3DDevice)
    {
        Platform::AlertOK("DirectX Error!", "Failed to initialize Direct3D! Make sure you have DirectX 9 installed, and "
            "are running a graphics card that supports Pixel Shader 1.1.");
        Platform::forceShutdown(1);
    }

    // Check up on things
    Con::printf("   Cur. D3DDevice ref count=%d", mD3DDevice->AddRef() - 1);
    mD3DDevice->Release();

    mTextureManager = new GFXD3DTextureManager(mD3DDevice);

    // Now re aquire all the resources we trashed earlier
    reacquireDefaultPoolResources();

    // Setup default states
    initStates();

    //-------- Output init info ---------   
    D3DCAPS9 caps;
    mD3DDevice->GetDeviceCaps(&caps);

    U8* pxPtr = (U8*)&caps.PixelShaderVersion;
    mPixVersion = pxPtr[1] + pxPtr[0] * 0.1;
    Con::printf("   Pix version detected: %f", mPixVersion);

    bool forcePixVersion = Con::getBoolVariable("$pref::Video::forcePixVersion", false);
    if (forcePixVersion)
    {
        float forcedPixVersion = Con::getFloatVariable("$pref::Video::forcedPixVersion", mPixVersion);
        if (forcedPixVersion < mPixVersion)
        {
            mPixVersion = forcedPixVersion;
            Con::errorf("   Forced pix version: %f", mPixVersion);
        }
    }

    U8* vertPtr = (U8*)&caps.VertexShaderVersion;
    F32 vertVersion = vertPtr[1] + vertPtr[0] * 0.1;
    Con::printf("   Vert version detected: %f", vertVersion);

    mCardProfiler = new GFXD3DCardProfiler();
    mCardProfiler->init();

    gScreenShot = new ScreenShotD3D;
}

//-----------------------------------------------------------------------------
// Activate
//-----------------------------------------------------------------------------
void GFXD3DDevice::activate()
{
    D3DPRESENT_PARAMETERS d3dpp = setupPresentParams(getVideoMode());

    reset(d3dpp);
}

//-----------------------------------------------------------------------------
// Setup D3D present parameters - init helper function
//-----------------------------------------------------------------------------
D3DPRESENT_PARAMETERS GFXD3DDevice::setupPresentParams(const GFXVideoMode& mode)
{
    // Create D3D Presentation params
    D3DPRESENT_PARAMETERS d3dpp;
    dMemset(&d3dpp, 0, sizeof(d3dpp));

    D3DFORMAT fmt = D3DFMT_X8R8G8B8; // 32 bit

    if (mode.bitDepth == 16)
    {
        fmt = D3DFMT_R5G6B5;
    }

    D3DDISPLAYMODE displayMode;
    mD3D->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &displayMode);

    d3dpp.BackBufferWidth = mode.resolution.x;
    d3dpp.BackBufferHeight = mode.resolution.y;
    d3dpp.BackBufferFormat = fmt;
    d3dpp.BackBufferCount = 1;
    d3dpp.MultiSampleType = D3DMULTISAMPLE_NONE;
    d3dpp.MultiSampleQuality = 0;
    d3dpp.SwapEffect = D3DSWAPEFFECT_FLIP;
    d3dpp.hDeviceWindow = winState.appWindow;
    d3dpp.Windowed = !mode.fullScreen;
    d3dpp.EnableAutoDepthStencil = TRUE;
    d3dpp.AutoDepthStencilFormat = D3DFMT_D24S8;
    d3dpp.Flags = 0;
    d3dpp.FullScreen_RefreshRateInHz = D3DPRESENT_RATE_DEFAULT;
    d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_ONE;

    if (Con::getBoolVariable("$pref::Video::killFramesAhead", false))
    {
        d3dpp.Flags |= D3DPRESENTFLAG_LOCKABLE_BACKBUFFER;
    }

    if (Con::getBoolVariable("$pref::Video::disableVerticalSync", true))
    {
        d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;	// This does NOT wait for vsync
    }

    return d3dpp;
}

//-----------------------------------------------------------------------------
// Reset D3D device
//-----------------------------------------------------------------------------
void GFXD3DDevice::reset(D3DPRESENT_PARAMETERS& d3dpp)
{
    // First release all the stuff we allocated from D3DPOOL_DEFAULT
    releaseDefaultPoolResources();

    // reset device   
    Con::printf("--- Resetting D3D Device ---");
    HRESULT hres = S_OK;
    hres = mD3DDevice->Reset(&d3dpp);

    if (FAILED(hres))
    {
        while (mD3DDevice->TestCooperativeLevel() == D3DERR_DEVICELOST)
        {
            Sleep(100);
        }

        hres = mD3DDevice->Reset(&d3dpp);
    }

    D3DAssert(hres, "Failed to create D3D Device");

    // Now re aquire all the resources we trashed earlier
    reacquireDefaultPoolResources();

    // Setup default states
    initStates();

    // Mark everything dirty and flush to card, for sanity.
    GFX->updateStates(true);
}

//-----------------------------------------------------------------------------
// Get a string indicating the installed DirectX version, revision and letter number
//-----------------------------------------------------------------------------
char* GFXD3DDevice::getDXVersion()
{
#ifdef UNICODE
    UTF16 dxVersionLetter;
#else
    char dxVersionLetter;
#endif
    char versionLetter[4];

    static char dxVersionString[32];

    DWORD dxVersion, dxRevision;

    NVDXDiagWrapper::DXDiagNVUtil* dxDiag = new NVDXDiagWrapper::DXDiagNVUtil();
    dxDiag->InitIDxDiagContainer();
    dxDiag->GetDirectXVersion(&dxVersion, &dxRevision, &dxVersionLetter);
    dxDiag->FreeIDxDiagContainer();
    delete dxDiag;

#ifdef UNICODE
    convertUTF16toUTF8(&dxVersionLetter, (UTF8*)&versionLetter, sizeof(versionLetter));
#else
    versionLetter = dxVersionLetter;
#endif

    dSprintf(dxVersionString, sizeof(dxVersionString), "%d.%d%c", dxVersion, dxRevision, versionLetter);

    return dxVersionString;
}

//-----------------------------------------------------------------------------
// Enumerate D3D adapters
//-----------------------------------------------------------------------------
void GFXD3DDevice::enumerateAdapters(Vector<GFXAdapter>& adapterList)
{
    // Grab a D3D9 handle here to first get the D3D9 devices
    LPDIRECT3D9 d3d9 = Direct3DCreate9(D3D_SDK_VERSION);

    AssertISV(d3d9, "Could not create D3D object, make sure you have the latest version of DirectX installed");

    // Print DX version ASAP - this is not at top of function because it has not
    // been tested with DX 8, so it may crash and the user would not get a dialog box...
    Con::printf("DirectX version - %s", getDXVersion());

    for (U32 i = 0; i < d3d9->GetAdapterCount(); i++)
    {
        GFXAdapter toAdd;
        toAdd.type = Direct3D9;
        toAdd.index = i;

        D3DADAPTER_IDENTIFIER9 temp;

        d3d9->GetAdapterIdentifier(i, NULL, &temp); // The NULL is the flags which deal with WHQL

        // Change this when we get string library
        dStrcpy(toAdd.name, temp.Description);

        adapterList.push_back(toAdd);
    }

    d3d9->Release();
}
//------------------------------------------------------------------------------
void GFXD3DDevice::enterDebugEvent(ColorI color, const char* name)
{
    // We aren't really Unicode compliant (boo hiss BJGFIX) so we get the
    // dubious pleasure of runtime string conversion! :)
    WCHAR  eventName[260];
    MultiByteToWideChar(CP_ACP, 0, name, -1, eventName, 260);

    D3DPERF_BeginEvent(D3DCOLOR_ARGB(color.alpha, color.red, color.green, color.blue),
        (LPCWSTR)&eventName);
}

//------------------------------------------------------------------------------
void GFXD3DDevice::leaveDebugEvent()
{
    D3DPERF_EndEvent();
}

//------------------------------------------------------------------------------
void GFXD3DDevice::setDebugMarker(ColorI color, const char* name)
{
    // We aren't really Unicode compliant (boo hiss BJGFIX) so we get the
    // dubious pleasure of runtime string conversion! :)
    WCHAR  eventName[260];
    MultiByteToWideChar(CP_ACP, 0, name, -1, eventName, 260);

    D3DPERF_SetMarker(D3DCOLOR_ARGB(color.alpha, color.red, color.green, color.blue),
        (LPCWSTR)&eventName);
}

//------------------------------------------------------------------------------
// Check for texture mis-match between GFX internal state and what is on the card
// This function is expensive because of the readbacks from DX, and additionally
// won't work unless it's a non-pure device.
//
// This function can crash or give false positives when the game
// is shutting down or returning to the main menu as some of the textures
// present in the mCurrentTexture array will have been freed.
//
// This function is best used as a quick check for mismatched state when it is
// suspected.
//------------------------------------------------------------------------------
void GFXD3DDevice::doParanoidStateCheck()
{
    // Read back all states and make sure they match what we think they should be.

    // For now just do texture binds.
    for (U32 i = 0; i < TEXTURE_STAGE_COUNT; i++)
    {
        IDirect3DBaseTexture9* b = NULL;
        getDevice()->GetTexture(i, &b);
        if (mCurrentTexture[i] == NULL)
        {
            AssertFatal(b == NULL, "GFXD3DDevice::doParanoidStateCheck - got non-null texture in expected NULL slot!");
            getDevice()->SetTexture(i, NULL);
        }
        else
        {
            AssertFatal(mCurrentTexture[i], "GFXD3DDevice::doParanoidStateCheck - got null texture in expected non-null slot!");
            if (dynamic_cast<GFXD3DCubemap*>((GFXCubemap*)mCurrentTexture[i]))
            {
                IDirect3DCubeTexture9* cur = ((GFXD3DCubemap*)mCurrentTexture[i])->mCubeTex;
                AssertFatal(cur == b, "GFXD3DDevice::doParanoidStateCheck - mismatched cubemap!");
            }
            else
            {
                IDirect3DBaseTexture9* cur = ((GFXD3DTextureObject*)mCurrentTexture[i])->getTex();
                AssertFatal(cur == b, "GFXD3DDevice::doParanoidStateCheck - mismatched 2d texture!");
            }
        }

        SAFE_RELEASE(b);
    }
}