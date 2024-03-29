//-----------------------------------------------------------------------------
// Torque Game Engine
//
// Copyright (c) 2001 GarageGames.Com
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Hook into the client update messages to maintain our player list
// and scoreboard.
//-----------------------------------------------------------------------------

addMessageCallback('MsgClientJoin', handleClientJoin);
addMessageCallback('MsgClientDrop', handleClientDrop);
addMessageCallback('MsgClientScoreChanged', handleClientScoreChanged);
addMessageCallback('MsgClientSetServerParams', handleClientSetServerParams);
addMessageCallback('MsgClientVoiceStatus', handleVoiceStatus);
addMessageCallback('MsgClientUserRatingUpdated', handleUserRatingUpdated);
addMessageCallback('MsgClientSettingsChanged', handleSettingsChanged);
addMessageCallback('MsgClientReadyStatusChanged', handleReadyStatusChanged);
addMessageCallback('MsgAdminForce', handleAdminForce);
addMessageCallback('MsgClientKilled', handleClientKilled);
addMessageCallback('MsgClientAllClientsReady', handleAllClientsReady);
addMessageCallback('MsgClientUpdateLobbyStatus', handleUpdateLobbyStatus);
addMessageCallback('MsgArbRegStart', handleArbRegStart);
addMessageCallback('MsgArbReg', handleArbReg);
addMessageCallback('MsgArbPlayerList', handleArbPlayerList);
addMessageCallback('MsgClientVoiceStatus', handleVoiceStatus);
addMessageCallback('MsgMPGameOver', handleMPGameOver);

//-----------------------------------------------------------------------------
// This message is sent by the server to inform the client of global server 
// parameters, such as whether the server is multiplayer or not.  Parameters
// can vary during server lifetime, so clients can expect to receive this 
// message fairly frequently.
//
// Note that these paremeters
// are stored on the client ServerConnection object, which means that
// bot clients will not have access to them.
function handleClientSetServerParams(%msgType, %msgString, %message)
{
   // first check server type
   %serverType = getRecord(%message, 1);
   // store a global indicating if we are connected to a multiplayer server.  do this instead of using a property
   // on the ServerConnection, so that it hangs around even if the server connection is deleted (e.g. if we lose 
   // it for some reason)
   %isMultiplayer = %serverType $= "MultiPlayer";
   //$Client::connectedMultiplayer = %serverType $= "MultiPlayer";
      
   if (isObject(ServerConnection))
   {
      // unpack server params data
      ServerConnection.hostName = getRecord(%message, 0);
      ServerConnection.isMultiplayer = %isMultiplayer;
      ServerConnection.gameModeId = getRecord(%message, 2);
      ServerConnection.missionId = getRecord(%message, 3);
      ServerConnection.gameCounts = getRecord(%message, 4);
      ServerConnection.pubSlotsFree = getRecord(%message, 5);
      ServerConnection.pubSlotsUsed = getRecord(%message, 6);
      ServerConnection.priSlotsFree = getRecord(%message, 7);
      ServerConnection.priSlotsUsed = getRecord(%message, 8);
      ServerConnection.isRanked = getRecord(%message, 9);
      ServerConnection.guid = getRecord(%message, 10);
      ServerConnection.missionName = getRecord(%message, 11);
            
      // set flag indicating that server params are present
      ServerConnection.hasParams = true;

      echo("@@@@@ ServerParams:" SPC ServerConnection.isMultiplayer SPC ServerConnection.gameModeId SPC 
         ServerConnection.missionId SPC ServerConnection.gameCounts);      
      
      // UI stuff 
      LobbyGui.updateHostInfo();
      
      if (%isMultiplayer)
      {
         XBLiveSetGameModeContext(ServerConnection.gameModeId);
      }
   }
}

