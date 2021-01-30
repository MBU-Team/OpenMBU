//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "console/console.h"
#include "gui/core/guiCanvas.h"
#include "gui/controls/guiRadioCtrl.h"
#include "console/consoleTypes.h"

//---------------------------------------------------------------------------
IMPLEMENT_CONOBJECT(GuiRadioCtrl);

GuiRadioCtrl::GuiRadioCtrl()
{
    mButtonType = ButtonTypeRadio;
}
