//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------
#include "glowBuffer.h"
#include "console/consoleTypes.h"
#include "gfx/gfxDevice.h"
#include "primBuilder.h"
#include "core/bitStream.h"
#include "sceneGraph/sceneGraph.h"

#define GLOW_BUFF_SIZE 320

IMPLEMENT_CONOBJECT(GlowBuffer);

//****************************************************************************
// Glow Buffer
//****************************************************************************


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------
GlowBuffer::GlowBuffer()
{
    mBlurShader = NULL;
    mBlurShaderName = NULL;
    mCallbackHandle = -1;
    mDisabled = false;
}

//--------------------------------------------------------------------------
// Init fields
//--------------------------------------------------------------------------
void GlowBuffer::initPersistFields()
{
    Parent::initPersistFields();

    addField("shader", TypeString, Offset(mBlurShaderName, GlowBuffer));
}

//--------------------------------------------------------------------------
// onAdd
//--------------------------------------------------------------------------
bool GlowBuffer::onAdd()
{
    if (!Parent::onAdd())
        return false;

    init();

    return true;
}

//--------------------------------------------------------------------------
// onRemove
//--------------------------------------------------------------------------
void GlowBuffer::onRemove()
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
void GlowBuffer::init()
{
    if (Con::getBoolVariable("$pref::Video::disableGlowBuffer", false))
    {
        mDisabled = true;
        return;
    }

    if (GFXDevice::devicePresent() && mCallbackHandle != -1)
    {
        GFX->registerTexCallback(texManagerCallback, (void*)this, mCallbackHandle);
    }

    mSurface[0].set(GLOW_BUFF_SIZE, GLOW_BUFF_SIZE, GFXFormatR8G8B8A8, &GFXDefaultRenderTargetProfile, 1);
    mSurface[1].set(GLOW_BUFF_SIZE, GLOW_BUFF_SIZE, GFXFormatR8G8B8A8, &GFXDefaultRenderTargetProfile, 1);

    mBlurShader = static_cast<ShaderData*>(Sim::findObject(mBlurShaderName));
    if (!mBlurShader)
    {
        Con::errorf("GlowBuffer: Could not find shader %s", mBlurShaderName);
    }

    setupOrthoGeometry();

    // this is necessary for Show Tool use
    if (mBlurShader)
        mBlurShader->initShader();

    mTarget = GFX->allocRenderToTextureTarget();
}

//--------------------------------------------------------------------------
// Setup orthographic geometry for small buffer.
//--------------------------------------------------------------------------
void GlowBuffer::setupOrthoGeometry()
{
    if (mVertBuff) return;

    // 0.5 pixel offset - the render is spread from -1 to 1 ( 2 width )
    F32 copyOffset = 1.0 / GLOW_BUFF_SIZE;

    GFXVertexPT points[4];
    points[0].point = Point3F(-1.0 - copyOffset, -1.0 + copyOffset, 0.0);
    points[0].texCoord = Point2F(0.0, 1.0);

    points[1].point = Point3F(-1.0 - copyOffset, 1.0 + copyOffset, 0.0);
    points[1].texCoord = Point2F(0.0, 0.0);

    points[2].point = Point3F(1.0 - copyOffset, 1.0 + copyOffset, 0.0);
    points[2].texCoord = Point2F(1.0, 0.0);

    points[3].point = Point3F(1.0 - copyOffset, -1.0 + copyOffset, 0.0);
    points[3].texCoord = Point2F(1.0, 1.0);

    mVertBuff.set(GFX, 4, GFXBufferTypeStatic);
    GFXVertexPT* vbVerts = mVertBuff.lock();
    dMemcpy(vbVerts, points, sizeof(GFXVertexPT) * 4);
    mVertBuff.unlock();

}

