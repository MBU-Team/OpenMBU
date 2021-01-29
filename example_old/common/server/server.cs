//-----------------------------------------------------------------------------
// Torque Shader Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

function portInit(%port)
{
   %failCount = 0;
   while(%failCount < 10 && !setNetPort(%port)) {
      echo("Port init failed on port " @ %port @ " trying next port.");
      %port++; %failCount++;
   }
}
 
function createServer(%serverType, %mission)
{
   if (%mission $= "") {
      error("createServer: mission name unspecified");
      return;
   }

   destroyServer();

   //
   $missionSequence = 0;
   $Server::PlayerCount = 0;
   $Server::ServerType = %serverType;

   // Setup for multi-player, the network must have been
   // initialized before now.
   if (%serverType $= "MultiPlayer") {
      echo("Starting multiplayer mode");

      // Make sure the network port is set to the correct pref.
      portInit($Pref::Server::Port);
      allowConnections(true);

      if ($pref::Net::DisplayOnMaster !$= "Never" )
         schedule(0,0,startHeartbeat);
   }

   // Load the mission
   $ServerGroup = new SimGroup(ServerGroup);
   onServerCreated();
   loadMission(%mission, true);
}


//-----------------------------------------------------------------------------

function destroyServer()
{
   $Server::ServerType = "";
   allowConnections(false);
   stopHeartbeat();
   $missionRunning = false;
   
   // End any running mission
   endMission();
   onServerDestroyed();

   // Delete all the server objects
   if (isObject(MissionGroup))
      MissionGroup.delete();
   if (isObject(MissionCleanup))
      MissionCleanup.delete();
   if (isObject($ServerGroup))
      $ServerGroup.delete();

   // Delete all the connections:
   while (ClientGroup.getCount())
   {
      %client = ClientGroup.getObject(0);
      %client.delete();
   }

   $Server::GuidList = "";

   // Delete all the data blocks...
   deleteDataBlocks();
   
   // Save any server settings
   echo( "Exporting server prefs..." );
   export( "$Pref::Server::*", "~/prefs.cs", false );

   // Dump anything we're not using
   purgeResources();
}


//--------------------------------------------------------------------------

function resetServerDefaults()
{
   echo( "Resetting server defaults..." );
   
   // Override server defaults with prefs:   
   exec( "~/defaults.cs" );
   exec( "~/prefs.cs" );

   loadMission( $Server::MissionFile );
}


//------------------------------------------------------------------------------
// Guid list maintenance functions:
function addToServerGuidList( %guid )
{
   %count = getFieldCount( $Server::GuidList );
   for ( %i = 0; %i < %count; %i++ )
   {
      if ( getField( $Server::GuidList, %i ) == %guid )
         return;
   }

   $Server::GuidList = $Server::GuidList $= "" ? %guid : $Server::GuidList TAB %guid;
}

function removeFromServerGuidList( %guid )
{
   %count = getFieldCount( $Server::GuidList );
   for ( %i = 0; %i < %count; %i++ )
   {
      if ( getField( $Server::GuidList, %i ) == %guid )
      {
         $Server::GuidList = removeField( $Server::GuidList, %i );
         return;
      }
   }

   // Huh, didn't find it.
}


//-----------------------------------------------------------------------------

function onServerInfoQuery()
{
   // When the server is queried for information, the value
   // of this function is returned as the status field of
   // the query packet.  This information is accessible as
   // the ServerInfo::State variable.
   return "Doing Ok";
}

