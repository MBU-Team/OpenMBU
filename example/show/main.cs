//-----------------------------------------------------------------------------
// Torque Shader Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

// Load up common script base
loadDir("common");

// For the show tool to have access to shapes and art in your game,
// it must be run as a mod on top of your game.

//-----------------------------------------------------------------------------

function initShow()
{
   $showAutoDetail = false;

   exec("~/ui/tsShowGui.gui");
   exec("~/ui/tsShowLoadDlg.gui");
   exec("~/ui/tsShowMiscDlg.gui");
   exec("~/ui/tsShowThreadControlDlg.gui");
   exec("~/ui/tsShowEditScale.gui");
   exec("~/ui/tsShowLightDlg.gui");
   exec("~/ui/tsShowTransitionDlg.gui");
   exec("~/ui/tsShowTranDurEditDlg.gui");
   exec("~/ui/tsShowDetailControlDlg.gui");
   exec("~/scripts/show.bind.cs");
   
   loadMaterials();   
}   


//-----------------------------------------------------------------------------

function stopShow()
{
   Canvas.popDialog(TSShowThreadControlDlg); 
   Canvas.popDialog(TSShowTransitionDlg);
   Canvas.popDialog(TSShowLoadDialog); 
   Canvas.popDialog(TSShowLightDlg); 
   Canvas.popDialog(TSShowMiscDialog); 
   Canvas.popDialog(TShowEditScale);
   Canvas.popDialog(TSShowDetailControlDlg);

   showMoveMap.pop(); 
   quit();
}


//-----------------------------------------------------------------------------

function startShow()
{
   // The client mod has already set it's own content, but we'll
   // just load something new.
   Canvas.setContent(TSShowGui);
   Canvas.setCursor("DefaultCursor");

   $showShapeList = nextToken(trim($showShapeList), shape, " ");
   while (%shape !$= "")
   {
      showShapeLoad(%shape);
      $showShapeList = nextToken($showShapeList, shape, " ");
   }
}


//-----------------------------------------------------------------------------
// Package overrides to initialize the mod.
// This module currently loads on top of the client mod, but it probably
// doesn't need to.  Should look into having disabling the client and
// doing our own canvas init.
package Show {

function displayHelp() {
   Parent::displayHelp();
   error(
      "Show Mod options:\n"@
      "  -show <shape>          Launch the TS show tool\n"@
      "  -file <shape>          Load the mission\n"
   );
}

function parseArgs()
{
   Parent::parseArgs();
   exec("./defaults.cs");
   exec("./prefs.cs");

   for (%i = 1; %i < $Game::argc ; %i++)
   {
      %arg = $Game::argv[%i];
      %nextArg = $Game::argv[%i+1];
      %hasNextArg = $Game::argc - %i > 1;
   
      switch$ (%arg)
      {
         //-------------------
         case "-show":
            $argUsed[%i]++;
            // The "-mod show" shortcut can have an optional file argument
            if (%hasNextArg && strpos(%nextArg, "-") == -1)
            {
               $showShapeList = $showShapeList @ " " @ %nextArg;
               $argUsed[%i+1]++;
               %i++;
            }

         //-------------------
         case "-file":
            // Load a file
            $argUsed[%i]++;
            if (%hasNextArg)
            {
               $showShapeList = $showShapeList @ " " @ %nextArg;
               $argUsed[%i+1]++;
               %i++;
            }
            else
               error("Error: Missing Command Line argument. Usage: -file <file_name>");
      }
   }
}

function onStart()
{
   Parent::onStart();
   echo("\n--------- Initializing MOD: Show ---------");

   if (!isObject(Canvas))  {
      // If the parent onStart didn't open a canvas, then we're
      // probably not running as a mod.  We'll have to do the work
      // ourselves.
      initCanvas("Torque Show Tool");
   }
   initShow();
   startShow();
}

}; // Show package
activatePackage(Show);
