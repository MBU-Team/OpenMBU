//-----------------------------------------------------------------------------
// Torque Game Engine
//
// Copyright (c) 2001 GarageGames.Com
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Penalty and bonus times.
$Game::TimeTravelBonus = 5000;

// Item respawn values, only powerups currently respawn
$Item::PopTime = 10 * 1000;
$Item::RespawnTime = 7 * 1000;

// Game duration in secs, no limit if the duration is set to 0
$Game::Duration = 0;

// Pause while looking over the end game screen (in secs)
$Game::EndGamePause = 5;

// Simulated net parameters
$Game::packetLoss = 0;
$Game::packetLag = 0;

//-----------------------------------------------------------------------------
// Variables extracted from the mission
$Game::GemCount = 0;
$Game::StartPad = 0;
$Game::EndPad = 0;

// Variables for tracking end game condition
$Game::GemsFound = 0;
$Game::ClientsFinished = 0;

//-----------------------------------------------------------------------------
//  Functions that implement game-play
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
function execServerScripts()
{
   if ($Server::ScriptsLoaded)
   {
      error("Server scripts already loaded, exiting");
      return;
   }
      
   $Server::ScriptsLoaded = true;
   
   // game commands (lobby, ready status, and stuff)
   exec("./gameCommands.cs");
   
   // Load up all datablocks, objects etc.  
   exec("./audioProfiles.cs");
   exec("./camera.cs");
   exec("./markers.cs");
   exec("./triggers.cs");
   exec("./inventory.cs");
   exec("./shapeBase.cs");
   exec("./staticShape.cs");
   exec("./item.cs");
   exec("./worldObjects.cs");

   // Basic items
   exec("./powerUps.cs");
   exec("./marble.cs");
   exec("./gems.cs");
   //exec("./buttons.cs"); // Unused
   exec("./hazards.cs");
   exec("./pads.cs");
   exec("./bumpers.cs");
   exec("./signs.cs");
   exec("./fireworks.cs");

   exec("./glowBuffer.cs");
   // Platforms and interior doors
   exec("./pathedInteriors.cs");

   // Easter Eggs
   exec("./easter.cs");

  // Tim Particles & Environment
  //exec("./particle_effects.cs");
  //exec("./environment.cs");
  
  
   exec("common/server/lightingSystem.cs");
}

function onServerCreated()
{
   // Server::GameType is sent to the master server.
   // This variable should uniquely identify your game and/or mod.
   $Server::GameType = "OpenMBU";

   // Server::MissionType sent to the master server.  Clients can
   // filter servers based on mission type.
   $Server::MissionType = "Deathmatch";

   // GameStartTime is the sim time the game started. Used to calculated
   // game elapsed time.
   $Game::StartTime = 0;

   // Keep track of when the game started
   $Game::StartTime = $Sim::Time;
}

function onServerDestroyed()
{
   // Perform any game cleanup without actually ending the game
   destroyGame();
}

//-----------------------------------------------------------------------------

function onMissionLoaded()
{
   // Called by loadMission() once the mission is finished loading.
   updateHostedMatchInfo();
   updateServerParams();

   // Start the game in a wait state
   setGameState("wait");

   if (MissionInfo.gameMode $= "Sumo")
      $Game::GemCount = MissionInfo.maxGems;
   else
      $Game::GemCount = countGems(MissionGroup);
      
   initRandomSpawnPoints();

   // JMQ: don't start mission yet, wait for command from Lobby 
   // (Also note that serverIsInLobby check may not be valid at this point; its possible that the 
   // mission is loading while the game is being created)

   // Start the game here if multiplayer and we're not in lobby
   //if ($Server::ServerType $= "MultiPlayer" && !serverIsInLobby())
   //   startGame();
}

function onMissionEnded()
{
   // Called by endMission(), right before the mission is destroyed
   // This part of a normal mission cycling or end.
   endGame();
}

function onMissionReset()
{
   //serverEndFireWorks();
   if (isObject(ServerGroup))
      ServerGroup.onMissionReset();

   // Reset globals for next round
   $tied = false;
   $Game::GemsFound = 0;
   $Game::ClientsFinished = 0;
   $timeKeeper = 0;

   // Reset the players
   for( %clientIndex = 0; %clientIndex < ClientGroup.getCount(); %clientIndex++ ) {
      %cl = ClientGroup.getObject( %clientIndex );
      commandToClient(%cl, 'GameStart');
      %cl.resetStats();
   }
   
   initRandomSpawnPoints();

   if (MissionInfo.gameMode $= "Scrum")
      SetupGems(MissionInfo.numGems);

   $Game::Running = true;
}

function initRandomSpawnPoints()
{
   $Game::UseDetermSpawn = $Game::SPGemHunt && MissionInfo.gameMode $= "Scrum" && $Game::SPGemHuntSeeded;
   $Game::DetermSpawnSeed = $Game::SPGemHuntSeed;

   if ($Game::UseDetermSpawn)
   {
      $Game::SpawnRandFunction = getDetermRandom;
      setDetermRandomSeed($Game::DetermSpawnSeed);
   }
   else
   {
      $Game::SpawnRandFunction = getRandom;
   }   
   
   if ($Game::UseDetermSpawn)
   {
      $Game::PlayerSpawnRandFunction = getDetermRandom2;
      setDetermRandom2Seed($Game::DetermSpawnSeed);
   }
   else
   {
      $Game::PlayerSpawnRandFunction = getRandom;
   }
}

function SimGroup::onMissionReset(%this, %checkpointConfirmNum )
{
   for(%i = 0; %i < %this.getCount(); %i++)
      %this.getObject(%i).onMissionReset( %checkpointConfirmNum );
}

function SimObject::onMissionReset(%this, %checkpointConfirmNum)
{
}

function GameBase::onMissionReset(%this, %checkpointConfirmNum)
{
   %this.getDataBlock().onMissionReset(%this, %checkpointConfirmNum);
}

//-----------------------------------------------------------------------------

function startGame()
{
   if ($Game::Running)
   {
      error("startGame: End the game first!");
      return;
   }
   echo("Starting game");

   $Game::Running = true;
   $Game::Qualified = false;

   if (MissionInfo.gameMode $= "Hunt" ||  MissionInfo.gameMode $= "Sumo" ||  MissionInfo.gameMode $= "Scrum")
      $Game::Duration = MissionInfo.time;

   onMissionReset();

   resetAvgPlayerCount();
}

function endGame()
{
   if (!$Game::Running) {
      error("endGame: No game running!");
      return;
   }
   destroyGame();

   // Inform the clients the game is over
   for (%index = 0; %index < ClientGroup.getCount(); %index++)
   {
      %client = ClientGroup.getObject(%index);
      commandToClient(%client, 'GameEnd');
      commandToClient(%client, 'setTimer',"stop");
      %client.scoreTime = %client.player.getMarbleTime();
      %client.isReady = false;
      echo("Gameconnection time: " @ %client.scoreTime);
   }

   // If single player, stop timer immediately and update game stats using playgui
   if (isObject(PlayGui))
   {
      PlayGUI.stopTimer();
      $Game::ScoreTime = PlayGUI.elapsedTime;
      $Game::ElapsedTime = %client.player.getFullMarbleTime();
      $Game::PenaltyTime = PlayGUI.penaltyTime;
      $Game::BonusTime = $Game::ElapsedTime-$Game::ScoreTime;
      echo("PlayGui time: " @ PlayGui.elapsedTime);
   }

   // Not all missions have time qualifiers
   $Game::Qualified = MissionInfo.time? $Game::ScoreTime < MissionInfo.time: true;

   // Bump up the max level
   if (!$playingDemo && $Game::Qualified && (MissionInfo.level + 1) > $Pref::QualifiedLevel[MissionInfo.type])
         $Pref::QualifiedLevel[MissionInfo.type] = MissionInfo.level + 1;
         
   if (serverGetGameMode() $= "scrum")
   {
      // decrement rounds remaining
      $Server::NumRounds--;
      echo($Server::NumRounds SPC "rounds left");
      if ($Server::NumRounds <= 0)
      {
         // all done, return to lobby
         if (!$Server::Dedicated)
            enterLobbyMode();
         else
            GameMissionInfo.selectNextMission();
         return;
      }
      
      // start the next round
      for (%index = 0; %index < ClientGroup.getCount(); %index++)
      {
         %client = ClientGroup.getObject(%index);
   
         %client.onClientJoinGame();
         %client.respawnPlayer();
      }
   }
}

$pauseTimescale = 0.00001;

function pauseGame()
{
   // we always store a variable indicating when they have requested a game pause (the UI keys off this)
   $gamePauseWanted = true;
   stopDemoTimer(); 
   //sfxPauseAll();
   sfxPause($SimAudioType);
   //if(xbHasPlaybackControl())
   //   pauseMusic();
   sfxPlay(PauseSfx);
   if ($Server::ServerType $= "SinglePlayer" && $Game::Running)
   {
      // we only actually halt the game events in single player games
      $gamePaused = true;
      if (serverGetGameMode() $= "scrum")
      {
         if ($timeScale > $pauseTimescale)
            $saveTimescale = $timescale;

         $timescale = $pauseTimescale;

         //Stop the Discord Timer since the actual in-game Timer isn't running. ~Connie
         XBLivePresenceStopTimer();
      }
   }
}

