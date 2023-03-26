//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _GUIXBOXOPTIONLISTCTRL_H_
#define _GUIXBOXOPTIONLISTCTRL_H_

#ifndef _GUIGAMELISTMENUCTRL_H_
#include "gui/controls/guiGameListMenuCtrl.h"
#endif

class GuiXboxOptionListCtrl : public GuiGameListMenuCtrl
{
private:
    typedef GuiGameListMenuCtrl Parent;

protected:
    Vector<S32> mRowOptionIndex;
    Vector<S32> mRowIconIndex;
    Vector<bool> mRowDataWrap;
    bool mBorder;
    bool mShowRects;
    Vector<S32> mColumnWidth;
    Vector<S32> mColumnMargins;
    S32 mColumnGap;
#ifndef MBO_UNTOUCHED_MENUS
    bool mMouseDown;
    S32 mArrowHover;
#endif
    bool mButtonsEnabled;

public:
    DECLARE_CONOBJECT(GuiXboxOptionListCtrl);
    GuiXboxOptionListCtrl();
    static void initPersistFields();

    virtual bool onWake();
    virtual void onSleep();

    bool areButtonsEnabled();
    void setButtonsEnabled(bool enabled);

    S32 getTopRow();
    S32 getBottomRow();
    bool onGamepadButtonPressed(U32 button);
    U32 getRowCount();
    S32 getRowIndex(Point2I localPoint);
    const char* getRowData(S32 idx);
    void onRender(Point2I offset, const RectI& updateRect);
    const char* getSelectedText();
    S32 getSelectedIndex();
    const char* getSelectedData();
#ifndef MBO_UNTOUCHED_MENUS
    void onMouseLeave(const GuiEvent& event);
    void onMouseMove(const GuiEvent& event);
#endif
    void onMouseDown(const GuiEvent& event);
    void onMouseUp(const GuiEvent& event);
    void onMouseDragged(const GuiEvent& event);
    void addRow(const char* text, const char* data, S32 bitmapIndex);
    void clear();
    void clickOption(S32 xPos);
    void decOption();
    void incOption();
    U32 getOptionCount(S32 row);
    S32 getOptionIndex(S32 row);
    void setOptionIndex(S32 row, S32 idx);
    void setOptionWrap(S32 row, bool wrap);
    const char* getOptionText(S32 row, U32 index);
};

#endif // _GUIXBOXOPTIONLISTCTRL_H_