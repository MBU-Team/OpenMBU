//-----------------------------------------------------------------------------
// Torque Game Engine
// 
// Copyright (c) 2001 GarageGames.Com
// Portions Copyright (c) 2001 by Sierra Online, Inc.
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// Message Hud
//-----------------------------------------------------------------------------
// chat hud sizes
$outerChatLenY[1] = 72;
$outerChatLenY[2] = 140;
$outerChatLenY[3] = 200;
// Only play sound files that are <= 5000ms in length.
$MaxMessageWavLength = 5000;
// Helper function to play a sound file if the message indicates.
// Returns starting position of wave file indicator.
function playMessageSound(%message, %voice, %pitch)
{
   // Search for wav tag marker.
   %wavStart = strstr(%message, "~w");
   if (%wavStart == -1) {
      return -1;
   }
   %wav = getSubStr(%message, %wavStart + 2, 1000);
   if (%voice !$= "") {
      %wavFile = "~/data/sound/voice/" @ %voice @ "/" @ %wav;
   }
   else {
      %wavFile = "~/data/sound/" @ %wav;
   }
   if (strstr(%wavFile, ".wav") != (strlen(%wavFile) - 4)) {
      %wavFile = %wavFile @ ".wav";
   }
   // XXX This only expands to a single filepath, of course; it
   // would be nice to support checking in each mod path if we
   // have multiple mods active.
   %wavFile = ExpandFilename(%wavFile);
   if ((%pitch < 0.5) || (%pitch > 2.0)) {
      %pitch = 1.0;
   }
   %wavLengthMS = sfxGetWaveLen(%wavFile) * %pitch;
   if (%wavLengthMS == 0) {
      error("** WAV file \"" @ %wavFile @ "\" is nonexistent or sound is zero-length **");
   }
   else if (%wavLengthMS <= $MaxMessageWavLength) {
      if ($ClientChatHandle[%sender] != 0) {
         sfxStop($ClientChatHandle[%sender]);
      }
      $ClientChatHandle[%sender] = sfxCreateSource(AudioMessage, %wavFile);
      if (%pitch != 1.0) {
         sfxSourcef($ClientChatHandle[%sender], "AL_PITCH", %pitch);
      }
      sfxPlay($ClientChatHandle[%sender]);
   }
   else {
      error("** WAV file \"" @ %wavFile @ "\" is too long **");
   }
   return %wavStart;
}

// All messages are stored in this HudMessageVector, the actual
// MainChatHud only displays the contents of this vector.
new MessageVector(HudMessageVector);
$LastHudTarget = 0;

//-----------------------------------------------------------------------------
// let newfangled NewChatHud handle this
//function onChatMessage(%message, %voice, %pitch)
//{
   //// XXX Client objects on the server must have voiceTag and voicePitch
   //// fields for values to be passed in for %voice and %pitch... in
   //// this example mod, they don't have those fields.
   //// Clients are not allowed to trigger general game sounds with their
   //// chat messages... a voice directory MUST be specified.
   //if (%voice $= "") {
      //%voice = "default";
   //}
   //// See if there's a sound at the end of the message, and play it.
   //if ((%wavStart = playMessageSound(%message, %voice, %pitch)) != -1) {
      //// Remove the sound marker from the end of the message.
      //%message = getSubStr(%message, 0, %wavStart);
   //}
   //// Chat goes to the chat HUD.
   //addChatLine(%message);
//}
function chatFade(%fade)
{
   ChatTextForeground.setAlpha(%fade);
   if(%fade > 0)
   {
      %nextFade = (!$GamePauseGuiActive) ? %fade - 0.1 : %fade;
      if(%nextFade < 0)
         %nextFade = 0;
      $ChatFadeTimer = schedule(32, 0, chatFade, %nextFade);
   }
   else
   {
      ChatTextForeground.setVisible( false );
   }
}
function addChatLine(%message)
{
   if (getWordCount(%message)) {
      %text = "<just:center><font:Arial Bold:30>" @ %message;
      ChatTextForeground.setText(%text @ "\n ");
      cancel($ChatFadeTimer);
      ChatTextForeground.setAlpha(1.0);
      ChatTextForeground.setVisible( true );
      $ChatFadeTimer = schedule(3000, 0, chatFade, 1.0);
   }
}
package GuiMLTextHelper
{
// strip out any <func:> tags and call the display func on them
function GuiMLTextCtrl::setText(%this, %text)
{
   %start = 0;
   while((%pos = strpos(%text, "<func:", %start)) != -1)
   {
      %end = strpos(%text, ">", %pos + 5);
      if(%end == -1)
         break;
      %pre = getSubStr(%text, 0, %pos);
      %post = getSubStr(%text, %end+1, 100000);
      %func = getSubStr(%text, %pos + 6, %end - (%pos + 6));
      %val = %this.evalTextFunc(%func);
      %start = strlen(%val) + %pos;
      %text = %pre @ %val @ %post;
   }
   Parent::setText(%this, %text);
}
};
activatePackage(GuiMLTextHelper);

