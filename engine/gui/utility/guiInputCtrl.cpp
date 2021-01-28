//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "gui/utility/guiInputCtrl.h"
#include "sim/actionMap.h"

IMPLEMENT_CONOBJECT(GuiInputCtrl);

//------------------------------------------------------------------------------
bool GuiInputCtrl::onWake()
{
   // Set the default profile on start-up:
   if ( !mProfile )
   {
      SimObject *obj = Sim::findObject("GuiInputCtrlProfile");
      if ( obj )
         mProfile = dynamic_cast<GuiControlProfile*>( obj );
   }

   if ( !Parent::onWake() )
      return( false );

   mouseLock();
   setFirstResponder();

   return( true );
}


//------------------------------------------------------------------------------
void GuiInputCtrl::onSleep()
{
   Parent::onSleep();
   mouseUnlock();
   clearFirstResponder();
}


//------------------------------------------------------------------------------
static bool isModifierKey( U16 keyCode )
{
   switch ( keyCode )
   {
      case KEY_LCONTROL:
      case KEY_RCONTROL:
      case KEY_LALT:
      case KEY_RALT:
      case KEY_LSHIFT:
      case KEY_RSHIFT:
         return( true );
   }

   return( false );
}

//------------------------------------------------------------------------------
bool GuiInputCtrl::onInputEvent( const InputEvent &event )
{
   // TODO - add POV support...
   if ( event.action == SI_MAKE )
   {
      if ( event.objType == SI_BUTTON
        || event.objType == SI_POV
        || ( ( event.objType == SI_KEY ) && !isModifierKey( event.objInst ) ) )
      {
         char deviceString[32];
         if ( !ActionMap::getDeviceName( event.deviceType, event.deviceInst, deviceString ) )
            return( false );

         const char* actionString = ActionMap::buildActionString( &event );

         Con::executef( this, 4, "onInputEvent", deviceString, actionString, "1" );
         return( true );
      }
   }
   else if ( event.action == SI_BREAK )
   {
      if ( ( event.objType == SI_KEY ) && isModifierKey( event.objInst ) )
      {
         char keyString[32];
         if ( !ActionMap::getKeyString( event.objInst, keyString ) )
            return( false );

         Con::executef( this, 4, "onInputEvent", "keyboard", keyString, "0" );
         return( true );
      }
   }

   return( false );
}
