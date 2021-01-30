//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "console/consoleTypes.h"
#include "console/console.h"
#include "gfx/gBitmap.h"
#include "core/resManager.h"
#include "platform/event.h"
#include "gui/core/guiArrayCtrl.h"
#include "gui/containers/guiScrollCtrl.h"
#include "gui/core/guiDefaultControlRender.h"
#include "gfx/gfxDevice.h"

IMPLEMENT_CONOBJECT(GuiScrollCtrl);

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- //

GuiScrollCtrl::GuiScrollCtrl()
{
    mBounds.extent.set(200, 200);
    mChildMargin.set(0, 0);
    mBorderThickness = 1;
    mScrollBarThickness = 16;
    mScrollBarArrowBtnLength = 16;
    stateDepressed = false;
    curHitRegion = None;

    mWillFirstRespond = true;
    mUseConstantHeightThumb = false;

    mForceVScrollBar = ScrollBarAlwaysOn;
    mForceHScrollBar = ScrollBarAlwaysOn;
}

static EnumTable::Enums scrollBarEnums[] =
{
   { GuiScrollCtrl::ScrollBarAlwaysOn,     "alwaysOn"     },
   { GuiScrollCtrl::ScrollBarAlwaysOff,    "alwaysOff"    },
   { GuiScrollCtrl::ScrollBarDynamic,      "dynamic"      },
};
static EnumTable gScrollBarTable(3, &scrollBarEnums[0]);

ConsoleMethod(GuiScrollCtrl, scrollToTop, void, 2, 2, "() - scrolls the scroll control to the top of the child content area.")
{
    argc; argv;
    object->scrollTo(0, 0);
}

ConsoleMethod(GuiScrollCtrl, scrollToBottom, void, 2, 2, "() - scrolls the scroll control to the bottom of the child content area.")
{
    argc; argv;
    object->scrollTo(0, 0x7FFFFFFF);
}

void GuiScrollCtrl::initPersistFields()
{
    Parent::initPersistFields();

    addField("willFirstRespond", TypeBool, Offset(mWillFirstRespond, GuiScrollCtrl));
    addField("hScrollBar", TypeEnum, Offset(mForceHScrollBar, GuiScrollCtrl), 1, &gScrollBarTable);
    addField("vScrollBar", TypeEnum, Offset(mForceVScrollBar, GuiScrollCtrl), 1, &gScrollBarTable);
    addField("constantThumbHeight", TypeBool, Offset(mUseConstantHeightThumb, GuiScrollCtrl));
    addField("childMargin", TypePoint2I, Offset(mChildMargin, GuiScrollCtrl));

}

void GuiScrollCtrl::resize(const Point2I& newPos, const Point2I& newExt)
{
    Parent::resize(newPos, newExt);
    computeSizes();
}

void GuiScrollCtrl::childResized(GuiControl* child)
{
    Parent::childResized(child);
    computeSizes();
}

bool GuiScrollCtrl::onWake()
{
    if (!Parent::onWake())
        return false;

    mTextureObject = mProfile->mTextureObject;
    if (mTextureObject)
    {
        bool result = mProfile->constructBitmapArray() >= BmpStates * BmpCount;

        AssertFatal(result, "Failed to create the bitmap array");

        mBitmapBounds = mProfile->mBitmapArrayRects.address();

        //init
        mBaseThumbSize = mBitmapBounds[BmpStates * BmpVThumbTopCap].extent.y +
            mBitmapBounds[BmpStates * BmpVThumbBottomCap].extent.y;
        mScrollBarThickness = mBitmapBounds[BmpStates * BmpVPage].extent.x;
        mScrollBarArrowBtnLength = mBitmapBounds[BmpStates * BmpUp].extent.y;
        computeSizes();
    }
    else
    {
        Con::warnf("No texture loaded for scroll control named %s with profile %s", getName(), mProfile->getName());
    }
    return true;
}

void GuiScrollCtrl::onSleep()
{
    Parent::onSleep();
    mTextureObject = NULL;
}

