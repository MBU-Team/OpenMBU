// Handy function
function BIT(%place)
{
   return 1 << %place;
}

$totalAchievements = 15;
$totalSPAchievements = 9;

function setupAchievementMasks()
{
   echo("Setting up achievement masks");
   
   $pointThreshold11 = 75;
   $pointThreshold12 = 2000;

   $allBeginnerMask = 0;
   $allIntermediateMask = 0;
   $allAdvancedMask = 0;
   
   $numLevelsBeginner = 0;
   $numLevelsIntermediate = 0;
   $numLevelsAdvanced   = 0;

   // Map Pack Achievement code:
   $numGemsNeededMapPackB = 40;  // You need to get $numGemsNeededMapPackB gems in $numlevelsNeededMapPackB out of the five levels to get the achievement
   $numGemsNeededMapPackC = 50;  // Num gems you need to collect in Spires to get the achievement
   $numLevelsNeededMapPackB = 3;  // You need to get $numGemsNeededMapPackB gems in $numlevelsNeededMapPackB out of the five levels to get the achievement

   $numEasterEggs = 20;
   $allEggsMask = 0;
   for (%i = 1; %i <= $numEasterEggs; %i++)
   {
      $allEggsMask |= BIT(%i);
   }
   
   for (%i = 0; %i < SinglePlayMissionGroup.getCount(); %i++)
   {
      %mis = SinglePlayMissionGroup.getObject(%i);
      
      if (%mis.difficultySet $= "")
      {
         error("   Mission does not have a difficulty set!" SPC %mis.file);
         continue;
      }
      
      %beginnerStart = 1;
      %beginnerEnd = 20;
      
      %intermediateStart = 21;
      %intermediateEnd = 40;
      
      %advancedStart = 41;
      %advancedEnd   = 60;
      
      switch$ (%mis.difficultySet)
      {
         case "beginner":
            // make sure level id is in range
            if (%mis.level < %beginnerStart || %mis.level > %beginnerEnd)
            {
               error("   Mission level is out of range for difficulty:" SPC %mis.file);
               continue;
            }
            $allBeginnerMask |= BIT(%mis.level - %beginnerStart + 1);
            $numLevelsBeginner++;
         case "intermediate":
            // make sure level id is in range
            if (%mis.level < %intermediateStart || %mis.level > %intermediateEnd)
            {
               error("   Mission level is out of range for difficulty:" SPC %mis.file);
               continue;
            }
            $allIntermediateMask |= BIT(%mis.level - %intermediateStart + 1);
            $numLevelsIntermediate++;
         case "advanced":
            // make sure level id is in range
            if (%mis.level < %advancedStart || %mis.level > %advancedEnd)
            {
               error("   Mission level is out of range for difficulty:" SPC %mis.file);
               continue;
            }
            $allAdvancedMask |= BIT(%mis.level - %advancedStart + 1);
            $numLevelsAdvanced++;
      }
   }
   echo("   Found" SPC $numLevelsBeginner SPC "beginner levels");
   echo("   Found" SPC $numLevelsIntermediate SPC "intermediate levels");
   echo("   Found" SPC $numLevelsAdvanced SPC "advanced levels");
   
   if ($Test::CheatsEnabled !$= "" && $Test::EasyAchievements)
   {
      error("   ACHIEVEMENT CHEATS ENABLED!");
      
      $allBeginnerMask = 3;
      $allIntermediateMask = 3;
      $allAdvancedMask = 3;
   
      $numLevelsBeginner = 2;
      $numLevelsIntermediate = 2;
      $numLevelsAdvanced   = 2;
      
      $numEasterEggs = 2;
      $allEggsMask = 3;

      $pointThreshold11 = 10;
      $pointThreshold12 = 20;
   }
}

setupAchievementMasks();

$loadLoops = 0; // Just to make sure we don't end up in an infinite loop somehow

