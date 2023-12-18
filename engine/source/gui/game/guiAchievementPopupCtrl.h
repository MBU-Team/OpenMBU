#ifndef _GUIACHIEVEMENTPOPUPCTRL_H_
#define _GUIACHIEVEMENTPOPUPCTRL_H_

#include "gui/core/guiControl.h"

#define ACHIEVEMENT_ICON_OFFSET_X 20
#define ACHIEVEMENT_ICON_OFFSET_Y 17

#define ACHIEVEMENT_HEADER_OFFSET_X 35
#define ACHIEVEMENT_HEADER_OFFSET_Y 40

#define ACHIEVEMENT_TITLE_OFFSET_X 20
#define ACHIEVEMENT_TITLE_OFFSET_Y 10

class GuiAchievementPopupCtrl : public GuiControl
{
    typedef GuiControl Parent;

protected:
    StringTableEntry mIconBitmapName;
    GFXTexHandle mIconTextureObject;
    StringTableEntry mBackgroundBitmapName;
    GFXTexHandle mBackgroundTextureObject;
    StringTableEntry mTitle;
    StringTableEntry mHeader;

public:
    GuiAchievementPopupCtrl();
    static void initPersistFields();
    void onRender(Point2I offset, const RectI &updateRect);
    bool onWake();
    void setIcon(const char *name);
    void setBackground(const char *name);
    void setTitle(const char *title);
    void setHeader(const char *header);

    DECLARE_CONOBJECT(GuiAchievementPopupCtrl);
};

#endif // _GUIACHIEVEMENTPOPUPCTRL_H_
