#include "renderMarbleShadowMgr.h"
#include "gfx/primBuilder.h"


//**************************************************************************
// RenderMarbleShadowMgr
//**************************************************************************
RenderMarbleShadowMgr::RenderMarbleShadowMgr()
{
    mShaderStencil = NULL;
}

//-----------------------------------------------------------------------------
// render
//-----------------------------------------------------------------------------
void RenderMarbleShadowMgr::render()
{
    if (!mElementList.size())
        return;

    if (!this->mShaderStencil)
        Sim::findObject<ShaderData>("Shader_Stencil", mShaderStencil);

    if (!mShaderStencil || !mShaderStencil->getShader())
        return;

    if (getCurrentClientSceneGraph()->isReflectPass())
        return;

    PROFILE_START(RenderMarbleShadowMgrRender);

    const LightInfo* sunLight = getCurrentClientSceneGraph()->getLightManager()->sgGetSpecialLight(LightManager::sgSunLightType);

    const MatrixF worldMat = GFX->getWorldMatrix();
    const MatrixF projMat = GFX->getProjectionMatrix();

    //ColorI clearColor = ColorI(0x00, 0x00, 0x00, 0xFF);
    //GFX->clear(GFXClearStencil, clearColor, 1.0f, 0);

    mShaderStencil->getShader()->process();

    // setup stencil
    GFX->setStencilEnable(true);
    GFX->setStencilFunc(GFXCmpAlways);
    GFX->setStencilPassOp(GFXStencilOpKeep);
    GFX->setStencilFailOp(GFXStencilOpKeep);
    GFX->setStencilRef(1);
    GFX->setStencilMask(0xFF);
    GFX->setStencilWriteMask(0xFF);

    GFX->setZWriteEnable(false);
    GFX->setZFunc(GFXCmpLess);

    GFX->enableColorWrites(false, false, false, false);

    // draw shadow volumes
    for (S32 i = 0; i < mElementList.size(); i++)
    {
        RenderInst* inst = mElementList[i].inst;

        GFX->setVertexBuffer(*inst->vertBuff);
        GFX->setPrimitiveBuffer(*inst->primBuff);
        GFX->setVertexShaderConstF(0, (float*)inst->worldXform, 4);

        // ideally this would be using two sided stencil instead
        GFX->setCullMode(GFXCullCCW);
        GFX->setStencilZFailOp(GFXStencilOpIncr);

        GFX->drawIndexedPrimitive(GFXTriangleList, 0, inst->vertBuff->getPointer()->mNumVerts, 0, inst->primBuff->getPointer()->mPrimitiveCount);

        GFX->setCullMode(GFXCullCW);
        GFX->setStencilZFailOp(GFXStencilOpDecr);

        GFX->drawIndexedPrimitive(GFXTriangleList, 0, inst->vertBuff->getPointer()->mNumVerts, 0, inst->primBuff->getPointer()->mPrimitiveCount);
    }

    GFX->enableColorWrites(true, true, true, true);
    GFX->setCullMode(GFXCullCCW);

    GFX->setAlphaBlendEnable(true);
    GFX->setSrcBlend(GFXBlendSrcAlpha);
    GFX->setDestBlend(GFXBlendInvSrcAlpha);
    GFX->setTextureStageColorOp(0, GFXTOPDisable);

    GFX->setStencilFunc(GFXCmpLessEqual);
    GFX->setStencilZFailOp(GFXStencilOpKeep);

    MatrixF newMat(true);
    GFX->setWorldMatrix(newMat);
    GFX->setProjectionMatrix(newMat);

    // draw shadow overlay
    PrimBuild::begin(GFXTriangleList, 6);
    GFX->setupGenericShaders();
    PrimBuild::color(sunLight->mShadowColor);
    PrimBuild::vertex2f(-1.0f, 1.0f);
    PrimBuild::vertex2f(1.0f, 1.0f);
    PrimBuild::vertex2f(-1.0f, -1.0f);
    PrimBuild::vertex2f(1.0f, 1.0f);
    PrimBuild::vertex2f(1.0f, -1.0f);
    PrimBuild::vertex2f(-1.0f, -1.0f);
    PrimBuild::end();

    GFX->setStencilRef(0);
    GFX->setStencilFunc(GFXCmpAlways);
    GFX->setStencilEnable(false);

    GFX->setZFunc(GFXCmpLessEqual);
    GFX->setZWriteEnable(true);

    GFX->setWorldMatrix(worldMat);
    GFX->setProjectionMatrix(projMat);

    PROFILE_END();
}
