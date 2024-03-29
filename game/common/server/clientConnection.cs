//-----------------------------------------------------------------------------
// Torque Shader Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

// return true if the current game counts for Multiplayer rankings, false otherwise
function gameCountsForRankings()
{  
   // must be a non demo server, be in multiplayer mode, and have at least two clients connected
   %counts = !isDemoLaunch() && $Server::ServerType $= "MultiPlayer" && ClientGroup.getCount() > 1;
   
   // also check to see if team games have humans on both teams
//   if (%counts && $Game::TeamGame)
//   {
//      %blueHumans = countHumans(BlueTeam);
//      %greenHumans = countHumans(GreenTeam);
//      
//      if (%blueHumans == 0 || %greenHumans == 0)
//         %counts = false;
//   }
   
   return %counts;
}

function updateServerParams()
{
   // build the server params data
   %message = "";
   %message = %message @ $Player::Name @ "\n"; //  host name
   %message = %message @ $Server::ServerType @ "\n"; //  server type
   %message = %message @ $Server::GameModeId @ "\n"; //  game mode
   %message = %message @ $Server::MissionId  @ "\n"; //  mission index
   %message = %message @ gameCountsForRankings() @ "\n"; //  does it count for rankings 
   %message = %message @ serverGetPublicSlotsFree() @ "\n"; //  public slots avail
   %message = %message @ serverGetPublicSlotsUsed() @ "\n"; //  public slots used
   %message = %message @ serverGetPrivateSlotsFree() @ "\n"; //  private slots avail
   %message = %message @ serverGetPrivateSlotsUsed() @ "\n"; //  private slots used
   %message = %message @ (!isPCBuild() && XBLiveIsRanked()) @ "\n"; // ranked?
   %message = %message @ $Server::MissionGuid  @ "\n"; //  mission guid
   %message = %message @ $Server::MissionName  @ "\n"; //  mission name
   
   // update the server parameters on all clients
   messageAll('MsgClientSetServerParams', "", %message);
}

//-----------------------------------------------------------------------------
// This script function is called before a client connection
// is accepted.  Returning "" will accept the connection,
// anything else will be sent back as an error to the client.
// All the connect args are passed also to onConnectRequest
//
function GameConnection::onConnectRequest( %client, %netAddress, %name, %xbLiveId, %xbLiveVoice, %invited, %isDemoLaunch)
{
   echo("Connect request from: " @ %netAddress);

   %flag = $Server::PlayerCount >= ($pref::Server::MaxPlayers - $Pref::Server::PrivateSlots);

   if (%invited $= "" && %flag)
      return "CR_SERVERFULL";

   if ((%invited !$= "" && %invited != 0) && $Server::PrivatePlayerCount >= $Pref::Server::PrivateSlots && %flag)
      return "CR_SERVERFULL";

   if (%invited $= "" && $Server::IsPrivate)
      return "CR_SERVERFULL";


   %banEntry = $banlist[%xbLiveId];
   if (%banEntry !$= "")
   {
      %startTime = getField(%banEntry, 0);
      %duration = getField(%banEntry, 1);
      %now = getRealTime();
      
      if (%now < %startTime + %duration)
         return "CR_YOUAREBANNED";
   }
   
   %demoSame = %isDemoLaunch == isDemoLaunch();
   //echo("   Demo status" SPC %isDemoLaunch SPC isDemoLaunch());
   if (!isPCBuild() && !%demoSame)
   {
      // demo status mismatch
      
      // if the client is a demo, then we are not.  tell them to upgrade.
      if (%isDemoLaunch)
      {
         echo("   Connect request rejected: client is a demo and we are not");
         return "CR_DEMOUPGRADE";
      }
      else // otherwise we are a demo, tell them they cannot play
      {
         echo("   Connect request rejected: client is not a demo and we are");
         return "CR_DEMOREJECT";
      }
   }

   if (%invited !$= "" && %invited !$= $Server::InviteCode)
   {
        return "CHR_PASSWORD";
   }
   
   // kick any players with this xblive id
   //if (%xbLiveId !$= "") // TODO: uncomment when this is set up in the engine
   //{
      //%count = ClientGroup.getCount();
      //for (%cl = 0; %cl < %count; %cl++) {
         //%other = ClientGroup.getObject(%cl);
         //if (%other.xbLiveId $= %xbLiveId)
         //{
            //error("@@@@@ Duplicate xblive Id detected: kicking previous player");
            //%other.delete("CR_DUPID");
         //}
      //}
   //}
   
   return "";
}

