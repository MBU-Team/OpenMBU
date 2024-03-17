//-----------------------------------------------------------------------------
// Variables used by client scripts & code.  The ones marked with (c)
// are accessed from code.  Variables preceeded by Pref:: are client
// preferences and stored automatically in the ~/client/prefs.cs file
// in between sessions.
//
//    (c) Client::MissionFile             Mission file name
//    ( ) Client::Password                Password for server join
//    (?) Pref::Player::CurrentFOV
//    (?) Pref::Player::DefaultFov
//    ( ) Pref::Input::KeyboardTurnSpeed
//    (c) pref::Master[n]                 List of master servers
//    (c) pref::Net::RegionMask     
//    (c) pref::Client::ServerFavoriteCount
//    (c) pref::Client::ServerFavorite[FavoriteCount]
//    .. Many more prefs... need to finish this off
// Moves, not finished with this either...
//    (c) firstPerson
//    $mv*Action...
//-----------------------------------------------------------------------------
// These are variables used to control the shell scripts and
// can be overriden by mods:
//-----------------------------------------------------------------------------

$EnableFMS = true;

if ($testLevel || $disablePreviews)
   $EnableFMS = false;
   
if ($buildMega)
   $EnableFMS = true;
   
if ($EnableFMS)
   $buildMega = true; // Always use build mega, for custom support

