//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "console/console.h"
#include "console/consoleTypes.h"
#include "guiSliderCtrl.h"
#include "platform/event.h"
#include "gui/core/guiDefaultControlRender.h"
#include "gfx/gfxDevice.h"

IMPLEMENT_CONOBJECT(GuiSliderCtrl);

//----------------------------------------------------------------------------
GuiSliderCtrl::GuiSliderCtrl(void)
{
    mActive = true;
    mRange.set(0.0f, 1.0f);
    mTicks = 10;
    mValue = 0.5f;
    mThumbSize.set(8, 20);
    mShiftPoint = 5;
    mShiftExtent = 10;
    mDisplayValue = false;
    mBitmapName = StringTable->insert("");
    mBitmap = NULL;
}

//----------------------------------------------------------------------------
void GuiSliderCtrl::initPersistFields()
{
    Parent::initPersistFields();

    addGroup("Slider");
    addField("range", TypePoint2F, Offset(mRange, GuiSliderCtrl));
    addField("ticks", TypeS32, Offset(mTicks, GuiSliderCtrl));
    addField("value", TypeF32, Offset(mValue, GuiSliderCtrl));
    addField("bitmap", TypeFilename, Offset(mBitmapName, GuiSliderCtrl));
    endGroup("Slider");
}

//----------------------------------------------------------------------------
ConsoleMethod(GuiSliderCtrl, getValue, F32, 2, 2, "Get the position of the slider.")
{
    return object->getValue();
}

//----------------------------------------------------------------------------
void GuiSliderCtrl::setScriptValue(const char* val)
{
    mValue = dAtof(val);
    updateThumb(mValue);
}

//----------------------------------------------------------------------------
bool GuiSliderCtrl::onWake()
{
    if (!Parent::onWake())
        return false;

    setBitmap(mBitmapName);

    mValue = mClampF(getFloatVariable(), mRange.x, mRange.y);

    if (mThumbSize.y + mProfile->mFonts[0].mFont->getHeight() - 4 <= mBounds.extent.y)
        mDisplayValue = true;
    else
        mDisplayValue = false;

    updateThumb(mValue, true);

    return true;
}

//----------------------------------------------------------------------------
void GuiSliderCtrl::onMouseDown(const GuiEvent& event)
{
    if (!mActive || !mAwake || !mVisible)
        return;

    mouseLock();
    setFirstResponder();

    Point2I curMousePos = globalToLocalCoord(event.mousePoint);
    F32 value;
    if (mBounds.extent.x >= mBounds.extent.y)
        value = F32(curMousePos.x - mShiftPoint) / F32(mBounds.extent.x - mShiftExtent) * (mRange.y - mRange.x) + mRange.x;
    else
        value = F32(curMousePos.y) / F32(mBounds.extent.y) * (mRange.y - mRange.x) + mRange.x;
    updateThumb(value);
}

//----------------------------------------------------------------------------
void GuiSliderCtrl::onMouseDragged(const GuiEvent& event)
{
    if (!mActive || !mAwake || !mVisible)
        return;

    Point2I curMousePos = globalToLocalCoord(event.mousePoint);
    F32 value;
    if (mBounds.extent.x >= mBounds.extent.y)
        value = F32(curMousePos.x - mShiftPoint) / F32(mBounds.extent.x - mShiftExtent) * (mRange.y - mRange.x) + mRange.x;
    else
        value = F32(curMousePos.y) / F32(mBounds.extent.y) * (mRange.y - mRange.x) + mRange.x;

    if (value > mRange.y)
        value = mRange.y;
    else if (value < mRange.x)
        value = mRange.x;

    if ((event.modifier & SI_SHIFT) && mTicks > 2) {
        // If the shift key is held, snap to the nearest tick, if any are being drawn

        F32 tickStep = (mRange.y - mRange.x) / F32(mTicks + 1);

        F32 tickSteps = (value - mRange.x) / tickStep;
        S32 actualTick = S32(tickSteps + 0.5);

        value = actualTick * tickStep + mRange.x;
        AssertFatal(value <= mRange.y && value >= mRange.x, "Error, out of bounds value generated from shift-snap of slider");
    }

    updateThumb(value);
}

//----------------------------------------------------------------------------
void GuiSliderCtrl::onMouseUp(const GuiEvent&)
{
    if (!mActive || !mAwake || !mVisible)
        return;

    mouseUnlock();
    if (mConsoleCommand[0])
    {
        char buf[16];
        dSprintf(buf, sizeof(buf), "%d", getId());
        Con::setVariable("$ThisControl", buf);
        Con::evaluate(mConsoleCommand, false);
    }
}

