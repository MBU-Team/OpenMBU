//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

dbgSetParameters(8777, "foo");

$defaultGame = "marble";
$displayHelp = false;

$vidAlreadySet = false;

$pref::Console::alwaysUseDebugOutput = true;

//-----------------------------------------------------------------------------
// Support functions used to manage the mod string

function pushFront(%list, %token, %delim)
{
   if (%list !$= "")
      return %token @ %delim @ %list;
   return %token;
}

function pushBack(%list, %token, %delim)
{
   if (%list !$= "")
      return %list @ %delim @ %token;
   return %token;
}

function popFront(%list, %delim)
{
   return nextToken(%list, unused, %delim);
}

// Some localization debug helpers
$locCharacterSet = ANSI;
$locLanguage = ""; 

//------------------------------------------------------------------------------
// Process command line arguments
for ($i = 1; $i < $Game::argc ; $i++)
{
   $arg = $Game::argv[$i];
   $nextArg = $Game::argv[$i+1];
   $hasNextArg = $Game::argc - $i > 1;
   $logModeSpecified = false;
   
   switch$ ($arg)
   {
      case "-test":
         $skipLogos = true;
         $i++;
      case "-language":
         $argUsed[$i]++;
         if( $hasNextArg )
         {
            switch$( $nextArg )
            {
               case "french":
                  $locLanguage = "french";
                  $locCharacterSet = ANSI;
               case "german":
                  $locLanguage = "german";
                  $locCharacterSet = ANSI;
               case "italian":
                  $locLanguage = "italian";
                  $locCharacterSet = ANSI;
               case "japanese":
                  $locLanguage = "japanese";
                  $locCharacterSet = SHIFTJIS;
               case "korean":
                  $locLanguage = "korean";
                  $locCharacterSet = SHIFTJIS;
               case "portuguese":
                  $locLanguage = "portuguese";
                  $locCharacterSet = ANSI;
               case "spanish":
                  $locLanguage = "spanish";
                  $locCharacterSet = ANSI;
               case "chinese":
                  $locLanguage = "chinese";
                  $locCharacterSet = CHINESEBIG5;
               case "polish":
                  $locLanguage = "polish";
                  $locCharacterSet = ANSI;
            }
            $argUsed[$i+1]++;
            $i++;
         }
         else
            error( "Error: Missing Command Line argument. Usage: -language <english|french|german|italian|japanese|korean|portuguese|spanish|chinese|polish>" );
      //--------------------
      case "-log":
         $argUsed[$i]++;
         if ($hasNextArg)
         {
            // Turn on console logging
            if ($nextArg != 0)
            {
               // Dump existing console to logfile first.
               $nextArg += 4;
            }
            setLogMode($nextArg);
            $logModeSpecified = true;
            $argUsed[$i+1]++;
            $i++;
         }
         else
            error("Error: Missing Command Line argument. Usage: -log <Mode: 0,1,2>");

      //--------------------
      case "-mod":
         $argUsed[$i]++;
         if ($hasNextArg)
         {
            // Append the mod to the end of the current list
            $userMods = strreplace($userMods, $nextArg, "");
            $userMods = pushFront($userMods, $nextArg, ";");
            $argUsed[$i+1]++;
            $i++;
	    $modcount++;
         }
         else
            error("Error: Missing Command Line argument. Usage: -mod <mod_name>");
            
      //--------------------
      case "-game":
         $argUsed[$i]++;
         if ($hasNextArg)
         {
            // Remove all mods, start over with game
            $userMods = $nextArg;
            $argUsed[$i+1]++;
            $i++;
	    $modcount = 1;
         }
         else
            error("Error: Missing Command Line argument. Usage: -game <game_name>");
            
      //--------------------
      case "-show":
         // A useful shortcut for -mod show
         $userMods = strreplace($userMods, "show", "");
         $userMods = pushFront($userMods, "show", ";");
         $argUsed[$i]++;
	 $modcount++;

      //--------------------
      case "-console":
         enableWinConsole(true);
         $argUsed[$i]++;

      //--------------------
      case "-jSave":
         $argUsed[$i]++;
         if ($hasNextArg)
         {
            echo("Saving event log to journal: " @ $nextArg);
            saveJournal($nextArg);
            $argUsed[$i+1]++;
            $i++;
         }
         else
            error("Error: Missing Command Line argument. Usage: -jSave <journal_name>");

      //--------------------
      case "-jPlay":
         $argUsed[$i]++;
         if ($hasNextArg)
         {
            playJournal($nextArg,false);
            $argUsed[$i+1]++;
            $i++;
         }
         else
            error("Error: Missing Command Line argument. Usage: -jPlay <journal_name>");

      //--------------------
      case "-jDebug":
         $argUsed[$i]++;
         if ($hasNextArg)
         {
            playJournal($nextArg,true);
            $argUsed[$i+1]++;
            $i++;
         }
         else
            error("Error: Missing Command Line argument. Usage: -jDebug <journal_name>");

      //-------------------
      case "-help":
         $displayHelp = true;
         $argUsed[$i]++;

      //--------------------
      case "-wide":
         $pref::Video::resolution = "1280 720 32";
         $vidAlreadySet = true;
         if ($hasNextArg) 
         {
            $argUsed[%i+1]++;
            %i++;
            switch ($nextArg)
            {
               case 0 : 
                  $pref::Video::resolution = "1280 720 32";
               case 1 : 
                  $pref::Video::resolution = "848 480 32";
               case 2 : 
                  $pref::Video::resolution = "1280 768 32";
               case 3 : 
                  $pref::Video::resolution = "1360 768 32";
               case 4 :
                  $pref::Video::resolution = "1920 1080 32";
            }
            echo("Setting wide screen mode using resolution of" SPC $pref::Video::resolution);
         }

      //--------------------
      case "-norm":
         $pref::Video::resolution = "640 480 32";
         $vidAlreadySet = true;
         if ($hasNextArg) 
         {
            $argUsed[%i+1]++;
            %i++;
            switch ($nextArg)
            {
               case 0 : 
                  $pref::Video::resolution = "640 480 32";
               case 1 : 
                  $pref::Video::resolution = "1024 768 32";
               case 2 : 
                  $pref::Video::resolution = "1280 1024 32";
               case 3 :
                  $pref::Video::resolution = "640 576 32";
               case 4 :
                  $pref::Video::resolution = "753 565 32";                  
            }
            
            echo("Setting normal screen mode using resolution of" SPC $pref::Video::resolution);
         }

      //-------------------
      default:
         $argUsed[$i]++;
         if($userMods $= "")
            $userMods = $arg;
   }
}