function initClient()
{
   echo("\n--------- Initializing FPS: Client ---------");
   exec("./myprefs.cs"); 
   exec("./prefs.cs"); 
   execPrefs("prefs.cs"); 
   
   initLanguage();
   
   // Make sure this variable reflects the correct state.
   $Server::Dedicated = false;
   // Game information used to query the master server
   $Client::GameTypeQuery = "OpenMBU";
   $Client::MissionTypeQuery = "Any";
   // Default level qualification
   if (!$pref::QualifiedLevel["Beginner"])
      $pref::QualifiedLevel["Beginner"] = 1;
   if (!$pref::QualifiedLevel["Intermediate"])
      $pref::QualifiedLevel["Intermediate"] = 1;
   if (!$pref::QualifiedLevel["Advanced"])
      $pref::QualifiedLevel["Advanced"] = 1;
   // The common module provides basic client functionality
   initBaseClient();
   // initCanvasSize determines some virtual sizing and widescreen vs. normal
   // parameters.
   initCanvasSize();
   // InitCanvas starts up the graphics system.
   // The canvas needs to be constructed before the gui scripts are
   // run because many of the controls assume the canvas exists at
   // load time.
   initCanvas("Marble Blast");
     
   if (isPCBuild())
      Canvas.setCursor("DefaultCursor");
   else
      Canvas.hideCursor();
     
   // Loadup Misc client functions
   exec("./scripts/client.cs");
   
   /// Load client-side Audio Profiles/Descriptions
   exec("./scripts/audioProfiles.cs");
   // Load up the Game GUIs
   exec("./ui/defaultGameProfiles.cs");
   exec("./ui/loadingGui.gui");
   exec("./ui/missionLoadingGui.gui");
   exec("./ui/presentsGui.gui");
   // Get something on the screen as fast as possible 
   //Canvas.setContent(presentsGui);
   Canvas.setContent(loadingGui);
   Canvas.repaint();
   $EnableDatablockCanvasRepaint = true;
   
   // fire up networkin'
   setNetPort(0);
   
   // fire up Mr. Telnet Console
   if (!isRelease())
      telnetSetParameters(23, "","");

   // Resource preloads
   exec("./scripts/resourcePreloads.cs");
   clientPreloadTextures();

   // initialize game data - this is done here because materials and shaders may need the gfx object, 
   // which isn't available until the canvas is created
   exec("~/data/init.cs");   
   
   // now we keep loadin' er up
   // Load up the shell GUIs
   exec("./ui/RootGui.gui");
   exec("./ui/MainMenuGui.gui");
   exec("./ui/DifficultySelectGui.gui");
   exec("./ui/PlayGui.gui");
   exec("./ui/XMessagePopupDlg.gui");
   exec("./ui/optimatchGui.gui");
   exec("./ui/GamePauseGui.gui");
   exec("./ui/ConnErrorGui.gui");
   exec("./ui/MultiPlayerGui.gui");
   exec("./ui/FindGameGui.gui");
   exec("./ui/CreateGameGui.gui");
   exec("./ui/LobbyGui.gui");
   exec("./ui/LobbyPopupDlg.gui");
   exec("./ui/lbMenuGui.gui");
   exec("./ui/LevelPreviewGui.gui");
   exec("./ui/LevelScoresGui.gui");
   exec("./ui/ControlerUnplugGui.gui");
   exec("./ui/UpsellGui.gui");
   exec("./ui/PlayerListGui.gui");
   exec("./ui/HelpAndOptionsGui.gui");
   exec("./ui/ESRBGui.gui");
   exec("./ui/inputAndSoundOptionsGui.gui");
   exec("./ui/videoOptionsGui.gui");
   exec("./ui/miscOptionsGui.gui");
   exec("./ui/marblePickerGui.gui");
   exec("./ui/aboutMenuOptionsGui.gui");
   exec("./ui/helpGui.gui");
   exec("./ui/demoTimerGui.gui");
   exec("./ui/GameEndGui.gui");
   exec("./ui/StartupErrorGui.gui");
   exec("./ui/controlerDisplayGui.gui");
   exec("./ui/joinGameGui.gui");
   
   //exec("./ui/AboutGui.gui");   
   //exec("./ui/LevelScoresGui.gui");
   //exec("./ui/overallScoresGui.gui");
   //exec("./ui/LevelCompleteTimeGui.gui");
   //exec("./ui/LevelCompleteScoreGui.gui");
   //exec("./ui/LevelCompleteLogOnDemoGui.gui");
   //exec("./ui/OptionsGui.gui");
   //exec("./ui/ErrorUnplugGui.gui");
   //exec("./ui/UIXOverlayGui.gui");
   //exec("./ui/PauseScoresGui.gui");
   //exec("./ui/ConfirmGui.gui");
   //exec("./ui/UpdateRequiredGui.gui");
   //exec("./ui/MainScoresGui.gui");
   //exec("./ui/GenericErrorGui.gui");
   //exec("./ui/NetworkDisconnectGui.gui");
   
   // Client scripts
   exec("./scripts/missionDownload.cs");
   exec("./scripts/serverConnection.cs");
   exec("./scripts/chatHud.cs");
   exec("./scripts/playGui.cs");
   exec("./scripts/centerPrint.cs");
   exec("./scripts/game.cs");
   exec("./scripts/fireworks.cs");
   exec("./scripts/version.cs");
   exec("./scripts/playerList.cs");
   exec("./scripts/recordings.cs");
   exec("./scripts/achievements.cs");
   
   exec("./ui/AchievementDlg.gui");
   exec("./ui/AchievementListGui.gui");
   exec("./ui/JoinGameInviteDlg.gui");

   exec("./scripts/xbLive.cs");
   
   exec("./ui/chathud/init.cs");
   
   // Default player key bindings
   exec("./scripts/default.bind.cs");
   exec("./scripts/xbControler.cs");
   
   exec("./scripts/interiorTest.cs");
   
   if (isObject(MusicPlayer))
      MusicPlayer.activate();

   // Copy saved script prefs into C++ code.
   setShadowDetailLevel( $pref::shadows );
   setDefaultFov( $pref::Player::defaultFov );
   setZoomSpeed( $pref::Player::zoomSpeed );
        
   $enableDirectInput = "1";
   activateDirectInput();
   enableGamepad();

   %missionFile = "";
   
   // Connect to server if requested.
   //if ($JoinGameAddress !$= "") 
   //{
   //   connect($JoinGameAddress, "", $Pref::Player::Name);  
   //}
   //else if($missionArg !$= "") 
   if($missionArg !$= "") 
   {
      %missionFile = findNamedFile($missionArg, ".mis");
   }
   else if($interiorArg !$= "") {
      %file = findNamedFile($interiorArg, ".dif");
      if(%file !$= "")
      {
         // hack - need the server scripts now
         execServerScripts();
         
         //%missionGroup = createEmptyMission($interiorArg);
         //%interior = new InteriorInstance() {
                        //position = "0 0 0";
                        //rotation = "1 0 0 0";
                        //scale = "1 1 1";
                        //interiorFile = %file;
                     //};
         //%missionGroup.add(%interior);
         //%interior.magicButton();
//
         //if (!isObject(SpawnPoints))
            //%spawnPointsGroup = new SimGroup(SpawnPoints);
         //MissionGroup.add(%spawnPointsGroup);
//
         //if (!isObject(StartPoint))
         //{
            //// create spawn points group
            //new StaticShape(StartPoint) {
               //position = "0 -5 25";
               //rotation = "1 0 0 0";
               //scale = "1 1 1";
               //dataBlock = "StartPad";
            //};
         //}
         //%spawnPointsGroup.add(StartPoint);
//
         //if(!isObject(EndPoint))
         //{
            //%pt = new StaticShape(EndPoint) {
               //position = "0 5 100";
               //rotation = "1 0 0 0";
               //scale = "1 1 1";
               //dataBlock = "EndPad";
            //};
            //MissionGroup.add(%pt);
         //}
         //%box = %interior.getWorldBox();
         //%mx = getWord(%box, 0);
         //%my = getWord(%box, 1);
         //%mz = getWord(%box, 2);
         //%MMx = getWord(%box, 3);
         //%MMy = getWord(%box, 4);
         //%MMz = getWord(%box, 5);
         //%pos = (%mx - 3) SPC (%MMy + 3) SPC (%mz - 3);
         //%scale = (%MMx - %mx + 6) SPC (%MMy - %my + 6) SPC (%MMz - %mz + 20);
         //echo(%box);
         //echo(%pos);
         //echo(%scale);
         //new Trigger(Bounds) {
            //position = %pos;
            //scale = %scale;
            //rotation = "1 0 0 0";
            //dataBlock = "InBoundsTrigger";
            //polyhedron = "0.0000000 0.0000000 0.0000000 1.0000000 0.0000000 0.0000000 0.0000000 -1.0000000 0.0000000 0.0000000 0.0000000 1.0000000";
         //};
         //MissionGroup.add(Bounds);
//
         ////%missionFile = "marble/data/missions/testMission.mis";
         //%missionFile = strreplace(%file, ".dif", ".mis");
         //%missionGroup.save(%missionFile);
         //%missionGroup.delete();
         
         %missionFile = createMissionFromDif(%file);
         
         schedule(0,0,loadTestLevel,%missionFile);
         return;
      }
   }
   
   // Start up the main menu... this is separated out into a 
   // method for easier mod override.
   schedule(0,0,loadMainMenu,%missionFile);
   
   //makeFonts();
}

function loadTestLevel(%missionFile)
{
   $Host::QuickLoad = true;
   createServer("SinglePlayer", %missionFile);
   connectToServer("");
   waitTestLevel();
}

