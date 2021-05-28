#ifndef _GUISTACKCTRL_H_
#define _GUISTACKCTRL_H_

#ifndef _GUICONTROL_H_
#include "gui/core/guiControl.h"
#endif

#include "gfx/gfxDevice.h"
#include "console/console.h"
#include "console/consoleTypes.h"

/// A stack of GUI controls.
///
/// This maintains a horizontal or vertical stack of GUI controls. If one is deleted, or
/// resized, then the stack is resized to fit. The order of the stack is
/// determined by the internal order of the children (ie, order of addition).
///
///
/// @todo Make this support horizontal right to left stacks.
class GuiStackControl : public GuiControl
{
protected:
    typedef GuiControl Parent;
    bool             mResizing;
    S32              mPadding;
    S32 mStackHorizSizing;      ///< Set from horizSizingOptions.
    S32 mStackVertSizing;       ///< Set from vertSizingOptions.
    S32 mStackingType;

public:
    GuiStackControl();

    enum stackingOptions
    {
        horizStackLeft = 0,///< Stack from left to right when horizontal
        horizStackRight,   ///< Stack from right to left when horizontal
        vertStackTop,      ///< Stack from top to bottom when vertical
        vertStackBottom,   ///< Stack from bottom to top when vertical
        stackingTypeVert,  ///< Always stack vertically
        stackingTypeHoriz, ///< Always stack horizontally
        stackingTypeDyn    ///< Dynamically switch based on width/height
    };


    void resize(const Point2I& newPosition, const Point2I& newExtent);
    void childResized(GuiControl* child);

    bool onWake();
    void onSleep();

    void updatePanes();

    void stackFromLeft();
    void stackFromRight();
    void stackFromTop();
    void stackFromBottom();

    S32 getCount() { return size(); }; /// Returns the number of children in the stack

    void addObject(SimObject* obj);
    void removeObject(SimObject* obj);

    bool reOrder(SimObject* obj, SimObject* target = 0);

    static void initPersistFields();
    DECLARE_CONOBJECT(GuiStackControl);
};

#endif