//----------------------------------------------------------------------------
void GuiSliderCtrl::updateThumb(F32 value, bool onWake)
{
    mValue = value;
    // clamp the thumb to legal values
    if (mValue < mRange.x)  mValue = mRange.x;
    if (mValue > mRange.y)  mValue = mRange.y;

    Point2I ext = mBounds.extent;
    ext.x -= (mShiftExtent + mThumbSize.x) / 2;
    // update the bounding thumb rect
    if (mBounds.extent.x >= mBounds.extent.y)
    {  // HORZ thumb
        S32 mx = (S32)((F32(ext.x) * (mValue - mRange.x) / (mRange.y - mRange.x)));
        S32 my = ext.y / 2;
        if (mDisplayValue)
            my = mThumbSize.y / 2;

        mThumb.point.x = mx - (mThumbSize.x / 2);
        mThumb.point.y = my - (mThumbSize.y / 2);
        mThumb.extent = mThumbSize;
    }
    else
    {  // VERT thumb
        S32 mx = ext.x / 2;
        S32 my = (S32)((F32(ext.y) * (mValue - mRange.x) / (mRange.y - mRange.x)));
        mThumb.point.x = mx - (mThumbSize.y / 2);
        mThumb.point.y = my - (mThumbSize.x / 2);
        mThumb.extent.x = mThumbSize.y;
        mThumb.extent.y = mThumbSize.x;
    }
    setFloatVariable(mValue);
    setUpdate();

    // Use the alt console command if you want to continually update:
    if (!onWake && mAltConsoleCommand[0])
        Con::evaluate(mAltConsoleCommand, false);
}

//----------------------------------------------------------------------------
void GuiSliderCtrl::onRender(Point2I offset, const RectI& updateRect)
{
    Point2I pos(offset.x + mShiftPoint, offset.y);
    Point2I ext(mBounds.extent.x - mShiftExtent, mBounds.extent.y);

    if (mBitmap)
    {
        Point2I pos(mThumb.point.x + offset.x, mThumb.point.y + offset.y);
        GFX->clearBitmapModulation();
        GFX->drawBitmap(mBitmap, pos);
        return;
    }

    if (mBounds.extent.x >= mBounds.extent.y)
    {
        Point2I mid(ext.x, ext.y / 2);
        if (mDisplayValue)
            mid.set(ext.x, mThumbSize.y / 2);

        // horz rule
        GFX->drawLine(pos.x, pos.y + mid.y, pos.x + mid.x, pos.y + mid.y, ColorI(0, 0, 0, 255));

        // tick marks
        for (U32 t = 0; t <= (mTicks + 1); t++)
        {
            S32 x = (S32)(F32(mid.x - 1) / F32(mTicks + 1) * F32(t));
            GFX->drawLine(pos.x + x, pos.y + mid.y - mShiftPoint,
                pos.x + x, pos.y + mid.y + mShiftPoint, ColorI(0, 0, 0, 255));
        }
        //glEnd();
    }
    else
    {
        Point2I mid(ext.x / 2, ext.y);
        GFX->drawLine(pos.x + mid.x, pos.y, pos.x + mid.x, pos.y + mid.y, ColorI(0, 0, 0, 1));

        for (U32 t = 0; t <= (mTicks + 1); t++)
        {
            S32 y = (S32)(F32(mid.y - 1) / F32(mTicks + 1) * F32(t));

            GFX->drawLine(pos.x + mid.x - mShiftPoint, pos.y + y,
                pos.x + mid.x + mShiftPoint, pos.y + y, ColorI(0, 0, 0, 1));
        }
        mDisplayValue = false;
    }
    // draw the thumb
    RectI thumb = mThumb;
    thumb.point += pos;
    renderRaisedBox(thumb, mProfile);

    if (mDisplayValue)
    {
        char buf[20];
        dSprintf(buf, sizeof(buf), "%0.3f", mValue);

        Point2I textStart = thumb.point;

        S32 txt_w = mProfile->mFonts[0].mFont->getStrWidth((const UTF8*)buf);

        textStart.x += (S32)((thumb.extent.x / 2.0f));
        textStart.y += thumb.extent.y - 2; //19
        textStart.x -= (txt_w / 2);
        if (textStart.x < offset.x)
            textStart.x = offset.x;
        else if (textStart.x + txt_w > offset.x + mBounds.extent.x)
            textStart.x -= ((textStart.x + txt_w) - (offset.x + mBounds.extent.x));

        GFX->setBitmapModulation(mProfile->mFontColor);
        GFX->drawText(mProfile->mFonts[0].mFont, textStart, buf, mProfile->mFontColors);
    }
    renderChildControls(offset, updateRect);
}

void GuiSliderCtrl::setBitmap(const char *name)
{
    mBitmapName = StringTable->insert(name);
    if (*mBitmapName)
    {
        mBitmap.set(mBitmapName, &GFXDefaultGUIProfile);

        Point2I bitmapExtent(0, 0);
        // Resize the control to fit the bitmap
        if (mBitmap)
        {
            bitmapExtent.x = mBitmap->getWidth();
            bitmapExtent.y = mBitmap->getHeight();
        }

        mThumbSize.x = bitmapExtent.x;
        mThumbSize.y = bitmapExtent.y;
        mShiftExtent = 0;
    }
    else
        mBitmap = NULL;

    setUpdate();
}

