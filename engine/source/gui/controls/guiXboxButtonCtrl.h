#ifndef _GUIXBOXBUTTONCTRL_H_
#define _GUIXBOXBUTTONCTRL_H_

#ifndef _GUICONTROL_H_
#include "gui/core/guiControl.h"
#endif

class GuiXboxButtonCtrl : public GuiControl
{
private:
    typedef GuiControl Parent;

public:
    enum Constants { MAX_STRING_LENGTH = 255 };
    enum ButtonState
    {
        Normal,
        Hover,
        Down
    };

protected:
    StringTableEntry mInitialText;
    StringTableEntry mInitialTextID;
    UTF8 mText[MAX_STRING_LENGTH + 1];
    ButtonState mButtonState;
    bool mHovering;

public:
    DECLARE_CONOBJECT(GuiXboxButtonCtrl);
    GuiXboxButtonCtrl();
    static void initPersistFields();

    //void setVisible(bool value) override;
    void setButtonHover(bool hover);
    void setText(const char* text);
    void setTextID(S32 id);
    void setTextID(const char* id);
    const char* getText();

    void inspectPostApply() override;

    bool onAdd() override;
    bool onWake() override;
    void onSleep() override;

    void onPreRender() override;
    void onRender(Point2I offset, const RectI &updateRect) override;

    void onMouseDragged(const GuiEvent &event) override;
    void onMouseDown(const GuiEvent &event) override;
    void onMouseUp(const GuiEvent &event) override;
    void onMouseMove(const GuiEvent &event) override;
    void onMouseEnter(const GuiEvent &event) override;
    void onMouseLeave(const GuiEvent &event) override;

};

#endif // _GUIXBOXBUTTONCTRL_H_