//-----------------------------------------------------------------------------
function handleClientJoin(%msgType, %msgString, %clientName, %joinData, %isMe)
{
   // unpack join data.  note that join data should not contain any tagged strings
   // (these should be passed separately).
   %recordIndex = -1; // no difference between ++foo and foo++ in TS, so we need to start at -1
   %clientId = getRecord(%joinData, %recordIndex++);
   %isAI = getRecord(%joinData, %recordIndex++);
   %isAdmin = getRecord(%joinData, %recordIndex++);
   %isSuperAdmin = getRecord(%joinData, %recordIndex++);
   %xbLiveId = getRecord(%joinData, %recordIndex++);
   %xbLiveSkill = getRecord(%joinData, %recordIndex++);
   %xbLiveVoice = getRecord(%joinData, %recordIndex++);
   %address = getRecord(%joinData, %recordIndex++);
   %rating = getRecord(%joinData, %recordIndex++);
   %ready = getRecord(%joinData, %recordIndex++);
   %score = getRecord(%joinData, %recordIndex++);
   %invited = getRecord(%joinData, %recordIndex++);
   %demoOutOfTime = getRecord(%joinData, %recordIndex++);
   %joinTime = getRecord(%joinData, %recordIndex++);
   %joinInProgress = getRecord(%joinData, %recordIndex++);
   
   //error(%joinData);
   error("@@@@@ handleClientJoin parsed:" SPC detag(%clientName) SPC %clientId SPC %isAI SPC %isAdmin SPC %isSuperAdmin SPC %xbLiveId SPC %xbLiveSkill SPC %xbLiveVoice SPC %address SPC %rating);
   error("@@@@@    isMe:" SPC %isMe);
   
   %name = detag(%clientName);
   
   if (%isMe)
   {
      ServerConnection.selfJoined = true;
      $Player::ClientId = %clientId;

      resetClientGracePeroid();
   }
   
   if (%isAdmin)
   {
      $Host::ClientId = %clientId;
      $Host::XBLiveId = %xbLiveId;
   }
   
   if (!%isAI)
   {
      LobbyGui.update(%clientId,detag(%clientName),%xbLiveId,%xbLiveSkill,
         %xbLiveVoice,%address,%rating,%ready,%invited,%demoOutOfTime);
   }
   
   if (ServerConnection.isMultiplayer && %joinInProgress)
      clientAddJoinInProgressClient(%clientId, %xbLiveId, %joinTime);
   
   if (ServerConnection.isMultiplayer && !%isMe && ServerConnection.selfJoined)
   {
      // spew about new player
      echo(detag(%clientName) SPC "joined the game");
      sfxPlay(PlayerJoinSfx);
      %displayName = detag(%clientName);
      if ($pref::Lobby::StreamerMode)
      {
          %displayName = getSubStr(%displayName, 0, 1) @ "...";
      }
      %msg = avar($Text::Msg::PlayerJoin, %displayName);
      addChatLine(%msg);
   }
   
   PlayerListGui.update(%clientId,detag(%clientName),%isSuperAdmin,
      %isAdmin,%isAI,0,0,"");
      
   if (%isMe) PlayerListGui.initShortPlayerList(%clientId);
}

function handleClientDrop(%msgType, %msgString, %clientName, %clientId, %xbLiveId)
{
   if (ServerConnection.isMultiplayer && XBLiveIsStatsSessionActive())
   {
      // keep track of people who drop when a game is active
 
      // This should now call clientAddDroppedClient ONLY if it is a ranked game
      // or if the client is in his grace peroid, or if this is NOT our drop message -pw
      if( $Client::isInGracePeroid || XBLiveIsRanked() || $Player::ClientId != %clientId )
         clientAddDroppedClient(%clientId, %xbLiveId);
      else if( !$Client::isInGracePeroid && $Player::ClientId == %clientId && $Client::willfullDisconnect )
         zeroMyClientScoreCuzICheat();
   }

   if (XBLiveIsRanked() && ServerConnection.gameState !$= "wait" && XBLiveIsStatsSessionActive())
   {
      // write scores now to catch this dropped player
      clientWriteMultiplayerScores();      
   }
   
   PlayerListGui.remove(%clientId);
   LobbyGui.remove(%clientId,%xbLiveId);
   
   if (ServerConnection.isMultiplayer && $Player::ClientId !$= %clientId)
   {
      // spew about dropping player
      echo(detag(%clientName) SPC "left the game");
      sfxPlay(PlayerDropSfx);
      %displayName = detag(%clientName);
      if ($pref::Lobby::StreamerMode)
      {
          %displayName = getSubStr(%displayName, 0, 1) @ "...";
      }
      %msg = avar($Text::Msg::PlayerDrop, %displayName);
      addChatLine(%msg);
   }
   
}

function handleMPGameOver(%msgType, %msgString, %tied, %leaderName, %leaderPoints)
{
   %name = detag(%leaderName);
   if ($pref::Lobby::StreamerMode)
   {
       %name = getSubStr(%name, 0, 1) @ "...";
   }
   
   %msg = "";
   if (%tied)
   {
      %msg = $Text::Msg::Tied;
   }
   else
   {
      if (%leaderPoints == 1)
         %msg = $Text::Msg::WonMP1;
      else
         %msg = $Text::Msg::WonMPN;
   }
     
   %msg = avar(%msg, %name, %leaderPoints);
   addChatLine(%msg);
}      

function zeroMyClientScoreCuzICheat()
{
   echo( "[Client " @ $Player::ClientId @ "]: ZEROING SCORE FOR CALCULATIONS CUZ I AM A JERK WHO QUITS EARLY!" );
   PlayerListGui.updateScore( $Player::ClientId, 0, 0 );
}

function handleClientScoreChanged(%msgType, %msgString, %clientId, %isMe, %newScore, %oldScore)
{
//    if (%isMe)
//       PlayGui.setPoints(%newScore);
   %delta = %newScore - %oldScore;
   if(%isMe && %delta != 0)
   {
      RootScoreCountHud.addScore(%newScore - %oldScore, RootScoreCountHud.pointColor[%delta], "0 0 0 1");
   }
   PlayerListGui.updateScore(%clientId,%newScore,%newScore);
}

function handleVoiceStatus(%msgType, %msgString, %client, %status, %xbLiveId)
{      
   LobbyGui.updateVoiceStatus(%client, %status, %xbLiveId);
}

