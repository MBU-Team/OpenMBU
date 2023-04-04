#ifndef _GUIACHIEVEMENTPOPUPCTRL_H_
#define _GUIACHIEVEMENTPOPUPCTRL_H_

#include "gui/core/guiControl.h"

#define ACHIEVEMENT_ICON_OFFSET_X 20
#define ACHIEVEMENT_ICON_OFFSET_Y 17

#define HEADER_OFFSET_Y 30

class GuiAchievementPopupCtrl : public GuiControl
{
    typedef GuiControl Parent;

protected:
    StringTableEntry mIconBitmapName;
    GFXTexHandle mIconTextureObject;
    StringTableEntry mBackgroundBitmapName;
    GFXTexHandle mBackgroundTextureObject;
    StringTableEntry mTitle;

public:
    GuiAchievementPopupCtrl();
    static void initPersistFields();
    void onRender(Point2I offset, const RectI &updateRect);
    bool onWake();
    void setIcon(const char *name);
    void setBackground(const char *name);
    void setTitle(const char *title);

    DECLARE_CONOBJECT(GuiAchievementPopupCtrl);
};

#endif // _GUIACHIEVEMENTPOPUPCTRL_H_
