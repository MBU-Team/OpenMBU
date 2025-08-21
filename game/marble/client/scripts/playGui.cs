//-----------------------------------------------------------------------------
// Torque Game Engine
// 
// Copyright (c) 2001 GarageGames.Com
// Portions Copyright (c) 2001 by Sierra Online, Inc.
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// PlayGui is the main TSControl through which the game is viewed.
// The PlayGui also contains the hud controls.
//-----------------------------------------------------------------------------
function PlayGui::AddSubDialog(%this, %dialog)
{
   if (!isObject(%this.subDialogs))
   {
      %this.subDialogs = new SimSet();
      RootGroup.add(%this.subDialogs); // beware $instantGroup !
   }
   %this.subDialogs.add(%dialog);
}

function PlayGui::onWake(%this)
{
   // Turn off any shell sounds...
   // sfxStop( ... );
   
   // Load user prefs
   //if( xbIsProfileContentReady( $Input::LockedController ) )
   //   loadUserPrefs();

   %this.timerInc = 50;
   // just update the action map here
   if($playingDemo)
      demoMap.push();
   else
      moveMap.push();

   if (isObject(%this.subDialogs))
      for (%i = 0; %i < %this.subDialogs.getCount(); %i++)
         Canvas.pushDialog(%this.subDialogs.getObject(%i));
   
   // Fix inputs getting stuck
   clearInputs();

   if (ServerConnection.isMultiplayer)// || $Game::SPGemHunt)
   {
      Canvas.pushDialog(PlayerListGui);
      
      // Always expanded by default (Bug 14955) -pw
      // expand player list by default in widescreen version
      //if (isWidescreen() && ShortPlayerList.visible)
      //   togglePlayerListLength();
      // contract player list by default in non widescreen
      //if (!isWidescreen() && !ShortPlayerList.visible)
      //   togglePlayerListLength();
   }
   else if (PlayerListGui.isAwake())
      Canvas.popDialog(PlayerListGui);
      
   if ($gamePauseWanted)
      Canvas.pushDialog(GamePauseGui);
   else
      setHudVisible( true );
      
   DemoTimerGui.init(PlayGui);
      
   PG_BoostBar.setVisible(true);
      
   RootGui.setTitle("");
   if (! $GamePauseGuiActive)
      RootBackgroundBitmaps.SetVisible(false);
   else
      RootGui.setTitle($Text::Paused);
   
   // hack city - these controls are floating around and need to be clamped
   schedule(0, 0, "refreshCenterTextCtrl");
   schedule(0, 0, "refreshBottomTextCtrl");
   //playGameMusic();
   %this.setPowerUp( "" );
   %this.setBlastBar();
   
   if (newMessageHud.isAwake())
   {
      schedule(10, 0, deactivateKeyboard);
      NMH_Type.schedule(10, makeFirstResponder, true);
   }

   // hack town - pause if system UI is active
   if ($Client::SystemUIActive && !GamePauseGui.isAwake() && !$GameEndNoAllowPause)
   {
      pauseGame();
      Canvas.pushDialog(GamePauseGui);
   }
   
   // make sure our controller is plugged in
   checkLockedController();

   if(!isDemoLaunch())
   {
	   %levelPlaying = GameMissionInfo.getCurrentMission().level;
	   %hasLevel = PDLCAllowMission(%levelPlaying);
	   %isFreeLevel = (%levelPlaying == $PDLC::MapPack0LevelStart);
	   UpsellGui.displayPDLCUpsell = %isFreeLevel ? false : !%hasLevel;
   }

   autosplitterSetIsLoading(false);
   autosplitterSetLevelStarted(false);
   sendAutosplitterData("loading finished");
}

function PlayGui::show(%this)
{
   // Make sure the display is up to date
   if (!$GamePauseGuiActive) 
   {
	  	%this.resizeGemSlots();	
	    %this.setGemCount(%this.gemCount);
	    %this.setMaxGems(%this.maxGems);
	    %this.setPoints(%this.points);
	    %this.scaleGemArrows();
	   
		// recenter center things
      reCenter(TimeBox, 0);
      reCenter(HelpTextBox, 0);
      reCenter(ChatTextBox, 0); 
	}	
}

