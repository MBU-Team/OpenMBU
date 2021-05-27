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

//-----------------------------------------------------------------------------
// loadMaterials - load all materials.cs files
//-----------------------------------------------------------------------------
function loadMaterials()
{
   for( %file = findFirstFile( "*/materials.cs" ); %file !$= ""; %file = findNextFile( "*/materials.cs" ))
   {
      exec( %file );
   }
}

function reloadMaterials()
{
   reloadTextures();
   loadMaterials();
   reInitMaterials();
}

//-----------------------------------------------------------------------------
function initClient()
{
   echo("\n--------- Initializing MOD: Torque Demo Client ---------");

   // Make sure this variable reflects the correct state.
   $Server::Dedicated = false;

   // Game information used to query the master server
   $CLient::GameType = "";
   $Client::GameTypeQuery = "";
   $Client::MissionTypeQuery = "Any";

   //
   exec("./ui/customProfiles.cs"); // override the base profiles if necessary

   // The common module provides basic client functionality
   initBaseClient();

   // Always start up the shell in 800x600. If the pref is not set, then it
   // will defaul to 800x600 in the platform code.
   if ($Pref::Video::Resolution !$= "") {
      $width = getWord($pref::Video::resolution,0);
      if ($width < 800) {
         $pref::Video::resolution = "800 600" SPC getWord($pref::Video::resolution,2);
         echo("Forcing resolution to 800 x 600");
      }
   }
   
   // InitCanvas starts up the graphics system.
   // The canvas needs to be constructed before the gui scripts are
   // run because many of the controls assume the canvas exists at
   // load time.
   initCanvas(".: Torque Shader Engine :. (Early Adopter EA1 Demo)");

   /// Load client-side Audio Profiles/Descriptions
   exec("./scripts/audioProfiles.cs");

   // In-game idle timeout
   $IdleTimeout = 30000;

   // Load up the Game GUIs
   exec("./ui/defaultGameProfiles.cs");
   exec("./ui/PlayGui.gui");
   exec("./ui/CameraGui.gui");
   exec("./ui/SceneGui.gui");
   exec("./ui/PageGui.gui");
   exec("./ui/ChatHud.gui");
   exec("./ui/playerList.gui");

   // Load up the shell GUIs
   exec("./ui/overlayDlg.gui");
   exec("./ui/mainMenuGui.gui");
   exec("./ui/aboutDlg.gui");
   exec("./ui/startMissionGui.gui");
   exec("./ui/joinServerGui.gui");
   exec("./ui/loadingGui.gui");
   exec("./ui/endGameGui.gui");
   exec("./ui/optionsDlg.gui");
   exec("./ui/remapDlg.gui");
   exec("./ui/StartupGui.gui");
   exec("./ui/ProductGui.gui");

   // Demo page & scene gui
   exec("./ui/MainMenuDlg.gui");
   exec("./ui/overview_main.gui");
   
   exec("./ui/features/features_main.gui");
   exec("./ui/features/features_script.gui");
   exec("./ui/features/features_gui.gui");
   exec("./ui/features/features_net.gui");
   exec("./ui/features/features_render.gui");
   exec("./ui/features/features_shader.gui");
   exec("./ui/features/features_terrain.gui");
   exec("./ui/features/features_interior.gui");
   exec("./ui/features/features_mesh.gui");
   exec("./ui/features/features_water.gui");
   exec("./ui/features/features_sound.gui");
   
   exec("./ui/tools/tools_main.gui");
   exec("./ui/tools/tools_gui.gui");
   exec("./ui/tools/tools_world.gui");
   exec("./ui/tools/tools_terrain.gui");
   exec("./ui/tools/tools_texture.gui");
   exec("./ui/tools/tools_heightfield.gui");
   
   exec("./ui/products/product_main.gui");
   exec("./ui/products/product_tribes2.gui");
   exec("./ui/products/product_hunting.gui");
   exec("./ui/products/product_marbleblast.gui");
   exec("./ui/products/product_thinktanks.gui");
   exec("./ui/products/product_tenniscritters.gui");
   exec("./ui/products/product_orbz.gui");   
   
   exec("./ui/testimonials/community.gui");   
   exec("./ui/testimonials/testimonials_main.gui");
   
   exec("./ui/license/publishing.gui");
   exec("./ui/license/license_main.gui");
   exec("./ui/license/license_indie.gui");
   exec("./ui/license/license_corp.gui");
   exec("./ui/license/license_other.gui");
   
   exec("./ui/garagegames/garagegames_main.gui");

   exec("./ui/AnimationSceneGui.gui");
   exec("./ui/DetailSceneGui.gui");
   exec("./ui/MountingSceneGui.gui");
   
   // Client scripts
   exec("./scripts/client.cs");
   exec("./scripts/game.cs");
   exec("./scripts/missionDownload.cs");
   exec("./scripts/serverConnection.cs");
   exec("./scripts/playerList.cs");
   exec("./scripts/loadingGui.cs");
   exec("./scripts/optionsDlg.cs");
   exec("./scripts/chatHud.cs");
   exec("./scripts/messageHud.cs");
   exec("./scripts/mainMenuGui.cs");
   exec("./scripts/playGui.cs");
   exec("./scripts/cameraGui.cs");
   exec("./scripts/fpsGui.cs");
   exec("./scripts/racingGui.cs");
   exec("./scripts/sceneGui.cs");
   exec("./scripts/joinServerGui.cs");
   exec("./scripts/startMissionGui.cs");
   exec("./scripts/centerPrint.cs");

   // load menu thread data
   exec("./menu_threads.cs");

   // Default player key bindings
   exec("./scripts/default.bind.cs");
   exec("./config.cs");

   // material related
   exec("./scripts/glowBuffer.cs");
   exec("./scripts/precip.cs");
   exec("common/client/shaders.cs");
   exec("./scripts/shaders.cs");
   exec("./scripts/commonMaterialData.cs" );
   
   initRenderInstManager();

   // Really shouldn't be starting the networking unless we are
   // going to connect to a remote server, or host a multi-player
   // game.
   setNetPort(0);

   // Copy saved script prefs into C++ code.
   setShadowDetailLevel( $pref::shadows );
   setDefaultFov( $pref::Player::defaultFov );
   setZoomSpeed( $pref::Player::zoomSpeed );

   // Start up the main menu... this is separated out into a 
   // method for easier mod override.

   if ($JoinGameAddress !$= "") {
      // If we are instantly connecting to an address, load the
      // main menu then attempt the connect.
      loadMainMenu();
      connect($JoinGameAddress, "", $Pref::Player::Name);
   }
   else {
      // Otherwise go to the splash screen.
      Canvas.setCursor("DefaultCursor");
      loadStartup();
   }
}


