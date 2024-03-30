//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _GUIBITMAPCTRL_H_
#define _GUIBITMAPCTRL_H_

#ifndef _GUICONTROL_H_
#include "gui/core/guiControl.h"
#endif

/// Renders a bitmap.
class GuiBitmapCtrl : public GuiControl
{
private:
    typedef GuiControl Parent;

protected:
    StringTableEntry mBitmapName;
    StringTableEntry mOnMouseUpCommand;
    GFXTexHandle mTextureObject;
    Point2I startPoint;
    bool mWrap;
    bool flipY;

public:
    //creation methods
    DECLARE_CONOBJECT(GuiBitmapCtrl);
    GuiBitmapCtrl();
    static void initPersistFields();

    //Parental methods
    bool onWake();
    void onSleep();
    void inspectPostApply();
    void setBitmap(const char* name, bool resize = false);
    void setBitmap(GFXTexHandle handle, bool resize = false);
    S32 getWidth() const { return(mTextureObject->getWidth()); }
    S32 getHeight() const { return(mTextureObject->getHeight()); }
    void onMouseUp(const GuiEvent &event) override;

    void onRender(Point2I offset, const RectI& updateRect);
    void setValue(S32 x, S32 y);
};

#endif