//--------------------------------------------------------------------------
// Setup orthographic projection
//--------------------------------------------------------------------------
MatrixF GlowBuffer::setupOrthoProjection()
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
void GlowBuffer::setupRenderStates()
{
    GFX->setTextureStageColorOp(0, GFXTOPModulate);
    GFX->setTextureStageColorOp(1, GFXTOPDisable);

    for (U32 i = 0; i < 4; i++)
    {
        GFX->setTextureStageAddressModeU(i, GFXAddressClamp);
        GFX->setTextureStageAddressModeV(i, GFXAddressClamp);
        GFX->setTextureStageMagFilter(i, GFXTextureFilterLinear);
        GFX->setTextureStageMinFilter(i, GFXTextureFilterLinear);
        GFX->setTextureStageMipFilter(i, GFXTextureFilterLinear);
    }

    // No color information is provided in the vertex stream, use this generic shader -patw
    GFX->setupGenericShaders( GFXDevice::GSTexture );
    GFX->setZEnable(false);
}

//--------------------------------------------------------------------------
// Perform 2 pass blur operation.  Ping pongs back and forth between 2
// render targets.  Does horizontal pass first, then vertical.
//--------------------------------------------------------------------------
void GlowBuffer::blur()
{
    // set blur shader
    if (!mBlurShader->getShader())
        return;

    GFX->setAlphaTestEnable(false);

    mBlurShader->getShader()->process();

    float offsets[13] = { -7.5, -6.25, -5, -3.75, -2.5, -1.25, 0, 1.25, 2.5, 3.75, 5, 6.25, 7.5 };
    float divisors[13] = { 0.1f, 0.3f, 0.4f, 0.5f, 0.6f, 0.7f, 1.0f, 0.7f, 0.5f, 0.5f, 0.4f, 0.3f, 0.1f };

    float divisor = 0.0;
    
    Point4F kernel[13];
    for (int i = 0; i < 13; i++)
        kernel[i].set(offsets[i] / GLOW_BUFF_SIZE, 0, divisors[i], 0);

    for (int i = 0; i < 13; i++)
        divisor += divisors[i];

    // set blur kernel
    GFX->setPixelShaderConstF(0, (float*)kernel, 13);
    GFX->setPixelShaderConstF(30, &divisor, 1);

    GFX->setTexture(0, mSurface[0]);
    mTarget->attachTexture(GFXTextureTarget::Color0, mSurface[1]);
    GFX->drawPrimitive(GFXTriangleFan, 0, 2);
    
    for (int i = 0; i < 13; i++)
        kernel[i].set(0, offsets[i] / GLOW_BUFF_SIZE, divisors[i], 0);

    //// set blur kernel
    GFX->setPixelShaderConstF(0, (float*)kernel, 13);
    GFX->setPixelShaderConstF(30, &divisor, 1);

    GFX->setTexture(0, mSurface[1]);
    mTarget->attachTexture(GFXTextureTarget::Color0, mSurface[0]);
    GFX->drawPrimitive(GFXTriangleFan, 0, 2);

    mTarget->clearAttachments();
}

