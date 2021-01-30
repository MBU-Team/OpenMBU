//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------
#include "glowBuffer.h"
#include "console/consoleTypes.h"
#include "gfx/gfxDevice.h"
#include "primBuilder.h"
#include "core/bitStream.h"

#define GLOW_BUFF_SIZE 256

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

    GFXVideoMode vm = GFX->getVideoMode();
    mSurface[2].set(vm.resolution.x, vm.resolution.y, GFXFormatR8G8B8A8, &GFXDefaultRenderTargetProfile, 1);

    mBlurShader = static_cast<ShaderData*>(Sim::findObject(mBlurShaderName));
    if (!mBlurShader)
    {
        Con::errorf("GlowBuffer: Could not find shader %s", mBlurShaderName);
    }

    setupOrthoGeometry();

    // this is necessary for Show Tool use
    if (mBlurShader)
    {
        mBlurShader->initShader();
    }
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
    }

    GFX->disableShaders();
    GFX->setZEnable(false);
}

//--------------------------------------------------------------------------
// Setup pixel offsets for the blur shader
//--------------------------------------------------------------------------
void GlowBuffer::setupPixelOffsets(Point4F offsets, bool horizontal)
{
    Point2F offsetConsts[4];

    if (horizontal)
    {
        offsetConsts[0].set(offsets.x / GLOW_BUFF_SIZE, 0.0);
        offsetConsts[1].set(offsets.y / GLOW_BUFF_SIZE, 0.0);
        offsetConsts[2].set(offsets.z / GLOW_BUFF_SIZE, 0.0);
        offsetConsts[3].set(offsets.w / GLOW_BUFF_SIZE, 0.0);
    }
    else
    {
        offsetConsts[0].set(0.0, offsets.x / GLOW_BUFF_SIZE);
        offsetConsts[1].set(0.0, offsets.y / GLOW_BUFF_SIZE);
        offsetConsts[2].set(0.0, offsets.z / GLOW_BUFF_SIZE);
        offsetConsts[3].set(0.0, offsets.w / GLOW_BUFF_SIZE);
    }


    GFX->setVertexShaderConstF(4, (float*)offsetConsts[0], 1);
    GFX->setVertexShaderConstF(5, (float*)offsetConsts[1], 1);
    GFX->setVertexShaderConstF(6, (float*)offsetConsts[2], 1);
    GFX->setVertexShaderConstF(7, (float*)offsetConsts[3], 1);
}

//--------------------------------------------------------------------------
// Perform 4 pass blur operation.  Ping pongs back and forth between 2
// render targets.  Does horizontal passes first, then vertical.
//--------------------------------------------------------------------------
void GlowBuffer::blur()
{
    // set blur shader
    if (!mBlurShader->shader)
        return;

    GFX->setAlphaTestEnable(false);

    mBlurShader->shader->process();

    // PASS 1
    //-------------------------------
    setupPixelOffsets(Point4F(3.5, 2.5, 1.5, 0.5), true);

    GFX->setTexture(0, mSurface[0]);
    GFX->setTexture(1, mSurface[0]);
    GFX->setTexture(2, mSurface[0]);
    GFX->setTexture(3, mSurface[0]);

    GFX->setActiveRenderSurface(mSurface[1]);
    GFX->drawPrimitive(GFXTriangleFan, 0, 2);


    // PASS 2
    //-------------------------------
    setupPixelOffsets(Point4F(-3.5, -2.5, -1.5, -0.5), true);

    GFX->setTexture(0, mSurface[1]);
    GFX->setTexture(1, mSurface[1]);
    GFX->setTexture(2, mSurface[1]);
    GFX->setTexture(3, mSurface[1]);

    GFX->setActiveRenderSurface(mSurface[0]);
    GFX->drawPrimitive(GFXTriangleFan, 0, 2);


    // PASS 3
    //-------------------------------
    setupPixelOffsets(Point4F(3.5, 2.5, 1.5, 0.5), false);

    GFX->setTexture(0, mSurface[0]);
    GFX->setTexture(1, mSurface[0]);
    GFX->setTexture(2, mSurface[0]);
    GFX->setTexture(3, mSurface[0]);

    GFX->setActiveRenderSurface(mSurface[1]);
    GFX->drawPrimitive(GFXTriangleFan, 0, 2);


    // PASS 4
    //-------------------------------
    setupPixelOffsets(Point4F(-3.5, -2.5, -1.5, -0.5), false);

    GFX->setTexture(0, mSurface[1]);
    GFX->setTexture(1, mSurface[1]);
    GFX->setTexture(2, mSurface[1]);
    GFX->setTexture(3, mSurface[1]);

    GFX->setActiveRenderSurface(mSurface[0]);
    GFX->drawPrimitive(GFXTriangleFan, 0, 2);
}

//--------------------------------------------------------------------------
// Copy to screen
//--------------------------------------------------------------------------
void GlowBuffer::copyToScreen(RectI& viewport)
{
    if (!mBlurShader || !mBlurShader->shader || mDisabled)
        return;

    if (!mSurface[0] || !mSurface[1] || !mSurface[2]) return;

    PROFILE_START(Glow);

    GFXVertexBuffer* glowVB = NULL;

    // setup
    GFX->pushActiveRenderSurfaces();

    MatrixF proj = setupOrthoProjection();
    setupRenderStates();

    // set blur kernel
    Point4F kernel(0.175, 0.275, 0.375, 0.475);
    GFX->setPixelShaderConstF(0, (float*)kernel, 1);

    // draw from full-size glow buffer to smaller buffer for 4 pass shader work
    GFX->setTexture(0, mSurface[2]);
    GFX->setActiveRenderSurface(mSurface[0]);
    GFX->setVertexBuffer(mVertBuff);
    GFX->drawPrimitive(GFXTriangleFan, 0, 2);

    blur();

    GFX->popActiveRenderSurfaces();


    //*** DAW: When the render target is changed, it appears that the view port
    //*** is reset.  We need to put it back into place here.
    GFX->setViewport(viewport);

    // blend final result to back buffer
    GFX->disableShaders();
    GFX->setTexture(0, mSurface[0]);

    GFX->setAlphaBlendEnable(Con::getBoolVariable("$translucentGlow", true));
    GFX->setSrcBlend(GFXBlendOne);
    GFX->setDestBlend(GFXBlendOne);
    GFX->setAlphaTestEnable(true);
    GFX->setAlphaRef(1);
    GFX->setAlphaFunc(GFXCmpGreaterEqual);

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

    // render state cleanup
    GFX->setAlphaTestEnable(false);
    GFX->setZEnable(true);
    GFX->setAlphaBlendEnable(false);
    GFX->popWorldMatrix();
    GFX->disableShaders();

    PROFILE_END();

}


//--------------------------------------------------------------------------
// Set glow buffer as render target
//--------------------------------------------------------------------------
void GlowBuffer::setAsRenderTarget()
{
    GFX->setActiveRenderSurface(mSurface[2]);
    GFX->clear(GFXClearTarget, ColorI(0, 0, 0, 0), 1.0f, 0);
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
        GFXVideoMode vm = GFX->getVideoMode();
        glowBuff->mSurface[2].set(vm.resolution.x, vm.resolution.y, GFXFormatR8G8B8A8, &GFXDefaultRenderTargetProfile, 1);
        return;
    }

}
