//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _GUIBITMAPBUTTON_H_
#define _GUIBITMAPBUTTON_H_

#ifndef _GUIBUTTONCTRL_H_
#include "gui/controls/guiButtonCtrl.h"
#endif

///-------------------------------------
/// Bitmap Button Contrl
/// Set 'bitmap' comsole field to base name of bitmaps to use.  This control will
/// append '_n' for normal
/// append '_h' for hilighted
/// append '_d' for depressed
///
/// if bitmap cannot be found it will use the default bitmap to render.
///
/// if the extent is set to (0,0) in the gui editor and appy hit, this control will
/// set it's extent to be exactly the size of the normal bitmap (if present)
///
class GuiBitmapButtonCtrl : public GuiButtonCtrl
{
private:
    typedef GuiButtonCtrl Parent;

protected:
    StringTableEntry mBitmapName;

    GFXTexHandle mTextureNormal;
    GFXTexHandle mTextureHilight;
    GFXTexHandle mTextureDepressed;
    GFXTexHandle mTextureInactive;

    void renderButton(GFXTexHandle texture, Point2I& offset, const RectI& updateRect);

public:
    DECLARE_CONOBJECT(GuiBitmapButtonCtrl);
    GuiBitmapButtonCtrl();

    static void initPersistFields();

    //Parent methods
    bool onWake();
    void onSleep();
    void inspectPostApply();

    void setBitmap(const char* name);

    void onRender(Point2I offset, const RectI& updateRect);
};

class GuiBitmapButtonTextCtrl : public GuiBitmapButtonCtrl
{
    typedef GuiBitmapButtonCtrl Parent;
public:
    DECLARE_CONOBJECT(GuiBitmapButtonTextCtrl);
    void onRender(Point2I offset, const RectI& updateRect);
};

#endif //_GUI_BITMAP_BUTTON_CTRL_H
