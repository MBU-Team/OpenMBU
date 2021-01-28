//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _GUIINPUTCTRL_H_
#define _GUIINPUTCTRL_H_

#ifndef _GUICONTROL_H_
#include "gui/core/guiControl.h"
#endif
#ifndef _EVENT_H_
#include "platform/event.h"
#endif

class GuiInputCtrl : public GuiControl
{
   private:
      typedef GuiControl Parent;

   public:
      DECLARE_CONOBJECT(GuiInputCtrl);

      bool onWake();
      void onSleep();

      bool onInputEvent( const InputEvent &event );
};

#endif // _GUI_INPUTCTRL_H
