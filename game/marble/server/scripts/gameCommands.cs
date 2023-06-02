//-----------------------------------------------------------------------------
// server command handlers
//-----------------------------------------------------------------------------
function serverCmdSetReadyStatus(%client, %ready)
{
   serverSetClientReadyStatus(%client, %ready);
}

function serverCmdSetUserRating(%client, %levelRating, %overallRating)
{
   %client.levelRating = %levelRating;
   %client.overallRating = %overallRating;
   messageAll('MsgClientUserRatingUpdated', "", %client, %levelRating, %overallRating);
}

//-----------------------------------------------------------------------------
// server admin/game functions
//-----------------------------------------------------------------------------
function serverAreAllPlayersReady()
{
   for(%i = 0; %i < ClientGroup.getCount(); %i++)
   {
      %client = ClientGroup.getObject(%i);
      if (!%client.ready)
         return false;
   }
   return true;
}

function serverGetPublicSlotsUsed()
{
   return $Server::PlayerCount;
}

function serverGetPrivateSlotsUsed()
{
   return $Server::PrivatePlayerCount;
}
 
function serverGetPublicSlotsFree()
{
   return $Pref::Server::MaxPlayers - $Pref::Server::PrivateSlots - serverGetPublicSlotsUsed();
}

function serverGetPrivateSlotsFree()
{
   return $Pref::Server::PrivateSlots - serverGetPrivateSlotsUsed();
}

function serverIsInLobby()
{
   // filter test mode that does not use lobby
   if (!$Server::UsingLobby)
      return false; 
      
   // if there is a game running, assume we are not in the lobby
   return $Server::ServerType $= "MultiPlayer" && !$Game::Running;
}

function serverSetClientReadyStatus(%client, %ready)
{
   if (!isObject(%client))
   {
      error("Server: client doesn't exist, can't set ready status");
      return;
   }
      
   %changed = %client.ready != %ready;
   
   if (%changed)
   {
      %client.ready = %ready;
      messageAllExcept(%client, -1, 'MsgClientReadyStatusChanged', "", %client, 0, %client.ready);
      messageClient(%client, 'MsgClientReadyStatusChanged', "", %client, 1, %client.ready);
   }
   
   %canPlay = !XBLiveIsRanked() || ClientGroup.getCount() > 1;
   if (%canPlay && serverAreAllPlayersReady() && serverIsInLobby())
   {
      // drop out of time players
      if (isDemoLaunch())
         serverDropOutOfTimeClients();
               
      // if we're ranked, we need to do the arbitration registration song and dance
      if (!isPCBuild() && XBLiveIsRanked() && !XBLiveIsArbRegistered())
         serverStartArbRegistration();
      else
         serverStartGameNow();
   }  
}

function serverDropOutOfTimeClients()
{
   for(%i = 0; %i < ClientGroup.getCount(); %i++)
   {
      %client = ClientGroup.getObject(%i);
      if (%client.demoOutOfTime)
         %client.delete("DEMO_OUTOFTIME");
   }
}

function serverSetJIPEnabled(%enabled)
{
   cancel($Game::BlockJIPSchedule);
   
   echo("Server: Join in progress enabled:" SPC %enabled);
   allowConnections(%enabled);
   $Server::BlockJIP = !%enabled;
   
   updateHostedMatchInfo();
}

// ----------------------------------------------------------------------------
// Arbitration
// ----------------------------------------------------------------------------
function serverStartArbRegistration()
{
   echo("Arbitration: starting registration");
   
   // turn off connections until we get back into lobby after the game
   allowConnections(false);
   
   // tell everyone we're starting arbitration (for informational/UI purposes)
   messageAll('MsgArbRegStart', "");
   
   // first mark all clients as not yet responded to the arb register message
   for (%i = 0; %i < ClientGroup.getCount(); %i++)
   {
      ClientGroup.getObject(%i).arbRegAck = false; 
      ClientGroup.getObject(%i).arbRegSuccess = false;
   }
   
   // mark self as acknowledged (we don't send the register message to self)
   LocalClientConnection.arbRegAck = true;
   LocalClientConnection.arbRegSuccess = true;
   
   // send register message
   if (!$Test::ServerDontSendArbReg)
      messageAllExcept(LocalClientConnection.getId(), -1, 'MsgArbReg', "", XBLiveGetSessionNonce());
   
   // clients have a certain amount of time to respond and then we finish the process anyway
   $Server::ArbSched = schedule($Server::ArbRegTimeout, 0, "serverCompleteArbRegistration");     
}