function resumeGame()
{
   $gamePaused = false;
   $gamePauseWanted = false;
   //sfxUnpauseAll();
   sfxResume();
   //if(xbHasPlaybackControl())
   //   unpauseMusic();
   // restart demo timer if necessary
   if (ServerConnection.gameState $= "play" && $Client::connectedMultiplayer)
      startDemoTimer();
      
   if (serverGetGameMode() $= "scrum")
   {
      if ($saveTimescale $= "")
         $saveTimescale = 1.0;
      $timescale = $saveTimescale;
   }

   //Resume the Discord Timer if it's a Singleplayer game. ~Connie
   if ($Server::ServerType $= "SinglePlayer" && $Game::Running && serverGetGameMode() $= "scrum" && ($Game::State $= "play" || $Game::State $= "go")) {
      XBLivePresenceStartTimer(($Game::Duration - PlayGui.elapsedTime) / 1000);
   }
}

function destroyGame()
{
   // Set the game back to a wait state
   setGameState("wait");

   // Cancel any client timers
   for (%index = 0; %index < ClientGroup.getCount(); %index++)
   {
      %client = ClientGroup.getObject(%index);
      if (isEventPending(%client.respawnSchedule))
         cancel(%client.respawnSchedule);
      if (isEventPending(%client.stateSchedule))
         cancel(%client.stateSchedule);
      %client.respawnSchedule = 0;
      %client.stateSchedule = 0;
   }

   if (isObject(ActiveGemGroups))
      ActiveGemGroups.delete();
      
   if (isObject(FreeGemGroups))
      FreeGemGroups.delete();

   // Perform cleanup to reset the game.
   if (isEventPending($Game::cycleSchedule))
      cancel($Game::CycleSchedule);
      
   if (isEventPending($finishPointSchedule))
      cancel($finishPointSchedule);
      
   if (isEventPending($Game::BlockJIPSchedule))
      cancel($Game::BlockJIPSchedule);
      
   $Game::cycleSchedule = 0;
   $Game::Duration = 0;

   $Game::Running = false;

   $timeKeeper = 0;
}

function faceGems( %player )
{
   // In multi-player, set the position, and face gems
   if( serverGetGameMode() $= "scrum" &&
       isObject( $LastFilledGemSpawnGroup ) &&
       $LastFilledGemSpawnGroup.getCount() )
   {
      %avgGemPos = 0;
      // Ok, for right now, just use the first gem in the group. In final implementation
      // do an average of all the positions of the gems, and look at that.
      for( %i = 0; %i < $LastFilledGemSpawnGroup.getCount(); %i++ )
      {
         %gem = $LastFilledGemSpawnGroup.getObject( %i );
         
         %avgGemPos = VectorAdd( %avgGemPos, %gem.getPosition() );
      }
      %avgGemPos = VectorScale( %avgGemPos, 1 / $LastFilledGemSpawnGroup.getCount() );
      
      %player.cameraLookAtPt( %avgGemPos );
   }
}
//-----------------------------------------------------------------------------

function onGameDurationEnd()
{
   cancel($Game::CycleSchedule);
   // This "redirect" is here so that we can abort the game cycle if
   // the $Game::Duration variable has been cleared, without having
   // to have a function to cancel the schedule.
   if ($Game::Duration && !isObject(EditorGui))
   {
      //cycleGame();
      // Cosmetic: it looks odd when the marble time doesn't match the duration time
      // Find a new $timeKeeper
      for ( %clientIndex = 0; %clientIndex < ClientGroup.getCount(); %clientIndex++ )
      {
         %cl = ClientGroup.getObject( %clientIndex );
         commandToClient(%cl, 'setTimer', "set", $Game::Duration);
      }

      buildRanks();

      doRankNotifications();

      setGameState("end");
   }
}

function cycleGame()
{
   // This is setup as a schedule so that this function can be called
   // directly from object callbacks.  Object callbacks have to be
   // carefull about invoking server functions that could cause
   // their object to be deleted.
   if (!$Game::Cycling) {
      $Game::Cycling = true;
      $Game::CycleSchedule = schedule(0, 0, "onCycleExec");
   }
}

function onCycleExec()
{
   cancel($Game::CycleSchedule);
   // End the current game and start another one, we'll pause for a little
   // so the end game victory screen can be examined by the clients.
   endGame();
   $Game::CycleSchedule = schedule($Game::EndGamePause * 1000, 0, "onCyclePauseEnd");
}

function onCyclePauseEnd()
{
   cancel($Game::CycleSchedule);
   $Game::Cycling = false;
   loadNextMission();
}

function loadNextMission()
{
   %nextMission = "";

   // Cycle to the next level, or back to the start if there aren't
   // any more levels.
   for (%file = findFirstFile($Server::MissionFileSpec);
         %file !$= ""; %file = findNextFile($Server::MissionFileSpec))
      if (strStr(%file, "CVS/") == -1 && strStr(%file, "common/") == -1)
      {
         %mission = getMissionObject(%file);
         if (%mission.type $= MissionInfo.type) {
            if (%mission.level == 1)
               %nextMission = %file;
            if ((%mission.level + 0) == MissionInfo.level + 1) {
               echo("Found one!");
               %nextMission = %file;
               break;
            }
         }
      }
   loadMission(%nextMission);
}

function getGem(%gemDB)
{
   if (!isObject(%gemDB))
   {
      // ugh
      error("error, invalid gem db passed to getGem()");
      return 0;
   }
   
   %gemDB = %gemDB.getId();
   
   %freeGroup = nameToId("FreeGems" @ %gemDB);
   if (isObject(%freeGroup) && %freeGroup.getCount() > 0)
   {
      //echo("allocating gem from pool");
      %gem = %freeGroup.getObject(0);
      %freeGroup.remove(%gem);
      %gem.setHidden(false);
      return %gem;
   } 
   
   //echo("allocating new gem");
   // spawn new Gem   
   %gem = new Item() {
      dataBlock = %gemDB;
      collideable = "0";
      static = "1";
      rotate = "1";
      permanent = "0";
      deleted = "0";
   };
   return %gem;
}

function freeGem(%gem)
{
   if (!isObject(%gem))
      return;
      
   if (!isObject(FreeGemGroups))
      new SimGroup(FreeGemGroups);
      
   %gemDB = %gem.getDatablock();
   %gemDB = %gemDB.getId();
   
   %freeGroupName = "FreeGems" @ %gemDB;
   %freeGroup = nameToId(%freeGroupName);
   if (!isObject(%freeGroup))
   {
      %freeGroup = new SimGroup(%freeGroupName);
      FreeGemGroups.add(%freeGroup);
   }
   
   //echo("freeing gem to pool");
   %freeGroup.add(%gem);
   %gem.setHidden(true);
}

function removeGem(%gem)
{
   freeGem(%gem);
   
   // refill gem groups if necessary
   if (serverGetGameMode() $= "scrum")
      refillGemGroups();
}

// due to gem groups, this function isn't used anymore.  use spawnGemAt() instead
//function spawnGem()
//{
//   if (!isObject(TargetGems))
//   {
//      %targetGroup = new SimGroup(TargetGems);
//
//      MissionGroup.add(%targetGroup);
//   }
//
//   if (!isObject(TargetGemsLights))
//   {
//      %targetGroup = new SimGroup(TargetGemsLights);
//
//      MissionGroup.add(%targetGroup);
//   }
//
//   %group = nameToID("MissionGroup/GemSpawns");
//
//   if (%group != -1)
//   {
//      %count = %group.getCount();
//
//      if (%count > 0)
//      {
//         %spawnpos = "0 0 0";
//
//         for (%i = 0; %i < 6; %i++)
//         {
//            %index = getRandom(%count - 1);
//            %spawn = %group.getObject(%index);
//            %spawnpos = %spawn.getTransform();
//
//            %badSpawn = false;
//            for (%j = 0; %j < TargetGems.getCount(); %j++)
//            {
//               %obj = TargetGems.getObject(%j);
//
//               if (%obj.deleted)
//                  continue;
//
//               %objpos = %obj.getPosition();
//               if (VectorDist(%objpos, %spawnpos) < 3.0)
//               {
//                  %badSpawn = true;
//                  break;
//               }
//            }
//
//            if (!%badSpawn)
//               break;
//         }
//
//         %gem = new Item() {
//            dataBlock = "GemItem";
//            collideable = "0";
//            static = "1";
//            rotate = "1";
//            permanent = "0";
//            deleted = "0";
//         };
//
//         %gem.setTransform(%spawnpos);
//         
//         TargetGems.add(%gem);
//
//         %light = new Item() {
//            dataBlock = "GemLightShaft";
//         };
//
//         %light.setTransform(%spawnpos);
//         
//         TargetGemsLights.add(%light);
//
//         %gem.gemLight = %light;
//         
//         return %gem;
//      }
//      else
//         error("No spawn points found in GemSpawns SimGroup");
//	}
//	else
//		error("Missing GemSpawns SimGroup");
//}

