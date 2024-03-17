function getDiscordUsername(%userId)
{
    echo("USER ID: " @ %userId);
    if (isObject(DiscordUsernameRequest))
        DiscordUsernameRequest.delete();
    new HTTPObject(DiscordUsernameRequest);
    DiscordUsernameRequest.get("https://openmbu.com", "/api/v1/discord/user/" @ %userId, "");
}

function DiscordUsernameRequest::onLine(%this, %line)
{
    %resp = jsonParse(%line);
    if (%resp.code == 200)
    { 
        %this.success = true;
        if ($Player::Name $= $Player::DiscordUsername && %resp.realname !$= "") // Change only if we haven't changed during the time the HTTP request was occuring
        {
            $Player::Name = %resp.realname;
        }
    } 
    else
    {
        %this.success = false;
    }
    %resp.delete();
}

function DiscordUsernameRequest::onDisconnect(%this)
{
    if (!%this.success) {
        echo("Failed to fetch discord display name!");
    }
    %this.delete();
}

function refreshPDLC()
{
	echo(" *** Refreshing PDLC ownership");

	// Enumerate content:
	if(!isDemoLaunch())
	{
		// Re-enumerate PDLC, and 
		checkForPDLC();

		// Figure out what screen we be on:
		%contentGui = canvas.getContent().contentGui;
		%content = %contentGui.getName();

		// Refresh only those which matter:
		if((%content $= "MainMenuGui") || (%content $= "CreateGameGui") || 
		   (%content $= "MultiPlayerGui" ) || (%content $= "LevelPreviewGui"))
		{
            echo(" *** Refreshing screen " SPC %content);

			// redisplay content on the root gui so that it can update itself to full version
			RootGui.redisplayContent();
		}
	}
}

function onXBLiveSignInStateChanged()
{
   error("@@@@@ Sign in state changed");   
   XBLiveClearLoadedStats();
   
   clientUpdatePlayerInfo();   
}

function onXBLiveSignoff(%port)
{
   error("@@@@@ User signed off on port" SPC %port);
     
   // call script signoff callback.  we don't wait until system ui deactivates (if its active).  might
   // need to delay until it deactivates.  will wait for feedback from test team.
   clientOnSignedOff(%port);

   refreshPDLC();
}

function onXBLiveSignon(%port)
{
   // somebody signed on
   echo(" *** XBLive signon to port: " SPC %port);
   refreshPDLC();
      
   // if we have a locked controller we ignore this
//   if ($Input::LockedController == -1 || $Input::LockedController $= "-1")
//   {
//      error("@@@@@ User signed in, updating sign-in port" SPC %port);
//      clientSetSigninPort(%port);
//   }
//   else
//   {
//      error("@@@@@ User signed on port" SPC %port SPC "but controller is locked on port" SPC $Input::LockedController SPC " - ignoring");
//   }
}

function onXBLiveSystemUIActivated()
{
   error("@@@@@ System UI activated");
   
   $Client::SystemUIActive = true;
   
   if (PlayGui.isAwake() && !GamePauseGui.isAwake() && !$GameEndNoAllowPause)
   {
      pauseGame();
      Canvas.pushDialog(GamePauseGui);
   }
   
   //pauseGame();
}

function onXBLiveSystemUIDeactivated()
{
   error("@@@@@ System UI deactivated");
   
   $Client::SystemUIActive = false;

   // do not resume game here, let user resume manually from pause menu.
   // fixes issues with game getting unpaused when it shouldn't.
   //resumeGame();
      
   if ($Client::SigningIn)
      clientCompleteSignIn();
      
   if ($Client::ViewingMarketplace)
      clientCompleteMarketplace();
      
   if ($Client::ProfileContentCallback !$= "")
   {
      eval($Client::ProfileContentCallback);
      $Client::ProfileContentCallback = "";
   }
   
   //if ($Client::RequestSignIn)
   //   clientRequestSignIn();
      
   if ($Client::SignInCompleteCallback !$= "")
   {
      eval($Client::SignInCompleteCallback);
      $Client::SignInCompleteCallback = "";
   }
}

function clientInitProfileContent(%callback)
{
   if (!XBLiveIsSignedIn() || xbIsProfileContentReady(XBLiveGetSignInPort()))
   {
      eval(%callback);
      return;
   }
   
   $Client::ProfileContentCallback = %callback;
   xbInitProfileContent(XBLiveGetSignInPort(), true);
}

//-----------------------------------------------------------------------------
// Marketplace
//-----------------------------------------------------------------------------
function clientShowMarketplaceUI()
{
//   if (!XBLiveIsSignedInSilver())
//   {
//      error("client not signed in with silver access, can't show marketplace");
//      return;
//   }
//   if (!isDemoLaunch() && hasAllMapPacks())
//   {
//      // uhhh
//      error("attempting to view marketplace and we have nothing to purchase...");
//      return;
//   }

   UpsellGui.hasMapPack1 = hasMapPack1();
   UpsellGui.hasMapPack2 = hasMapPack2();
   UpsellGui.hasFreeMap = hasFreeMapPack();

   if (!isDemoLaunch())
	   $PDLCMarketView = true;

   // Setup category/itemId for XBShowMarketplaceUI:
   if(isDemoLaunch())
   {
      %category = -1;
      %itemId = 1;
   }
   else
   {
      %category = $PDLC::ContentCategory;
      %itemId = 0;
   }
   
   $Client::ViewingMarketplace = XBShowMarketplaceUI(XBLiveGetSigninPort(), %category, %itemId);
}