bool GuiScrollCtrl::calcChildExtents()
{
    // right now the scroll control really only deals well with a single
    // child control for its content
    if (!size())
        return false;
    GuiControl* ctrl = (GuiControl*)front();
    mChildExt = ctrl->mBounds.extent;
    mChildPos = ctrl->mBounds.point;
    return true;
}

void GuiScrollCtrl::scrollRectVisible(RectI rect)
{
    // rect is passed in virtual client space
    if (rect.extent.x > mContentExt.x)
        rect.extent.x = mContentExt.x;
    if (rect.extent.y > mContentExt.y)
        rect.extent.y = mContentExt.y;

    // Determine the points bounding the requested rectangle
    Point2I rectUpperLeft = rect.point;
    Point2I rectLowerRight = rect.point + rect.extent;
    // Determine the points bounding the actual visible area...
    Point2I visUpperLeft = mChildRelPos;
    Point2I visLowerRight = mChildRelPos + mContentExt;
    Point2I delta(0, 0);

    // We basically try to make sure that first the top left of the given
    // rect is visible, and if it is, then that the bottom right is visible.

    // Make sure the rectangle is visible along the X axis...
    if (rectUpperLeft.x < visUpperLeft.x)
        delta.x = rectUpperLeft.x - visUpperLeft.x;
    else if (rectLowerRight.x > visLowerRight.x)
        delta.x = rectLowerRight.x - visLowerRight.x;

    // Make sure the rectangle is visible along the Y axis...
    if (rectUpperLeft.y < visUpperLeft.y)
        delta.y = rectUpperLeft.y - visUpperLeft.y;
    else if (rectLowerRight.y > visLowerRight.y)
        delta.y = rectLowerRight.y - visLowerRight.y;

    // If we had any changes, scroll, otherwise don't.
    if (delta.x || delta.y)
        scrollDelta(delta.x, delta.y);
}


void GuiScrollCtrl::addObject(SimObject* object)
{
    Parent::addObject(object);
    computeSizes();
}

GuiControl* GuiScrollCtrl::findHitControl(const Point2I& pt, S32 initialLayer)
{
    if (pt.x < mProfile->mBorderThickness || pt.y < mProfile->mBorderThickness)
        return this;
    if (pt.x >= mBounds.extent.x - mProfile->mBorderThickness - (mHasVScrollBar ? mScrollBarThickness : 0) ||
        pt.y >= mBounds.extent.y - mProfile->mBorderThickness - (mHasHScrollBar ? mScrollBarThickness : 0))
        return this;
    return Parent::findHitControl(pt, initialLayer);
}