function spawnGemAt(%spawnPoint, %includeLight)
{
   if (!isObject(%spawnPoint))
   {
      error("Unable to spawn gem at specified point: spawn point does not exist" SPC %spawnPoint);
      return 0;
   }
   
   // if it has a gem on it, that is not good
   if (isObject(%spawnPoint.gem) && !%spawnPoint.gem.isHidden())
   {
      // TODO: So currently some gems don't spawn when they should.
      // This makes singleplayer gem hunt inconsistent.
      // For now we can do this but this could cause issues, we need to
      // double check and implement a proper fix.
      $TempFix::BadGems = $Game::SPGemHunt;
      if ($TempFix::BadGems)
      {
         error("Gem spawn point already has an active gem on it (" @ %spawnPoint.gem @ "), ignoring due to tempfix");
      } else {
         error("Gem spawn point already has an active gem on it");
         return %spawnPoint.gem;
      }
   }
     
   // see if the spawn point has a custom gem datablock
   %gemDB = %spawnPoint.getGemDataBlock();
   if (!%gemDB)
      %gemDB = "GemItem";
      
   %gem = getGem(%gemDB);
   %gem.setBuddy(%includeLight);
   
   %gem.setTransform(%spawnPoint.getTransform());
   
   // point the gem on the spawn point
   %spawnPoint.gem = %gem;
      
   return %gem;
}

function getRandomObject(%groupName,%randFunction)
{
   %group = nameToID(%groupName);

   %object = 0;

   if (%randFunction $= "")
      %randFunction = getRandom;
   
   if (%group != -1)
   {
      %count = %group.getCount();

      if (%count > 0)
      {
         %index = call(%randFunction, %count - 1);
         %object = %group.getObject(%index);
      }
   }
   return %object;
}

// returns a random gem spawn point
function findGemSpawn()
{
   return getRandomObject("MissionGroup/GemSpawns", $Game::SpawnRandFunction);
}

// returns a random gem spawn group
function findGemSpawnGroup()
{
   return getRandomObject("GemSpawnGroups", $Game::SpawnRandFunction);
}

// test function
function gemSpawnReport()
{
   %group = nameToId("MissionGroup/GemSpawns");
   for (%i = 0; %i < %group.getCount(); %i++)
   {
      %spawn = %group.getObject(%i);
      if (isObject(%spawn.gem))
         echo(%spawn.gem.getDatablock().getName() SPC "object found on gem spawn point");
   }
}

// test function
function deleteSomeGemSpawnGroups()
{
   for (%i = GemSpawnGroups.getCount() - 1; %i > 0; %i--)
   {
      %x = GemSpawnGroups.getObject(%i);
      GemSpawnGroups.remove(%x);
      %x.delete();
   }
}

// returns a random gem spawn group that is sufficiently far from other active gem groups that still
// have gems in them.
function pickGemSpawnGroup()
{
   if (!isObject(ActiveGemGroups))
   {
      error("ActiveGemGroups is not an object");
      return findGemSpawnGroup();
   }
   
   %gemGroupRadius = $Server::GemGroupRadius;   
   // allow mission to override radius
   if (MissionInfo.gemGroupRadius > 0)
      %gemGroupRadius = MissionInfo.gemGroupRadius;
   echo("PickGemSpawnGroup: using radius" SPC %gemGroupRadius);
   
   // double the radius to make it that much more unlikely that we'll pick a group that is too close
   // to another active group
   %gemGroupRadius *= 2;
   
   // we'll make 6 attempts to find a group that is sufficiently far from other groups
   // after that we give up and use a random group
   %group = 0;
   for (%attempt = 0; %attempt < 6; %attempt++)
   {
      %group = findGemSpawnGroup();
      if (!isObject(%group))
      {
         error("findGemSpawnGroup returned non-object");
         return 0;
      }
      if (%group.getCount() == 0)
      {
         error("gem spawn group contains no spawn points");
         continue;
      }

      %groupSpawn = %group.getObject(0);
            
      // see if the spawn group is far enough from the primary spawn point of point of the active
      // gem groups
      %ok = true;
      for (%i = 0; %i < ActiveGemGroups.getCount(); %i++)
      {
         %active = ActiveGemGroups.getObject(%i);
         if (%active.getCount() == 0)
            continue;
         %gem = %active.getObject(0);
         if (VectorDist(%gem.getTransform(), %groupSpawn.getTransform()) < %gemGroupRadius)
         {
            %ok = false;
            break;
         }
      }

      if (%ok) 
         return %group;
   }
   
   // if we get here we did not find an appropriate group, just use a random one
   error("unable to find a spawn group that works with active gem groups, using random");
   return findGemSpawnGroup();
}

// re-fill any gem groups that are empty
function refillGemGroups()
{  
   if (!isObject(ActiveGemGroups))
   {
      error("ActiveGemGroups does not exist, can't refill gem groups");
      return;
   }
     
   for (%i = 0; %i < ActiveGemGroups.getCount(); %i++)
   {
      %gemGroup = ActiveGemGroups.getObject(%i);
      
      if (%gemGroup.getCount() == 0)
      {
         // current gem group is dead, inform everyone
         if ($Server::CurrentGemGroup != 0)
            messageAll('MsgGemGroupCollected', "Gem Group " @ $Server::CurrentGemGroup @ " completed!"); // JMQMERGE: localize
         $Server::CurrentGemGroup++;
         
         // pick a new spawnGroup for the group
         %spawnGroup = pickGemSpawnGroup();
         if (!isObject(%spawnGroup))
         {
            error("Unable to locate gem spawn group, aborting");
            break;
         }
         %gemGroup.spawnGroup = %spawnGroup;
         fillGemGroup(%gemGroup);
      }
   }
}

// fill the specified gem group with gems using spawn points from its spawnGroup
function fillGemGroup(%gemGroup)
{
   %start = getRealTime();
   
   if (!isObject(%gemGroup))
   {
      error("Can't fill gem group, supplied group is not an object:" SPC %gemGroup);
      return;
   }

   %spawnGroup = %gemGroup.spawnGroup;
   if (!isObject(%spawnGroup))
   {
      error("Can't fill gem group, it does not contain a valid spawn group:" SPC %gemGroup);
      return;
   }
   
   // it should be empty already but clear it out anyway
   %gemGroup.clear();
   
   // refill it using spawn points from its spawn group
   for (%i = 0; %i < %spawnGroup.getCount(); %i++)
   {
      %spawn = %spawnGroup.getObject(%i);
      
      // don't spawn duplicate gems
      if (isObject(%spawn.gem) && !%spawn.gem.isHidden())
      {
         // TODO: So currently some gems don't spawn when they should.
         // This makes singleplayer gem hunt inconsistent.
         // For now we can do this but this could cause issues, we need to
         // double check and implement a proper fix.
         $TempFix::BadGems = $Game::SPGemHunt;
         if ($TempFix::BadGems)
         {
            error("There appears to be a valid gem already (" @ %spawn.gem @ "), but it might not be the case, ignoring due to tempfix.");
         }
         else
         {
            continue;
         }
      }
      
      // spawn a gem and light at the spawn point
      %gem = spawnGemAt(%spawn,true);
            
      // add gem to gemGroup
      %gemGroup.add(%gem);
   }
   
   echo("Took" SPC (getRealTime() - %start) @ "ms to fill gem groups");
   
   $LastFilledGemSpawnGroup = %gemGroup;
}

// setup the gem groups
function SetupGems(%numGemGroups)
{
   %start = getRealTime();
   
   // delete any active gem groups and their associated gems
   if (isObject(ActiveGemGroups))
      ActiveGemGroups.delete();
      
   // get gem group configuration params
   %gemGroupRadius = $Server::GemGroupRadius;
   %maxGemsPerGroup = $Server::MaxGemsPerGroup;
      
   // allow mission to override
   if (MissionInfo.gemGroupRadius > 0)
      %gemGroupRadius = MissionInfo.gemGroupRadius;
   if (MissionInfo.maxGemsPerGroup > 0)
      %maxGemsPerGroup = MissionInfo.maxGemsPerGroup;
   echo("SetupGems: using radius" SPC %gemGroupRadius SPC "and max gems per group" SPC %maxGemsPerGroup);
   
   // clear the "gem" field on each spawn point.  these may contain bogus but valid object ids that were
   // persisted by the mission editor   
   %group = nameToId("MissionGroup/GemSpawns");
   for (%i = 0; %i < %group.getCount(); %i++)
      %group.getObject(%i).gem = 0;
   
   // set up the gem spawn groups (this is done in engine code to improve performance).
   // it will create a GemSpawnGroups object that contains groups of clustered spawn points
   SetupGemSpawnGroups(%gemGroupRadius, %maxGemsPerGroup);
   $Server::CurrentGemGroup = 0;
   
   // ActiveGemGroups contains groups of spawned gems.  the groups are populated using the spawn point
   // information from the ActiveGemGroups object.  when a group is empty, a new gem spawn group is 
   // selected for it, and then it is refilled with gems
   new SimGroup(ActiveGemGroups);
   MissionCleanup.add(ActiveGemGroups);
   
   // create the active gem groups
   for (%i = 0; %i < %numGemGroups; %i++)
   {
      %gemGroup = new SimGroup();
      ActiveGemGroups.add(%gemGroup);
   }
   
   // fill them up
   refillGemGroups();
   
   echo("Took" SPC (getRealTime() - %start) @ "ms to set up gems for mission");
}