function waitTestLevel()
{
   cancel($Client::TestWaitSched);
   if (ServerConnection.getControlObject() == 0)
   {
      $Client::TestWaitSched = schedule(100,0,waitTestLevel);
      return;
   }
   
   $EnableDatablockCanvasRepaint = false;
   
   RootGui.show();
   RootGui.setContent(ESRBGui);
   RootGui.setContent(PlayGui);
   
   commandToServer('JoinGame');
}

function initCanvasSize()
{
   // note: the following assumes resolution already set.  In current
   // calling order, that is fine even though we haven't called initCanvas
   // yet because canvas is now created well before initCanvas.
   %width = getWord(getResolution(), 0);
   %height = getWord(getResolution(), 1);
   
   // add root center control now and figure out how to size it
   if (%width < 1280 || %height < 720)
   {
      %w = 640;
      %h = 480;
      $Shell::Widescreen = false;
   }
   else
   {
      %w = 1280;
      %h = 720;
      $Shell::Widescreen = true;
   }
   %offsetX = (%width-%w)/2;
   %offsetY = (%height-%h)/2;
   $Shell::Resolution = %w SPC %h;
   $Shell::Offset = %offsetX SPC %offsetY;
   $Shell::VirtualResolution = %w SPC %h;
   $PlayGui::safeVerMargin = 1 + (%height * 0.15)/2;
   $PlayGui::safeHorMargin  = 1 + (%width * 0.15)/2;
}

function fixSizing()
{
   initCanvasSize();

   // resize to screen resolution
   %width = getWord(getResolution(), 0);
   %height = getWord(getResolution(), 1);
   RootGui.resize(0, 0, %width, %height);
   GamePauseGui.resize(0, 0, %width, %height);
   
   %w = getWord($Shell::Resolution,0);
   %h = getWord($Shell::Resolution,1);
   %offsetX = getWord($Shell::Offset,0);
   %offsetY = getWord($Shell::Offset,1);
   
   RootCenterCtrl.resize(%offsetX,%offsetY,%w,%h);
   PauseCenterCtrl.resize(%offsetX,%offsetY,%w,%h);
   
   if (PlayGui.isAwake() && !$GamePauseGuiActive)
   {
      // TODO: Figure out why doing this sometimes causes BlastBar to offset from bitmap
      RootGui.setContent(PlayGui);
   }
}

function makeFonts()
{
   populateFontCacheRange("Arial", 12, 1, 255);
   populateFontCacheRange("Arial", 13, 1, 255);
   populateFontCacheRange("Arial", 14, 1, 255);
   populateFontCacheRange("Arial", 16, 1, 255);
   populateFontCacheRange("Arial", 32, 1, 255);
   populateFontCacheRange("Arial Bold", 18, 1, 255);
   populateFontCacheRange("Arial Bold", 20, 1, 255);
   populateFontCacheRange("Arial Bold", 22, 1, 255);
   populateFontCacheRange("Arial Bold", 24, 1, 255);
   populateFontCacheRange("Arial Bold", 30, 1, 255);
   populateFontCacheRange("Arial Bold", 32, 1, 255);
   populateFontCacheRange("Arial Bold", 36, 1, 255);
   populateFontCacheRange("Lucida Console", 12, 1, 255);
   //populateFontCacheRange("Convection Symbol", 18, 1, 255); // unused
   //populateFontCacheRange("Convection Symbol", 24, 1, 255); // unused
   populateFontCacheRange("ColiseumRR Medium", 48, 1, 255);
   writeFontCache();
}

function findNamedFile(%file, %ext)
{
   if(fileExt(%file) !$= %ext)
      %file = %file @ %ext;
   %found = findFirstFile(%file);
   if(%found $= "")
      %found = findFirstFile("*/" @ %file);
   return %found;
}

//-----------------------------------------------------------------------------
function loadMainMenu(%missionFile)
{
   // Startup the client with the Main menu...
   //runPresentation();
   createPreviewServer(%missionFile);
   waitForPreviewLevel(); 
   
   //$PresentationIsDone = true;
   //Canvas.setContent( LevelPreviewGui );

   //playShellMusic();  

}

