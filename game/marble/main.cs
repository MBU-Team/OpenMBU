//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

// Load up common script base
loadDir("common");

//-----------------------------------------------------------------------------
// Load up defaults console values.

// Defaults console values
exec("./client/defaults.cs");
exec("./server/defaults.cs");

//-----------------------------------------------------------------------------
// Package overrides to initialize the mod.
package marble {

function displayHelp() {
   Parent::displayHelp();
   error(
      "Marble Mod options:\n"@
      "  -dedicated             Start as dedicated server\n"@
      "  -connect <address>     For non-dedicated: Connect to a game at <address>\n" @
      "  -mission <filename>    For dedicated or non-dedicated: Load the mission\n" @
      "  -test <.dif filename>  Test an interior map file\n"
   );
}

function parseArgs()
{
   // Call the parent
   Parent::parseArgs();

   // Arguments, which override everything else.
   for (%i = 1; %i < $Game::argc ; %i++)
   {
      %arg = $Game::argv[%i];
      %nextArg = $Game::argv[%i+1];
      %hasNextArg = $Game::argc - %i > 1;
   
      switch$ (%arg)
      {
         //--------------------
         case "-dedicated":
            $Server::Dedicated = true;
            enableWinConsole(true);
            $argUsed[%i]++;

         //--------------------
         case "-mission":
            $argUsed[%i]++;
            if (%hasNextArg) {
               $testLevel = true;
               $missionArg = %nextArg;
               $argUsed[%i+1]++;
               %i++;
            }
            else
               error("Error: Missing Command Line argument. Usage: -mission <filename>");

         //--------------------
         case "-connect":
            $argUsed[%i]++;
            if (%hasNextArg) {
               $JoinGameAddress = %nextArg;
               $argUsed[%i+1]++;
               %i++;
            }
            else
               error("Error: Missing Command Line argument. Usage: -connect <ip_address>");
         case "-test":
            $argUsed[%i]++;
            if(%hasNextArg) {
               $testCheats = true;
               $testLevel = true;
               $interiorArg = %nextArg;
               $argUsed[%i+1]++;
               %i++;
               error("Building mission file for" SPC $interiorArg);
            }
            else
               error("Error: Missing Command Line argument. Usage: -test <interior filename>");
         case "-editor":
            $disablePreviews = true;
            $testCheats = true;
            $argUsed[%i]++;
         //--------------------
         case "-buildMega":
            $buildMega = true;
         //--------------------
         case "-cheats":
            $testCheats = true;
            $argUsed[%i]++;
            
         //--------------------
         case "-titlesafe":
            $showtitlesafearea = 1;
      }
   }
}


// Magical mission dekludgery.
function prepareMissionDekludger()
{
   %filename = "common/local/englishStrings.inf";
   
   echo("MissionDekludger - Loading Locale Inf file:" SPC %filename);
   %fo = new FileObject();
   
   if (!%fo.openForRead(%filename))
   {
      error("   File not found:" SPC %filename);
      %fo.delete();
      return;
   }
   
   // Build a magical lookup array of strings so we can
   // determine what variables they correspond to.
   $Dekludger::VarName[0] = "";
   $Dekludger::VarValue[0] = "";
   $Dekludger::VarCount = 0;
   
      
      while(!%fo.isEOF())
      {
         %line = %fo.readLine();
         %line = trim(%line);
         
         // if the line is empty, ignore
         if (%line $= "")
         {
            continue;
         }
            
         // if its a section header, ignore
         // This was changed for UTF8 lead-in characters
         //if (startswith(%line, "[") && endswith(%line, "]"))
         if (endswith(%line, "]"))
         {
            continue;
         }
         
         // if its a comment, ignore
         if (startswith(%line, ";"))
         {
            continue;
         }
            
         // chop off anything after the last semicolon (treat it as a comment)
         %semiPos = strrchrpos(%line, ";");
         
         if (%semiPos != -1)
            %line = getSubStr(%line, 0, %semiPos + 1); // + 1 includes the semicolon
         
         // it should now be a valid "$var = value" torquescript string...but we'll give them some slack if they
         // are missing a semicolon
         if (!endswith(%line, ";"))
            %line = %line @ ";";

        // Cut up into sections.
        
        %eqPos = strrchrpos( %line, "=" );
        if( %eqPos != -1 )
        {
           %varId = getSubStr(%line, 0, %eqPos);
           %varId = trim( %varId );
           
           // Ok, get the remainder, the string..
           %varValue = trim(getSubStr(%line, %eqPos + 2, strlen(%line) - (%eqPos + 2 + 1)));
           
           // And store it.
           echo("  -- " @ %varId @ " = " @ %varValue @ ";");
           $Dekludger::VarName [$Dekludger::VarCount] = %varId;
           $Dekludger::VarValue[$Dekludger::VarCount] = %varValue;
           $Dekludger::VarCount++;
        }
    }
      
   %fo.delete();
}

function processPossibleVarMatchDekludge(%line)
{
   // %in is of format:
   //     foo = "bar"

   %eqPos = strrchrpos( %line, "=" );   
   
   // Try all our matches against the part after the equals and
   // see if we get any matches. If so, rocking!

   %varId = getSubStr(%line, 0, %eqPos);
   %varId = trim( %varId );

   // Ok, get the remainder, the string..
   %varValue = trim(getSubStr(%line, %eqPos + 2, strlen(%line) - (%eqPos + 2 + 1)));
   
   // Try against %varValue
   for(%i = 0; %i < $Dekludger::VarCount; %i++)
      if(!stricmp(%varValue, $Dekludger::VarValue[%i]))
         return (%varId SPC " = " SPC $Dekludger::VarName[%i] @ ";");
   
   return %line;
}

function repairMissionKludgeryInFile(%filename)
{
   echo("Processing mission file:" SPC %filename);
   
   // Basically, first process into a temporary file,
   // then copy the temp file into the new one.

   %fo = new FileObject();
   
   if (!%fo.openForRead(%filename))
   {
      error("   File not found:" SPC %filename);
      %fo.delete();
      return;
   }
   
   %fOut = new FileObject();
   
   if(!%fOut.openForWrite(%filename @ ".tmp"))
   {
      error("   File not writable:" SPC %filename @ ".tmp");
      %fOut.delete();
      %fo.delete();
      return;
   }
   
   // Done opening, time to process.

   %active = false;
   while(!%fo.isEOF())
   {
      %realLine = %fo.readLine();
      %line = trim(%realLine);
      
      // We deactivate if we hit a ") {" or a "};" (ie, start or end of new
      // object definition.
      if(strstr(%line, ") {") != -1 ||
         strstr(%line, "};") != -1)
      {
         %active = false;
      }

      // We activate in one of two cases, per Alex's description, either
      // after a line containing "new ScriptObject(MissionInfo)" or a line
      // containing "new Trigger".
      if(strstr(%line, "new ScriptObject(MissionInfo)") != -1 ||
         strstr(%line, "new Trigger") != -1)
      {
         %active = true;
      }
      
      // If we're not active, copy line out and continue.
      if(!%active)
      {
         %fOut.writeLine(%realLine);
         continue;
      }
         
      // Appropriate fields are...
      // name = $Text::LevelName1;
      // startHelpText = $Text::LevelStartHelp1;
      // text = $Text::TriggerText1_0; 
      
      // But with "something" instead of a variable.
      
      // So let's see if we have any of those fields before
      // the = sign.
      %eqPos = strrchrpos( %line, "=" );
      if( %eqPos == -1 ) // Bork if no equal!
      {
         %fOut.writeLine(%realLine);
         continue;
      }
      
      // Try the possibilities...
      %res = strstr(%line, "name");
      if(%res != -1 && %res < %eqPos)
      {
         %fixCount++;
         %realLine = processPossibleVarMatchDekludge(%realLine);
      }
      
      %res = strstr(%line, "startHelpText");
      if(%res != -1 && %res < %eqPos)
      {
         %fixCount++;
         %realLine = processPossibleVarMatchDekludge(%realLine);
      }

      %res = strstr(%line, "text");
      if(%res != -1 && %res < %eqPos)
      {
         %fixCount++;
         %realLine = processPossibleVarMatchDekludge(%realLine);
      }
      
      // Copy it out.
      %fOut.writeLine(%realLine);
   }
   
   %fo.delete();
   %fOut.delete();
   
   echo("   Done processing file, fixed " @ %fixCount @ " lines.");
   
   // Now copy...
   %foIn = new FileObject();
   %foOut = new FileObject();
   if (!%foIn.openForRead(%filename @ ".tmp"))
   {
      error("   File not found:" SPC %filename);
      %foIn.delete();
      return;
   }
            
   if(!%foOut.openForWrite(%filename))
   {
      error("   File not writable:" SPC %filename);
      %foIn.delete();
      %foOut.delete();
      return;
   }
   
   
   while(!%foIn.isEOF())
   {
      %realLine = %foIn.readLine();
      %foOut.writeLine(%realLine);
   }
   
   // Clean up.
   %foOut.delete();
   %foIn.delete();
   echo("   Done copying back to original mission file.");   
}

function missionDekludge()
{
   prepareMissionDekludger();
   
   %mis = findFirstFile("*.mis");
   
   while(%mis !$= "")
   {
      repairMissionKludgeryInFile(%mis);
      %mis = findNextFile("*.mis");
   }
}

function onStart()
{
   Parent::onStart();
   echo("\n--------- Initializing MOD: FPS ---------");

   // Load the scripts that start it all...
   exec("./client/init.cs");
   exec("./server/init.cs");
   exec("~/data/init.cs");
   exec("./data/GameMissionInfo.cs");
   // init GameMissionInfo
   GameMissionInfo.init();
   
   // Server gets loaded for all sessions, since clients
   // can host in-game servers.
   initServer();

   // Start up in either client, or dedicated server mode
   if ($Server::Dedicated)
   {
      // need to load up the materials so that we have friction properties and what not
      //exec("~/data/init.cs");
      
      // Load server scripts and resources
      execServerScripts();
      initDedicated();
   }
   else
   {
      initClient();
      // Load server scripts and resources - we do this after the client is initialized because 
      // when the datablocks are preloaded they will need the GFX object, which isn't available 
      // until after the Canvas has been created
      execServerScripts();
      preloadClientDataBlocks();
   }
}

function onExit()
{
   echo ("Exporting prefs");
   export("$pref::*", "prefs.cs");   
   
   Parent::onExit();
}

}; // Client package
activatePackage(marble);