//-----------------------------------------------------------------------------
// GameConnection Methods
// These methods are extensions to the GameConnection class. Extending
// GameConnection make is easier to deal with some of this functionality,
// but these could also be implemented as stand-alone functions.
//-----------------------------------------------------------------------------

function GameConnection::incPenaltyTime(%this,%dt)
{
   %this.penaltyTime += %dt;
}

function GameConnection::incBonusTime(%this,%dt)
{
   %this.player.setMarbleBonusTime(%this.player.getMarbleBonusTime() + %dt);
}

//-----------------------------------------------------------------------------

function GameConnection::onClientEnterGame(%this)
{
   if (!$EnableFMS)
   {
      // create a camera for the client
      if (!isObject(%this.camera))
      {
         // Create a new camera object.
         %this.camera = new Camera() {
            dataBlock = Observer;
         };
         MissionCleanup.add( %this.camera );
         %this.camera.scopeToClient(%this);
      }
   }
   
   if ($Game::State $= "wait")
   {
      // all clients enter wait state when joining
      %this.enterWaitState();
   }
   else
   {
      %this.onClientJoinGame();
      if ($Game::Running)
         commandToClient(%this, 'GameStart');
   }

   // Setup game parameters and create the player
   %this.resetStats();

   // tell the client whether we are in the lobby   
   messageClient(%this, 'MsgClientUpdateLobbyStatus', "", serverIsInLobby() );
   
   // tell him his score 
   messageClient(%this, 'MsgClientScoreChanged', "", %this, true, %this.points, %this.points);
   // tell him about everyone else's score
   for ( %clientIndex = 0; %clientIndex < ClientGroup.getCount(); %clientIndex++ )
   {
      %cl = ClientGroup.getObject( %clientIndex );
      if (%cl == %this)
         continue;
         
      messageClient(%this, 'MsgClientScoreChanged', "", %cl, false, %cl.points, %cl.points);
   }

   if ($Game::packetLoss || $Game::packetLag)
      %this.setSimulatedNetParams($Game::packetLoss,$Game::packetLag/2);
}

function GameConnection::onClientJoinGame(%this)
{
   // If we don't have a player object then create one
   if (!isObject(%this.player))
      %this.spawnPlayer();

   %this.isReady = true;

   // If this is the first client to join then start the game
   // Otherwise catchthis client up to the current game state
   if ($Game::State $= "wait")
   {
      // Start the game now
      startGame();
      // Flag this client as the time keeper
      $timeKeeper = LocalClientConnection;
      // Start the game state machine
      setGameState("start");
   }
   else
      %this.updateGameState();

   updateAvgPlayerCount();
   
   faceGems( %this.player );
   
   // keep Lobby status updated   
   messageClient(%this, 'MsgClientUpdateLobbyStatus', "", serverIsInLobby() );

   if (MissionInfo.gameMode $= "Scrum")
      commandToClient(%this,'addStartMessage',$Text::MultiplayerStartup,true);
}

function GameConnection::enterWaitState(%this)
{
   %spawnPoint = pickSpawnPointRandom();
   %spawnPos = getSpawnPosition(%spawnPoint);
   
   if (isSinglePlayerMode())
   {
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
      
      %this.camera = $previewCamera;
   }

   if (!isObject(%this.camera))
   {
      // Create a new camera object.
      %this.camera = new Camera() {
         dataBlock = Observer;
      };
      MissionCleanup.add( %this.camera );
      %this.camera.scopeToClient(%this);
   }
   
   if (!$EnableFMS)
      %this.camera.setTransform(%spawnPos);
   %this.setControlObject(%this.camera);
   
   // smoke the player
   if (isObject(%this.player))
      %this.player.delete();
   %this.player = 0;
}

function GameConnection::onClientLeaveGame(%this)
{
   %this.isReady = true;
   // Check to see if we need to set a new $timeKeeper
   if (%this == $timeKeeper && ClientGroup.getCount() > 1)
   {
      // Find a new $timeKeeper
      for ( %clientIndex = 0; %clientIndex < ClientGroup.getCount(); %clientIndex++ )
      {
         %cl = ClientGroup.getObject( %clientIndex );
         if (%cl != %this)
         {
            $timeKeeper = %cl;
            break;
         }
      }
   }

   if ($Server::ServerType $= "MultiPlayer" && $Game::State $= "start") // We didnt start the game yet somebody left so check if we can start
   {
       %allReady = true;
   
       for (%i = 0; %i < ClientGroup.getCount(); %i++)
       {
           %client = ClientGroup.getObject(%i);
           if (!%client.isReady) 
           {
               %allReady = false;
               break;
           }
       }
       if (%allReady) {
           cancel($stateSchedule);
           $stateSchedule = schedule(500, 0, "setGameState", "ready");
       }
   }

   if (isObject(%this.camera) && %this.camera != $previewCamera)
      %this.camera.delete();
   if (isObject(%this.player))
      %this.player.delete();
   %this.camera = 0;
   %this.player = 0;

   // update average player count, but need to temporarily dec player
   // count (because count doesn't get decremented till later)
   $Server::PlayerCount--;
   updateAvgPlayerCount();
   $Server::PlayerCount++;
}

function GameConnection::resetStats(%this)
{
   // Reset game stats
   %this.gemCount = 0;
   %this.points = 0;
   %this.rank = 0;
   %this.finishTime = 0;
   if (isObject(%this.player) && serverGetGameMode() $= "race")
      %this.player.setMarbleTime(0);

   // Reset the checkpoint, except in single player modes
   // JMQ: not even sure why this is here, there are no checkpoints in
   // multiplayer...
   if (serverGetGameMode() !$= "race" && isObject(%this.checkPoint))
   {
      %this.checkPoint = 0;
      %this.checkPointPowerUp = 0;
      %this.checkPointShape = 0;
      %this.checkPointGemCount = 0;
      %this.checkPointBlastEnergy = 0.0;
      %this.checkPointGemConfirm = 0;
   }
}


//-----------------------------------------------------------------------------

function GameConnection::onEnterPad(%this)
{
   // Handle level ending
   if (serverGetGameMode() $= "race")
   {
      if (%this.player.getPad() == $Game::EndPad) {

         if ($Game::GemCount && %this.gemCount < $Game::GemCount) {
            %this.play2d(MissingGemsSfx);
            messageClient(%this, 'MsgMissingGems', $Text::Tagged::NotAllGems );
         }
         else
         {
            %this.player.setMode(Victory);
            messageClient(%this, 'MsgRaceOver', $Text::Tagged::Congratulations );
            serverplay2d(WonRaceSfx);
            setGameState("end");
         }
      }
   }
}

function GameConnection::onLeavePad(%this)
{
   // Don't care if the leave
}

function GameConnection::onFinishPoint(%this)
{
   error("onFinishPoint called, but shouldn't be?  (Race mode only");
   
   // Freeze the player
   %this.player.setMode(Victory);

   // Grab the marble time (which should stop when the mode is set to Victory)
   %this.finishTime = %this.player.getMarbleTime();

   // A new person finished
   $Game::ClientsFinished++;

   // If this is the first person to finish set a timer to end the whole race
   if (!isEventPending($finishPointSchedule))
      $finishPointSchedule = schedule(10000, 0, "endFinishPoint");

   // Notify them of what palce they came in
//   if ($Game::ClientsFinished == 1)
//      %msg = $Text::Tagged::WonMP;
//   else if ($Game::ClientsFinished == 2)
//      %msg = $Text::Tagged::SecondMP;
//   else if ($Game::ClientsFinished == 3)
//      %msg = $Text::Tagged::ThirdMP;
//   else
//      %msg = $Text::Tagged::NthMP;

   // JMQ: we aren't using race mode anymore so just send down "game over"
   %msg = $Text::Tagged::GameOver;
   messageClient(%this, 'MsgFinish', %msg, $Game::ClientsFinished);

   // If everyone has finished end the game
   if ($Game::ClientsFinished >= ClientGroup.getCount())
      endFinishPoint();
}

function endFinishPoint()
{
   cancel($finishPointSchedule);
   
   // Rank the players
   buildRanks();

   setGameState("end");
}

function doRankNotifications()
{
   if (!isObject(PlayersRanks))
   {
      error("PlayersRanks does not exist, can't do rank notifications");
      return;
   }
   
   if (PlayersRanks.rowCount() == 0)
   {
      error("PlayersRanks has no rows, can't do rank notifications");
      return;
   }
   
   // assume the ranks are sorted in descending order
   // get the name of the leader
   %leader = PlayersRanks.getRowId(0);
   // %leader is a client Id (game connection object)
   %leaderName = %leader.name;
   %leaderPoints = %leader.points;
   
   for ( %clientIndex = 0; %clientIndex < ClientGroup.getCount(); %clientIndex++ )
   {
      %cl = ClientGroup.getObject( %clientIndex );
      %rank = PlayersRanks.getRowTextById(%cl);
      
      if (%rank == PlayersRanks.rowCount() - 1)
      {
         %cl.player.setMode(Victory);
         //if ($tied)
         //   messageClient(%cl, 'MsgRaceOver', $Text::Tagged::TiedMP );
         //else
         //   messageClient(%cl, 'MsgRaceOver', $Text::Tagged::WonMP );
         %cl.play2D(WonRaceSfx);
      }
      else
      {
         %cl.player.setMode(Lost);
         //messageClient(%cl, 'MsgRaceOver', $Text::Tagged::YouLost );
         %cl.play2D(LostRaceSfx);
      }
      messageClient(%cl, 'MsgMPGameOver', "", $tied, %leaderName, %leaderPoints );
   }
}


