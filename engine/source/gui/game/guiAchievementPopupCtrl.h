#ifndef _GUIACHIEVEMENTPOPUPCTRL_H_
#define _GUIACHIEVEMENTPOPUPCTRL_H_

#include "gui/core/guiControl.h"

class GuiAchievementPopupCtrl : public GuiControl
{
    typedef GuiControl Parent;

protected:
    StringTableEntry mBitmapName;
    GFXTexHandle mTextureObject;

public:
    GuiAchievementPopupCtrl();
    static void initPersistFields();
    void onRender(Point2I offset, const RectI &updateRect);
    bool onWake();
    void setBitmap(const char *name);

    DECLARE_CONOBJECT(GuiAchievementPopupCtrl);
};

#endif // _GUIACHIEVEMENTPOPUPCTRL_H_
