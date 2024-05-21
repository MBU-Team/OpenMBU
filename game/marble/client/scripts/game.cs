//-----------------------------------------------------------------------------
// Torque Game Engine
// 
// Copyright (c) 2001 GarageGames.Com
//-----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// Game events sent from the server
//----------------------------------------------------------------------------
function clientCmdShowMission(%missionName)
{
   %missionIndex = GameMissionInfo.findIndexByPath(%missionName);

   $Client_CMD_Select_Mission_Hack = true;
   GameMissionInfo.selectMission(%missionIndex);
   $Client_CMD_Select_Mission_Hack = false;
}

function clientCmdSetCamera()
{
   // Make sure we have a $previewCamera
   if (!isObject($previewCamera))
   {
      $previewCamera = new Camera() {
         datablock = Observer;
      };
      
      // Make sure our $previewCamera doesn't end up in MissionCleanup
      if (isObject(MegaServerGroup))
         MegaServerGroup.add( $previewCamera );
      else
         ServerGroup.add( $previewCamera );
   }
      
   ServerConnection.setControlObject($previewCamera);
}

function GameConnection::switchedSinglePlayerMode(%onoroff)
{
   ServerConnection.setControlObject($previewCamera);
   registerLights();
}

function clientCmdGameStart()
{
   $timeScale = 1.0;
   
   if($playingDemo)
      return;
      
   PlayerListGui.zeroScores();

   // Map Pack Achievement code:
   $UserAchievements::MPGotABlueGem = 0;
}

function clientCmdGameEnd()
{
   if($playingDemo)
      return;
      
   // Restore simulation time
   $timeScale = 1.0;
   //GameMissionInfo
   // Get out of the game.
   // Additional check here is to make sure that, when this executes, the same
   // user is signed in as was when they finished the leve, this is for
   // bug # 15401 -pw
   
   if( !$Client::connectedMultiplayer && $GameEndUserName $= XBLiveGetUserName() )
   {
      if ($testLevel)
      {
         restartLevel();
      } else
      {
         if (!UpsellGui.isAwake()) // wait user gets off the upsell gui 
            RootGui.setContent(GameEndGui);
      }
   }
      
   // Copy the current player scores from the player list into the
   // end game gui (bit of a hack for now).
//   EndGamePlayerList.clear();
//   for (%i = 0; %i < PlayerListGuiList.rowCount(); %i++)
//   {
//      %text = PlayerListGuiList.getRowText(%i);
//      %id = PlayerListGuiList.getRowId(%i);
//      EndGamePlayerList.addRow(%id,%text);
//   }
//   EndGamePlayerList.sortNumerical(2,false);

   // JMQ: cycle back to wait mode for prototype
   //commandToServer('SetWaitState');

   // Map Pack Achievement code:
   $UserAchievements::MPGotABlueGem = 0;
   return;
}

// Map Pack Achievement code:
function clientCmdPickedUpABlueGem()
{
   $UserAchievements::MPGotABlueGem = 1;
}

function clientCmdSetGemCount(%gems,%maxGems)
{
   PlayGui.setGemCount(%gems);
   PlayGui.setMaxGems(%maxGems);
}

function clientCmdSetPoints(%clientid, %points)
{
   PlayGui.setPoints(%points);
}

function clientCmdSetGameDuration(%duration, %playStart)
{
   PlayGui.gameDuration = %duration;
   PlayGui.setTimer(0);
         
   if (%playStart > 0)
      $Client::LastGamePlayStart = %playStart;
}

function clientCmdSetEndTime(%playEnd)
{
   $Client::LastGamePlayEnd = %playEnd;
}

function clientCmdSetPowerup(%shapeFile)
{
   PlayGui.setPowerUp(%shapeFile);
}

function clientCmdSetMessage(%message,%time)
{
   PlayGui.setMessage(%message,%time);
   
   if( %message $= "ready" && isObject($Game::StartPad) )
   {
      $Game::StartPad.stopThread( 0 );
      $Game::StartPad.playThread( 0, "start" );
   }
}

function clientCmdSetHelpLine(%helpLine,%beep)
{
   addHelpLine(%helpLine,%beep);
}

function clientCmdAddStartMessage(%message,%isOptional)
{
   if (!%isOptional || $pref::displayMPHelpText)
      addStartMessage(%message);
}

function clientCmdSetTimer(%cmd,%time)
{
   switch$ (%cmd)
   {
      case "reset":
         PlayGui.resetTimer();
      case "start":
         PlayGui.startTimer();
      case "stop":
         PlayGui.stopTimer();
      case "set":
         PlayGui.setTimer(%time);
   }
}

function clientCmdCheckpointHit( %sequenceNum )
{
   // If we want to do something with this, here it is
}

function clientAddUserRating(%clientId, %levelRating, %overallRating)
{
   if (!isObject(StatsUserRatings_Level))
   {
      new GuiTextListCtrl(StatsUserRatings_Level);
      RootGroup.add(StatsUserRatings_Level);
   }
   if (!isObject(StatsUserRatings_Overall))
   {
      new GuiTextListCtrl(StatsUserRatings_Overall);
      RootGroup.add(StatsUserRatings_Overall);
   }
   echo("Adding user rating for client:" SPC %clientId SPC %levelRating SPC %overallRating); 
   //StatsUserRatings_Level.setRowById(%clientId, %levelRating);
   StatsUserRatings_Overall.setRowById(%clientId, %overallRating);
}

function clientAddJoinInProgressClient(%clientId, %xbLiveId, %joinTime)
{
   if (!isObject(StatsJoinInProgressClients))
   {
      new GuiTextListCtrl(StatsJoinInProgressClients);
      RootGroup.add(StatsJoinInProgressClients);
   }
   // get rid of flags on xbliveid
   %xbLiveId = getField(%xbLiveId, 0);   
   echo("Adding JoinInProgress client for stats:" SPC %clientId SPC %xbLiveId SPC %joinTime); 
   StatsJoinInProgressClients.setRowById(%clientId, %xbLiveId SPC %joinTime);
}

