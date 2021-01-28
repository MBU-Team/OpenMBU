//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _GUIMLTEXTEDITCTRL_H_
#define _GUIMLTEXTEDITCTRL_H_

#ifndef _GUIMLTEXTCTRL_H_
#include "gui/controls/guiMLTextCtrl.h"
#endif

class GuiMLTextEditCtrl : public GuiMLTextCtrl
{
   typedef GuiMLTextCtrl Parent;

   //-------------------------------------- Overrides
  protected:
   StringTableEntry mEscapeCommand;

   // Events
   bool onKeyDown(const GuiEvent&event);

   // Event forwards
   void handleMoveKeys(const GuiEvent&);
   void handleDeleteKeys(const GuiEvent&);

   // rendering
   void onRender(Point2I offset, const RectI &updateRect);

  public:
   GuiMLTextEditCtrl();
   ~GuiMLTextEditCtrl();

   void resize(const Point2I &newPosition, const Point2I &newExtent);

   DECLARE_CONOBJECT(GuiMLTextEditCtrl);
   static void initPersistFields();
};

#endif  // _H_GUIMLTEXTEDITCTRL_