function clientCompleteMarketplace()
{
   if (!$Client::ViewingMarketplace)
      return;
      
   $Client::ViewingMarketplace = false;
   
   // if we are not a demo launch now, then they must have purchased
   if (!isDemoLaunch())
   {
      echo("User purchased game, converting demo to full version");
      
      if (UpsellGui.isAwake())
      {
          if(RootGui.BEnabled())
             UpsellGui.onB();
          else if(RootGui.AEnabled())
             UpsellGui.onA();
      }
      
      // just in case
      $Demo::TimeRemaining = $Demo::DefaultStartingTime; 
      ServerConnection.demoOutOfTime = false;
      
      // Re-enumerate PDLC, and 
      checkForPDLC();
	  StartAsynchContentQuery();
      
      // redisplay content on the root gui so that it can update itself to full version
      RootGui.redisplayContent();
      
      // thank them for their money
      if( !$PDLCMarketView )
         XMessagePopupDlg.show(0, $Text::PurchasedThanks, $Text::OK);
	  else
	  {
		 %hasPack1 = hasMapPack1();
		 %hasPack2 = hasMapPack2();
		 %hasFreeMap = hasFreeMapPack();
	
		 // thank them for buying map packs
		 if(!UpsellGui.hasMapPack2 && %hasPack2 && !UpsellGui.hasMapPack1 && %hasPack1)
		 {
			 echo("TEST**********************bought both thank you************************");

			 XMessagePopupDlg.show(0, $Text::PDLCThanksPackBoth, $Text::OK);
		 }
		 else if(!UpsellGui.hasMapPack1 && %hasPack1) 
		 {
			 echo("TEST**********************pack 1 thank you************************");

			 XMessagePopupDlg.show(0, $Text::PDLCThanksPack1, $Text::OK);
		 }
		 else if(!UpsellGui.hasMapPack2 && %hasPack2)
		 {
			 echo("TEST**********************pack 2 thank you************************");

			 XMessagePopupDlg.show(0, $Text::PDLCThanksPack2, $Text::OK);
		 }
		 else if(!UpsellGui.hasFreeMap && %hasFreeMap) 
		 {
			 echo("TEST**********************free map thank you************************");

			 XMessagePopupDlg.show(0, $Text::PDLCFreeThanks, $Text::OK);
         }
      }
   }

   // Update ownership flags:
   UpsellGui.hasMapPack1 = hasMapPack1();
   UpsellGui.hasMapPack2 = hasMapPack2();
   UpsellGui.hasFreeMap = hasFreeMapPack();

   $PDLCMarketView = false;
}

function onXBStorageDevicesChanged()
{
	echo(" *** Storage device change");
	refreshPDLC();
}

//-----------------------------------------------------------------------------
// voice notifications
//-----------------------------------------------------------------------------
function onXBLiveVoiceStatus(%status)
{
   echo("updating voice status" SPC %status);
   if ($Client::connectedMultiplayer)
	   commandToServer('SetVoiceStatus', %status);
}

function onXBLivePlayerTalking(%xbLiveId, %talking)
{
   echo("updating talking status" SPC %talking);
   if ($Client::connectedMultiplayer)
   {
      LobbyGui.updateCommunicatorTalking(%xbLiveId, %talking);
      PlayerListGui.updateVoiceStatus(%xbLiveId, %talking);
   }
}

function onXBLiveMuteListChanged()
{
   echo("updating mute list");
   if ($Client::connectedMultiplayer)
      LobbyGui.updateMuteStatus();
}

//-----------------------------------------------------------------------------
// invites
//-----------------------------------------------------------------------------
// this misnamed function is actually called after the invite is accepted but 
// before the search for the target match begins.
function onXBLivePreInviteAccept()
{
   error("@@@@@ Pre Invite Accept");
   
   // if the esrb gui or main menu gui is awake, we may be accepting an invite that 
   // occured on startup.  hide the gui and display a "please wait" message while we search
   // for the invite match.  the "onXBLiveInviteFound" function below will get called whether
   // the search is successful or not, and it will restore a gui in all cases.  we only do 
   // this extra UI processing in the case of an invite accepted on startup or main menu, 
   // because this is the only time where messing with the UI in this fashion has 
   // sufficiently low risk of introducing bugs.
   if (ESRBGui.isAwake() || MainMenuGui.isAwake())
   {
      RootGui.removeContent();
      RootGui.setCenterText($Text::TestingNetwork);
      RootGui.displayLoadingAnimation( "center" );
   }      
}

function onXBLiveInviteFound(%port)
{
   error("@@@@@ Invite Found");
   
   // invite search complete.  if there is no content on the root gui, activate 
   // the main menu so that the user isn't stuck if we don't complete the invite process 
   // for some reason.  see comment in onXBLivePreInviteAccept for an example of where
   // this code will be used.
   if (!RootGui.hasContent())
   {
      echo("   No content on RootGui, activating MainMenu for UI fallback");
      RootGui.setContent(MainMenuGui);
   }
      
   // don't accept invite if game is not fully loaded.  rely on ESRBGui to call us when it exits
   if (!$Client::GameLoaded)
   {
      echo("   Not processing invite, game not loaded");
      return;
   }
   // if the ESRB gui is awake, don't accept yet.
   // ESRB will call us again when it exits and 
   // we will accept then
   if (ESRBGui.isAwake())
   {
      echo("   Not processing invite, ESRB is active");
      return;
   }
 
   if ($Server::Hosting || $Game::Running)
   {
      // need to confirm destructive action blah blah
      if (PlayGui.isAwake() && !GamePauseGui.isAwake())
      {
         Canvas.pushDialog(GamePauseGui);
         pauseGame();
      }
      if ($Server::Hosting)
         %msg = $Text::HostMMInviteConfirm;
      else
         %msg = $Text::ExitLevelConfirm; //$Text::ClientMMConfirm;
      %callback = "doInviteAccept(" @ %port @ ");";
      XMessagePopupDlg.show(0, %msg, $Text::OK, %callback, $Text::Cancel);
   }
   else
      doInviteAccept(%port);
}