//-----------------------------------------------------------------------------

function GameConnection::onOutOfBounds(%this)
{
   if ($Game::State $= "End" || $Game::State $= "wait")
      return;

   // Lose a gem if you fall off in Sumo
//   if (MissionInfo.gameMode $= "Sumo" || MissionInfo.gameMode $= "Scrum")
//   {
//      %oldPoints = %this.points;
//      %this.points--;
//
//      if (MissionInfo.gameMode $= "Scrum")
//         if (%this.points < 0)
//            %this.points = 0;
//
//      // send message to client telling him he scored (true parameter means msg recipient is scoring client)
//      messageClient(%this, 'MsgClientScoreChanged', "", %this, true, %this.points, %oldPoints);
//      // send message to everybody except client telling them that he scored
//      messageAllExcept(%this, -1, 'MsgClientScoreChanged', "", %this, false, %this.points, %oldPoints);
//      
//      //commandToClient(%this, 'setPoints', %this, %this.points);
//
//   }

   // Reset the player back to the last checkpoint
   commandToClient(%this,'setMessage',"outOfBounds",2000);
   %this.play2d(OutOfBoundsVoiceSfx);
   %this.player.setOOB(true);
   if(!isEventPending(%this.respawnSchedule))
      %this.respawnSchedule = %this.schedule(2500, respawnPlayer);
}

function Marble::onOOBClick(%this)
{
   if($Game::State $= "play" || $Game::State $= "go")
      %this.client.respawnPlayer();
}

function GameConnection::onDestroyed(%this)
{
   if ($Game::State $= "End")
      return;

   // Reset the player back to the last checkpoint
   //PlayGui.setMessage("destroyed",2000); destroyed?
   %client.play2d(DestroyedVoiceSfx);
   %this.player.setOOB(true);
   if(!isEventPending(%this.respawnSchedule))
      %this.respawnSchedule = %this.schedule(2500, respawnPlayer);
}

function buildRanks()
{
   // MDFHACK: Please mommy don't hit me!
   if (!isObject(PlayersRanks))
      new GuiTextListCtrl(PlayersRanks);

   PlayersRanks.clear();

   // Shove the clients and their gemCounts into the textlist
   for ( %clientIndex = 0; %clientIndex < ClientGroup.getCount(); %clientIndex++ )
   {
      %cl = ClientGroup.getObject( %clientIndex );

      if (MissionInfo.gameMode $= "Sumo" || MissionInfo.gameMode $= "Scrum")
         %val = %cl.points;
      else if (MissionInfo.gameMode $= "Race")
      {
         %val = %cl.finishTime;
         // JMQ: not all clients may have finished.  use marble time in this case
         if (%val == 0)
            %val = %cl.player.getMarbleTime();
      }
      else
         %val = %cl.gemCount;

      PlayersRanks.addRow(%cl, %val);
   }
   
   // set the relative ranks
   $tied = setRelativeRanks($Server::GameModeId, PlayersRanks);
}

function GameConnection::onFoundGem(%this,%amount,%gem,%points)
{
   if (MissionInfo.gameMode $= "Scrum")
   {
      %gem.deleted = true;

      schedule(0, 0, "removeGem", %gem);

      %oldPoints = %this.points;
      %this.points += %points;

      // send message to client telling him he scored (true parameter means msg recipient is scoring client)
      %this.play2D(GotGemSfx);
      %count = ClientGroup.getCount();
      for(%cl= 0; %cl < %count; %cl++)
      {
         %recipient = ClientGroup.getObject(%cl);
         if(%recipient != %this)
            %recipient.play3D(OpponentGotGemSfx, %this.getControlObject().getTransform());
      }
      
      messageClient(%this, 'MsgClientScoreChanged', "", %this, true, %this.points, %oldPoints);
      // send message to everybody except client telling them that he scored
      messageAllExcept(%this, -1, 'MsgClientScoreChanged', "", %this, false, %this.points, %oldPoints);
      
      commandToClient(%this, 'setPoints', %this, %this.points);
      //if (%points == 1)
      //   messageClient(%this, 'MsgItemPickup', $Text::Tagged::GemPickupOnePoint);
      //else
      //   messageClient(%this, 'MsgItemPickup', $Text::Tagged::GemPickupNPoints, %points);

      // Map Pack Achievement code:
      %this.gemCount += %amount;
      commandToClient(%this,'setGemCount',%this.gemCount,$Game::GemCount);
      if(%points == GemItem_5pts.points)
         commandToClient(%this, 'pickedUpABlueGem');
         
      return;
   }

   $Game::GemsFound += %amount;
   %this.gemCount += %amount;

   if (MissionInfo.gameMode $= "Hunt")
      %remaining = $Game::gemCount - $Game::GemsFound;
   else
      %remaining = $Game::gemCount - %this.gemCount;

   if (%remaining <= 0) {
      if (MissionInfo.gameType $= "Multiplayer")
      {
         // Rank the players
         buildRanks();

         doRankNotifications();
         
         setGameState("end");
      }
      else
      {
         messageClient(%this, 'MsgHaveAllGems', $Text::Tagged::HaveAllGems );
         %this.play2d(GotAllGemsSfx);
         %this.gemCount = $Game::GemCount;
      }
   }
   else
   {
      if(%remaining == 1)
         %msg = $Text::Tagged::OneGemLeft;
      else
         %msg = $Text::Tagged::NGemsLeft;

      messageClient(%this, 'MsgItemPickup', %msg, %remaining);
      %this.play2d(GotGemSfx);
   }
   
   // If we are in single player mode, keep track of the gems we have found
   // with regard to checkpoint status so you can't cheat with checkpoints
   if( serverGetGameMode() $= "race" && isObject( %this.checkPoint ) )
   {
      %gem.checkPointConfirmationNumber = %this.checkPointGemConfirm + 1;
   }

   commandToClient(%this,'setGemCount',%this.gemCount,$Game::GemCount);

   if(%point == GemItem_5pts.points)
      commandToClient(%this,pickedUpABlueGem);
}

//-----------------------------------------------------------------------------

function GameConnection::spawnPlayer(%this)
{
   // Combination create player and drop him somewhere
   %this.checkPoint = 0; // clear any checkpoints
   %this.checkPointPowerUp = 0;
   %this.checkPointShape = 0;
   %this.checkPointGemCount = 0;
   %this.checkPointBlastEnergy = 0.0;
   %this.checkPointGemConfirm = 0;
   
   %spawnPoint = pickSpawnPoint();
   %this.createPlayer(%spawnPoint);
   serverPlay3d(spawnSfx, %this.player.getTransform());
}

function restartLevel()
{
   commandToClient(LocalClientConnection, 'SPRestarting', true);
   
   if( isObject( LocalClientConnection.checkPointShape ) )
      LocalClientConnection.checkPointShape.stopThread( 0 );
   
   LocalClientConnection.checkPoint = 0;
   LocalClientConnection.checkPointPowerUp = 0;
   LocalClientConnection.checkPointShape = 0;
   LocalClientConnection.CheckPointGemCount = 0;
   LocalClientConnection.checkPointBlastEnergy = 0.0;
   LocalClientConnection.checkPointGemConfirm = 0;

   if (serverGetGameMode() $= "scrum")
   {
      // tear down the whole game and player so that we don't have any
      // wacky left over state
      //LocalClientConnection.setControlObject(LocalClientConnection.camera);
      //LocalClientConnection.player.delete();
      //LocalClientConnection.player.schedule(0, delete);
      //LocalClientConnection.player = 0;
      destroyGame();
      LocalClientConnection.player.setMarbleTime(0);
      LocalClientConnection.player.setMarbleBonusTime(0);
      LocalClientConnection.player.setOOB(false);
      LocalClientConnection.player.setVelocity("0 0 0");
      LocalClientConnection.player.setVelocityRot("0 0 0");
      LocalClientConnection.player.setBlastEnergy( 0.0 );
      LocalClientConnection.player.setPowerUp("");
      LocalClientConnection.player.clearLastContactPosition();
      LocalClientConnection.onClientJoinGame();
      LocalClientConnection.respawnPlayer();
      setGameState("start");
   }
   else
      LocalClientConnection.respawnPlayer();

   commandToClient(LocalClientConnection, 'SPRestarting', false);
}

