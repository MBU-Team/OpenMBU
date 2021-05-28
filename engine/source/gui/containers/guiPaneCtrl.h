
#ifndef _GUIPANECTRL_H_
#define _GUIPANECTRL_H_

#ifndef _GUICONTROL_H_
#include "gui/core/guiControl.h"
#endif

#include "gfx/gfxDevice.h"
#include "console/console.h"
#include "console/consoleTypes.h"

/// Collapsable pane control.
///
/// This class wraps a single child control and displays a header with caption
/// above it. If you click the header it will collapse or expand. The control
/// resizes itself based on its collapsed/expanded size.
///
/// In the GUI editor, if you just want the header you can make collapsable
/// false. The caption field lets you set the caption. It expects a bitmap
/// (from the GuiControlProfile) that contains two images - the first is
/// displayed when the control is expanded and the second is displayed when
/// it is collapsed. The header is sized based off of the first image.
class GuiPaneControl : public GuiControl
{
private:
    typedef GuiControl Parent;

    Resource<GFont>  mFont;
    bool             mCollapsable;
    bool             mCollapsed;
    bool             mBarBehindText;
    Point2I          mOriginalExtents;
    StringTableEntry mCaption;
    StringTableEntry mCaptionID;
    Point2I          mThumbSize;

    bool mMouseOver;
    bool mDepressed;

public:
    GuiPaneControl();

    void resize(const Point2I& newPosition, const Point2I& newExtent);
    void onRender(Point2I offset, const RectI& updateRect);

    bool getCollapsed() { return mCollapsed; };
    void setCollapsed(bool isCollapsed);

    bool onWake();
    void onSleep();

    virtual void setCaptionID(S32 id);
    virtual void setCaptionID(const char* id);
    void onMouseDown(const GuiEvent& event);
    void onMouseUp(const GuiEvent& event);
    void onMouseMove(const GuiEvent& event);
    void onMouseLeave(const GuiEvent& event);
    void onMouseEnter(const GuiEvent& event);

    static void initPersistFields();
    DECLARE_CONOBJECT(GuiPaneControl);
};

#endif