function PlayGui::onSleep(%this)
{
   if (PlayerListGui.isAwake())
      Canvas.popDialog(PlayerListGui);
   if (GamePauseGui.isAwake())
      Canvas.popDialog(GamePauseGui);

   if (isObject(%this.subDialogs))
      for (%i = 0; %i < %this.subDialogs.getCount(); %i++)
         Canvas.popDialog(%this.subDialogs.getObject(%i));
      
   // Terminate all playing sounds
   sfxStopAll($SimAudioType);
   //playShellMusic();
   // pop the keymaps
   moveMap.pop();
   //demoMap.pop();
   
   // Fix inputs getting stuck
   clearInputs();
}

//-----------------------------------------------------------------------------
function PlayGui::setMessage(%this,%text,%timer)
{
   if (%text $= "ready")
   {
      // Handle controler removed
      if( $ControlerRemovedDuringLoad )
      {
         lockedControlerRemoved();
      }
      
      // Handle network cable pull
      if( $NetworkPulledDuringLoad )
      {
         $NetworkPulledDuringLoad = false;
         NetworkDisconnectGui.onNetworkDisconnect();
      }
   }

   switch$ (%text)
   {
      case "ready": 
      	%text = ($Client::connectedMultiplayer) ? $Text::InGameReady : "";              
      case "go": 
      	%text = ($Client::connectedMultiplayer) ? $Text::InGameGo : "";                   
      case "outOfBounds": 
      	%text = $Text::InGameOutofBounds;
	}
   
   // Set the center message text
   if (%text !$= "")  {
		CenterMessageDlgText.setText(%text);
      CenterMessageDlg.setVisible(true);
      cancel(CenterMessageDlg.timer);
      if (%timer)
         CenterMessageDlg.timer = CenterMessageDlg.schedule(%timer,"setVisible",false);
   }
   else
      CenterMessageDlg.setVisible(false);
}

//-----------------------------------------------------------------------------
function PlayGui::setPowerUp(%this,%bmpFile)
{
   %this.currentPowerUpFile = %bmpFile;
   %path = "marble/client/ui/game/pc/";
   if ($pref::UI::LegacyUI)
      %path = "marble/client/ui/game/";
   
   // Update the power up hud control
   if (%bmpFile $= "")
   {
      HUD_ShowPowerUp.setBitmap(%path @ "powerup.png");
   }
   else
   {
      HUD_ShowPowerUp.setBitmap(%path @ %bmpFile);
   }
}   

function PlayGui::setBlastBar(%this)
{
   %path = "marble/client/ui/game/pc/";
   if ($pref::UI::LegacyUI)
      %path = "marble/client/ui/game/";
      
   BoostBarFG.setBitmap(%path @ "powerbar.png");
}

function PlayGui::updatePlayGui(%this)
{
   %this.setPowerUp(%this.currentPowerUpFile);
   %this.setBlastBar();
}

//-----------------------------------------------------------------------------

function PlayGui::resizeGemSlots(%this)
{
	// These control gemCount/Max number placement
	$gemsVertPos = 4;
	$showGemVertPos = 2;
	
   // resize the position variables
   %newExt = GemBox.extent;
   %oldExt = GemBox.normextent;
   %horzScale = getWord(%newExt,0)/getWord(%oldExt,0);
   
   $gemsFoundFirstSlot  = 20*%horzScale;
	$gemsFoundSecondSlot = 38*%horzScale;
	$gemsFoundThirdSlot  = 56*%horzScale;
	                          
	$gemsSlashPos = 73*%horzScale;
	                          
	$gemsTotalFirstSlot  = 89*%horzScale;
	$gemsTotalSecondSlot = 107*%horzScale;
	$gemsTotalThirdSlot  = 125*%horzScale;
	                          
	$showGemFirstSlot = 107*%horzScale;
	$showGemSecondSlot = 125*%horzScale;
	$showGemThirdSlot = 144*%horzScale;
}

