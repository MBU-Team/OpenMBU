//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "console/console.h"
#include "console/consoleTypes.h"

#include "gui/controls/guiXboxOptionListCtrl.h"

//-----------------------------------------------------------------------------
IMPLEMENT_CONOBJECT(GuiXboxOptionListCtrl);

GuiXboxOptionListCtrl::GuiXboxOptionListCtrl()
{
    mBorder = false;
    mColumnWidth.push_back(50);
    mColumnWidth.push_back(50);
    mShowRects = false;
}

void GuiXboxOptionListCtrl::initPersistFields()
{
    Parent::initPersistFields();

    addField("border", TypeBool, Offset(mBorder, GuiXboxOptionListCtrl));
    addField("columns", TypeS32Vector, Offset(mColumnWidth, GuiXboxOptionListCtrl));
    addField("columnMargins", TypeS32Vector, Offset(mColumnMargins, GuiXboxOptionListCtrl));
    addField("showRects", TypeBool, Offset(mShowRects, GuiXboxOptionListCtrl));
}

//-----------------------------------------------------------------------------

bool GuiXboxOptionListCtrl::onWake()
{
    return Parent::onWake();
}

void GuiXboxOptionListCtrl::onSleep()
{
    Parent::onSleep();
}

//-----------------------------------------------------------------------------

ConsoleMethod(GuiXboxOptionListCtrl, getTopRow, S32, 2, 2, "()")
{
    return object->getTopRow();
}

ConsoleMethod(GuiXboxOptionListCtrl, getBottomRow, S32, 2, 2, "()")
{
    return object->getBottomRow();
}

ConsoleMethod(GuiXboxOptionListCtrl, getSelectedIndex, S32, 2, 2, "()")
{
    return object->getSelectedIndex();
}

ConsoleMethod(GuiXboxOptionListCtrl, getOptionIndex, S32, 3, 3, "(row)")
{
    return object->getOptionIndex(dAtoi(argv[2]));
}

ConsoleMethod(GuiXboxOptionListCtrl, setOptionIndex, void, 4, 4, "(row, idx)")
{
    object->setOptionIndex(dAtoi(argv[2]), dAtoi(argv[3]));
}

ConsoleMethod(GuiXboxOptionListCtrl, setOptionWrap, void, 4, 4, "(row, wrap)")
{
    object->setOptionWrap(dAtoi(argv[2]), dAtob(argv[3]));
}

ConsoleMethod(GuiXboxOptionListCtrl, getRowCount, S32, 2, 2, "()")
{
    return object->getRowCount();
}

ConsoleMethod(GuiXboxOptionListCtrl, getSelectedText, const char*, 2, 2, "()")
{
    return object->getSelectedText();
}

ConsoleMethod(GuiXboxOptionListCtrl, getSelectedData, const char*, 2, 2, "()")
{
    return object->getSelectedData();
}

ConsoleMethod(GuiXboxOptionListCtrl, addRow, void, 3, 5, "(text, [data], [bitmapIndex])")
{
    argc;

    const char* data;
    if (argc <= 3)
        data = "";
    else
        data = argv[3];

    S32 bitmapIndex;
    if (argc <= 4)
        bitmapIndex = -1;
    else
        bitmapIndex = dAtoi(argv[4]);

    object->addRow(argv[2], data, bitmapIndex);
}

ConsoleMethod(GuiXboxOptionListCtrl, clear, void, 2, 2, "()")
{
    return object->clear();
}

//-----------------------------------------------------------------------------

S32 GuiXboxOptionListCtrl::getTopRow()
{
    return mTopRow;
}

S32 GuiXboxOptionListCtrl::getBottomRow()
{
    return mTopRow + mRowsPerPage - 1;
}

void GuiXboxOptionListCtrl::onMouseDragged(const GuiEvent& event)
{
    onMouseDown(event);
}

bool GuiXboxOptionListCtrl::onGamepadButtonPressed(U32 button)
{
    if (mVisible)
    {
        switch(button)
        {
            case XI_DPAD_UP:
                moveUp();
            return true;
            case XI_DPAD_DOWN:
                moveDown();
            return true;
            case XI_DPAD_LEFT:
                decOption();
            return true;
            case XI_DPAD_RIGHT:
                incOption();
            return true;
        }
    }
    
    return Parent::onGamepadButtonPressed(button);
}

U32 GuiXboxOptionListCtrl::getRowCount()
{
    return mRowText.size();
}

S32 GuiXboxOptionListCtrl::getRowIndex(Point2I localPoint)
{
    // TODO: Implement GuiXboxOptionListCtrl::getRowIndex
    return 0;
}

void GuiXboxOptionListCtrl::onRender(Point2I offset, const RectI& updateRect)
{
    // TODO: Implement GuiXboxOptionListCtrl::onRender
    Parent::onRender(offset, updateRect);
}

const char* GuiXboxOptionListCtrl::getSelectedText()
{
    return mRowText[mSelected];
}

