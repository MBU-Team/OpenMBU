//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "gui/containers/guiPaneCtrl.h"

IMPLEMENT_CONOBJECT(GuiPaneControl);

GuiPaneControl::GuiPaneControl()
{
    mMinExtent.set(10, 10);
    mActive = true;
    mCollapsable = true;
    mCollapsed = false;
    mBarBehindText = true;
    mMouseOver = false;
    mDepressed = false;
    mOriginalExtents.set(10, 10);
    mCaption = StringTable->insert("A Pane");
    mCaptionID = StringTable->insert("");
}

void GuiPaneControl::initPersistFields()
{
    Parent::initPersistFields();

    addField("caption", TypeString, Offset(mCaption, GuiPaneControl));
    addField("captionID", TypeString, Offset(mCaptionID, GuiPaneControl));
    addField("collapsable", TypeBool, Offset(mCollapsable, GuiPaneControl));
    addField("barBehindText", TypeBool, Offset(mBarBehindText, GuiPaneControl));
}

bool GuiPaneControl::onWake()
{
    if (!Parent::onWake())
        return false;

    mFont = mProfile->mFonts[0].mFont;
    AssertFatal(mFont, "GuiPaneControl::onWake: invalid font in profile");
    if (mCaptionID && *mCaptionID != 0)
    {
        setCaptionID(mCaptionID);
    }

    mProfile->constructBitmapArray();

    if (mProfile->mBitmapArrayRects.size())
    {
        mThumbSize.set(mProfile->mBitmapArrayRects[0].extent.x, mProfile->mBitmapArrayRects[0].extent.y);
        mThumbSize.setMax(mProfile->mBitmapArrayRects[1].extent);

        if (mFont->getHeight() > mThumbSize.y)
            mThumbSize.y = mFont->getHeight();
    }
    else
    {
        mThumbSize.set(20, 20);
    }

    return true;
}

void GuiPaneControl::onSleep()
{
    Parent::onSleep();
    mFont = NULL;
}

void GuiPaneControl::setCaptionID(const char* id)
{
    S32 n = Con::getIntVariable(id, -1);
    if (n != -1)
    {
        mCaptionID = StringTable->insert(id);
        setCaptionID(n);
    }
}

void GuiPaneControl::setCaptionID(S32 id)
{
    const UTF8* str = getGUIString(id);
    if (str)
        mCaption = StringTable->insert((const char*)str);
}

void GuiPaneControl::resize(const Point2I& newPosition, const Point2I& newExtent)
{

    //call set update both before and after
    setUpdate();
    Point2I actualNewExtent = Point2I(getMax(mMinExtent.x, newExtent.x),
        getMax(mMinExtent.y, newExtent.y));

    mBounds.set(newPosition, actualNewExtent);
    mOriginalExtents.x = actualNewExtent.x;

    GuiControl* parent = getParent();
    if (parent)
        parent->childResized(this);
    setUpdate();

    // Resize the child control if we're not collapsed
    if (size() && !mCollapsed)
    {
        GuiControl* gc = dynamic_cast<GuiControl*>(operator[](0));

        if (gc)
        {
            Point2I offset(0, mThumbSize.y);

            gc->resize(offset, newExtent - offset);
        }
    }
}