function onDiscordJoinRequest(%userId, %username, %avatar)
{
    if (!$Server::Hosting) {
        XBLiveRespondJoinRequest(%userId, 0);
        return;
    }

    if ($Server::PlayerCount >= ($pref::Server::MaxPlayers - $Pref::Server::PrivateSlots) || $Server::IsPrivate) {
        XBLiveRespondJoinRequest(%userId, 0);
        return;
    }

    JoinGameInviteDlg::show(%userId, %username, %avatar);
}

// populate client data fields, send join messages and server param updates, update xblive records.
// this is pulled out from onConnect() so that the host can update his data after he starts a server
// without needing to reconnect.
function GameConnection::updateClientData(%client, %name, %xbLiveId, %xbLiveVoice, %invited)
{
   // make sure %client is an Id, in case someone called us using name "LocalClientConnection"
   if (!isObject(%client))
   {
      error("Cannot update a client that doesn't exist");
      return;
   }
   %client = %client.getId();
   
   // determine if we already populated data for this client or not
   %newClient = !%client.dataInitialized;
   %client.dataInitialized = true;
   
   // set various parameters if this is the host
   %isHost = %client.getAddress() $= "local";
   %client.isAdmin = %isHost;
   %client.isSuperAdmin = %isHost;
   %client.isHost = %isHost;

   // Save client preferences on the connection object for later use.
   %client.setPlayerName(%name);
   %client.score = 0;
   %client.xbLiveId = %xbLiveId;
   %client.xbLiveSkill = 0;
   %client.xbLiveVoice = %xbLiveVoice;
   %client.invited = %invited;
   %client.ready = false;
   if (!%client.isAIControlled())
      %client.address = %client.getXnAddr();
   else
      %client.address = "";
   
   // if we have not yet added this client to our count, update it - note that changing invite
   // status for an existing client does not properly change the count.  so don't do that.
   // (it would be possible to update the count by comparing the old and new invite status)
   if (%newClient)
   {  
      // note the time that this client joined 
      %client.joinTime = getSimTime();
      %client.joinInProgress = $Game::State $= "play";
   
      // the client was invited and we have a free private slot for them, stick them into it
      if (%client.invited && $Server::PrivatePlayerCount < $Pref::Server::PrivateSlots)
      {
         %client.usingPrivateSlot = true;
         $Server::PrivatePlayerCount++;
      }
      else
      {
         $Server::PlayerCount++;
      }
   }

   // Inform all clients of server global parameters - need to do this early, so that the information is available
   // in client join processing.  We inform all clients because some parameters (e.g. gameCountsForRankings) can change
   // depending on the number of clients in the game.
   updateServerParams();

   // If new client, inform the client of all the other clients
   if (%newClient)
   {
      %count = ClientGroup.getCount();
      for (%cl = 0; %cl < %count; %cl++) {
         %other = ClientGroup.getObject(%cl);
         if ((%other != %client)) {
            %joinData = buildClientJoinData(%other);
            messageClient(%client, 'MsgClientJoin', "", %other.name, %joinData);
         }
      }
   }

   // Inform the client we've joined up - note the "true" parameter on the end of the 
   // join message which tells the client that it is he who is joining (so that 
   // the client side code can differentate local client from other dudes)
   %joinData = buildClientJoinData(%client);
   
   messageClient(%client, 'MsgClientJoin', "", %client.name, %joinData, true);
   // Inform all the other clients of the new guy
   messageAllExcept(%client, -1, 'MsgClientJoin', "", %client.name, %joinData);
   
   // if a game is in progress, inform the client of everyone's ratings
   if ($Game::State !$= "wait")
   {
      %count = ClientGroup.getCount();
      for (%cl = 0; %cl < %count; %cl++) {
         %other = ClientGroup.getObject(%cl);
         if ((%other != %client)) {
            messageClient(%client, 'MsgClientUserRatingUpdated', "", %other, %other.levelRating, %other.overallRating);
         }
      }
   }

    commandToClient(%client, 'SetPartyId', XBLiveGetPartyId());
         
   // update the hosted xbox match
	updateHostedMatchInfo();
}

