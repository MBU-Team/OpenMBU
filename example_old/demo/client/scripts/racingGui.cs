//-----------------------------------------------------------------------------
// Torque Shader Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// RacingGui is the main TSControl through which the racing game is viewed.
// The RacingGui also contains the hud controls.
//-----------------------------------------------------------------------------

function RacingGui::onWake(%this)
{
   // Turn off any shell sounds...
   // alxStop( ... );

   $enableDirectInput = "1";
   activateDirectInput();

   // Message hud dialog
   Canvas.pushDialog(MainChatHud);
   chatHud.attach(HudMessageVector);

   // Push the game's action key mapings
   moveMap.push();
   
   // Force third person view as the default.
   $firstPerson = false;
}

function RacingGui::onSleep(%this)
{
   Canvas.popDialog(MainChatHud);
   
   // Pop the keymaps
   moveMap.pop();
}