function PlayGui::setMaxGems(%this,%count)
{
   if ($Game::SPGemHunt)
      return;
   %this.maxGems = %count;
   %one = %count % 10;
   %ten = (%count - %one) % 100;
   %hundred = (%count - %ten - %one) % 1000;
   %ten /= 10;
   %hundred /= 100;
   GemsTotalHundred.setNumber(%hundred);
   GemsTotalTen.setNumber(%ten);
   GemsTotalOne.setNumber(%one);
   %visible = %count != 0;

   // Decide which of these should be visible and
   // where they should be positioned
   if (%count < 10)
   {
      GemsFoundOne.setVisible(%visible);
      GemsTotalOne.setVisible(%visible);
      GemsFoundTen.setVisible(false);
      GemsTotalTen.setVisible(false);
      GemsFoundHundred.setVisible(false);
      GemsTotalHundred.setVisible(false);

      GemsFoundOne.position = $gemsFoundThirdSlot - 48 SPC $gemsVertPos;
      GemsTotalOne.position = $gemsTotalFirstSlot - 48 SPC $gemsVertPos;

      GemsSlash.position = $gemsSlashPos - 48 SPC $gemsVertPos;

      HUD_ShowGem.setVisible(%visible);
      HUD_ShowGem.position = $showGemFirstSlot - 48 SPC $showGemVertPos;
   }
   else if (%count < 100)
   {
      GemsFoundOne.setVisible(%visible);
      GemsTotalOne.setVisible(%visible);
      GemsFoundTen.setVisible(%visible);
      GemsTotalTen.setVisible(%visible);
      GemsFoundHundred.setVisible(false);
      GemsTotalHundred.setVisible(false);

      GemsFoundOne.position = $gemsFoundThirdSlot - 24 SPC $gemsVertPos;
      GemsTotalOne.position = $gemsTotalSecondSlot - 24 SPC $gemsVertPos;
      GemsFoundTen.position = $gemsFoundSecondSlot - 24 SPC $gemsVertPos;
      GemsTotalTen.position = $gemsTotalFirstSlot - 24 SPC $gemsVertPos;

      GemsSlash.position = $gemsSlashPos - 24 SPC $gemsVertPos;

      HUD_ShowGem.setVisible(%visible);
      HUD_ShowGem.position = $showGemSecondSlot - 24 SPC $showGemVertPos;
   }
   else
   {
      GemsFoundOne.setVisible(%visible);
      GemsTotalOne.setVisible(%visible);
      GemsFoundTen.setVisible(%visible);
      GemsTotalTen.setVisible(%visible);
      GemsFoundHundred.setVisible(%visible);
      GemsTotalHundred.setVisible(%visible);

      GemsFoundOne.position = $gemsFoundThirdSlot SPC $gemsVertPos;
      GemsTotalOne.position = $gemsTotalThirdSlot SPC $gemsVertPos;
      GemsFoundTen.position = $gemsFoundSecondSlot SPC $gemsVertPos;
      GemsTotalTen.position = $gemsTotalSecondSlot SPC $gemsVertPos;
      GemsFoundHundred.position = $gemsFoundFirstSlot SPC $gemsVertPos;
      GemsTotalHundred.position = $gemsTotalFirstSlot SPC $gemsVertPos;

      GemsSlash.position = $gemsSlashPos SPC $gemsVertPos;

     %width = getWord(getResolution(), 0);
     if (%width > 640)
        HUD_ShowGem.setVisible(%visible);
     else
        HUD_ShowGem.setVisible(false);
     HUD_ShowGem.position = $showGemThirdSlot SPC $showGemVertPos;
   }
   
   GemsSlash.setVisible(%visible);
   SUMO_NegSign.setVisible(false);

   //HUD_ShowGem.setModel("marble/data/shapes/items/gem.dts","");
}

