//-----------------------------------------------------------------------------
// Torque Shader Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Load up defaults console values.

exec("./defaults.cs");

// Unicode test strings
exec("./client/unicodeStrings.cs");

//-----------------------------------------------------------------------------

function initCommon()
{
   // All mods need the random seed set
   setRandomSeed();

   // Very basic functions used by everyone
   exec("./client/canvas.cs");
   exec("./client/audio.cs");
}

function initBaseClient()
{
   // Base client functionality
   exec("./client/message.cs");
   exec("./client/mission.cs");
   exec("./client/missionDownload.cs");
   exec("./client/actionMap.cs");

   // There are also a number of support scripts loaded by the canvas
   // when it's first initialized.  Check out client/canvas.cs
}

function initBaseServer()
{
   // Base server functionality
   exec("./server/audio.cs");
   exec("./server/server.cs");
   exec("./server/message.cs");
   exec("./server/commands.cs");
   exec("./server/missionInfo.cs");
   exec("./server/missionLoad.cs");
   exec("./server/missionDownload.cs");
   exec("./server/clientConnection.cs");
   exec("./server/kickban.cs");
   exec("./server/game.cs");
}   


//-----------------------------------------------------------------------------
package Common {

function displayHelp() {
   Parent::displayHelp();
   error(
      "Common Mod options:\n"@
      "  -fullscreen            Starts game in full screen mode\n"@
      "  -windowed              Starts game in windowed mode\n"@
      "  -autoVideo             Auto detect video, but prefers OpenGL\n"@
      "  -openGL                Force OpenGL acceleration\n"@
      "  -directX               Force DirectX acceleration\n"@
      "  -voodoo2               Force Voodoo2 acceleration\n"@
      "  -noSound               Starts game without sound\n"@
      "  -prefs <configFile>    Exec the config file\n"
   );
}

function parseArgs()
{
   Parent::parseArgs();

   // Arguments override defaults...
   for (%i = 1; %i < $Game::argc ; %i++)
   {
      %arg = $Game::argv[%i];
      %nextArg = $Game::argv[%i+1];
      %hasNextArg = $Game::argc - %i > 1;
   
      switch$ (%arg)
      {
         //--------------------
         case "-fullscreen":
            $pref::Video::fullScreen = 1;
            $argUsed[%i]++;

         //--------------------
         case "-windowed":
            $pref::Video::fullScreen = 0;
            $argUsed[%i]++;

         //--------------------
         case "-noSound":
            error("no support yet");
            $argUsed[%i]++;

         //--------------------
         case "-openGL":
            $pref::Video::displayDevice = "OpenGL";
            $argUsed[%i]++;

         //--------------------
         case "-directX":
            $pref::Video::displayDevice = "D3D";
            $argUsed[%i]++;

         //--------------------
         case "-voodoo2":
            $pref::Video::displayDevice = "Voodoo2";
            $argUsed[%i]++;

         //--------------------
         case "-autoVideo":
            $pref::Video::displayDevice = "";
            $argUsed[%i]++;

         //--------------------
         case "-prefs":
            $argUsed[%i]++;
            if (%hasNextArg) {
               exec(%nextArg, true, true);
               $argUsed[%i+1]++;
               %i++;
            }
            else
               error("Error: Missing Command Line argument. Usage: -prefs <path/script.cs>");
      }
   }
}

function onStart()
{
   Parent::onStart();
   echo("\n--------- Initializing MOD: Common ---------");
   initCommon();
}

function onExit()
{
   echo("Exporting client prefs");
   export("$pref::*", "./client/prefs.cs", False);

   echo("Exporting server prefs");
   export("$Pref::Server::*", "./server/prefs.cs", False);
   BanList::Export("./server/banlist.cs");

   OpenALShutdown();
   Parent::onExit();
}

}; // Common package
activatePackage(Common);
