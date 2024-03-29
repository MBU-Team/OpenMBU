// Skies
$sky_beginner = "marble/data/skies/sky_beginner.dml";
$sky_intermediate = "marble/data/skies/sky_intermediate.dml";
$sky_advanced = "marble/data/skies/sky_advanced.dml";

// leaderboard ids
$Leaderboard::SPOverall = "{D8785EA3-2188-4BCC-89D1-B77CB8DF9A54}";
$Leaderboard::SPCompletion = "{09B491BA-BC32-44E9-AF1F-92C02031BB5E}";
//$Leaderboard::CustomSPOverall = "{C591744D-DE67-43B4-A4E0-D0EC781B7418}";
$Leaderboard::MPScrumOverall = "{B7E841BE-AAA7-4054-AE9D-729C13937037}";
// skill leaderboard variables are defined in engine code (since they may be changed by xlast)
// variables are:
// $Leaderboard::MPScrumSkill

// Languages are no longer loaded at this point so we need to set this later
function initLBText()
{
   // titles for the overall leaderboards
   $LeaderboardTitles[$Leaderboard::SPOverall] = $Text::LB_SPOverall;
   // not displayed --> //$LeaderboardTitles[$Leaderboard::SPCompletion]  = $Text::LB_SPCompletion; 
   $LeaderboardTitles[$Leaderboard::MPScrumOverall] = $Text::Standard;
   $LeaderboardTitles[$Leaderboard::MPScrumSkill] = $Text::Ranked;
}

// overall leaderboards for each game mode
$OverallLeaderboards[0] = $Leaderboard::MPScrumOverall;


new ScriptObject(GameMissionInfo)
{
   currentSPIndex = -1;
   currentMPIndex = -1;
   currentCustomIndex = -1;
   
   // various "constants"
   MissionFileSpec = "*/missions/*.mis";
   SPMode = "SinglePlayer";
   MPMode = "MultiPlayer";
   SpecialMode = "Special";
   CustomMode = "Custom";
};

function GameMissionInfo::filterDifficulty(%this)
{
   return %this.filterDifficulty;
}

function GameMissionInfo::setFilterDifficulty(%this, %filter)
{
   %this.filterDifficulty = %filter;
}

function GameMissionInfo::init(%this)
{
   buildMissionList();
   buildLeaderboardList(); 
   %this.setMode(GameMissionInfo.SPMode);
}

function GameMissionInfo::getMode(%this)
{
   return %this.mode;
}

function GameMissionInfo::setMode(%this,%mode)
{
   // Since we are switching modes we need to hide the current mission
   if ($EnableFMS)
   {
      // Hide current mission
      %mission = %this.getCurrentMission();
      if (isObject(%mission))
      {
         //error("Hiding " @ %mission.missionGroup);
         %mission.missionGroup.setHidden(true);
      }
   }

   if (%mode $= "")
      %mode = GameMissionInfo.SPMode;

   // if our mode is changing clear the difficulty filter
   if (%mode !$= %this.mode)
      %this.setFilterDifficulty("");
           
   %this.mode = %mode;
}

function GameMissionInfo::getCurrentMissionGroup(%this)
{
   if (%this.mode $= GameMissionInfo.MPMode)
      %missionGroup = MultiPlayMissionGroup;
   else if (%this.mode $= GameMissionInfo.SpecialMode)
      %missionGroup = SpecialMissionGroup;
   else if (%this.mode $= GameMissionInfo.CustomMode)
      %missionGroup = CustomSinglePlayMissionGroup;
   else 
      %missionGroup = SinglePlayMissionGroup;
      
   return %missionGroup;
}

function GameMissionInfo::getLeaderboardTitle(%this,%lbid)
{
   // Init Leaderboard Text here as this should ensure that
   // the correct language text is used.
   initLBText();
   
   return $LeaderboardTitles[%lbid];
}

function GameMissionInfo::getOverallLeaderboard(%this,%gameModeId)
{
   return $OverallLeaderboards[%gameModeId];
}

function GameMissionInfo::findIndexByPath(%this, %missionFile)
{
   %index = -1;
   
   %missionGroup = GameMissionInfo.getCurrentMissionGroup();
   
   for (%i = 0; %i < %missionGroup.getCount(); %i++)
   {
      if (%missionGroup.getObject(%i).file $= %missionFile)
      {
         %index = %i;
         break;
      }
   }
   
   return %index;
}

function GameMissionInfo::findMissionById(%this,%missionId, %group)
{
   if (%group $= "")
      %group = %this.getCurrentMissionGroup();
   for (%i = 0; %i < %group.getCount(); %i++)
   {
      %mission = %group.getObject(%i);
      if (%mission.level == %missionId)
         return %mission;
   }
   return 0;
}

