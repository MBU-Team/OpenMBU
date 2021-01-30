#ifndef _GUIBITMAPANIMATION_H_
#define _GUIBITMAPANIMATION_H_

#include "gui/core/guiControl.h"

// This control is from MBU...used for the gui "loading" animation (rotating TSE logo)
class GuiBitmapAnimation : public GuiControl
{
    typedef GuiControl Parent;

protected:
    U32 mLastFrameSwitchTime;
    S32 mAnimationFrameDelay;
    U32 mFrameNum;
    bool mBoundsCorrect;


public:
    GuiBitmapAnimation();
    static void initPersistFields();
    void onRender(Point2I offset, const RectI& updateRect);
    bool onWake();

    DECLARE_CONOBJECT(GuiBitmapAnimation);
};

#endif