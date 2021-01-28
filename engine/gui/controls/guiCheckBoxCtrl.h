//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _GUICHECKBOXCTRL_H_
#define _GUICHECKBOXCTRL_H_

#ifndef _GUIBUTTONBASECTRL_H_
#include "gui/controls/guiButtonBaseCtrl.h"
#endif

class GuiCheckBoxCtrl : public GuiButtonBaseCtrl
{
   typedef GuiButtonBaseCtrl Parent;

protected:
public:
   S32 mIndent;
   DECLARE_CONOBJECT(GuiCheckBoxCtrl);
   GuiCheckBoxCtrl();

   void onRender(Point2I offset, const RectI &updateRect);
   bool onWake();
};

#endif //_GUI_CHECKBOX_CTRL_H
