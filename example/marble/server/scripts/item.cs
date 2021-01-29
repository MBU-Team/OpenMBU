//-----------------------------------------------------------------------------
// Torque Game Engine
// 
// Copyright (c) 2001 GarageGames.Com
//-----------------------------------------------------------------------------

// These scripts make use of dynamic attribute values on Item datablocks,
// these are as follows:
//
//    maxInventory      Max inventory per object (100 bullets per box, etc.)
//    pickupText        Text to display when client pickups item
//
// Item objects can have:
//
//    count             The # of inventory items in the object.  This
//                      defaults to maxInventory if not set.

//-----------------------------------------------------------------------------
// ItemData base class methods used by all items
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------

$RespawnThreadTimer = 1000;

function Item::respawn(%this, %time)
{
   // This method is used to respawn static ammo and weapon items
   // and is usually called when the item is picked up.
   // Instant fade...
   %this.startFade(0, 0, true);
   %this.setHidden(true);
   %this.respawnThread(%time+$RespawnThreadTimer);
}   

function Item::respawnThread(%this, %time)
{
   if (!$gamePaused)
   {
      %time -= $RespawnThreadTimer;
      // echo("Tick...tick..." SPC %time SPC "ms till respawn");
   }
   else
   {
      // echo("Game paused," SPC %time SPC "ms till respawn");
   }

   if (%time<=0)
   {
      // Schedule a reapearance
      %this.setHidden(false);
      %this.schedule(100, "startFade", 1000, 0, false);
   }
   else
      %this.schedule($RespawnThreadTimer,"respawnThread",%time);
}

function Item::onMissionReset(%this, %checkpointConfirmNum)
{
   // Don't reset objects if they weren't found at the last checkpoint the user
   // was at. (Gems mostly)
   if( %checkpointConfirmNum && %this.checkPointConfirmationNumber <= %checkpointConfirmNum )
      return;
   else
   {
      cancelAll(%this);
      %this.setHidden(false);
      %this.startFade(0, 0, false);
      %this.checkPointConfirmationNumber = 0;
   }
}

function stringToPowerupId( %pickupText )
{
   switch$( %pickupText )
   {
      case $Text::ASuperJump:
         return 0;
      case $Text::ASuperSpeed:
         return 1;
      case $Text::AGyrocopter:
         return 2;
      case $Text::AGravityMod:
         return 3;
      case $Text::AMegaMarble:
         return 4;
      case $Text::ABlast:
         return 5;
      case $Text::ATimeTravelBonus:
         return 6;
   }
   
   if( %pickupText !$= "" )
      error( "ERROR - PICKUP STRING BEING TRANSMITTED NON LOCALIZED: " @ %this.pickupText );
      
   return -1;
}

function powerupIdToString( %id, %data )
{
   switch( %id )
   {
      case 0:
         return $Text::ASuperJump;
      case 1:
         return $Text::ASuperSpeed;
      case 2:
         return $Text::AGyrocopter;
      case 3:
         return $Text::AGravityMod;
      case 4:
         return $Text::AMegaMarble;
      case 5:
         return $Text::ABlast;
      case 6:
         return avar($Text::ATimeTravelBonus,%data);
   }
   
   if( %id != -1 )
      error( "ERROR - PIKCUP STRING TO ID FAILED FOR ID: " @ %id );
      
   return "";
}

function ItemData::getPickupTextData(%this, %obj)
{
   return 0;
}

function ItemData::getPickupTextId(%this, %obj)
{
   return stringToPowerupId( %this.pickupText );
}

function ItemData::getPickupText(%this, %obj)
{
   return %this.pickupText;
}

function ItemData::onPickup(%this,%obj,%user,%amount)
{
   // Inform the client what they got.
   if (%user.client && !%this.noPickupMessage)
   {
      if( %this.getPickupTextId(%obj) != -1 )
         messageClient(%user.client, 'MsgLocalizedItemPickup', 0, %this.getPickupTextId(%obj),%this.getPickupTextData(%obj));
      else
         messageClient(%user.client, 'MsgItemPickup', %this.getPickupText(%obj), %this.getPickupTextData(%obj));
   }
     

   // If the item is a static respawn item, then go ahead and
   // respawn it, otherwise remove it from the world.
   // Anything not taken up by inventory is lost.

   if (%this.permanent || %obj.permanent)
   {
      %obj.setCollisionTimeout(%user);
   }
   else
   {
      if (%obj.isStatic())
      {
         if (%this.noRespawn)
            %obj.setHidden(true);
         else
         {
            if (%obj.respawnTime > 0)
               %obj.respawn(%obj.respawnTime);
            else if (%this.respawnTime > 0)
               %obj.respawn(%this.respawnTime);
            else
               %obj.respawn($Item::RespawnTime);
         }
      }
      else
         %obj.delete();
   }

   return true;
}


//-----------------------------------------------------------------------------
// Hook into the mission editor.

function ItemData::create(%data)
{
   // The mission editor invokes this method when it wants to create
   // an object of the given datablock type.  For the mission editor
   // we always create "static" re-spawnable rotating objects.
   %obj = new Item() {
      dataBlock = %data;
      static = true;
      rotate = true;
   };
   return %obj;
}


