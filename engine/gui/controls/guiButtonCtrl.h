//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _GUIBUTTONCTRL_H_
#define _GUIBUTTONCTRL_H_

#ifndef _GUIBUTTONBASECTRL_H_
#include "gui/controls/guiButtonBaseCtrl.h"
#endif

class GuiButtonCtrl : public GuiButtonBaseCtrl
{
   typedef GuiButtonBaseCtrl Parent;

  public:
   DECLARE_CONOBJECT(GuiButtonCtrl);
   GuiButtonCtrl();

   void onRender(Point2I offset, const RectI &updateRect);
};

#endif //_GUI_BUTTON_CTRL_H
