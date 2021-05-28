//-----------------------------------------------------------------------------
// Torque Shader Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------

function kick(%client)
{
   messageAll( 'MsgAdminForce', "", "kick", %client.name);

   if (!%client.isAIControlled())
      //BanList::add(%client.guid, %client.getAddress(), $Pref::Server::KickBanTime);
      $banlist[%client.xbLiveId] = getRealTime() TAB $Server::KickBanTime;
   %client.delete("CR_YOUAREKICKED");
}

function ban(%client)
{
   messageAll('MsgAdminForce', "", "ban", %client.name);

   if (!%client.isAIControlled())
      //BanList::add(%client.guid, %client.getAddress(), $Pref::Server::BanTime);
      $banlist[%client.xbLiveId] = getRealTime() TAB $Server::BanTime;
   %client.delete("CR_YOUAREBANNED");
}