function doInviteAccept(%port)
{
   if (GamePauseGui.isAwake())
      Canvas.popDialog(GamePauseGui);
   resumeGame(); // just in case
   
   echo("INVITE:" SPC isObject(ESRBGui) SPC ESRBGui.isAwake());
   
   // indicate that we have accepted this invite
   XBLiveAcceptInvite();
   
   // get the match result count
   %count = XBLiveGetMatchResultsCount();
   
   if (%count == 0)
   {
      // hmmmm
      error("No matches found after invite completed");
      return;
   }
   
   if (%count > 1)
   {
      // hmmmmmmmmm
      error("Whoa, more than one match returned from invite process.  Using first");
   }
   
   // get the entry
   %entry = XBLiveGetMatchResultsEntry(0);
   if (%entry $= "")
   {
      // hmm
      error("Invite match result entry is empty");
      return;
   }
   
   // check demo status
   %selfDemo = isDemoLaunch();
   %otherDemo = getField(%entry, 7);
   
   // allow full version players to join demos, but not vice versa
   // if we're a demo and the other is not, display some upsell
   if (%selfDemo && !%otherDemo)
   {
      pauseGame();
      XMessagePopupDlg.show(0, $Text::InviteBuyNow, $Text::OK, "resumeGame();");
      return;
   }
   
   // JMQ: this appears to be unnecessary, the system UI blocks silver accounts from
   // joining games
   
   // if we're not a gold account, we can't play online, so we get some upsell
//   if (!XBLiveIsSignedInGold())
//   {
//      pauseGame();
//      XMessagePopupDlg.show(0, $Text::InviteGetGold, $Text::OK, "resumeGame();");
//      return;
//   }
  
   // if the accept port is not the sign in port, we need to do the invite port switcharoo
   if (%port != XBLiveGetSignInPort())
   {
      if (!XBLiveIsSignedInGold(%port))
      {
         error("new sign in port is not signed in with gold access, can't accept invite");
         return;
      }
      echo("   Invite accepted on non sign in port, forcing signoff of existing sign in port");
      // this will evict us from current games, if necessary
      clientOnSignedOff(XBLiveGetSignInPort(), true);
      // set new sign in port
      clientSetSigninPort(%port);
   }
   // game invite process complete.  disconnect from old game if playing
   // single or multi player
   else if ($Client::connectedMultiplayer || $Game::Running)
   {
      escapeFromGame(true);
   }
      
   // prevent reconnect to preview server
   cancel($reconnectSched);

   // we are now in multiplayer mode
   $Client::shellMode = "MultiPlayer";
   
   // lock controller to sign in port
   $Input::LockedController = XBLiveGetSigninPort();
      
   // join new game now
   FindGameGui.joinGame(0,true); 
}

function clientUpdatePlayerInfo()
{
   if (XBLiveIsSignedIn()) // defaults to player on the sign in port
   {
      // update $Player variables
      $Player::Name = XBLiveGetUserName();
      $Player::XBLiveId = XBLiveGetUserId();
      if ($Player::Name $= "" || $Player::XBLiveId $= "")
      {
         error("Signed in player information is invalid!");
         $Player::Name = $pref::Player::Name;
      }
   }
   else
   {
      $Player::Name = $pref::Player::Name;
      $Player::XBLiveId = "";
   }
}

function clientSetSigninPort(%port)
{
   XBLiveSetSigninPort(%port);
   clientUpdatePlayerInfo();
   
   if (XBLiveIsSignedIn(%port))
   {
      echo("Loading user prefs");
      // load settings from profile 
      %inverted = XBLiveGetYAxisInversion(%port);
      if (%inverted != -1)
         $pref::invertYCamera = %inverted;
         
      // load profile data
      loadUserProfile();
      
      // load achievements
      echo("Loading achievements");
      XBLiveLoadAchievements(%port, "");
      
      clientLockController(%port);
      
      // set presence info to "menus"
      XBLiveSetRichPresence(%port, 0, "", "");
      
      if( !isDemoLaunch() )
      {
         // Check for PDLC
         checkForPDLC();
      }

	  // Perform initial (blocking) content query for MapPacks category:
	  performContentQuery(2);
   }
}

function clientLockController(%port)
{
   if (isPCBuild())
      return;
      
   error("Locking controller" SPC %port);
   $Input::LockedController = %port; 
   // don't do the locked controller check unless root gui is awake 
   // (so that we don't push the controller dialog while the game is still loading)
   if (RootGui.isAwake()) 
      checkLockedController();
}

function clientUnlockController()
{
   $Input::LockedController = -1;
}

function clientOnSignedOff(%port,%forceSignoff)
{
   // ignore if this isn't the sign in port
   if (XBLiveGetSigninPort() != %port)
      return;
      
   // ok, if we are still signed in then we probably just lost our live connection
   // bounce to the main menu if we are multiplayer, otherwise do nothing
   if (XBLiveIsSignedIn(%port) && !%forceSignoff)
   {
      error("@@@@@ Connection to live lost");
      if ($Client::shellMode $= "MultiPlayer" || $Client::connectedMultiplayer)
      {
         // cannot sign off in Multiplayer, bounce to main menu
         XMessagePopupDlg.show(1, $Text::ErrorSignedOff, $Text::OK);
         
         // boot us out of any active games
         if ($Client::connectedMultiplayer)
         {
            $disconnectGui = MainMenuGui;
            escapeFromGame(true);
         }
         else
            RootGui.setContent(MainMenuGui);
      } // If they are in the score screen, back them out of it.
      else if( levelScoresGui.isAwake() )
         levelScoresGui.onB();
      
      return;
   }
   
   // not signed in at all, bounce to title screen 
   error("@@@@@ Sign in port has signed off");
   
   // delete achievement variables
   deleteVariables("$UserAchievements*");
   deleteVariables("$pref::invert*");
   deleteVariables("$CachedUserTime*");
   deleteVariables("$pref::marbleIndex*");
   // hackamundo: don't want to reexec defaults.cs here, it will stomp things like the demo time
   // remaining
   $pref::Option::MusicVolume = $Option::DefaultMusicVolume;
   $pref::Option::FXVolume = $Option::DefaultFXVolume;
   
   clientUnlockController();
   
   // boot us out of any active games   
   if ($Client::connectedMultiplayer || $Game::Running || MissionLoadingGui.isAwake())
   {
      $disconnectGui = ESRBGui;
      escapeFromGame(true);
   }
   else
      RootGui.setContent(ESRBGui);
}

