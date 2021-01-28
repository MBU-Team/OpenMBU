//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

// Game duration in secs, no limit if the duration is set to 0
$Game::Duration = 20 * 60;

// When a client score reaches this value, the game is ended.
$Game::EndGameScore = 30;

// Pause while looking over the end game screen (in secs)
$Game::EndGamePause = 10;

//-----------------------------------------------------------------------------
//  Functions that implement game-play
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------

function onServerCreated()
{
   // Server::GameType is sent to the master server.
   // This variable should uniquely identify your game and/or mod.
   $Server::GameType = "TSE Demo";

   // Server::MissionType sent to the master server.  Clients can
   // filter servers based on mission type.
   $Server::MissionType = "Demo Mission";

   // GameStartTime is the sim time the game started. Used to calculated
   // game elapsed time.
   $Game::StartTime = 0;

   // Load up basic datablocks, objects etc.
   exec("./audioProfiles.cs");
   exec("./camera.cs");
   exec("./markers.cs");
   exec("./triggers.cs");
   exec("./inventory.cs");
   exec("./shapeBase.cs");
   exec("./staticShape.cs");
   exec("./radiusDamage.cs");
   exec("./chimneyfire.cs");
   exec("./item.cs");
   exec("./weapon.cs");
   exec("./flag.cs");
   exec("./environment.cs");
   
   exec("common/server/lightingSystem.cs");


   // Room demo...
   exec("./scene.cs");

   // Fps game...
   exec("./fps.cs");
   exec("./player.cs");
   exec("./aiPlayer.cs");
   exec("./crossbow.cs");
   exec("./car.cs");

   // Keep track of when the game started
   $Game::StartTime = $Sim::Time;
}

function onServerDestroyed()
{
   // This function is called as part of a server shutdown.
}


//-----------------------------------------------------------------------------

function onMissionLoaded()
{
   // Called by loadMission() once the mission is finished loading.
   // Nothing special for now, just start up the game play.

   // Determin what type of game play we have and activate the
   // associated package.
   echo("Mission Type: " @ MissionInfo.type);
   activatePackage(MissionInfo.type @ "Game");

   // Override the default game type
   $Server::GameType = MissionInfo.type;
   $Server::MissionType = "Play";
   $Game::Duration = 0;

   // Start game play, this doesn't wait players...
   startGame();
}

function onMissionEnded()
{
   // Called by endMission(), right before the mission is destroyed

   // Normally the game should be ended first before the next
   // mission is loaded, this is here in case loadMission has been
   // called directly.  The mission will be ended if the server
   // is destroyed, so we only need to cleanup here.
   deactivatePackage($Server::GameType @ "Game");
   cancel($Game::Schedule);
   $Game::Running = false;
   $Game::Cycling = false;
}


//-----------------------------------------------------------------------------

function startGame()
{
   // Start up the game.  Normally only called once ater a mission is loaded,
   // but theoretically a game can be stopped and restarted without reloading
   // the mission.
   if ($Game::Running) {
      error("startGame: End the game first!");
      return;
   }

   // Inform the client we're starting up
   for( %clientIndex = 0; %clientIndex < ClientGroup.getCount(); %clientIndex++ ) {
      %cl = ClientGroup.getObject( %clientIndex );
      commandToClient(%cl, 'GameStart');

      // Other client specific setup..
      %cl.score = 0;
   }

   // Start the game timer
   if ($Game::Duration)
      $Game::Schedule = schedule($Game::Duration * 1000, 0, "onGameDurationEnd" );

   $Game::Running = true;
}

function endGame()
{
   // Game specific cleanup...
   if (!$Game::Running)  {
      error("endGame: No game running!");
      return;
   }

   // Stop any game timers
   cancel($Game::Schedule);

   // Inform the client the game is over
   for( %clientIndex = 0; %clientIndex < ClientGroup.getCount(); %clientIndex++ ) {
      %cl = ClientGroup.getObject( %clientIndex );
      commandToClient(%cl, 'GameEnd');
   }

   // Delete all the temporary mission objects
   resetMission();
   $Game::Running = false;
}

function onGameDurationEnd()
{
   // This "redirect" is here so that we can abort the game cycle if
   // the $Game::Duration variable has been cleared, without having
   // to have a function to cancel the schedule.
   if ($Game::Duration && !isObject(EditorGui))
      cycleGame();
}


//-----------------------------------------------------------------------------

function cycleGame()
{
   // This is setup as a schedule so that this function can be called
   // directly from object callbacks.  Object callbacks have to be
   // carefull about invoking server functions that could cause
   // their object to be deleted.
   if (!$Game::Cycling) {
      $Game::Cycling = true;
      $Game::Schedule = schedule(0, 0, "onCycleExec");
   }
}

function onCycleExec()
{
   // End the current game and start another one, we'll pause for a little
   // so the end game victory screen can be examined by the clients.
   endGame();
   $Game::Schedule = schedule($Game::EndGamePause * 1000, 0, "onCyclePauseEnd");
}