function clientAddDroppedClient(%clientId, %xbLiveId)
{
   if (!isObject(StatsDroppedClients))
   {
      new GuiTextListCtrl(StatsDroppedClients);
      RootGroup.add(StatsDroppedClients);
   }
   // get rid of flags on xbliveid
   %xbLiveId = getField(%xbLiveId, 0);   
   echo("Adding dropped client for stats:" SPC %clientId SPC %xbLiveId); 
   StatsDroppedClients.setRowById(%clientId, %xbLiveId);
}

function getExtrapolatedScore(%clientId, %clientScore)
{
   // see if we should apply bonus (bogus?) points for join in progress dudes     
   // but only if our game time is valid
   %gameTime = 0; 
   if ($Client::LastGamePlayEnd > 0 && $Client::LastGamePlayStart > 0)
   {
      %gameTime = $Client::LastGamePlayEnd - $Client::LastGamePlayStart;
      if (%gameTime <= 0)
      {
         error("game time is less than or equal to zero");
         %gameTime = 0;
      }  
   }
   //else
   //   echo("start or end time is invalid" SPC $Client::LastGamePlayEnd SPC $Client::LastGamePlayStart);

   if (isObject(StatsJoinInProgressClients) && %gameTime > 0)
   {
      %rowText = StatsJoinInProgressClients.getRowTextById(%clientId);
      if (%rowText !$= "")
      {
         %joinTime = getWord(%rowText, 1);
         if (%joinTime !$= "" && %joinTime > 0)
         {
            // all of these time values come from the server, so they should be consistent across clients
            
            // determine how long we've been in the game (seconds)
            %timeInGame = ($Client::LastGamePlayEnd - %joinTime) / 1000;
            if (%timeInGame < 0)
               %timeInGame = 0;
            if (%timeInGame > %gameTime)
               %timeInGame = %gameTime;
            
            // determine our score rate
            %scoreRate = %clientScore/%timeInGame;
            
            //echo(%scoreRate SPC %clientScore SPC %timeInGame);
               
            // determine how much time we missed (seconds)
            %missedTime = (%joinTime - $Client::LastGamePlayStart) / 1000;
            if (%missedTime < 0)
               %missedTime = 0;
            
            // determine how much more we would have scored in missed time using scoreRate
            %scoreBonus = %scoreRate * %missedTime;
            // now reduce the bonus so that we don't add too much
            %scoreBonus = mCeil(%scoreBonus * 0.7);
            // now add bonus
            %clientScore += %scoreBonus;
            
            echo("JIP client" SPC %clientId SPC %timeInGame SPC %missedTime SPC %scoreRate);
            echo("   client earned" SPC %scoreBonus SPC "bonus points for being late");
         }
         else
         {
            echo("not adding bogus join in progress points for late client: no join time" SPC %clientId);
         }
      }
   }     

   return %clientScore;
}

function buildClientRanks(%allowScoreExtrapolation)
{
   // Make sure we have our ClientRanks object
   if (!isObject(ClientRanks))
   {
      new GuiTextListCtrl(ClientRanks);
      RootGroup.add(ClientRanks);
   }
   ClientRanks.clear();
   
   // in non-ranked mode, we extrapolate scores for join in progress clients.  we'd rather not 
   // score join in progress as at, but MS is concerned about their Cert people having issues.
   //%allowScoreExtrapolation = !XBLiveIsRanked();

   // Fish out the data from the scoreboard
   for (%i = 0; %i < PlayerListGuiList.rowCount(); %i++)
   {
      %clientId = PlayerListGuiList.getRowId(%i);
      
      %rowText = PlayerListGuiList.getRowText(%i);
      %clientScore = getField(%rowText, 2);

      // Add it to ClientRanks
      ClientRanks.setRowById(%clientId, %clientScore);
   }
   
   ClientRanks.numFinishers = ClientRanks.rowCount();
   
   // now we add all of the people that dropped and slam their scores
   // to zero 
   if (isObject(StatsDroppedClients))
   {
      echo("Num dropped clients:" SPC StatsDroppedClients.rowCount());
      for (%i = 0; %i < StatsDroppedClients.rowCount(); %i++)
      {
         %droppedId = StatsDroppedClients.getRowId(%i);
         // if the host dropped, don't penalize him as a dropped client if he is the only
         // one left in the game.
         if ($Server::Hosting)
         {
            if ($Player::ClientId == %droppedId && ClientGroup.getCount() <= 1)
            {
               echo("   Not penalizing dropped hosted since no other clients are in game");
               continue;
            }
         }
         
         %score = -1; // so that they are below even the zero score people
         ClientRanks.setRowById(%droppedId, %score);
         echo("Setting score for dropped client" SPC %droppedId SPC ":" SPC %score);
      }
   }
   
   // now we set the relative ranks based on the scoring information
   $Client::MPGameTied = setRelativeRanks($Game::ModeId, ClientRanks);     
}

function DumpClientRanks()
{
   // Dump ClientRanks for debugging
   error("Dumping ClientRanks:");
   for (%i = 0; %i < ClientRanks.rowCount(); %i++)
   {
      %clid = ClientRanks.getRowId(%i);
      %val = ClientRanks.getRowText(%i);

      error(%clid SPC %val);
   }
}

function clientCleanupStats()
{
   echo("Cleaning up stat variables");
   
   // Don't do this; lobby needs these scores
   //if (isObject(ClientRanks))
   //   ClientRanks.clear();
      
   if (isObject(StatsDroppedClients))
      StatsDroppedClients.clear();
      
   if (isObject(StatsJoinInProgressClients))
      StatsJoinInProgressClients.clear();
   
   $Game::Mode = "scrum";
   $Game::ModeId = -1;
   $Game::MissionId = -1;
   $Client::MPGameTied = false;
   $Client::currentGameCounts = false;
   $Client::LastGamePlayStart = -1;
   $Client::LastGamePlayEnd = -1;
}