// Check to see if the player has won another achievement
function checkForAchievements()
{
   // Get the Live/controller port
   %port = XBLiveGetSigninPort();

   //// Make sure the player's achievements are loaded
   //if (XBLiveAreAchievementsLoaded(%port))
      //$loadLoops = 0;
   //else
   //{
      //echo("checkForAchievements: I am loading achievements, not good");
      //if ($loadLoops < 3)
      //{
         //XBLiveLoadAchievements(%port, "checkForAchievements();");
         //$loadLoops++;
         //return;
      //}
      //else
         //errorf("@@@@@ checkForAchievements(): unable to load achievements after 3 attempts");
   //}

   // complete a level under par time
   if (!XBLiveHasAchievement(%port, 1))
   {
      %pars = getBeginnerPars() | getIntermediatePars() | getAdvancedPars();
      if (%pars)
         XBLiveWriteAchievement(%port, 1, "notifyAwardOfAchievement(" @ 1 @ ");");
   }

   // Check to see if we have finished all Beginner levels
   if (!XBLiveHasAchievement(%port, 2))
   {
      %fblvs = getBeginnerLevels();
      // Check to see if all of the levels have been completed
      if (countBits(%fblvs) >= countBits($allBeginnerMask))
         XBLiveWriteAchievement(%port, 2, "notifyAwardOfAchievement(" @ 2 @ ");");
   }

   // Check to see if we have finished all Intermediate levels
   if (!XBLiveHasAchievement(%port, 3))
   {
      %falvs = getIntermediateLevels();
      // Check to see if all of the levels have been completed
      if (countBits(%falvs) >= countBits($allIntermediateMask))
         XBLiveWriteAchievement(%port, 3, "notifyAwardOfAchievement(" @ 3 @ ");");
   }

   // Check to see if we have finished all Advanced levels
   if (!XBLiveHasAchievement(%port, 4))
   {
      %felvs = getAdvancedLevels();
      // Check to see if all of the levels have been completed
      if (countBits(%felvs) >= countBits($allAdvancedMask))
         XBLiveWriteAchievement(%port, 4, "notifyAwardOfAchievement(" @ 4 @ ");");
   }
   
   // Check to see if we have gotten all Beginner Par Times
   if (!XBLiveHasAchievement(%port, 5))
   {
      %bPar = getBeginnerPars();
      // Check to see if all of the levels have been completed with Par times
      if (countBits(%bPar) >= countBits($allBeginnerMask))
         XBLiveWriteAchievement(%port, 5, "notifyAwardOfAchievement(" @ 5 @ ");");
   }

   // Check to see if we have gotten all Intermediate Par Times
   if (!XBLiveHasAchievement(%port, 6))
   {
      %aPar = getIntermediatePars();
      // Check to see if all of the levels have been completed with Par times
      if (countBits(%aPar) == countBits($allIntermediateMask))
         XBLiveWriteAchievement(%port, 6, "notifyAwardOfAchievement(" @ 6 @ ");");
   }

   // Check to see if we have gotten all Advanced Par Times
   if (!XBLiveHasAchievement(%port, 7))
   {
      %ePar = getAdvancedPars();
      // Check to see if all of the levels have been completed with Par times
      if (countBits(%ePar) == countBits($allAdvancedMask))
         XBLiveWriteAchievement(%port, 7, "notifyAwardOfAchievement(" @ 7 @ ");");
   }

   // Check to see if we found our first Easter Egg
   if (!XBLiveHasAchievement(%port, 8))
   {
      %eggs = getEasterEggs();
      // Check to see if have any easter eggs
      if (%eggs)
         XBLiveWriteAchievement(%port, 8, "notifyAwardOfAchievement(" @ 8 @ ");");
   }

   // Check to see if we found all Easter Eggs
   if (!XBLiveHasAchievement(%port, 9))
   {
      %eggs = getEasterEggs();
      // Check to see if all of the easter eggs have been found
      if (countBits(%eggs) >= countBits($allEggsMask))
         XBLiveWriteAchievement(%port, 9, "notifyAwardOfAchievement(" @ 9 @ ");");
   }
   
   // Win first place in multiplayer
   if (!XBLiveHasAchievement(%port, 10))
   {
      %gotIt = getFirstPlace();
      if (%gotIt)
         XBLiveWriteAchievement(%port, 10, "notifyAwardOfAchievement(" @ 10 @ ");");
   }

   // Get 75 points in multiplayer
   if (!XBLiveHasAchievement(%port, 11))
   {
      if ($UserAchievements::MPHighScore >= $pointThreshold11)
         XBLiveWriteAchievement(%port, 11, "notifyAwardOfAchievement(" @ 11 @ ");");
   }

   // Get 2000 points in multiplayer
   if (!XBLiveHasAchievement(%port, 12))
   {
      %mpOverallpoints = getMPOverallPoints();
      if (%mpOverallpoints >= $pointThreshold12)
         XBLiveWriteAchievement(%port, 12, "notifyAwardOfAchievement(" @ 12 @ ");");      
   }

   // Map Pack Achievement code:
   // Map Pack A - Get a Blue gem with at least one other person in the level.
   if (!XBLiveHasAchievement(%port, 13))
   {
      echo("\n\n------------------------- TEST ACHIEVEMENT 13");
      %eMapPackA = getMapPackA_Achievement();
      echo("------------------------- MAP PACK A MASK: " @ %eMapPackA);
      if (%eMapPackA)
      {
         echo("------------------------- YOU GOT MAP PACK A ACHIEVEMENT");
         XBLiveWriteAchievement(%port, 13, "notifyAwardOfAchievement(" @ 13 @ ");");
      }
   }

   // Map Pack B - Get X number of gems in 3 out of the 5 levels.  With at least one other person in the level.
   if (!XBLiveHasAchievement(%port, 14))
   {
      echo("\n\n------------------------- TEST ACHIEVEMENT 14");
      %eMapPackB = getMapPackB_Achievement();
      echo("------------------------- MAP PACK B MASK: " @ %eMapPackB);
      if (countBits(%eMapPackB) >= $numLevelsNeededMapPackB)
      {
         echo("------------------------- YOU GOT MAP PACK B ACHIEVEMENT");
         XBLiveWriteAchievement(%port, 14, "notifyAwardOfAchievement(" @ 14 @ ");");
      }
   }

   // Map Pack C - In Spires you have to collect X number of gems.  You can do this solo.
   if (!XBLiveHasAchievement(%port, 15))
   {
      echo("\n\n------------------------- TEST ACHIEVEMENT 15");
      %eMapPackC = getMapPackC_Achievement();
      echo("------------------------- MAP PACK C MASK: " @ %eMapPackC);
      if (%eMapPackC)
      {
         echo("------------------------- U GOT MAP PACK C ACHIEVEMENT");
         XBLiveWriteAchievement(%port, 15, "notifyAwardOfAchievement(" @ 15 @ ");");
      }
   }
}