void GuiScrollCtrl::computeSizes()
{
    S32 thickness = (mProfile ? mProfile->mBorderThickness : 1);
    Point2I borderExtent(thickness, thickness);
    mContentPos = borderExtent + mChildMargin;
    mContentExt = mBounds.extent - (mChildMargin * 2)
        - (borderExtent * 2);

    Point2I childLowerRight;

    mHBarEnabled = false;
    mVBarEnabled = false;
    mHasVScrollBar = (mForceVScrollBar == ScrollBarAlwaysOn);
    mHasHScrollBar = (mForceHScrollBar == ScrollBarAlwaysOn);

    setUpdate();

    if (calcChildExtents())
    {
        childLowerRight = mChildPos + mChildExt;

        if (mHasVScrollBar)
            mContentExt.x -= mScrollBarThickness;
        if (mHasHScrollBar)
            mContentExt.y -= mScrollBarThickness;
        if (mChildExt.x > mContentExt.x && (mForceHScrollBar == ScrollBarDynamic))
        {
            mHasHScrollBar = true;
            mContentExt.y -= mScrollBarThickness;
        }
        if (mChildExt.y > mContentExt.y && (mForceVScrollBar == ScrollBarDynamic))
        {
            mHasVScrollBar = true;
            mContentExt.x -= mScrollBarThickness;
            // doh! ext.x changed, so check hscrollbar again
            if (mChildExt.x > mContentExt.x && !mHasHScrollBar && (mForceHScrollBar == ScrollBarDynamic))
            {
                mHasHScrollBar = true;
                mContentExt.y -= mScrollBarThickness;
            }
        }
        Point2I contentLowerRight = mContentPos + mContentExt;

        // see if the child controls need to be repositioned (null space in control)
        Point2I delta(0, 0);

        if (mChildPos.x > mContentPos.x)
            delta.x = mContentPos.x - mChildPos.x;
        else if (contentLowerRight.x > childLowerRight.x)
        {
            S32 diff = contentLowerRight.x - childLowerRight.x;
            delta.x = getMin(mContentPos.x - mChildPos.x, diff);
        }

        //reposition the children if the child extent > the scroll content extent
        if (mChildPos.y > mContentPos.y)
            delta.y = mContentPos.y - mChildPos.y;
        else if (contentLowerRight.y > childLowerRight.y)
        {
            S32 diff = contentLowerRight.y - childLowerRight.y;
            delta.y = getMin(mContentPos.y - mChildPos.y, diff);
        }

        // apply the deltas to the children...
        if (delta.x || delta.y)
        {
            SimGroup::iterator i;
            for (i = begin(); i != end(); i++)
            {
                GuiControl* ctrl = (GuiControl*)(*i);
                ctrl->mBounds.point += delta;
            }
            mChildPos += delta;
            childLowerRight += delta;
        }
        // enable needed scroll bars
        if (mChildExt.x > mContentExt.x)
            mHBarEnabled = true;
        if (mChildExt.y > mContentExt.y)
            mVBarEnabled = true;
        mChildRelPos = mContentPos - mChildPos;
    }
    // build all the rectangles and such...
    calcScrollRects();
    calcThumbs();
}

void GuiScrollCtrl::calcScrollRects(void)
{
    S32 thickness = (mProfile ? mProfile->mBorderThickness : 1);
    if (mHasHScrollBar)
    {
        mLeftArrowRect.set(thickness,
            mBounds.extent.y - thickness - mScrollBarThickness - 1,
            mScrollBarArrowBtnLength,
            mScrollBarThickness);

        mRightArrowRect.set(mBounds.extent.x - thickness - (mHasVScrollBar ? mScrollBarThickness : 0) - mScrollBarArrowBtnLength,
            mBounds.extent.y - thickness - mScrollBarThickness - 1,
            mScrollBarArrowBtnLength,
            mScrollBarThickness);
        mHTrackRect.set(mLeftArrowRect.point.x + mLeftArrowRect.extent.x,
            mLeftArrowRect.point.y,
            mRightArrowRect.point.x - (mLeftArrowRect.point.x + mLeftArrowRect.extent.x),
            mScrollBarThickness);
    }
    if (mHasVScrollBar)
    {
        mUpArrowRect.set(mBounds.extent.x - thickness - mScrollBarThickness,
            thickness,
            mScrollBarThickness,
            mScrollBarArrowBtnLength);
        mDownArrowRect.set(mBounds.extent.x - thickness - mScrollBarThickness,
            mBounds.extent.y - thickness - mScrollBarArrowBtnLength - (mHasHScrollBar ? (mScrollBarThickness + 1) : 0),
            mScrollBarThickness,
            mScrollBarArrowBtnLength);
        mVTrackRect.set(mUpArrowRect.point.x,
            mUpArrowRect.point.y + mUpArrowRect.extent.y,
            mScrollBarThickness,
            mDownArrowRect.point.y - (mUpArrowRect.point.y + mUpArrowRect.extent.y));
    }
}