function clientWriteMultiplayerScores()
{
   // bail if we are demo
   if (isDemoLaunch())
   {
      echo("Not writing stats because I'm a demo");
      return;
   }
      
   // bail more if we don't have gold access
   if (!XBLiveIsSignedInGold())
   {
      echo("Not writing stats because I don't have gold Live access");
      return;
   }
      
   // bail even more if we don't have a stats session active
   if (!XBLiveIsStatsSessionActive())
   {
      echo("Not writing stats because there is no session");
      return;
   }
     
   // make sure we have clientranks
   // false means we don't allow score extrapolation for join in progress clients
   //  (we don't want that to factor in to the relative ranks)
   buildClientRanks(false);
   if (!isObject(ClientRanks))
   {
      echo("Client ranks does not exist, can't write stats");
      return;
   }

   // in ranked mode, all clients write the relative ranks.  otherwise only the server
   // writes it
   if (XBLiveIsRanked() || $Server::Hosting)
      // write out the relative ranks
      clientWriteRelativeRanks();
   
   // we don't write our rating formula stuff to the mp overall leaderboard in ranked mode        
   if (!XBLiveIsRanked())
   {     
      // get our client Id from the Lobby
      %client = LobbyGui.findClientIdByXBLiveId($Player::XBLiveId);
      if (%client == 0)
         error("Unable to determine my client Id for scores write");
      else
         clientWriteMultiplayerScore(%client);
   }
}

function clientWriteRelativeRanks()
{
   // make sure we have clientranks
   if (!isObject(ClientRanks))
      return;
      
   echo("Writing relative ranks");
   
   // our relative ranks should already be built and sorted propertly (this is done 
   // by buildClientRanks).  so just write the data to xblive

   // we use global script variables to communicate this to the live engine.  this is more
   // convenient doing a bunch of argument parsing in the engine.
   %numAdded = 0;
   for ( %i = 0; %i < ClientRanks.rowCount(); %i++ )
   {
      %clid = ClientRanks.getRowId(%i);
      // skip any dropped clients they aren't in the session anymore.
      // its possible for a dropped client to still be in the session (if, for example, I 
      // am dropping from the session, I'm both dropped and still in the session)
      if (isObject(StatsDroppedClients))
      {
         %droppedLiveId = StatsDroppedClients.getRowTextById(%clid);
         if (%droppedLiveId !$= "" && !XBLiveIsPlayerRegistered(%droppedLiveId))
         {
            echo("   Skipping dropped client, no longer in session" SPC %clid);
            continue;
         }
      }
         
      %row = ClientRanks.getRowText(%i);
      // rank is first word
      %rank = getWord(%row, 0);
      %score = getWord(%row, 1);
      
      echo("   client:" SPC %clid SPC "rank:" SPC %rank SPC "score:" SPC %score);
           
      // find the xblive for the client
      %xbLiveId = LobbyGui.findXBLiveIdByClientId(%clid);
      // we don't care about the xblive id flags, if any
      %xbLiveId = getField(%xbLiveId, 0);
      if (%xbLiveId $= "")
      {
         echo("Trying to get live id from dropped client list");
         if (isObject(StatsDroppedClients))
            %xbLiveId = StatsDroppedClients.getRowTextById(%clid);
         if (%xbLiveId $= "")
         {
            error("rank error, client" SPC %clid SPC "does not have an xbLiveId");
            continue;
         }
      }
        
      $XBLive::Ranking::PlayerId[%numAdded] = %xbLiveId;
      $XBLive::Ranking::PlayerRank[%numAdded] = %rank;         
      %numAdded++;
   }
   $XBLive::Ranking::NumPlayers = %numAdded;
   XBLiveWriteRelativeRanks();   
}

// for testing
function resetMyMPRating(%leaderboardId)
{
   if (XBLiveGetSignInPort() > -1 && %leaderboardId > 0)
   {
      XBLiveResetLeaderboards(%leaderboardId, XBLiveGetSignInPort());
      XBLiveResetLeaderboards($Leaderboard::MPScrumOverall, XBLiveGetSignInPort());
   }  
}

function clientWriteRatingChanges(%client, %xbLiveId, %leaderboardId, %ratingsList)
{
   // ratingslist is a guitextlistctrl that should contain the ratings for each client in the 
   // game for the specified leaderboard
   if (!isObject(%ratingsList)) 
      return;
      
   // this should contain the scores for each client in the game
   if (!isObject(ClientRanks))
      return;
      
   // prepare input for the computeRatingChanges function
   // first set num players who will be used
   %numPlayers = ClientRanks.rowCount();
   
   // populate the rating and score for each player
   %numAdded = 0;
   for ( %i = 0; %i < ClientRanks.rowCount(); %i++ )
   {
      %clid = ClientRanks.getRowId(%i);
      // skip dropped clients, we don't care about them for our rating purposes
      if (isObject(StatsDroppedClients) && StatsDroppedClients.getRowTextById(%clid) !$= "")
      {
         echo("   Ignoring dropped client" SPC %clid SPC "for rating calculation");
         continue;
      }
         
      %row = ClientRanks.getRowText(%i);
      // xblive relative rank is first word, ignore it
      //%rank = getWord(%row, 0);
      // score is second word
      %score = getWord(%row, 1);
      
      // find this client's rating
      %rating = -1;
      if (%ratingsList.getRowNumById(%clid) != -1)
         %rating = %ratingsList.getRowTextById(%clid);
         
      if (%rating == -1)
      {
         error("   client" SPC %clid SPC "does not have a local rating");
         continue;
      }
         
      // check for initial rating
      if (%rating <= 0)
         %rating = 160000;
      
      // check for minimum rating
      if (%rating < 50000)
         %rating = 50000;
         
      // spew
      echo("   Using rating" SPC %rating SPC "and score" SPC %score SPC "for client" SPC %clid);        
         
      // stuff the info into our globals
      $XBLive::Ranking::rating[%numAdded] = %rating;
      $XBLive::Ranking::origScore[%numAdded] = %score;
      $XBLive::Ranking::score[%numAdded] = getExtrapolatedScore(%clid, %score);
      // this is for our own tracking, it isn't used by the computeRatingChanges function
      $XBLive::Ranking::clientId[%numAdded] = %clid;
      %numAdded++;
   }
   $XBLive::Ranking::numPlayers = %numAdded;
   
   // call the ratings computator to get new ratings for everybody
   computeRatingChanges(false);
   
   // walk the results, updating the ratings for everyone and storing my own updated rating
   %myNewRating = -1;
   %myScore = -1;
   for ( %i = 0; %i < $XBLive::Ranking::numPlayers; %i++ )
   {
      %clid = $XBLive::Ranking::clientId[%i];
      %rating = $XBLive::Ranking::rating[%i];
      %score = $XBLive::Ranking::origScore[%i];
      
      // if this is me, store the rating
      if (%client == %clid)
      {
         %myNewRating = %rating;
         %myScore = %score;
      }
      
      %ratingsList.setRowById(%clid, %rating);
      
      // spew
      echo("   New rating" SPC %rating SPC "for client" SPC %clid);        
   }
   
   if (%myNewRating == -1 || %myScore == -1)
   {
      // doh
      echo("No rating or score for me, skipping leaderboard rating update:" SPC %myNewRating SPC %myNewScore);
   }
   else
   {    
      //echo("Writing to" SPC %xbLiveId SPC %leaderboardId);
      //echo("Writing" SPC %myScore SPC %myNewRating);
      // add the score into the "gems" column
      XBLiveWriteStatsXuid(%xbLiveId, %leaderboardId, "gems", %myScore, "");
      // write the rating      
      XBLiveWriteStatsXuid(%xbLiveId, %leaderboardId, "rating", %myNewRating, "");

      // set stats dirty for this leaderboard
      XBLiveSetStatsDirty(%leaderboardId);  
   }
}

