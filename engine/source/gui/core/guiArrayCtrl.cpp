//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "console/console.h"
#include "console/consoleTypes.h"
#include "platform/event.h"
#include "gui/containers/guiScrollCtrl.h"
#include "gui/core/guiArrayCtrl.h"

IMPLEMENT_CONOBJECT(GuiArrayCtrl);

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- //

GuiArrayCtrl::GuiArrayCtrl()
{
    mActive = true;

    mCellSize.set(80, 30);
    mSize = Point2I(5, 30);
    mSelectedCell.set(-1, -1);
    mMouseOverCell.set(-1, -1);
    mHeaderDim.set(0, 0);

    mTopRow = 0;
    mRowsPerPage = -1;
    mAllowUnselectedScroll = 1;
}

void GuiArrayCtrl::initPersistFields()
{
    Parent::initPersistFields();
    addField("rowsPerPage", TypeS32, Offset(mRowsPerPage, GuiArrayCtrl));
    addField("allowUnselectedScroll", TypeBool, Offset(mAllowUnselectedScroll, GuiArrayCtrl));
}

bool GuiArrayCtrl::onWake()
{
    if (!Parent::onWake())
        return false;

    //get the font
    mFont = mProfile->mFonts[0].mFont;

    return true;
}

void GuiArrayCtrl::onSleep()
{
    Parent::onSleep();
    mFont = NULL;
}

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- //

void GuiArrayCtrl::setSize(Point2I newSize)
{
    mSize = newSize;
    Point2I newExtent(newSize.x * mCellSize.x + mHeaderDim.x, newSize.y * mCellSize.y + mHeaderDim.y);

    resize(mBounds.point, newExtent);
}

void GuiArrayCtrl::getScrollDimensions(S32& cell_size, S32& num_cells)
{
    cell_size = mCellSize.y;
    num_cells = mSize.y;
}

void GuiArrayCtrl::setTopRow(S32 row)
{
    mTopRow = row;

    if (row >= 0)
    {
        if (row >= mSize.y)
            mTopRow = mSize.y - 1;
    } else {
        mTopRow = 0;
    }

    if (mSelectedCell.y != -1)
        mSelectedCell.y = mTopRow;

}

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- //

bool GuiArrayCtrl::cellSelected(Point2I cell)
{
    if (cell.x < 0 || cell.x >= mSize.x || cell.y < 0 || cell.y >= mSize.y)
    {
        mSelectedCell = Point2I(-1, -1);
        return false;
    }

    mSelectedCell = cell;
    scrollSelectionVisible();
    onCellSelected(cell);
    setUpdate();
    return true;
}

void GuiArrayCtrl::onCellSelected(Point2I cell)
{
    Con::executef(this, 3, "onSelect", Con::getFloatArg(cell.x), Con::getFloatArg(cell.y));

    //call the console function
    if (mConsoleCommand[0])
        Con::evaluate(mConsoleCommand, false);
}

void GuiArrayCtrl::setSelectedCell(Point2I cell)
{
    cellSelected(cell);
}

Point2I GuiArrayCtrl::getSelectedCell()
{
    return mSelectedCell;
}

void GuiArrayCtrl::scrollSelectionVisible()
{
    scrollCellVisible(mSelectedCell);
}

void GuiArrayCtrl::scrollCellVisible(Point2I cell)
{
    //make sure we have a parent
    //make sure we have a valid cell selected
    GuiScrollCtrl* parent = dynamic_cast<GuiScrollCtrl*>(getParent());
    if (!parent || cell.x < 0 || cell.y < 0)
        return;

    RectI cellBounds(cell.x * mCellSize.x, cell.y * mCellSize.y, mCellSize.x, mCellSize.y);
    parent->scrollRectVisible(cellBounds);
}

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- //

void GuiArrayCtrl::onRenderColumnHeaders(Point2I offset, Point2I parentOffset, Point2I headerDim)
{
    if (mProfile->mBorder)
    {
        RectI cellR(offset.x + headerDim.x, parentOffset.y, mBounds.extent.x - headerDim.x, headerDim.y);
        GFX->drawRectFill(cellR, mProfile->mBorderColor);
    }
}

