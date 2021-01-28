//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "gui/core/guiControl.h"

//------------------------------------------------------------------------------
class GuiNoMouseCtrl : public GuiControl
{
   typedef GuiControl Parent;
   public:

      // GuiControl
      bool pointInControl(const Point2I &)   { return(false); }
      DECLARE_CONOBJECT(GuiNoMouseCtrl);
};
IMPLEMENT_CONOBJECT(GuiNoMouseCtrl);