function clientWriteMultiplayerScore(%client)
{
   if (!isObject(ClientRanks))
   {
      // uh oh
      error("Don't have scores for clients, can't write scores");
      return;
   }
      
   if (!isObject(StatsUserRatings_Level) ||
       !isObject(StatsUserRatings_Overall))
   {
      // uh oh
      error("Don't have ratings for clients, can't write scores");
      return;
   }
      
//   if (!XBLiveAreStatsLoaded($Game::MissionId))
//   {
//      // uh oh
//      error("Stats not loaded, can't write scores");
//      return;
//   }
   
   %numPlayers = ClientRanks.rowCount();
   if (%numPlayers <= 0)
   {
      error("Num players <= 0, not writing stats");
      return;
   }
      
   // find the xblive for the client
   %xbLiveId = LobbyGui.findXBLiveIdByClientId(%client);
   // we don't care about the xblive id flags, if any
   %xbLiveId = getField(%xbLiveId, 0);
   if (%xbLiveId $= "")
   {
      error("rank error, client" SPC %client SPC "does not have an xbLiveId");
      return;
   }
   
   // unpack stuff from the rank row
   %row = ClientRanks.getRowTextById(%client);
   if (%row $= "")
   {
      // doh
      error("rank error, client" SPC %client SPC "does not have any row data");
      return;
   }
   
   // get the score for this client
   %score = getWord(%row, 1);

   // now write stats for the game mode
   if ($Game::Mode $= "scrum")
   {
      echo("Writing updated rating for scrum mode");
      
      // get my client id
      %myClient = $Player::ClientId;
      
      // first, do the high score achievement
      // Update high score for us! (Yay us!)
      if( %myClient == %client )
      {
         // high score achievement
         if (ClientRanks.numFinishers > 1 && %score > $UserAchievements::MPHighScore )
         {
            echo("   checking for new high score for client: " @ %client @ " (This client)");
            echo("      new MP high score: " @ %score );
            $UserAchievements::MPHighScore = %score;
         }
         
         // first place achievement
         if (ClientRanks.numFinishers > 1 && !$Client::MPGameTied)
         {
            %fpClient = ClientRanks.getRowId(0);
            %otherXbLiveId = LobbyGui.findXBLiveIdByClientId(%fpClient);
            if (%otherXbLiveId !$= "" && %otherXbLiveId $= $Player::XBLiveId)
            {
               // woo hoo we're winner
               finishedFirstPlaceInMP();
            }
         }

         %mission = GameMissionInfo.getCurrentMission();

         echo("LEVEL ENDED");
         // Map Pack Achievement code:
         if(ClientRanks.numFinishers > 1)
         {
            if(hasFreeMapPack())
            {
               if(%mission.level == 80 && gotABlueGemInTheLevel())
               {
                  echo("\n\n-------------------------- MAP PACK A COMPLETED!!!!\n\n");
                  completedMapPackA_Achievement();   
               }
            }
            if(hasMapPack1())
            {
               if(%mission.level >= 81 && %mission.level <= 85 && %score >= $numGemsNeededMapPackB)
               {
                  echo("\n\n-------------------------- MAP PACK B COMPLETED  LEVEL:" @ %mission.level @ "\n\n");
                  completedMapPackB_Achievement(%mission.level);   
               }
            }
         }

         if(hasMapPack2())
            if(%mission.level == 90 && %score >= $numGemsNeededMapPackC)
            {
               echo("\n\n-------------------------- MAP PACK C COMPLETED!!!!\n\n");
               completedMapPackC_Achievement();   
            }
      }

      if (ClientRanks.numFinishers > 1)
      {      
         // need to rebuild the ranks with extrapolated score information before we compute ratings
         buildClientRanks(true);
         dumpClientRanks();
      
         // write overall rating changes
         echo("Writing rating for client to overall leaderboard");
         clientWriteRatingChanges(%myClient, %xbLiveId, $Leaderboard::MPScrumOverall, StatsUserRatings_Overall);         
         // restore client ranks to "normal" for other operations (Achievements, etc).
         buildClientRanks();
      }
   }
   else
   {
      error("Unknown game mode, not writing stats:" SPC $Game::Mode);
   }
}

function clientAreStatsAllowed()
{
   return !isDemoLaunch();// && !isPCBuild() && XBLiveIsSignedInSilver() && $Client::UseXBLiveMatchMaking;
}

function clientAreOfflineStatsAllowed()
{
   return !isDemoLaunch();// && !isPCBuild();
}

function clientAreAchievementsAllowed()
{
   return !isDemoLaunch();// && !isPCBuild();
}

function onMultiplayerStatsLoaded()
{  
   if (!XBLiveAreStatsLoaded($Leaderboard::MPScrumOverall))
   {
      error("Failed to load MP stats for overall leaderboards");
      return;
   }
   
//   if (!XBLiveAreStatsLoaded(ServerConnection.missionId))
//   {
//      error("Failed to load MP stats for mission");
//      return;
//   }
   
   //%levelRating = XBLiveGetStatValue(ServerConnection.missionId, "rating");
   %levelRating = 0;
   %overallRating = XBLiveGetStatValue($Leaderboard::MPScrumOverall, "rating");
   
   commandToServer('SetUserRating', %levelRating, %overallRating);
   echo("Updating Rating on server:" SPC %levelRating SPC %overallRating);
}

