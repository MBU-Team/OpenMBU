//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------
#include "gammaBuffer.h"
#include "console/consoleTypes.h"
#include "gfx/gfxDevice.h"
#include "primBuilder.h"
#include "core/bitStream.h"
#include "../../example/shaders/shdrConsts.h"

#define GAMMA_BUFF_SIZE 256

IMPLEMENT_CONOBJECT(GammaBuffer);

//****************************************************************************
// Gamma Buffer
//****************************************************************************


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------
GammaBuffer::GammaBuffer()
{
    mGammaShader = NULL;
    mGammaShaderName = NULL;
    mCallbackHandle = -1;
    mDisabled = false;
}

//--------------------------------------------------------------------------
// Init fields
//--------------------------------------------------------------------------
void GammaBuffer::initPersistFields()
{
    Parent::initPersistFields();

    addField("shader", TypeString, Offset(mGammaShaderName, GammaBuffer));

}

//--------------------------------------------------------------------------
// onAdd
//--------------------------------------------------------------------------
bool GammaBuffer::onAdd()
{
    if (!Parent::onAdd())
        return false;

    init();

    return true;
}

//--------------------------------------------------------------------------
// onRemove
//--------------------------------------------------------------------------
void GammaBuffer::onRemove()
{
    if (GFXDevice::devicePresent())
    {
        GFX->unregisterTexCallback(mCallbackHandle);
    }

    Parent::onRemove();
}


//--------------------------------------------------------------------------
// Init
//--------------------------------------------------------------------------
void GammaBuffer::init()
{
    if (Con::getBoolVariable("$pref::Video::disableGammaBuffer", false))
    {
        mDisabled = true;
        return;
    }

    if (GFXDevice::devicePresent() && mCallbackHandle == -1)
    {
        GFX->registerTexCallback(texManagerCallback, (void*)this, mCallbackHandle);
    }

    GFXVideoMode vm = GFX->getVideoMode();

    mSurface.set(vm.resolution.x, vm.resolution.y, GFXFormatR8G8B8A8, &GFXDefaultRenderTargetProfile, 1);



    


    for (U32 i = 0; i < 256; ++i) {
        U32 value = i * 1023 / 255;
        mGammaRamp.normal[i].value = value | (value << 10) | (value << 20);
    }
    for (U32 i = 0; i < 128; ++i) {
        U32 value = (i * 65535 / 127) & ~63;
        if (i < 127) {
            value |= 0x200 << 16;
        }
        for (U32 j = 0; j < 3; ++j) {
            mGammaRamp.pwl[i].values[j].value = value;
        }
    }

    // swap
    for (uint32_t i = 0; i < 256; ++i) {
        uint32_t value = mGammaRamp.normal[i].value;
        // Swap red and blue (Project Sylpheed has settings allowing separate
        // configuration).
        mMapping[i] = ((value & 1023) << 20) | (value & (1023 << 10)) | ((value >> 20) & 1023);
    }

    for (uint32_t i = 0; i < 128; ++i) {
        // TODO(Triang3l): Find a game to test if red and blue need to be swapped.
        mMappingPWL[i] = (mGammaRamp.pwl[i].values[0].base >> 6) |
            (uint32_t(mGammaRamp.pwl[i].values[1].base >> 6) << 10) |
            (uint32_t(mGammaRamp.pwl[i].values[2].base >> 6) << 20);
    }

    GBitmap bitmap(256, 1);
    for (U32 i = 0; i < 256; i++)
    {
        U8 alpha = (mMapping[i] >> 24) & 0xFF;
        U8 red = (mMapping[i] >> 16) & 0xFF;
        U8 green = (mMapping[i] >> 8) & 0xFF;
        U8 blue = (mMapping[i] >> 0) & 0xFF;
        ColorI color = ColorI(red, green, blue, alpha); 
        bitmap.setColor(i, 0, color);
    }
    mGamma.set(&bitmap, &GFXDefaultStaticDiffuseProfile, false);

    GBitmap bitmapPWL(128, 1);
    for (U32 i = 0; i < 128; i++)
    {
        U8 alpha = (mMappingPWL[i] >> 24) & 0xFF;
        U8 red = (mMappingPWL[i] >> 16) & 0xFF;
        U8 green = (mMappingPWL[i] >> 8) & 0xFF;
        U8 blue = (mMappingPWL[i] >> 0) & 0xFF;
        ColorI color = ColorI(red, green, blue, alpha);
        bitmapPWL.setColor(i, 0, color);
    }
    mGammaPWL.set(&bitmapPWL, &GFXDefaultStaticDiffuseProfile, false);

    mGammaShader = static_cast<ShaderData*>(Sim::findObject(mGammaShaderName));
    if (!mGammaShader)
    {
        Con::errorf("GammaBuffer: Could not find shader %s", mGammaShaderName);
    }

    // this is necessary for Show Tool use
    if (mGammaShader)
    {
        mGammaShader->initShader();
    }
}

