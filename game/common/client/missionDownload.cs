//-----------------------------------------------------------------------------
// Torque Shader Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// Mission Loading
// Server download handshaking.  This produces a number of onPhaseX
// calls so the game scripts can update the game's GUI.
//
// Loading Phases:
// Phase 1: Download Datablocks
// Phase 2: Download Ghost Objects
// Phase 3: Scene Lighting
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// Phase 1 
//----------------------------------------------------------------------------

function clientCmdMissionStartPhase1(%seq, %missionName, %musicTrack, %hasMaterials)
{
   $missionDownloadStart = getRealTime();
   
   // These need to come after the cls.
   echo ("*** New Mission: " @ %missionName);
   echo ("*** Phase 1: Download Datablocks & Targets");
   onMissionDownloadPhase1(%missionName, %musicTrack, %hasMaterials);
   commandToServer('MissionStartPhase1Ack', %seq);
}

function onDataBlockObjectReceived(%index, %total)
{
   onPhase1Progress(%index / %total);
}

//----------------------------------------------------------------------------
// Phase 2
//----------------------------------------------------------------------------

function clientCmdMissionStartPhase2(%seq,%missionName)
{
   echo("Mission phase completed in:" SPC (getRealTime() - $missionDownloadStart));
   $missionDownloadStart = getRealTime();
   
   onPhase1Complete();
   echo ("*** Phase 2: Download Ghost Objects");
   purgeResources();
   onMissionDownloadPhase2(%missionName);
   commandToServer('MissionStartPhase2Ack', %seq);
}

function onGhostAlwaysStarted(%ghostCount)
{
   $ghostCount = %ghostCount;
   $ghostsRecvd = 0;
}

function onGhostAlwaysObjectReceived()
{
   $ghostsRecvd++;
   onPhase2Progress($ghostsRecvd / $ghostCount);
}

//----------------------------------------------------------------------------

function onFileChunkReceived(%file, %count, %max)
{
   if ($lastReceivedFile !$= %file
      || $lastChunkStart $= ""
      || %count < $lastChunkCount
      || (getSimTime() - $lastChunkStart) > 1000)
   {
      $lastReceivedFile = %file;
      $lastChunkStartCount = %count;
      $lastChunkStart = getSimTime();
   }
   $lastChunkCount = %count;
   %rate = (%count - $lastChunkStartCount) / ((getSimTime() - $lastChunkStart) / 1000);
   %rate = (%rate / 1024) @ "kB/s";

   echo(%file SPC %rate SPC %count SPC %max);
}

function onFileDownloaded(%file)
{
    if (%file !$= "" && %file $= $Server::MaterialFilePath)
        loadMaterialJson($Server::MaterialFilePath);
}

//----------------------------------------------------------------------------
// Phase 3
//----------------------------------------------------------------------------

function clientCmdMissionStartPhase3(%seq,%missionName)
{
   echo("Mission phase completed in:" SPC (getRealTime() - $missionDownloadStart));
   $missionDownloadStart = getRealTime();
   onPhase2Complete();
   echo ("*** Phase 3: Mission Lighting");
   $MSeq = %seq;
   $Client::MissionFile = %missionName;

   onMissionDownloadPhase3(%missionName);
   registerLights();
   onPhase3Complete();
   // The is also the end of the mission load cycle.
   onMissionDownloadComplete();
   commandToServer('MissionStartPhase3Ack', $MSeq);
   
   echo("Mission phase completed in:" SPC (getRealTime() - $missionDownloadStart));
   $missionDownloadStart = getRealTime();
}

function updateLightingProgress()
{
   onPhase3Progress($SceneLighting::lightingProgress);
   if ($lightingMission)
      $lightingProgressThread = schedule(1, 0, "updateLightingProgress");
}

function sceneLightingComplete()
{  
   echo("Mission lighting done");
   onPhase3Complete();
   
   // The is also the end of the mission load cycle.
   onMissionDownloadComplete();
   commandToServer('MissionStartPhase3Ack', $MSeq);
}

//----------------------------------------------------------------------------
// Helper functions
//----------------------------------------------------------------------------

function connect(%server)
{
   %conn = new GameConnection();
   %conn.connect(%server);
}
