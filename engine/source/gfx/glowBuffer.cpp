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
    mKernel = Point4F(0.175f, 0.275f, 0.375f, 0.475f);
    mDisabled = false;
    mMaxGlowPasses = 4;
    mPixelOffsets = Point4F(3.5f, 2.5f, 1.5f, 0.5f);
}

//--------------------------------------------------------------------------
// Init fields
//--------------------------------------------------------------------------
void GlowBuffer::initPersistFields()
{
    Parent::initPersistFields();

    addField("shader", TypeString, Offset(mBlurShaderName, GlowBuffer));
    addField("kernel", TypePoint4F, Offset(mKernel, GlowBuffer));
    addField("pixelOffsets", TypePoint4F, Offset(mPixelOffsets, GlowBuffer));
    addField("maxGlowPasses", TypeS32, Offset(mMaxGlowPasses, GlowBuffer));
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
    if (!mBlurShader->getShader())
        return;

    GFX->setAlphaTestEnable(false);

    mBlurShader->getShader()->process();

    // Attempt at MBU X360 Glow Buffer
    /*Point4F pixelOffsets = mPixelOffsets;

    // PASS 1
    //-------------------------------
    setupPixelOffsets(pixelOffsets, true);

    Point3F kernel[13];
    kernel[0].set(0.0f, -0.0078125f, 0.0f);
    kernel[1].set(0.6f, 0.0f, -0.00390625f);
    kernel[2].set(0.0f, 0.69999999f, 0.0f);
    kernel[3].set(0.0f, 0.0f, 1.0f);
    kernel[4].set(0.0f, 0.00390625f, 1.0f);
    kernel[5].set(0.69999999f, 0.0f, 0.0078125f);
    kernel[6].set(0.0f, 0.6f, 0.0f);
    kernel[7].set(0.01171875f, 0.0f, 0.5f);
    kernel[8].set(0.0f, 0.015625f, 0.0f);
    kernel[9].set(0.4f, 0.0f, 0.01953125f);
    kernel[10].set(0.0f, 0.3f, 0.0f);
    kernel[11].set(0.0234375f, 0.0f, 0.1f);
    kernel[12].set(0.0f, 0.0f, 0.0f);

    float divisibles[13];
    divisibles[0] = 0.1f;
    divisibles[1] = 0.0f;
    divisibles[2] = -0.01953125f;
    divisibles[3] = 0.0f;
    divisibles[4] = 0.3f;
    divisibles[5] = 0.0f;
    divisibles[6] = -0.015625f;
    divisibles[7] = 0.0f;
    divisibles[8] = 0.4f;
    divisibles[9] = 0.0f;
    divisibles[10] = -0.01171875f;
    divisibles[11] = 0.0f;
    divisibles[12] = 0.5f;

    // set blur kernel
    GFX->setPixelShaderConstF(0, (float*)kernel, 13);

    float divisor = 0.0f;
    for (S32 i = 0; i < 13; i++)
        divisor += divisibles[i];

    GFX->setPixelShaderConstF(30, &divisor, 1);

    GFX->setTexture(0, mSurface[0]);
    GFX->setTexture(1, mSurface[0]);
    GFX->setTexture(2, mSurface[0]);
    GFX->setTexture(3, mSurface[0]);
    GFX->setActiveRenderSurface(mSurface[1]);
    GFX->drawPrimitive(GFXTriangleFan, 0, 2);


    // PASS 2
    //-------------------------------
    setupPixelOffsets(pixelOffsets * -1.0f, true);

    Point3F kernel2[13];
    kernel2[0].set(0.0f, 0.0f, -0.0078125f);
    kernel2[1].set(0.6f, 0.0f, 0.0f);
    kernel2[2].set(-0.00390625f, 0.69999999f, 0.0f);
    kernel2[3].set(0.0f, 0.0f, 1.0f);
    kernel2[4].set(0.0f, 0.0f, 0.00390625f);
    kernel2[5].set(0.69999999f, 0.0f, 0.0f);
    kernel2[6].set(0.0078125, 0.6f, 0.0f);
    kernel2[7].set(0.0f, 0.01171875f, 0.5f);
    kernel2[8].set(0.0f, 0.0f, 0.015625f);
    kernel2[9].set(0.4f, 0.0f, 0.0f);
    kernel2[10].set(0.01953125f, 0.3f, 0.0f);
    kernel2[11].set(0.0f, 0.0234375f, 0.1f);
    kernel2[12].set(0.0f, 0.0f, 0.0f);

    float divisibles2[13];
    divisibles2[0] = 0.1f;
    divisibles2[1] = 0.0f;
    divisibles2[2] = 0.0f;
    divisibles2[3] = -0.01953125f;
    divisibles2[4] = 0.3f;
    divisibles2[5] = 0.0f;
    divisibles2[6] = 0.0f;
    divisibles2[7] = -0.015625f;
    divisibles2[8] = 0.4f;
    divisibles2[9] = 0.0f;
    divisibles2[10] = 0.0f;
    divisibles2[11] = -0.01171875f;
    divisibles2[12] = 0.5f;

    // set blur kernel
    GFX->setPixelShaderConstF(0, (float*)kernel2, 13);

    float divisor2 = 0.0f;
    for (S32 i = 0; i < 13; i++)
        divisor2 += divisibles2[i];

    GFX->setPixelShaderConstF(30, &divisor2, 1);

    GFX->setTexture(0, mSurface[1]);
    GFX->setTexture(1, mSurface[1]);
    GFX->setTexture(2, mSurface[1]);
    GFX->setTexture(3, mSurface[1]);
    GFX->setActiveRenderSurface(mSurface[0]);
    GFX->drawPrimitive(GFXTriangleFan, 0, 2);
    */

    Point4F pixelOffsets = mPixelOffsets;

    float divisor = 1.25f;
    GFX->setPixelShaderConstF(2, &divisor, 1);

    if (mMaxGlowPasses > 0)
    {
        // PASS 1
        //-------------------------------
        setupPixelOffsets(pixelOffsets, true);
        mTarget->attachTexture( GFXTextureTarget::Color0, mSurface[1] );

        GFX->setTexture(0, mSurface[0]);
        GFX->setTexture(1, mSurface[0]);
        GFX->setTexture(2, mSurface[0]);
        GFX->setTexture(3, mSurface[0]);

        GFX->drawPrimitive(GFXTriangleFan, 0, 2);

        if (mMaxGlowPasses > 1)
        {
            // PASS 2
            //-------------------------------
            setupPixelOffsets(pixelOffsets * -1.0f, true);
            mTarget->attachTexture( GFXTextureTarget::Color0, mSurface[0] );

            GFX->setTexture(0, mSurface[1]);
            GFX->setTexture(1, mSurface[1]);
            GFX->setTexture(2, mSurface[1]);
            GFX->setTexture(3, mSurface[1]);

            GFX->drawPrimitive(GFXTriangleFan, 0, 2);

            if (mMaxGlowPasses > 2)
            {
                // PASS 3
                //-------------------------------
                setupPixelOffsets(pixelOffsets * -1.0f, false);
                mTarget->attachTexture( GFXTextureTarget::Color0, mSurface[1] );

                GFX->setTexture(0, mSurface[0]);
                GFX->setTexture(1, mSurface[0]);
                GFX->setTexture(2, mSurface[0]);
                GFX->setTexture(3, mSurface[0]);

                GFX->drawPrimitive(GFXTriangleFan, 0, 2);

                if (mMaxGlowPasses > 3)
                {
                    // PASS 4
                    //-------------------------------
                    setupPixelOffsets(pixelOffsets * -1.0f, false);
                    mTarget->attachTexture( GFXTextureTarget::Color0, mSurface[0] );

                    GFX->setTexture(0, mSurface[1]);
                    GFX->setTexture(1, mSurface[1]);
                    GFX->setTexture(2, mSurface[1]);
                    GFX->setTexture(3, mSurface[1]);

                    GFX->drawPrimitive(GFXTriangleFan, 0, 2);
                }
            }
        }
    }

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
    GFX->setPixelShaderConstF(0, (float*)mKernel, 1);

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
    GFX->setTexture(1, NULL);
    GFX->setTexture(2, NULL);
    GFX->setTexture(3, NULL);
    GFX->setTexture(4, NULL);
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
    Point2I goalResolution = gClientSceneGraph->getDisplayTargetResolution();
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