function XBLiveWriteAchievement(%port, %id, %callback)
{
	$UserAchievementsGot::achieved[%id] = 1;
	eval(%callback);
}

function XBLiveHasAchievement(%port, %id)
{
   return $UserAchievementsGot::achieved[%id] $= 1;
}

function getNumAwardedAchievements()
{
   //if (XBLiveAreAchievementsLoaded(XBLiveGetSigninPort()))
      //return countbits(XBLiveGetAchievementMask(XBLiveGetSigninPort()));
   //return 0;
   
   %num = 0;
   
   for (%i = 1; %i <= $totalAchievements; %i++)
   {
      if(XBLiveHasAchievement(%i))
      {
         %num++;
      }
   }
   
   return %num;
}

function dumpAchievements()
{
   echo("Achievement data");
   
   for (%i = 1; %i <= 12; %i++)
   {
      echo("   " @ %i SPC XBLiveHasAchievement(XBLiveGetSigninPort(), %i));
   }
}

$tasks = 0;
$ctask = 0;
function addTask(%task)
{
   $taskArray[$tasks] = %task;
   $tasks++;
   if ($ctask == 0)
   {
      nextTask();
   }
}

function nextTask()
{
   if ($ctask >= $tasks)
   {
		$tasks = 0;
		$ctask = 0;
		return;
   }
   eval($taskArray[$ctask]);
   $ctask++;
   
}