function waitForPreviewLevel()
{
   cancel($Client::PreviewWaitSched);
   if (ServerConnection.getControlObject() == 0)
   {
      $Client::PreviewWaitSched = schedule(100,0,waitForPreviewLevel);
      return;
   }
   
   $EnableDatablockCanvasRepaint = false;
   
   RootGui.show();
   RootGui.setContent(ESRBGui);
   //RootGui.setContent(MainMenuGui);
   
   %errors = "";
   
   if ($ScriptError !$= "")
   {
      %errors = %errors @ "Script Errors:" NL $ScriptErrorContext NL "";
      //XMessagePopupDlg.show(0, $ScriptErrorContext, $Text::OK);
      error($ScriptError);
      error($ScriptErrorContext);
   }
   if ($LocaleErrors !$= "")
   {
      %errors = %errors @ "Localization File Errors:" NL $LocaleErrors NL "";
      //XMessagePopupDlg.show(0, "Errors in locale file detected", $Text::OK);
      error($LocaleErrors);
   }
   if (GameMissionInfo.dupErrors !$= "")
   {
      %errors = %errors @ "Duplicate Mission Errors:" NL GameMissionInfo.dupErrors NL "";
      //XMessagePopupDlg.show(0, "Duplicate Mission Ids detected", $Text::OK);
      %group = SinglePlayMissionGroup;
      for (%i = 0; %i < %group.getCount(); %i++)
         if (GameMissionInfo.duplevelIds[%group.getObject(%i).level])
            error("LEVEL ID DUP:" SPC %group.getObject(%i).file SPC %group.getObject(%i).level);
         else
            echo(%group.getObject(%i).file SPC %group.getObject(%i).level);
            
      %group = MultiPlayMissionGroup;
      for (%i = 0; %i < %group.getCount(); %i++)
         if (GameMissionInfo.duplevelIds[%group.getObject(%i).level])
            error("LEVEL ID DUP:" SPC %group.getObject(%i).file SPC %group.getObject(%i).level);
         else
            echo(%group.getObject(%i).file SPC %group.getObject(%i).level);
   }
   if ($dupLocGlobals !$= "")
   {
      %errors = %errors @ "Duplicate Localization Variables:" NL $dupLocGlobals NL "";
      //XMessagePopupDlg.show(0, "Duplicate Localization variable detected (press ~ for details)", $Text::OK);
      error($dupLocGlobals);
   }
   
   if (%errors !$= "")
   {
      RootGui.setContent(StartupErrorGui, %errors);
      XMessagePopupDlg.show(0, "Errors detected on startup, press A to view errors", $Text::OK);
   }
   
   //playShellMusic();  
   
   // set global indicating that the game is loaded
   $Client::GameLoaded = true;
}


// Connect to a server using a chosen %method
// Methods:
// 0: Direct Connect
// 1: Arranged Connect through NAT hole punching
// 2: Relay Connect using a relay server
function connectUsing(%address, %method)
{
   $disconnectGui = RootGui.contentGui;
   GameMissionInfo.setMode(GameMissionInfo.MPMode);

   if ($EnableFMS)
   {
      %missionIndex = GameMissionInfo.getCurrentIndex();
      if (%missionIndex == -1)
         %missionIndex = 0;
   
      GameMissionInfo.selectMission(%missionIndex);
   }
   RootGui.setContent(MissionLoadingGui);
    
   echo("ESTABLISH CONNECTION" SPC %address SPC %mp SPC %invited);
   if (isObject(ServerConnection))
      ServerConnection.delete();
      
   // clear the scores gui here so that our the client id from the preview server doesn't
   // show up in the scores list.
   PlayerListGui.clear();
   // reset client Id since we are connecting to a different server
   $Player::ClientId = 0;
   
   %conn = new GameConnection(ServerConnection);
   RootGroup.add(ServerConnection);

   %xbLiveVoice = 0; // XBLiveGetVoiceStatus();
   
   // we expect $Player:: variables to be properly populated at this point   
   %isDemoLaunch = isDemoLaunch();
   %conn.setConnectArgs($Player::Name, $Player::XBLiveId, %xbLiveVoice, %invited, %isDemoLaunch);
   %conn.setJoinPassword($Client::Password);

   $Client::connectedMultiplayer = true;
   $Game::SPGemHunt = false;

   if (%method == 0) { 
      %conn.connect(%address);
   } else if (%method == 1) {
      %conn.arrangeConnection(%address);
   } else if (%method == 2) { 
      %conn.relayConnection(%address);
   }

   clearClientGracePeroid();
}

// Manually connect to server by ip
function connectManual(%address, %local, %invited)
{
   $disconnectGui = RootGui.contentGui;
   
   GameMissionInfo.setMode(GameMissionInfo.MPMode);

   if ($EnableFMS)
   {
      %missionIndex = GameMissionInfo.getCurrentIndex();
   
      if (%missionIndex == -1)
         %missionIndex = 0;
   
      GameMissionInfo.selectMission(%missionIndex);
   }

   RootGui.setContent(MissionLoadingGui);
   if ($EnableFMS)
      establishConnection(%address, true, %local, %invited);
   else
      connectToServer(%address, %invited);
}

// connect to a server.  if address is empty a local connect is assumed
function connectToServer(%address,%invited)
{
    echo("CONNECT TO SERVER" SPC %address SPC %invited);
   if (isObject(ServerConnection))
      ServerConnection.delete();
      
   // clear the scores gui here so that our the client id from the preview server doesn't
   // show up in the scores list.
   PlayerListGui.clear();
   // reset client Id since we are connecting to a different server
   $Player::ClientId = 0;
   
   %conn = new GameConnection(ServerConnection);
   RootGroup.add(ServerConnection);

   %xbLiveVoice = 0; // XBLiveGetVoiceStatus();
   
   // we expect $Player:: variables to be properly populated at this point   
   %isDemoLaunch = isDemoLaunch();
   %conn.setConnectArgs($Player::Name, $Player::XBLiveId, %xbLiveVoice, %invited, %isDemoLaunch);
   %conn.setJoinPassword($Client::Password); 
   
   if (%address $= "")
   {
      // local connections are not considered to be multiplayer connections initially.  
      // they become multiplayer connections when multiplayer mode is started
      $Client::connectedMultiplayer = false;
      %conn.connectLocal();
      setNetPort(0);
   }
   else
   {
      // connecting as a client to remote server
      // need to open up the port so that we can get voice data
      if (!isPCBuild())
      {
         setNetPort(0);
         portInit($Pref::Server::Port);
      }
      $Client::connectedMultiplayer = true;
      $Game::SPGemHunt = false;
      if ($pref::forceDirectConnect)
         %conn.connect(%address);
      else
         %conn.arrangeConnection(%address);
   }

   clearClientGracePeroid();
}