// called when a client completes registration, with bool indicating success or failure
function serverCmdArbRegComplete(%client, %success)
{
   echo("Arbitration: received arb reg complete:" SPC %client SPC %success);
   // store fact that they have responded
   %client.arbRegAck = true;
   %client.arbRegSuccess = %success;
   
   // if the clientf failed to register, we'll need to drop them.  but we can't drop them now 
   // because it will cause an exception since we are still processing the server command.  so
   // schedule an event to do it later
   cancel($Server::ArbRegCheckSched);
   $Server::ArbRegCheckSched = schedule(0, 0, serverCheckPlayerRegistration);
}

function serverCheckPlayerRegistration()
{
   // drop any players who have acked but failed to register
   for(%i = 0; %i < ClientGroup.getCount(); %i++)
   {
      %client = ClientGroup.getObject(%i);
      if (%client.arbRegAck && !%client.arbRegSuccess)
         serverArbDropClient(%client);
   }
      
   // if everybody has responded, server can complete registration
   if (serverAreAllPlayersRegistered())
   {
      echo("Arbitration: all players registered, completing registration");
      serverCompleteArbRegistration();
      return;
   }
   
   // if we get here, not everyone has responded, so we keep waiting until the ArbSched fires
}

// evict a client due to failed registration
function serverArbDropClient(%client)
{
   echo("Arbitration: dropping client due to failed registration:" SPC %client);
   // tell everyone that we are dropping a client due to failed arb registration
   //messageAllExcept(%client, -1, 'MsgArbDropClient', %client, %client.xbLiveId);
   
   %client.delete("ARB_FAILED");
}

// return true if all players have responded to MsgArbReg, false otherwise
function serverAreAllPlayersRegistered()
{
   for(%i = 0; %i < ClientGroup.getCount(); %i++)
   {
      %client = ClientGroup.getObject(%i);
      if (!%client.arbRegAck)
         return false;
   }
   return true;
}

// complete the arbitration process on the server.  called when all clients have responded
// to MsgArbReg or when the timeout expires.  
function serverCompleteArbRegistration()
{
   echo("Arbitration: completing registration");
   // cancel pending arb registration (if any)
   cancel($Server::ArbSched);

   // drop anyone who has failed to ack or register   
   for(%i = 0; %i < ClientGroup.getCount(); %i++)
   {
      %client = ClientGroup.getObject(%i);
      if (!%client.arbRegAck || !%client.arbRegSuccess)
         serverArbDropClient(%client);
   }
   
   // register self
   XBLiveArbRegister(XBLiveGetSessionNonce(), "onServerArbRegComplete();");
}