function PlayGui::setGemCount(%this,%count)
{
   if ($Game::SPGemHunt)
      return;
   %this.gemCount = %count;
   %one = %count % 10;
   %ten = (%count - %one) % 100;
   %hundred = (%count - %ten - %one) % 1000;
   %ten /= 10;
   %hundred /= 100;
   GemsFoundHundred.setNumber(%hundred);
   GemsFoundTen.setNumber(%ten);
   GemsFoundOne.setNumber(%one);
}

//-----------------------------------------------------------------------------
function PlayGui::setPoints(%this, %points)
{
   if (!$Game::SPGemHunt)
      return;
   %this.points = %points;
   %pts = %points;

   %drawNeg = false;
   %negOffset = 0;
   if(%pts < 0)
   {
      %pts = - %pts;
      %drawNeg = true;
      %negOffset = 24;
   }

   %one = %pts % 10;
   %ten = (%pts - %one) % 100;
   %hundred = (%pts - %ten - %one) % 1000;
   %ten /= 10;
   %hundred /= 100;
   GemsFoundHundred.setNumber(%hundred);
   GemsFoundTen.setNumber(%ten);
   GemsFoundOne.setNumber(%one);

   %visible = true;//%pts != 0;

   GemsSlash.setVisible(false);
   HUD_ShowGem.setVisible(false);
   GemsTotalOne.setVisible(false);
   GemsTotalTen.setVisible(false);
   GemsTotalHundred.setVisible(false);

   SUMO_NegSign.setVisible(%drawNeg);

   // Decide which of these should be visible and
   // where they should be positioned
   if (%pts < 10)
   {
      GemsFoundOne.setVisible(%visible);
      GemsFoundTen.setVisible(false);
      GemsFoundHundred.setVisible(false);

      GemsFoundOne.position = $gemsFoundThirdSlot - %negOffset SPC $gemsVertPos;
      
      //HUD_ShowGem.setVisible(%visible);
      //HUD_ShowGem.position = $showGemFirstSlot - 48 SPC $showGemVertPos;
   }
   else if (%pts < 100)
   {
      GemsFoundOne.setVisible(%visible);
      GemsFoundTen.setVisible(%visible);
      GemsFoundHundred.setVisible(false);

      GemsFoundOne.position = $gemsFoundThirdSlot SPC $gemsVertPos;
      GemsFoundTen.position = $gemsFoundSecondSlot SPC $gemsVertPos;
      
      //HUD_ShowGem.setVisible(%visible);
      //HUD_ShowGem.position = $showGemSecondSlot - 24 SPC $showGemVertPos;
   }
   else
   {
      GemsFoundOne.setVisible(%visible);
      GemsFoundTen.setVisible(%visible);
      GemsFoundHundred.setVisible(%visible);

      GemsFoundOne.position = $gemsFoundThirdSlot + %negOffset SPC $gemsVertPos;
      GemsFoundTen.position = $gemsFoundSecondSlot + %negOffset SPC $gemsVertPos;
      GemsFoundHundred.position = $gemsFoundFirstSlot + %negOffset SPC $gemsVertPos;

     //%width = getWord(getResolution(), 0);
     //if (%width > 640)
        //HUD_ShowGem.setVisible(%visible);
     //else
        //HUD_ShowGem.setVisible(false);
     //HUD_ShowGem.position = $showGemThirdSlot SPC $showGemVertPos;
   }
   
   HUD_ShowGem.setVisible(%visible);
   HUD_ShowGem.position = $showGemFirstSlot - 48 SPC $showGemVertPos;
}

//-----------------------------------------------------------------------------
// Elapsed Timer Display
function PlayGui::setTimer(%this,%dt)
{
   %this.elapsedTime = %dt;
   %this.updateControls();
}

function PlayGui::resetTimer(%this)
{
   %this.elapsedTime = 0;
   %this.stopTimer();
   %this.updateControls();
}

function PlayGui::startTimer(%this)
{
}

function PlayGui::stopTimer(%this)
{
   if($BonusSfx !$= "")
   {
      sfxStop($BonusSfx);
      $BonusSfx = "";
   }
}