function listResolutions()
{
   %deviceList = getDisplayDeviceList();
   for(%deviceIndex = 0; (%device = getField(%deviceList, %deviceIndex)) !$= ""; %deviceIndex++)
   {
      %resList = getResolutionList(%device);
      for(%resIndex = 0; (%res = getField(%resList, %resIndex)) !$= ""; %resIndex++)
         echo(%device @ " - " @ getWord(%res, 0) @ " x " @ getWord(%res, 1) @ " (" @ getWord(%res, 2) @ " bpp)");
   }
}

//-------------------------------------------------------------
// Below is some code for testing for memory leaks.  It creates
// an interior and deletes it and then logs the leaked memory.
// It does the same for pathed interiors, items, marbles.  After
// that, it runs a mission -- twice -- for  10 seconds and records 
// leaks from the second running of the mission (so that one time
// instance data isn't included).  Before dumping the log, we delete
// all sorts of things that hold onto instance memory (but aren't
// leaks) and then we log it and quit.
//-------------------------------------------------------------
function testmem()
{
   profilerEnable(1);
   
   // schedule this out so the profiler 
   // enable has time to work (don't ask)
   schedule(100,0,"testMem2");
}

function testMem2()
{
   destroyServer();
   ServerGroup.delete();
   ServerConnection.delete();

   // ---------------------------   
   // Begin test interior leaks
   // ---------------------------   
   
   flagCurrentAllocs();
   
   $testItr = new InteriorInstance() {
      position = "0 0 0";
      rotation = "1 0 0 0";
      scale = "1 1 1";
      interiorFile = "marble/data/missions/Multiplayer/Walled Sprawl/sprawl_walled.dif";
      showTerrainInside = "0";
   };
   error("interior id:" SPC $testItr.getId());
   $testItr.delete();
   
   purgeResources();
   dumpUnflaggedAllocs("itrdmp.txt");
   
   // ---------------------------   
   // Begin test pathed interior leaks
   // ---------------------------   
   
   flagCurrentAllocs();
   
   $testPItr = new PathedInterior(MustChange) {
      position = "0 0 12";
      rotation = "1 0 0 0";
      scale = "1 1 1";
      hidden = "0";
      dataBlock = "PathedDefault";
      interiorResource = "marble/data/missions/advanced/Cube Root/cube.dif";
      interiorIndex = "0";
      basePosition = "0 0 0";
      baseRotation = "1 0 0 0";
      baseScale = "1 1 1";
      initialTargetPosition = "-1";
   };
   error("pathed interior id:" SPC $testPItr.getId());
   $testPItr.delete();
   
   purgeResources();
   dumpUnflaggedAllocs("pitrdmp.txt");
   
   // ---------------------------   
   // Begin test item leaks
   // ---------------------------   
   
   flagCurrentAllocs();

   $testItem = new Item() {
      position = "-7.69364 7.04311 89";
      rotation = "1 0 0 90";
      scale = "1 1 1";
      dataBlock = "AntiGravityItem";
      collideable = "0";
      static = "1";
      rotate = "1";
      permanent = "1";
   };
   error("item id:" SPC $testItem.getId());
   $testItem.delete();

   purgeResources();
   dumpUnflaggedAllocs("itemdmp.txt");
   
   // ---------------------------   
   // Begin test marble leaks
   // ---------------------------   

   flagCurrentAllocs();

   $testMarble = new Marble() {
      datablock = defaultMarble;
   };
   error("marble id:" SPC $testMarble.getId());
   $testMarble.delete();
   
   purgeResources();
   dumpUnflaggedAllocs("marbledmp.txt");

   // ---------------------------   
   // Begin test mission leaks
   // ---------------------------   

   flagCurrentAllocs();

   createServer("Multiplayer", "marble/data/missions/advanced/Battlements/battlements.mis");
   connectToPreviewServer();
   commandToServer('JoinGame');
   RootGui.setContent(PlayGui);
   schedule(10000,0,"againTestMem");   
}