const char* GuiXboxOptionListCtrl::getSelectedData()
{
    return mRowData[mSelected];
}

void GuiXboxOptionListCtrl::onMouseDown(const GuiEvent& event)
{
    Point2I localPoint = globalToLocalCoord(event.mousePoint);
    S32 row = getRowIndex(localPoint);

    if (row >= 0 && row != mSelected)
        move(row - mSelected);
}

void GuiXboxOptionListCtrl::onMouseUp(const GuiEvent& event)
{
    Point2I localPoint = globalToLocalCoord(event.mousePoint);

    if (mSelected == getRowIndex(localPoint))
        clickOption(localPoint.x);
}

void GuiXboxOptionListCtrl::clickOption(S32 xPos)
{
    // TODO: Implement GuiXboxOptionListCtrl::clickOption
}

void GuiXboxOptionListCtrl::addRow(const char* text, const char* data, S32 bitmapIndex)
{
    mRowText.push_back(StringTable->insert(text, true));
    mRowData.push_back(StringTable->insert(data, true));
    mRowDataWrap.push_back(true);
    mRowOptionIndex.push_back(0);
    mRowIconIndex.push_back(bitmapIndex);
    if (mRowText.size() == 1)
        mSelected = 0;
}

void GuiXboxOptionListCtrl::clear()
{
    Parent::clear();

    mRowDataWrap.clear();
    mRowIconIndex.clear();
}

void GuiXboxOptionListCtrl::incOption()
{
    if (mProfile->mSoundOptionChanged)
        alxPlay(mProfile->mSoundOptionChanged);

    if (mRowDataWrap[mSelected] || mRowOptionIndex[mSelected] + 1 < getOptionCount(mSelected))
    {
        mRowOptionIndex[mSelected]++;

        if (mRowOptionIndex[mSelected] >= getOptionCount(mSelected))
            mRowOptionIndex[mSelected] = 0;

        Con::executef(this, 2, "onOptionChange", "1");
    }
}

void GuiXboxOptionListCtrl::decOption()
{
    if (mProfile->mSoundOptionChanged)
        alxPlay(mProfile->mSoundOptionChanged);

    if (mRowDataWrap[mSelected] || mRowOptionIndex[mSelected] - 1 >= 0)
    {
        mRowOptionIndex[mSelected]--;

        if (mRowOptionIndex[mSelected] < 0)
            mRowOptionIndex[mSelected] = getOptionCount(mSelected) - 1;

        Con::executef(this, 2, "onOptionChange", "0");
    }
}

U32 GuiXboxOptionListCtrl::getOptionCount(S32 row)
{
    const char* rowData = mRowData[row];

    U32 count = 0;
    if (*rowData)
    {
        char currentChar = *rowData;
        char lastChar;
        do
        {
            lastChar = currentChar;
            rowData++;
            if (currentChar == '\t')
            {
                count++;
                lastChar = 0;
            }
            currentChar = *rowData;
        } while (*rowData);
        if (lastChar)
            count++;
    }
    return count;
}

void GuiXboxOptionListCtrl::setOptionIndex(S32 row, S32 idx)
{
    if (row >= 0 && row < mRowOptionIndex.size() && idx >= 0)
    {
        // That's odd... Why do they call this through script? It's an engine function...
        // If the purpose to was to allow overriding, why would you ever override getFieldCount anyway?
        // This is silly...
        const char* res = Con::executef(2, "getFieldCount", mRowData[row]);
        if (idx < dAtoi(res))
            mRowOptionIndex[row] = idx;
    }
}

void GuiXboxOptionListCtrl::setOptionWrap(S32 row, bool wrap)
{
    if (row >= 0 && row < mRowDataWrap.size())
        mRowDataWrap[row] = wrap;
}

S32 GuiXboxOptionListCtrl::getOptionIndex(S32 row)
{
    if (row < 0 || row >= mRowOptionIndex.size())
        return -1;

    return mRowOptionIndex[row];
}

const char* GuiXboxOptionListCtrl::getOptionText(S32 row, U32 index)
{
    const char* rowData = mRowData[row];

    while(index)
    {
        index--;
        if (!*rowData)
            return "";

        const char* tab = &rowData[dStrcspn(rowData, "\t")];
        if (!*tab)
            return "";
        rowData = tab + 1;
    }

    const U32 len = dStrcspn(rowData, "\t");
    if (len)
    {
        char* buffer = Con::getReturnBuffer(len + 1);
        dStrncpy(buffer, rowData, len);
        buffer[len] = 0;
        return buffer;
    }
    
    return "";
}

const char* GuiXboxOptionListCtrl::getRowData(S32 idx)
{
    if (idx < 0 || idx >= mRowData.size())
        return "";

    return mRowData[idx];
}

S32 GuiXboxOptionListCtrl::getSelectedIndex()
{
    return mSelected;
}