function GameMissionInfo::findMissionByGuid(%this,%guid)
{
   %group = %this.getCurrentMissionGroup();
   for (%i = 0; %i < %group.getCount(); %i++)
   {
      %mission = %group.getObject(%i);
      if (%mission.guid $= %guid)
         return %mission;
   }
   return 0;
}

function GameMissionInfo::findMissionByPath(%this,%missionPath)
{
   %group = %this.getCurrentMissionGroup();
   for (%i = 0; %i < %group.getCount(); %i++)
   {
      %mission = %group.getObject(%i);
      if (%mission.file $= %missionPath)
         return %mission;
   }
   return 0;
}

function GameMissionInfo::setCurrentMission(%this,%missionFile)
{
   // find the index of the mission and update the appropriate index variable
   %index = %this.findIndexByPath(%missionFile);
   if (%index == -1)
   {
      error("setCurrentMission: idx should not be -1");
      return;
   }
   %this.setCurrentIndex(%index);
}

function GameMissionInfo::getCurrentMission(%this)
{
   if (%this.getCurrentIndex() == -1)
      return 0;

   %mission = %this.getCurrentMissionGroup().getObject(%this.getCurrentIndex());
   if (!isObject(%mission))
      error("getCurrentMission: no current mission");
   return %mission;
}

function GameMissionInfo::setCurrentIndex(%this,%index)
{
   if (%this.mode $= GameMissionInfo.MPMode)
      %this.currentMPIndex = %index;
   else if (%this.mode $= GameMissionInfo.SpecialMode)
      $this.currentSpecialIndex = %index;
   else if (%this.mode $= GameMissionInfo.CustomMode)
      %this.currentCustomIndex = %index;
   else
      %this.currentSPIndex = %index;
      
   //$Server::GameType = GameMissionInfo.getCurrentMission().gameMode;
   $Server::MissionType = GameMissionInfo.getCurrentMission().guid;
}

function GameMissionInfo::getCurrentIndex(%this)
{
   if (%this.mode $= GameMissionInfo.MPMode)
      return %this.currentMPIndex;
   else if (%this.mode $= GameMissionInfo.SpecialMode)
      return %this.currentSpecialIndex;
   else if (%this.mode $= GameMissionInfo.CustomMode)
      return %this.currentCustomIndex;
   else
      return %this.currentSPIndex;
}

function GameMissionInfo::getGameModeDisplayName(%this, %modeId)
{
   switch (%modeId)
   {
      case 0:
         return $Text::GameModeScrum;
   }
   error("GameMissionInfo::getGameModeDisplayName: unknown game mode:" SPC %modeId);
   return "";
}

function GameMissionInfo::getGameModeDisplayList(%this)
{
   return %this.getGameModeDisplayName(0);
}

function GameMissionInfo::getGameModeIdFromMultiplayerMission(%this, %missionId)
{
   // we suck so we have to do a linear search for the missionId
   %group = MultiPlayMissionGroup;
   
   for (%i = 0; %i < %group.getCount(); %i++)
   {
      %mission = %group.getObject(%i);
      if (%mission.level == %missionId)
         return %this.getGameModeIdFromString(%mission.gameMode);
   }
     
   return -1;
}

function GameMissionInfo::getGameModeIdFromString(%this, %modeString)
{
   if (stricmp(%modeString, "scrum") == 0)
      return 0;      
  
   return -1;
}

function GameMissionInfo::getValidIndexForDifficulty(%this, %index)
{
   if( %this.filterDifficulty() !$= "" )
   {
      %group = %this.getCurrentMissionGroup();
      %mission = %group.getObject( %index );
      
      while( %mission.difficultySet !$= %this.filterDifficulty() )
      {
         if (%goNeg)
            %index--;
         else
            %index++;
         
         if( %index < 0 )
            %index = %this.getCurrentMissionGroup().numAddedMissions - 1;
         else if ( %index > %this.getCurrentMissionGroup().numAddedMissions - 1 )
            %index = 0;
         
         %mission = %group.getObject( %index );
      }
   }
   return %index;
}