function clientCmdSPRestarting(%restarting)
{
   $Client::SPRestarting = %restarting;
   error("SP restarting:" SPC $Client::SPRestarting);
}

function clientCmdSetGameState(%state, %data)
{
   //echo("@@@@@@@@@@@@ got" SPC %state SPC "state");
   
   // Ok, if the game state is end. Don't let them pause, this will fix the
   // bug where if they pause at the right second it will fuck stuff up -pw
   if( %state $= "end" )
   {
      $GameEndUserName = XBLiveGetUserName();
      $GameEndNoAllowPause = true;

      // Tell autosplitter we finished the level
      XBLivePresenceStopTimer();
      autosplitterSetLevelFinished(true);
      sendAutosplitterData("finish" SPC GameMissionInfo.getCurrentMission().level);
   }
   else
      $GameEndNoAllowPause = false;
        
   if (!isObject(ServerConnection))
   {
      error("clientCmdSetGameState: ServerConnection does not exist");
      return;
   }

   %wasWait = ServerConnection.gameState $= "wait";
   ServerConnection.gameState = %state;
   %isWait = ServerConnection.gameState $= "wait";
   
   if (%wasWait)
   {
      // if we aren't in wait state anymore, we'll need to clear the help message
      // if it isn't already scheduled to be cleared (i.e. it wasn't replaced by another message)
      if (!isEventPending($HelpFadeTimer))
         helpFade(0.50);
   }
   
   // keep rich presence information updated
   if ($Client::connectedMultiplayer)
   {
      // multiplayer
      %mission = GameMissionInfo.getCurrentMission();
      %misName = (ServerConnection.gameState $= "wait") ? LobbyGuiSelection.getSelectedData() : MissionInfo.name;
      if (%misName $= "")
         error("Invalid Mission Name for Rich Presence");
      XBLiveSetRichPresence(XBLiveGetSignInPort(), 2, %misName, %mission.guid);
      //echo("Setting Rich Presence GUID: " SPC %mission.guid);
   }
   else if (%isWait)
   {
      // menus
      XBLiveSetRichPresence(XBLiveGetSignInPort(), 0, "", "");
   }
   else 
   {
      // single player
      %mission = GameMissionInfo.getCurrentMission();
      XBLiveSetRichPresence(XBLiveGetSignInPort(), 1, MissionInfo.name, %mission.guid);
      //echo("Setting Rich Presence GUID: " SPC %mission.guid);
   }

   %allowStats = clientAreStatsAllowed();
   %allowOfflineStats = clientAreOfflineStatsAllowed();
         
   // in single player games, make sure the overall leaderboard is loaded when we enter the ready state
   if (%allowStats && !$Client::connectedMultiplayer && %state $= "ready")
   {
      %mission = GameMissionInfo.getCurrentMission();
      if (!XBLiveAreStatsLoaded($Leaderboard::SPOverall) || 
          !XBLiveAreStatsLoaded(%mission.level))
      {
         echo("Loading single player overall leaderboard & mission leaderboard" SPC %mission.level);
         // read the pair of leaderboards
         XBLiveReadStats($Leaderboard::SPOverall, %mission.level, "", true, true);
      }
      autosplitterSetLevelFinished(false);
      autosplitterSetEggFound(false);
   }
   
   // Check here to see if we need to pop the upsell
   if( isDemoLaunch() &&  !$Client::connectedMultiplayer && %state $= "end" )
      UpsellGui.displayUpsell( false, "SPUpsellCallback();", GameEndGui);
      
   // Update the end game gui here
   if( !$Client::connectedMultiplayer && %state $= "end" )
   {
      if (!$Game::SPGemHunt)
      {
         %mission = GameMissionInfo.getCurrentMission();
         
         %elapsed = PlayGui.elapsedTime;
         %rating = calcRating( %elapsed, %mission.time, %mission.goldTime + $GoldTimeDelta, %mission.difficulty );
         
         //if (%mission.difficultySet $= "custom")
         //{
         //   %theCachedTime = $CachedUserTime::customLevelTime[%mission.customId];
         //} else {
         //   %theCachedTime = $CachedUserTime::levelTime[%mission.level];
         //}
         
         %theCachedTime = $CachedUserScore::LevelScore[%mission.guid];
         
         %cachedBestTime = %theCachedTime;
          if( XBLiveAreStatsLoaded( %mission.level ) )
            %bestTime = XBLiveGetStatValue( %mission.level, "time" );
          else
            %bestTime = %cachedBestTime;
         
         if( %bestTime > %cachedBestTime || %bestTime == 0 )
             %bestTime = %cachedBestTime;
         
         if( %elapsed < %bestTime || %bestTime == 0 )
            %isNewBestTime = 1;
            
         if( %isNewBestTime && %allowOfflineStats )
         {
            //if (%mission.difficultySet $= "custom")
            //{
               //$CachedUserTime::customLevelTime[%mission.customId] = %elapsed;
            //} else {
               //$CachedUserTime::levelTime[%mission.level] = %elapsed;
            //}
            
            $CachedUserScore::LevelScore[%mission.guid] = %elapsed;
         }
         
         // Color coding based on difference from Par Time.
         if (%elapsed < %mission.time) %elapColor = "\c1";
         else if (%elapsed == %mission.time) %elapColor = "\c3";
         else %elapColor = "\c2";
         
         GE_Stats.clear();
         // Format: "Centered `yay` column for `New Best Time!`" TAB "Time Column" TAB "Tag Column" 
         GE_Stats.addRow(-1, " " TAB %elapColor @ formatTime( %elapsed ) TAB $Text::EndTime );
         GE_Stats.addRow(-1, " " TAB "\c3" @ formatTime( %mission.time ) TAB $Text::ParTime );
         GE_Stats.addRow(-1, " " TAB %rating TAB $Text::EndScore );
         if (%isNewBestTime)
            GE_Stats.addRow(-1, $Text::NewBestTime SPC " ");
         else GE_Stats.addRow(-1, " " TAB formatTime( %bestTime ) TAB $Text::BestTime );
      }
      else
      {
         // update scrum mode text (uses score, not time)
         %mission = GameMissionInfo.getCurrentMission();
         //if (%mission.difficultySet $= "custom")
         //{
            //%theCachedTime = $CachedUserTime::customLevelTime[%mission.customId];
         //} else {
            //%theCachedTime = $CachedUserTime::levelTime[%mission.level];
         //}
         %theCachedTime = $CachedUserScore::LevelScore[%mission.guid];
         %cachedBestTime = %theCachedTime;
         if( XBLiveAreStatsLoaded( %mission.level ) )
            %bestTime = XBLiveGetStatValue( %mission.level, "time" );
         else
            %bestTime = %cachedBestTime;

         if( %bestTime < %cachedBestTime || %bestTime == 0 )
             %bestTime = %cachedBestTime;

         %elapsed = LocalClientConnection.points;

         if( %elapsed  > %bestTime || %bestTime == 0 )
            %isNewBestTime = 1;

         if( %isNewBestTime && %allowOfflineStats )
         {
            //if (%mission.difficultySet $= "custom")
            //{
               //$CachedUserTime::customLevelTime[%mission.customId] = %elapsed;
            //} else {
               //$CachedUserTime::levelTime[%mission.level] = %elapsed;
            //}
            $CachedUserScore::LevelScore[%mission.guid] = %elapsed;
         }

         // Color coding based on difference from best time
         if (%elapsed > %bestTime) %elapColor = "\c1";
         else if (%elapsed == %bestTime) %elapColor = "\c3";
         else %elapColor = "\c2";

         GE_Stats.clear();
         // Format: "Centered `yay` column for `New Best Time!`" TAB "Time Column" TAB "Tag Column"
         GE_Stats.addRow(-1, " " TAB %elapColor @ %elapsed TAB $Text::EndScore2 );
         //GE_Stats.addRow(-1, " " TAB "\c3" @ %bestScore TAB $Text::BestScore2 );
         //GE_Stats.addRow(-1, " " TAB %rating TAB $Text::EndScore );
         if (%isNewBestTime)
            GE_Stats.addRow(-1, $Text::NewHighScore SPC " ");
         else
            GE_Stats.addRow(-1, " " TAB %bestTime TAB $Text::BestScore );
      }

      if (%isNewBestTime)
      {
         // schedule this out a bit, so that we don't compete with end pad sound
         if ($newhighScoreDelay $= "")
            $newhighScoreDelay = 2500;
         //if ($localhighscores)
         if ($pref::DoNewHighScoreSFX)
            schedule($newhighScoreDelay, 0, sfxPlay, NewHighScoreSfx);
      }
   }
   
   // Do stat write
   if (%allowStats && !$Client::connectedMultiplayer && %state $= "end")
      doSPStatWrite();
     
   if ($Client::connectedMultiplayer)
   {
      // handle multiplayer stats stuff      
      if (%state $= "ready")
      {
         // clear any JoinInProgress users from prior sessions
         if (isObject(StatsJoinInProgressClients))
            StatsJoinInProgressClients.clear();
      }

      // start an MP stats session if we need to.
      // do this even in demo mode (although we won't actually write anything in demo mode)
      // this is so that the join in progress flag for the session works even in a demo.
      // the "play" state is for people who join in progress.
      // JMQ: changed this from "start" to "ready", because client's seem to get multiple 
      // spurious start state messages      
      if (%state $= "ready" || %state $= "play")
      {
         // set flag indicating whether this game should count for rankings.  store this as a global
         // so that we know if our game counted for rankings even if our connection goes away.
         $Client::currentGameCounts = true;
         if (!%allowStats)
            $Client::currentGameCounts = false;
      
         if ($Client::currentGameCounts)
            echo("Game will count for rankings");
         else
            echo("Game will NOT count for rankings");

         if (!XBLiveIsStatsSessionActive())
         {
            
            echo("Starting stats session due to game " @ %state @ " state");
            XBLiveStartStatsSession();

            // we're allowed one read at the beginning of the session.  read the stats 
            // for the level and the MP overall leaderboard for this guy
            if (%allowStats && XBLiveIsStatsSessionActive())
            {
               // at the moment we always reload, we don't care if we already have them loaded
               echo("Loading stats for multiplayer overall board:" SPC $Leaderboard::MPScrumOverall);
               XBLiveReadStats($Leaderboard::MPScrumOverall, $Leaderboard::MPScrumOverall, "onMultiplayerStatsLoaded();", false);
            }       
         
            // we'll need these globals for scoring
            $Game::Mode = "scrum"; // the only mode we support
            $Game::ModeId = ServerConnection.gameModeId;
            $Game::MissionId = ServerConnection.missionId;
         }
         else if (%state $= "ready")
         {
            // its an error if a session is already started when we get "start"
            error("Stats session already active on game start");
         }
      }
      
      
      // if we are in end state, write scores
      if (XBLiveIsStatsSessionActive() && (%state $= "end" || %state $= "wait"))
      {
         if (%allowStats && $Client::currentGameCounts && %state $= "end")
            clientWriteMultiplayerScores();
         echo("clientCmdSetGameState: Ending stats session and cleaning up stats");
         XBLiveEndStatsSession();
         clientCleanupStats();
      }  
   }
   
   // handle join in progress on server
   if ($Server::Hosting)
   {
      // if we aren't in ranked mode, re-enable join in progress on end or "wait" state
      if (!XBLiveIsRanked() && (%state $= "end" || %state $= "wait"))
         serverSetJIPEnabled(true);
      // if we are in ranked mode, turn of JIP when we enter end state
      if (XBLiveIsRanked() && %state $= "end")
         serverSetJIPEnabled(false);
   }
   
   // update achievement data
   if (clientAreAchievementsAllowed())
   {
//      if (%state $= "ready" && !$UserAchievements::Loaded)
//      {
//         loadAchData();
//      }
      
      if (%state $= "end") // && $UserAchievements::Loaded)
      {
         if (!$Client::connectedMultiplayer)
            doSPAchievementWrite();
         else
         {
            // multiplayer
            
            // the 75 points in MP is set in clientWriteMultiplayerScore
            
            // the achievement code will pull scoring values directly from the leaderboard for
            // the other MP achievements
            checkForAchievements();
         }
         
         // JMQ: this really doesn't work all that well...achievements may not actually be saved yet
         // so getNumAwardedAchievements will return the wrong value.
         // award a gamer picture if they have 3 achievements
//         if (!isPCBuild() && !isDemoLaunch() && XBLiveIsSignedIn())
//         {
//            if (getNumAwardedAchievements() >= 3)
//            {
//               echo("Awarding gamer picture 0");
//               XBAwardGamerPicture( XBLiveGetSignInPort(), 0 );
//            }
//            if (getNumAwardedAchievements() >= 6)
//            {
//               echo("Awarding gamer picture 1");
//               XBAwardGamerPicture( XBLiveGetSignInPort(), 1 );
//            }
//         }
      }
   }
   
   // Do ONE save profile here, instead of a bunch of them, which would
   // cause errors. Use offline stats as the check because we want to make
   // sure that stuff is saved.
   if( %state $= "end" && %allowOfflineStats )
      saveUserProfile();
   
   if (%state $= "start" && isObject(ClientRanks))
      // clear ClientRanks each time a new game starts
      ClientRanks.clear();
      
   if (%state $= "end" && ServerConnection.isMultiplayer)
   {
      // make sure scores in Lobby reflect scoreboard on games that finished normally
      LobbyGui.updateScores();
   }
      
   // demo timer stuff
   if (isDemoLaunch())
   {
      if (%state $= "play" && $Client::connectedMultiplayer)
         startDemoTimer();
      if (%state !$= "play")
         stopDemoTimer();
         
//      if (%state $= "wait" && $Demo::TimeRemaining == 0 && $Client::connectedMultiplayer)
//      {
//         // out of time
//         schedule(0,0,onMPOutOfTime);        
//      }
   }
}