function jmqtestSlowLoad()
{
   $pref::net::PacketSize = 100;
   $pref::net::PacketRateToClient = 3;
   if (isObject(ServerConnection))
      ServerConnection.checkMaxRate();
   
   for (%i = 0; %i < ClientGroup.getCount(); %i++)
      ClientGroup.getObject(%i).checkMaxRate();
}

// make sure that user is signed in on the current sign in port.  %msg is displayed in a popup window
// first.  if the user B's out of this window, %signInRefuseCallback is called.  see below on how 
// this parameter is interpreted.
// when user hits A on the popup, the system 
// UI is activated to allow the user to sign in.  if user is still not signed in when system UI returns
// the popup is redisplayed.  when the user finally signs in, or if the user was signed in in the first
// place, the prefs and achievements will be loaded if %loadUserPrefs and %loadUserAchievements are true.
// default is to load these.  finally the %callback is called when the process succesfully completes.
//
// the %signInCheckFunction is used to determine whether the user is signed in.  default is 
// XBLiveIsSignedInGold()
//
// if the %signInRefuseCallback is not specified, the default behavior is to re-show the dialog when 
// the user presses B.  If it is "none", the dialog will just disappear.  If it is a user specified
// callback, it will be evaled when the user presses B.

// for debugging
$Client::TraceSignIn = 1;

function clientPromptSignedIn(
   %callback, 
   %msg, %signInCheckFunction, %signInRefuseCallback, 
   %loadUserPrefs, %loadUserAchievements)
{
   if (isPCBuild())
      return;
      
   if ($Client::TraceSignIn)
      echo("Entering clientPromptSignedIn");
      
   $Client::SignInMessage = %msg;
   $Client::SignInCallback = %callback;
     
   // set sign in check function
   if (%signInCheckFunction $= "")
      %signInCheckFunction = "XBLiveIsSignedInGold();";
   $Client::SignInCheckFunction = %signInCheckFunction;
   
   // set sign in refuse callback - default behavior is to re-prompt
   if (%signInRefuseCallback $= "none")
      %signInRefuseCallback = "";
   else if (%signInRefuseCallback $= "")
      %signInRefuseCallback = "clientDoSignIn();";
   $Client::SignInRefuseCallback = %signInRefuseCallback;
         
   // set whether we should load user prefs after sign in
   if (%loadUserPrefs $= "")
      %loadUserPrefs = true;
   $Client::LoadUserPrefs = %loadUserPrefs;
   
   // set whether we should load achievement data after sign in
   if (%loadUserAchievements $= "")
      %loadUserAchievements = true;
   $Client::LoadUserAchievements = %loadUserAchievements;
   
   // see if we are signed in already, if so just complete the sign in
   eval("%signedIn = " @ $Client::SignInCheckFunction);
   if (%signedIn)
      // skip to complete step
      clientCompleteSignIn();
   else 
      // kick off sign in process
      clientDoSignIn();  
}

function clientDoSignIn()
{
   if ($Client::TraceSignIn)
      echo("Entering clientDoSignIn");
   
   // if we have a sign in message, push a dialog to display it.  otherwise just start the sign
   // in process
   if ($Client::SignInMessage !$= "")
      clientShowSignInMessage();
   else
      clientShowSignInUI();
}

// helper function for clientPromptSignedIn
function clientShowSignInUI()
{  
   if ($Client::TraceSignIn)
      echo("Entering clientShowSignInUI");
      
   $Client::SigningIn = true;
   XBLiveShowSigninUI();
}

// helper function for clientPromptSignedIn
function clientShowSignInMessage()
{
   if ($Client::TraceSignIn)
      echo("Entering clientShowSignInMessage");
      
   XMessagePopupDlg.show(0, $Client::SignInMessage, 
      $Text::OK, "clientShowSignInUI();", 
      $Text::Cancel, $Client::SignInRefuseCallback);   
}

$profileVersion = 72;
$profileUpdateTag = 195948557;   // 0xbadf00d
$profileUpdateIdx = 14 + 60 + 1; // previous profile settings + numSinglePlayerLevels

function XBGamerProfile::dumpProfileInfo(%this, %title)
{
   echo(" GamerProfile Info: " @ %title);
   for(%i = 0; %i < 100; %i++)
      echo( " - profileInfo[" @ %i @ "] = " @ %this.intProfileInfo[%i]);
}