function GameMissionInfo::selectMission(%this, %index)
{
   if ($EnableFMS)
   {
      // Hide current mission
      %mission = %this.getCurrentMission();
      if (isObject(%mission))
      {
         //error("Hiding " @ %mission.missionGroup);
         %mission.missionGroup.setHidden(true);
      }
   }

   // Need to determine which way to go for demo/filtered levels
   %goNeg = false;
   if (%index < %this.getCurrentIndex())
      %goNeg = true;
      
   %haxGoNeg = %goNeg;
   if( %this.getCurrentIndex() == 0 && %index == ( %this.getCurrentMissionGroup().numAddedMissions - 1 ) )
      %haxGoNeg = true;
      
   if( %index < 0 )
      %index = %this.getCurrentMissionGroup().numAddedMissions - 1;
   else if ( %index > %this.getCurrentMissionGroup().numAddedMissions - 1 )
      %index = 0;
   
   if( %this.filterDifficulty() !$= "" )
   {
      %group = %this.getCurrentMissionGroup();
      %mission = %group.getObject( %index );
      
      while( %mission.difficultySet !$= %this.filterDifficulty() )
      {
         if (%goNeg)
            %index--;
         else
         %index++;
         
         if( %index < 0 )
            %index = %this.getCurrentMissionGroup().numAddedMissions - 1;
         else if ( %index > %this.getCurrentMissionGroup().numAddedMissions - 1 )
            %index = 0;
         
         %mission = %group.getObject( %index );
      }
   }
   else if( %this.mode $= GameMissionInfo.MPMode ) // Only execute PDLC check for mutliplayer
   {
      %group = %this.getCurrentMissionGroup();
      %mission = %group.getObject( %index );

	  // Assumed that mission is 'locked' by gui which invoked (skip ownership check)
      while( !%this.levelIsVisible( %mission.level, false ) )
      {
         //echo( "PDLCAllowMission false for level " @ %mission.level @ " [" @ %index @ "]" );
         if (%haxGoNeg)
            %index--;
         else
            %index++;
            
         //echo( "Adjusting index [" @ %index @ "]" );

		 %index = %this.wrapMissionIndex( %index );
            
         //echo( "Using index [" @ %index @ "]" );
         
         %mission = %group.getObject( %index );
         //echo( "-- Trying level: " @ %mission.level );
      }
   }
   
   %this.setCurrentIndex(%index);
   
   // Load the mission
   if ($EnableFMS)
   {
      //error("Unhiding " @ %this.getCurrentMission().missionGroup);
      %this.setCamera();
      %this.getCurrentMission().missionGroup.setHidden(false);
      
      if ($curr_sky !$= %this.getCurrentMission().sky)
      {
         // First hide our clouds
         if ($curr_sky $= $sky_beginner || $curr_sky $= "")
            Cloud_Beginner.setHidden(true);
         if ($curr_sky $= $sky_intermediate || $curr_sky $= "")
            Cloud_Intermediate.setHidden(true);
         if ($curr_sky $= $sky_advanced || $curr_sky $= "")
            Cloud_Advanced.setHidden(true);

         // Then set the material
         Sky.setSkyMaterial(%this.getCurrentMission().sky);
         $curr_sky = %this.getCurrentMission().sky;

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
   else
   {
      cancel(%this.loadSched);
      %this.loadSched = schedule(1000, 0, loadMission, %this.getCurrentMission().file);
   }
}

function GameMissionInfo::setCamera(%this)
{
   if (isObject($previewCamera))
      $previewCamera.setTransform(%this.getCurrentMission().cameraPos);
}

function GameMissionInfo::currentMissionLocked(%this)
{
	if(isDemoLaunch())
	{
		if(%this.getCurrentMission().isInDemoMode)
			return false;
		else
			return true;
	}
	else if(%this.mode $= %this.MPMode) 
	{
		if(PDLCAllowMission(%this.getCurrentMission().level))
			return false;
		else
			return true;
	}
	else
		return false;
}

function GameMissionInfo::wrapMissionIndex(%this, %index)
{
	if( %index < 0 )
	   %index = %this.getCurrentMissionGroup().numAddedMissions - 1;
	else if ( %index > %this.getCurrentMissionGroup().numAddedMissions - 1 )
	   %index = 0;
	return %index;
}

function GameMissionInfo::levelIsVisible(%this, %level, %checkOwnership)
{
	if(!isLevelContentAvailable(%level))
	{
		// In the event that MarketPlace query fails; still allow owned content at least.
		return PDLCAllowMission(%level);
	}

	if(%checkOwnership)
	{
		if(isDemoLaunch())
			return false;

		return PDLCAllowMission(%level);
	}

	return true;
}

function GameMissionInfo::selectPreviousMission(%this, %checkOwnership)
{
   %index = %this.getCurrentIndex() - 1;
   
   if( %this.mode $= GameMissionInfo.MPMode )
   {
      %group = %this.getCurrentMissionGroup();
      %index = %this.wrapMissionIndex(%index);
      %mission = %group.getObject( %index );
      
      while( !%this.levelIsVisible( %mission.level, %checkOwnership ) )
      {
         %index--;
		   %index = %this.wrapMissionIndex(%index);
         
         %mission = %group.getObject( %index );
      }
   }
      
   %this.selectMission( %index );
}

function GameMissionInfo::selectNextMission(%this, %checkOwnership)
{
   %index = %this.getCurrentIndex() + 1;

   if( %this.mode $= GameMissionInfo.MPMode )
   {
      %group = %this.getCurrentMissionGroup();
      %index = %this.wrapMissionIndex(%index);
      %mission = %group.getObject( %index );
      
      while( !%this.levelIsVisible( %mission.level, %checkOwnership ) )
      {
         %index++;
		   %index = %this.wrapMissionIndex(%index);
         
         %mission = %group.getObject( %index );
      }
   }
      
   %this.selectMission( %index );
}

function GameMissionInfo::getDefaultMission(%this)
{
   %mission = %this.findMissionByPath($Server::MissionFile);
   if (!isObject(%mission))
   {
      // invalid mission for current mode.  
      // if an index is set for the current mode, load that mission
      %index = %this.getCurrentIndex();
      if (%index == -1)
         // use first mission
         %index = 0;
      
      %group = %this.getCurrentMissionGroup();
      %mission = %group.getObject(%index);
   }

   return %mission.file;
}

function GameMissionInfo::setDefaultMission(%this,%missionId) // note: %missionId parameter broken by FMS
{
   if ($EnableFMS)
   {
      %index = %this.getCurrentIndex();
      if (%index == -1) // handle case where index hasn't been set yet
         %index = 0;
      %this.selectMission(%index);
      return;
   }

   // don't do this in test level mode
   if ($testLevel)
      return;
     
   // if they specified a mission Id, see if we can find it
   %mission = 0;
   if (%missionId !$= "")
      %mission = %this.findMissionById(%missionId);
      
   // if we have a mission object, make sure the server is running that mission
   if (isObject(%mission))
   {
      if (%mission.file !$= "" && $Server::MissionFile !$= %mission.file)
         loadMission(%mission.file);
   }
   else
   {
      // no mission object found.  if the server is not running a valid mission for this mode,
      // we'll need to switch it
      
      %mission = %this.findMissionByPath($Server::MissionFile);
      if (!isObject(%mission))
      {
         // invalid mission for current mode.  
         // if an index is set for the current mode, load that mission
         %index = %this.getCurrentIndex();
         if (%index == -1)
            %index = %this.getValidIndexForDifficulty(0);
         else // make sure difficulty is correct
            %index = %this.getValidIndexForDifficulty(%this.getCurrentIndex());
         
         %group = %this.getCurrentMissionGroup();
         %mission = %group.getObject(%index);
         loadMission(%mission.file);
      }
      else
      {
         // valid mission, but if we have a difficulty filter it may be inappropriate.  if so move 
         // forward until we find a mission with the right difficulty
         %index = %this.getValidIndexForDifficulty(%this.getCurrentIndex());
         if (%index != %this.getCurrentIndex())
         {
            echo("Mission does not match difficulty, switching");
            %group = %this.getCurrentMissionGroup();
            %mission = %group.getObject(%index);
            loadMission(%mission.file);
         }
      }
   }
   
   // should always have a mission object at this point
   if (!isObject(%mission))
      error("setDefaultMission: I don't have a mission object and I should");
   
   // update current mission
   %this.setCurrentMission(%mission.file);
   
   return %mission;
}

// the ordering of the list returned by this function must match getMissionDisplayNameList below
function GameMissionInfo::getMissionIdList(%this)
{
   %group = %this.getCurrentMissionGroup();
   %list = "";
   for (%i = 0; %i < %group.getCount(); %i++)
   {
      %mission = %group.getObject(%i);
      
      if( %this.filterDifficulty() $= "" || %this.filterDifficulty() $= %mission.difficultySet )
      {
         %list = %list @ %mission.level;
         if (%i < %group.getCount() - 1)
            %list = %list @ "\t";
      }
   }
   return %list;
}

// the ordering of the list returned by this function must match getMissionDisplayNameList above
function GameMissionInfo::getMissionDisplayNameList(%this)
{
   %group = %this.getCurrentMissionGroup();
   %list = "";
   for (%i = 0; %i < %group.getCount(); %i++)
   {
      %mission = %group.getObject(%i);
      
      if( %this.filterDifficulty() $= "" || %this.filterDifficulty() $= %mission.difficultySet )
      {
         %list = %list @ getMissionNameFromNameVar(%mission);
         if (%i < %group.getCount() - 1)
            %list = %list @ "\t";
      }
   }
   return %list;
}

// the ordering of the list returned by this function must match getMissionDisplayNameList above
function GameMissionInfo::getMissionGuidList(%this)
{
   %group = %this.getCurrentMissionGroup();
   %list = "";
   for (%i = 0; %i < %group.getCount(); %i++)
   {
      %mission = %group.getObject(%i);
      
      if( %this.filterDifficulty() $= "" || %this.filterDifficulty() $= %mission.difficultySet )
      {
         %list = %list @ %mission.guid;
         if (%i < %group.getCount() - 1)
            %list = %list @ "\t";
      }
   }
   return %list;
}

function GameMissionInfo::getMissionDisplayName(%this, %missionId)
{
   %mission = %this.findMissionById(%missionId);
   if (!isObject(%mission))
      return "";
   else
      return getMissionNameFromNameVar(%mission);
}

function GameMissionInfo::getMissionDisplayNameByGuid(%this, %guid)
{
   %mission = %this.findMissionByGuid(%guid);
   if (!isObject(%mission))
      return "";
   else
      return getMissionNameFromNameVar(%mission);
}

function getMissionNameFromNameVar(%mission)
{
   %name = %mission.name;   
   
   if (%mission.nameVar !$= "")
      eval("%name = " @ %mission.nameVar @ ";");
      
   return %name;
}

// returns true if there was a tie, false otherwise
function setRelativeRanks(%gameModeId, %textListCtrl)
{
   if (!isObject(%textListCtrl))
   {
      error("Parameter is not a text list control:" SPC %textListCtrl);
      return 0;
   }
   
   // sort by score first
   echo("setRelativeRanks: Sorting scores descending");
   %textListCtrl.sortNumerical(0, false);

   %rank = %textListCtrl.rowCount() - 1;
   %rankDecr = 1;
   for ( %i = 0; %i < %textListCtrl.rowCount() - 1; %i++ )
   {
      %rowid = %textListCtrl.getRowId(%i);
      %val = %textListCtrl.getRowText(%i);

      // Set the rank
      %textListCtrl.setRowById(%rowid, %rank SPC %val);

      // Get the next row and decide if we need to increment rank
      %nextval = %textListCtrl.getRowText(%i + 1);

      if (%val != %nextval)
      {
         // score is different, so decrement the rank by the current decrement value
         // then reset the rank decrement to 1
         %rank -= %rankDecr;
         %rankDecr = 1;
      }
      else
      {
         // increase the rank decrement for each tied player
         %rankDecr++;
      }
   }

   // Set the last
   %rowid = %textListCtrl.getRowId(%textListCtrl.rowCount() - 1);
   %val = %textListCtrl.getRowText(%textListCtrl.rowCount() - 1);
   %textListCtrl.setRowById(%rowid, %rank SPC %val);

   %tied = false;
   
   // Detect a tie for first
   if (%textListCtrl.rowCount() > 1)
   {
      if (getWord(%textListCtrl.getRowText(0), 0) == getWord(%textListCtrl.getRowText(1), 0))      
         %tied = true;
   }

   // Dump %textListCtrl for debugging
//   error("Dumping Ranks:");
//   for ( %i = 0; %i < %textListCtrl.rowCount(); %i++ )
//   {
//      %clid = %textListCtrl.getRowId(%i);
//      %val = %textListCtrl.getRowText(%i);
//
//      error(%clid SPC %val);
//   }

   return %tied;
}


function buildLeaderboardList()
{
   // start sp list with overall leaderboard
   %spList = $Leaderboard::SPOverall;
   
   // append all of the sp missions
   for (%i = 0; %i < SinglePlayMissionGroup.getCount(); %i++)
   {
      %mis = SinglePlayMissionGroup.getObject(%i);
      %spList = %spList TAB %mis.guid;//%mis.level;
   }
   
   // store the list as a property of the mission group
   SinglePlayMissionGroup.lbList = %spList;
   SinglePlayMissionGroup.lbListCount = getFieldCount(%spList);
   
   // start spcustom list with overall leaderboard
   %spcustomList = "";//$Leaderboard::CustomSPOverall;
   
   // append all of the sp custom missions
   for (%i = 0; %i < CustomSinglePlayMissionGroup.getCount(); %i++)
   {
      %mis = CustomSinglePlayMissionGroup.getObject(%i);
      if (%spcustomList $= "")
         %spcustomList = %mis.guid;//%mis.level;
      else
         %spcustomList = %spcustomList TAB %mis.guid;//%mis.level;
   }
   
   // store the list as a property of the mission group
   CustomSinglePlayMissionGroup.lbList = %spcustomList;
   CustomSinglePlayMissionGroup.lbListCount = getFieldCount(%spcustomList);
   
   // start mp list with overall leaderboards
   %mpList = 
      $Leaderboard::MPScrumOverall TAB $Leaderboard::MPScrumSkill;
      
      // JMQ: disable MP level leaderboards
   // append all of the mp missions
//   for (%i = 0; %i < MultiPlayMissionGroup.getCount(); %i++)
//   {
//      %mis = MultiPlayMissionGroup.getObject(%i);
//      %mpList = %mpList TAB %mis.guid;//%mis.level;
//   }
   
   // store the list as a property of the mission group
   MultiPlayMissionGroup.lbList = %mpList;
   MultiPlayMissionGroup.lbListCount = getFieldCount(%mpList);
}

function buildMissionList()
{
   if( !isObject( SinglePlayMissionGroup ) || !isObject( CustomSinglePlayMissionGroup ) ||!isObject( MultiPlayMissionGroup ) || !isObject( SpecialMissionGroup ) )
   {
      if( !isObject( SinglePlayMissionGroup ) )
      {
         new SimGroup( SinglePlayMissionGroup );
         RootGroup.add( SinglePlayMissionGroup );
      }
      
      if( !isObject( CustomSinglePlayMissionGroup ) )
      {
         new SimGroup( CustomSinglePlayMissionGroup );
         RootGroup.add( CustomSinglePlayMissionGroup );
      }
      
      if( !isObject( MultiPlayMissionGroup ) )
      {
         new SimGroup( MultiPlayMissionGroup );
         RootGroup.add( MultiPlayMissionGroup );
      }
      if( !isObject( SpecialMissionGroup ) )
      {
         new SimGroup( SpecialMissionGroup );
         RootGroup.add( SpecialMissionGroup );
      }

      for(%file = findFirstFile ( GameMissionInfo.MissionFileSpec );
            %file !$= ""; %file = findNextFile( GameMissionInfo.MissionFileSpec ) )
      { 
         if (strStr(%file, "CVS/") == -1 && strStr(%file, "common/") == -1 && strStr(%file, "testMission.mis") == -1 &&
             strStr(%file, "megaEmpty.mis") == -1 && strStr(%file, "megaMission.mis") == -1)
         {
            getMissionObject(%file);
         }
      }

      sortByLevel( SinglePlayMissionGroup );
      sortByLevel( CustomSinglePlayMissionGroup, true );
      sortByLevel( MultiPlayMissionGroup, true );
      sortByLevel( SpecialMissionGroup );
      // hack, do this twice to get things into the proper order, don't have time to figure out why a 
      // single sort doesn't work
      sortByLevel( SinglePlayMissionGroup );
      sortByLevel( CustomSinglePlayMissionGroup, true );
      sortByLevel( MultiPlayMissionGroup, true );
      sortByLevel( SpecialMissionGroup );
      
      // verify that level Ids are unique
      //GameMissionInfo.dupErrors = "";
      for (%i = 0; %i < SinglePlayMissionGroup.getCount(); %i++)
      {
         %mis = SinglePlayMissionGroup.getObject(%i);
         if (%levelIds[%mis.level] !$= "")
         {
            GameMissionInfo.dupErrors = GameMissionInfo.dupErrors @ "duplicate mission Id for level:" SPC %mis.file @ "\n";
            GameMissionInfo.dupLevelIds[%mis.level] = true;
         }
         %levelIds[%mis.level] = true;
         if (%mis.guid !$= "")
         {
            if (%guids[%mis.guid] !$= "")
               GameMissionInfo.dupErrors = GameMissionInfo.dupErrors @ "duplicate GUID for level:" SPC %mis.file @ "\n";
            %guids[%mis.guid] = true;
         }
      }
      for (%i = 0; %i < CustomSinglePlayMissionGroup.getCount(); %i++)
      {
         %mis = CustomSinglePlayMissionGroup.getObject(%i);
         //if (%levelIds[%mis.level] !$= "")
         //{
            //GameMissionInfo.dupErrors = GameMissionInfo.dupErrors @ "duplicate mission Id for level:" SPC %mis.file @ "\n";
            //GameMissionInfo.dupLevelIds[%mis.level] = true;
         //}
         //%levelIds[%mis.level] = true;
         if (%mis.guid !$= "")
         {
            if (%guids[%mis.guid] !$= "")
               GameMissionInfo.dupErrors = GameMissionInfo.dupErrors @ "duplicate GUID for level:" SPC %mis.file @ "\n";
            %guids[%mis.guid] = true;
         }
      }
      for (%i = 0; %i < MultiPlayMissionGroup.getCount(); %i++)
      {
         %mis = MultiPlayMissionGroup.getObject(%i);
         //if (%levelIds[%mis.level] !$= "")
         //{
            //GameMissionInfo.dupErrors = GameMissionInfo.dupErrors @ "duplicate mission Id for level:" SPC %mis.file @ "\n";
            //GameMissionInfo.dupLevelIds[%mis.level] = true;
         //}
         //%levelIds[%mis.level] = true;
         if (%mis.guid !$= "")
         {
            if (%guids[%mis.guid] !$= "")
               GameMissionInfo.dupErrors = GameMissionInfo.dupErrors @ "duplicate GUID for level:" SPC %mis.file @ "\n";
            %guids[%mis.guid] = true;
         }
      }
      
      for (%i = 0; %i < SpecialMissionGroup.getCount(); %i++)
      {
         %mis = SpecialMissionGroup.getObject(%i);
         if (%levelIds[%mis.level] !$= "")
         {
            GameMissionInfo.dupErrors = GameMissionInfo.dupErrors @ "duplicate mission Id for level:" SPC %mis.file @ "\n";
            GameMissionInfo.dupLevelIds[%mis.level] = true;
         }
         %levelIds[%mis.level] = true;
         if (%mis.guid !$= "")
         {
            if (%guids[%mis.guid] !$= "")
               GameMissionInfo.dupErrors = GameMissionInfo.dupErrors @ "duplicate GUID for level:" SPC %mis.file @ "\n";
            %guids[%mis.guid] = true;
         }
      }
      
      if (GameMissionInfo.dupErrors !$= "")
         error(GameMissionInfo.dupErrors);
   }
}

function isDemoMission( %level ) 
{
  error( "ISDEMOMISSION DEPRICATED -- Use 'isInDemoMode = \"1\";' in mission info instead!" );
  // if( %level == 1 || %level == 3 || %level == 27 || %level == 38 || %level == 49 || %level == 75 )
  //    return 1;
   //else if( %level == 70 || %level == 52 || %level == 87 )
      //return 2;
   
   //return 0;
}

function sortByLevel(%grp, %useLevelNum)
{
   if (%useLevelNum $= "")
      %useLevelNum = false;
   %grp.numAddedMissions = 0;
   %newLevelNum = 0;
   %ngrp = new SimGroup();
   // take all the objects out of grp and put them in ngrp
   while((%obj = %grp.getObject(0)) != -1)
      %ngrp.add(%obj);
   while(%ngrp.getCount() != 0)
   {
      //%lowestDif = %ngrp.getObject(0).difficulty;
      %lowestLevel = %ngrp.getObject(0).level;
      
      %lowestIndex = 0;
      for(%i = 1; %i < %ngrp.getCount(); %i++)
      {
         //%dif = %ngrp.getObject(%i).difficulty;
         %level = %ngrp.getObject(%i).level;
      
         //if( %dif < %lowestDif )
         //{
         //   %lowestDif = %dif;
         //   %lowestIndex = %i;
         //}
         //else if( %dif == %lowestDif )
         //{
            if( %lowestLevel > %level )
            {
               %lowestIndex = %i;
               %lowestLevel = %level;
            }
         //}
      }

      %newLevelNum++;
      
      //if( isDemoLaunch() && isDemoMission( %newLevelNum ) == 0 )
      //{
      //   %obj = %ngrp.getObject(%lowestIndex);
      //   %ngrp.remove(%obj);
      //}
      //else
      //{
         %grp.numAddedMissions++;
         %obj = %ngrp.getObject(%lowestIndex);
         if (%useLevelNum)
            %obj.level = %newLevelNum;
         %grp.add(%obj);
      //}
   }
   %ngrp.delete();
}

function getMissionObject( %missionFile ) 
{
   %file = new FileObject();
   
   %missionInfoObject = "";
   %missionNameVar = "";
   
   %isCustom = false;
   if( filePath(filePath(%missionFile)) $= "marble/data/missions/custom" )
      %isCustom = true;
   
   if ( %file.openForRead( %missionFile ) ) {
		%inInfoBlock = false;

        %matFilePath = filePath(%missionFile) @ "/" @ fileBase(%missionFile) @ ".mat.json";
        if (isFile(%matFilePath))
        {
            echo("Loaded custom materials from" SPC %matFilePath);
            loadMaterialJson(%matFilePath);
        }
		
		while ( !%file.isEOF() ) {
			%line = %file.readLine();
			%line = trim( %line );
			
			if( %line $= "new ScriptObject(MissionInfo) {" ) {
				%line = "new ScriptObject() {";
				%inInfoBlock = true;
			}
			else if( %inInfoBlock && %line $= "};" ) {
				%inInfoBlock = false;
				%missionInfoObject = %missionInfoObject @ %line; 
				break;
			}
			
			if( %inInfoBlock )
			{
			   %missionInfoObject = %missionInfoObject @ %line @ " "; 	
			   
			   //error("checking line:" SPC %line);
			   if (strpos(%missionfile, "marblepicker") == -1 && 
			         (strpos(%line, "name =") != -1 || strpos(%line, "startHelpText =") != -1) &&
			         strpos(%line, "$Text::") == -1)
			   {
			      if (!%isCustom && $Dev::ShowLocInfo)
                  GameMissionInfo.dupErrors = GameMissionInfo.dupErrors @ "Bad loc info in" SPC %missionFile @ "\n";
			   }
			   
			   %missionNameVarIndex = strpos(%line, "$Text::LevelName");
			   if (%missionNameVarIndex != -1)
			   {
			      %missionNameVar = getSubStr(%line, %missionNameVarIndex, 99);
			      %missionNameVar = strreplace(%missionNameVar, ";", "");
			      %missionNameVar = stripTrailingSpaces(%missionNameVar);
			   }
			}
		}
		
		%file.close();
		%file.delete();
	}
	echo("checking mission" SPC %missionFile);
	%missionInfoObject = "%missionInfoObject = " @ %missionInfoObject;
	eval( %missionInfoObject );
	
	if (%missionInfoObject.guid $= "")
	   GameMissionInfo.dupErrors = GameMissionInfo.dupErrors @ "Missing GUID in" SPC %missionFile @ "\n";
   
   if (%missionNameVar !$= "")
      %missionInfoObject.nameVar = %missionNameVar;
	
	if (!%missionInfoObject.include)
	{
	   if (%missionInfoObject.nameVar !$= "")
	      %missionName = %missionInfoObject.nameVar;
      else
	      %missionName = %missionInfoObject.name;
	   echo("skipping mission because include field is not true:" SPC %missionName);
	   %missionInfoObject.delete();
	   return;
	}
   
   // All missions available to view, but not to play, in demo mode -pw
   //if( isDemoLaunch() && !%missionInfoObject.isInDemoMode )
   //{
   //   echo( "skipping mission because isInDemoMode field is not true:" SPC %missionInfoObject.name );
   //   %missionInfoObject.delete();
	//   return;
   //}
	   
   // find the directory this file belongs in:
   %path = filePath(%missionFile);
   %misPath = filePath(%path);
   
   // So we can sort by difficulty
   if( %misPath $= "marble/data/missions/beginner" )
      %missionInfoObject.difficultySet = "beginner";
   else if( %misPath $= "marble/data/missions/intermediate" )
      %missionInfoObject.difficultySet = "intermediate";
   else if( %misPath $= "marble/data/missions/advanced" )
      %missionInfoObject.difficultySet = "advanced";
   else if( %misPath $= "marble/data/missions/custom" )
      %missionInfoObject.difficultySet = "custom";
   else if( %misPath $= "marble/data/missions/special" )
      %missionInfoObject.difficultySet = "special";
   
   if (%missionInfoObject.gameType $= "")
      %missionInfoObject.gameType = GameMissionInfo.SPMode;
      
   if (%missionInfoObject.difficultySet $= "custom")
      CustomSinglePlayMissionGroup.add(%missionInfoObject);
   else if (%missionInfoObject.gameType $= GameMissionInfo.SPMode)
      SinglePlayMissionGroup.add(%missionInfoObject);
   else if (%missionInfoObject.gameType $= GameMissionInfo.MPMode)
      MultiPlayMissionGroup.add(%missionInfoObject);
   else if (%missionInfoObject.gameType $= GameMissionInfo.SpecialMode)
      SpecialMissionGroup.add(%missionInfoObject);
      
   if (%missionInfoObject.difficultySet $= "custom")
   {
      %missionInfoObject.customId = %missionInfoObject.name @ %path;
      error("CustomID: " @ %missionInfoObject.customId);
   }

   //%missionInfoObject.type = %groupTab;
   %missionInfoObject.setName("");
	%missionInfoObject.file = %missionFile;
   %missionInfoObject.missionGroup = fileBase(%missionFile) @ "Group";
   
   if (%missionInfoObject.type $= "beginner")
      %missionInfoObject.sky = $sky_beginner;
   if (%missionInfoObject.type $= "intermediate")
      %missionInfoObject.sky = $sky_intermediate;
   if (%missionInfoObject.type $= "advanced")
      %missionInfoObject.sky = $sky_advanced;
      
   if (%missionInfoObject.customType $= "beginner")
      %missionInfoObject.sky = $sky_beginner;
   if (%missionInfoObject.customType $= "intermediate")
      %missionInfoObject.sky = $sky_intermediate;
   if (%missionInfoObject.customType $= "advanced")
      %missionInfoObject.sky = $sky_advanced;
}