//--------------------------------------------------------------------------
// Copy to screen
//--------------------------------------------------------------------------
void GlowBuffer::copyToScreen(RectI& viewport)
{
    if (!mBlurShader || !mBlurShader->getShader() || mDisabled)
        return;

    if (!mSurface[0] || !mSurface[1] || !mSurface[2]) return;

    PROFILE_START(Glow);

    // setup
    GFX->disableShaders();
    GFX->pushActiveRenderTarget();

    setupRenderStates();
    MatrixF proj = setupOrthoProjection();

    // set blur kernel
    // GFX->setPixelShaderConstF(0, (float*)mKernel, 1);

    // draw from full-size glow buffer to smaller buffer for 4 pass shader work
    GFX->setTexture(0, mSurface[2]);

    mTarget->attachTexture(GFXTextureTarget::Color0, mSurface[0] );
    mTarget->attachTexture(GFXTextureTarget::DepthStencil, NULL );
    GFX->setActiveRenderTarget( mTarget );

    GFX->setVertexBuffer(mVertBuff);
    GFX->drawPrimitive(GFXTriangleFan, 0, 2);

    blur();

    mTarget->clearAttachments();

    GFX->popActiveRenderTarget();


    //*** DAW: When the render target is changed, it appears that the view port
    //*** is reset.  We need to put it back into place here.
    GFX->setViewport(viewport);

    // blend final result to back buffer
    GFX->disableShaders();
    // Mod color texture for this draw because we are providing color information to the PrimBuilder -patw
    GFX->setupGenericShaders( GFXDevice::GSModColorTexture );
    GFX->setTexture(0, mSurface[0]);
    
    for (U32 i = 1; i < GFX->getNumSamplers(); i++)
    {
        GFX->setTextureStageColorOp(i, GFXTOPDisable);
    }

    GFX->setAlphaBlendEnable(Con::getBoolVariable("$translucentGlow", true));
    GFX->setSrcBlend(GFXBlendOne);
    GFX->setDestBlend(GFXBlendOne);
    GFX->setAlphaTestEnable(true);//false); // TODO: verify this
    GFX->setAlphaRef(1);
    GFX->setAlphaFunc(GFXCmpGreaterEqual);

    Point2I resolution = GFX->getActiveRenderTarget()->getSize();
    F32 copyOffsetX = 1.0f / (F32)resolution.x;
    F32 copyOffsetY = 1.0f / (F32)resolution.y;

    // setup geometry and draw
    PrimBuild::begin(GFXTriangleFan, 4);
    PrimBuild::color4f(1.0, 1.0, 1.0, 1.0);
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

    // render state cleanup
    GFX->setAlphaTestEnable(false);
    GFX->setZEnable(true);
    GFX->setAlphaBlendEnable(false);
    GFX->popWorldMatrix();
    GFX->disableShaders();

    mTarget->clearAttachments();

    PROFILE_END();

}


//--------------------------------------------------------------------------
// Set glow buffer as render target
//--------------------------------------------------------------------------
void GlowBuffer::setAsRenderTarget()
{
    // Make sure we have a final display target of the same size as the view
    // we're rendering.
    Point2I goalResolution = getCurrentClientSceneGraph()->getDisplayTargetResolution();
    if(mSurface[2].isNull() || goalResolution != Point2I(mSurface[2].getWidth(), mSurface[2].getHeight()))
    {
        Con::printf("GlowBuffer (%x) - Resizing glow texture to be %dx%dpx", this, goalResolution.x, goalResolution.y);
        mSurface[2].set( goalResolution.x, goalResolution.y, GFXFormatR8G8B8A8, &GFXDefaultRenderTargetZBufferProfile, 1 );
    }

    // Set up render target.
    mTarget->attachTexture(GFXTextureTarget::Color0, mSurface[2]);
    mTarget->attachTexture(GFXTextureTarget::DepthStencil, GFXTextureTarget::sDefaultDepthStencil );
    GFX->setActiveRenderTarget( mTarget );
    GFX->clear( GFXClearTarget, ColorI(0,0,0,0), 1.0f, 0 );
}

//-----------------------------------------------------------------------------
// Texture manager callback - for resetting textures on video change
//-----------------------------------------------------------------------------
void GlowBuffer::texManagerCallback(GFXTexCallbackCode code, void* userData)
{
    GlowBuffer* glowBuff = (GlowBuffer*)userData;

    if (code == GFXZombify)
    {
        glowBuff->mSurface[2] = NULL;
        return;
    }

    if (code == GFXResurrect)
    {
        GFXTarget *target = GFX->getActiveRenderTarget();
        if ( target )
        {
            Point2I res = GFX->getActiveRenderTarget()->getSize();
            glowBuff->mSurface[2].set( res.x, res.y, GFXFormatR8G8B8A8, &GFXDefaultRenderTargetZBufferProfile, 1 );
        }
        return;
    }

}