//-----------------------------------------------------------------------------
// This script function is the first called on a client accept
//
function GameConnection::onConnect(%client, %name, %xbLiveId, %xbLiveVoice, %invited)
{
   // set flag on client indicating that he connected succesfully
   %client.connected = true;
   
   // Send down the connection error info, the client is
   // responsible for displaying this message if a connection
   // error occures.
   messageClient(%client,'MsgConnectionError',"",$Pref::Server::ConnectionError);

   // Send mission information to the client
   sendLoadInfoToClient( %client );

   // Simulated client lag for testing...
   // %client.setSimulatedNetParams(0.1, 30);

   $instantGroup = MissionCleanup;
   echo("CADD: " @ %client @ " " @ %client.getAddress());

   // most of the meat happens in this function
   %client.updateClientData(%name, %xbLiveId, %xbLiveVoice, %invited);
   
   // If the mission is running, go ahead download it to the client
   if ($missionRunning)
      %client.loadMission();
}

// if you change this function, be sure to change client side function that unpacks this message
// (see client playerlist.cs file)
function buildClientJoinData(%client)
{
	%message = "";		
	
	// if this is an AI, fake some of the data
	if (%client.isAIControlled())
	{
      %message = %message @ %client @ "\n";
      %message = %message @ 1 @ "\n";
      %message = %message @ 0 @ "\n";
      %message = %message @ 0 @ "\n";
      %message = %message @ "" @ "\n";
      %message = %message @ 0 @ "\n";
      %message = %message @ 2 @ "\n"; // 2 = no communicator, although it shouldn't matter for bots
      %message = %message @ 0 @ "\n";
      %message = %message @ 0 @ "\n";
      %message = %message @ 1 @ "\n";
      %message = %message @ %client.score @ "\n";
      %message = %message @ 0 @ "\n";
      %message = %message @ 0 @ "\n";
      %message = %message @ 0 @ "\n";
      %message = %message @ 0 @ "\n";
   }
   else
   {
      %message = %message @ %client @ "\n";
      %message = %message @ %client.isAIControlled() @ "\n";
      %message = %message @ %client.isAdmin @ "\n";
      %message = %message @ %client.isSuperAdmin @ "\n";
      %message = %message @ %client.xbLiveId @ "\n";
      %message = %message @ %client.xbLiveSkill @ "\n";
      %message = %message @ %client.xbLiveVoice @ "\n";
      %message = %message @ %client.address @ "\n";
      %message = %message @ %client.rating @ "\n";
      %message = %message @ %client.ready @ "\n";
      %message = %message @ %client.score @ "\n";
      %message = %message @ %client.invited @ "\n";
      %message = %message @ %client.demoOutOfTime @ "\n";
      %message = %message @ %client.joinTime @ "\n";
      %message = %message @ %client.joinInProgress @ "\n";
   }
   
   return %message;
}

//-----------------------------------------------------------------------------
// A player's name could be obtained from the auth server, but for
// now we use the one passed from the client.
// %realName = getField( %authInfo, 0 );
//
function GameConnection::setPlayerName(%client,%name)
{
   %client.sendGuid = 0;
   %client.name = "";

   // Minimum length requirements -- REMOVED FOR 360 -pw
   %name = stripTrailingSpaces( strToPlayerName( %name ) );
   //if ( strlen( %name ) < 3 )
   //   %name = "Poser";

   // Make sure the alias is unique, we'll hit something eventually
   if (!isNameUnique(%name))
   {
      %isUnique = false;
      for (%suffix = 1; !%isUnique; %suffix++)  {
         %nameTry = %name @ "." @ %suffix;
         %isUnique = isNameUnique(%nameTry);
      }
      %name = %nameTry;
   }

   // Tag the name with the "smurf" color:
   %client.nameBase = %name;
   %client.name = addTaggedString(%name);
}

