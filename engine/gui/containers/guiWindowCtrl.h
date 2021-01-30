//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _GUIWINDOWCTRL_H_
#define _GUIWINDOWCTRL_H_

#ifndef _GUITEXTCTRL_H_
#include "gui/controls/guiTextCtrl.h"
#endif

class GuiWindowCtrl : public GuiTextCtrl
{
private:
    typedef GuiTextCtrl Parent;

    bool mResizeWidth;
    bool mResizeHeight;
    bool mCanMove;
    bool mCanClose;
    bool mCanMinimize;
    bool mCanMaximize;
    bool mPressClose;
    bool mPressMinimize;
    bool mPressMaximize;
    Point2I mMinSize;

    GuiCursor* mDefaultCursor;
    GuiCursor* mLeftRightCursor;
    GuiCursor* mUpDownCursor;
    GuiCursor* mNWSECursor;
    GuiCursor* mNESWCursor;

    StringTableEntry mCloseCommand;

    S32 mTitleHeight;
    S32 mResizeRightWidth;
    S32 mResizeBottomHeight;

    bool mMouseMovingWin;
    bool mMouseResizeWidth;
    bool mMouseResizeHeight;
    bool mMinimized;
    bool mMaximized;

    Point2I mMouseDownPosition;
    RectI mOrigBounds;
    RectI mStandardBounds;

    RectI mCloseButton;
    RectI mMinimizeButton;
    RectI mMaximizeButton;
    S32 mMinimizeIndex;
    S32 mTabIndex;

    void PositionButtons(void);

protected:
    enum BitmapIndices
    {
        BmpClose,
        BmpMaximize,
        BmpNormal,
        BmpMinimize,

        BmpCount
    };
    enum {
        BorderTopLeftKey = 12,
        BorderTopRightKey,
        BorderTopKey,
        BorderTopLeftNoKey,
        BorderTopRightNoKey,
        BorderTopNoKey,
        BorderLeft,
        BorderRight,
        BorderBottomLeft,
        BorderBottom,
        BorderBottomRight,
        NumBitmaps
    };

    enum BitmapStates
    {
        BmpDefault = 0,
        BmpHilite,
        BmpDisabled,

        BmpStates
    };
    RectI* mBitmapBounds;  //bmp is [3*n], bmpHL is [3*n + 1], bmpNA is [3*n + 2]
    GFXTexHandle mTextureObject;


    void drawWinRect(const RectI& myRect);

public:
    GuiWindowCtrl();
    DECLARE_CONOBJECT(GuiWindowCtrl);
    static void initPersistFields();

    bool onWake();
    void onSleep();

    bool isMinimized(S32& index);

    bool initCursors();
    void getCursor(GuiCursor*& cursor, bool& showCursor, const GuiEvent& lastGuiEvent);

    void setFont(S32 fntTag);

    GuiControl* findHitControl(const Point2I& pt, S32 initialLayer = -1);
    void resize(const Point2I& newPosition, const Point2I& newExtent);

    void onMouseDown(const GuiEvent& event);
    void onMouseDragged(const GuiEvent& event);
    void onMouseUp(const GuiEvent& event);

    //only cycle tabs through the current window, so overwrite the method
    GuiControl* findNextTabable(GuiControl* curResponder, bool firstCall = true);
    GuiControl* findPrevTabable(GuiControl* curResponder, bool firstCall = true);

    bool onKeyDown(const GuiEvent& event);

    S32 getTabIndex(void) { return mTabIndex; }
    void selectWindow(void);

    void onRender(Point2I offset, const RectI& updateRect);
};

#endif //_GUI_WINDOW_CTRL_H
