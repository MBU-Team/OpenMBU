//-----------------------------------------------------------------------------
// Torque Game Engine
// 
// Copyright (c) 2001 GarageGames.Com
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// Server Admin Commands
//-----------------------------------------------------------------------------
function SAD(%password)
{
   if (%password !$= "")
      commandToServer('SAD', %password);
}
function SADSetPassword(%password)
{
   commandToServer('SADSetPassword', %password);
}

//----------------------------------------------------------------------------
// Misc server commands
//----------------------------------------------------------------------------
function clientCmdSyncClock(%time)
{
   // Store the base time in the hud control it will automatically increment.
   HudClock.setTime(%time);
}
function GameConnection::prepDemoRecord(%this)
{
   %this.demoChatLines = HudMessageVector.getNumLines();
   for(%i = 0; %i < %this.demoChatLines; %i++)
   {
      %this.demoChatText[%i] = HudMessageVector.getLineText(%i);
      %this.demoChatTag[%i] = HudMessageVector.getLineTag(%i);
      echo("Chat line " @ %i @ ": " @ %this.demoChatText[%i]);
   }
}
function GameConnection::prepDemoPlayback(%this)
{
   for(%i = 0; %i < %this.demoChatLines; %i++)
      HudMessageVector.pushBackLine(%this.demoChatText[%i], %this.demoChatTag[%i]);
   RootGui.setContent(PlayGui);
   $LoadingDone = true;
}

function clientCmdSetPartyId(%partyId)
{
    if (!$Server::Hosting)
        XBLiveSetPartyId(%partyId);
}

//----------------------------------------------------------------------------
// Various client support functions
//----------------------------------------------------------------------------
function clientSetReadyStatus(%ready)
{
   if ($Client::ConnectedMultiplayer && isObject(ServerConnection))
   {
      ServerConnection.ready = %ready;
      commandToServer('SetReadyStatus', %ready); 
   }
}

function clientIsReady()
{
   return ServerConnection.ready;
}

//----------------------------------------------------------------------------
// Misc functions
//----------------------------------------------------------------------------
function isWidescreen()
{
   return $Shell::wideScreen;
}

function ttsa()
{
	$showtitlesafearea = !$showtitlesafearea;
}

function rGui(%gui) // reload gui
{
   if (!isObject(%gui))
      return;
      
   if (%gui.isAwake())
      RootGui.removeContent();
      
   %gui.delete();
   exec("marble/client/ui/" @ %gui @ ".gui");
}

function tGui(%gui) // test gui
{
   if (!isObject(%gui))
      return;

   RootGui.removeContent();
   RootGui.setContent(%gui);
}

function rtGui(%gui) // reload and test gui
{
   rGui(%gui);
   tGui(%gui);
}

function reCenter(%guiCtl, %axis) // recenter gui in parent container. %axis: 0=x 1=y
{
	%parent = %guiCtl.getGroup();
	
	%pExt = %parent.getExtent();
	%gExt = %guiCtl.getExtent();
	%gPos = %guiCtl.getPosition();
	
	%center = (getWord(%pExt,%axis)/2) - (getWord(%gExt,%axis)/2);
	
	if (!%axis) %guiCtl.position = %center SPC getWord(%gPos,1);
	else %guiCtl.position = getWord(%gPos,0) SPC %center;
}