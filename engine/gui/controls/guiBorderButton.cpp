//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "gui/core/guiCanvas.h"
#include "gui/controls/guiButtonBaseCtrl.h"


class GuiBorderButtonCtrl : public GuiButtonBaseCtrl
{
    typedef GuiButtonBaseCtrl Parent;

protected:
public:
    DECLARE_CONOBJECT(GuiBorderButtonCtrl);

    void onRender(Point2I offset, const RectI& updateRect);
};

IMPLEMENT_CONOBJECT(GuiBorderButtonCtrl);

void GuiBorderButtonCtrl::onRender(Point2I offset, const RectI& updateRect)
{
    RectI bounds(offset, mBounds.extent);
    if (mActive && mMouseOver)
    {
        bounds.inset(2, 2);
        GFX->drawRect(bounds, mProfile->mFontColorHL);
        bounds.inset(-2, -2);
    }
    if (mActive && (mStateOn || mDepressed))
    {
        GFX->drawRect(bounds, mProfile->mFontColorHL);
        bounds.inset(1, 1);
        GFX->drawRect(bounds, mProfile->mFontColorHL);
    }
    renderChildControls(offset, updateRect);
}

