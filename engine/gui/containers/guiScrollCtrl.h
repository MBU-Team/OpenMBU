//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _GUISCROLLCTRL_H_
#define _GUISCROLLCTRL_H_

#ifndef _GUICONTROL_H_
#include "gui/core/guiControl.h"
#endif

class GuiScrollCtrl : public GuiControl
{
private:
    typedef GuiControl Parent;

protected:
    // the scroll control uses a bitmap array to draw all its
    // graphics... these are the bitmaps it needs:

    enum BitmapIndices
    {
        BmpUp,
        BmpDown,
        BmpVThumbTopCap,
        BmpVThumb,
        BmpVThumbBottomCap,
        BmpVPage,
        BmpLeft,
        BmpRight,
        BmpHThumbLeftCap,
        BmpHThumb,
        BmpHThumbRightCap,
        BmpHPage,
        BmpResize,

        BmpCount
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

    S32     mBorderThickness;           // this gets set per class in the constructor
    Point2I mChildMargin;               // the thickeness of the margin around the child controls

    // note - it is implicit in the scroll view that the buttons all have the same
    // arrow length and that horizontal and vertical scroll bars have the
    // same thickness

    S32 mScrollBarThickness;        // determined by the width of the vertical page bmp
    S32 mScrollBarArrowBtnLength;   // determined by the height of the up arrow

    bool mHBarEnabled;
    bool mVBarEnabled;
    bool mHasHScrollBar;
    bool mHasVScrollBar;

    Point2I mContentPos;  // the position of the content region in the control's coord system
    Point2I mContentExt;  // the extent of the content region

    Point2I mChildPos;   // the position of the upper left corner of the child control(s)
    Point2I mChildExt;

    Point2I mChildRelPos; // the relative position of the upper left content corner in
                          // the child's coordinate system - 0,0 if scrolled all the way to upper left.

    //--------------------------------------
    // for mouse dragging the thumb
    Point2I mChildRelPosAnchor; // the original childRelPos when scrolling started
    S32 mThumbMouseDelta;

    S32 mLastUpdated;

    S32 mHThumbSize;
    S32 mHThumbPos;

    S32 mVThumbSize;
    S32 mVThumbPos;

    S32 mBaseThumbSize;

    RectI mUpArrowRect;
    RectI mDownArrowRect;
    RectI mLeftArrowRect;
    RectI mRightArrowRect;
    RectI mHTrackRect;
    RectI mVTrackRect;

    //--------------------------------------
    // for determing hit area
public:      //called by the ComboPopUp class
    enum Region
    {
        UpArrow,
        DownArrow,
        LeftArrow,
        RightArrow,
        UpPage,
        DownPage,
        LeftPage,
        RightPage,
        VertThumb,
        HorizThumb,
        None
    };
    enum {
        ScrollBarAlwaysOn = 0,
        ScrollBarAlwaysOff = 1,
        ScrollBarDynamic = 2
    };

    bool stateDepressed;
    Region curHitRegion;

    bool disabled;
    S32 mForceHScrollBar;
    S32 mForceVScrollBar;

    bool mUseConstantHeightThumb;
    bool mWillFirstRespond;     // for automatically handling arrow keys

    Region findHitRegion(const Point2I&);

protected:

    virtual bool calcChildExtents();
    virtual void calcScrollRects(void);
    void calcThumbs();
    void scrollByRegion(Region reg);

    //--------------------------------------

    //--------------------------------------

public:
    GuiScrollCtrl();
    DECLARE_CONOBJECT(GuiScrollCtrl);
    static void initPersistFields();
    void autoScroll(Region reg);

    void scrollTo(S32 x, S32 y);
    void scrollDelta(S32 x, S32 y);
    void scrollRectVisible(RectI rect);

    void computeSizes();

    // you can change the bitmap array dynamically.
    void loadBitmapArray();

    void addObject(SimObject* obj);
    void resize(const Point2I& newPosition, const Point2I& newExtent);
    void childResized(GuiControl* child);
    Point2I getChildPos() { return mChildPos; }
    Point2I getChildExtent() { return mChildExt; }
    Point2I getContentExtent() { return mContentExt; }
    S32 getBorderThickness(void) { return mBorderThickness; }
    S32 scrollBarThickness() const { return(mScrollBarThickness); }
    S32 scrollBarArrowBtnLength() const { return(mScrollBarArrowBtnLength); }
    bool hasHScrollBar() const { return(mHasHScrollBar); }
    bool hasVScrollBar() const { return(mHasVScrollBar); }
    bool enabledHScrollBar() const { return(mHBarEnabled); }
    bool enabledVScrollBar() const { return(mVBarEnabled); }

    bool isScrolledToBottom() { return mChildPos.y + mChildExt.y <= mContentPos.y + mContentExt.y; }

    bool wantsTabListMembership();
    bool becomeFirstResponder();
    bool loseFirstResponder();

    Region getCurHitRegion(void) { return curHitRegion; }

    bool onKeyDown(const GuiEvent& event);
    void onMouseDown(const GuiEvent& event);
    void onMouseRepeat(const GuiEvent& event);
    void onMouseUp(const GuiEvent& event);
    void onMouseDragged(const GuiEvent& event);
    bool onMouseWheelUp(const GuiEvent& event);
    bool onMouseWheelDown(const GuiEvent& event);

    bool onWake();
    void onSleep();

    void onPreRender();
    void onRender(Point2I offset, const RectI& updateRect);
    virtual void drawBorder(const Point2I& offset, bool isFirstResponder);
    virtual void drawVScrollBar(const Point2I& offset);
    virtual void drawHScrollBar(const Point2I& offset);
    virtual void drawScrollCorner(const Point2I& offset);
    virtual GuiControl* findHitControl(const Point2I& pt, S32 initialLayer = -1);
};

#endif //_GUI_SCROLL_CTRL_H