void GuiScrollCtrl::calcThumbs()
{
    if (mHBarEnabled)
    {
        U32 trackSize = mHTrackRect.len_x();

        if (mUseConstantHeightThumb)
            mHThumbSize = mBaseThumbSize;
        else
            mHThumbSize = getMax(mBaseThumbSize, S32((mContentExt.x * trackSize) / mChildExt.x));

        mHThumbPos = mHTrackRect.point.x + (mChildRelPos.x * (trackSize - mHThumbSize)) / (mChildExt.x - mContentExt.x);
    }
    if (mVBarEnabled)
    {
        U32 trackSize = mVTrackRect.len_y();

        if (mUseConstantHeightThumb)
            mVThumbSize = mBaseThumbSize;
        else
            mVThumbSize = getMax(mBaseThumbSize, S32((mContentExt.y * trackSize) / mChildExt.y));

        mVThumbPos = mVTrackRect.point.y + (mChildRelPos.y * (trackSize - mVThumbSize)) / (mChildExt.y - mContentExt.y);
    }
}


void GuiScrollCtrl::scrollDelta(S32 deltaX, S32 deltaY)
{
    scrollTo(mChildRelPos.x + deltaX, mChildRelPos.y + deltaY);
}

void GuiScrollCtrl::scrollTo(S32 x, S32 y)
{
    if (!size())
        return;

    setUpdate();
    if (x > mChildExt.x - mContentExt.x)
        x = mChildExt.x - mContentExt.x;
    if (x < 0)
        x = 0;

    if (y > mChildExt.y - mContentExt.y)
        y = mChildExt.y - mContentExt.y;
    if (y < 0)
        y = 0;

    Point2I delta(x - mChildRelPos.x, y - mChildRelPos.y);
    mChildRelPos += delta;
    mChildPos -= delta;

    for (SimSet::iterator i = begin(); i != end(); i++)
    {
        GuiControl* ctrl = (GuiControl*)(*i);
        ctrl->mBounds.point -= delta;
    }
    calcThumbs();
}

GuiScrollCtrl::Region GuiScrollCtrl::findHitRegion(const Point2I& pt)
{
    if (mVBarEnabled && mHasVScrollBar)
    {
        if (mUpArrowRect.pointInRect(pt))
            return UpArrow;
        else if (mDownArrowRect.pointInRect(pt))
            return DownArrow;
        else if (mVTrackRect.pointInRect(pt))
        {
            if (pt.y < mVThumbPos)
                return UpPage;
            else if (pt.y < mVThumbPos + mVThumbSize)
                return VertThumb;
            else
                return DownPage;
        }
    }
    if (mHBarEnabled && mHasHScrollBar)
    {
        if (mLeftArrowRect.pointInRect(pt))
            return LeftArrow;
        else if (mRightArrowRect.pointInRect(pt))
            return RightArrow;
        else if (mHTrackRect.pointInRect(pt))
        {
            if (pt.x < mHThumbPos)
                return LeftPage;
            else if (pt.x < mHThumbPos + mHThumbSize)
                return HorizThumb;
            else
                return RightPage;
        }
    }
    return None;
}

bool GuiScrollCtrl::wantsTabListMembership()
{
    return true;
}

bool GuiScrollCtrl::loseFirstResponder()
{
    setUpdate();
    return true;
}

bool GuiScrollCtrl::becomeFirstResponder()
{
    setUpdate();
    return mWillFirstRespond;
}

bool GuiScrollCtrl::onKeyDown(const GuiEvent& event)
{
    if (mWillFirstRespond)
    {
        switch (event.keyCode)
        {
        case KEY_RIGHT:
            scrollByRegion(RightArrow);
            return true;

        case KEY_LEFT:
            scrollByRegion(LeftArrow);
            return true;

        case KEY_DOWN:
            scrollByRegion(DownArrow);
            return true;

        case KEY_UP:
            scrollByRegion(UpArrow);
            return true;

        case KEY_PAGE_UP:
            scrollByRegion(UpPage);
            return true;

        case KEY_PAGE_DOWN:
            scrollByRegion(DownPage);
            return true;
        }
    }
    return Parent::onKeyDown(event);
}

