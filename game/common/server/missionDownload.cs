//-----------------------------------------------------------------------------
// Torque Shader Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Mission Loading
// The server portion of the client/server mission loading process
//-----------------------------------------------------------------------------

//--------------------------------------------------------------------------
// Loading Phases:
// Phase 1: Transmit Datablocks
//          Transmit targets
// Phase 2: Transmit Ghost Objects
// Phase 3: Start Game
//
// The server invokes the client MissionStartPhase[1-3] function to request
// permission to start each phase.  When a client is ready for a phase,
// it responds with MissionStartPhase[1-3]Ack.

function GameConnection::loadMission(%this)
{
   // Send over the information that will display the server info
   // when we learn it got there, we'll send the data blocks
   %this.currentPhase = 0;
   if (%this.isAIControlled())
   {
      // Cut to the chase...
      %this.onClientEnterGame();
   }
   else
   {
      %matPath = filePath($Server::MissionFile) @ "/" @ fileBase($Server::MissionFile) @ ".mat.json";
      commandToClient(%this, 'MissionStartPhase1', $missionSequence,
         $Server::MissionFile, MissionGroup.musicTrack, isFile(%matPath));
      echo("*** Sending mission load to client: " @ $Server::MissionFile);
   }
}

function serverCmdMissionStartPhase1Ack(%client, %seq)
{
   // Make sure to ignore calls from a previous mission load
   if (%seq != $missionSequence || !$MissionRunning)
      return;
   if (%client.currentPhase != 0)
      return;
   %client.currentPhase = 1;

   // Start with the CRC
   %client.setMissionCRC( $missionCRC );

   // Send over the datablocks...
   // OnDataBlocksDone will get called when have confirmation
   // that they've all been received.
   //%client.transmitDataBlocks($missionSequence);
   
   // JMQ: we assume all datablocks have been exec'ed client side and preloaded already
   %client.onDataBlocksDone($missionSequence);
}

function GameConnection::onDataBlocksDone( %this, %missionSequence )
{
   // Make sure to ignore calls from a previous mission load
   if (%missionSequence != $missionSequence)
      return;
   if (%this.currentPhase != 1)
      return;
   %this.currentPhase = 1.5;

   // On to the next phase
   commandToClient(%this, 'MissionStartPhase2', $missionSequence, $Server::MissionFile);
}

function serverCmdMissionStartPhase2Ack(%client, %seq)
{
   // Make sure to ignore calls from a previous mission load
   if (%seq != $missionSequence || !$MissionRunning)
      return;
   if (%client.currentPhase != 1.5)
      return;
   %client.currentPhase = 2;

   // Update mod paths, this needs to get there before the objects.
   %client.transmitPaths();

   // Start ghosting objects to the client
   %client.activateGhosting();
   
}

function GameConnection::clientWantsGhostAlwaysRetry(%client)
{
   if($MissionRunning)
      %client.activateGhosting();
}

function GameConnection::onGhostAlwaysFailed(%client)
{

}

function GameConnection::onGhostAlwaysObjectsReceived(%client)
{
   // ENABLE_INTERIOR_STENCILS
   //buildMissionShadows();

   // Ready for next phase.
   commandToClient(%client, 'MissionStartPhase3', $missionSequence, $Server::MissionFile);
}

function GameConnection::onFileChunkSent(%this, %file, %count, %max)
{
   if (%this.lastSentFile !$= %file
      || %this.lastChunkStart $= ""
      || %count < %this.lastChunkCount
      || (getSimTime() - %this.lastChunkStart) > 1000)
   {
      %this.lastSentFile = %file;
      %this.lastChunkStartCount = %count;
      %this.lastChunkStart = getSimTime();
   }
   %this.lastChunkCount = %count;
   %rate = (%count - %this.lastChunkStartCount) / ((getSimTime() - %this.lastChunkStart) / 1000);
   %rate = (%rate / 1024) @ "kB/s";

   echo(%this.nameBase SPC %file SPC %rate SPC %count SPC %max);
}

function serverCmdMissionStartPhase3Ack(%client, %seq)
{
   // Make sure to ignore calls from a previous mission load
   if(%seq != $missionSequence || !$MissionRunning)
      return;
   if(%client.currentPhase != 2)
      return;
   %client.currentPhase = 3;

   // Server is ready to drop into the game
   %client.startMission();
   %client.onClientEnterGame();

   if ($EnableFMS)
   {
      commandToClient(%client, 'ShowMission', GameMissionInfo.getCurrentMission().file);
      
      if (isSinglePlayerMode()) // We just loaded the MegaMission
      {
         commandToClient(%client, 'SetCamera');
      }
      else //if (GameMissionInfo.mode $= GameMissionInfo.SPMode) // We just started a singleplayer level
      {
         %client.onClientJoinGame();
      }
   }

   %diff = getRealTime() - $missionLoadStart;
   error("Took " @ %diff / 1000 @ " seconds to load" @ $Server::MissionFile);
}