//function onMPOutOfTime()
//{
//   if (!$Client::connectedMultiplayer)
//      return;
//      
//   $disconnectGui = "";
//   if ($Server::Hosting)
//      enterPreviewMode("DEMO_OUTOFTIME");
//   else // client
//      disconnect();     
//   UpsellGui.displayMPPostGame(false);
//   UpsellGui.setBackToGui(MainMenuGui);
//}

//------------------------------------------------------------------------------

function SPUpsellCallback()
{
   if( !isDemoLaunch() )
   {
      doSPStatWrite();
      doSPAchievementWrite();
      saveUserProfile();
   }
}

//------------------------------------------------------------------------------
function getCurrentSPScore()
{
   if ($Game::SPGemHunt)
      return LocalClientConnection.points;
   else
      return PlayGui.elapsedTime;
}

function isScoreBetter(%oldScore, %newScore)
{
   if ($Game::SPGemHunt)
      return %newScore > %oldScore; // point values, higher is better
   else
      return %newScore < %oldScore; // time values, lower is better
}

function doSPStatWrite()
{
   // get our time for current mission
   %mission = GameMissionInfo.getCurrentMission();

   // store the best score on the player object so that we can report it in the 
   // SessionScores notification.     
   LocalClientConnection.player.score = getCurrentSPScore();
   LocalClientConnection.player.bestScore = "";
   LocalClientConnection.player.bestRating = "";
   LocalClientConnection.player.overallRating = "";
   
   if (!XBLiveAreStatsLoaded(%mission.level))
   {
      // uh oh
      error("uh oh, stats not loaded");
   }
   else
   {
      %scoreColumn = "time";
      if ($Game::SPGemHunt)
         %scoreColumn = "score";
         
      %oldScore = XBLiveGetStatValue(%mission.level, %scoreColumn);
      %oldRating = XBLiveGetStatValue(%mission.level, "rating");
      if (%oldRating $= "")
         %oldRating = 0;
      if (%oldScore $= "")
         %oldScore = 0;
      
      %newScore = getCurrentSPScore();
      //%elapsed = PlayGui.elapsedTime;
      
      //if (%mission.difficultySet $= "custom")
      //{
         //%theCachedTime = $CachedUserTime::customLevelTime[%mission.customId];
      //} else {
         //%theCachedTime = $CachedUserTime::levelTime[%mission.level];
      //}      
      
      %theCachedScore = $CachedUserScore::LevelScore[%mission.guid];
      
      // offline sync hack; used the cache time if its better
      if (%theCachedScore > 0 && isScoreBetter(%newScore, %theCachedScore))
      {
         echo("Using cached profile score instead of level score because cached score is better");
         %newScore = %theCachedScore;
      }

      %updated = false;
      LocalClientConnection.player.bestScore = %oldScore;
      LocalClientConnection.player.bestRating = %oldRating;
      
      XBLiveStartStatsSession();
      
      %updated = false;
      if (%oldScore == 0 || isScoreBetter(%oldScore, %newScore))
      {
         %updated = true;
         //%elapsed = $Game::ScoreTime;
         %newRating = calcRating( %newScore, %mission.time, %mission.goldTime + $GoldTimeDelta, %mission.difficulty );
         error("new score:" SPC %newScore SPC %rating);
         LocalClientConnection.player.bestScore = %newScore;
         LocalClientConnection.player.bestRating = %newRating;
         
         XBLiveWriteStats(%mission.level, %scoreColumn, %elapsed, "");
         XBLiveWriteStats(%mission.level, "rating", %rating, "");
         
         // update score on the overall leaderboard
         if (!XBLiveAreStatsLoaded($Leaderboard::SPOverall))
         {
            error("SP Overall leaderboard not loaded, can't store overall score");
         }
         else 
         {
            // if they had an old rating, subtract it from their score on the overall leaderboard
            // and add the new score
            %overall = XBLiveGetStatValue($Leaderboard::SPOverall, "rating");
            error("old overall rating:" SPC unscientific( %overall ) );
            if (%oldRating > 0)
            {
               error("Subtracting old rating from overall rating" SPC unscientific( %oldRating ) SPC unscientific( %overall ) );
               unscientific( %overall -= %oldRating );
               if( unscientific( %overall ) < 0 )
               {
                  error("Overall rating went negative, this should not happen");
                  %overall = 0;
               }
            }
            unscientific( %overall += %rating );
            error("new overall rating:" SPC unscientific( %overall ) );
            LocalClientConnection.player.overallRating = unscientific( %overall );
            XBLiveWriteStats($Leaderboard::SPOverall, "rating", unscientific( %overall ), "");
         }
      }
      XBLiveEndStatsSession();
      // if we updated our stuff, mark stats data as dirty so that leaderboard
      // reloads it if needed.
      if (%updated)
         XBLiveSetStatsDirty(%mission.level);
   }
}

