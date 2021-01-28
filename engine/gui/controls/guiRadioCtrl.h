//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _GUIRADIOCTRL_H_
#define _GUIRADIOCTRL_H_

#ifndef _GUICHECKBOXCTRLL_H_
#include "gui/controls/guiCheckBoxCtrl.h"
#endif

// the radio button renders exactly the same as the check box
// the only difference is it sends messages to its siblings to
// turn themselves off.

class GuiRadioCtrl : public GuiCheckBoxCtrl
{
   typedef GuiCheckBoxCtrl Parent;

public:
   DECLARE_CONOBJECT(GuiRadioCtrl);
   GuiRadioCtrl();
};

#endif //_GUI_RADIO_CTRL_H
