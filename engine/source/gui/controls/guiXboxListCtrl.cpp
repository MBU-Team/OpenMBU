//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "console/console.h"
#include "console/consoleTypes.h"
#include "core/units.h"

#include "gui/controls/guiXboxListCtrl.h"

IMPLEMENT_CONOBJECT(GuiXboxListCtrl);

GuiXboxListCtrl::GuiXboxListCtrl()
{
    mInitialRows = StringTable->insert("");
#ifndef MBO_UNTOUCHED_MENUS
    mMouseDown = false;
#endif
}

void GuiXboxListCtrl::initPersistFields()
{
    Parent::initPersistFields();
    addField("rows", TypeString, Offset(mInitialRows, GuiXboxListCtrl));
}

//--------------------------------------------------------

bool GuiXboxListCtrl::onWake()
{
    if (!Parent::onWake())
        return false;

    S32 currentRow = 0;
    if (mRowText.size() == 0)
    {
        const char* initialRows = mInitialRows;
        if (initialRows && *initialRows)
        {
            S32 numRows = getUnitCount(initialRows, "\n");
            if (numRows > 0)
            {
                do
                {
                    const char* row = getUnit(mInitialRows, currentRow, "\n");
                    char* tab = (char*)dStrchr(row, '\t');
                    const char* initVal = "";
                    if (tab)
                    {
                        initVal = tab + 1;
                        *tab = 0;
                    }
                    addRow(row, initVal, 0, -1);
                    ++currentRow;
                } while (currentRow < numRows);
            }
        }
    }

    return true;
}

void GuiXboxListCtrl::onSleep()
{
    Parent::onSleep();
}

//--------------------------------------------------------

ConsoleMethod(GuiXboxListCtrl, getTopRow, S32, 2, 2, "()")
{
    return object->getTopRow();
}

ConsoleMethod(GuiXboxListCtrl, getBottomRow, S32, 2, 2, "()")
{
    return object->getBottomRow();
}

ConsoleMethod(GuiXboxListCtrl, getSelectedIndex, S32, 2, 2, "()")
{
    return object->getSelectedIndex();
}

ConsoleMethod(GuiXboxListCtrl, setSelectedIndex, void, 3, 3, "(selected)")
{
    argc;

    object->setSelectedIndex(dAtoi(argv[2]));
}

ConsoleMethod(GuiXboxListCtrl, getRowCount, S32, 2, 2, "()")
{
    return object->getRowCount();
}

ConsoleMethod(GuiXboxListCtrl, getSelectedText, const char*, 2, 2, "()")
{
    return object->getSelectedText();
}

ConsoleMethod(GuiXboxListCtrl, getSelectedData, const char*, 2, 2, "()")
{
    return object->getSelectedData();
}

ConsoleMethod(GuiXboxListCtrl, setRowEnabled, void, 4, 4, "(idx, enabled)")
{
    argc;

    object->setRowEnabled(dAtoi(argv[2]), dAtob(argv[3]));
}

ConsoleMethod(GuiXboxListCtrl, getRowEnabled, bool, 3, 3, "(idx)")
{
    argc;

    return object->getRowEnabled(dAtoi(argv[2]));
}

ConsoleMethod(GuiXboxListCtrl, addRow, void, 3, 6, "(text, [data], [addedHeight], [bitmapIndex])")
{
    argc;

    const char* data;
    if (argc <= 3)
        data = "";
    else
        data = argv[3];

    U32 addedHeight;
    if (argc <= 4)
        addedHeight = 0;
    else
        addedHeight = dAtoi(argv[4]);

    S32 bitmapIndex;
    if (argc <= 5)
        bitmapIndex = -1;
    else
        bitmapIndex = dAtoi(argv[5]);

    object->addRow(argv[2], data, addedHeight, bitmapIndex);
}

ConsoleMethod(GuiXboxListCtrl, clear, void, 2, 2, "()")
{
    return object->clear();
}

//--------------------------------------------------------

S32 GuiXboxListCtrl::getTopRow()
{
    return mTopRow;
}

S32 GuiXboxListCtrl::getBottomRow()
{
    return mTopRow + mRowsPerPage - 1;
}

void GuiXboxListCtrl::onMouseDragged(const GuiEvent& event)
{
    onMouseDown(event);
}

