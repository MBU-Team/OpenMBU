//-----------------------------------------------------------------------------
// Torque Game Engine
//
// Copyright (c) 2001 GarageGames.Com
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------

datablock SFXProfile(ButtonClick)
{
   filename    = "~/data/sound/ButtonClick.wav";
   description = AudioDefault3d;
   preload = true;
};


//-----------------------------------------------------------------------------
// Single action button that resets after a default elapsed time.
// When activated the button triggers all the objects in it's group.

datablock StaticShapeData(PushButton)
{
   className = "Button";
   category = "Buttons";
   shapeFile = "~/data/shapes/buttons/pushButton.dts";
   resetTime = 5000;
};

function PushButton::onAdd(%this, %obj)
{
   %obj.activated = false;
   if (%obj.triggerMesg $= "")
      %obj.triggerMesg = "true";
   if (%obj.resetTime $= "")
      %obj.resetTime = "Default";
}

function PushButton::onCollision(%this,%obj,%col,%vec, %vecLen, %material)
{
   echo(%material);
   // Currently activates when any object hits it.
   if (%material $= "ButtonMaterial")
      %this.activate(%obj,true);
}

function PushButton::onEndSequence(%this, %obj, %slot)
{
   if (%obj.activated) {
      // Trigger all the objects in our mission group
      %group = %obj.getGroup();
      for (%i = 0; %i < %group.getCount(); %i++)
         %group.getObject(%i).onTrigger(%this,%obj.triggerMesg);
//         %group.getObject(%i).onTrigger(%this,%this.triggerMesg);

      // Schedule the button reset
      %resetTime = (%obj.resetTime $= "Default")? %this.resetTime: %obj.resetTime;
      if (%resetTime)
         %this.schedule(%resetTime,activate,%obj,false);
   }
}

function PushButton::activate(%this,%obj,%state)
{
   if (%state && !%obj.activated) {
      %obj.playThread(0,"push");
      %obj.setThreadDir(0,true);
      %obj.playAudio(0,ButtonClick);
      %obj.activated = true;
   }
   else
      if (!%state && %obj.activated) {
         %obj.setThreadDir(0,false);
         %obj.playAudio(0,ButtonClick);
         %obj.activated = false;
      }
}

