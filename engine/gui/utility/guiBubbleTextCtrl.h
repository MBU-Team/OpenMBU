//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _GUIBUBBLETEXTCTRL_H_
#define _GUIBUBBLETEXTCTRL_H_

#ifndef _GUITEXTCTRL_H_
#include "gui/controls/guiTextCtrl.h"
#endif
#ifndef _GUIMLTEXTCTRL_H_
#include "gui/controls/guiMLTextCtrl.h"
#endif

class GuiBubbleTextCtrl : public GuiTextCtrl
{
  private:
   typedef GuiTextCtrl Parent;

  protected:
     bool mInAction;
   GuiControl *mDlg;
   GuiControl *mPopup;
   GuiMLTextCtrl *mMLText;

   void popBubble();

  public:
   DECLARE_CONOBJECT(GuiBubbleTextCtrl);

   GuiBubbleTextCtrl() { mInAction = false; }

   virtual void onMouseDown(const GuiEvent &event);
};

#endif /* _GUI_BUBBLE_TEXT_CONTROL_H_ */