function handleUserRatingUpdated(%msgType, %msgString, %client, %levelRating, %overallRating)
{
   clientAddUserRating(%client, %levelRating, %overallRating);
}

function handleSettingsChanged(%msgType, %msgString, %client, %isMe, %team, %tank)
{
   LobbyGui.updateClientSettings(%client, %team, %tank);
   PlayerListGui.updateClientTeam(%client, %team);
   if (%isMe)
      setLocalPlayerTeam(%team);
}

function handleReadyStatusChanged(%msgType, %msgString, %client, %isMe, %ready, %demoOutOfTime)
{
   if (%isMe)
   {
      ServerConnection.ready = %ready;
      ServerConnection.demoOutOfTime = %demoOutOfTime;
   }
      
   LobbyGui.updateReadyStatus(%client, %isMe, %ready, %demoOutOfTime);
}

function handleAdminForce(%msgType, %msgString, %action, %clientName)
{
   if (%action $= "kick")
      LobbyGui.setMessage(avar($Text::E37, detag(%clientName)));
   else if (%action $= "swapteam")
      LobbyGui.setMessage(avar($Text::E38, detag(%clientName)));
}

function handleAllClientsReady(%msgType, %msgString)
{
   // activate mission load gui so that we can play
   // but handle case where we backed out to create game gui
   if (LobbyGui.isAwake())
      RootGui.setContent(MissionLoadingGui);        
}

function handleUpdateLobbyStatus(%msgType, %msgString, %inLobby)
{
   if (!$Client::connectedMultiplayer)
      // don't care
      return;
      
   registerLights();

   %wasInLobby = ServerConnection.inLobby;
   ServerConnection.inLobby = %inLobby;
   // also set a global so that the client determine whether to save stats when the 
   // connection was dropped.  
   $Client::inLobby = ServerConnection.inLobby;
   
   // force unpause if loading lobby
   if (ServerConnection.inLobby)
      resumeGame();
      
   // if we aren't all ready in the lobby, clear our ready status
   if (ServerConnection.inLobby)
   {
      // clear ready status when entering the Lobby for the first time
      if (!%wasInLobby && clientIsReady())
         // now we aren't
         clientSetReadyStatus(false);
         
      if (!LobbyGui.isAwake() && !EndGameGui.isAwake())
         RootGui.setContent(LobbyGui);
   }
   else if (!PlayGui.isAwake() && isObject(ServerConnection.getControlObject()))
   {
      RootGui.setContent(PlayGui);
   }
   else if (!MissionLoadingGui.isAwake())
   {
      RootGui.setContent(MissionLoadingGui);
   }
}

function setLocalPlayerTeam(%teamName, %msg)
{
   echo("Changing local player team to:" SPC %teamName);
   $Player::TeamName = %teamName;
      
   if (%teamName $= "BlueTeam")
      %msg2 = "<color:1F5CFF>" @ $Text::G25;
   else if (%teamName $= "GreenTeam")
      %msg2 = "<color:3BB800>" @ $Text::G26;

   if (%msg !$= "") playGui.joinMsg = %msg;
   if (%msg2 !$= "") playGui.joinMsg2 = %msg2;
   
   playGui.setTeamColor(%teamName);
}

function XBLiveCommunicatorTalking(%id, %status)
{
   LobbyGui.updateCommunicatorTalking(%id, %status);
   //if (%status)
   //   %msg = "is talking";
   //else
   //   %msg = "is not talking";
      
   //error("Player" SPC %id SPC %msg);
}

// arbitration
function handleArbRegStart(%msgType, %msgString)
{
   RootGui.setContent(MissionLoadingGui);        
   //RootGui.removeContent();
   //RootGui.setCenterText($Text::TestingNetwork);
   //RootGui.displayLoadingAnimation( "center" );
}

function handleArbReg(%msgType, %msgString, %nonce)
{
   if (!$Test::ClientIgnoreArbReg)
      XBLiveArbRegister(%nonce, "onClientArbRegComplete();");
}

function onClientArbRegComplete()
{
   if (!$Test::ClientFailArbReg)
      commandToServer('ArbRegComplete', XBLiveIsArbRegistered());
   else
      commandToServer('ArbRegComplete', false);
}

function handleArbPlayerList(%msgType, %msgString, %players)
{
   $XBLive::ArbPlayerList = %players;
   
   // the server should have sent us a bunch of drops for all the players not in the list.  those drops
   // should now be pending or complete.  install a leave callback so that we can acknowledge the server
   // when the drops are done.  if they are already done this callback will fire immediately
   if (!$Test::ClientIgnoreArbPlayers)
      XBLiveSetLeaveCompleteCallback("onClientLeavesComplete();");
}

function onClientLeavesComplete()
{
   if (!$Client::connectedMultiplayer)
      // guess we were dropped! 
      return;
      
   echo("Arbitration: client sending player list ack");
   // send player list ack
   commandToServer('ArbPlayerListComplete');
}

// voice
function handleVoiceStatus(%msgType, %msgString, %client, %status, %xbLiveId)
{      
   LobbyGui.updateVoiceStatus(%client, %status, %xbLiveId);
}