function connectToPreviewServer()
{
   if ($EnableFMS)
   {
      // Delete everything
      if (isObject($ServerGroup))
         $ServerGroup.delete();
      $ServerGroup = new SimGroup(ServerGroup);
      
      if (isObject(MissionGroup))
         MissionGroup.delete();
      
      if (isObject(MissionCleanup))
         MissionCleanup.delete();
   
      // Flip back to SinglePlayerMode
      // The current mission should still be unhidden
      setSinglePlayerMode(true);
   
      // need to end the current game
      commandToServer('SetWaitState');
   
      establishConnection("");
   }
   else
      connectToServer("");
}

// connect to a server.  if address is empty a local connect is assumed
function establishConnection(%address, %mp, %isLocal, %invited)
{
    echo("ESTABLISH CONNECTION" SPC %address SPC %mp SPC %invited);
   if (isObject(ServerConnection))
      ServerConnection.delete();
      
   // clear the scores gui here so that our the client id from the preview server doesn't
   // show up in the scores list.
   PlayerListGui.clear();
   // reset client Id since we are connecting to a different server
   $Player::ClientId = 0;
   
   %conn = new GameConnection(ServerConnection);
   RootGroup.add(ServerConnection);

   %xbLiveVoice = 0; // XBLiveGetVoiceStatus();
   
   // we expect $Player:: variables to be properly populated at this point   
   %isDemoLaunch = isDemoLaunch();
   %conn.setConnectArgs($Player::Name, $Player::XBLiveId, %xbLiveVoice, %invited, %isDemoLaunch);
   %conn.setJoinPassword($Client::Password);
   
   if (%address $= "")
   {
      if (%mp)
      {
         $Client::connectedMultiplayer = true;
         $Game::SPGemHunt = false;
      }
      else
         $Client::connectedMultiplayer = false;
      %conn.connectLocal();
      setNetPort(0);
   }
   else
   {
      // connecting as a client to remote server
      // need to open up the port so that we can get voice data
      if (!isPCBuild())
      {
         setNetPort(0);
         portInit($Pref::Server::Port);
      }
      $Client::connectedMultiplayer = true;
      $Game::SPGemHunt = false;
      if (%isLocal || $pref::forceDirectConnect)
         %conn.connect(%address);
      else
         %conn.arrangeConnection(%address);
   }

   clearClientGracePeroid();
}

function populatePreviewMission()
{
   %start = getRealTime();
   
   $instantGroup = MegaMissionGroup;

   for (%i = 0; %i < SinglePlayMissionGroup.getCount(); %i++)
   {
      %info = SinglePlayMissionGroup.getObject(%i);      
      %mission = fileName(%info.file);

      // First the InteriorInstance's
      %info.missionGroup = loadObjectsFromMission(%mission);

      // Then any camera markers
      loadObjectsFromMission(%mission, "SpawnSphere", "CameraSpawnSphereMarker");

      // Then the glass
      loadObjectsFromMission(%mission, "StaticShape", "glass_3shape");
      
      // Then the glass
      loadObjectsFromMission(%mission, "StaticShape", "glass_6shape");
      
      // Then the glass
      loadObjectsFromMission(%mission, "StaticShape", "glass_9shape");
      
      // Then the glass
      loadObjectsFromMission(%mission, "StaticShape", "glass_12shape");
      
      // Then the glass
      loadObjectsFromMission(%mission, "StaticShape", "glass_15shape");
      
      // Then the glass
      loadObjectsFromMission(%mission, "StaticShape", "glass_18shape");
      
      // TSStatics
      loadObjectsFromMission(%mission, "TSStatic");
   }
   
   for (%i = 0; %i < CustomSinglePlayMissionGroup.getCount(); %i++)
   {
      %info = CustomSinglePlayMissionGroup.getObject(%i);      
      %mission = fileName(%info.file);

      // First the InteriorInstance's
      %info.missionGroup = loadObjectsFromMission(%mission);

      // Then any camera markers
      loadObjectsFromMission(%mission, "SpawnSphere", "CameraSpawnSphereMarker");

      // Then the glass
      loadObjectsFromMission(%mission, "StaticShape", "glass_3shape");
      
      // Then the glass
      loadObjectsFromMission(%mission, "StaticShape", "glass_6shape");
      
      // Then the glass
      loadObjectsFromMission(%mission, "StaticShape", "glass_9shape");
      
      // Then the glass
      loadObjectsFromMission(%mission, "StaticShape", "glass_12shape");
      
      // Then the glass
      loadObjectsFromMission(%mission, "StaticShape", "glass_15shape");
      
      // Then the glass
      loadObjectsFromMission(%mission, "StaticShape", "glass_18shape");
      
      // TSStatics
      loadObjectsFromMission(%mission, "TSStatic");
   }

   for (%i = 0; %i < MultiPlayMissionGroup.getCount(); %i++)
   {
      %info = MultiPlayMissionGroup.getObject(%i);      
      %mission = fileName(%info.file);

      // First the InteriorInstance's
      %info.missionGroup = loadObjectsFromMission(%mission);
      
      // Then any camera markers
      loadObjectsFromMission(%mission, "SpawnSphere", "CameraSpawnSphereMarker");

      // Then the glass
      loadObjectsFromMission(%mission, "StaticShape", "glass_3shape");
      
      // Then the glass
      loadObjectsFromMission(%mission, "StaticShape", "glass_6shape");
      
      // Then the glass
      loadObjectsFromMission(%mission, "StaticShape", "glass_9shape");
      
      // Then the glass
      loadObjectsFromMission(%mission, "StaticShape", "glass_12shape");
      
      // Then the glass
      loadObjectsFromMission(%mission, "StaticShape", "glass_15shape");
      
      // Then the glass
      loadObjectsFromMission(%mission, "StaticShape", "glass_18shape");
      
      // TSStatics
      loadObjectsFromMission(%mission, "TSStatic");
   }
   
   for (%i = 0; %i < SpecialMissionGroup.getCount(); %i++)
   {
      %info = SpecialMissionGroup.getObject(%i);      
      %mission = fileName(%info.file);
      
      // First the InteriorInstance's
      %info.missionGroup = loadObjectsFromMission(%mission);
      
      // Then any camera markers
      loadObjectsFromMission(%mission, "SpawnSphere", "CameraSpawnSphereMarker");

      // Then the SpawnSphere's
      loadObjectsFromMission(%mission, "SpawnSphere", "SpawnSphereMarker");

      // Then the StartPad's
      loadObjectsFromMission(%mission, "StaticShape", "StartPad");

      // Then the glass
      loadObjectsFromMission(%mission, "StaticShape", "glass_3shape");

      // Then the other glass
      loadObjectsFromMission(%mission, "TSStatic");
   }

   %diff = getRealTime() - %start;
   error("Took " @ %diff / 1000 @ " seconds to populate the preview mission");
}

