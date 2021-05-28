//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "console/console.h"
#include "console/consoleTypes.h"
#include "game/gameConnection.h"
#include "game/marble/marble.h"

#include "gui/game/guiEnergyBarHud.h"

//--------------------------------------------------------
IMPLEMENT_CONOBJECT(GuiEnergyBarCtrl);

GuiEnergyBarCtrl::GuiEnergyBarCtrl()
{
    for (S32 i =  0; i < 3; i++)
        mStateColors[i] = ColorI(0, 0, 128);

    minActiveValue = 0.2f;
}

void GuiEnergyBarCtrl::initPersistFields()
{
    Parent::initPersistFields(false);

    addField("inactiveColor", TypeColorI, Offset(mStateColors[0], GuiEnergyBarCtrl));
    addField("activeColor", TypeColorI, Offset(mStateColors[1], GuiEnergyBarCtrl));
    addField("ultraColor", TypeColorI, Offset(mStateColors[2], GuiEnergyBarCtrl));
    addField("minActiveValue", TypeF32, Offset(minActiveValue, GuiEnergyBarCtrl));
}

//--------------------------------------------------------

F32 GuiEnergyBarCtrl::getValue()
{
    GameConnection* con = GameConnection::getConnectionToServer();
    if (!con)
        return 0.0f;

    ShapeBase* control = con->getControlObject();
    if (!control || (control->getTypeMask() & PlayerObjectType) == 0)
        return 0.0f;

    Marble* marble = dynamic_cast<Marble*>(control);
    if (!marble)
        return control->getEnergyValue();

    F32 pct = marble->getRenderBlastPercent();
    if (pct >= 0.8333333f)
        return (pct - 0.8333333f) * 1.2f / 0.2f + 1.0f;

    return pct / 0.8333333f;
}

void GuiEnergyBarCtrl::onRender(Point2I offset, const RectI& updateRect)
{
    F32 blastVal = getValue();
    for (S32 i = 0; i < 2; i++)
    {
        F32 val;
        ColorI color;

        if (i == 1)
        {
            val = mClampF(blastVal - 1.0f, 0.0f, 1.0f);
            color = mStateColors[2];
        } else
        {
            val = mClampF(blastVal, 0.0f, 1.0f);
            if (val < minActiveValue)
                color = mStateColors[0];
            else
                color = mStateColors[1];
        }
        
        RectI ctrlRect(offset, mBounds.extent);

        Point2F texCoords[4];
        ColorI colors[4];

        if (val > 0.00001f)
        {
            RectI renderRect(offset, ctrlRect.extent);
            
            if (mOrientation == LeftRight)
            {
                renderRect.extent.x = (F32)ctrlRect.extent.x * val;
                colors[0] = color;
                colors[1] = color;
                colors[2] = color;
                colors[3] = color;
                texCoords[0].set(0.0f, 0.0f);
                texCoords[1].set(val, 0.0f);
                texCoords[2].set(val, 1.0f);
                texCoords[3].set(0.0f, 1.0f);
            }

            if (mMaskTexture.isValid())
            {
                F32 width = (F32)mMaskTexture->mBitmapSize.x / mMaskTexture->mTextureSize.x;
                F32 height = (F32)mMaskTexture->mBitmapSize.y / mMaskTexture->mTextureSize.y;

                for (S32 j = 0; j < 4; j++)
                {
                    if (texCoords[j].x != 0.0f)
                        texCoords[j].x *= width;
                    if (texCoords[j].y != 0.0f)
                        texCoords[j].y *= height;
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
            }
            else
            {
                GFX->setTextureStageColorOp(0, GFXTOPDisable);
                GFX->setTextureStageColorOp(1, GFXTOPDisable);
                GFX->setupGenericShaders();
            }

            GFX->setVertexBuffer(verts);
            GFX->drawPrimitive(GFXTriangleFan, 0, 2);
            GFX->setAlphaBlendEnable(false);
        }
    }
}