function getMapDisplayName( %device, %action, %fullText )
{
	if ( strstr( %device, "joystick" ) != -1 )
	{
		// Substitute "joystick" for "button" in the action string:
		%pos = strstr( %action, "button" );
		if ( %pos != -1 )
		{
			%mods = getSubStr( %action, 0, %pos );
			%object = getSubStr( %action, %pos, 1000 );
			%instance = getSubStr( %object, strlen( "button" ), 1000 );
         
         switch( %instance )
         {
            case 0:
               return $Text::ButtonA; //"<bitmap:marble/client/ui/xbox/pad_button_a.png>";
            case 1:
               return $Text::ButtonB; //"<bitmap:marble/client/ui/xbox/pad_button_b.png>";
            case 2:
               return $Text::ButtonX; //"<bitmap:marble/client/ui/xbox/pad_button_x.png>";
            case 3:
               return $Text::ButtonY; //"<bitmap:marble/client/ui/xbox/pad_button_y.png>";
            case 4:
               return $Text::ButtonBlack; //"<bitmap:marble/client/ui/xbox/pad_button_black.png>";
            case 5:
               return $Text::ButtonWhite; //"<bitmap:marble/client/ui/xbox/pad_button_white.png>";
            case 6:
               return $Text::LTrigger; //"<bitmap:marble/client/ui/xbox/pad_trigger_l.png>";
            case 7:
               return $Text::RTrigger; //"<bitmap:marble/client/ui/xbox/pad_trigger_r.png>";
            case 8:
               return $Text::ButtonStart; //"<bitmap:marble/client/ui/xbox/pad_button_start.png>";
            case 9:
               return $Text::ButtonBack; //"<bitmap:marble/client/ui/xbox/pad_button_back.png>";
            case 10:
               return $Text::LStickBtn; //"<bitmap:marble/client/ui/xbox/pad_stick_l_press.png>";
            case 11:
               return $Text::RStickBtn; //"<bitmap:marble/client/ui/xbox/pad_stick_r_press.png>";
         }
		}
		else
	   { 
	      %pos = strstr( %action, "pov" );
         if ( %pos != -1 )
         {
            %wordCount = getWordCount( %action );
            %mods = %wordCount > 1 ? getWords( %action, 0, %wordCount - 2 ) @ " " : "";
            %object = getWord( %action, %wordCount - 1 );
            switch$ ( %object )
            {
               case "upov":   %object = "POV1 up";
               case "dpov":   %object = "POV1 down";
               case "lpov":   %object = "POV1 left";
               case "rpov":   %object = "POV1 right";
               case "upov2":  %object = "POV2 up";
               case "dpov2":  %object = "POV2 down";
               case "lpov2":  %object = "POV2 left";
               case "rpov2":  %object = "POV2 right";
               default:       %object = "??";
            }
            return( %mods @ %object );
         }
      }
	}
   
   error( "Unsupported Joystick input object passed to getDisplayMapName! Device: " @ %device @ " Action: " @ %action );

	return( "??" );
}

function GuiMLTextCtrl::evalTextFunc(%this, %text)
{
   %func = getWord(%text, 0);
   switch$(%func)
   {
      case "bind":
         %binding = moveMap.getBinding(getWord(%text, 1));
         echo( "Trying to find binding for: " @ %binding );
         return getMapDisplayName(getField(%binding, 0), getField(%binding, 1), true);
   }
}
function helpFade(%fade)
{
   HelpTextForeground.setAlpha(1.0 * %fade);
   if(%fade > 0)
   {
      %nextFade = (!$GamePauseGuiActive) ? %fade - 0.1 : %fade;
      if(%nextFade < 0)
         %nextFade = 0;
      $HelpFadeTimer = schedule(32, 0, helpFade, %nextFade);
   }
   else
   {
      HelpTextForeground.setVisible(false);
   }   
}
function addHelpLine(%message, %playBeep, %fade)
{
   // default for %fade is true
   if (%fade $= "")
      %fade = true;
      
   %fontSize = "<font:Arial Bold:30>";
      
   if (getWordCount(%message)) {
      %text = "<just:center>" @ %fontSize @ %message;
      HelpTextForeground.setText("<color:ebebeb>" @ %text @ "\n ");
      cancel($HelpFadeTimer);
      HelpTextForeground.setAlpha(1.0);
      HelpTextForeground.setVisible(true);
      if (%fade)
         $HelpFadeTimer = schedule(3000, 0, helpFade, 1.0);
   }
   if(%playBeep)
   {
      serverplay2d(HelpDingSfx);
   }
}
function addStartMessage(%message)
{
   %fontSize = "<font:Arial Bold:30>";
      
   if (getWordCount(%message)) 
   {
      %text = "<just:center>" @ %fontSize @ %message;
      HelpTextForeground.setText("<color:ebebeb>" @ %text @ "\n ");
      cancel($HelpFadeTimer);
      HelpTextForeground.setAlpha(1.0);
      HelpTextForeground.setVisible(true);
      $HelpFadeTimer = schedule(5000, 0, helpFade, 1.0);
   }
}
function onServerMessage(%message)
{
   // See if there's a sound at the end of the message, and play it.
   if ((%wavStart = playMessageSound(%message)) != -1) {
      // Remove the sound marker from the end of the message.
      %message = getSubStr(%message, 0, %wavStart);
   }
   addChatLine(%message);
}