function GameConnection::respawnPlayer(%this)
{
   // Reset the player back to the last checkpoint
   cancel(%this.respawnSchedule);
   
   // clear any messages being displayed to client
   commandToClient(%this, 'setMessage',"");

   if (serverGetGameMode() $= "race")
   {
      // if the player has no checkpoint, assume a full mission restart is occuring
      if (!isObject(%this.checkPoint))
      {
         onMissionReset();
         setGameState("start");
         %this.player.setMode(Start);
         %this.player.setMarbleTime(0);
         %this.gemCount = 0;
         commandToClient(%this, 'setGemCount', %this.gemCount, $Game::GemCount);
      }
      else
      {
         // Reset the moving platforms ONLY
         if( isObject( ServerGroup ) )
            ServerGroup.onMissionReset( %this.checkPointGemConfirm );
         
         // This animation is fucked up -pw
         //%this.checkPointShape.stopThread( 1 );
         //%this.checkPointShape.schedule( 200, "playThread", 1, "respawn" );
         %this.gemCount = %this.checkPointGemCount;
         commandToClient(%this, 'setGemCount', %this.gemCount, $Game::GemCount);
         
         %this.player.setBlastEnergy( %this.checkPointBlastEnergy );
      }
   }
   
   %this.player.setMarbleBonusTime(0);
   
   %powerUp = %this.player.getPowerUp();
   %this.player.setPowerUp(0, true);
   if (serverGetGameMode() $= "scrum")
      %this.player.setPowerUp(%powerUp);
   else if( serverGetGameMode() $= "race" && isObject( %this.checkPoint ) && isObject( %this.checkPointPowerUp ) )
      %this.player.setPowerUp( %this.checkPointPowerUp );

   %this.player.setOOB(false);
   %this.player.setVelocity("0 0 0");
   %this.player.setVelocityRot("0 0 0");
   
   if ((serverGetGameMode() $= "race") && isObject(%this.checkPoint))
   {
      // Check and if there is a checkpoint shape, use that instead so the marble gets centered
      // on it properly. Mantis bug: 0000819
      if( isObject( %this.checkPointShape ) )
         %spawnPoint = %this.checkPointShape;
      else
         %spawnPoint = %this.checkPoint;
   }
   else
   {
      // in multiplayer mode, try to use the spawn point closest to the 
      // marble's last contact position.  if this position is invalid 
      // the findClosest function will defer to pickSpawnPoint()
      if (serverGetGameMode() $= "scrum")
         %spawnPoint = findClosestSpawnPoint(%this.player.getLastContactPosition());
      else
      {
         // If we restarted the level, reset the blast energy
         %this.player.setBlastEnergy( 0.0 );
         
         %spawnPoint = pickSpawnPoint();
      }
   }

   %spawnPos = getSpawnPosition(%spawnPoint);
   
   %this.player.setPosition(%spawnPos, 0.45);
   setGravity(%this.player, %spawnPoint);
   
   faceGems( %this.player );
   
   //%this.player.setGravityDir("1 0 0 0 -1 0 0 0 -1",true);
   
   serverPlay3d(spawnSfx, %this.player.getTransform());
}

//-----------------------------------------------------------------------------

function GameConnection::getMarbleChoice( %this )
{
   switch( %this.marbleIndex )
   {
      case 1:
         return MarbleTwo;
      case 2:
         return MarbleThree;
      case 3:
         return MarbleFour;
      case 4:
         return MarbleFive;
      case 5:
         return MarbleSix;
      case 6:
         return MarbleSeven;
      case 7:
         return MarbleEight;
      case 8:
         return MarbleNine;
      case 9:
         return MarbleTen;
      case 10:
         return MarbleEleven;
      case 11:
         return MarbleTwelve;
      case 12:
         return MarbleThirteen;
      case 13:
         return MarbleFourteen;
      case 14:
         return MarbleFifteen;
      case 15:
         return MarbleSixteen;
      case 16:
         return MarbleSeventeen;
      case 17:
         return MarbleEightteen;
      case 18:
         return MarbleNineteen;
      case 19:
         return MarbleTwenty;
      case 20:
         return MarbleTwentyOne;
      case 21:
         return MarbleTwentyTwo;
      case 22:
         return MarbleTwentyThree;
      case 23:
         return MarbleTwentyFour;
      case 24:
         return MarbleTwentyFive;
      case 25:
         return MarbleTwentySix;
      case 26:
         return MarbleTwentySeven;
      case 27:
         return MarbleTwentyEight;
      case 28:
         return MarbleTwentyNine;
      case 29:
         return MarbleThirty;
      case 30:
         return MarbleThirtyOne;
      case 31:
         return MarbleThirtyTwo;
	   case 32:
         return MarbleThirtyThree;         
      case 33:
         return MarbleThirtyFour;
      case 34:
         return MarbleThirtyFive;
      case 35:
         return MarbleThirtySix;
      case 36:
         return MarbleThirtySeven;
   }
   
   return MarbleOne;
}

function GameConnection::createPlayer(%this, %spawnPoint)
{
   if (%this.player > 0)  {
      // The client should not have a player currently
      // assigned.  Assigning a new one could result in
      // a player ghost.
      error( "Attempting to create an angus ghost!" );
   }
   
   %size = 1.5;
   if (MissionInfo.marbleSize !$= "")
      %size = MissionInfo.marbleSize;
   
   %player = new Marble() {
      dataBlock = %this.getMarbleChoice();
      client = %this;
      size = %size;
   };
   MissionCleanup.add(%player);
   
   %physics = "MBU";
   
   if (MissionInfo.physics !$= "")
      %physics = MissionInfo.physics;
      
   echo ("Using " @ %physics @ " Physics");
   %player.setPhysics(%physics);

   // Player setup...
   %spawnPos = getSpawnPosition(%spawnPoint);
   %player.setPosition(%spawnPos, 0.45);
   %player.setEnergyLevel(60);
   %player.setShapeName(%this.name);
   %player.client.status = 1;
   if (serverGetGameMode() $= "scrum")
      %player.setUseFullMarbleTime(true);
   setGravity(%player, %spawnPoint);
   
   //%player.setGravityDir("1 0 0 0 -1 0 0 0 -1",true);

   // Update the camera to start with the player
   if (isObject(%this.camera))
      %this.camera.setTransform(%player.getEyeTransform());       
   
   // In multi-player, set the position, and face gems
   faceGems( %player );
   echo( "Created Player." );

   // Give the client control of the player
   %this.player = %player;
   %this.setControlObject(%player);
}


//-----------------------------------------------------------------------------

function GameConnection::setCheckpoint(%this, %checkPoint)
{
   // The CheckPoint triggers should have a sequence field that can
   // help us keep them in order
   if (%checkPoint.sequence >= %this.checkPoint.sequence && gravityIsEqual( %this.player, %checkpoint ) )
   {
      %this.checkPointGemConfirm++;
      //echo("checkpoint: " @ %this.checkPointGemConfirm);
      
      %sameOne = %checkPoint == %this.checkPoint;
      %this.checkPoint = %checkPoint;
      %this.checkPointPowerUp = %this.player.getPowerUp();
      %this.checkPointGemCount = %this.gemCount;
      %this.checkPointBlastEnergy = %this.player.getBlastEnergy();

      if (!%sameOne)
      {
         %chkGrp = %checkPoint.getGroup();
         
         for(%i = 0; ( %chkShape = %chkGrp.getObject( %i ) ) != -1; %i++ )
         {
            // This looks wonkey, however "checkpointShape" is the name of the datablock which
            // the shapes use. -pw
            if( %chkShape.getClassName() $= "StaticShape" && %chkShape.getDatablock().getId() == checkpointShape.getId() )
            {
               if( isObject( %this.checkPointShape ) )
                  %this.checkPointShape.stopThread( 0 );
               
               %this.checkPointShape = %chkShape;
               break;
            }
         }
   
         serverplay2d(CheckpointSfx);
         %this.checkPointShape.stopThread( 0 );
         %this.checkPointShape.playThread( 0, "activate" );
         commandToClient(%this, 'CheckpointHit', %checkPoint.sequence );
      }
   }
}

function setGravity(%player, %spawnobj)
{
   %rotation = getWords(%spawnobj.getTransform(), 3);
   %ortho = vectorOrthoBasis(%rotation);
   %orthoinv = getWords(%ortho, 0, 5) SPC VectorScale(getWords(%ortho, 6), -1);
   %player.setGravityDir(%orthoinv,true);
}

function gravityIsEqual( %shapebaseA, %shapebaseB )
{
   // Hack for now
   return true;
   
   %rotationA = getWords( %shapebaseA.getTransform(), 3 );
   %orthoA = vectorOrthoBasis( %rotationA );
   %orthoinvA = getWords( %orthoA, 0, 5 ) SPC VectorScale( getWords( %orthoA, 6 ), -1 );
   
   %rotationB = getWords( %shapebaseB.getTransform(), 3 );
   %orthoB = vectorOrthoBasis( %rotationB );
   %orthoinvB = getWords( %orthoB, 0, 5 ) SPC VectorScale( getWords( %orthoB, 6 ), -1 );
   
   echo( "ShapeBaseA (" @ %shapebaseA.getName() @ "): " @ %orthoinvA );
   echo( "ShapeBaseB (" @ %shapebaseB.getName() @ "): " @ %orthoinvB );
   
   return %orthoinvA $= %orthoinvB;
}

function getSpawnPosition(%spawnobj)
{
   if (!isObject(%spawnobj))
      return "0 0 0";

   %pos = %spawnobj.getWorldBoxCenter();
   %rotation = getWords(%spawnobj.getTransform(), 3);

   if (isSinglePlayerMode())
      %offset = MatrixMulVector(%spawnobj.getTransform(), "0 0 1.5");
   else
      %offset = MatrixMulVector(%spawnobj.getTransform(), "0 0 0.5");

   %pos = VectorAdd(%pos, %offset);

   return %pos SPC %rotation;
}

$Server::PlayerSpawnMinDist = 3.0;

