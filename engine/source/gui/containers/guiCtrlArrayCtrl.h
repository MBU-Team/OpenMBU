#ifndef _GUICTRLARRAYCTRL_H_
#define _GUICTRLARRAYCTRL_H_

#ifndef _GUICONTROL_H_
#include "gui/core/guiControl.h"
#endif

#include "gfx/gfxDevice.h"
#include "console/console.h"
#include "console/consoleTypes.h"

class GuiControlArrayControl : public GuiControl
{
private:
    typedef GuiControl Parent;

    S32 mCols;
    Vector<S32> mColumnSizes;
    S32 mRowSize;
    S32 mRowSpacing;
    S32 mColSpacing;

public:
    GuiControlArrayControl();

    void resize(const Point2I& newPosition, const Point2I& newExtent);

    bool onWake();
    void onSleep();
    void inspectPostApply();

    static void initPersistFields();
    DECLARE_CONOBJECT(GuiControlArrayControl);
};

#endif