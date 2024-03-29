//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "console/console.h"
#include "console/consoleTypes.h"
#include "sfx/sfxSystem.h"

#include "gui/controls/guiXboxOptionListCtrl.h"

//-----------------------------------------------------------------------------
IMPLEMENT_CONOBJECT(GuiXboxOptionListCtrl);

GuiXboxOptionListCtrl::GuiXboxOptionListCtrl()
{
    mBorder = false;
    mColumnWidth.push_back(50);
    mColumnWidth.push_back(50);
    mShowRects = false;
#ifndef MBO_UNTOUCHED_MENUS
    mMouseDown = false;
    mArrowHover = 0;
#endif
    mButtonsEnabled = true;
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

bool GuiXboxOptionListCtrl::areButtonsEnabled()
{
    return mButtonsEnabled;
}

void GuiXboxOptionListCtrl::setButtonsEnabled(bool enabled)
{
    mButtonsEnabled = enabled;
}

//-----------------------------------------------------------------------------

ConsoleMethod(GuiXboxOptionListCtrl, areButtonsEnabled, bool, 2, 2, "()")
{
    return object->areButtonsEnabled();
}

ConsoleMethod(GuiXboxOptionListCtrl, setButtonsEnabled, void, 3, 3, "(enabled)")
{
    object->setButtonsEnabled(dAtob(argv[2]));
}

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
    // TODO: Deal with floating point issues better
    //S32 idx = dAtoi(argv[3]);
    S32 idx = (S32)dAtof(argv[3]);

    object->setOptionIndex(dAtoi(argv[2]), idx);
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
    S32 yExtent = 0;
    if (!mProfile->mBitmapArrayRects.empty())
        yExtent = mProfile->mBitmapArrayRects[0].extent.y;

    S32 rowHeight = mRowHeight + mProfile->mTextOffset.y;
    if (rowHeight <= yExtent)
        rowHeight = yExtent;

    S32 hitTop = 0;
    S32 hitBottom = rowHeight;
    if (mProfile->mHitArea.size() == 2)
    {
        hitTop = mProfile->mHitArea[0];
        hitBottom = mProfile->mHitArea[1];
    }

    S32 result = mTopRow;
    S32 lastRow = result + mRowsPerPage;

    if (result < lastRow)
    {
        S32 cur = rowHeight * result;
        while (result < mRowText.size())
        {
            S32 num;
            if (mProfile->mRowHeight)
                num = result * mProfile->mRowHeight;
            else
                num = cur;

            if (num + hitTop <= localPoint.y && localPoint.y <= hitBottom + num)
                return result;

            cur += rowHeight;
            result++;
            if (result >= lastRow)
                return -1;
        }
    }

    return -1;
}