function getMissionSky(%missionFile)
{
   %file = new FileObject();
   
   if ( %file.openForRead( %missionFile ) ) {
		%inInfoBlock = false;
		
		while ( !%file.isEOF() ) {
			%line = %file.readLine();
			%line = trim( %line );
			
			if( %line $= "new ScriptObject(MissionInfo) {" ) {
				%line = "new ScriptObject() {";
				%inInfoBlock = true;
			}
			else if( %inInfoBlock && %line $= "};" ) {
				%inInfoBlock = false;
				break;
			}
			
			if( %inInfoBlock )
			{
			   if (strpos(%line, "type =") != -1)
			   {
			      %type = eval("%" @ getSubStr(%line, strpos(%line, "type ="), 99));
			   }
			   
			   if (strpos(%line, "customType =") != -1)
			   {
			      %customType = eval("%" @ getSubStr(%line, strpos(%line, "customType ="), 99));
			   }
			}
		}
		
		%file.close();
		%file.delete();
	}
   
   if (%type $= "beginner")
      %sky = $sky_beginner;
   if (%type $= "intermediate")
      %sky = $sky_intermediate;
   if (%type $= "advanced")
      %sky = $sky_advanced;
      
   if (%customType $= "beginner")
      %sky = $sky_beginner;
   if (%customType $= "intermediate")
      %sky = $sky_intermediate;
   if (%customType $= "advanced")
      %sky = $sky_advanced;
      
   return %sky;
}

function addTempPreviewMission(%missionFile)
{
   %start = getRealTime();
   
   %oldInstantGroup = $instantGroup;
   $instantGroup = MegaMissionGroup;
   
   %mission = fileName(%missionFile);

   // First the InteriorInstance's
   %missionGroup = loadObjectsFromMission(%mission, "", "", "MegaMissionGroup");

   // Then any camera markers
   loadObjectsFromMission(%mission, "SpawnSphere", "CameraSpawnSphereMarker", "MegaMissionGroup");

   // Then the glass
   loadObjectsFromMission(%mission, "StaticShape", "glass_3shape", "MegaMissionGroup");
   
   // Then the glass
   loadObjectsFromMission(%mission, "StaticShape", "glass_6shape", "MegaMissionGroup");
   
   // Then the glass
   loadObjectsFromMission(%mission, "StaticShape", "glass_9shape", "MegaMissionGroup");
   
   // Then the glass
   loadObjectsFromMission(%mission, "StaticShape", "glass_12shape", "MegaMissionGroup");
   
   // Then the glass
   loadObjectsFromMission(%mission, "StaticShape", "glass_15shape", "MegaMissionGroup");
   
   // Then the glass
   loadObjectsFromMission(%mission, "StaticShape", "glass_18shape", "MegaMissionGroup");
   
   // TSStatics
   loadObjectsFromMission(%mission, "TSStatic", "", "MegaMissionGroup");
   
   $instantGroup = %oldInstantGroup;
   
   %diff = getRealTime() - %start;
   error("Took " @ %diff / 1000 @ " seconds to add mission to preview");
   
   return %missionGroup;
}

function removeTempPreviewMission(%group)
{
   %group.delete();
}

