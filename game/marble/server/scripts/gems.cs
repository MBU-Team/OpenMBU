//-----------------------------------------------------------------------------
// Torque Game Engine
//
// Copyright (c) 2001 GarageGames.Com
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Gem base class
//-----------------------------------------------------------------------------

datablock SFXProfile(GotGemSfx)
{
   filename    = "~/data/sound/gem_collect.wav";
   description = Audio2D;
   volume = 0.6;
   preload = true;
};

datablock SFXProfile(OpponentGotGemSfx)
{
   filename    = "~/data/sound/opponent_gem_collect.wav";
   description = AudioClose3d;
   volume = 0.6;
   preload = true;
};

datablock SFXProfile(GotAllGemsSfx)
{
   filename    = "~/data/sound/gem_all.wav";
   description = Audio2D;
   volume = 0.6;
   preload = true;
};


//-----------------------------------------------------------------------------

$GemSkinColors[0] = "base";

function Gem::onAdd(%this,%obj)
{
   if (%this.skin !$= "")
      %obj.setSkinName(%this.skin);
   else {
      // Random skin if none assigned
      %obj.setSkinName($GemSkinColors[0]);
   }
}

function Gem::onPickup(%this,%obj,%user,%amount)
{
   Parent::onPickup(%this,%obj,%user,%amount);
   %user.client.onFoundGem(%amount,%obj,%this.points);
   return true;
}

function Gem::saveState(%this,%obj,%state)
{
   %state.object[%obj.getId()] = %obj.isHidden();
}

function Gem::restoreState(%this,%obj,%state)
{
   %obj.setHidden(%state.object[%obj.getId()]);
}

//-----------------------------------------------------------------------------

datablock ItemData(GemItem)
{
   // Mission editor category
   category = "Gems";
   className = "Gem";

   // Basic Item properties
   shapeFile = "~/data/shapes/items/gem.dts";
   mass = 1;
   friction = 1;
   elasticity = 0;
   gravityMod = 0;

   // Dynamic properties defined by the scripts
   pickupName = "a gem!";
   maxInventory = 1;
   noRespawn = true;
   gemType = 1;
   noPickupMessage = true;

   scaleFactor = 1.5;

   renderGemAura = true;
   gemAuraTextureName = "~/data/textures/gemAura";
   noRenderTranslucents = true;
   referenceColor = "0.9 0 0";
   points = 1;
   
   addToHUDRadar = true;

   buddyShapeName = "marble/data/shapes/items/gembeam.dts";
   buddySequence = "ambient";
};

datablock ItemData(GemItem_2pts: GemItem) 
{
   skin = "yellow";
   points = 2;
   referenceColor = "0.9 1 0";
};

datablock ItemData(GemItem_5pts: GemItem) 
{
   skin = "blue";
   points = 5;
   referenceColor = "0 0 0.9";
};

datablock ItemData(GemItemRespawn : GemItem)
{
   noRespawn = false;
   respawnTime = 5000;
   referenceColor = "0.9 0 0";
};

// datablock ItemData(GemItemBlue: GemItem) 
// {
   // skin = "blue";
   // points = 3;
// };
// 
// datablock ItemData(GemItemRed: GemItem)
// {
   // skin = "base";
// };
// 
// datablock ItemData(GemItemYellow: GemItem)
// {
   // skin = "base";
// };
// 
// datablock ItemData(GemItemPurple: GemItem)
// {
   // skin = "base";
// };
// 
// datablock ItemData(GemItemGreen: GemItem)
// {
   // skin = "base";
// };
// 
// datablock ItemData(GemItemTurquoise: GemItem)
// {
   // skin = "base";
// };
// 
// datablock ItemData(GemItemOrange: GemItem)
// {
   // skin = "base";
// };
// 
// datablock ItemData(GemItemBlack: GemItem)
// {
   // skin = "base";
// };

