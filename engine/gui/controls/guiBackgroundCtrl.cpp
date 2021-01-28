//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "console/console.h"
#include "gui/controls/guiBackgroundCtrl.h"

IMPLEMENT_CONOBJECT(GuiBackgroundCtrl);

//--------------------------------------------------------------------------
GuiBackgroundCtrl::GuiBackgroundCtrl() : GuiControl()
{
   mDraw = false;
}

//--------------------------------------------------------------------------
void GuiBackgroundCtrl::onRender(Point2I offset, const RectI &updateRect)
{
   if ( mDraw )
      Parent::onRender( offset, updateRect );

   renderChildControls(offset, updateRect);
}