function showTempPreviewMission(%group, %sky)
{
   GameMissionInfo.getCurrentMission().missionGroup.setHidden(true);
   %group.setHidden(false);
   %cameraPoint = getCameraObject(%group);
   %cameraPos = getSpawnPosition(%cameraPoint);
   if (isObject($previewCamera))
      $previewCamera.setTransform(%cameraPos);
   
   if ($curr_sky !$= %sky)
   {
      // First hide our clouds
      if ($curr_sky $= $sky_beginner || $curr_sky $= "")
         Cloud_Beginner.setHidden(true);
      if ($curr_sky $= $sky_intermediate || $curr_sky $= "")
         Cloud_Intermediate.setHidden(true);
      if ($curr_sky $= $sky_advanced || $curr_sky $= "")
         Cloud_Advanced.setHidden(true);

      // Then set the material
      Sky.setSkyMaterial(%sky);
      $curr_sky = %sky;

      // Now unhide our new clouds
      if ($curr_sky $= $sky_beginner)
         Cloud_Beginner.setHidden(false);
      if ($curr_sky $= $sky_intermediate)
         Cloud_Intermediate.setHidden(false);
      if ($curr_sky $= $sky_advanced)
         Cloud_Advanced.setHidden(false);
         
      // Switch our sun properties
      if ($curr_sky $= $sky_beginner)
      {
         Mega_Sun.direction = Beginner_Sun.direction;
         Mega_Sun.color = Beginner_Sun.color;
         Mega_Sun.ambient = Beginner_Sun.ambient;
         Mega_Sun.shadowColor = Beginner_Sun.shadowColor;
      }
      if ($curr_sky $= $sky_intermediate)
      {
         Mega_Sun.direction = Intermediate_Sun.direction;
         Mega_Sun.color = Intermediate_Sun.color;
         Mega_Sun.ambient = Intermediate_Sun.ambient;
         Mega_Sun.shadowColor = Intermediate_Sun.shadowColor;
      }
      if ($curr_sky $= $sky_advanced)
      {
         Mega_Sun.direction = Advanced_Sun.direction;
         Mega_Sun.color = Advanced_Sun.color;
         Mega_Sun.ambient = Advanced_Sun.ambient;
         Mega_Sun.shadowColor = Advanced_Sun.shadowColor;
      }
   }
}

function hideTempPreviewMission(%group)
{
   GameMissionInfo.getCurrentMission().missionGroup.setHidden(false);
   %group.setHidden(true);
   GameMissionInfo.setCamera();
   if ($curr_sky !$= GameMissionInfo.getCurrentMission().sky)
   {
      // First hide our clouds
      if ($curr_sky $= $sky_beginner || $curr_sky $= "")
         Cloud_Beginner.setHidden(true);
      if ($curr_sky $= $sky_intermediate || $curr_sky $= "")
         Cloud_Intermediate.setHidden(true);
      if ($curr_sky $= $sky_advanced || $curr_sky $= "")
         Cloud_Advanced.setHidden(true);

      // Then set the material
      Sky.setSkyMaterial(GameMissionInfo.getCurrentMission().sky);
      $curr_sky = GameMissionInfo.getCurrentMission().sky;

      // Now unhide our new clouds
      if ($curr_sky $= $sky_beginner)
         Cloud_Beginner.setHidden(false);
      if ($curr_sky $= $sky_intermediate)
         Cloud_Intermediate.setHidden(false);
      if ($curr_sky $= $sky_advanced)
         Cloud_Advanced.setHidden(false);
         
      // Switch our sun properties
      if ($curr_sky $= $sky_beginner)
      {
         Mega_Sun.direction = Beginner_Sun.direction;
         Mega_Sun.color = Beginner_Sun.color;
         Mega_Sun.ambient = Beginner_Sun.ambient;
         Mega_Sun.shadowColor = Beginner_Sun.shadowColor;
      }
      if ($curr_sky $= $sky_intermediate)
      {
         Mega_Sun.direction = Intermediate_Sun.direction;
         Mega_Sun.color = Intermediate_Sun.color;
         Mega_Sun.ambient = Intermediate_Sun.ambient;
         Mega_Sun.shadowColor = Intermediate_Sun.shadowColor;
      }
      if ($curr_sky $= $sky_advanced)
      {
         Mega_Sun.direction = Advanced_Sun.direction;
         Mega_Sun.color = Advanced_Sun.color;
         Mega_Sun.ambient = Advanced_Sun.ambient;
         Mega_Sun.shadowColor = Advanced_Sun.shadowColor;
      }
   }
}

function getCameraObject(%missionGroup)
{
   %spawnObj = 0;

   // Attempt to find a SpawnSphere
   %obj = 0;
   %found = false;
   for (%i = 0; %i < %missionGroup.getCount(); %i++)
   {
      %obj = %missionGroup.getObject(%i);

      if (%obj.getClassName() $= "SpawnSphere")
      {
         %found = true;
         break;
      }
      else if (%obj.getClassName() $= "StaticShape")
      {
         if (%obj.getDataBlock().getName() $= "StartPad")
         {
            %found = true;
            break;
         }
      }
   }

   if (%found)
      %spawnObj = %obj;
   else
   {
      error("Was unable to find a camera position in " @ %missionGroup);

      // See if there is one in SpawnPoints
      %group = nameToID("MissionGroup/SpawnPoints");

      if (%group != -1)
      {
        if (%group.getCount() > 0)
         %spawnObj = %group.getObject(0);
      }
   }

   return %spawnObj;
}

function setupCameras()
{
   for (%i = 0; %i < SinglePlayMissionGroup.getCount(); %i++)
   {
      %info = SinglePlayMissionGroup.getObject(%i);
      %info.cameraPoint = getCameraObject(%info.missionGroup);
      %info.cameraPos = getSpawnPosition(%info.cameraPoint);
   }
   
   for (%i = 0; %i < CustomSinglePlayMissionGroup.getCount(); %i++)
   {
      %info = CustomSinglePlayMissionGroup.getObject(%i);
      %info.cameraPoint = getCameraObject(%info.missionGroup);
      %info.cameraPos = getSpawnPosition(%info.cameraPoint);
   }

   for (%i = 0; %i < MultiPlayMissionGroup.getCount(); %i++)
   {
      %info = MultiPlayMissionGroup.getObject(%i);
      %info.cameraPoint = getCameraObject(%info.missionGroup);
      %info.cameraPos = getSpawnPosition(%info.cameraPoint);
   }
   
   for (%i = 0; %i < SpecialMissionGroup.getCount(); %i++)
   {
      %info = SpecialMissionGroup.getObject(%i);
      %info.cameraPoint = getCameraObject(%info.missionGroup);
      %info.cameraPos = getSpawnPosition(%info.cameraPoint);
   }
}

