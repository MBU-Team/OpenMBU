//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _GUIBARCTRL_H_
#define _GUIBARCTRL_H_

#ifndef _GUICONTROL_H_
#include "gui/core/guiControl.h"
#endif

class GuiBarCtrl : public GuiControl
{
private:
    typedef GuiControl Parent;

public:
    enum Orientation
    {
        LeftRight = 0,
        RightLeft = 1,
        TopBottom = 2,
        BottomTop = 3,
    };

protected:

    Orientation mOrientation;
    F32 mValue;
    ColorI mColors[2];
    StringTableEntry mMaskTextureName;
    GFXTexHandle mMaskTexture;
    
    virtual F32 getValue();

public:
    DECLARE_CONOBJECT(GuiBarCtrl);
    GuiBarCtrl();
    static void initPersistFields(bool accessVal = true);

    bool onWake();
    void inspectPostApply();

    void onRender(Point2I offset, const RectI& updateRect);
};

#endif // _GUIBARCTRL_H_