//------------------------------------------------------------------------------

function doSPAchievementWrite()
{
   // single player
   
   %mission = GameMissionInfo.getCurrentMission();
   
   finishedMission(%mission.level);
   
   %elapsed = PlayGui.elapsedTime;
   if (%elapsed < %mission.time)
      finishedPar(%mission.level);
      
   checkForAchievements();
}

//----------------------------------------------------------------------------
// Game events sent from the server
//----------------------------------------------------------------------------
function formatTime(%time)
{
   %isNeg = "";
   if (%time < 0)
   {
      %time = -%time;
      %isNeg = "-";
   }
   
   // Hack for italian
   if( getLanguage() $= "italian" )
      %secondSeperator = ":";
   else
      %secondSeperator = ".";
   
   %hundredth = mFloor((%time % 1000) / 10);
   %totalSeconds = mFloor(%time / 1000);
   %seconds = %totalSeconds % 60;
   %minutes = (%totalSeconds - %seconds) / 60;
   %secondsOne   = %seconds % 10;
   %secondsTen   = (%seconds - %secondsOne) / 10;
   %minutesOne   = %minutes % 10;
   %minutesTen   = (%minutes - %minutesOne) / 10;
   %hundredthOne = %hundredth % 10; 
   %hundredthTen = (%hundredth - %hundredthOne) / 10;
   
   return %isNeg @ %minutesTen @ %minutesOne @ ":" @
       %secondsTen @ %secondsOne @ %secondSeperator @
       %hundredthTen @ %hundredthOne;
}