void GuiScrollCtrl::onMouseDown(const GuiEvent& event)
{
    mouseLock();
    setFirstResponder();

    setUpdate();

    Point2I curMousePos = globalToLocalCoord(event.mousePoint);
    curHitRegion = findHitRegion(curMousePos);
    stateDepressed = true;

    // Set a 0.5 second delay before we start scrolling
    mLastUpdated = Platform::getVirtualMilliseconds() + 500;

    scrollByRegion(curHitRegion);

    if (curHitRegion == VertThumb)
    {
        mChildRelPosAnchor = mChildRelPos;
        mThumbMouseDelta = curMousePos.y - mVThumbPos;
    }
    else if (curHitRegion == HorizThumb)
    {
        mChildRelPosAnchor = mChildRelPos;
        mThumbMouseDelta = curMousePos.x - mHThumbPos;
    }
}

void GuiScrollCtrl::onMouseUp(const GuiEvent&)
{
    mouseUnlock();

    setUpdate();

    curHitRegion = None;
    stateDepressed = false;
}

void GuiScrollCtrl::onMouseDragged(const GuiEvent& event)
{
    Point2I curMousePos = globalToLocalCoord(event.mousePoint);
    setUpdate();

    if ((curHitRegion != VertThumb) && (curHitRegion != HorizThumb))
    {
        Region hit = findHitRegion(curMousePos);
        if (hit != curHitRegion)
            stateDepressed = false;
        else
            stateDepressed = true;
        return;
    }

    // ok... if the mouse is 'near' the scroll bar, scroll with it
    // otherwise, snap back to the previous position.

    if (curHitRegion == VertThumb)
    {
        if (curMousePos.x >= mVTrackRect.point.x - mScrollBarThickness &&
            curMousePos.x <= mVTrackRect.point.x + mVTrackRect.extent.x - 1 + mScrollBarThickness &&
            curMousePos.y >= mVTrackRect.point.y - mScrollBarThickness &&
            curMousePos.y <= mVTrackRect.point.y + mVTrackRect.extent.y - 1 + mScrollBarThickness)
        {
            S32 newVThumbPos = curMousePos.y - mThumbMouseDelta;
            if (newVThumbPos != mVThumbPos)
            {
                S32 newVPos = (newVThumbPos - mVTrackRect.point.y) *
                    (mChildExt.y - mContentExt.y) /
                    (mVTrackRect.extent.y - mVThumbSize);

                scrollTo(mChildRelPosAnchor.x, newVPos);
            }
        }
        else
            scrollTo(mChildRelPosAnchor.x, mChildRelPosAnchor.y);
    }
    else if (curHitRegion == HorizThumb)
    {
        if (curMousePos.x >= mHTrackRect.point.x - mScrollBarThickness &&
            curMousePos.x <= mHTrackRect.point.x + mHTrackRect.extent.x - 1 + mScrollBarThickness &&
            curMousePos.y >= mHTrackRect.point.y - mScrollBarThickness &&
            curMousePos.y <= mHTrackRect.point.y + mHTrackRect.extent.y - 1 + mScrollBarThickness)
        {
            S32 newHThumbPos = curMousePos.x - mThumbMouseDelta;
            if (newHThumbPos != mHThumbPos)
            {
                S32 newHPos = (newHThumbPos - mHTrackRect.point.x) *
                    (mChildExt.x - mContentExt.x) /
                    (mHTrackRect.extent.x - mHThumbSize);

                scrollTo(newHPos, mChildRelPosAnchor.y);
            }
        }
        else
            scrollTo(mChildRelPosAnchor.x, mChildRelPosAnchor.y);
    }
}

bool GuiScrollCtrl::onMouseWheelUp(const GuiEvent& event)
{
    if (!mAwake || !mVisible)
        return(false);

    scrollByRegion((event.modifier & SI_CTRL) ? UpPage : UpArrow);

    // Tell the kids that the mouse moved (relatively):
    iterator itr;
    for (itr = begin(); itr != end(); itr++)
    {
        GuiControl* grandKid = static_cast<GuiControl*>(*itr);
        grandKid->onMouseMove(event);
    }

    return(true);
}