echo( "LOCALIZATION: Using language " @ $locLanguage @ " with character set " @ $locCharacterSet );

if( $modcount == 0 ) 
{
   $userMods = $defaultGame;
   $modcount = 1;
}

// DO NOT Run the Torque Creator mod if a dedicated server.
// Note: this fails if -dedicated not first parameter.
if( !( $Game::argc > 1 && $Game::argv[1] $= "-dedicated" ) && $platform !$= "xenon" && $platform !$= "xbox" )
{
   $modcount++;
   $userMods = "creator;" @ $userMods;
}

// Test
$userMods = "shaders;" @ $userMods;
$modcount++;

//-----------------------------------------------------------------------------
// The displayHelp, onStart, onExit and parseArgs function are overriden
// by mod packages to get hooked into initialization and cleanup. 

function onStart()
{
   // Default startup function
}

function onExit()
{
   // OnExit is called directly from C++ code, whereas onStart is
   // invoked at the end of this file.
}

function parseArgs()
{
   // Here for mod override, the arguments have already
   // been parsed.
}   

package Help {
   function onExit() {
      // Override onExit when displaying help
   }
};

function displayHelp() {
   activatePackage(Help);

      // Notes on logmode: console logging is written to console.log.
      // -log 0 disables console logging.
      // -log 1 appends to existing logfile; it also closes the file
      // (flushing the write buffer) after every write.
      // -log 2 overwrites any existing logfile; it also only closes
      // the logfile when the application shuts down.  (default)

   error(
      "Torque Demo command line options:\n"@
      "  -log <logmode>         Logging behavior; see main.cs comments for details\n"@
      "  -game <game_name>      Reset list of mods to only contain <game_name>\n"@
      "  <game_name>            Works like the -game argument\n"@
      "  -mod <mod_name>        Add <mod_name> to list of mods\n"@
      "  -console               Open a separate console\n"@
      "  -show <shape>          Launch the TS show tool\n"@
      "  -jSave  <file_name>    Record a journal\n"@
      "  -jPlay  <file_name>    Play back a journal\n"@
      "  -jDebug <file_name>    Play back a journal and issue an int3 at the end\n"@
      "  -help                  Display this help message\n"
   );
}