void GuiXboxOptionListCtrl::onRender(Point2I offset, const RectI& updateRect)
{
    S32 maxLen = 0;
    for (S32 i = 0; i < mRowText.size(); i++)
    {
        S32 strWidth = mProfile->mFonts[0].mFont->getStrWidth(mRowText[i]);
        if (strWidth > maxLen)
            maxLen = strWidth;
    }

    S32 numRects = mProfile->mBitmapArrayRects.size();

    S32 bitmapSelectWidth = 0;
    S32 bitmapSelectHeight = 0;
    S32 bitmapArrowWidth = 0;
    S32 bitmapArrowHeight = 0;
    S32 bitmapButtonWidth = 0;
    S32 bitmapButtonHeight = 0;
    S32 unselectedBitmapIndex = (numRects > 0) - 1;
    S32 selectedBitmapIndex = 2 * (numRects > 1) - 1;
    S32 unselectedLeftArrowIndex = numRects <= 2 ? -1 : 2;
    S32 selectedLeftArrowIndex = 4 * (numRects > 3) - 1;
    S32 unselectedRightArrowIndex = numRects <= 4 ? -1 : 4;
    S32 selectedRightArrowIndex = numRects <= 5 ? -1 : 5;
#ifndef MBO_UNTOUCHED_MENUS
    S32 buttonIndex = numRects <= 6 ? -1 : 6;
    S32 hoverButtonIndex = numRects <= 7 ? -1 : 7;
#endif
    bool hasSelectBitmap = numRects > 0;

    if (hasSelectBitmap)
    {
        bitmapSelectWidth = mProfile->mBitmapArrayRects[0].extent.x;
        bitmapSelectHeight = mProfile->mBitmapArrayRects[0].extent.y;
    }

    if (numRects > 2)
    {
        bitmapArrowWidth = mProfile->mBitmapArrayRects[2].extent.x;
        bitmapArrowHeight = mProfile->mBitmapArrayRects[2].extent.y;
    }

#ifndef MBO_UNTOUCHED_MENUS
    if (numRects > 5)
    {
        bitmapButtonWidth = mProfile->mBitmapArrayRects[6].extent.x;
        if (numRects > 6)
            bitmapButtonHeight = mProfile->mBitmapArrayRects[7].extent.y;
    }
#endif

    S32 c1LM = 0;
    S32 c1RM = 0;
    S32 c2LM = 0;
    S32 c2RM = 0;

    S32 numMargins = mColumnMargins.size();
    if (numMargins > 0)
        c1LM = mColumnMargins[0];
    if (numMargins > 1)
        c1RM = mColumnMargins[1];
    if (numMargins > 2)
        c2LM = mColumnMargins[2];
    if (numMargins > 3)
        c2RM = mColumnMargins[3];

    RectI leftRect;
    leftRect.point.set(offset.x + c1LM, offset.y);

    S32 c1 = (S32)((F32)mColumnWidth[0] / 100.0f * (F32)mBounds.extent.x);
    leftRect.extent.set(c1 - c1RM - c1LM, mBounds.extent.y);

    RectI rightRect;
    rightRect.point.set(c2LM + offset.x + c1, offset.y);

    S32 c2 = (S32)((F32)mBounds.extent.x * (F32)mColumnWidth[1] / 100.0f);
    rightRect.extent.set(c2 - c2RM - c2LM, mBounds.extent.y);

    if (mShowRects)
    {
        GFX->drawRect(RectI(offset, mBounds.extent), ColorI(255, 0, 255));
        GFX->drawRect(leftRect, ColorI(255, 255, 0));
        GFX->drawRect(rightRect, ColorI(255, 255, 0));
    }

    S32 rowHeight = mRowHeight + mProfile->mTextOffset.y;
    if (rowHeight <= bitmapSelectHeight)
        rowHeight = bitmapSelectHeight;

    S32 rowWidth = c1 + c2;
    if (rowWidth <= bitmapSelectWidth)
        rowWidth = bitmapSelectWidth;

    if (mBorder || mProfile->mBorder)
    {
        GFX->drawRect(leftRect, mProfile->mBorderColor);
        GFX->drawRect(rightRect, mProfile->mBorderColor);
    }

    S32 index = mTopRow;
    if (mTopRow < mTopRow + mRowsPerPage)
    {
        S32 curRow = mTopRow;
        S32 unk2 = rowHeight * curRow;
        for (S32 i = unk2; curRow < mRowText.size(); unk2 = i)
        {
            S32 yPos = mProfile->mRowHeight ? curRow * mProfile->mRowHeight : unk2;

            ColorI fontColor;

            S32 iconIndex = mRowIconIndex[curRow];

#ifndef MBO_UNTOUCHED_MENUS
            if (iconIndex != -1)
                iconIndex += 2;
#endif
            if (curRow == mSelected)
            {
                if (iconIndex != -1)
                    iconIndex++;
                if (selectedBitmapIndex != -1)
                {
                    GFX->clearBitmapModulation();
                    
                    RectI srcRect = mProfile->mBitmapArrayRects[selectedBitmapIndex];

                    RectI dstRect;
                    dstRect.point.set(offset.x + mProfile->mTextOffset.x, yPos + offset.y);
                    dstRect.extent.set(rowWidth, rowHeight);

                    GFX->drawBitmapStretchSR(mProfile->mTextureObject, dstRect, srcRect);
                }

                fontColor = mProfile->mFontColors[1];
            } else
            {
                if (unselectedBitmapIndex != -1)
                {
                    GFX->clearBitmapModulation();

                    RectI srcRect = mProfile->mBitmapArrayRects[unselectedBitmapIndex];

                    RectI dstRect;
                    dstRect.point.set(offset.x + mProfile->mTextOffset.x, yPos + offset.y);
                    dstRect.extent.set(rowWidth, rowHeight);

                    GFX->drawBitmapStretchSR(mProfile->mTextureObject, dstRect, srcRect);
                }

                fontColor = mProfile->mFontColors[0];
            }

            if (iconIndex != -1 && iconIndex < mProfile->mBitmapArrayRects.size())
            {
                GFX->clearBitmapModulation();
                
                Point2I renderPos(offset.x + mProfile->mIconPosition.x, yPos + offset.y + mProfile->mIconPosition.y);

                GFX->drawBitmapSR(mProfile->mTextureObject, renderPos, mProfile->mBitmapArrayRects[iconIndex]);
            }

            fontColor.alpha = 255;
            GuiControlProfile::AlignmentType oldAlignment = mProfile->mAlignment;
            if (hasSelectBitmap)
                mProfile->mAlignment = GuiControlProfile::RightJustify;

            GFX->setBitmapModulation(fontColor);

            renderJustifiedText(Point2I(leftRect.point.x + mProfile->mTextOffset.x, yPos + leftRect.point.y),
                                Point2I(leftRect.extent.x, rowHeight),
                                mRowText[index]);

            mProfile->mAlignment = oldAlignment;
            
            if (index == mSelected)
                fontColor = mProfile->mFontColors[3];
            else
                fontColor = mProfile->mFontColors[2];
            fontColor.alpha = 255;

            mProfile->mAlignment = GuiControlProfile::CenterJustify;

            const char* optionText = getOptionText(index, mRowOptionIndex[index]);

            GFX->setBitmapModulation(fontColor);

            renderJustifiedText(Point2I(rightRect.point.x + mProfile->mTextOffset.x, yPos + rightRect.point.y), Point2I(rightRect.extent.x, rowHeight), optionText);

            mProfile->mAlignment = oldAlignment;

            S32 arrowOffset = 0;
            if (hasSelectBitmap)
            {
                if ((rowHeight - bitmapArrowHeight) / 2 <= 0)
                    arrowOffset = 0;
                else
                    arrowOffset = (rowHeight - bitmapArrowHeight) / 2;
            }

#ifdef MBO_UNTOUCHED_MENUS
            GFX->clearBitmapModulation();
#else
            ColorI arrowHighlightColor(255, 255, 255);
            if (smLegacyUI || !mButtonsEnabled)
                arrowHighlightColor = ColorI(128, 128, 255);
#endif

            S32 leftArrowIndex = unselectedLeftArrowIndex;
            if (index == mSelected && (smLegacyUI || !mButtonsEnabled))
                leftArrowIndex = selectedLeftArrowIndex;

            if (leftArrowIndex != -1)
            {
#ifndef MBO_UNTOUCHED_MENUS
                if (!smLegacyUI && mButtonsEnabled)
                {
                    Point2I clickOffset(0, 0);

                    if (mArrowHover == -1 && index == mSelected && mButtonsEnabled)
                    {
                        GFX->setBitmapModulation(arrowHighlightColor);
                        if (mMouseDown)
                        {
                            GFX->setBitmapModulation(ColorI(128, 128, 128, 255));
                            clickOffset = Point2I(1, 1);
                        }
                    } else
                        GFX->clearBitmapModulation();
                    //GFX->setBitmapModulation(ColorI(200, 200, 200, 255));

                    RectI srcRect = mProfile->mBitmapArrayRects[buttonIndex];
                    if (mArrowHover == -1 && index == mSelected && mButtonsEnabled)
                        srcRect = mProfile->mBitmapArrayRects[hoverButtonIndex];

                    RectI dstRect;
                    dstRect.point.set(rightRect.point.x,
                                      yPos + offset.y);//offset.x + mProfile->mTextOffset.x, yPos + offset.y);
                    //dstRect.point += clickOffset;
                    dstRect.extent.set(bitmapButtonWidth, bitmapButtonHeight);

                    GFX->drawBitmapStretchSR(mProfile->mTextureObject, dstRect, srcRect);

                    GFX->clearBitmapModulation();
                } else
                {
                    if (mArrowHover == -1 && index == mSelected && mButtonsEnabled)
                        GFX->setBitmapModulation(arrowHighlightColor);
                    else
                        GFX->clearBitmapModulation();
                }
#endif

                RectI rect;
                if (smLegacyUI || !mButtonsEnabled)
                {
                    rect.point.set(rightRect.point.x, yPos + arrowOffset + rightRect.point.y);
                    rect.extent.set(bitmapArrowWidth, bitmapArrowHeight);
                } else
                {
                    S32 arrowX = rightRect.point.x;
                    arrowX -= (S32) (bitmapArrowWidth / 1.2f);
                    arrowX += bitmapButtonWidth / 2;

                    rect.point.set(arrowX, yPos + arrowOffset + rightRect.point.y);
                    rect.extent.set(bitmapArrowWidth, bitmapArrowHeight);
                }

                if (mButtonsEnabled)
                    GFX->drawBitmapStretchSR(mProfile->mTextureObject, rect, mProfile->mBitmapArrayRects[leftArrowIndex]);
            }

            S32 rightArrowIndex = unselectedRightArrowIndex;
            if (index == mSelected && (smLegacyUI || !mButtonsEnabled))
                rightArrowIndex = selectedRightArrowIndex;

            if (rightArrowIndex != -1)
            {
#ifndef MBO_UNTOUCHED_MENUS

                if (!smLegacyUI && mButtonsEnabled)
                {
                    Point2I clickOffset(0, 0);

                    if (mArrowHover == 1 && index == mSelected && mButtonsEnabled)
                    {
                        GFX->setBitmapModulation(arrowHighlightColor);
                        if (mMouseDown)
                        {
                            GFX->setBitmapModulation(ColorI(128, 128, 128, 255));
                            clickOffset = Point2I(1, 1);
                        }
                    } else
                        GFX->clearBitmapModulation();
                    //GFX->setBitmapModulation(ColorI(200, 200, 200, 255));

                    RectI srcRect = mProfile->mBitmapArrayRects[buttonIndex];
                    if (mArrowHover == 1 && index == mSelected && mButtonsEnabled)
                        srcRect = mProfile->mBitmapArrayRects[hoverButtonIndex];

                    RectI dstRect;
                    dstRect.point.set(rightRect.extent.x + rightRect.point.x - bitmapButtonWidth,
                                      yPos + offset.y);//offset.x + mProfile->mTextOffset.x, yPos + offset.y);
                    //dstRect.point += clickOffset;
                    dstRect.extent.set(bitmapButtonWidth, bitmapButtonHeight);

                    GFX->drawBitmapStretchSR(mProfile->mTextureObject, dstRect, srcRect);

                    GFX->clearBitmapModulation();
                } else {
                    if (mArrowHover == 1 && index == mSelected && mButtonsEnabled)
                        GFX->setBitmapModulation(arrowHighlightColor);
                    else
                        GFX->clearBitmapModulation();
                }
#endif

                RectI rect;
                if (smLegacyUI || !mButtonsEnabled)
                {
                    rect.point.set(
                        rightRect.extent.x + rightRect.point.x - bitmapArrowWidth,
                        yPos + arrowOffset + rightRect.point.y);
                    rect.extent.set(bitmapArrowWidth, bitmapArrowHeight);
                } else
                {
                    rect.point.set(
                        rightRect.extent.x + rightRect.point.x - (S32) (bitmapArrowWidth / 3.2) - bitmapButtonWidth / 2,
                        yPos + arrowOffset + rightRect.point.y);
                    rect.extent.set(bitmapArrowWidth, bitmapArrowHeight);
                }

                if (mButtonsEnabled)
                    GFX->drawBitmapStretchSR(mProfile->mTextureObject, rect, mProfile->mBitmapArrayRects[rightArrowIndex]);
            }

#ifndef MBO_UNTOUCHED_MENUS
            GFX->clearBitmapModulation();
#endif

            if (mShowRects)
            {
                S32 rHeight = rowHeight;
                S32 separation = 0;
                if (mProfile->mHitArea.size() == 2)
                {
                    separation = mProfile->mHitArea[0];
                    rHeight = mProfile->mHitArea[1];
                }

                RectI rect;
                rect.point.set(offset.x + mProfile->mTextOffset.x, offset.y + yPos + separation);
                rect.extent.set(rowWidth, rHeight + yPos);

                GFX->drawRect(rect, ColorI(255, 192, 255));
            }

            i += rowHeight;
            index++;
            if (index >= mTopRow + mRowsPerPage)
                break;
            curRow = index;
        }
    }

    renderChildControls(offset, updateRect);
}