void GuiPaneControl::onRender(Point2I offset, const RectI& updateRect)
{
    // Render our awesome little doogong
    if (mProfile->mBitmapArrayRects.size() >= 2 && mCollapsable)
    {
        S32 idx = mCollapsed ? 0 : 1;

        GFX->clearBitmapModulation();
        GFX->drawBitmapStretchSR(
            mProfile->mTextureObject,
            RectI(offset, mProfile->mBitmapArrayRects[idx].extent),
            mProfile->mBitmapArrayRects[idx]
        );

    }

    S32 textWidth = 0;

    if (!mBarBehindText)
    {
        GFX->setBitmapModulation((mMouseOver ? mProfile->mFontColorHL : mProfile->mFontColor));
        textWidth = GFX->drawText(
            mFont,
            Point2I(mThumbSize.x, 0) + offset,
            mCaption,
            mProfile->mFontColors
        );
    }


    // Draw our little bar, too
    if (mProfile->mBitmapArrayRects.size() >= 5)
    {
        GFX->clearBitmapModulation();

        S32 barStart = mThumbSize.x + offset.x + textWidth;
        S32 barTop = mThumbSize.y / 2 + offset.y - mProfile->mBitmapArrayRects[3].extent.y / 2;

        Point2I barOffset(barStart, barTop);

        // Draw the start of the bar...
        GFX->drawBitmapStretchSR(
            mProfile->mTextureObject,
            RectI(barOffset, mProfile->mBitmapArrayRects[2].extent),
            mProfile->mBitmapArrayRects[2]
        );

        // Now draw the middle...
        barOffset.x += mProfile->mBitmapArrayRects[2].extent.x;

        S32 barMiddleSize = (getExtent().x - (barOffset.x - offset.x)) - mProfile->mBitmapArrayRects[4].extent.x;

        if (barMiddleSize > 0)
        {
            // We have to do this inset to prevent nasty stretching artifacts
            RectI foo = mProfile->mBitmapArrayRects[3];
            foo.inset(1, 0);

            GFX->drawBitmapStretchSR(
                mProfile->mTextureObject,
                RectI(barOffset, Point2I(barMiddleSize, mProfile->mBitmapArrayRects[3].extent.y)),
                foo
            );
        }

        // And the end
        barOffset.x += barMiddleSize;

        GFX->drawBitmapStretchSR(
            mProfile->mTextureObject,
            RectI(barOffset, mProfile->mBitmapArrayRects[4].extent),
            mProfile->mBitmapArrayRects[4]
        );
    }

    if (mBarBehindText)
    {
        GFX->setBitmapModulation((mMouseOver ? mProfile->mFontColorHL : mProfile->mFontColor));
        GFX->drawText(
            mFont,
            Point2I(mThumbSize.x, 0) + offset,
            mCaption,
            mProfile->mFontColors
        );
    }

    // Draw child controls if appropriate
    if (!mCollapsed)
        renderChildControls(offset, updateRect);
}

ConsoleMethod(GuiPaneControl, setCollapsed, void, 3, 3, "(bool)")
{
    object->setCollapsed(dAtob(argv[2]));
}

void GuiPaneControl::setCollapsed(bool isCollapsed)
{
    // Get the child
    if (size() == 0 || !mCollapsable) return;

    GuiControl* gc = dynamic_cast<GuiControl*>(operator[](0));

    if (mCollapsed && !isCollapsed)
    {
        mCollapsed = false;

        resize(getPosition(), mOriginalExtents);

        if (gc)
            gc->setVisible(true);
    }
    else if (!mCollapsed && isCollapsed)
    {
        mCollapsed = true;

        mOriginalExtents = getExtent();
        resize(getPosition(), Point2I(getExtent().x, mThumbSize.y));

        if (gc)
            gc->setVisible(false);
    }
}

void GuiPaneControl::onMouseMove(const GuiEvent& event)
{
    Point2I localMove = globalToLocalCoord(event.mousePoint);

    // If we're clicking in the header then resize
    mMouseOver = (localMove.y < mThumbSize.y);
    if (isMouseLocked())
        mDepressed = mMouseOver;

}

void GuiPaneControl::onMouseEnter(const GuiEvent& event)
{
    setUpdate();
    if (isMouseLocked())
    {
        mDepressed = true;
        mMouseOver = true;
    }
    else
    {
        mMouseOver = true;
    }

}

void GuiPaneControl::onMouseLeave(const GuiEvent& event)
{
    setUpdate();
    if (isMouseLocked())
        mDepressed = false;
    mMouseOver = false;
}

void GuiPaneControl::onMouseDown(const GuiEvent& event)
{
    if (!mCollapsable)
        return;

    Point2I localClick = globalToLocalCoord(event.mousePoint);

    // If we're clicking in the header then resize
    if (localClick.y < mThumbSize.y)
    {
        mouseLock();
        mDepressed = true;

        //update
        setUpdate();
    }
}

void GuiPaneControl::onMouseUp(const GuiEvent& event)
{
    // Make sure we only get events we ought to be getting...
    if (!mActive)
        return;

    if (!mCollapsable)
        return;

    mouseUnlock();
    setUpdate();

    Point2I localClick = globalToLocalCoord(event.mousePoint);

    // If we're clicking in the header then resize
    if (localClick.y < mThumbSize.y && mDepressed)
        setCollapsed(!mCollapsed);
}