function XBGamerProfile::unpackProfile(%this, %string)
{
   %this.dumpProfileInfo("unpackProfile");

   %version = %this.intProfileInfo[0];
   
   if (%version != $profileVersion)
   {
      error("   profile version mismatch, can't load profile string:" SPC $profileVersion SPC %version);
      return;
   }

   // TitleUpdate cannot version this data (users can switch from new<->old version? crap!)
   //  - append the profileInfo with the TitleUpdate achievements
   //  - look for special tag to signify data is present
   %isUpdatedProfile = (%this.intProfileInfo[$profileUpdateIdx] == $profileUpdateTag);

   if(%isUpdatedProfile)
      echo(" !!! Loading updated profile.");

   // if you add something here make sure you delete the appropriate global variable 
   // when the user signs off (see deleteVariables() in clientOnSignedOff())
   $pref::invertXCamera = %this.intProfileInfo[1];
   $pref::invertYCamera = %this.intProfileInfo[2];
   $pref::marbleIndex = %this.intProfileInfo[3];
   $UserAchievements::beginnerLevels = %this.intProfileInfo[4];
   $UserAchievements::IntermediateLevels = %this.intProfileInfo[5];
   $UserAchievements::AdvancedLevels = %this.intProfileInfo[6];
   $UserAchievements::easterEggs = %this.intProfileInfo[7];
   $UserAchievements::beginnerPars = %this.intProfileInfo[8];
   $UserAchievements::IntermediatePars = %this.intProfileInfo[9];
   $UserAchievements::AdvancedPars = %this.intProfileInfo[10];
   $UserAchievements::MPFirstPlace = %this.intProfileInfo[11];
   $UserAchievements::MPHighScore = %this.intProfileInfo[12];
   $pref::Option::MusicVolume = %this.intProfileInfo[13];
   $pref::Option::FXVolume = %this.intProfileInfo[14];
   
   // Map Pack Achievement code:
   if(%isUpdatedProfile)
   {
      $UserAchievements::MPMapPackA = %this.intProfileInfo[$profileUpdateIdx + 1];
      $UserAchievements::MPMapPackB = %this.intProfileInfo[$profileUpdateIdx + 2];
      $UserAchievements::MPMapPackC = %this.intProfileInfo[$profileUpdateIdx + 3];
   }
   else
   {
      $UserAchievements::MPMapPackA = 0;
      $UserAchievements::MPMapPackB = 0;
      $UserAchievements::MPMapPackC = 0;
   }

   if(0) // Clear achievements
   {
      $UserAchievements::beginnerLevels = 0;
      $UserAchievements::IntermediateLevels = 0;
      $UserAchievements::AdvancedLevels = 0;
      $UserAchievements::easterEggs = 0;
      $UserAchievements::beginnerPars = 0;
      $UserAchievements::IntermediatePars = 0;
      $UserAchievements::AdvancedPars = 0;
      $UserAchievements::MPFirstPlace = 0;
      $UserAchievements::MPHighScore = 0;
      $UserAchievements::MPMapPackA = 0;
      $UserAchievements::MPMapPackB = 0;
      $UserAchievements::MPMapPackC = 0;
   }

   if((14 + SinglePlayMissionGroup.getCount()) != ($profileUpdateIdx - 1))
      error(" @@@ Single player mission count assumption blown!");

   for( %i = 0; %i < SinglePlayMissionGroup.getCount(); %i++ )
   {
      %mission = SinglePlayMissionGroup.getObject( %i );
      if((14 + %mission.level) >= $profileUpdateIdx)
         error(" @@@ Profile index out of range for level: " @ %obj.level);
      %cacheTime = %this.intProfileInfo[14 + %mission.level];
      $CachedUserTime::levelTime[%mission.level] = %cacheTime;
   }
   
   // JMQ: commented this out until we know what we are doing here
   
   // Approach1: multiple reads, multiple sessions
   //tryUpdateXBLiveTimesFromCache( 0 );
   
   // Approach2: single read, single or multiple sessions
   // 
//   if (!isDemoLaunch() && XBLiveIsSignedInSilver())
//   {
//      $DoingMegaSPStatLoad = true;
//      // load all single player level data, and when its done update the cache stuff
//      // false means that this read is uncancellable (by leaderboards and level preview gui)
//      // hardcode the leaderboard range because I suck
//      XBLiveReadStats(1, 60, "onXBLiveAllSPStatsLoaded();", false);
//   }

   xbSetMusicVolume($pref::Option::MusicVolume * 0.01);
   xbSetFXVolume($pref::Option::FXVolume * 0.01);
}

//function onXBLiveAllSPStatsLoaded()
//{
//   $DoingMegaSPStatLoad = false;
//   
//   if (!XBLiveIsSignedInSilver())
//      return;
//   
//   // update the live times from profile cached times   
//   // update each mission
//   XBLiveStartStatsSession();
//   for (%i = 0; %i < SinglePlayMissionGroup.getCount(); %i++)
//   {
//      updateXBLiveLevelFromCache2(%i);  
//   }     
//   XBLiveEndStatsSession();
//}
//
//function tryUpdateXBLiveTimesFromCache( %missionIdx )
//{
//   if( %missionIdx > SinglePlayMissionGroup.getCount() )
//      return;
//   
//   %mission = SinglePlayMissionGroup.getObject( %missionIdx );
//   
//   %cacheTime = $CachedUserTime::levelTime[%mission.level];
//   
//   if( %cacheTime > 0 && XBLiveAreStatsLoaded( %mission.level ) )
//      updateXBLiveLevelFromCache( %missionIdx );
//   else if( %cacheTime > 0 )
//      XBLiveReadStats(%mission.level, %mission.level, "updateXBLiveLevelFromCache(" @ %missionIdx @ ");" );
//   else
//      tryUpdateXBLiveTimesFromCache( %missionIdx + 1 );
//}

// this profile-sync variant is used with the tryUpdateXBLiveTimesFromCache function
//function updateXBLiveLevelFromCache( %missionIdx )
//{
//   %mission = SinglePlayMissionGroup.getObject( %missionIdx );
//   
//   // Now check and see if the time is better than the live time, and then
//   // update it if it is.
//   XBLiveStartStatsSession();
//   
//   if( XBLiveAreStatsLoaded( %mission.level ) )
//   {
//      %liveTime = XBLiveGetStatValue( %mission.level, "time" );
//      %cacheTime = $CachedUserTime::levelTime[%mission.level];
//      
//      if( ( %cacheTime < %liveTime && %cacheTime > 0 ) || %liveTime == 0 && %cacheTime > 0 )
//      {
//         %rating = calcRating( %cacheTime, %mission.time, %mission.goldTime + $GoldTimeDelta, %mission.difficulty );
//         // Update the time and rating
//         echo( "%%% Updating XBLive time with better cache time for '" @ %mission.name @ "'. (" @ %cacheTime @ " < " @ %liveTime @ ")" );
//         XBLiveWriteStats(%mission.level, "time", %cacheTime, "");
//         XBLiveWriteStats(%mission.level, "rating", %rating, "tryUpdateXBLiveTimesFromCache(" @ %missionIdx + 1 @ ");" );
//      }
//      else
//         tryUpdateXBLiveTimesFromCache( %missionIdx + 1 );
//   }
//   else
//      echo( "   Stats not loaded, can't do the biscuit." );
//      
//   XBLiveEndStatsSession();
//}