//--------------------------------------------------------------------------

function startProfile()
{
   echo("Starting profile session...");
   profilerDump();
   profilerEnable(true);
}

function stopProfile()
{
   echo("Ending profile session...");
   profilerDump();
   profilerEnable(false);
}

// Default to a new logfile each session.
if( !$logModeSpecified )
{
   if( $platform !$= "xbox" && $platform !$= "xenon" )
      setLogMode(6);
}

// Set the mod path which dictates which directories will be visible
// to the scripts and the resource engine.
setModPaths($userMods);

function initVideo()
{
   $pref::Video::displayDevice = "D3D";
   $pref::Video::allowOpenGL = 1;
   $pref::Video::allowD3D = 1;
   $pref::Video::preferOpenGL = 1;
   $pref::Video::appliedPref = 0;
   $pref::Video::VSync = 1;
   $pref::Video::monitorNum = 0;
   $pref::Video::fullScreen = "0";

   if (!$vidAlreadySet)
   {
      // We need to load video prefs
      execPrefs("prefs.cs");   
      
      if ($pref::Video::resolution $= "")
         $pref::Video::resolution = "1280 720 32";
         
      if ($pref::Video::windowedRes $= "")
         $pref::Video::windowedRes = "1280 720";
         
      if ($pref::Video::FSAALevel $= "")
         $pref::Video::FSAALevel = 0;
   }

   $canvasCreated = createCanvas("Marble Blast Ultra! - 1.17.4");
   
   // We need to set up Gamma Ramp here so that the splash screens are affected.
   
   //-----------------------------------------------------------------------------
   // Gamma Buffer Datablock
   //-----------------------------------------------------------------------------
   new ShaderData( GammaShader )
   {
      DXVertexShaderFile 	= "shaders/gammaV.hlsl";
      DXPixelShaderFile 	= "shaders/gammaP.hlsl";
      pixVersion = 2.0;
   };

   new GammaBuffer(GammaBufferData)
   {
      shader = GammaShader;
      bitmap = "marble/data/gammaRamp";
   };
   //-----------------------------------------------------------------------------

   new GuiControlProfile (SplashLoadingProfile)
   {
      tab = false;
      canKeyFocus = false;
      hasBitmapArray = false;
      mouseOverSelected = false;

      // fill color
      opaque = false;
      fillColor = "0 0 0";

      // border color
      border = false;

      // font
      fontType = "";
      
      // bitmap information
      bitmap = "";
      bitmapBase = "";
      textOffset = "0 0";
   };
   new GuiControlProfile (SplashLoadingProgressProfile)
   {
      tab = false;
      canKeyFocus = false;
      hasBitmapArray = false;
      mouseOverSelected = false;

      // fill color
      opaque = true;
      fillColor = "255 255 255";

      // border color
      border = true;
      borderColor = "0 0 0";

      // font
      fontType = "";
      
      // bitmap information
      bitmap = "";
      bitmapBase = "";
      textOffset = "0 0";
   };

   new GuiBitmapCtrl (SplashLoadingGui)
   {
      profile = "SplashLoadingProfile";
      horizSizing = "width";
      vertSizing = "height";
      position = "0 0";
      extent = "640 480";
      bitmap = "marble/client/ui/EngineSplashBG.jpg";

      new GuiControl ()
      {
         profile = "SplashLoadingProfile";
         horizSizing = "center";
         vertSizing = "center";
         position = "0 0";
         extent = "640 480";
         new GuiBitmapCtrl(SplashGGLogoBitmapGui)
         {
            profile = "SplashLoadingProfile";
            horizSizing = "center";
            vertSizing = "center";
            position = "64 40";
            extent = "512 393";
            minExtent = "8 8";
            bitmap = "marble/client/ui/GG_Logo.png";
         };
         new GuiBitmapCtrl (SplashTSELogoBitmapGui)
         {
            profile = "SplashLoadingProfile";
            horizSizing = "right";
            vertSizing = "bottom";
            position = "100 72";
            extent = "435 352";
            minExtent = "8 8";
            bitmap = "marble/client/ui/EngineSplash.png";
         };
         new GuiProgressCtrl (SplashLoadingProgress)
         {
            profile = "SplashLoadingProgressProfile";
            horizSizing = "width";
            vertSizing = "bottom";
            position = "100 100";
            extent = "500 50";
         };
         new GuiProgressCtrl (SplashDecompressionProgress)
         {
            profile = "SplashLoadingProgressProfile";
            horizSizing = "width";
            vertSizing = "bottom";
            position = "100 200";
            extent = "500 50";
         };
         new GuiProgressCtrl (SplashAddingProgress)
         {
            profile = "SplashLoadingProgressProfile";
            horizSizing = "width";
            vertSizing = "bottom";
            position = "100 300";
            extent = "500 50";
         };
      };
   };
   
   Canvas.setContent(SplashLoadingGui);
   SplashLoadingProgress.setVisible(false);
   SplashDecompressionProgress.setVisible(false);
   SplashAddingProgress.setVisible(false);
   SplashGGLogoBitmapGui.setVisible(true);
   SplashTSELogoBitmapGui.setVisible(false);
   Canvas.repaint();
}

