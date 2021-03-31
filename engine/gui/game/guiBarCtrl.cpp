//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "console/console.h"
#include "console/consoleTypes.h"
#include "gui/core/guiDefaultControlRender.h"

#include "gui/game/guiBarCtrl.h"


//--------------------------------------------------------
IMPLEMENT_CONOBJECT(GuiBarCtrl);

GuiBarCtrl::GuiBarCtrl()
{
    mMaskTexture = NULL;
    mValue = 0.0f;
    mColors[0] = ColorI(255, 255, 255);
    mColors[1] = ColorI(255, 255, 255);

    mMaskTextureName = "";
}

static EnumTable::Enums barCtrlEnums[] =
{
    { GuiBarCtrl::LeftRight,     "LeftRight"  },
    { GuiBarCtrl::RightLeft,     "RightLeft"  },
    { GuiBarCtrl::TopBottom,     "TopBottom"  },
    { GuiBarCtrl::BottomTop,     "BottomTop"  },
};
static EnumTable gBarCtrlEnums(sizeof(barCtrlEnums) / sizeof(EnumTable::Enums), &barCtrlEnums[0]);

void GuiBarCtrl::initPersistFields(bool accessVal)
{
    Parent::initPersistFields();

    addField("direction", TypeEnum, Offset(mOrientation, GuiBarCtrl), 1, &gBarCtrlEnums);
    addField("texture", TypeFilename, Offset(mMaskTextureName, GuiBarCtrl));
    addField("startColor", TypeColorI, Offset(mColors[0], GuiBarCtrl));
    addField("endColor", TypeColorI, Offset(mColors[1], GuiBarCtrl));

    if (accessVal)
        addField("value", TypeF32, Offset(mValue, GuiBarCtrl));
}

void GuiBarCtrl::inspectPostApply()
{
    if (mMaskTextureName)
        mMaskTexture.set(mMaskTextureName, &GFXDefaultStaticDiffuseProfile);

    Parent::inspectPostApply();
}

//--------------------------------------------------------

bool GuiBarCtrl::onWake()
{
    if (mMaskTextureName)
        mMaskTexture.set(mMaskTextureName, &GFXDefaultStaticDiffuseProfile);

    return Parent::onWake();
}

F32 GuiBarCtrl::getValue()
{
    return mValue;
}