function againTestMem()
{
   destroyServer();
   ServerConnection.delete();

   // ---------------------------   
   // Begin test mission leaks (2)
   // ---------------------------   

   flagCurrentAllocs();

   createServer("Multiplayer", "marble/data/missions/advanced/Battlements/battlements.mis");
   connectToPreviewServer();
   commandToServer('JoinGame');
   RootGui.setContent(PlayGui);
   schedule(10000,0,"finishTestMem");   
}

function finishTestMem()
{
   destroyServer();
   ServerConnection.delete();

   datablockgroup.delete(); // datablocks often hold onto memory that was allocated
                            // for instance purposes -- e.g., tsshapes are held onto
                            // by datablocks and tsshapes hold onto instance render
                            // data (should probably be done a little differently, but
                            // that's what we have got).  Deleting datablocks gets this
                            // memory off our radar -- at the risk of missing a leak --
                            // but requires us to exit when this function is done.

   sfxStopAll($SimAudioType); // stopping sounds should be handled by server destruction
                 // that it isn't properly (tons of sfx leaks if this line not
                 // included) indicates we may have a problem here.
                 
   cleanDetailManager(); // this gets rid of a bunch of accumulated working memory
                         // it isn't strictly a leak (much like console dictionaries,
                         // datachunkers, and frameallocator) but gets in the way
                         // of seeing the real leaks.
                         
   clearServerPaths();  // Server path manager is normally cleared out pre mission-load.
   clearClientPaths();  // Client path manager is cleared out indirectly in the process.
                        // We clear them both out here so that they don't appear as
                        // leaks.

   RootGui.delete();    // Get rid of all gui's since they hold some memory that may have
   GuiGroup.delete();   // been created by playing mission (but not leaks).
   GuiDataGroup.delete();
   
   cleanPrimBuild();    // clean out anything prim builder holding onto
   
   purgeResources();
   dumpUnflaggedAllocs("missiondmp.txt");
   
   schedule(1000,0,"quit");
}
function onFileChunkSent(%file, %count, %max) {
   if ($lastChunkStart $= "" || %count < $lastChunkCount || (getSimTime() - $lastChunkStart) > 1000)
   {
      $lastChunkStartCount = %count;
      $lastChunkStart = getSimTime();
   }
   $lastChunkCount = %count;
   %rate = (%count - $lastChunkStartCount) / ((getSimTime() - $lastChunkStart) / 1000);
   %rate = (%rate / 1024) @ "kB/s";

   echo(%file SPC %rate SPC %count SPC %max);
}


function onFileChunkReceived(%file, %count, %max) {
   if ($lastChunkStart $= "" || %count < $lastChunkCount || (getSimTime() - $lastChunkStart) > 1000)
   {
      $lastChunkStartCount = %count;
      $lastChunkStart = getSimTime();
   }
   $lastChunkCount = %count;
   %rate = (%count - $lastChunkStartCount) / ((getSimTime() - $lastChunkStart) / 1000);
   %rate = (%rate / 1024) @ "kB/s";

   echo(%file SPC %rate SPC %count SPC %max);
}
