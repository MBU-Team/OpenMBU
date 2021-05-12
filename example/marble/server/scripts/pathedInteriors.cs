//******************************************************************************
//* Moving platforms data blocks
//******************************************************************************

datablock SFXProfile(MovingBlockSustainSfx)
{
   filename = "~/data/sound/MovingBlockLoop.wav";
   description = AudioClosestLooping3d;
   preload = true;
};

datablock PathedInteriorData(PathedDefault)
{
   foo = bla;
};

datablock PathedInteriorData(PathedMovingBlock)
{
   sustainSound = MovingBlockSustainSfx;
};

function PathedInteriorData::onMissionReset(%data, %obj)
{
   %obj.setPathPosition(%obj.initialPosition);
   %obj.setTargetPosition(%obj.initialTargetPosition);
}

function PathedInterior::onTrigger(%this,%temp,%triggerMesg)
{
   // default just makes it loop
   if (%triggerMesg == "true")
      %triggerMesg = -2;

//   echo(%this.delayTargetTime);
   %this.setTargetPosition(%triggerMesg);
}

datablock TriggerData(TriggerGotoTarget)
{
   tickPeriodMS = 100;
};

function TriggerGotoTarget::onEnterTrigger(%this,%trigger,%obj)
{
   %grp = %trigger.getGroup();
   for(%i = 0; (%plat = %grp.getObject(%i)) != -1; %i++)
   {
      if(%plat.getClassName() $= "PathedInterior")
      {
         if(%trigger.delayTargetTime !$= "")
            %plat.delayTargetTime = %trigger.delayTargetTime;
         %plat.setTargetPosition(%trigger.targetTime);
      }
   }
}

function TriggerGotoTarget::onLeaveTrigger(%this, %trigger, %obj)
{

}

datablock TriggerData(TriggerGotoDelayTarget)
{
   tickPeriodMS = 100;
};

function TriggerGotoDelayTarget::onEnterTrigger(%this,%trigger,%obj)
{
   %grp = %trigger.getGroup();
   for(%i = 0; (%plat = %grp.getObject(%i)) != -1; %i++)
   {
      if(%plat.getClassName() $= "PathedInterior")
         %plat.setTargetPosition(%plat.delayTargetTime);
   }
   // Entering an out of bounds area
}

function TriggerGotoDelayTarget::onLeaveTrigger(%this, %trigger, %obj)
{

}


