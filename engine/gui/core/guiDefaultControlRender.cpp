//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (c) 2002 GarageGames.Com
//-----------------------------------------------------------------------------

#include "guiDefaultControlRender.h"
#include "gui/core/guiDefaultControlRender.h"
#include "gui/core/guiTypes.h"
#include "core/color.h"
#include "math/mRect.h"
#include "gfx/gfxDevice.h"

static ColorI colorLightGray(192, 192, 192);
static ColorI colorGray(128, 128, 128);
static ColorI colorDarkGray(64, 64, 64);
static ColorI colorWhite(255, 255, 255);
static ColorI colorBlack(0, 0, 0);

void renderRaisedBox(RectI& bounds, GuiControlProfile* profile)
{
    S32 l = bounds.point.x, r = bounds.point.x + bounds.extent.x - 1;
    S32 t = bounds.point.y, b = bounds.point.y + bounds.extent.y - 1;

    GFX->drawRectFill(bounds, profile->mFillColor);
    GFX->drawLine(l, t, l, b - 1, colorWhite);
    GFX->drawLine(l, t, r - 1, t, colorWhite);

    GFX->drawLine(l, b, r, b, colorBlack);
    GFX->drawLine(r, b - 1, r, t, colorBlack);

    GFX->drawLine(l + 1, b - 1, r - 1, b - 1, profile->mBorderColor);
    GFX->drawLine(r - 1, b - 2, r - 1, t + 1, profile->mBorderColor);
}

void renderSlightlyRaisedBox(RectI& bounds, GuiControlProfile* profile)
{
    S32 l = bounds.point.x, r = bounds.point.x + bounds.extent.x - 1;
    S32 t = bounds.point.y, b = bounds.point.y + bounds.extent.y - 1;

    GFX->drawRectFill(bounds, profile->mFillColor);
    GFX->drawLine(l, t, l, b, colorWhite);
    GFX->drawLine(l, t, r, t, colorWhite);
    GFX->drawLine(l + 1, b, r, b, profile->mBorderColor);
    GFX->drawLine(r, t + 1, r, b - 1, profile->mBorderColor);
}

void renderLoweredBox(RectI& bounds, GuiControlProfile* profile)
{
    S32 l = bounds.point.x, r = bounds.point.x + bounds.extent.x - 1;
    S32 t = bounds.point.y, b = bounds.point.y + bounds.extent.y - 1;

    GFX->drawRectFill(bounds, profile->mFillColor);

    GFX->drawLine(l, b, r, b, colorWhite);
    GFX->drawLine(r, b - 1, r, t, colorWhite);

    GFX->drawLine(l, t, r - 1, t, colorBlack);
    GFX->drawLine(l, t + 1, l, b - 1, colorBlack);

    GFX->drawLine(l + 1, t + 1, r - 2, t + 1, profile->mBorderColor);
    GFX->drawLine(l + 1, t + 2, l + 1, b - 2, profile->mBorderColor);
}

void renderSlightlyLoweredBox(RectI& bounds, GuiControlProfile* profile)
{
    S32 l = bounds.point.x, r = bounds.point.x + bounds.extent.x - 1;
    S32 t = bounds.point.y, b = bounds.point.y + bounds.extent.y - 1;

    GFX->drawRectFill(bounds, profile->mFillColor);
    GFX->drawLine(l, b, r, b, colorWhite);
    GFX->drawLine(r, t, r, b - 1, colorWhite);
    GFX->drawLine(l, t, l, b - 1, profile->mBorderColor);
    GFX->drawLine(l + 1, t, r - 1, t, profile->mBorderColor);
}

void renderBorder(RectI& bounds, GuiControlProfile* profile)
{
    S32 l = bounds.point.x, r = bounds.point.x + bounds.extent.x - 1;
    S32 t = bounds.point.y, b = bounds.point.y + bounds.extent.y - 1;

    switch (profile->mBorder)
    {
    case 1:
        GFX->drawRect(bounds, profile->mBorderColor);
        break;
    case 2:
        GFX->drawLine(l + 1, t + 1, l + 1, b - 2, colorWhite);
        GFX->drawLine(l + 2, t + 1, r - 2, t + 1, colorWhite);
        GFX->drawLine(r, t, r, b, colorWhite);
        GFX->drawLine(l, b, r - 1, b, colorWhite);
        GFX->drawLine(l, t, r - 1, t, profile->mBorderColorNA);
        GFX->drawLine(l, t + 1, l, b - 1, profile->mBorderColorNA);
        GFX->drawLine(l + 1, b - 1, r - 1, b - 1, profile->mBorderColorNA);
        GFX->drawLine(r - 1, t + 1, r - 1, b - 2, profile->mBorderColorNA);
        break;
    case 3:
        GFX->drawLine(l, b, r, b, colorWhite);
        GFX->drawLine(r, t, r, b - 1, colorWhite);
        GFX->drawLine(l + 1, b - 1, r - 1, b - 1, profile->mFillColor);
        GFX->drawLine(r - 1, t + 1, r - 1, b - 2, profile->mFillColor);
        GFX->drawLine(l, t, l, b - 1, profile->mBorderColorNA);
        GFX->drawLine(l + 1, t, r - 1, t, profile->mBorderColorNA);
        GFX->drawLine(l + 1, t + 1, l + 1, b - 2, colorBlack);
        GFX->drawLine(l + 2, t + 1, r - 2, t + 1, colorBlack);
        break;
    case 4:
        GFX->drawLine(l, t, l, b - 1, colorWhite);
        GFX->drawLine(l + 1, t, r, t, colorWhite);
        GFX->drawLine(l, b, r, b, colorBlack);
        GFX->drawLine(r, t + 1, r, b - 1, colorBlack);
        GFX->drawLine(l + 1, b - 1, r - 1, b - 1, profile->mBorderColor);
        GFX->drawLine(r - 1, t + 1, r - 1, b - 2, profile->mBorderColor);
        break;
    }
}

void renderFilledBorder(RectI& bounds, GuiControlProfile* profile)
{
    renderFilledBorder(bounds, profile->mBorderColorHL, profile->mFillColor);
}

void renderFilledBorder(RectI& bounds, ColorI& borderColor, ColorI& fillColor)
{
    S32 l = bounds.point.x, r = bounds.point.x + bounds.extent.x - 1;
    S32 t = bounds.point.y, b = bounds.point.y + bounds.extent.y - 1;

    RectI fillBounds = bounds;
    fillBounds.inset(1, 1);

    GFX->drawRect(bounds, borderColor);
    GFX->drawRectFill(fillBounds, fillColor);
}

