//-----------------------------------------------
// Synapse Gaming - Lighting System
// Copyright � Synapse Gaming 2003
// Written by John Kabus
//-----------------------------------------------
#include "gfx/gfxDevice.h"
#include "gfx/primBuilder.h"
#include "console/console.h"
#include "console/consoleTypes.h"
#include "lightingSystem/sgDynamicRangeLighting.h"
#include "lightingSystem/sgFormatManager.h"
#include "sceneGraph/sceneGraph.h"


IMPLEMENT_CONOBJECT(sgGuiTexTestCtrl);


extern GFXTexHandle* gTexture;

void sgGuiTexTestCtrl::onRender(Point2I offset, const RectI& updateRect)
{
    //if(!gChain)
    //	return;

    GFX->clearBitmapModulation();
    RectI rect(offset, mBounds.extent);

    if (gTexture && ((GFXTextureObject*)(*gTexture)))
        GFX->drawBitmapStretch((*gTexture), rect);
    /*
        if(gChain->sgSurfaceChain.size() <= sgTextureLevel)
        {
            if(LightManager::sgAllowBloom())
                GFX->drawBitmapStretch(gChain->sgBloom2, rect);
        }
        else
            GFX->drawBitmapStretch(gChain->sgSurfaceChain[sgTextureLevel], rect);
    */
    renderChildControls(offset, updateRect);
}

void sgGuiTexTestCtrl::initPersistFields()
{
    Parent::initPersistFields();
    addGroup("Misc");
    addField("textureLevel", TypeS32, Offset(sgTextureLevel, sgGuiTexTestCtrl));
    endGroup("Misc");
}


void sgDRLSystem::sgPrepSystem(const Point2I& offset, const Point2I& extent)
{
    sgPrepChain(offset, extent);

    sgDidPrep = LightManager::sgAllowDRLSystem() && (sgSurfaceChain.size() > 0);
    if (!sgDidPrep)
        return;

    // Ben's change - unknown cause/effect (moved from GuiTSCtrl)...
    GFX->clear(GFXClearTarget, ColorI(255, 255, 0), 1.0f, 0);

    // to avoid the back buffer copy just use
    // the copy-to texture as the surface...
    GFX->pushActiveRenderTarget();
    GFXTextureTargetRef myTarg = GFX->allocRenderToTextureTarget();
    myTarg->attachTexture(GFXTextureTarget::Color0, sgSurfaceChain[0] );
    myTarg->attachTexture(GFXTextureTarget::DepthStencil, GFXTextureTarget::sDefaultDepthStencil);
    GFX->setActiveRenderTarget( myTarg );
}

void sgDRLSystem::sgRenderSystem()
{
    if (!sgDidPrep)
        return;

    sgDidPrep = false;

    // put back the back buffer...
    GFX->popActiveRenderTarget();

    sgRenderChain();
    sgRenderDRL();
}