function onServerArbRegComplete()
{
   %arbRegistered = XBLiveIsArbRegistered();
   if ($Test::ServerFailSelfReg)
      %arbRegistered = false;
      
   echo("Arbitration: server registration complete:" SPC %arbRegistered);
   
   // check global to see if it succeeded
   if (!%arbRegistered)
   {
      // sucky
      serverArbRegFailed();
      return;
   }

   // registration succeeded - get the official player list
   %players = XBLiveGetArbPlayerList();
   
   // various test hooks
   if ($Test::ServerArbNoPlayers)
      %players = "";
   else if ($Test::ServerArbOnlyHost)
      %players = XBLiveGetUserID();
        
   // list is of form "xbliveid TAB xbliveid TAB xbliveid ...";
   // drop any players that failed to register
   for(%i = 0; %i < ClientGroup.getCount(); %i++)
   {
      %client = ClientGroup.getObject(%i);
      
      // if the client's xblive Id contains a tab, just use the first part (backwards compatibility with 
      // ids containing flags)
      %xbLiveId = %client.xbLiveId;
      if (strstr(%client.xbLiveId, "\t") != -1)
         %xbLiveId = getField(%xbLiveId, 0);
         
      // just do a string search to see if the live id is in the player list
      if (%xbLiveId $= "" || strstr(%players, %xbLiveId) == -1)
      {
         // not in the list.
         // if this guy is the local client connection, we have bad thing happening
         if (%client == LocalClientConnection.getId())
         {
            serverArbRegFailed();
            return;
         }
         else
         {
            // bye bye
            serverArbDropClient(%client);
         }
      }
      else
      {
         // got a valid player.  we'll send the player list shortly, but first
         // indicate that this client not yet responded to players list message
         %client.arbPlayersAck = false;
      }
   }
   
   // indicate that local client has responded to players list message (since we don't sent it to 
   // local client)
   LocalClientConnection.arbPlayersAck = true;
   
   // tell all the clients about the official player list.  clients will attempt to drop anyone registered
   // that isn't in this list.  when the drops are complete, clients send back an acknowledgement.  
   // we start the game when all clients have acked or when a certain amount of time has passed.
   if (!$Test::ServerDontSendArbPlayerList)
      messageAllExcept(LocalClientConnection.getId(), -1, 'MsgArbPlayerList', "", %players);
   
   // we start the game unconditionally after a period of time
   cancel($Server::ArbSched);
   $Server::ArbSched = schedule($Server::ArbRegTimeout, 0, "serverStartGameNow");     

   // check to see if we should start now (usually not since we just sent the players message)
   serverCheckArbStart();   
}

function serverArbRegFailed()
{
   // this shouldn't happen normally
   error("Arbitration: server registration failed");
   
   escapeFromGame();
}

function serverCmdArbPlayerListComplete(%client)
{
   echo("Arbitration: received player list complete:" SPC %client);
   // store the fact that they have responded
   %client.arbPlayersAck = true;
   
   serverCheckArbStart();   
}

function serverCheckArbStart()
{
   // if everyone has responded to player list, we can finally start this F@#$ing game!
   %allResponded = true;
   for(%i = 0; %i < ClientGroup.getCount(); %i++)
   {
      %client = ClientGroup.getObject(%i);
      if (!%client.arbPlayersAck)
         %allResponded = false;
   }
   
   if (%allResponded && !$Game::Running)
   {
      echo("Arbitration: all players responded to players list, starting game");
      serverStartGameNow();
   }
}

// ----------------------------------------------------------------------------
function serverStartGameNow()
{
   if ($Game::Running)
   {
      error("serverStartGameNow: a game is already running!");
      return;
   }
   
   cancel($Server::ArbSched);
   
   // Disable quick load for multi player so that the host doesn't get an advantage
   $Host::QuickLoad = (ClientGroup.getCount() == 1);
   
   if ($EnableFMS)
   {
      setSinglePlayerMode(false);
      $Game::renderPreview = true; // we'll turn this off when we get a control object
      registerLights();

      // Create a multiplayer server
      loadMission(GameMissionInfo.getCurrentMission().file, true);
   }
   
   // JMQ: always use 1 round (multiple rounds not supported by shell)
   $Server::NumRounds = 1;
   
   messageAll('MsgClientAllClientsReady', "");

   if (!$EnableFMS)
   {
      // make all clients join (this will also start the game)
      for (%i = 0; %i < ClientGroup.getCount(); %i++)
         ClientGroup.getObject(%i).onClientJoinGame();
   }
}

// ----------------------------------------------------------------------------
function serverCmdSetVoiceStatus(%client, %status)
{
	%client.xbLiveVoice = %status;
	messageAll('MsgClientVoiceStatus', "", %client, %status, %client.xbLiveId);
}

function serverCmdDemoOutOfTime(%client)
{
   %client.demoOutOfTime = true;
   messageAllExcept(%client, -1, 'MsgClientReadyStatusChanged', "", %client, 0, %client.ready, %client.demoOutOfTime);
   messageClient(%client, 'MsgClientReadyStatusChanged', "", %client, 1, %client.ready, %client.demoOutOfTime);
}