void GuiBarCtrl::onRender(Point2I offset, const RectI& updateRect)
{
    F32 val = mClampF(getValue(), 0.0f, 1.0f);
    RectI ctrlRect(offset, mBounds.extent);

    Point2F texCoords[4];
    ColorI colors[4];
    
    static ColorI colorEnd = mColors[1] * val + mColors[0] * (1.0f - val);

    if (val > 0.00001f)
    {
        RectI renderRect(offset, ctrlRect.extent);
        
        if (mOrientation == LeftRight)
        {
            renderRect.extent.x = (F32)ctrlRect.extent.x * val;
            colors[0] = mColors[0];
            colors[1] = colorEnd;
            colors[2] = colorEnd;
            colors[3] = colors[0];
            texCoords[0].set(0.0f, 0.0f);
            texCoords[1].set(val, 0.0f);
            texCoords[2].set(val, 1.0f);
            texCoords[3].set(0.0f, 1.0f);
        }
        else if (mOrientation == RightLeft)
        {
            S32 xOffset = ctrlRect.extent.x - (S32)(val * (F32)ctrlRect.extent.x);
            renderRect.point.x = xOffset + offset.x;
            renderRect.extent.x -= xOffset;
            colors[0] = colorEnd;
            colors[1] = mColors[0];
            colors[2] = colors[1];
            colors[3] = colorEnd;
            texCoords[0].set(1.0f, 0.0f);
            texCoords[1].set(1.0f - val, 0.0f);
            texCoords[2].set(1.0f - val, 1.0f);
            texCoords[3].set(1.0f, 1.0f);
        }
        else if (mOrientation == BottomTop)
        {
            S32 yOffset = ctrlRect.extent.y - (S32)(val * (F32)ctrlRect.extent.y);
            renderRect.point.y = yOffset + offset.y;
            renderRect.extent.y -= yOffset;
            colors[0] = colorEnd;
            colors[1] = colorEnd;
            colors[2] = mColors[0];
            colors[3] = colors[2];
            texCoords[0].set(0.0f, 1.0f - val);
            texCoords[1].set(1.0f, 1.0f - val);
            texCoords[2].set(1.0f, 1.0f);
            texCoords[3].set(0.0f, 1.0f);
        }
        else if (mOrientation == TopBottom)
        {
            renderRect.extent.y = (F32)ctrlRect.extent.y * val;
            colors[0] = mColors[0];
            colors[1] = colors[0];
            colors[2] = colorEnd;
            colors[3] = colorEnd;
            texCoords[0].set(0.0f, 0.0f);
            texCoords[1].set(1.0f, 0.0f);
            texCoords[2].set(1.0f, val);
            texCoords[3].set(0.0f, val);
        }

        if (mMaskTexture.isValid())
        {
            F32 width = (F32)mMaskTexture->mBitmapSize.x / mMaskTexture->mTextureSize.x;
            F32 height = (F32)mMaskTexture->mBitmapSize.y / mMaskTexture->mTextureSize.y;

            for (S32 i = 0; i < 4; i++)
            {
                if (texCoords[i].x != 0.0f)
                    texCoords[i].x *= width;
                if (texCoords[i].y != 0.0f)
                    texCoords[i].y *= height;
            }
        }

        GFXVertexBufferHandle<GFXVertexPCT> verts(GFX, 4, GFXBufferTypeVolatile);
        verts.lock();

        verts[0].point.x = renderRect.point.x;
        verts[0].point.y = renderRect.point.y;
        verts[0].point.z = 0.0f;
        verts[0].color = colors[0];
        verts[0].texCoord = texCoords[0];

        verts[1].point.x = renderRect.extent.x + renderRect.point.x;
        verts[1].point.y = renderRect.point.y;
        verts[1].point.z = 0.0f;
        verts[1].color = colors[1];
        verts[1].texCoord = texCoords[1];

        verts[2].point.x = renderRect.extent.x + renderRect.point.x;
        verts[2].point.y = renderRect.extent.y + renderRect.point.y;
        verts[2].point.z = 0.0f;
        verts[2].color = colors[2];
        verts[2].texCoord = texCoords[2];

        verts[3].point.x = renderRect.point.x;
        verts[3].point.y = renderRect.extent.y + renderRect.point.y;
        verts[3].point.z = 0.0f;
        verts[3].color = colors[3];
        verts[3].texCoord = texCoords[3];

        verts.unlock();

        GFX->setBaseRenderState();
        GFX->setCullMode(GFXCullNone);
        GFX->setLightingEnable(false);
        GFX->setAlphaBlendEnable(true);
        GFX->setSrcBlend(GFXBlendSrcAlpha);
        GFX->setDestBlend(GFXBlendInvSrcAlpha);

        if (mMaskTexture.isValid())
        {
            GFX->setTextureStageColorOp(0, GFXTOPModulate);
            GFX->setTextureStageColorOp(1, GFXTOPDisable);
            GFX->setTexture(0, mMaskTexture);
            GFX->setupGenericShaders(GFXDevice::GSModColorTexture);
        } else
        {
            GFX->setTextureStageColorOp(0, GFXTOPDisable);
            GFX->setTextureStageColorOp(1, GFXTOPDisable);
            GFX->setupGenericShaders();
        }

        GFX->setVertexBuffer(verts);
        GFX->drawPrimitive(GFXTriangleFan, 0, 2);
        GFX->setAlphaBlendEnable(false);
    }

    if (mProfile && mProfile->mBorder)
        renderBorder(ctrlRect, mProfile);
}