// this profile-sync variant is used with the single read sync method
//function updateXBLiveLevelFromCache2( %missionIdx )
//{
//   %mission = SinglePlayMissionGroup.getObject( %missionIdx );
//     
//   if( XBLiveAreStatsLoaded( %mission.level ) )
//   {
//      %liveTime = XBLiveGetStatValue( %mission.level, "time" );
//      %cacheTime = $CachedUserTime::levelTime[%mission.level];
//      
//      if( ( %cacheTime < %liveTime && %cacheTime > 0 ) || %liveTime == 0 && %cacheTime > 0 )
//      {
//         
//         %rating = calcRating( %cacheTime, %mission.time, %mission.goldTime + $GoldTimeDelta, %mission.difficulty );
//         // Update the time and rating
//         echo( "%%% Updating XBLive time with better cache time for '" @ %mission.name @ "'. (" @ %cacheTime @ " < " @ %liveTime @ ")" );
//         XBLiveWriteStats(%mission.level, "time", %cacheTime, "");
//         XBLiveWriteStats(%mission.level, "rating", %rating, "");
//      }
//   }
//   else
//      echo( "   Stats not loaded, can't do the biscuit." );
//}

function XBGamerProfile::packProfile(%this)
{
   %this.intProfileInfo[0] = $profileVersion;
   %this.intProfileInfo[1] = $pref::invertXCamera;
   %this.intProfileInfo[2] = $pref::invertYCamera;
   %this.intProfileInfo[3] = $pref::marbleIndex;
   %this.intProfileInfo[4] = $UserAchievements::beginnerLevels;
   %this.intProfileInfo[5] = $UserAchievements::IntermediateLevels;
   %this.intProfileInfo[6] = $UserAchievements::AdvancedLevels;
   %this.intProfileInfo[7] = $UserAchievements::easterEggs;
   %this.intProfileInfo[8] = $UserAchievements::beginnerPars;
   %this.intProfileInfo[9] = $UserAchievements::IntermediatePars;
   %this.intProfileInfo[10] = $UserAchievements::AdvancedPars;
   %this.intProfileInfo[11] = $UserAchievements::MPFirstPlace;
   %this.intProfileInfo[12] = $UserAchievements::MPHighScore;
   %this.intProfileInfo[13] = $pref::Option::MusicVolume;
   %this.intProfileInfo[14] = $pref::Option::FXVolume;
   
   for( %i = 0; %i < SinglePlayMissionGroup.getCount(); %i++ )
   {
      %obj = SinglePlayMissionGroup.getObject( %i );
      if((14 + %obj.level) >= $profileUpdateIdx)
         error(" @@@ Profile index out of range for level: " @ %obj.level);
      %this.intProfileInfo[14 + %obj.level] = $CachedUserTime::levelTime[%obj.level];
   }

   // Map Pack Achievement code (cannot bump base version.. ugh):
   %this.intProfileInfo[$profileUpdateIdx] = $profileUpdateTag;
   %this.intProfileInfo[$profileUpdateIdx + 1] = $UserAchievements::MPMapPackA;
   %this.intProfileInfo[$profileUpdateIdx + 2] = $UserAchievements::MPMapPackB;
   %this.intProfileInfo[$profileUpdateIdx + 3] = $UserAchievements::MPMapPackC;

   %this.dumpProfileInfo("packProfile");
   return "";
}

function savePCUserProfile()
{
   export("$pref*", "prefs.cs");
   export("$CachedUserScore*", "scores.cs");
   if (variablesExist("$CachedUserTime*"))
      export("$CachedUserTime*", "old_scores.cs");
   else
      deletePrefs("old_scores.cs");
   export("$UserAchievements*", "achievements.cs");
}

function loadPCUserProfile()
{
   execPrefs("prefs.cs");
   execPrefs("scores.cs");
   execPrefs("old_scores.cs");
   execPrefs("achievements.cs");
   
   convertScores(); // Update from old index/path format to new guid format
   
   xbSetMusicVolume($pref::Option::MusicVolume * 0.01);
   xbSetFXVolume($pref::Option::FXVolume * 0.01);
   
   // Set the new marble biscuit
   commandToServer('SetMarble', $pref::marbleIndex);
}

// Update from old index/path format to new guid format
function convertScores()
{
   if (!variablesExist("$CachedUserTime*"))
      return;
   //%oldMode = GameMissionInfo.getMode();
   //%oldFilter = GameMissionInfo.filterDifficulty();
   //%oldIndex = GameMissionInfo.getCurrentIndex();
   
   //GameMissionInfo.setMode(GameMissionInfo.SPMode);
   for (%i = 0; %i < 100; %i++)
   {
      if ($CachedUserTime::levelTime[%i] !$= "")
      {
         %mission = GameMissionInfo.findMissionById(%i, SinglePlayMissionGroup);
         if (!isObject(%mission))
            continue;
         
         if ($CachedUserScore::LevelScore[%mission.guid] $= "")
         {
            error("Converting score format for" SPC %mission.file);
            $CachedUserScore::LevelScore[%mission.guid] = $CachedUserTime::levelTime[%i];
            
            $CachedUserTime::levelTime[%i] = "";
            //deleteVariables("$CachedUserTime::levelTime[" @ %i @ "]");
         }
      }
   }
   
   //GameMissionInfo.setMode(GameMissionInfo.CustomMode);
   
   %count = CustomSinglePlayMissionGroup.numAddedMissions;
   for (%i = 0; %i < %count; %i++)
   {
      %mission = GameMissionInfo.findMissionById(%i, CustomSinglePlayMissionGroup);
      if (!isObject(%mission))
         continue;
      
      if ($CachedUserTime::customLevelTime[%mission.customId] !$= "")
      {
         if ($CachedUserScore::LevelScore[%mission.guid] $= "")
         {
            error("Converting score format for" SPC %mission.file);
            $CachedUserScore::LevelScore[%mission.guid] = $CachedUserTime::customLevelTime[%mission.customId];
            
            //deleteVariables("$CachedUserTime::customLevelTime[" @ %mission.customId @ "]");
            $CachedUserTime::customLevelTime[%mission.customId] = "";
         }
      }
   }
   
   deleteEmptyVariables("$CachedUserTime*");
   if (variablesExist("$CachedUserTime*"))
      error("There are old scores that could not be mapped!");
   
   //GameMissionInfo.setMode(%oldMode);
   //GameMissionInfo.setFilterDifficulty(%oldFilter);
   //GameMissionInfo.setCurrentIndex(%oldIndex);
   
   savePCUserProfile();
}

