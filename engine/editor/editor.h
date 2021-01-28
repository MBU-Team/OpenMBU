//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _EDITOR_H_
#define _EDITOR_H_

#ifndef _MMATRIX_H_
#include "math/mMatrix.h"
#endif
#ifndef _GUICONTROL_H_
#include "gui/core/guiControl.h"
#endif

class GameBase;

//------------------------------------------------------------------------------

class EditManager : public GuiControl
{
   private:
      typedef GuiControl Parent;

   public:
      EditManager();
      ~EditManager();

      bool onWake();
      void onSleep();

      // SimObject
      bool onAdd();

      MatrixF mBookmarks[10];
      DECLARE_CONOBJECT(EditManager);
};

extern bool gEditingMission;

//------------------------------------------------------------------------------

#endif