bool GuiScrollCtrl::onMouseWheelDown(const GuiEvent& event)
{
    if (!mAwake || !mVisible)
        return(false);

    scrollByRegion((event.modifier & SI_CTRL) ? DownPage : DownArrow);

    // Tell the kids that the mouse moved (relatively):
    iterator itr;
    for (itr = begin(); itr != end(); itr++)
    {
        GuiControl* grandKid = static_cast<GuiControl*>(*itr);
        grandKid->onMouseMove(event);
    }

    return(true);
}

void GuiScrollCtrl::onPreRender()
{
    Parent::onPreRender();

    // Short circuit if not depressed to save cycles
    if (stateDepressed != true)
        return;

    //default to one second, though it shouldn't be necessary
    U32 timeThreshold = 1000;

    // We don't want to scroll by pages at an interval the same as when we're scrolling
    // using the arrow buttons, so adjust accordingly.
    switch (curHitRegion)
    {
    case UpPage:
    case DownPage:
    case LeftPage:
    case RightPage:
        timeThreshold = 200;
        break;
    case UpArrow:
    case DownArrow:
    case LeftArrow:
    case RightArrow:
        timeThreshold = 20;
        break;
    default:
        // Neither a button or a page, don't scroll (shouldn't get here)
        return;
        break;
    };

    S32 timeElapsed = Platform::getVirtualMilliseconds() - mLastUpdated;

    if ((timeElapsed > 0) && (timeElapsed > timeThreshold))
    {

        mLastUpdated = Platform::getVirtualMilliseconds();
        scrollByRegion(curHitRegion);
    }

}

void GuiScrollCtrl::scrollByRegion(Region reg)
{
    setUpdate();
    if (!size())
        return;
    GuiControl* content = (GuiControl*)front();
    U32 rowHeight, columnWidth;
    U32 pageHeight, pageWidth;

    content->getScrollLineSizes(&rowHeight, &columnWidth);

    if (rowHeight >= mContentExt.y)
        pageHeight = 1;
    else
        pageHeight = mContentExt.y - rowHeight;

    if (columnWidth >= mContentExt.x)
        pageWidth = 1;
    else
        pageWidth = mContentExt.x - columnWidth;

    if (mVBarEnabled)
    {
        switch (reg)
        {
        case UpPage:
            scrollDelta(0, -pageHeight);
            break;
        case DownPage:
            scrollDelta(0, pageHeight);
            break;
        case UpArrow:
            scrollDelta(0, -rowHeight);
            break;
        case DownArrow:
            scrollDelta(0, rowHeight);
            break;
        }
    }

    if (mHBarEnabled)
    {
        switch (reg)
        {
        case LeftPage:
            scrollDelta(-pageWidth, 0);
            break;
        case RightPage:
            scrollDelta(pageWidth, 0);
            break;
        case LeftArrow:
            scrollDelta(-columnWidth, 0);
            break;
        case RightArrow:
            scrollDelta(columnWidth, 0);
            break;
        }
    }
}


void GuiScrollCtrl::onRender(Point2I offset, const RectI& updateRect)
{
    RectI r(offset.x, offset.y, mBounds.extent.x, mBounds.extent.y);
    GFX->setTextureStageMagFilter(0, GFXTextureFilterPoint);
    if (mProfile->mOpaque)
        GFX->drawRectFill(r, mProfile->mFillColor);

    if (mProfile->mBorder)
        renderBorder(r, mProfile);

    // draw scroll bars
    if (mHasVScrollBar)
        drawVScrollBar(offset);

    if (mHasHScrollBar)
        drawHScrollBar(offset);

    //draw the scroll corner
    if (mHasVScrollBar && mHasHScrollBar)
        drawScrollCorner(offset);

    // draw content controls
    // create a rect to intersect with the updateRect
    RectI contentRect(mContentPos.x + offset.x, mContentPos.y + offset.y, mContentExt.x, mContentExt.y);
    if (contentRect.intersect(updateRect))
        renderChildControls(offset, contentRect);
}

void GuiScrollCtrl::drawBorder(const Point2I& offset, bool /*isFirstResponder*/)
{
}

