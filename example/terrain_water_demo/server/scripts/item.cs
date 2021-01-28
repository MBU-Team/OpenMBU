//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

// These scripts make use of dynamic attribute values on Item datablocks,
// these are as follows:
//
//    maxInventory      Max inventory per object (100 bullets per box, etc.)
//    pickupName        Name to display when client pickups item
//
// Item objects can have:
//
//    count             The # of inventory items in the object.  This
//                      defaults to maxInventory if not set.

// Respawntime is the amount of time it takes for a static "auto-respawn"
// object, such as an ammo box or weapon, to re-appear after it's been
// picked up.  Any item marked as "static" is automaticlly respawned.
$Item::RespawnTime = 20 * 1000;

// Poptime represents how long dynamic items (those that are thrown or
// dropped) will last in the world before being deleted.
$Item::PopTime = 10 * 1000;


//-----------------------------------------------------------------------------
// ItemData base class methods used by all items
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------

function Item::respawn(%this)
{
   // This method is used to respawn static ammo and weapon items
   // and is usually called when the item is picked up.
   // Instant fade...
   %this.startFade(0, 0, true);
   %this.hide(true);

   // Shedule a reapearance
   %this.schedule($Item::RespawnTime, "hide", false);
   %this.schedule($Item::RespawnTime + 100, "startFade", 1000, 0, false);
}   

function Item::schedulePop(%this)
{
   // This method deletes the object after a default duration. Dynamic
   // items such as thrown or drop weapons are usually popped to avoid
   // world clutter.
   %this.schedule($Item::PopTime - 1000, "startFade", 1000, 0, true);
   %this.schedule($Item::PopTime, "delete");
}


//-----------------------------------------------------------------------------
// Callbacks to hook items into the inventory system

function ItemData::onThrow(%this,%user,%amount)
{
   // Remove the object from the inventory
   if (%amount $= "")
      %amount = 1;
   if (%this.maxInventory !$= "")
      if (%amount > %this.maxInventory)
         %amount = %this.maxInventory;
   if (!%amount)
      return 0;
   %user.decInventory(%this,%amount);

   // Construct the actual object in the world, and add it to 
   // the mission group so it's cleaned up when the mission is
   // done.  The object is given a random z rotation.
   %obj = new Item() {
      datablock = %this;
      rotation = "0 0 1 " @ (getRandom() * 360);
      count = %amount;
   };
   MissionGroup.add(%obj);
   %obj.schedulePop();
   return %obj;
}

function ItemData::onPickup(%this,%obj,%user,%amount)
{
   // Add it to the inventory, this currently ignores the request
   // amount, you get what you get.  If the object doesn't have
   // a count or the datablock doesn't have maxIventory set, the
   // object cannot be picked up.
   %count = %obj.count;
   if (%count $= "")
      if (%this.maxInventory !$= "") {
         if (!(%count = %this.maxInventory))
            return;
      }
      else
         %count = 1;
   %user.incInventory(%this,%count);

   // Inform the client what they got.
   if (%user.client)
      messageClient(%user.client, 'MsgItemPickup', 'You picked up %1', %this.pickupName);

   // If the item is a static respawn item, then go ahead and
   // respawn it, otherwise remove it from the world.
   // Anything not taken up by inventory is lost.
   if (%obj.isStatic())
      %obj.respawn();
   else
      %obj.delete();
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

//-----------------------------------------------------------------------------
// TEST ITEM
//-----------------------------------------------------------------------------
datablock ItemData( BoxTest )
{
   shapeFile = "~/data/shapes/test/newBox2.dts";
   dynamicReflection = false;
};