// Execute startup scripts for each mod, starting at base and working up
function loadDir(%dir)
{
   setModPaths(pushback($userMods, %dir, ";"));
   exec(%dir @ "/main.cs");
}

function loadMods(%modPath)
{
   %modPath = nextToken(%modPath, token, ";");
   if (%modPath !$= "")
      loadMods(%modPath);

   if(exec(%token @ "/main.cs") != true){
      error("Error: Unable to find specified mod: " @ %token );
      $modcount--;
   }
}
   
function continueStartup()
{
   // Get the first mod on the list, which will be the last to be applied... this
   // does not modify the list.
   nextToken($userMods, currentMod, ";");

   echo("--------- Loading MODS ---------");
   loadMods($userMods);
   echo("");

   if($modcount == 0) {
      enableWinConsole(true);
      error("Error: Unable to load any specified mods");
      quit();	
   }
   // Parse the command line arguments
   echo("--------- Parsing Arguments ---------");
   parseArgs();

   // Either display the help message or startup the app.
   if ($displayHelp) {
      enableWinConsole(true);
      displayHelp();
      quit();
   }
   else {
      onStart();
      echo("Engine initialized...");
   }

   // Display an error message for unused arguments
   for ($i = 1; $i < $Game::argc; $i++)  {
      if (!$argUsed[$i])
         error("Error: Unknown command line argument: " @ $Game::argv[$i]);
   }
}

function loaderSetEngineLogo()
{
   SplashGGLogoBitmapGui.setVisible(false);
   SplashTSELogoBitmapGui.setVisible(true);
   SplashTSELogoBitmapGui.setBitmap("marble/client/ui/EngineSplash.png");
   
   schedule(1000, 0, continueStartup);
}

initVideo();

if ($skipLogos)
{
   continueStartup();
} else
{
   schedule(3000, 0, loaderSetEngineLogo);
}