function xbSetMusicVolume(%volume)
{
   $pref::SFX::channelVolume[$MusicAudioType] = %volume;
   sfxSetChannelVolume($MusicAudioType, $pref::SFX::channelVolume[$MusicAudioType]);
}

function xbSetFXVolume(%volume)
{
   $pref::SFX::channelVolume[$SimAudioType] = %volume;
   sfxSetChannelVolume($SimAudioType, $pref::SFX::channelVolume[$SimAudioType]);
}

function saveUserProfile()
{
   if (isDemoLaunch())
      return;

   if (!XBLiveIsSignedIn())
      return;
      
   %port = XBLiveGetSignInPort();
   
   echo("Saving user profile for port" SPC %port);
   
   if (isPCBuild())
   {
      savePCUserProfile();
      return;
   }
   
   if (!isObject(CurrentGamerProfile))
   {
      echo("No CurrentGamerProfile, making one");
      new XBGamerProfile(CurrentGamerProfile);
      RootGroup.add(CurrentGamerProfile); // so that it doesn't go into the f***ing MissionCleanup group via $instantGroup
      CurrentGamerProfile.init(%port);
   }
   
   %string = CurrentGamerProfile.packProfile();
   echo("   Profile string is" SPC strlen(%string) SPC "characters long");
   //CurrentGamerProfile.dump(); // Commenting this out for now -pw
   
   echo("Saving profile for object" SPC CurrentGamerProfile.getId());
   if( !CurrentGamerProfile.saveProfile(%string) )
   {
      if ($XBGamerProfile::SaveFailed)
      {
         error("Profile save failed!");
      }
      else
      {
         echo("Scheduling future profile save due to failure");
         $saveUserProfileSchedule = schedule( 1000, 0, "saveUserProfile" );
      }   
   }
   else
      cancel( $saveUserProfileSchedule );
}

function loadUserProfile()
{
   if (!XBLiveIsSignedIn())
      return;
   
   if (!isPCBuild())
   {
      if( isObject( CurrentGamerProfile ) && CurrentGamerProfile.isBusy() )
      {
         $delUserProfileSchedule = schedule( 1000, 0, "loadUserProfile" );
         return;
      }
      else
         cancel( $delUserProfileSchedule );
   }
   
   %port = XBLiveGetSignInPort();
   
   echo("Loading user profile for port" SPC %port);
   
   if (isPCBuild())
   {
      loadPCUserProfile();
      return;
   }
   
   // delete any previous profile object
   if (isObject(CurrentGamerProfile))
      CurrentGamerProfile.delete();
   
   new XBGamerProfile(CurrentGamerProfile);
   RootGroup.add(CurrentGamerProfile); // so that it doesn't go into the f***ing MissionCleanup group via $instantGroup
   CurrentGamerProfile.init(%port);
   
   echo("Loading profile for object" SPC CurrentGamerProfile.getId());
   if( !CurrentGamerProfile.loadProfile() )
      $loadUserProfileSchedule = schedule( 1000, 0, "loadUserProfile" );
   else
      cancel( $loadUserProfileSchedule );
}

function XBGamerProfile::onProfileLoaded(%this, %string)
{
   echo("Profile Loaded:" SPC %string);

   %this.unpackProfile(%string);
   
   // Set the new marble biscuit
   commandToServer('SetMarble', $pref::marbleIndex);
}

function XBGamerProfile::onBlankProfileLoaded( %this )
{
   echo( "BLANK profile loaded...setting default settings" );
   deleteVariables("$UserAchievements*");
   deleteVariables("$pref::invert*");
   deleteVariables("$CachedUserTime*");
   deleteVariables("$pref::marbleIndex*");
   
   $pref::invertYCamera = XBLiveGetYAxisInversion( XBLiveGetSigninPort() );

   $pref::Option::MusicVolume = $Option::DefaultMusicVolume;
   $pref::Option::FXVolume = $Option::DefaultFXVolume;
   
   commandToServer('SetMarble', $pref::marbleIndex);
}

function XBAwardGamerPictureDone( %success )
{
   // If we want to do any processing regarding gamer picture awards, do it here
}

// helper function for clientPromptSignedIn 
function clientCompleteSignIn()
{
   if ($Client::TraceSignIn)
      echo("Entering clientCompleteSignIn");
      
   $Client::SigningIn = false;
   
   // if we are not signed in, retry the signin until we get an explicit refusal from user
   eval("%signedIn = " @ $Client::SignInCheckFunction);
   if (!%signedIn)
   {
      clientDoSignIn();
      return;
   }
   
   // we are now signed in, get sign in port
   %port = XBLiveGetSignInPort();
   
   // lock the sign in port
   //clientLockController(%port);
   
   if ($Client::LoadUserPrefs)
   {
//      echo("Loading user prefs");
//      // load settings from profile 
//      %inverted = XBLiveGetYAxisInversion(%port);
//      if (%inverted != -1)
//         $pref::invertYCamera = %inverted;
//         
//      // load profile data
//      loadUserProfile();
   }
   
//   if ($Client::LoadUserAchievements)
//   {
//      loadAchData();
//   }
      
   eval($Client::SignInCallback);
//   if (isDemoLaunch())
//   {
//      // demo doesn't load pref variables from disk and we are not allowed to prompt for a save device, 
//      // so we're done.
//      eval($Client::SignInCallback);
//      return;
//   }
//   
//   // start the select save device process
//   clientSelectSaveDevice();
}