void GuiArrayCtrl::onRenderRowHeader(Point2I offset, Point2I parentOffset, Point2I headerDim, Point2I cell)
{
    ColorI color;
    RectI cellR;
    if (cell.x % 2)
        color.set(255, 0, 0, 255);
    else
        color.set(0, 255, 0, 255);

    cellR.point.set(parentOffset.x, offset.y);
    cellR.extent.set(headerDim.x, mCellSize.y);
    GFX->drawRectFill(cellR, color);
}

void GuiArrayCtrl::onRenderCell(Point2I offset, Point2I cell, bool selected, bool mouseOver)
{
    ColorI color(255 * (cell.x % 2), 255 * (cell.y % 2), 255 * ((cell.x + cell.y) % 2), 255);
    if (selected)
    {
        color.set(255, 0, 0, 255);
    }
    else if (mouseOver)
    {
        color.set(0, 0, 255, 255);
    }

    //draw the cell
    RectI cellR(offset.x, offset.y, mCellSize.x, mCellSize.y);
    GFX->drawRectFill(cellR, color);
}

void GuiArrayCtrl::onRender(Point2I offset, const RectI& updateRect)
{
    //make sure we have a parent
    GuiControl* parent = getParent();
    if (!parent)
        return;

    S32 i, j;
    RectI headerClip;
    RectI newUpdateRect(updateRect.point, Point2I(updateRect.extent.x, updateRect.extent.y + mTopRow * mCellSize.y));
    RectI clipRect(newUpdateRect.point, newUpdateRect.extent);

    Point2I parentOffset = parent->localToGlobalCoord(Point2I(0, 0));

    //if we have column headings
    if (mHeaderDim.y > 0)
    {
        headerClip.point.x = parentOffset.x + mHeaderDim.x;
        headerClip.point.y = parentOffset.y;
        headerClip.extent.x = clipRect.extent.x;// - headerClip.point.x; // This seems to fix some strange problems with some Gui's, bug? -pw
        headerClip.extent.y = mHeaderDim.y;

        if (headerClip.intersect(clipRect))
        {
            GFX->setClipRect(headerClip);

            //now render the header
            onRenderColumnHeaders(offset, parentOffset, mHeaderDim);

            clipRect.point.y = headerClip.point.y + headerClip.extent.y - 1;
        }
        offset.y += mHeaderDim.y;
    }

    //if we have row headings
    if (mHeaderDim.x > 0)
    {
        clipRect.point.x = getMax(newUpdateRect.point.x, parentOffset.x + mHeaderDim.x);
        offset.x += mHeaderDim.x;
    }

    //save the original for clipping the row headers
    RectI origClipRect = clipRect;
    origClipRect.extent.y = newUpdateRect.extent.y;

    S32 rowsRendered = 0;
    for (j = 0; j < mSize.y; j++)
    {
        if (mRowsPerPage != -1 && rowsRendered >= mRowsPerPage)
            break;

        //skip until we get to a visible row
        if ((j + 1) * mCellSize.y + offset.y < newUpdateRect.point.y)
            continue;

        //break once we've reached the last visible row
        if (j * mCellSize.y + offset.y >= newUpdateRect.point.y + newUpdateRect.extent.y)
            break;

        //render the header
        if (mHeaderDim.x > 0)
        {
            headerClip.point.x = parentOffset.x;
            headerClip.extent.x = mHeaderDim.x;
            headerClip.point.y = offset.y + j * mCellSize.y;
            headerClip.extent.y = mCellSize.y;
            if (headerClip.intersect(origClipRect))
            {
                GFX->setClipRect(headerClip);

                //render the row header
                onRenderRowHeader(Point2I(0, offset.y + j * mCellSize.y),
                    Point2I(parentOffset.x, offset.y + j * mCellSize.y),
                    mHeaderDim, Point2I(0, j));
            }
        }

        bool renderedCell = false;

        //render the cells for the row
        for (i = 0; i < mSize.x; i++)
        {
            //skip past columns off the left edge
            if ((i + 1) * mCellSize.x + offset.x < newUpdateRect.point.x)
                continue;

            //break once past the last visible column
            if (i * mCellSize.x + offset.x >= newUpdateRect.point.x + newUpdateRect.extent.x)
                break;

            S32 cellx = offset.x + i * mCellSize.x;
            S32 celly = offset.y + (j - mTopRow) * mCellSize.y;

            RectI cellClip(cellx, celly, mCellSize.x, mCellSize.y);

            //make sure the cell is within the update region
            if (cellClip.intersect(clipRect))
            {
                //set the clip rect
                GFX->setClipRect(cellClip);

                //render the cell
                onRenderCell(Point2I(cellx, celly), Point2I(i, j),
                    i == mSelectedCell.x && j == mSelectedCell.y,
                    i == mMouseOverCell.x && j == mMouseOverCell.y);
                renderedCell = true;
            }
        }

        if (renderedCell)
            rowsRendered++;
    }
}