void sgDRLSurfaceChain::sgPrepChain(const Point2I& offset, const Point2I& extent)
{
    if (!LightManager::sgAllowDRLSystem())
    {
        sgDestroyChain();
        return;
    }

    AssertFatal((sgSurfaceSize.size() == sgSurfaceChain.size()), "DRL:> Ouch!");

    //Point2I framesize = GFX->getVideoMode().resolution;

    // this doesn't affect chain...
    sgOffset = offset;
    sgExtent = extent;
    Point2I cursize = getCurrentClientSceneGraph()->getDisplayTargetResolution();

    if (sgSurfaceSize.size() > 0)
    {
        Point2I& size = sgSurfaceSize[0];
        if ((size.x != cursize.x) || (size.y != cursize.y))
            sgDestroyChain();
    }

    if (sgCachedIsHDR != LightManager::sgAllowFullHighDynamicRangeLighting())
    {
        sgDestroyChain();
    }

    if (sgSurfaceSize.size() == 0)
    {
        sgCachedIsHDR = LightManager::sgAllowFullHighDynamicRangeLighting();

        //Sim::findObject("AlphaBloomShader", sgAlphaBloom);
        Sim::findObject("DownSample4x4Shader", sgDownSample4x4);
        Sim::findObject("DownSample4x4BloomClampShader", sgDownSample4x4BloomClamp);
        Sim::findObject("BloomBlurShader", sgBloomBlur);
        Sim::findObject("DRLFullShader", sgDRLFull);
        Sim::findObject("DRLOnlyBloomToneShader", sgDRLOnlyBloomTone);

        if (//(!sgAlphaBloom) || (!sgAlphaBloom->shader) ||
            (!sgDownSample4x4) || (!sgDownSample4x4->getShader()) ||
            (!sgDownSample4x4BloomClamp) || (!sgDownSample4x4BloomClamp->getShader()) ||
            (!sgBloomBlur) || (!sgBloomBlur->getShader()) ||
            (!sgDRLFull) || (!sgDRLFull->getShader()) ||
            (!sgDRLOnlyBloomTone) || (!sgDRLOnlyBloomTone->getShader()))
            return;

        sgSurfaceSize.increment(1);
        sgSurfaceSize[0] = cursize;
        Point2I lastsize = cursize;

        while (1)
        {
            sgSurfaceSize.increment(1);
            Point2I& size = sgSurfaceSize.last();

            size.x = lastsize.x / sgdrlscSampleWidth;
            size.y = lastsize.y / sgdrlscSampleWidth;

            if (size.x < 1)
                size.x = 1;
            if (size.y < 1)
                size.y = 1;

            if ((size.x * size.y) <= 1)
                break;

            lastsize = size;
        }

        // use the sizes to create the handles...
        sgSurfaceChain.setSize(sgSurfaceSize.size());
        for (U32 i = 0; i < sgSurfaceSize.size(); i++)
        {
            Point2I& size = sgSurfaceSize[i];

            // this is designed to avoid too many float calcs, but having the whole chain
            // as float targets could be a good idea, hmm...
            if ((i == 0) && (LightManager::sgAllowFullHighDynamicRangeLighting()) && (GFX->getPixelShaderVersion() >= 3.0))
            {
                sgSurfaceChain[i] = GFXTexHandle(size.x, size.y,
                    sgFormatManager::sgHDRTextureFormat, &DRLTargetTextureProfile);
            }
            else
            {
                sgSurfaceChain[i] = GFXTexHandle(size.x, size.y,
                    sgFormatManager::sgDRLTextureFormat, &DRLTargetTextureProfile);
            }
        }

        gTexture = &sgSurfaceChain[1];

        sgBloom = GFXTexHandle(sgSurfaceSize[sgdlrscBloomIndex].x, sgSurfaceSize[sgdlrscBloomIndex].y,
            sgFormatManager::sgDRLTextureFormat, &DRLTargetTextureProfile);
        sgBloom2 = GFXTexHandle(sgSurfaceSize[sgdlrscBloomIndex].x, sgSurfaceSize[sgdlrscBloomIndex].y,
            sgFormatManager::sgDRLTextureFormat, &DRLTargetTextureProfile);
        sgToneMap = GFXTexHandle("common/lighting/sgToneMap", &GFXDefaultStaticDiffuseProfile);
        sgGrayMap = GFXTexHandle("common/lighting/sgGrayMap", &GFXDefaultStaticDiffuseProfile);
        sgDRLViewMap = GFXTexHandle("common/lighting/sgDRLViewMap", &GFXDefaultStaticDiffuseProfile);
    }
}

