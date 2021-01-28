//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _GUICONSOLEEDITCTRL_H_
#define _GUICONSOLEEDITCTRL_H_

#ifndef _GUITYPES_H_
#include "gui/core/guiTypes.h"
#endif
#ifndef _GUITEXTEDITCTRL_H_
#include "gui/controls/guiTextEditCtrl.h"
#endif
#ifndef _GUISCROLLCTRL_H_
#include "gui/containers/guiScrollCtrl.h"
#endif

class GuiConsoleEditCtrl : public GuiTextEditCtrl
{
private:
   typedef GuiTextEditCtrl Parent;

protected:
   bool mUseSiblingScroller;
   GuiScrollCtrl* mSiblingScroller;

public:
   GuiConsoleEditCtrl();
   DECLARE_CONOBJECT(GuiConsoleEditCtrl);

   static void initPersistFields();

   bool onKeyDown(const GuiEvent &event);
};

#endif //_GUI_TEXTEDIT_CTRL_H
