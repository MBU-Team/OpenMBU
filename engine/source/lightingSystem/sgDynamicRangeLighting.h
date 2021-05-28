//-----------------------------------------------
// Synapse Gaming - Lighting System
// Copyright © Synapse Gaming 2003
// Written by John Kabus
//-----------------------------------------------
#ifndef _SGLIGHTINGFEATURES_H_
#define _SGLIGHTINGFEATURES_H_

#include "core/tVector.h"
#include "gfx/gfxTextureHandle.h"
#include "gui/core/guiControl.h"
#include "materials/shaderData.h"


class sgDRLSurfaceChain
{
private:
    enum
    {
        sgdrlscSampleWidth = 4,
        sgdlrscBloomIndex = 2
    };

    Point2I sgOffset;
    Point2I sgExtent;

    //ShaderData *sgAlphaBloom;
    ShaderData* sgDownSample4x4;
    ShaderData* sgDownSample4x4BloomClamp;
    ShaderData* sgBloomBlur;
    ShaderData* sgDRLFull;
    ShaderData* sgDRLOnlyBloomTone;

    Vector<Point2I> sgSurfaceSize;
    GFXTexHandle sgBloom;
    GFXTexHandle sgBloom2;
    GFXTexHandle sgToneMap;
    GFXTexHandle sgGrayMap;
    GFXTexHandle sgDRLViewMap;

    bool sgCachedIsHDR;
    void sgDestroyChain();

protected:
    Vector<GFXTexHandle> sgSurfaceChain;

    sgDRLSurfaceChain()
    {
        sgOffset = Point2I(0, 0);
        //sgExtent = Point2I(0, 0);
        sgDownSample4x4 = NULL;
        sgDownSample4x4BloomClamp = NULL;
        sgBloomBlur = NULL;
        sgDRLFull = NULL;
        sgDRLOnlyBloomTone = NULL;
        sgCachedIsHDR = false;
    }
    virtual ~sgDRLSurfaceChain() { sgDestroyChain(); }

    // detect primary surface changes...
    void sgPrepChain(const Point2I& offset, const Point2I& extent);
    // make copies and down-sample...
    void sgRenderChain();
    // render...
    void sgRenderDRL();
};

class sgDRLSystem : public sgDRLSurfaceChain
{
public:
    bool sgDidPrep;
    sgDRLSystem() { sgDidPrep = false; }
    void sgPrepSystem(const Point2I& offset, const Point2I& extent);
    void sgRenderSystem();
};


class sgGuiTexTestCtrl : public GuiControl
{
private:
    typedef GuiControl Parent;

    S32 sgTextureLevel;

public:
    //creation methods
    DECLARE_CONOBJECT(sgGuiTexTestCtrl);

    sgGuiTexTestCtrl() { sgTextureLevel = 0; }
    static void initPersistFields();
    void onRender(Point2I offset, const RectI& updateRect);
};


#endif//_SGLIGHTINGFEATURES_H_
