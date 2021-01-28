//-----------------------------------------------------------------------------
// Torque Shader Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// FpsGui is the main TSControl through which the racing game is viewed.
// The FpsGui also contains the hud controls.
//-----------------------------------------------------------------------------

function FpsGui::onWake(%this)
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
}

function FpsGui::onSleep(%this)
{
   Canvas.popDialog(MainChatHud);
   
   // Pop the keymaps
   moveMap.pop();
}


