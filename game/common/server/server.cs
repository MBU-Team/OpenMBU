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
      %port += 3; %failCount++;
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
   $Server::PrivatePlayerCount = 0;
   $Server::ServerType = %serverType;
   $Server::ArbRegTimeout = 10000;
   
   // Enable quick load here, as there can only be the host at this point
   $Host::QuickLoad = true;  

   if (!$EnableFMS)
   {
      // Setup for multi-player, the network must have been
      // initialized before now.
      if (%serverType $= "MultiPlayer") 
      {
         startMultiplayerMode();
      }
   }

   // Load the mission
   $ServerGroup = new SimGroup(ServerGroup);
   onServerCreated();
   loadMission(%mission, true);
}

function startMultiplayerMode()
{
   if (doesAllowConnections())
   {
      error("startMultiplayerMode(): Multiplayer mode is already active");
      return;
   }
   echo("Starting multiplayer mode");
   $Server::ServerType = "MultiPlayer";
   $Server::Hosting = true;
   $Server::BlockJIP = false;
   
   // Make sure the network port is set to the correct pref.
   portInit($Pref::Server::Port);
   allowConnections(true);

   //if ($pref::Server::DisplayOnMaster !$= "Never" )
   if ($Server::DisplayOnMaster !$= "Never" )
      schedule(0,0,startHeartbeat);
      
   // we are now considered to be connected to a multiplayer server (our own)
   $Client::connectedMultiplayer = true;
   $Game::SPGemHunt = false;
   
   // update the data for the local client connection
   LocalClientConnection.updateClientData($Player::Name, $Player::XBLiveId, XBLiveGetVoiceStatus(), false);
}

function stopMultiplayerMode()
{
//   if (!doesAllowConnections())
//      return;

   if (!$Server::Hosting)
      return;
      
   echo("Stopping multiplayer mode");
   $Server::ServerType = "SinglePlayer";
   $Server::Hosting = false;
   $Server::UsingLobby  = false;
   $Server::BlockJIP = false;
   
   portInit(0);
   allowConnections(false);
   stopHeartbeat();
   
   cancel($Server::ArbSched);
   
   if (isObject(ServerConnection))
      ServerConnection.ready = false;
   
   $Client::connectedMultiplayer = false;
}

//-----------------------------------------------------------------------------

function destroyServer()
{
   if (!$EnableFMS)
   {
      $Server::Hosting = false;
      allowConnections(false);
      stopHeartbeat();
   }
   
   $Server::ServerType = "";
   $missionRunning = false;
   
   // Clean up the game scripts
   onServerDestroyed();

   // Delete all the server objects
   if (isObject(MissionGroup))
      MissionGroup.delete();
   if (isObject(MissionCleanup))
      MissionCleanup.delete();
   if (isObject($ServerGroup))
      $ServerGroup.delete();
   if (isObject(MissionInfo))
      MissionInfo.delete();

   if (!$EnableFMS)
   {
      // Delete all the connections:
      while (ClientGroup.getCount())
      {
         %client = ClientGroup.getObject(0);
         %client.delete();
      }
   }

   $Server::GuidList = "";
   $Server::MissionFile = "";

   // Delete all the data blocks...
   // JMQ: we don't delete datablocks in this version of MB
   //deleteDataBlocks();
   
   // Save any server settings - Not on the XBox -pw
   //echo( "Exporting server prefs..." );
   //export( "$Pref::Server::*", "~/prefs.cs", false );

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