function createPreviewServer(%mission)
{
   %serverType = "SinglePlayer";
   if (%mission $= "")
      %mission = $pref::Client::AutoStartMission;

   if ($EnableFMS)
   {
      // This may seem redundant but it allows a standard server to shut
      // down properly before things are switched to SinglePlayerMode
      $Server::Hosting = false;
      allowConnections(false);
      stopHeartbeat();
      destroyServer();

      // Make the switch
      setSinglePlayerMode(true);
      
      %previewMission = "marble/data/missions/megaMission.mis";
      
      if ($buildMega)
         %previewMission = "marble/data/missions/megaEmpty.mis";

      // Create the server
      if ($buildMega)
      {
         createServer(%serverType, %previewMission);
         populatePreviewMission();
      }
      else
         createServer(%serverType, %previewMission);
      
      // "Save" our ServerGroup and MissionGroup so they don't get deleted
      ServerGroup.setName("MegaServerGroup");
      MissionGroup.setName("MegaMissionGroup");
      $ServerGroup = 0;

      // Grep our camera positions from the MegaMission
      setupCameras();
   }
   else
      createServer(%serverType, %mission);

   connectToPreviewServer();
   
   GameMissionInfo.setMode(GameMissionInfo.SPMode);

   if ($EnableFMS)
   {
      %missionIndex = GameMissionInfo.getCurrentIndex();
   
      if (%missionIndex == -1)
      {
         if (%mission $= "")
            %mission = $pref::Client::AutoStartMission;
         
         %missionIndex = GameMissionInfo.findIndexByPath(%mission);
      }
   
      // Teh hak
      GameMissionInfo.selectMission($pref::Client::AutoStartMissionIndex);
      //GameMissionInfo.selectMission(%missionIndex);
   }
   else
      GameMissionInfo.setCurrentMission(%mission);
}

function createEmptyMission(%interiorArg)
{
   return new SimGroup(MissionGroup) {
      new ScriptObject(MissionInfo) {
         level = "001";
         desc = "A preview mission";
         time = "0";
         include = "1";
         difficulty = "4";
         name = "Preview Mission";
         type = "custom";
         customType = "intermediate";
         goldTime = "0";
         gameType = "SinglePlayer";
         guid = GenerateGuid();
      };
      new MissionArea(MissionArea) {
         area = "-360 -648 720 1296";
         flightCeiling = "300";
         flightCeilingRange = "20";
         locked = "true";
      };
      new Sky(Sky) {
         position = "336 136 0";
         rotation = "1 0 0 0";
         scale = "1 1 1";
         materialList = "~/data/skies/sky_intermediate.dml";
         cloudHeightPer[0] = "0";
         cloudHeightPer[1] = "0";
         cloudHeightPer[2] = "0";
         cloudSpeed1 = "0.0001";
         cloudSpeed2 = "0.0002";
         cloudSpeed3 = "0.0003";
         visibleDistance = "1500";
         fogDistance = "1000";
         fogColor = "0.600000 0.600000 0.600000 1.000000";
         fogStorm1 = "0";
         fogStorm2 = "0";
         fogStorm3 = "0";
         fogVolume1 = "-1 7.45949e-031 1.3684e-038";
         fogVolume2 = "-1 1.07208e-014 8.756e-014";
         fogVolume3 = "-1 5.1012e-010 2.05098e-008";
         windVelocity = "1 0 0";
         windEffectPrecipitation = "0";
         SkySolidColor = "0.600000 0.600000 0.600000 1.000000";
         useSkyTextures = "1";
         renderBottomTexture = "1";
         noRenderBans = "1";
         renderBanOffsetHeight = "50";
         skyGlow = "0";
         skyGlowColor = "0.000000 0.000000 0.000000 0.000000";
            fogVolumeColor2 = "128.000000 128.000000 128.000000 0.000004";
            fogVolumeColor3 = "128.000000 128.000000 128.000000 14435505.000000";
            fogVolumeColor1 = "128.000000 128.000000 128.000000 0.000000";
      };
      new StaticShape(Cloud_Shape) {
         position = "0 0 0";
         rotation = "1 0 0 0";
         scale = "1 1 1";
         hidden = "0";
         reanderShadow = "0";
         dataBlock = "astrolabeCloudsIntermediateShape";
         receiveSunLight = "1";
         receiveLMLighting = "1";
         useCustomAmbientLighting = "0";
         customAmbientLighting = "0 0 0 1";
      };
      new Sun() {
         direction = "-0.573201 -0.275357 -0.771764";
         color = "1.08 1.03 0.9 1";
         ambient = "0.4 0.4 0.5 1";
      };
      new StaticShape() {
         position = "0 0 -500";
         rotation = "1 0 0 0";
         scale = "1 1 1";
         dataBlock = "astrolabeShape";
      };
      new SpawnSphere(CameraObj) {
         position = "-15.7107 -13.117 10.6244";
         rotation = "0.468686 -0.160109 0.868734 42.9325";
         scale = "1 1 1";
         hidden = "0";
         reanderShadow = "1";
         dataBlock = "CameraSpawnSphereMarker";
         radius = "100";
         sphereWeight = "100";
         indoorWeight = "100";
         outdoorWeight = "100";
      };
   };
}