void GuiScrollCtrl::drawVScrollBar(const Point2I& offset)
{
    Point2I pos = offset + mUpArrowRect.point;
    S32 bitmap = (mVBarEnabled ? ((curHitRegion == UpArrow && stateDepressed) ?
        BmpStates * BmpUp + BmpHilite : BmpStates * BmpUp) : BmpStates * BmpUp + BmpDisabled);

    GFX->clearBitmapModulation();
    GFX->drawBitmapSR(mTextureObject, pos, mBitmapBounds[bitmap]);

    pos.y += mScrollBarArrowBtnLength;
    S32 end;
    if (mVBarEnabled)
        end = mVThumbPos + offset.y;
    else
        end = mDownArrowRect.point.y + offset.y;

    bitmap = (mVBarEnabled ? ((curHitRegion == DownPage && stateDepressed) ?
        BmpStates * BmpVPage + BmpHilite : BmpStates * BmpVPage) : BmpStates * BmpVPage + BmpDisabled);

    if (end > pos.y)
    {
        GFX->clearBitmapModulation();
        GFX->drawBitmapStretchSR(mTextureObject, RectI(pos, Point2I(mBitmapBounds[bitmap].extent.x, end - pos.y)), mBitmapBounds[bitmap]);
    }

    pos.y = end;
    if (mVBarEnabled)
    {
        bool thumbSelected = (curHitRegion == VertThumb && stateDepressed);
        S32 ttop = (thumbSelected ? BmpStates * BmpVThumbTopCap + BmpHilite : BmpStates * BmpVThumbTopCap);
        S32 tmid = (thumbSelected ? BmpStates * BmpVThumb + BmpHilite : BmpStates * BmpVThumb);
        S32 tbot = (thumbSelected ? BmpStates * BmpVThumbBottomCap + BmpHilite : BmpStates * BmpVThumbBottomCap);

        // draw the thumb
        GFX->clearBitmapModulation();
        GFX->drawBitmapSR(mTextureObject, pos, mBitmapBounds[ttop]);
        pos.y += mBitmapBounds[ttop].extent.y;
        end = mVThumbPos + mVThumbSize - mBitmapBounds[tbot].extent.y + offset.y;

        if (end > pos.y)
        {
            GFX->clearBitmapModulation();
            GFX->drawBitmapStretchSR(mTextureObject, RectI(pos, Point2I(mBitmapBounds[tmid].extent.x, end - pos.y)), mBitmapBounds[tmid]);
        }

        pos.y = end;
        GFX->clearBitmapModulation();
        GFX->drawBitmapSR(mTextureObject, pos, mBitmapBounds[tbot]);
        pos.y += mBitmapBounds[tbot].extent.y;
        end = mVTrackRect.point.y + mVTrackRect.extent.y - 1 + offset.y;

        bitmap = (curHitRegion == DownPage && stateDepressed) ? BmpStates * BmpVPage + BmpHilite : BmpStates * BmpVPage;
        if (end > pos.y)
        {
            GFX->clearBitmapModulation();
            GFX->drawBitmapStretchSR(mTextureObject, RectI(pos, Point2I(mBitmapBounds[bitmap].extent.x, end - pos.y)), mBitmapBounds[bitmap]);
        }

        pos.y = end;
    }

    bitmap = (mVBarEnabled ? ((curHitRegion == DownArrow && stateDepressed) ?
        BmpStates * BmpDown + BmpHilite : BmpStates * BmpDown) : BmpStates * BmpDown + BmpDisabled);

    GFX->clearBitmapModulation();
    GFX->drawBitmapSR(mTextureObject, pos, mBitmapBounds[bitmap]);
}

