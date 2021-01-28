//-----------------------------------------------------------------------------
// Torque Shader Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------


//----------------------------------------------------------------------------
// Game start / end events sent from the server
//----------------------------------------------------------------------------

function clientCmdGameStart(%seq)
{
   PlayerListGui.zeroScores();
}

function clientCmdGameEnd(%seq)
{
   // Stop local activity... the game will be destroyed on the server
   alxStopAll();

   // Copy the current scores from the player list into the
   // end game gui (bit of a hack for now).
   EndGameGuiList.clear();
   for (%i = 0; %i < PlayerListGuiList.rowCount(); %i++) {
      %text = PlayerListGuiList.getRowText(%i);
      %id = PlayerListGuiList.getRowId(%i);
      EndGameGuiList.addRow(%id,%text);
   }
   EndGameGuiList.sortNumerical(1,false);

   // Display the end-game screen
   Canvas.setContent(EndGameGui);
}

//----------------------------------------------------------------------------
// Game gui
//----------------------------------------------------------------------------

// Initialize GameGUI to the default gui screen.
$Client::GameGUI = "PlayGUI";

function clientCmdSetGameGui(%gui)
{
   // The GameGUI isn't loaded here directly. Game gui is normally loaded
   // when the client first recieves a control object from the server. (This is
   // handled by the GameConnection scripts, located in serverConnection.cs)
   // But if we aren't in the editor and the game is currently in progress,
   // then we probably should load the gui...
   $Client::GameGUI = %gui; 
}