//// helper function for clientPromptSignedIn 
//function clientSelectSaveDevice()
//{
//   if ($Client::TraceSignIn)
//      echo("Entering clientSelectSaveDevice");
//      
//   if (isDemoLaunch())
//   {
//      error("clientSelectSaveDevice: This shouldn't happen");
//      eval($Client::SignInCallback);
//      return;
//   }
//   
//   %port = XBLiveGetSignInPort();
//   
//   // check for content device status
//   // if the device is already set up, just complete the process
//   if (xbIsProfileContentReady(%port))
//      clientOnSelectDeviceComplete();
//   else
//      // prompt to select device, set up a callback that will fire when system UI deactivates
//      // after they have finished
//      XMessagePopupDlg.show(0, $Client::SelectDevicePrompt, 
//         $Text::OK, "clientInitProfileContent(\"clientOnSelectDeviceComplete();\");", 
//         $Text::Cancel, $Client::SignInCallback);
//}
//
//// helper function for clientPromptSignedIn 
//function clientOnSelectDeviceComplete()
//{
//   if ($Client::TraceSignIn)
//      echo("Entering clientOnSelectDeviceComplete");
//      
//   if (!XBLiveIsSignedIn())
//   {
//      // uhhh
//      error("Not signed in after selecting save device");
//      eval($Client::SignInCallback);
//      return;
//   }
//   
//   %port = XBLiveGetSignInPort();
//   
//   if (!xbIsProfileContentReady(%port))
//   {
//      // reprompt for save device
//      clientSelectSaveDevice();
//      return;
//   }
//      
//   if ($Client::LoadUserPrefs)
//   {
//      echo("Loading user prefs");
//      loadUserPrefs();
//   }
//   
//   if ($Client::LoadUserAchievements)
//      loadAchData();
//   
//   eval($Client::SignInCallback);
//}

//------------------------------------------------------------------------------
// friends
//------------------------------------------------------------------------------

function onXBLiveFriendChanged()
{
   error( "@@@ Friends changed, setting stats dirty for friends leaderboard." );
   
   // Force-reload the leaderboards
   XBLiveClearLoadedLeaderboards();
   
   // ACK they are in the friend leaderboard now! Reload it.
   if( LevelScoresGui.isAwake() && levelScoresGui.leaderboard == 2 )
   {
      error( "@@@ Ack, user is in friends LB now, reloading it..." );
      levelScoresGui.updateLeaderboard(false);
   }
}

function XBLiveShowAchievementsUI() { 
   %backgui = RootGui.contentGui;
	RootGui.setContent(AchievementListGui, %backgui);
}

function XBLiveShowGamercardUI(%name, %gamerscore) {
	XMessagePopupDlg.show(0, %name @ "\n\n" @ %gamerscore @ " Gamer Score",$Text::OK, "","", "");
}

//-----------------------------------------------------------------------------
// PDLC/MapPack helpers:
//-----------------------------------------------------------------------------
$PDLC::MapPack0LevelStart = 80;
$PDLC::MapPack0LevelEnd = 80;
$PDLC::MapPack1LevelStart = 81;
$PDLC::MapPack1LevelEnd = 85;
$PDLC::MapPack2LevelStart = 86;
$PDLC::MapPack2LevelEnd = 90;

$PDLC::NumAvailableMapPacks = 0;	// Set to '3' if everything is known live and wanting to avoid query
$PDLC::ContentCategory = 2;

function hasFreeMapPack()
{
   return PDLCAllowMission($PDLC::MapPack0LevelStart);
}

function hasMapPack1()
{
   return PDLCAllowMission($PDLC::MapPack1LevelStart);
}

function hasMapPack2()
{
   return PDLCAllowMission($PDLC::MapPack2LevelStart);
}

function hasAllMapPacks()
{
	if($PDLC::NumAvailableMapPacks >= 3)
		return hasFreeMapPack() && hasMapPack1() && hasMapPack2();
	if($PDLC::NumAvailableMapPacks == 2)
		return hasFreeMapPack() && hasMapPack1();
	if($PDLC::NumAvailableMapPacks == 1)
		return hasFreeMapPack();
	return true;
}

function getMapPackUpsellTag(%level)
{
   if(%level >= $PDLC::MapPack0LevelStart && %level <= $PDLC::MapPack0LevelEnd)
      return $Text::UnlockPDLC0;
   if(%level >= $PDLC::MapPack1LevelStart && %level <= $PDLC::MapPack1LevelEnd)
      return $Text::UnlockPDLC1;
   if(%level >= $PDLC::MapPack2LevelStart && %level <= $PDLC::MapPack2LevelEnd)
      return $Text::UnlockPDLC2;
   return "";
}

function performContentQuery(%contentCategory)
{
	if($PDLC::NumAvailableMapPacks == 0)
	{
		echo(" *** Checking for available content in category:" SPC %contentCategory);

		// Returns "<newContentCount>, <totalContentCount>":
		%content = ContentQuery(%contentCategory);
	
		// Assumed mappacks become live sequentially and no other content is avilable:
		$PDLC::NumAvailableMapPacks = getWord(%content, 1);
	}

	echo("Num content packs available: " SPC $PDLC::NumAvailableMapPacks);
}

// Return boolean if the given level is available to play/purchase (but may not be owned)
function isLevelContentAvailable(%level)
{
	// Assume non-mappack level ok:
	if(%level < $PDLC::MapPack0LevelStart || %level > $PDLC::MapPack2LevelEnd)
		return true;

	// No packs available:
	if($PDLC::NumAvailableMapPacks <= 0)
		return false;

	// Get valid range:
	%start = $PDLC::MapPack0LevelStart;
	%end = $PDLC::MapPack0LevelEnd; 

	if($PDLC::NumAvailableMapPacks == 2)
		%end = $PDLC::MapPack1LevelEnd;

	if($PDLC::NumAvailableMapPacks >= 3)
		%end = $PDLC::MapPack2LevelEnd;

	// Check against range:
	if(%level >= %start && %level <= %end)
		return true;

	return false;
}
//-----------------------------------------------------------------------------

function onXBLiveContentCreateFailed(%contentName)
{
	%message = avar( $Text::CorruptPDLC, %contentName );
	XMessagePopupDlg.show(0, %message, $Text::OK);
}