function isNameUnique(%name)
{
   %count = ClientGroup.getCount();
   for ( %i = 0; %i < %count; %i++ )
   {
      %test = ClientGroup.getObject( %i );
      if (%test.name $= "")
         continue;
      %rawName = stripChars( detag( getTaggedString( %test.name ) ), "\cp\co\c6\c7\c8\c9" );
      if ( strcmp( %name, %rawName ) == 0 )
         return false;
   }
   return true;
}

//-----------------------------------------------------------------------------
// This function is called when a client drops for any reason
//
function GameConnection::onDrop(%client, %reason)
{
   // return if the client never finished connecting (i.e., they were rejected because 
   // server was full).  its ok to do this because none of the onConnect() logic will 
   // have been executed - if it was, this flag would be true.
   if (!%client.connected)
   {
      echo("onDrop received for unconnected client; ignoring");
      return;
   }
      
   %client.onClientLeaveGame();
   
   // removeFromServerGuidList( %client.guid );
   messageAllExcept(%client, -1, 'MsgClientDrop', "", %client.name, %client, %client.xbLiveId);

   removeTaggedString(%client.name);
   
   echo("CDROP: " @ %client @ " " @ %client.getAddress());
   
   
   
   if (%client.usingPrivateSlot)
      $Server::PrivatePlayerCount--;
   else
      $Server::PlayerCount--;

   // Reset the server if everyone has left the game
   if ($Server::PlayerCount == 0 && $Server::Dedicated)
      schedule(0, 0, "resetServerDefaults");
      
   // If everyone has left game, destroy it
   if($Server::PlayerCount == 0)
      destroyGame();
      
   // schedule update to server params (must schedule it because we are
   // not yet dropped)
   schedule(0,0,"updateServerParams");
   
 	// update the hosted xbox match
	updateHostedMatchInfo();
}


//-----------------------------------------------------------------------------

function GameConnection::startMission(%this)
{
   // Inform the client the mission starting
   commandToClient(%this, 'MissionStart', $missionSequence);
}


function GameConnection::endMission(%this)
{
   // Inform the client the mission is done
   commandToClient(%this, 'MissionEnd', $missionSequence);
}


//--------------------------------------------------------------------------
// Sync the clock on the client.

function GameConnection::syncClock(%client, %time)
{
   commandToClient(%client, 'syncClock', %time);
}


//--------------------------------------------------------------------------
// Update all the clients with the new score

function updateHostedMatchInfo()
{  
   // do nothing if we aren't hosting an MP game
   if (!$Server::Hosting)
      return;
      
   %publicUsed = serverGetPublicSlotsUsed();
   %inviteUsed = serverGetPrivateSlotsUsed();
   
   %publicFree = serverGetPublicSlotsFree(); 
   %inviteFree = serverGetPrivateSlotsFree();
     
   %status = 0; // this field is unused
   
   // if we are out of demo time, disable join in progress
   %disableJoinInProgress = false;
   if (isDemoLaunch() && $Demo::TimeRemaining == 0)
   {
      echo("updateHostedMatchInfo: no demo time remaining, disabling join in progress");
      %disableJoinInProgress = true;
   }
   
   if ($Server::BlockJIP)
   {
      echo("$Server::BlockJIP is true, disabling join in progress");
      %disableJoinInProgress = true;
   }

   error("@@@@@ Update Hosted Match:" SPC %publicUsed SPC %publicFree SPC %inviteUsed SPC %inviteFree SPC %status SPC $Server::GameModeId SPC $Server::MissionId SPC %disableJoinInProgress);
   XBLiveUpdateHostedMatch(%publicUsed, %publicFree, %inviteUsed, %inviteFree, %status, $Server::GameModeId, $Server::MissionId, %disableJoinInProgress);
}
