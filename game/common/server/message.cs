//-----------------------------------------------------------------------------
// Torque Shader Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Server side message commands
//-----------------------------------------------------------------------------

function messageClient(%client, %msgType, %msgString, %a1, %a2, %a3, %a4, %a5, %a6, %a7, %a8, %a9, %a10, %a11, %a12, %a13)
{
   commandToClient(%client, 'ServerMessage', %msgType, %msgString, %a1, %a2, %a3, %a4, %a5, %a6, %a7, %a8, %a9, %a10, %a11, %a12, %a13);
}

function messageTeam(%team, %msgType, %msgString, %a1, %a2, %a3, %a4, %a5, %a6, %a7, %a8, %a9, %a10, %a11, %a12, %a13)
{
   %count = ClientGroup.getCount();
   for(%cl= 0; %cl < %count; %cl++)
   {
      %recipient = ClientGroup.getObject(%cl);
	  if(%recipient.team == %team)
	      messageClient(%recipient, %msgType, %msgString, %a1, %a2, %a3, %a4, %a5, %a6, %a7, %a8, %a9, %a10, %a11, %a12, %a13);
   }
}

function messageTeamExcept(%client, %msgType, %msgString, %a1, %a2, %a3, %a4, %a5, %a6, %a7, %a8, %a9, %a10, %a11, %a12, %a13)
{
   %team = %client.team;
   %count = ClientGroup.getCount();
   for(%cl= 0; %cl < %count; %cl++)
   {
      %recipient = ClientGroup.getObject(%cl);
	  if((%recipient.team == %team) && (%recipient != %client))
	      messageClient(%recipient, %msgType, %msgString, %a1, %a2, %a3, %a4, %a5, %a6, %a7, %a8, %a9, %a10, %a11, %a12, %a13);
   }
}

function messageAll(%msgType, %msgString, %a1, %a2, %a3, %a4, %a5, %a6, %a7, %a8, %a9, %a10, %a11, %a12, %a13)
{
   %count = ClientGroup.getCount();
   for(%cl = 0; %cl < %count; %cl++)
   {
      %client = ClientGroup.getObject(%cl);
      messageClient(%client, %msgType, %msgString, %a1, %a2, %a3, %a4, %a5, %a6, %a7, %a8, %a9, %a10, %a11, %a12, %a13);
   }
}

function messageAllExcept(%client, %team, %msgtype, %msgString, %a1, %a2, %a3, %a4, %a5, %a6, %a7, %a8, %a9, %a10, %a11, %a12, %a13)
{  
   //can exclude a client, a team or both. A -1 value in either field will ignore that exclusion, so
   //messageAllExcept(-1, -1, $Mesblah, 'Blah!'); will message everyone (since there shouldn't be a client -1 or client on team -1).
   %count = ClientGroup.getCount();
   for(%cl= 0; %cl < %count; %cl++)
   {
      %recipient = ClientGroup.getObject(%cl);
      if((%recipient != %client) && (%recipient.team != %team))
         messageClient(%recipient, %msgType, %msgString, %a1, %a2, %a3, %a4, %a5, %a6, %a7, %a8, %a9, %a10, %a11, %a12, %a13);
   }
}


//---------------------------------------------------------------------------
// Server side client chat'n
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// silly spam protection...
$SPAM_PROTECTION_PERIOD     = 10000;
$SPAM_MESSAGE_THRESHOLD     = 4;
$SPAM_PENALTY_PERIOD        = 10000;
$SPAM_MESSAGE               = '\c3FLOOD PROTECTION:\cr You must wait another %1 seconds.';

function GameConnection::spamMessageTimeout(%this)
{
   if(%this.spamMessageCount > 0)
      %this.spamMessageCount--;
}

function GameConnection::spamReset(%this)
{
   %this.isSpamming = false;
}

function spamAlert(%client)
{
   if($Pref::Server::FloodProtectionEnabled != true)
      return(false);

   if(!%client.isSpamming && (%client.spamMessageCount >= $SPAM_MESSAGE_THRESHOLD))
   {
      %client.spamProtectStart = getSimTime();
      %client.isSpamming = true;
      %client.schedule($SPAM_PENALTY_PERIOD, spamReset);
   }

   if(%client.isSpamming)
   {
      %wait = mFloor(($SPAM_PENALTY_PERIOD - (getSimTime() - %client.spamProtectStart)) / 1000);
      messageClient(%client, "", $SPAM_MESSAGE, %wait);
      return(true);
   }

   %client.spamMessageCount++;
   %client.schedule($SPAM_PROTECTION_PERIOD, spamMessageTimeout);
   return(false);
}


//---------------------------------------------------------------------------

function chatMessageClient( %client, %sender, %voiceTag, %voicePitch, %msgString, %a1, %a2, %a3, %a4, %a5, %a6, %a7, %a8, %a9, %a10 )
{
	//see if the client has muted the sender
	if ( !%client.muted[%sender] )
	   commandToClient( %client, 'ChatMessage', %sender, %voiceTag, %voicePitch, %msgString, %a1, %a2, %a3, %a4, %a5, %a6, %a7, %a8, %a9, %a10 );
}

function chatMessageTeam( %sender, %team, %msgString, %a1, %a2, %a3, %a4, %a5, %a6, %a7, %a8, %a9, %a10 )
{
   if ( ( %msgString $= "" ) || spamAlert( %sender ) )
      return;
   %count = ClientGroup.getCount();
   for ( %i = 0; %i < %count; %i++ )
   {
      %obj = ClientGroup.getObject( %i );
      if ( %obj.team == %sender.team )
         chatMessageClient( %obj, %sender, %sender.voiceTag, %sender.voicePitch, %msgString, %a1, %a2, %a3, %a4, %a5, %a6, %a7, %a8, %a9, %a10 );
   }
}

function chatMessageAll( %sender, %msgString, %a1, %a2, %a3, %a4, %a5, %a6, %a7, %a8, %a9, %a10 )
{
   if ( ( %msgString $= "" ) || spamAlert( %sender ) )
      return;
   %count = ClientGroup.getCount();
   for ( %i = 0; %i < %count; %i++ )
   {
		%obj = ClientGroup.getObject( %i );
		if(%sender.team != 0)
	      chatMessageClient( %obj, %sender, %sender.voiceTag, %sender.voicePitch, %msgString, %a1, %a2, %a3, %a4, %a5, %a6, %a7, %a8, %a9, %a10 );
		else
		{
			// message sender is an observer -- only send message to other observers
			if(%obj.team == %sender.team)
		      chatMessageClient( %obj, %sender, %sender.voiceTag, %sender.voicePitch, %msgString, %a1, %a2, %a3, %a4, %a5, %a6, %a7, %a8, %a9, %a10 );
		}
	}
}

