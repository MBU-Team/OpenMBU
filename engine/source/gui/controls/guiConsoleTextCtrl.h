//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _GUICONSOLETEXTCTRL_H_
#define _GUICONSOLETEXTCTRL_H_

#ifndef _GFONT_H_
#include "gfx/gFont.h"
#endif
#ifndef _GUITYPES_H_
#include "gui/core/guiTypes.h"
#endif
#ifndef _GUICONTROL_H_
#include "gui/core/guiControl.h"
#endif

class GuiConsoleTextCtrl : public GuiControl
{
private:
    typedef GuiControl Parent;

public:
    enum Constants { MAX_STRING_LENGTH = 255 };


protected:
    const char* mConsoleExpression;
    const char* mResult;
    Resource<GFont> mFont;

public:

    //creation methods
    DECLARE_CONOBJECT(GuiConsoleTextCtrl);
    GuiConsoleTextCtrl();
    static void initPersistFields();

    //Parental methods
    bool onWake();
    void onSleep();

    //text methods
    virtual void setText(const char* txt = NULL);
    const char* getText() { return mConsoleExpression; }

    //rendering methods
    void calcResize();
    void onPreRender();    // do special pre render processing
    void onRender(Point2I offset, const RectI& updateRect);

    //Console methods
    const char* getScriptValue();
    void setScriptValue(const char* value);
};

#endif //_GUI_TEXT_CONTROL_H_
