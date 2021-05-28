//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "console/console.h"
#include "console/consoleTypes.h"


#include "gui/game/guiProgressCtrl.h"

IMPLEMENT_CONOBJECT(GuiProgressCtrl);

GuiProgressCtrl::GuiProgressCtrl()
{
    mProgress = 0.0f;
}

const char* GuiProgressCtrl::getScriptValue()
{
    char* ret = Con::getReturnBuffer(64);
    dSprintf(ret, 64, "%g", mProgress);
    return ret;
}

void GuiProgressCtrl::setScriptValue(const char* value)
{
    //set the value
    if (!value)
        mProgress = 0.0f;
    else
        mProgress = dAtof(value);

    //validate the value
    mProgress = mClampF(mProgress, 0.f, 1.f);
    setUpdate();
}

void GuiProgressCtrl::onPreRender()
{
    const char* var = getVariable();
    if (var)
    {
        F32 value = mClampF(dAtof(var), 0.f, 1.f);
        if (value != mProgress)
        {
            mProgress = value;
            setUpdate();
        }
    }
}

void GuiProgressCtrl::onRender(Point2I offset, const RectI& updateRect)
{
    RectI ctrlRect(offset, mBounds.extent);

    //draw the progress
    S32 width = (S32)((F32)mBounds.extent.x * mProgress);
    if (width > 0)
    {
        RectI progressRect = ctrlRect;
        progressRect.extent.x = width;
        GFX->drawRectFill(progressRect, mProfile->mFillColor);
    }

    //now draw the border
    if (mProfile->mBorder)
    {
        GFX->drawRect(ctrlRect, mProfile->mBorderColor);

        RectI copyRect = ctrlRect;
        copyRect.point.x += 1;
        copyRect.point.y += 1;
        copyRect.extent.x -= 2;
        copyRect.extent.y -= 2;
        ColorI color = mProfile->mBorderColor * 0.75f;
        color.alpha = 255;
        GFX->drawRect(copyRect, color);
    }

    //render the children
    renderChildControls(offset, updateRect);
}

