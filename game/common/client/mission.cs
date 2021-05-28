//-----------------------------------------------------------------------------
// Torque Shader Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// Mission start / end events sent from the server
//----------------------------------------------------------------------------

function clientCmdMissionStart(%seq)
{
   // The client recieves a mission start right before
   // being dropped into the game.
}

function clientCmdMissionEnd(%seq)
{
   // Recieved when the current mission is ended.
   sfxStopAll($SimAudioType);

   // Disable mission lighting if it's going, this is here
   // in case the mission ends while we are in the process
   // of loading it.
   $lightingMission = false;
   $sceneLighting::terminateLighting = true;
}