void GuiArrayCtrl::onMouseDown(const GuiEvent& event)
{
    if (!mActive || !mAwake || !mVisible)
        return;

    //let the guiControl method take care of the rest
    Parent::onMouseDown(event);

    Point2I pt = globalToLocalCoord(event.mousePoint);
    pt.x -= mHeaderDim.x; pt.y -= mHeaderDim.y;
    Point2I cell(
        (pt.x < 0 ? -1 : pt.x / mCellSize.x),
        (pt.y < 0 ? -1 : pt.y / mCellSize.y)
    );

    if (cell.x >= 0 && cell.x < mSize.x && cell.y >= 0 && cell.y < mSize.y)
    {

        //store the previously selected cell
        Point2I prevSelected = mSelectedCell;

        //select the new cell
        cellSelected(Point2I(cell.x, cell.y));

        //if we double clicked on the *same* cell, evaluate the altConsole Command
        if ((event.mouseClickCount > 1) && (prevSelected == mSelectedCell) && mAltConsoleCommand[0])
            Con::evaluate(mAltConsoleCommand, false);
    }
}

void GuiArrayCtrl::onMouseEnter(const GuiEvent& event)
{
    Point2I pt = globalToLocalCoord(event.mousePoint);
    pt.x -= mHeaderDim.x; pt.y -= mHeaderDim.y;

    //get the cell
    Point2I cell((pt.x < 0 ? -1 : pt.x / mCellSize.x), (pt.y < 0 ? -1 : pt.y / mCellSize.y));
    if (cell.x >= 0 && cell.x < mSize.x && cell.y >= 0 && cell.y < mSize.y)
    {
        mMouseOverCell = cell;
        setUpdateRegion(Point2I(cell.x * mCellSize.x + mHeaderDim.x,
            cell.y * mCellSize.y + mHeaderDim.y), mCellSize);
    }
}

void GuiArrayCtrl::onMouseLeave(const GuiEvent& /*event*/)
{
    setUpdateRegion(Point2I(mMouseOverCell.x * mCellSize.x + mHeaderDim.x,
        mMouseOverCell.y * mCellSize.y + mHeaderDim.y), mCellSize);
    mMouseOverCell.set(-1, -1);
}

void GuiArrayCtrl::onMouseDragged(const GuiEvent& event)
{
    // for the array control, the behaviour of onMouseDragged is the same
    // as on mouse moved - basically just recalc the currend mouse over cell
    // and set the update regions if necessary
    GuiArrayCtrl::onMouseMove(event);
}

void GuiArrayCtrl::onMouseMove(const GuiEvent& event)
{
    Point2I pt = globalToLocalCoord(event.mousePoint);
    pt.x -= mHeaderDim.x; pt.y -= mHeaderDim.y;
    Point2I cell((pt.x < 0 ? -1 : pt.x / mCellSize.x), (pt.y < 0 ? -1 : pt.y / mCellSize.y));
    if (cell.x != mMouseOverCell.x || cell.y != mMouseOverCell.y)
    {
        if (mMouseOverCell.x != -1)
        {
            setUpdateRegion(Point2I(mMouseOverCell.x * mCellSize.x + mHeaderDim.x,
                mMouseOverCell.y * mCellSize.y + mHeaderDim.y), mCellSize);
        }

        if (cell.x >= 0 && cell.x < mSize.x && cell.y >= 0 && cell.y < mSize.y)
        {
            setUpdateRegion(Point2I(cell.x * mCellSize.x + mHeaderDim.x,
                cell.y * mCellSize.y + mHeaderDim.y), mCellSize);
            mMouseOverCell = cell;
        }
        else
            mMouseOverCell.set(-1, -1);
    }
}