function findClosestSpawnPoint(%position)
{
   %spawn = 0;

   %group = nameToID("MissionGroup/SpawnPoints");
   if (%position !$= "" && %group != -1)
   {
      %count = %group.getCount();

      if (%count > 0)
      {
         %closest = 0;
         %closestDist = 0;
         for (%i = 0; %i < %count; %i++)
         {
            %spawn = %group.getObject(%i);
            %spawnpos = %spawn.getPosition();
            
            %dist = VectorDist(%spawnpos, %position);
            
            if (%closest == 0 || %dist < %closestDist)
            {
               %closest = %spawn;
               %closestDist = %dist;
            }
         }
         %spawn = %closest;
      }
   }
   
   if (!isObject(%spawn))
   {
      // defer to pickSpawnPoint()
      error("Unable to find closest spawn point to position, defering to pickSpawnPoint().  Input position:" SPC %position);
      %spawn = pickSpawnPoint();
   }
   else
   {
      // Use the container system to iterate through all the objects
      // within the spawn radius.
      InitContainerRadiusSearch(%spawn.getPosition(), $Server::PlayerSpawnMinDist, $TypeMasks::PlayerObjectType);

      if (containerSearchNext())
      {
         echo("Player spawn point occupied, picking random spawn");
         // spawn point occupied, get a random one
         %spawn = pickSpawnPoint();
      }
   }
   
   return %spawn;
}

// test function
function echoClosestMarble()
{
   if (!isObject(LocalClientConnection.player))
   {
      echo("can't find player");
      return;
   }
   InitContainerRadiusSearch(LocalClientConnection.player.getPosition(), 1000, $TypeMasks::PlayerObjectType);
   %marble = containerSearchNext();
   if (%marble.getId() == LocalClientConnection.player.getId())
      %marble = containerSearchNext();
   if (isObject(%marble))
   {
      %dist = VectorDist(%marble.getPosition(), LocalClientConnection.player.getPosition());
      echo("closest marble is" SPC %dist SPC "units");
   }
   else
   {
      echo("no other marbles found in radius");
   }
}

function pickSpawnPoint()
{
   return pickSpawnPointRandom($Game::PlayerSpawnRandFunction);
}

function pickSpawnPointRandom(%randFunction)
{
   if (%randFunction $= "")
      %randFunction = getRandom;   
   
   %group = nameToID("MissionGroup/SpawnPoints");

   if (%group != -1)
   {
      %count = %group.getCount();

      if (%count > 0)
      {
         for (%i = 0; %i < %count; %i++)
         {
            %index = call(%randFunction, %count-1);
            %spawn = %group.getObject(%index);
            %spawnpos = %spawn.getPosition();

            // Use the container system to iterate through all the objects
            // within the spawn radius.
            InitContainerRadiusSearch(%spawnpos, $Server::PlayerSpawnMinDist, $TypeMasks::PlayerObjectType);

            if (!containerSearchNext() || $Game::UseDetermSpawn)
               return %spawn;
         }

         // Unable to find an empty pad so spawn at a random one
         %index = call(%randFunction, %count-1);
         %spawn = %group.getObject(%index);

         return %spawn;
      }
      else
         error("No spawn points found in SpawnPoints SimGroup");
	}
	else
   {
		error("Missing SpawnPoints SimGroup");

      %missionGroup = GameMissionInfo.getCurrentMission().missionGroup;

      if (isObject(%missionGroup))
         return GameMissionInfo.getCurrentMission().cameraPoint;
   }

	// Could be no spawn points, in which case we'll stick the
	// player at the center of the world.
   return 0;
}

//-----------------------------------------------------------------------------
// Manage game state
//-----------------------------------------------------------------------------

// This is used to inform all of the clients that the
// server is in a "wait" state
function setWaitState(%client)
{
   commandToClient(%client, 'setGemCount', 0, $Game::GemCount);
   commandToClient(%client, 'setTimer', "reset");
   commandToClient(%client, 'setMessage', "");
   commandToClient(%client, 'setGameDuration', 0, 0);
}

// This is used to inform all of the clients that the
// server is in a "start" state
function setStartState(%client)
{
   if (serverGetGameMode() $= "scrum")
      commandToClient(%client, 'setPoints', 0);
   commandToClient(%client, 'setGemCount', 0, $Game::GemCount);
   %help = MissionInfo.startHelpText;
   if (%help !$= "")
      commandToClient(%client, 'setHelpLine', %help);
   commandToClient(%client, 'setMessage',"");

   commandToClient(%client, 'setTimer', "reset");
   if ($Game::Duration)
      commandToClient(%client, 'setGameDuration', $Game::Duration, 0);
   
   if (isObject($timeKeeper) && %client == $timeKeeper)
      %client.player.setMarbleTime(0);
   else if (isObject($timeKeeper))
      %client.player.setMarbleTime($timeKeeper.player.getMarbleTime());

   %client.player.setMode(Start);
}

// This is used to inform all of the clients that the
// server is in a "ready" state
function setReadyState(%client)
{
   commandToClient(%client, 'setMessage', "ready");
   if ($Game::Duration)
      commandToClient(%client, 'setGameDuration', $Game::Duration, 0);
   //%this.play2D(ReadyVoiceSfx);

   // Sync with the "first" marble
   if (isObject($timeKeeper) && %client != $timeKeeper)
      %client.player.setMarbleTime($timeKeeper.player.getMarbleTime());
}

// This is used to inform all of the clients that the
// server is in a "set" state
function setSetState(%client)
{
   commandToClient(%client, 'setMessage', "set");
   //%this.play2D(SetVoiceSfx);

   // Sync with the "first" marble
   if (isObject($timeKeeper) && %client != $timeKeeper)
      %client.player.setMarbleTime($timeKeeper.player.getMarbleTime());
}

// This is used to inform all of the clients that the
// server is in a "go" state
function setGoState(%client)
{
   commandToClient(%client, 'setMessage', "go");
   //%this.play2D(GetRollingVoiceSfx);
   commandToClient(%client, 'setTimer', "start");
   if ($Game::Duration)
      commandToClient(%client, 'setGameDuration', $Game::Duration, 0);

   if (serverGetGameMode() $= "race")
      %client.player.setPad($Game::EndPad);

   %client.player.setMode(Normal);
   if ($Game::Duration != 0)
      XBLivePresenceStartTimer($Game::Duration / 1000);
   else
      XBLivePresenceStartTimer();

   // Sync with the "first" marble
   if (isObject($timeKeeper) && %client != $timeKeeper)
      %client.player.setMarbleTime($timeKeeper.player.getMarbleTime());
}

// This is used to inform all of the clients that the
// server is in a "play" state
function setPlayState(%client)
{
   commandToClient(%client, 'setMessage', "");

   if ($Game::Duration)
      commandToClient(%client, 'setGameDuration', $Game::Duration, $Server::PlayStart);

   // Sync with the "first" marble
   if (isObject($timeKeeper) && %client != $timeKeeper)
      %client.player.setMarbleTime($timeKeeper.player.getMarbleTime());
}

// This is used to inform all of the clients that the
// server is in a "end" state
function setEndState(%client, %winner)
{
   commandToClient(%client, 'setTimer', "stop");
   commandToClient(%client, 'setEndTime', $Server::PlayEnd);
}

function packRanksForClient()
{
   if (!isObject(PlayersRanks))
   {
      error("packRanksForClient: PlayersRanks does not exist");
      return "";
   }
   
   // First the gameMode
   %data = MissionInfo.gameMode;
   
   // Second is the mission id
   %data = %data SPC MissionInfo.level;
   
   // Third is the "ScoreBase" for this game mode: par time for race, total number of gems collected
   // for hunt, 0 for sumo.  this is used to compute score multipliers
   %scoreBase = 0;
   if (MissionInfo.gameMode $= "race")
   {
      %scoreBase = MissionInfo.goldTime;
   }
   else if (MissionInfo.gameMode $= "hunt")
   {
      // walk the clients and sum up the gems
      for (%i = 0; %i < PlayersRanks.rowCount(); %i++)
      {
         %row = PlayersRanks.getRowText(%i);
         %gems = getWord(%row, 1);
         %scoreBase += %gems;
      }
   }
   %data = %data SPC %scoreBase;

   // Fourth is the number of clients
   %data = %data SPC PlayersRanks.rowCount();
   
   // Now push back the client data
   for ( %i = 0; %i < PlayersRanks.rowCount(); %i++ )
   {
      %clid = PlayersRanks.getRowId(%i);
      %val = PlayersRanks.getRowText(%i);

      %data = %data SPC %clid SPC %val;
   }

   return %data;
}

