//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (c) 2002 GarageGames.Com
//-----------------------------------------------------------------------------

#include "gui/core/guiControl.h"
#include "gfx/gfxDevice.h"
/// Renders a skinned border.
class GuiBitmapBorderCtrl : public GuiControl
{
    typedef GuiControl Parent;

    enum {
        BorderTopLeft,
        BorderTopRight,
        BorderTop,
        BorderLeft,
        BorderRight,
        BorderBottomLeft,
        BorderBottom,
        BorderBottomRight,
        NumBitmaps
    };
    RectI* mBitmapBounds;  ///< bmp is [3*n], bmpHL is [3*n + 1], bmpNA is [3*n + 2]
    GFXTexHandle mTextureObject;
public:
    bool onWake();
    void onSleep();
    void onRender(Point2I offset, const RectI& updateRect);
    DECLARE_CONOBJECT(GuiBitmapBorderCtrl);
};

IMPLEMENT_CONOBJECT(GuiBitmapBorderCtrl);

bool GuiBitmapBorderCtrl::onWake()
{
    if (!Parent::onWake())
        return false;

    //get the texture for the close, minimize, and maximize buttons
    mTextureObject = mProfile->mTextureObject;
    bool result = mProfile->constructBitmapArray() >= NumBitmaps;
    AssertFatal(result, "Failed to create the bitmap array");
    if (!result)
        return false;

    mBitmapBounds = mProfile->mBitmapArrayRects.address();
    return true;
}

void GuiBitmapBorderCtrl::onSleep()
{
    mTextureObject = NULL;
    Parent::onSleep();
}

void GuiBitmapBorderCtrl::onRender(Point2I offset, const RectI& updateRect)
{
    renderChildControls(offset, updateRect);
    GFX->setClipRect(updateRect);

    //draw the outline
    RectI winRect;
    winRect.point = offset;
    winRect.extent = mBounds.extent;

    winRect.point.x += mBitmapBounds[BorderLeft].extent.x;
    winRect.point.y += mBitmapBounds[BorderTop].extent.y;

    winRect.extent.x -= mBitmapBounds[BorderLeft].extent.x + mBitmapBounds[BorderRight].extent.x;
    winRect.extent.y -= mBitmapBounds[BorderTop].extent.y + mBitmapBounds[BorderBottom].extent.y;

    if (mProfile->mOpaque)
        GFX->drawRectFill(winRect, mProfile->mFillColor);

    GFX->clearBitmapModulation();
    GFX->drawBitmapSR(mTextureObject, offset, mBitmapBounds[BorderTopLeft]);
    GFX->drawBitmapSR(mTextureObject, Point2I(offset.x + mBounds.extent.x - mBitmapBounds[BorderTopRight].extent.x, offset.y),
        mBitmapBounds[BorderTopRight]);

    RectI destRect;
    destRect.point.x = offset.x + mBitmapBounds[BorderTopLeft].extent.x;
    destRect.point.y = offset.y;
    destRect.extent.x = mBounds.extent.x - mBitmapBounds[BorderTopLeft].extent.x - mBitmapBounds[BorderTopRight].extent.x;
    destRect.extent.y = mBitmapBounds[BorderTop].extent.y;
    RectI stretchRect = mBitmapBounds[BorderTop];
    stretchRect.inset(1, 0);
    GFX->drawBitmapStretchSR(mTextureObject, destRect, stretchRect);

    destRect.point.x = offset.x;
    destRect.point.y = offset.y + mBitmapBounds[BorderTopLeft].extent.y;
    destRect.extent.x = mBitmapBounds[BorderLeft].extent.x;
    destRect.extent.y = mBounds.extent.y - mBitmapBounds[BorderTopLeft].extent.y - mBitmapBounds[BorderBottomLeft].extent.y;
    stretchRect = mBitmapBounds[BorderLeft];
    stretchRect.inset(0, 1);
    GFX->drawBitmapStretchSR(mTextureObject, destRect, stretchRect);

    destRect.point.x = offset.x + mBounds.extent.x - mBitmapBounds[BorderRight].extent.x;
    destRect.extent.x = mBitmapBounds[BorderRight].extent.x;
    destRect.point.y = offset.y + mBitmapBounds[BorderTopRight].extent.y;
    destRect.extent.y = mBounds.extent.y - mBitmapBounds[BorderTopRight].extent.y - mBitmapBounds[BorderBottomRight].extent.y;

    stretchRect = mBitmapBounds[BorderRight];
    stretchRect.inset(0, 1);
    //dglDrawBitmapStretchSR(mTextureObject, destRect, stretchRect);
    GFX->drawBitmapStretchSR(mTextureObject, destRect, stretchRect);
    GFX->drawBitmapSR(mTextureObject, offset + Point2I(0, mBounds.extent.y - mBitmapBounds[BorderBottomLeft].extent.y), mBitmapBounds[BorderBottomLeft]);
    GFX->drawBitmapSR(mTextureObject, offset + mBounds.extent - mBitmapBounds[BorderBottomRight].extent, mBitmapBounds[BorderBottomRight]);

    destRect.point.x = offset.x + mBitmapBounds[BorderBottomLeft].extent.x;
    destRect.extent.x = mBounds.extent.x - mBitmapBounds[BorderBottomLeft].extent.x - mBitmapBounds[BorderBottomRight].extent.x;

    destRect.point.y = offset.y + mBounds.extent.y - mBitmapBounds[BorderBottom].extent.y;
    destRect.extent.y = mBitmapBounds[BorderBottom].extent.y;
    stretchRect = mBitmapBounds[BorderBottom];
    stretchRect.inset(1, 0);

    //dglDrawBitmapStretchSR(mTextureObject, destRect, stretchRect);
    GFX->drawBitmapStretchSR(mTextureObject, destRect, stretchRect);
}