bool GuiArrayCtrl::onKeyDown(const GuiEvent& event)
{
    //if this control is a dead end, kill the event
    if ((!mVisible) || (!mActive) || (!mAwake)) return true;

    //get the parent
    S32 pageSize = 1;
    GuiControl* parent = getParent();
    if (parent && mCellSize.y > 0)
    {
        pageSize = getMax(1, (parent->mBounds.extent.y / mCellSize.y) - 1);
    }

    Point2I delta(0, 0);
    switch (event.keyCode)
    {
    case KEY_LEFT:
        delta.set(-1, 0);
        break;
    case KEY_RIGHT:
        delta.set(1, 0);
        break;
    case KEY_UP:
        delta.set(0, -1);
        break;
    case KEY_DOWN:
        delta.set(0, 1);
        break;
    case KEY_PAGE_UP:
        delta.set(0, -pageSize);
        break;
    case KEY_PAGE_DOWN:
        delta.set(0, pageSize);
        break;
    case KEY_HOME:
        cellSelected(Point2I(0, 0));
        return(true);
    case KEY_END:
        cellSelected(Point2I(0, mSize.y - 1));
        return(true);
    default:
        return Parent::onKeyDown(event);
    }
    if (mSize.x < 1 || mSize.y < 1)
        return true;

    //select the first cell if no previous cell was selected
    if (mSelectedCell.x == -1 || mSelectedCell.y == -1)
    {
        cellSelected(Point2I(0, 0));
        return true;
    }

    //select the cell
    Point2I cell = mSelectedCell;
    cell.x = getMax(0, getMin(mSize.x - 1, cell.x + delta.x));
    cell.y = getMax(0, getMin(mSize.y - 1, cell.y + delta.y));
    cellSelected(cell);

    return true;
}

void GuiArrayCtrl::onRightMouseDown(const GuiEvent& event)
{
    if (!mActive || !mAwake || !mVisible)
        return;

    Parent::onRightMouseDown(event);

    Point2I pt = globalToLocalCoord(event.mousePoint);
    pt.x -= mHeaderDim.x; pt.y -= mHeaderDim.y;
    Point2I cell((pt.x < 0 ? -1 : pt.x / mCellSize.x), (pt.y < 0 ? -1 : pt.y / mCellSize.y));
    if (cell.x >= 0 && cell.x < mSize.x && cell.y >= 0 && cell.y < mSize.y)
    {
        char buf[32];
        dSprintf(buf, sizeof(buf), "%d %d", event.mousePoint.x, event.mousePoint.y);
        // Pass it to the console:
        Con::executef(this, 4, "onRightMouseDown", Con::getIntArg(cell.x), Con::getIntArg(cell.y), buf);
    }
}

bool GuiArrayCtrl::onGamepadButtonPressed(U32 button)
{
    if (button == XI_DPAD_UP)
        moveUp();
    else if (button == XI_DPAD_DOWN)
        moveDown();
    else
        return Parent::onGamepadButtonPressed(button);

    return true;
}

void GuiArrayCtrl::moveUp()
{
    if (mSelectedCell.y == -1)
    {
        if (mAllowUnselectedScroll && --mTopRow < 0)
            mTopRow = mSize.y - 1;
    } else {
        S32 newY = mSelectedCell.y - 1;
        if (newY < 0)
        {
            const char* res = Con::executef(this, 1, "onWrapToBottom");
            if (!dStrlen(res) || (newY = dAtoi(res), newY == -1))
                newY = mSize.y - 1;
        }
        if (newY > -1)
        {
            setSelectedCell(Point2I(mSelectedCell.x, newY));
            if (newY >= mTopRow)
            {
                if (newY <= mRowsPerPage + mTopRow - 1)
                    return;
                newY = newY - mRowsPerPage + 1;
            }
            mTopRow = newY;
        }
    }
}

void GuiArrayCtrl::moveDown()
{
    if (mSelectedCell.y == -1)
    {
        if (mAllowUnselectedScroll && ++mTopRow >= mSize.y)
            mTopRow = 0;
    } else {
        S32 newY = mSelectedCell.y + 1;
        if (newY >= mSize.y)
        {
            S32 v6;
            const char* res = Con::executef(this, 1, "onWrapToTop");
            if (!dStrlen(res) || (v6 = dAtoi(res), v6 == -1))
                newY = 0;
            else
                newY = v6;
        }
        if (newY > -1)
        {
            setSelectedCell(Point2I(mSelectedCell.x, newY));
            if (newY >= mTopRow)
            {
                if (newY <= mRowsPerPage + mTopRow - 1)
                    return;
                newY = newY - mRowsPerPage + 1;
            }
            mTopRow = newY;
        }
    }
}