//-----------------------------------------------------------------------------

function loadMainMenu()
{
   // Startup the client with the Main menu...
   Canvas.setContent( MainMenuGui );
   checkAudioInit();
   checkVideoInit();

   Canvas.setCursor("DefaultCursor");
}

function loadFeatureMission()
{
   loadMaterials();
   
   // Display the loading GUI
   Canvas.setContent(LoadingGui);
   LOAD_MapName.setText( "Creating Feature Game" );
   LOAD_MapDescription.setText( "<font:Arial:16>Please wait while the feature game is started...");
   Canvas.repaint();

   // Start up the server..
   createServer("SinglePlayer", "demo/data/missions/features.mis");
   %conn = new GameConnection(ServerConnection);
   RootGroup.add(ServerConnection);
   %conn.setConnectArgs($pref::Player::Name);
   %conn.setJoinPassword($Client::Password);
   %conn.connectLocal();
}

function checkAudioInit()
{
   if($Audio::initFailed)
   {
      MessageBoxOK("Audio Initialization Failed", 
         "The OpenAL audio system failed to initialize.  " @
         "You can get the most recent OpenAL drivers <a:www.garagegames.com/docs/torque/gstarted/openal.html>here</a>.");
   }
}   

function checkVideoInit()
{
   if($GFX::OutdatedDrivers)
   {
      MessageBoxOK("Old Video Drivers",
         "You have old video drivers.  " @
         "TSE uses many advanced rendering features, and older drivers may not support them correctly. " @
         "YOU SHOULD UPDATE YOUR DRIVERS BEFORE PROCEEDING.  " @
         $GFX::OutdatedDriversLink);
   }
}
