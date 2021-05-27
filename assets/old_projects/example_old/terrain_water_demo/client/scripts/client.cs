//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
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
//   HudClock.setTime(%time);
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
   Canvas.setContent(PlayGui);
}