function onCyclePauseEnd()
{
   $Game::Cycling = false;

   // Just cycle through the missions for now.
   %search = $Server::MissionFileSpec;
   for (%file = findFirstFile(%search); %file !$= ""; %file = findNextFile(%search)) {
      if (%file $= $Server::MissionFile) {
         // Get the next one, back to the first if there
         // is no next.
         %file = findNextFile(%search);
         if (%file $= "")
           %file = findFirstFile(%search);
         break;
      }
   }
   loadMission(%file);
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

function Game::onClientEnterGame(%this,%client)
{
   // Player has joined the game, normally called when a
   // new client has connected to the server.
}

function Game::onLeaveEnterGame(%this,%client)
{
   // Player has left the game, normally called when a client
   // disconnects from the server.
}

function Game::onDeath(%this,%player,%sourceObject, %sourceClient, %damageType, %damLoc)
{
   // Invoked when a client has died.
}

function Game::onLeaveMissionArea(%this,%player)
{
   // The control objects invoked this method when they
   // move out of the mission area.
}

function Game::onEnterMissionArea(%this,%player)
{
   // The control objects invoked this method when they
   // move back into the mission area.
}

function Game::spawnPlayer(%this,%client)
{
   // Create a new player for the client and drop him into the world.
}

function Game::createPlayer(%this, %client, %location)
{
   // Create a new player for the client and start him at the
   // given location.
}


//-----------------------------------------------------------------------------
// GameConnection Methods
// These methods are extensions to the GameConnection class. Extending
// GameConnection make is easier to deal with some of this functionality,
// but these could also be implemented as stand-alone functions.
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------

function GameConnection::onClientEnterGame(%this)
{
   commandToClient(%this, 'SyncClock', $Sim::Time - $Game::StartTime);
   commandToClient(%this, 'SetGameGUI',"PlayGUI");

   // Create a new camera object.
   %this.camera = new Camera() {
      dataBlock = Observer;
   };
   MissionCleanup.add( %this.camera );
   %this.camera.scopeToClient(%this);

   // Spawn the player
   %this.score = 0;
   %this.spawnPlayer();
}

function GameConnection::onClientLeaveGame(%this)
{
   if (isObject(%this.camera))
      %this.camera.delete();
   if (isObject(%this.player))
      %this.player.delete();
}


//-----------------------------------------------------------------------------

function GameConnection::onLeaveMissionArea(%this)
{
   // The control objects invoked this method when they
   // move out of the mission area.
}

function GameConnection::onEnterMissionArea(%this)
{
   // The control objects invoked this method when they
   // move back into the mission area.
}


//-----------------------------------------------------------------------------

function GameConnection::onDeath(%this, %sourceObject, %sourceClient, %damageType, %damLoc)
{
   // Clear out the name on the corpse
   %this.player.setShapeName("");

   // Switch the client over to the death cam and unhook the player object.
   if (isObject(%this.camera) && isObject(%this.player)) {
      %this.camera.setMode("Corpse",%this.player);
      %this.setControlObject(%this.camera);
   }
   %this.player = 0;

   // Doll out points and display an appropriate message
   if (%damageType $= "Suicide" || %sourceClient == %this) {
      %this.incScore(-1);
      messageAll('MsgClientKilled','%1 takes his own life!',%this.name);
   }
   else {
      %sourceClient.incScore(1);
      messageAll('MsgClientKilled','%1 gets nailed by %2!',%this.name,%sourceClient.name);
      if (%sourceClient.score >= $Game::EndGameScore)
         cycleGame();
   }
}


//-----------------------------------------------------------------------------

function GameConnection::spawnPlayer(%this)
{
   // Combination create player and drop him somewhere
   %spawnPoint = pickSpawnPoint();
   %this.createPlayer(%spawnPoint);
}


//-----------------------------------------------------------------------------

function GameConnection::createPlayer(%this, %spawnPoint)
{
   if (%this.player > 0)  {
      // The client should not have a player currently
      // assigned.  Assigning a new one could result in
      // a player ghost.
      error( "Attempting to create an angus ghost!" );
   }

   // Create the player object
   %player = new Player() {
      dataBlock = LightMaleHumanArmor;
      client = %this;
   };
   MissionCleanup.add(%player);

   // Player setup...
   %player.setTransform(%spawnPoint);
   %player.setEnergyLevel(60);
   %player.setShapeName(%this.name);

   // Update the camera to start with the player
   %this.camera.setTransform(%player.getEyeTransform());

   // Give the client control of the player
   %this.player = %player;
   %this.setControlObject(%player);
}


//-----------------------------------------------------------------------------
// Support functions
//-----------------------------------------------------------------------------

function pickSpawnPoint()
{
   %groupName = "MissionGroup/PlayerDropPoints";
   %group = nameToID(%groupName);

   if (%group != -1) {
      %count = %group.getCount();
      if (%count != 0) {
         %index = getRandom(%count-1);
         %spawn = %group.getObject(%index);
         return %spawn.getTransform();
      }
      else
         error("No spawn points found in " @ %groupName);
   }
   else
      error("Missing spawn points group " @ %groupName);

   // Could be no spawn points, in which case we'll stick the
   // player at the center of the world.
   return "0 0 300 1 0 0 0";
}