void sgDRLSurfaceChain::sgRenderChain()
{
    AssertFatal((sgSurfaceSize.size() == sgSurfaceChain.size()), "DRL:> Ouch!");

    if (sgSurfaceChain.size() <= 0)
        return;

    //GFX->copyBBToTexture(sgSurfaceChain[0]);
    GFX->pushActiveRenderTarget();

    // start copy and down-sample...
    RectI rect(-1, 1, 1, -1);
    GFXTexHandle* lasttexture = &sgSurfaceChain[0];
    GFX->clearBitmapModulation();

    GFX->setCullMode(GFXCullNone);
    GFX->setLightingEnable(false);

    // change me...
    GFX->setAlphaBlendEnable(true);
    GFX->setSrcBlend(GFXBlendOne);
    GFX->setDestBlend(GFXBlendZero);

    GFX->setZEnable(false);
    GFX->setTextureStageColorOp(0, GFXTOPModulate);
    GFX->setTextureStageAddressModeU(0, GFXAddressClamp);
    GFX->setTextureStageAddressModeV(0, GFXAddressClamp);
    GFX->setTextureStageColorOp(1, GFXTOPModulate);
    GFX->setTextureStageAddressModeU(1, GFXAddressClamp);
    GFX->setTextureStageAddressModeV(1, GFXAddressClamp);
    GFX->setTexture(1, sgDRLViewMap);
    GFX->setTextureStageColorOp(2, GFXTOPDisable);


    Point4F temp(LightManager::sgBloomCutOff, LightManager::sgBloomCutOff,
        LightManager::sgBloomCutOff, LightManager::sgBloomCutOff);
    GFX->setPixelShaderConstF(1, temp, 1);

    temp = Point4F(LightManager::sgBloomAmount, LightManager::sgBloomAmount,
        LightManager::sgBloomAmount, LightManager::sgBloomAmount);
    GFX->setPixelShaderConstF(2, temp, 1);

    for (U32 i = 1; i < sgSurfaceChain.size(); i++)
    {
        if (i == 1)
            sgDownSample4x4BloomClamp->getShader()->process();
        else
            sgDownSample4x4->getShader()->process();

        GFXTextureTargetRef myTarg = GFX->allocRenderToTextureTarget();
        myTarg->attachTexture(GFXTextureTarget::Color0, sgSurfaceChain[i] );
        //myTarg->attachTexture(GFXTextureTarget::DepthStencil, GFXTextureTarget::sDefaultDepthStencil );
        GFX->setActiveRenderTarget( myTarg );

        GFX->setTexture(0, (*lasttexture));

        // this stuff should be in a vertex buffer?
        Point2F offset;
        offset.x = 1.0f;// + (1.0f / F32(sgSurfaceSize[i].x));
        offset.y = 1.0f;// + (1.0f / F32(sgSurfaceSize[i].y));

        Point2I& lastsize = sgSurfaceSize[i - 1];
        Point4F stride((offset.x / F32(lastsize.x - 1)), (offset.y / F32(lastsize.y - 1)), 0.0f, 0.0f);
        GFX->setVertexShaderConstF(0, stride, 1);
        GFX->setPixelShaderConstF(0, stride, 1);

        PrimBuild::color(ColorF(1.0f, 1.0f, 1.0f));
        PrimBuild::begin(GFXTriangleStrip, 4);
        PrimBuild::texCoord2f(0.0f, 0.0f);
        PrimBuild::vertex2f(rect.point.x, rect.point.y);
        PrimBuild::texCoord2f(offset.x, 0.0f);
        PrimBuild::vertex2f(rect.extent.x, rect.point.y);
        PrimBuild::texCoord2f(0.0f, offset.y);
        PrimBuild::vertex2f(rect.point.x, rect.extent.y);
        PrimBuild::texCoord2f(offset.x, offset.y);
        PrimBuild::vertex2f(rect.extent.x, rect.extent.y);
        PrimBuild::end();

        if ((i == sgdlrscBloomIndex) && (!LightManager::sgAllowFullDynamicRangeLighting()))
            break;

        lasttexture = &sgSurfaceChain[i];
    }


    if (LightManager::sgAllowBloom())
    {
        sgBloomBlur->getShader()->process();
        GFXTextureTargetRef myTarg = GFX->allocRenderToTextureTarget();
        myTarg->attachTexture(GFXTextureTarget::Color0, sgBloom );
        myTarg->attachTexture(GFXTextureTarget::DepthStencil, GFXTextureTarget::sDefaultDepthStencil );
        GFX->setActiveRenderTarget( myTarg );
        GFX->setTexture(0, sgSurfaceChain[sgdlrscBloomIndex]);
        //GFX->setTextureBorderColor(0, ColorI(0, 0, 0, 0));
        GFX->setTextureStageAddressModeU(0, GFXAddressClamp);
        GFX->setTextureStageAddressModeV(0, GFXAddressClamp);

        Point2F offset;
        Point2I& lastsize = sgSurfaceSize[sgdlrscBloomIndex];
        offset.x = 1.0f;// + (1.0f / F32(lastsize.x));
        offset.y = 1.0f;// + (1.0f / F32(lastsize.y));

        Point4F stride((offset.x / F32(lastsize.x - 1)), (offset.y / F32(lastsize.y - 1)), 0.0f, 0.0f);
        GFX->setVertexShaderConstF(0, stride, 1);
        GFX->setPixelShaderConstF(0, stride, 1);

        Point4F temp(LightManager::sgBloomSeedAmount, LightManager::sgBloomSeedAmount,
            LightManager::sgBloomSeedAmount, LightManager::sgBloomSeedAmount);
        GFX->setPixelShaderConstF(1, temp, 1);

        PrimBuild::color(ColorF(1.0f, 1.0f, 1.0f));
        PrimBuild::begin(GFXTriangleStrip, 4);
        PrimBuild::texCoord2f(0.0f, 0.0f);
        PrimBuild::vertex2f(rect.point.x, rect.point.y);
        PrimBuild::texCoord2f(offset.x, 0.0f);
        PrimBuild::vertex2f(rect.extent.x, rect.point.y);
        PrimBuild::texCoord2f(0.0f, offset.y);
        PrimBuild::vertex2f(rect.point.x, rect.extent.y);
        PrimBuild::texCoord2f(offset.x, offset.y);
        PrimBuild::vertex2f(rect.extent.x, rect.extent.y);
        PrimBuild::end();

        //sgBloomBlur->shader->process();
        myTarg->attachTexture(GFXTextureTarget::Color0, sgBloom2 );
        GFX->setActiveRenderTarget( myTarg );
        GFX->setTexture(0, sgBloom);
        //GFX->setTextureBorderColor(0, ColorI(0, 0, 0, 0));
        GFX->setTextureStageAddressModeU(0, GFXAddressClamp);
        GFX->setTextureStageAddressModeV(0, GFXAddressClamp);

        PrimBuild::color(ColorF(1.0f, 1.0f, 1.0f));
        PrimBuild::begin(GFXTriangleStrip, 4);
        PrimBuild::texCoord2f(0.0f, 0.0f);
        PrimBuild::vertex2f(rect.point.x, rect.point.y);
        PrimBuild::texCoord2f(offset.x, 0.0f);
        PrimBuild::vertex2f(rect.extent.x, rect.point.y);
        PrimBuild::texCoord2f(0.0f, offset.y);
        PrimBuild::vertex2f(rect.point.x, rect.extent.y);
        PrimBuild::texCoord2f(offset.x, offset.y);
        PrimBuild::vertex2f(rect.extent.x, rect.extent.y);
        PrimBuild::end();
    }
    else
    {
        // makes sure texture doesn't affect DRL/Bloom composite shader...
        GFXTextureTargetRef myTarg = GFX->allocRenderToTextureTarget();
        myTarg->attachTexture(GFXTextureTarget::Color0, sgBloom2 );
        GFX->setActiveRenderTarget( myTarg );
        GFX->clear(GFXClearTarget, ColorI(0, 0, 0, 0), 1.0f, 0);
    }

    if (!LightManager::sgAllowFullDynamicRangeLighting())
    {
        // makes sure texture doesn't affect DRL/Bloom composite shader...
        GFXTextureTargetRef myTarg = GFX->allocRenderToTextureTarget();
        myTarg->attachTexture(GFXTextureTarget::Color0, sgSurfaceChain[sgSurfaceChain.size()-1] );
        GFX->setActiveRenderTarget( myTarg );
        GFX->clear(GFXClearTarget,
            ColorF(0, 0, 0, LightManager::sgDRLTarget), 1.0f, 0);
    }


    GFX->setZEnable(true);

    //GFX->setActiveRenderSurface(NULL);
    GFX->setAlphaBlendEnable(false);

    GFX->popActiveRenderTarget();
}

