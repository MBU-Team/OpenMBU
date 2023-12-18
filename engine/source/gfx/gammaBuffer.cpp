//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------
#include "gammaBuffer.h"
#include "console/consoleTypes.h"
#include "gfx/gfxDevice.h"
#include "primBuilder.h"
#include "core/bitStream.h"
#include "sceneGraph/sceneGraph.h"

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
    mGammaRamp = NULL;
    mGammaRampBitmapName = NULL;
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
    addField("bitmap", TypeFilename, Offset(mGammaRampBitmapName, GammaBuffer));
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

    if (GFXDevice::devicePresent() && mCallbackHandle != -1)
    {
        GFX->registerTexCallback(texManagerCallback, (void*)this, mCallbackHandle);
    }

    GFXVideoMode vm = GFX->getVideoMode();

    mSurface.set(vm.resolution.x, vm.resolution.y, GFXFormatR8G8B8A8, &GFXDefaultRenderTargetProfile, 1);

    mGammaRamp.set(mGammaRampBitmapName, &GFXDefaultStaticDiffuseProfile);
    if (!mGammaRamp)
    {
        Con::errorf("GammaBuffer: Could not find bitmap %s", mGammaRampBitmapName);
    }

    mGammaShader = static_cast<ShaderData*>(Sim::findObject(mGammaShaderName));
    if (!mGammaShader)
    {
        Con::errorf("GammaBuffer: Could not find shader %s", mGammaShaderName);
    }

    // this is necessary for Show Tool use
    if (mGammaShader)
        mGammaShader->initShader();

    mTarget = GFX->allocRenderToTextureTarget();
    if (mTarget.isNull())
    {
        Con::errorf("GammaBuffer: Could not allocate render target");
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
    if (!mGammaShader || !mGammaShader->getShader() || mDisabled)
        return;

    if (!mSurface || !mGammaRamp)
        return;

    PROFILE_START(Gamma);

    MatrixF proj = setupOrthoProjection();
    setupRenderStates();

    mGammaShader->getShader()->process();
    float invSize = (1.0f / (F32)mGammaRamp->getWidth());
    GFX->setPixelShaderConstF(0, &invSize, 1);

    GFX->setTexture(0, mSurface);
    GFX->setTexture(1, mGammaRamp);

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
    if (mTarget.isNull())
        return;
    // Make sure we have a final display target of the same size as the view
    // we're rendering.
    Point2I goalResolution = GFX->getVideoMode().resolution;
    if(mSurface.isNull() || goalResolution != Point2I(mSurface.getWidth(), mSurface.getHeight()))
    {
        Con::printf("GammaBuffer (%x) - Resizing gamma texture to be %dx%dpx", this, goalResolution.x, goalResolution.y);
        mSurface.set( goalResolution.x, goalResolution.y, GFXFormatR8G8B8A8, &GFXDefaultRenderTargetZBufferProfile, 1 );
    }

    mTarget->attachTexture(GFXTextureTarget::Color0, mSurface);
    mTarget->attachTexture(GFXTextureTarget::DepthStencil, GFXTextureTarget::sDefaultDepthStencil );
    GFX->setActiveRenderTarget( mTarget );
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
        GFXTarget *target = GFX->getActiveRenderTarget();
        if ( target )
        {
            Point2I res = GFX->getActiveRenderTarget()->getSize();
            gammaBuff->mSurface.set( res.x, res.y, GFXFormatR8G8B8A8, &GFXDefaultRenderTargetZBufferProfile, 1 );
        }
        return;
    }

}
