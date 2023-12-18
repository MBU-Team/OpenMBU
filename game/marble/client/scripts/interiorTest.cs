// MCR: I'd like for this to be in creator/, but that's not possible.
//      Filenames referenced in the dif that use ~/ will expand to creator/ 
//      if the script that calls .magicButton() is in creator/, so I moved
//      this code to a place in marble/ as a workaround.
//      Feel free to move it as long as it stays in marble/
function createMissionFromDif(%file)//,%isMultiplayer)
{
   echo("***** Editor: Creating new mission using .dif file: "@ %file);
   %missionGroup = createEmptyMission(%file);
   %saveIG = $instantGroup;
   $instantGroup = %missionGroup;
   
   // Add main interior object
   %interior = new InteriorInstance()
   {
      position = "0 0 0";
      rotation = "1 0 0 0";
      scale = "1 1 1";
      interiorFile = %file;
   };
   
   // Add bounds object
   %box = %interior.getWorldBox();
   %mx = getWord(%box, 0);
   %my = getWord(%box, 1);
   %mz = getWord(%box, 2);
   %MMx = getWord(%box, 3);
   %MMy = getWord(%box, 4);
   %MMz = getWord(%box, 5);
   %pos = (%mx - 3) SPC (%MMy + 3) SPC (%mz - 3);
   %scale = (%MMx - %mx + 6) SPC (%MMy - %my + 6) SPC (%MMz - %mz + 20);

   new Trigger(Bounds) {
      position = %pos;
      scale = %scale;
      rotation = "1 0 0 0";
      dataBlock = "InBoundsTrigger";
      polyhedron = "0.0000000 0.0000000 0.0000000 1.0000000 0.0000000 0.0000000 0.0000000 -1.0000000 0.0000000 0.0000000 0.0000000 1.0000000";
   };   

   // Create all the child objects defined in the interior
   %interior.magicButton();
   
   // Matt: Detect Multiplayer by checking if there is a gem spawn
   %isMultiplayer = false;
   for (%i = 0; %i < %missionGroup.getCount(); %i++)
   {
      %obj = %missionGroup.getObject(%i);
      if(%obj.getClassName() $= "SpawnSphere" && %obj.getDataBlock().getName() $= "GemSpawnSphereMarker")
      {
         %isMultiplayer = true;
         break;
      }
   }
    
   // For multiplayer: Add scrum game info
   if(%isMultiplayer)
   {
      // Organize the MissionGroup a little: Stick gem spawns in the GemSpawn group, player spawns in the SpawnPoints group
      %gemGroup = nameToId("MissionGroup/GemSpawns");
      if(%gemGroup == -1)
         %gemGroup = new SimGroup(GemSpawns);

      %spawnGroup = nameToId("MissionGroup/SpawnPoints");
      if(%spawnGroup == -1)
         %spawnGroup = new SimGroup(SpawnPoints);
         
      %i = 0;
      while(%i < %missionGroup.getCount())
      {
         %obj = %missionGroup.getObject(%i);
         if(%obj.getClassName() $= "SpawnSphere")
         {
            if(%obj.getDataBlock().getName() $= "GemSpawnSphereMarker")
            {
               %gemGroup.add(%obj);
               continue;
            }
            else if (%obj.getDataBlock().getName() $= "SpawnSphereMarker")
            {
               %spawnGroup.add(%obj);
               continue;
            }
         }
    
         %i++;
      }
         
      createMissionFromDif_AddScrumInfo();
   }
   else
   {
      // For singleplayer: Add start/end points if they were missing
      // Also, make sure the start point is added to the SpawnPoints group
      %spawnGroup = nameToId("MissionGroup/SpawnPoints");
      if(%spawnGroup == -1)
         %spawnGroup = new SimGroup(SpawnPoints);
         
      if (!isObject(StartPoint))
      {
         // create spawn points group
         new StaticShape(StartPoint)
         {
            position = "0 -5 25";
            rotation = "1 0 0 0";
            scale = "1 1 1";
            dataBlock = "StartPad";
         };
      }
      %spawnGroup.add(StartPoint);

      if(!isObject(EndPoint))
      {
         new StaticShape(EndPoint)
         {
            position = "0 5 100";
            rotation = "1 0 0 0";
            scale = "1 1 1";
            dataBlock = "EndPad";
         };
      }      
   }  
   
   %missionFile = strreplace(%file, ".dif", ".mis");
   %missionGroup.save(%missionFile);
   //%missionGroup.save("creator/data/lastNewMission.mis");
   %missionGroup.delete();
   
   $instantGroup = %saveIG;  
   
   // Probably should be when loading the mission instead of here, but this works for now.
   $Game::SPGemHunt = %isMultiplayer;  
   
   return %missionFile;
}

function createMissionFromDif_AddScrumInfo()
{
   MissionInfo.gameType = "MultiPlayer";
   MissionInfo.maxGemsPerGroup = "6";
   MissionInfo.time = "300000";
   MissionInfo.gameMode = "Scrum";
   MissionInfo.gemGroupRadius = "25";
   MissionInfo.goldTime = "0";
   MissionInfo.numgems = "1";   
}

function createMissionFromDif_FixGemSpawns(%group)
{
   for(%i = 0; %i < %group.getCount(); %i++)
   {
      %obj = %group.getObject(%i);
      if(%obj.getClassName() $= "SpawnSphere")
      {
         if(%obj.getDataBlock().getName() $= "GemSpawnSphereMarker")
         {
            if(!isObject(%obj.gemDataBlock))
            {
               echo("GS: defaulting the db");
               %obj.gemDataBlock = GemItem;
            }
            else
               echo("GS: had db");
         }
      }
      else if(%obj.getClassName() $= "SimGroup")
         createMissionFromDif_FixGemSpawns(%obj);
   }
}
