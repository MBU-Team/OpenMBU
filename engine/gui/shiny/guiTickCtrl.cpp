//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------
#include "gui/shiny/guiTickCtrl.h"

IMPLEMENT_CONOBJECT( GuiTickCtrl );

//------------------------------------------------------------------------------

ConsoleMethod( GuiTickCtrl, setProcessTicks, void, 2, 3, "( [tick = true] ) - This will set this object to either be processing ticks or not" )
{
   if( argc == 3 )
      object->setProcessTicks( dAtob( argv[2] ) );
   else
      object->setProcessTicks();
}