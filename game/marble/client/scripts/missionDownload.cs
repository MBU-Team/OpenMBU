//-----------------------------------------------------------------------------
// Torque Game Engine
// 
// Copyright (c) 2001 GarageGames.Com
// Portions Copyright (c) 2001 by Sierra Online, Inc.
//-----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// Mission Loading & Mission Info
// The mission loading server handshaking is handled by the
// common/client/missingLoading.cs.  This portion handles the interface
// with the game GUI.
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// Loading Phases:
// Phase 1: Download Datablocks
// Phase 2: Download Ghost Objects
// Phase 3: Scene Lighting
//----------------------------------------------------------------------------
// Phase 1
//----------------------------------------------------------------------------
function onMissionDownloadPhase1(%missionName, %musicTrack, %hasMaterials)
{
   // This callback happens when phase 1 loading starts
   $LoadingDone = false;
   if (%hasMaterials) {
        %matPath = filePath(%missionName) @ "/" @ fileBase(%missionName) @ ".mat.json";
        $Server::MaterialFilePath = %matPath;
        if (!isFile(%matPath))
		    ServerConnection.requestFileDownload(%matPath);
        else
            loadMaterialJson(%matPath);
   }
}

function onPhase1Progress(%progress)
{
   // This callback happens during phase 1 loading
}

function onPhase1Complete()
{
   // Callback on phase1 complete
}

//----------------------------------------------------------------------------
// Phase 2
//----------------------------------------------------------------------------
function onMissionDownloadPhase2()
{
   if ($Client::connectedMultiplayer && !$Server::Hosting && !isObject(MissionCleanup))
   {
      // Mission cleanup group
      new SimGroup( MissionCleanup );
      //$instantGroup = MissionCleanup;
   }
}

function onPhase2Progress(%progress)
{
   
}

function onPhase2Complete()
{
   
}   
//----------------------------------------------------------------------------
// Phase 3
//----------------------------------------------------------------------------
function onMissionDownloadPhase3()
{

}

function onPhase3Progress(%progress)
{

}

function onPhase3Complete()
{

   $lightingMission = false;
}
//----------------------------------------------------------------------------
// Mission loading done!
//----------------------------------------------------------------------------
function onMissionDownloadComplete()
{
   // Client will shortly be dropped into the game, so this is
   // good place for any last minute gui cleanup.
   
   // Do this a bit later
   //$LoadingDone = true;
}

//------------------------------------------------------------------------------
// Before downloading a mission, the server transmits the mission
// information through these messages.
//------------------------------------------------------------------------------
addMessageCallback( 'MsgLoadInfo', handleLoadInfoMessage );
addMessageCallback( 'MsgLoadDescripition', handleLoadDescriptionMessage );
addMessageCallback( 'MsgLoadInfoDone', handleLoadInfoDoneMessage );
//------------------------------------------------------------------------------
function handleLoadInfoMessage( %msgType, %msgString, %mapName ) 
{
   // %JMQ: need to make sure this is handled in the RootGui
   warn("server is loading mission");	
	// Need to pop up the loading gui to display this stuff.
	//Canvas.setContent("LoadingGui");
}
//------------------------------------------------------------------------------
function handleLoadDescriptionMessage( %msgType, %msgString, %line )
{
}
//------------------------------------------------------------------------------
function handleLoadInfoDoneMessage( %msgType, %msgString )
{
   // This will get called after the last description line is sent.
}