void GuiScrollCtrl::drawHScrollBar(const Point2I& offset)
{
    S32 bitmap;

    //draw the left arrow
    bitmap = (mHBarEnabled ? ((curHitRegion == LeftArrow && stateDepressed) ?
        BmpStates * BmpLeft + BmpHilite : BmpStates * BmpLeft) : BmpStates * BmpLeft + BmpDisabled);

    Point2I pos = offset;
    pos += mLeftArrowRect.point;

    GFX->clearBitmapModulation();
    GFX->drawBitmapSR(mTextureObject, pos, mBitmapBounds[bitmap]);

    pos.x += mLeftArrowRect.extent.x;

    //draw the left page
    S32 end;
    if (mHBarEnabled)
        end = mHThumbPos + offset.x;
    else
        end = mRightArrowRect.point.x + offset.x;

    bitmap = (mHBarEnabled ? ((curHitRegion == LeftPage && stateDepressed) ?
        BmpStates * BmpHPage + BmpHilite : BmpStates * BmpHPage) : BmpStates * BmpHPage + BmpDisabled);

    if (end > pos.x)
    {
        GFX->clearBitmapModulation();
        GFX->drawBitmapStretchSR(mTextureObject, RectI(pos, Point2I(end - pos.x, mBitmapBounds[bitmap].extent.y)), mBitmapBounds[bitmap]);
    }
    pos.x = end;

    //draw the thumb and the rightPage
    if (mHBarEnabled)
    {
        bool thumbSelected = (curHitRegion == HorizThumb && stateDepressed);
        S32 ttop = (thumbSelected ? BmpStates * BmpHThumbLeftCap + BmpHilite : BmpStates * BmpHThumbLeftCap);
        S32 tmid = (thumbSelected ? BmpStates * BmpHThumb + BmpHilite : BmpStates * BmpHThumb);
        S32 tbot = (thumbSelected ? BmpStates * BmpHThumbRightCap + BmpHilite : BmpStates * BmpHThumbRightCap);

        // draw the thumb
        GFX->clearBitmapModulation();
        GFX->drawBitmapSR(mTextureObject, pos, mBitmapBounds[ttop]);
        pos.x += mBitmapBounds[ttop].extent.x;
        end = mHThumbPos + mHThumbSize - mBitmapBounds[tbot].extent.x + offset.x;
        if (end > pos.x)
        {
            GFX->clearBitmapModulation();
            GFX->drawBitmapStretchSR(mTextureObject, RectI(pos, Point2I(end - pos.x, mBitmapBounds[tmid].extent.y)), mBitmapBounds[tmid]);
        }

        pos.x = end;
        GFX->clearBitmapModulation();
        GFX->drawBitmapSR(mTextureObject, pos, mBitmapBounds[tbot]);
        pos.x += mBitmapBounds[tbot].extent.x;
        end = mHTrackRect.point.x + mHTrackRect.extent.x - 1 + offset.x;

        bitmap = ((curHitRegion == RightPage && stateDepressed) ? BmpStates * BmpHPage + BmpHilite : BmpStates * BmpHPage);

        if (end > pos.x)
        {
            GFX->clearBitmapModulation();
            GFX->drawBitmapStretchSR(mTextureObject, RectI(pos, Point2I(end - pos.x, mBitmapBounds[bitmap].extent.y)), mBitmapBounds[bitmap]);
        }

        pos.x = end;
    }
    bitmap = (mHBarEnabled ? ((curHitRegion == RightArrow && stateDepressed) ?
        BmpStates * BmpRight + BmpHilite : BmpStates * BmpRight) : BmpStates * BmpRight + BmpDisabled);

    GFX->clearBitmapModulation();
    GFX->drawBitmapSR(mTextureObject, pos, mBitmapBounds[bitmap]);
}

void GuiScrollCtrl::drawScrollCorner(const Point2I& offset)
{
    Point2I pos = offset;
    pos.x += mRightArrowRect.point.x + mRightArrowRect.extent.x;
    pos.y += mRightArrowRect.point.y;
    GFX->clearBitmapModulation();
    GFX->drawBitmapSR(mTextureObject, pos, mBitmapBounds[BmpStates * BmpResize]);
}

void GuiScrollCtrl::autoScroll(Region reg)
{
    scrollByRegion(reg);
}