$Game::clientHiddenTime = 800;

function MarbleData::onClientCollision(%this,%obj,%col)
{
   // JMQ: workaround: skip hidden objects  
   if (%col.isHidden())
      return;
      
   // Try and pickup all items
   if (%col.getClassName() !$= "Item")
      return;

   %data = %col.getDatablock();

   if (%data.getName() $= "GemItem")
   {
      // it's a gem, simply hide the gem and leave, let the server
      // play sounds and update our gem count
      %col.setClientHidden($Game::clientHiddenTime);
      return;
   }

   if (strstr(%data.shapeFile,"ravity.dts") != -1)
   {
      // Must be the anti-gravity powerup (searched for
      // "ravity" in order to avoid potential caps issues).
      %rotation = getWords(%col.getTransform(),3);
      %ortho = vectorOrthoBasis(%rotation);
      %down = getWords(%ortho,6);
      if (VectorDot(%obj.getGravityDir(),%down)>0.9)
         // Don't pick up if same as current gravity:
         return;
      %obj.setGravityDir(%ortho);
   }

   if (%data.powerupId && (%data.powerUpId == %obj.getPowerUpId()))
   {
      // already have this powerup...don't pick up
      return;
   }

   // hide the powerup
   if (!%col.permanent)
      %col.setClientHidden($Game::clientHiddenTime);

   // The rest of this code handles client side pickup of powerups
   // Only do this if we are the control object.
   if (ServerConnection.getControlObject().getId() != %obj.getId())
   {
      //error("not control object" SPC ServerConnection.getControlObject().getId() SPC %obj.getId());
      return;
   }
      
   if (strstr(%data.shapeFile,"ravel.dts") != -1)
   {
      // Must be the time travel powerup (searched for
      // "ravel" in order to avoid potential caps issues).
      // Add some bonus time -- guess at the value, doesn't have
      // to be right because we'll be updated soon if it's wrong.
      %obj.setMarbleBonusTime(%obj.getMarbleBonusTime() + 5000);
   }

   // pick up the powerup
   if (%data.powerUpId)
      %obj.setPowerUpId(%data.powerUpId);
}

function TrapDoor::onClientCollision(%this,%obj,%col)
{
   // Hard-code open time to 200 ms since that is the only
   // value we actually use in the game.
   %obj.playThread(0,"fall",1,200);
}

//------------------------------------------------------------------------------

function calcRating( %time, %parTime, %goldTime, %difficulty )
{
   if ($Game::SPGemHunt)
      return %time * 10; // time will be points in this case 
      
   // Mask out the lowest two digits of the times for the improvement time so that roundoff
   // doesn't make the math look wrong
   %time = %time - ( %time % 10 );
   
   if( %time == 0 )
      return 0;
   
   $completionBonus = 1000;
   
   //$timeBonus = mFloor( ( %goldTime / %time ) * 1000 );
   $timeBonus = 0;
   if (%time < %parTime)
      $timeBonus = mFloor( ( %parTime / %time ) * 1000 );
   else
      $timeBonus = mFloor( ( %parTime / %time ) * 500 );

   $LCS_bottom_score = ( $completionBonus + $timeBonus ) * %difficulty;
   %finalScore = $LCS_bottom_score;
   
   return %finalScore;
}


//------------------------------------------------------------------------------

// Pickup messages
addMessageCallback( 'MsgLocalizedItemPickup', clientLocalizedItemPickupHandler );

function clientLocalizedItemPickupHandler( %msgType, %msgString, %id, %data )
{
   %message = powerupIdToString( %id,%data );
   addChatLine( %message );
}

//-------------------------------------------------------------------------------

// Gem related msgs, also handles easter eggs

addMessageCallback( 'MsgMissingGems', clientTaggedMessageHandler );
addMessageCallBack( 'MsgItemPickup', clientTaggedMessageHandler );
addMessageCallBack( 'MsgHaveAllGems', clientTaggedMessageHandler );
addMessageCallBack( 'MsgRaceOver', clientTaggedMessageHandler );

function clientTaggedMessageHandler(%msgType, %msgString, %id, %data)
{
   // Detag the message and try to load any data into it as required. For strings not requiring config
   // like "You've finished!" etc, avar is a no-op
   addChatLine(avar(detag(%msgString),%data));
}