bool GuiXboxListCtrl::onGamepadButtonPressed(U32 button)
{
    if (button == XI_DPAD_UP)
        moveUp();
    else if (button == XI_DPAD_DOWN)
        moveDown();
    else
        return Parent::onGamepadButtonPressed(button);

    return true;
}

U32 GuiXboxListCtrl::getRowCount()
{
    return mRowText.size();
}

S32 GuiXboxListCtrl::getRowIndex(Point2I localPoint)
{
    U32 rowHeight = mProfile->mRowHeight;
    if (!rowHeight)
        rowHeight = mRowHeight;
    
    S32 totalHeight = 0;
    S32 topRow = mTopRow;
    S32 totalTopRows = mRowsPerPage + topRow;
    while (topRow < totalTopRows && topRow < mRowText.size())
    {
        totalHeight += rowHeight;
        if (topRow != totalTopRows - 1)
            totalHeight += mRowHeights[topRow];
        ++topRow;
    }
    S32 hitTop = 0;
    S32 hitBottom = rowHeight;
    if (mProfile->mHitArea.size() == 2)
    {
        hitTop = mProfile->mHitArea[0];
        hitBottom = mProfile->mHitArea[1];
    }

    S32 posY = localPoint.y - (mBounds.extent.y - totalHeight) / 2;
    S32 heights = 0;
    for (S32 i = mTopRow; i < totalTopRows && i < mRowText.size(); i++)
    {
        if (heights + hitTop <= posY && posY <= heights + hitBottom)
            return i;
        heights += rowHeight;
        if (i != totalTopRows - 1)
            heights += mRowHeights[i];
    }

    return -1;
}

void GuiXboxListCtrl::onRender(Point2I offset, const RectI& updateRect)
{
    S32 numBitmaps = mProfile->mBitmapArrayRects.size();
    S32 bitmapHeight = 0;
    S32 selectedBitmapIndex = 2 * (numBitmaps > 1) - 1;
    bool hasBitmap = numBitmaps > 0;
    S32 unselectedBitmapIndex = (numBitmaps > 0) - 1;
    
    if (numBitmaps > 0)
        bitmapHeight = mProfile->mBitmapArrayRects[0].extent.y;

    GFont* font;
    if (mProfile->mFonts[0].mFont)
        font = mProfile->mFonts[0].mFont;
    else
        font = NULL;

    Point2I point(0, 0);

    S32 height = mRowHeight;
    S32 rowHeight = height;
    if (hasBitmap)
    {
        if (height <= bitmapHeight)
            rowHeight = bitmapHeight;
    }
    else
        bitmapHeight = height;

    if (mProfile->mRowHeight)
        rowHeight = mProfile->mRowHeight;

    S32 maxWidth = 0;
    U32 iter = mTopRow;
    if (iter < iter + mRowsPerPage)
    {
        U32 topRow;
        do
        {
            if (iter >= mRowText.size())
                break;

            S32 width = font->getStrWidth(mRowText[iter]);
            if (width > maxWidth)
                maxWidth = width;

            S32 curY = rowHeight + point.y;
            topRow = mRowsPerPage + mTopRow;
            point.y += rowHeight;
            if (iter != topRow - 1)
                point.y = mRowHeights[iter] + curY;
            ++iter;

        } while (iter < topRow);
    }

    offset.y += (mBounds.extent.y - point.y) / 2;

    int i = mTopRow;

    S32 y = 0;
    for (U32 curTop = i + mRowsPerPage; i < curTop; curTop = mRowsPerPage + mTopRow)
    {
        if (i < mRowText.size())
        {
            S32 extentX = mBounds.extent.x;

            point.x = offset.x;
            point.y = y + offset.y;
            
            y += rowHeight + mRowHeights[i];

            GFX->clearBitmapModulation();

            Point2I clickOffset(0, 0);

#ifndef MBO_UNTOUCHED_MENUS
            if (mMouseDown && i == mSelected)
            {
                GFX->setBitmapModulation(ColorI(128, 128, 128, 255));
                clickOffset.set(1, 1);
            }
#endif

            if (hasBitmap)
            {
                S32 bIndex;
                if (((bIndex = selectedBitmapIndex) != -1 && i == mSelected) ||
                    ((bIndex = unselectedBitmapIndex) != -1) && i != mSelected)
                {
                    GFX->drawBitmapSR(mProfile->mTextureObject, point + clickOffset, mProfile->mBitmapArrayRects[bIndex]);
                }
            }

            S32 bitmapIndex = mBitmapIndices[i];
            if (bitmapIndex != -1)
            {
                if (i == mSelected)
                    ++bitmapIndex;
                if (bitmapIndex < numBitmaps)
                    GFX->drawBitmapSR(mProfile->mTextureObject, point + clickOffset + mProfile->mIconPosition, mProfile->mBitmapArrayRects[bitmapIndex]);
            }

            GFX->clearBitmapModulation();

            ColorI fontColor;
            if (mRowEnabled[i])
            {
                if (i == mSelected)
                    fontColor = mProfile->mFontColors[1];
                else
                    fontColor = mProfile->mFontColors[0];
            }
            else
            {
                fontColor = mProfile->mFontColors[2];
            }

            fontColor.alpha = 255;

#ifdef MBO_UNTOUCHED_MENUS
            GFX->setBitmapModulation(fontColor);
#else
            if (mMouseDown && i == mSelected)
                GFX->setBitmapModulation(fontColor * 0.5f);
            else
                GFX->setBitmapModulation(fontColor);
#endif

            point += mProfile->mTextOffset;
            renderJustifiedText(point + clickOffset, Point2I(extentX, bitmapHeight), mRowText[i]);
        }
        ++i;
    }

    if (mProfile->mBorder)
        GFX->drawRect(RectI(point, mBounds.extent), mProfile->mBorderColor);

    renderChildControls(offset, updateRect);
}