const char* GuiXboxOptionListCtrl::getSelectedText()
{
    if (mSelected >= mRowText.size())
        return StringTable->insert("");
    return mRowText[mSelected];
}

const char* GuiXboxOptionListCtrl::getSelectedData()
{
    if (mSelected >= mRowData.size())
        return StringTable->insert("");
    return mRowData[mSelected];
}

#ifndef MBO_UNTOUCHED_MENUS
void GuiXboxOptionListCtrl::onMouseLeave(const GuiEvent& event)
{
    mMouseDown = false;
}
#endif

#ifndef MBO_UNTOUCHED_MENUS
void GuiXboxOptionListCtrl::onMouseMove(const GuiEvent& event)
{
    mArrowHover = 0;

    Point2I localPoint = globalToLocalCoord(event.mousePoint);
    S32 row = getRowIndex(localPoint);

    if (row >= 0)
    {
        if (row != mSelected)
            move(row - mSelected);

        // Arrow hover
        S32 bitmapArrowWidth = 0;
        if (mProfile->mBitmapArrayRects.size() > 2)
            bitmapArrowWidth = mProfile->mBitmapArrayRects[2].extent.x;

#ifndef MBO_UNTOUCHED_MENUS
        S32 bitmapButtonWidth = 0;
        if (mProfile->mBitmapArrayRects.size() > 5)
            bitmapButtonWidth = mProfile->mBitmapArrayRects[6].extent.x;
#endif

        S32 c2LM = 0;
        S32 c2RM = 0;
        if (mColumnMargins.size() > 2)
            c2LM = mColumnMargins[2];
        if (mColumnMargins.size() > 3)
            c2RM = mColumnMargins[3];

        S32 unk1 = (S32)((F32)mColumnWidth[0] / 100.0f * (F32)mBounds.extent.x);
        S32 unk2 = c2LM + unk1;
        S32 unk3 = unk1 + (S32)((F32)mBounds.extent.x * ((F32)mColumnWidth[1] / 100.0f)) - c2RM;

#ifdef MBO_UNTOUCHED_MENUS
        if (unk2 > localPoint.x || localPoint.x > bitmapArrowWidth + unk2)
        {
            if (unk3 - bitmapArrowWidth <= localPoint.x && localPoint.x <= unk3)
#else
        S32 wid = bitmapButtonWidth;
            if (smLegacyUI)
                wid = bitmapArrowWidth;
        if (unk2 > localPoint.x || localPoint.x > wid + unk2)
        {
            if (unk3 - wid <= localPoint.x && localPoint.x <= unk3)
#endif
            {
                // Right arrow hover
                mArrowHover = 1;
            }
        }
        else
        {
            // Left arrow hover
            mArrowHover = -1;
        }
    }
}
#endif

void GuiXboxOptionListCtrl::onMouseDown(const GuiEvent& event)
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

void GuiXboxOptionListCtrl::onMouseUp(const GuiEvent& event)
{
#ifndef MBO_UNTOUCHED_MENUS
    mMouseDown = false;
#endif

    Point2I localPoint = globalToLocalCoord(event.mousePoint);

    if (mSelected == getRowIndex(localPoint))
        clickOption(localPoint.x);
}

void GuiXboxOptionListCtrl::clickOption(S32 xPos)
{
    // Arrow hover
    S32 bitmapArrowWidth = 0;
    if (mProfile->mBitmapArrayRects.size() > 2)
        bitmapArrowWidth = mProfile->mBitmapArrayRects[2].extent.x;

#ifndef MBO_UNTOUCHED_MENUS
    S32 bitmapButtonWidth = 0;
    if (mProfile->mBitmapArrayRects.size() > 5)
        bitmapButtonWidth = mProfile->mBitmapArrayRects[6].extent.x;
#endif

    S32 c2LM = 0;
    S32 c2RM = 0;
    if (mColumnMargins.size() > 2)
        c2LM = mColumnMargins[2];
    if (mColumnMargins.size() > 3)
        c2RM = mColumnMargins[3];

    S32 unk1 = (S32)((F32)mColumnWidth[0] / 100.0f * (F32)mBounds.extent.x);
    S32 unk2 = c2LM + unk1;
    S32 unk3 = unk1 + (S32)((F32)mBounds.extent.x * ((F32)mColumnWidth[1] / 100.0f)) - c2RM;

#ifdef MBO_UNTOUCHED_MENUS
    if (unk2 > xPos || xPos > bitmapArrowWidth + unk2)
    {
        if (unk3 - bitmapArrowWidth <= xPos && xPos <= unk3)
        {
#else
    S32 wid = bitmapButtonWidth;
    if (smLegacyUI)
        wid = bitmapArrowWidth;
    if (unk2 > xPos || xPos > wid + unk2)
    {
        if (unk3 - wid <= xPos && xPos <= unk3)
        {
#endif
            incOption();
#ifdef MBO_UNTOUCHED_MENUS
            Con::executef(this, 1, "onRight");
#else
            const char* retval = Con::executef(this, 1, "onRight");
            if (!dStrcmp(retval, ""))
                Parent::onGamepadButtonPressed(XI_DPAD_RIGHT);
#endif
        }
    } else
    {
        decOption();
#ifdef MBO_UNTOUCHED_MENUS
        Con::executef(this, 1, "onLeft");
#else
        const char* retval = Con::executef(this, 1, "onLeft");
        if (!dStrcmp(retval, ""))
            Parent::onGamepadButtonPressed(XI_DPAD_LEFT);
#endif
    }
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
        SFX->playOnce(mProfile->mSoundOptionChanged);

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
        SFX->playOnce(mProfile->mSoundOptionChanged);

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