function notifyAwardOfAchievement(%achievementId)
{
   echo("got achievement" SPC %achievementId);
	addTask("notifyAchievement(" @ %achievementId @ ");"); 
}

function notifyAchievement(%achievementId)
{
   sfxPlay(AchievementSfx);
   //addChatLine("You got the " @ %achievementId @ " achievement!");
   $AchievementId = %achievementId;
   Canvas.pushDialog(AchievementDlg);
   $closeAchievement = schedule(3000, 0, CloseAchievementDlg);
   // TODO: Different notification for completing singleplayer and all achievements?
   //if (hasAllSPAchievements() || getNumAwardedAchievements() == $totalSPAchievements)
   //   $allAchievement = schedule(3500, 0, AllAchievement);
}

function hasAllSPAchievements()
{
   %port = XBLiveGetSignInPort();
   
   for(%i = 1; %i <= $totalSPAchievements; %i++)
   {
      if (!XBLiveHasAchievement(%port, %i))
         return false;
   }
   
   return true;
}

function AllAchievement()
{
   sfxPlay(AllAchievementSfx);
   $AchievementId = "all";
   Canvas.pushDialog(AchievementDlg);
   $closeAchievement = schedule(3000, 0, CloseAchievementDlg);
}

function CloseAchievementDlg()
{
	Canvas.popDialog(AchievementDlg);
	$newAchievement = schedule(500, 0, nextTask);
	
}

// Functions for accessing the various leaderboards to get point/finish values
// Set the id for the Completed Leaderboard here
$completedLB = 95;
// Set the Scrum Aggregate Leaderboard id here
$scrumABoard = 98;

function getBeginnerLevels()
{
   return $UserAchievements::beginnerLevels;
   //return XBLiveGetStatValue($completedLB, "beginnerLevels");
}

function getIntermediateLevels()
{
   return $UserAchievements::IntermediateLevels;
   //return XBLiveGetStatValue($completedLB, "IntermediateLevels");
}

function getAdvancedLevels()
{
   return $UserAchievements::AdvancedLevels;
   //return XBLiveGetStatValue($completedLB, "AdvancedLevels");
}

function getEasterEggs()
{
   return $UserAchievements::easterEggs;
   //return XBLiveGetStatValue($completedLB, "easterEggs");
}

function getBeginnerPars()
{
   return $UserAchievements::beginnerPars;
   //return XBLiveGetStatValue($completedLB, "beginnerPars");
}

function getIntermediatePars()
{
   return $UserAchievements::IntermediatePars;
   //return XBLiveGetStatValue($completedLB, "IntermediatePars");
}

function getAdvancedPars()
{
   return $UserAchievements::AdvancedPars;
   //return XBLiveGetStatValue($completedLB, "AdvancedPars");
}

function getMPOverallPoints()
{
   return XBLiveGetStatValue($scrumABoard, "gems");
}

function getFirstPlace()
{
   return $UserAchievements::MPFirstPlace;
}

// Map Pack Achievement code:
function getMapPackA_Achievement()
{
   return $UserAchievements::MPMapPackA;
}

function getMapPackB_Achievement()
{
   return $UserAchievements::MPMapPackB;
}

function getMapPackC_Achievement()
{
   return $UserAchievements::MPMapPackC;
}

function gotABlueGemInTheLevel()
{
   return $UserAchievements::MPGotABlueGem;
}