function PlayGui::updateTimer(%this, %time, %bonusTime)
{
   if (%bonusTime && $BonusSfx $= "")
   {
      $BonusSfx = sfxPlay(TimeTravelLoopSfx);
   }
   else if (%bonusTime == 0 && $BonusSfx !$= "")
   {
      sfxStop($BonusSfx);
      $BonusSfx = "";
   }

   %this.elapsedTime = %time;

   // Some sanity checking
   if (%this.elapsedTime > 3600000)
      %this.elapsedTime = 3599999;
   %this.updateControls();
}   
function PlayGui::updateControls(%this)
{
   if (PlayGui.gameDuration)
      %et = PlayGui.gameDuration - %this.elapsedTime;
   else
      %et = %this.elapsedTime;
   %drawNeg = false;
   if(%et < 0)
   {
      %et = - %et;
      %drawNeg = true;
   }
   %hundredth = mFloor((%et % 1000) / 10);
   %totalSeconds = mFloor(%et / 1000);
   %seconds = %totalSeconds % 60;
   %minutes = (%totalSeconds - %seconds) / 60;
   %secondsOne      = %seconds % 10;
   %secondsTen      = (%seconds - %secondsOne) / 10;
   %minutesOne      = %minutes % 10;
   %minutesTen      = (%minutes - %minutesOne) / 10;
   %hundredthOne    = %hundredth % 10; 
   %hundredthTen    = (%hundredth - %hundredthOne) / 10;
   // Update the controls
   Min_Ten.setNumber(%minutesTen);
   Min_One.setNumber(%minutesOne);
   Sec_Ten.setNumber(%secondsTen);
   Sec_One.setNumber(%secondsOne);
   Sec_Tenth.setNumber(%hundredthTen);
   Sec_Hundredth.setNumber(%hundredthOne);
   PG_NegSign.setVisible(%drawNeg);
   
   if (%this.lastHundredth != %hundredth) 
   {
   	TimeBox.animBitmap("timebackdrop");
   	%this.lastHundredth = %hundredth;
	}
}

function PlayGui::scaleGemArrows(%this)
{
	// defaults
// 	%ellipseScreenFractionX = "0.790000";
// 	%ellipseScreenFractionY = "0.990000";
	%fullArrowLength = "60";
	%fullArrowWidth = "40";
// 	%maxArrowAlpha = "0.6";
// 	%maxTargetAlpha = "0.4";
 	%minArrowFraction = "0.4";
	
	// defaults based on 640x480 scale up accordingly
// 	%xscale = (getWord(RootShapeNameHud.getExtent(), 0)/640);
	%yscale = (getWord(RootShapeNameHud.getExtent(), 1)/480);
	
//  	RootShapeNameHud.ellipseScreenFraction = (%ellipseScreenFractionX*%xscale) SPC (%ellipseScreenFractionY*%yscale);
 	RootShapeNameHud.fullArrowLength = %fullArrowLength*%yscale;
 	RootShapeNameHud.fullArrowWidth = %fullArrowWidth*%yscale;
//   	RootShapeNameHud.minArrowFraction = %minArrowFraction*%yscale;
}


//-----------------------------------------------------------------------------
function GuiBitmapCtrl::setNumber(%this,%number)
{
   %this.setBitmap($Con::Root @ "~/client/ui/game/numbers/" @ %number @ ".png");
}

function GuiBitmapCtrl::animBitmap(%this,%bitmapPrefix)
{
   %this.setBitmap($Con::Root @ "~/client/ui/game/" @ %bitmapPrefix @ %this.animCurrent @ ".png");
   %this.animCurrent = (%this.animCurrent >= %this.animMax) ? 0 : %this.animCurrent++;
}

//-----------------------------------------------------------------------------
function refreshBottomTextCtrl()
{
   BottomPrintText.position = "0 0";
}
function refreshCenterTextCtrl()
{
   CenterPrintText.position = "0 0";
}

