//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _GUIBACKGROUNDCTRL_H_
#define _GUIBACKGROUNDCTRL_H_

#ifndef _GUICONTROL_H_
#include "gui/core/guiControl.h"
#endif

/// Renders a background, so you can have a backdrop for your GUI.
class GuiBackgroundCtrl : public GuiControl
{
private:
   typedef GuiControl Parent;

public:
   bool  mDraw;

   //creation methods
   DECLARE_CONOBJECT(GuiBackgroundCtrl);
   GuiBackgroundCtrl();

   void onRender(Point2I offset, const RectI &updateRect);
};

#endif