const char* GuiXboxListCtrl::getSelectedText()
{
    return mRowText[mSelected];
}

const char* GuiXboxListCtrl::getSelectedData()
{
    return mRowData[mSelected];
}

void GuiXboxListCtrl::setRowEnabled(S32 idx, bool enabled)
{
    if (idx >= 0 && idx < mRowText.size())
        mRowEnabled[idx] = enabled;
}

bool GuiXboxListCtrl::getRowEnabled(S32 idx)
{
    if (idx < 0 || idx >= mRowText.size())
        return false;

    return mRowEnabled[idx];
}

#ifndef MBO_UNTOUCHED_MENUS
void GuiXboxListCtrl::onMouseLeave(const GuiEvent& event)
{
    mMouseDown = false;
}
#endif

#ifndef MBO_UNTOUCHED_MENUS
void GuiXboxListCtrl::onMouseMove(const GuiEvent& event)
{
    Point2I localPoint = globalToLocalCoord(event.mousePoint);
    S32 row = getRowIndex(localPoint);
    if (row >= 0 && row != mSelected)
        move(row - mSelected);
}
#endif

void GuiXboxListCtrl::onMouseDown(const GuiEvent& event)
{
    Point2I localPoint = globalToLocalCoord(event.mousePoint);
    S32 row = getRowIndex(localPoint);
#ifdef MBO_UNTOUCHED_MENUS
    if (row >= 0 && row != mSelected)
        move(row - mSelected);
#else
    if (row >= 0)
        mMouseDown = true;
#endif
}

void GuiXboxListCtrl::onMouseUp(const GuiEvent& event)
{
#ifndef MBO_UNTOUCHED_MENUS
    mMouseDown = false;
#endif

    Point2I localPoint = globalToLocalCoord(event.mousePoint);

    if (mSelected == getRowIndex(localPoint))
    {
#ifdef MBO_UNTOUCHED_MENUS
        Con::executef(this, 1, "onClick");
#else
        const char* retval = Con::executef(this, 1, "onClick");
        if (!dStrcmp(retval, ""))
            onGamepadButtonPressed(XI_A);
#endif
    }
}

void GuiXboxListCtrl::addRow(const char* text, const char* data, U32 addedHeight, S32 bitmapIndex)
{
    mRowText.push_back(StringTable->insert(text, true));
    mRowData.push_back(StringTable->insert(data, true));
    mRowHeights.push_back(addedHeight);
    mBitmapIndices.push_back(bitmapIndex);
    mRowEnabled.push_back(true);
    if (mRowText.size() == 1)
        setSelectedIndex(0);
}

void GuiXboxListCtrl::clear()
{
    Parent::clear();

    mRowHeights.clear();
    mBitmapIndices.clear();
    mRowEnabled.clear();
}