// Used to update the achievement leaderboards
function finishedMission(%levelid)
{
   echo("finished mission" SPC %levelid);
   %column = "";
   %offset = 0;

   // Which block of missions are we updating
   if (%levelid < 21)
   {
      %column = "beginnerLevels";
      %offset = 0;
   }
   else if (%levelid < 41)
   {
      %column = "IntermediateLevels";
      %offset = 20;
   }
   else
   {
      %column = "AdvancedLevels";
      %offset = 40;
   }
   
   // Determine the bit that represents this mission
   %bit = BIT(%levelid - %offset);
   //$UserAchievements::BeginnerLevels 238
   eval("$UserAchievements::" @ %column @ " |= %bit;");

   //%val = XBLiveGetStatValue($completedLB, %column);
   //%val |= %bit;

   //XBLiveWriteStats($completedLB, %column, %val, "");
}

function finishedFirstPlaceInMP()
{
   $UserAchievements::MPFirstPlace = 1;
}

// Map Pack Achievement code:
function completedMapPackA_Achievement()
{
   $UserAchievements::MPMapPackA = 1;
}

function completedMapPackB_Achievement(%levelid)
{
   //Levels: 81-85
   %bit = BIT(%levelid - 81);
   eval("$UserAchievements::MPMapPackB |= %bit;");
}

function completedMapPackC_Achievement()
{
   $UserAchievements::MPMapPackC = 1;
}

function finishedPar(%levelid)
{
   echo("finished Par on" SPC %levelid);
   %column = "";
   %offset = 0;

   // Which block of missions are we updating
   if (%levelid < 21)
   {
      %column = "beginnerPars";
      %offset = 0;
   }
   else if (%levelid < 41)
   {
      %column = "IntermediatePars";
      %offset = 20;
   }
   else
   {
      %column = "AdvancedPars";
      %offset = 40;
   }

   // Determine the bit that represents this mission
   %bit = BIT(%levelid - %offset);

   eval("$UserAchievements::" @ %column @ " |= %bit;");
   
   //%val = XBLiveGetStatValue($completedLB, %column);
   //%val |= %bit;

   //XBLiveWriteStats($completedLB, %column, %val, "");
}

function gotEasterEgg(%eggid)
{
   %hasFoundIt = !hasFoundEgg(%eggid);
   
   %bit = BIT(%eggid);

   %column = "easterEggs";
   eval("$UserAchievements::" @ %column @ " |= %bit;");

   // JMQ: don't need stat sessions to save achievements
   // Write out the easter egg immediatly
//   if( clientAreStatsAllowed() && %hasFoundIt )
//   {
//      %thisStartedStatSession = false;
//      
//      if( !XBLiveIsStatsSessionActive() )
//      {
//         %thisStartedStatSession = true;
//         XBLiveStartStatsSession();
//      }
    
   // do achievement save if we're signed in locally or better and it isn't a demo  
   if (%hasFoundIt && !isDemoLaunch() && XBLiveIsSignedIn())
   {
      checkForAchievements();
      saveUserProfile();
   }
      
//      if( %thisStartedStatSession )
//         XBLiveEndStatsSession();
//   }
}

//function loadAchData()
//{
//   if (!XBLiveIsSignedIn())
//   {
//      error("User is not logged on, cannot load achievement data");
//      return;
//   }
//   
//   %port = XBLiveGetSignInPort();
//   
//   if (!xbIsProfileContentReady( %port ))
//   {
//      error("Profile content not ready, can't load achievement data");
//      return;
//   }
//   
//   echo("Loading user achievement data");
//   XBLiveLoadAchData();
//   $UserAchievements::Loaded = true;
//}

//function saveAchData()
//{
//   if (!XBLiveIsSignedIn())
//   {
//      error("User is not logged on, cannot save achievement data");
//      return;
//   }
//   
//   echo("Saving user achievement data");
//   checkProfileWrite("XBLiveSaveAchData(\"$UserAchievements::*\");");
//}