//--------------------------------------------------------------------------
// Setup orthographic projection
//--------------------------------------------------------------------------
MatrixF GammaBuffer::setupOrthoProjection()
{
    // set ortho projection matrix
    MatrixF proj = GFX->getProjectionMatrix();
    MatrixF newMat(true);
    GFX->setProjectionMatrix(newMat);
    GFX->pushWorldMatrix();
    GFX->setWorldMatrix(newMat);
    GFX->setVertexShaderConstF(0, (float*)&newMat, 4);
    return proj;
}

//--------------------------------------------------------------------------
// Setup render states
//--------------------------------------------------------------------------
void GammaBuffer::setupRenderStates()
{
    GFX->setTextureStageColorOp(0, GFXTOPModulate);
    GFX->setTextureStageColorOp(1, GFXTOPDisable);

    for (U32 i = 0; i < 4; i++)
    {
        GFX->setTextureStageAddressModeU(i, GFXAddressClamp);
        GFX->setTextureStageAddressModeV(i, GFXAddressClamp);
    }

    GFX->disableShaders();
    GFX->setZEnable(false);
}

//--------------------------------------------------------------------------
// Copy to screen
//--------------------------------------------------------------------------
void GammaBuffer::copyToScreen(RectI& viewport)
{
    if (!mGammaShader || !mGammaShader->shader || mDisabled)
        return;

    if (!mSurface)
        return;

    PROFILE_START(Gamma);

    GFXVertexBuffer* gammaVB = NULL;

    MatrixF proj = setupOrthoProjection();
    setupRenderStates();

    mGammaShader->shader->process();
    bool usePWL = false;
    float invSize = usePWL ? (1.0f / 128.0f) : (1.0f / 256.0f);
    GFX->setPixelShaderConstF(8, &invSize, 1);

    GFX->setTexture(0, mSurface);
    GFX->setTexture(1, usePWL ? mGammaPWL : mGamma);

    GFXVideoMode mode = GFX->getVideoMode();
    F32 copyOffsetX = 1.0 / mode.resolution.x;
    F32 copyOffsetY = 1.0 / mode.resolution.y;

    // setup geometry and draw
    PrimBuild::color4f(1.0, 1.0, 1.0, 1.0);
    PrimBuild::begin(GFXTriangleFan, 4);
    PrimBuild::texCoord2f(0.0, 1.0);
    PrimBuild::vertex3f(-1.0 - copyOffsetX, -1.0 + copyOffsetY, 0.0);

    PrimBuild::texCoord2f(0.0, 0.0);
    PrimBuild::vertex3f(-1.0 - copyOffsetX, 1.0 + copyOffsetY, 0.0);

    PrimBuild::texCoord2f(1.0, 0.0);
    PrimBuild::vertex3f(1.0 - copyOffsetX, 1.0 + copyOffsetY, 0.0);

    PrimBuild::texCoord2f(1.0, 1.0);
    PrimBuild::vertex3f(1.0 - copyOffsetX, -1.0 + copyOffsetY, 0.0);
    PrimBuild::end();

    GFX->setProjectionMatrix(proj);

    GFX->popWorldMatrix();
    GFX->disableShaders();

    GFX->setTexture(0, NULL);

    PROFILE_END();

}


//--------------------------------------------------------------------------
// Set gamma buffer as render target
//--------------------------------------------------------------------------
void GammaBuffer::setAsRenderTarget()
{
    GFX->setActiveRenderSurface(mSurface);
    GFX->clear(GFXClearTarget, ColorI(0, 0, 0, 0), 1.0f, 0);
}

//-----------------------------------------------------------------------------
// Texture manager callback - for resetting textures on video change
//-----------------------------------------------------------------------------
void GammaBuffer::texManagerCallback(GFXTexCallbackCode code, void* userData)
{
    GammaBuffer* gammaBuff = (GammaBuffer*)userData;

    if (code == GFXZombify)
    {
        gammaBuff->mSurface = NULL;
        return;
    }

    if (code == GFXResurrect)
    {
        GFXVideoMode vm = GFX->getVideoMode();
        gammaBuff->mSurface.set(vm.resolution.x, vm.resolution.y, GFXFormatR8G8B8A8, &GFXDefaultRenderTargetProfile, 1);
        return;
    }

}