void sgDRLSurfaceChain::sgRenderDRL()
{
    if (!LightManager::sgAllowDRLSystem())
        return;

    if (sgSurfaceChain.size() < 3)
        return;

    // fix me - should use the right sized buffer and map directly to the screen...
    // chances are the original code worked correctly just the viewport wasn't set...
    //RectI rect(-1, 1, 1, -1);
    RectF rect;
    rect.extent.x = (F32(sgOffset.x + sgExtent.x) / F32(GFX->getVideoMode().resolution.x));
    rect.extent.y = 1.0f - (F32(sgOffset.y + sgExtent.y) / F32(GFX->getVideoMode().resolution.y));
    rect.extent = (rect.extent * 2.0) - Point2F(1, 1);
    rect.point.x = (F32(sgOffset.x) / F32(GFX->getVideoMode().resolution.x));
    rect.point.y = 1.0f - (F32(sgOffset.y) / F32(GFX->getVideoMode().resolution.y));
    rect.point = (rect.point * 2.0) - Point2F(1, 1);

    F32 uoff = 0.5f / F32(sgExtent.x);
    F32 voff = 0.5f / F32(sgExtent.y);
    RectF texcoord;
    texcoord.point = Point2F(uoff, voff);
    // yes this is correct! otherwise mapping is on pixels-1...
    texcoord.extent = Point2F(1.0, 1.0) + texcoord.point;

    //texcoord.point = Point2F(0, 0);
    //texcoord.extent = Point2F(1.0, 1.0);


    // time to get the intensity...
    // this was even slower...
    /*F32 intensity = 0.5;
    GFXTextureObject *obj = sgSurfaceChain[sgSurfaceChain.size()-1];
    GBitmap drldata(1, 1, 0, GFXFormatR8G8B8A8);
    if(obj->copyToBmp(&drldata))
    {
        const U8 *texel = drldata.getBits();
        intensity = F32(texel[3]) / 255.0f;
    }

    F32 fullrange = 1.0 / (intensity + 0.0001);
    intensity = LightManager::sgDRLTarget * fullrange;
    intensity = sqrt(intensity);
    intensity = getMin(intensity, LightManager::sgDRLMax);
    intensity = getMax(intensity, LightManager::sgDRLMin);
*/
/*
float1 intensity = tex2D(intensitymap, halfway).w;
float1 fullrange = 1.0 / (intensity + 0.0001);

intensity = drlinfo.z * fullrange;
intensity = sqrt(intensity);

//float1 fullrange = intensity;
intensity = min(intensity, drlinfo.x);
intensity = max(intensity, drlinfo.y);
*/


    GFX->setCullMode(GFXCullNone);
    GFX->setLightingEnable(false);
    GFX->setAlphaBlendEnable(true);
    GFX->setSrcBlend(GFXBlendOne);
    GFX->setDestBlend(GFXBlendOne);
    //GFX->setSrcBlend(GFXBlendZero);
    GFX->setDestBlend(GFXBlendZero);

    GFX->setZEnable(false);

    // intensity texel...
    GFX->setTextureStageColorOp(0, GFXTOPModulate);
    GFX->setTexture(0, sgSurfaceChain[sgSurfaceChain.size() - 1]);

    GFX->setTextureStageColorOp(1, GFXTOPModulate);
    GFX->setTexture(1, sgBloom2);
    GFX->setTextureStageColorOp(2, GFXTOPModulate);
    GFX->setTexture(2, sgBloom2);

    GFX->setTextureStageColorOp(3, GFXTOPModulate);
    GFX->setTexture(3, sgSurfaceChain[0]);

    // tone test...
    GFX->setTextureStageColorOp(4, GFXTOPModulate);
    GFX->setTexture(4, sgSurfaceChain[0]);
    GFX->setTextureStageColorOp(5, GFXTOPModulate);
    GFX->setTextureStageAddressModeU(5, GFXAddressClamp);
    GFX->setTextureStageAddressModeV(5, GFXAddressClamp);
    if (LightManager::sgAllowToneMapping())
        GFX->setTexture(5, sgToneMap);
    else
        GFX->setTexture(5, sgGrayMap);

    // no more needed...
    GFX->setTextureStageColorOp(6, GFXTOPDisable);

    if (LightManager::sgAllowFullDynamicRangeLighting())
        sgDRLFull->getShader()->process();
    else
        sgDRLOnlyBloomTone->getShader()->process();

    //Point4F temp(intensity, fullrange, 0, LightManager::sgDRLMultiplier);
    Point4F temp(LightManager::sgDRLMax, LightManager::sgDRLMin,
        LightManager::sgDRLTarget, LightManager::sgDRLMultiplier);
    if (!LightManager::sgAllowFullDynamicRangeLighting())
        temp.w = 1.0 / (LightManager::sgDRLTarget + 0.0001);
    GFX->setPixelShaderConstF(0, temp, 1);

    // render...
    PrimBuild::color(ColorF(1.0f, 1.0f, 1.0f));
    PrimBuild::begin(GFXTriangleStrip, 4);
    PrimBuild::texCoord2f(texcoord.point.x, texcoord.point.y);
    PrimBuild::vertex2f(rect.point.x, rect.point.y);
    PrimBuild::texCoord2f(texcoord.extent.x, texcoord.point.y);
    PrimBuild::vertex2f(rect.extent.x, rect.point.y);
    PrimBuild::texCoord2f(texcoord.point.x, texcoord.extent.y);
    PrimBuild::vertex2f(rect.point.x, rect.extent.y);
    PrimBuild::texCoord2f(texcoord.extent.x, texcoord.extent.y);
    PrimBuild::vertex2f(rect.extent.x, rect.extent.y);
    PrimBuild::end();

    for (S32 i = 7; i >= 0; i--)
    {
        GFX->setTexture(i, NULL);
        GFX->setTextureStageColorOp(i, GFXTOPDisable);
    }

    GFX->setZEnable(true);
    GFX->setAlphaBlendEnable(false);
}

void sgDRLSurfaceChain::sgDestroyChain()
{
    // free up the texture resources...
    for (U32 i = 0; i < sgSurfaceChain.size(); i++)
        sgSurfaceChain[i] = NULL;

    sgBloom = NULL;
    sgBloom2 = NULL;
    sgToneMap = NULL;
    sgGrayMap = NULL;
    sgDRLViewMap = NULL;

    sgDownSample4x4 = NULL;
    sgDownSample4x4BloomClamp = NULL;
    sgBloomBlur = NULL;
    sgDRLFull = NULL;
    sgDRLOnlyBloomTone = NULL;

    sgSurfaceSize.clear();
    sgSurfaceChain.clear();
}