// The server maintains the state and transitions to other states
function setGameState(%state)
{
   echo("STATE" SPC %state);
   // Skip out of the current transitions
   if (isEventPending($stateSchedule))
      cancel($stateSchedule);

   $stateSchedule = 0;
   
   if (%state $= "play")
      $Server::PlayStart = getSimTime();
   if (%state $= "end")
      $Server::PlayEnd = getSimTime();

   // Update client states
   for (%clientIndex = 0; %clientIndex < ClientGroup.getCount(); %clientIndex++)
   {
      %cl = ClientGroup.getObject( %clientIndex );

      switch$ (%state)
      {
         case "wait"  :
            setWaitState(%cl);
         case "start" :
            setStartState(%cl);
         case "ready" :
            setReadyState(%cl);
         case "go"    :
            setGoState(%cl);
         case "play"  :
            setPlayState(%cl);
         case "end"   :
            setEndState(%cl);
      }

      commandToClient(%cl, 'setGameState', %state);
   }

   // schedule the transition to the next state
   switch$ (%state)
   {
      case "start" :
         if ($Server::ServerType !$= "Multiplayer" || ClientGroup.getCount() == 1) // Don't do RSG yet unless we are the only one - or we are not MP
            $stateSchedule = schedule(500, 0, "setGameState", "ready");
      case "ready" :
         $stateSchedule = schedule(3000, 0, "setGameState", "go");
//      case "set"   :
//         $stateSchedule = schedule(1500, 0, "setGameState", "go");
      case "go"    :
         $stateSchedule = schedule(2000, 0, "setGameState", "play");
         if ($Game::Duration)
         {
            $Game::CycleSchedule = schedule($Game::Duration + 90, 0, "onGameDurationEnd" );
            if ($Server::ServerType $= "MultiPlayer")
            {
               // block join in progress in last minute in all games.  it may be already blocked
               // (e.g. in ranked games)
               %blockAt = $Game::Duration - 60 * 1000;
               if (%blockAt > 0)
               {
                  $Game::BlockJIPSchedule = schedule(%blockAt, 0, "serverSetJIPEnabled", false);
               }  
            }
         }
      case "end"   :
         $stateSchedule = schedule( 5000, 0, "endGame");
   }
   
   $Game::State = %state;
}

// This is used to "catch up" newly joining clients
function GameConnection::updateGameState(%this)
{
    if ($Server::ServerType $= "MultiPlayer")
    {
        %allReady = true;
    
        for (%i = 0; %i < ClientGroup.getCount(); %i++)
        {
            %client = ClientGroup.getObject(%i);
            if (!%client.isReady) 
            {
                %allReady = false;
                break;
            }
        }
        if (%allReady && $Game::State $= "start")
            $stateSchedule = schedule(500, 0, "setGameState", "ready");
    }

   switch$ ($Game::State)
   {
      case "wait"  :
         setWaitState(%this);
      case "start" :
         setStartState(%this);
      case "ready" :
         setReadyState(%this);
      case "set"   :
         setSetState(%this);
      case "go"    :
         setGoState(%this);
      case "play"  :
         setPlayState(%this);
      case "end"   :
         setEndState(%this);
   }

   commandToClient(%this, 'setGameState', $Game::State);
}

//-----------------------------------------------------------------------------
// Support functions
//-----------------------------------------------------------------------------

// return a string describing the current game mode
//  this can be "race", which is the single player "time" mode
//  or it can be "scrum", which is the multiplayer mode (also available
//   as a single player mode now).  note the game mode by itself doesn't
// specify whether it is a multiplayer game or not.
// note that the mode is also stored in the missionInfo, but that approach
// prevents playing the same mission in different modes, so we might want
// to drop that eventually
function serverGetGameMode()
{
   if ($Game::SPGemHunt || $Server::ServerType $= "MultiPlayer")
      return "scrum";
   if ($Server::ServerType $= "SinglePlayer")
      return "race";
   // if we get here, oops
   error("server: unknown game mode!");
   return "";
}

function serverCmdJoinGame(%client)
{
   %client.onClientJoinGame();
// this should not be necessary and it may cause a double sound play bug (spawn sfx)
//   if (serverGetGameMode() $= "scrum")
//      %client.respawnPlayer();
}

function serverCmdSetWaitState(%client)
{
   // %JMQ: in mp games, will probably only want to destroy the game when the host enters wait state
   destroyGame();
   %client.enterWaitState();
}

function spawnStupidMarble(%val, %forceNew)
{
   if (%val && $testCheats)
      commandToServer('spawnStupidMarble', %forceNew);
}

function serverCmdSpawnStupidMarble(%client,%forceNew)
{
   if (%client.getId() !$= LocalClientConnection.getId())
   {
      // Nope only host should be able to use this
      warn("Non-host trying to spawn stupid marble");
      return;
   }
   %obj = %client.getControlObject();
   if (isObject(%obj))
   {
      if( !%forceNew && isObject( %client.dummyMarble ) )
         %client.dummyMarble.delete();
      
      %pos = %obj.getPosition();
      %dummy = new Marble()
      {
         dataBlock = %client.getMarbleChoice();
      };
      MissionCleanup.add(%dummy);
      %client.dummyMarble = %dummy;

      %dummy.setPosition(VectorAdd(%pos,"0 1.5 1"), 0.45);
      %dummy.setShapeName(%client.getMarbleChoice());
      %dummy.setGravityDir("1 0 0 0 -1 0 0 0 -1",true);
      echo("stupid marble id" SPC %dummy SPC "position" SPC %pos);
   }
}

$pickerVelocity = "0 3 0";
$pickerRandomVelx = 2;
$pickerRandomVely = 1.5;
$pickerRandomVelz = 1;

function serverCmdSpawnMarblePickerMarble(%client)
{
   if (%client.getId() !$= LocalClientConnection.getId())
   {
      // Nope only host should be able to use this
      warn("Non-host trying to spawn marble picker marble");
      return;
   }
   if( isObject( %client.dummyMarble ) )
      %client.dummyMarble.delete();
   
   %pos = $previewCamera.getPosition();
   %dummy = new Marble()
   {
      dataBlock = %client.getMarbleChoice();
   };
   MissionCleanup.add(%dummy);
   %client.dummyMarble = %dummy;

   %velAdd = (1-2*getRandom()) * $pickerRandomVelx;
   %velAdd = %velAdd SPC (1-2*getRandom()) * $pickerRandomVely;
   %velAdd = %velAdd SPC (1-2*getRandom()) * $pickerRandomVelz;
   %vel = VectorAdd($pickerVelocity,%velAdd);

   %dummy.setPosition(VectorAdd(%pos,"0 1 1"), 0.45);
   %dummy.setVelocity(%vel);
   %dummy.setShapeName(%client.getMarbleChoice());
   %dummy.setGravityDir("1 0 0 0 -1 0 0 0 -1",true);
}

function serverCmdDestroyMarblePickerMarble(%client)
{
   if( isObject( %client.dummyMarble ) )
      %client.dummyMarble.delete();
   
   %client.dummyMarble = -1;
}
//-----------------------------------------------------------------------------

function countGems(%group)
{
   // Count up all gems out there are in the world
   %gems = 0;
   for (%i = 0; %i < %group.getCount(); %i++)
   {
      %object = %group.getObject(%i);
      %type = %object.getClassName();
      if (%type $= "SimGroup")
         %gems += countGems(%object);
      else
         if (%type $= "Item" &&
               %object.getDatablock().classname $= "Gem")
            %gems++;
   }
   return %gems;
}

//-----------------------------------------------------------------------------
// Averagae player count code -- track integral of # of players over time
//-----------------------------------------------------------------------------

function updateAvgPlayerCount()
{   
   if (isObject($timeKeeper))
   {
      // update player count averages
      echo("Updating average player count...");
      %time = $timeKeeper.player.getMarbleTime()/1000;
      %delta = %time-$Game::avgPlayerCountTime;
      echo("   Previous count was" SPC $Game::avgPlayerCount SPC "for" SPC $Game::avgPlayerCountTime SPC "seconds.");
      if (%delta<0)
      {
         // safety...
         error("updateAvgPlayerCount: negative time...");
         %delta=0;
      }
      echo("   Adding in" SPC $Game::lastPlayerCount SPC "players over an additional" SPC %delta SPC "seconds.");
      $Game::avgPlayerCount = $Game::avgPlayerCount * $Game::avgPlayerCountTime + $Game::lastPlayerCount * %delta;
      if (%time>0.0001)
         $Game::avgPlayerCount /= %time;
      $Game::avgPlayerCountTime = %time;
      $Game::lastPlayerCount =    $Server::PlayerCount;
      echo("   New average player count is" SPC $Game::avgPlayerCount SPC "and there are currently" SPC $Game::lastPlayerCount SPC "players in the game");
   }
   else
   {
      error("No time keeper marble");
      resetAvgPlayerCount();
   }
}

function resetAvgPlayerCount()
{
   $Game::avgPlayerCount = $Server::PlayerCount;
   $Game::avgPlayerCountTime = 0;
   $Game::lastPlayerCount = 0;
}

function getAvgPlayerCount()
{
    return $Game::avgPlayerCount;
}

//-----------------------------------------------------------------------------

function simLag(%loss,%lag)
{
   $Game::packetLoss = %loss;
   $Game::packetLag = %lag;
   // if in single player mode, don't change parameters on
   // connections now, wait till game loads.
   if (!isSinglePlayerMode())
   {
      for (%i=0; %i < ClientGroup.getCount(); %i++)
      {
         %client = ClientGroup.getObject(%i);
         %client.setSimulatedNetParams(%loss,%lag/2);
      }
      if (!isBroadcastSimNetParams())
         // not broadcasting sim net params, so set local value here
         ServerConnection.setSimulatedNetParams(%loss,%lag/2);
   }
   error("simLag called with loss" SPC %loss SPC "and lag" SPC %lag);
}
