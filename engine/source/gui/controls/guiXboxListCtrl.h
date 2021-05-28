//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _GUIXBOXLISTCTRL_H_
#define _GUIXBOXLISTCTRL_H_

#ifndef _GUIGAMELISTMENUCTRL_H_
#include "gui/controls/guiGameListMenuCtrl.h"
#endif

class GuiXboxListCtrl : public GuiGameListMenuCtrl
{
private:
    typedef GuiGameListMenuCtrl Parent;

protected:
    Vector<U32> mRowHeights;
    Vector<S32> mBitmapIndices;
    Vector<bool> mRowEnabled;
    StringTableEntry mInitialRows;
#ifndef MBO_UNTOUCHED_MENUS
    bool mMouseDown;
#endif

public:
    DECLARE_CONOBJECT(GuiXboxListCtrl);
    GuiXboxListCtrl();
    static void initPersistFields();

    virtual bool onWake();
    virtual void onSleep();

    S32 getTopRow();
    S32 getBottomRow();
    void onMouseDragged(const GuiEvent& event);
    bool onGamepadButtonPressed(U32 button);
    U32 getRowCount();
    S32 getRowIndex(Point2I localPoint);
    void onRender(Point2I offset, const RectI& updateRect);
    const char* getSelectedText();
    const char* getSelectedData();
    void setRowEnabled(S32 idx, bool enabled);
    bool getRowEnabled(S32 idx);
#ifndef MBO_UNTOUCHED_MENUS
    void onMouseLeave(const GuiEvent& event);
    void onMouseMove(const GuiEvent& event);
#endif
    void onMouseDown(const GuiEvent& event);
    void onMouseUp(const GuiEvent& event);
    void addRow(const char* text, const char* data, U32 addedHeight, S32 bitmapIndex);
    void clear();
};

#endif // _GUIXBOXLISTCTRL_H_