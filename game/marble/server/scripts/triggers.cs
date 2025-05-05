//-----------------------------------------------------------------------------
// Torque Game Engine
// 
// Copyright (c) 2001 GarageGames.Com
// Portions Copyright (c) 2001 by Sierra Online, Inc.
//-----------------------------------------------------------------------------

// Normally the trigger class would be sub-classed to support different
// functionality, but we're going to use the name of the trigger instead
// as an experiment.

//-----------------------------------------------------------------------------

datablock TriggerData(InBoundsTrigger)
{
   tickPeriodMS = 100;
   marbleGravityCheck = true;
};

function InBoundsTrigger::onLeaveTrigger(%this,%trigger,%obj)
{
   // Leaving an in bounds area.
   %obj.getDatablock().onOutOfBounds(%obj);
   if ($Marble::UseEmotives)
   {
      %obj.onEmotive("OOB");
   }
}


//-----------------------------------------------------------------------------

datablock TriggerData(OutOfBoundsTrigger)
{
   tickPeriodMS = 100;
};

function OutOfBoundsTrigger::onEnterTrigger(%this,%trigger,%obj)
{
   // Entering an out of bounds area
   %obj.getDatablock().onOutOfBounds(%obj);
}

//-----------------------------------------------------------------------------
datablock TriggerData(HelpTrigger)
{
   tickPeriodMS = 100;
};

function HelpTrigger::onEnterTrigger(%this,%trigger,%obj)
{
   // Leaving an in bounds area.
   commandToClient(%obj.client,'setHelpLine',%trigger.text,1);
}

datablock TriggerData(CheckPointTrigger)
{
   tickPeriodMS = 100;
};

function CheckPointTrigger::onEnterTrigger(%this, %trigger, %obj)
{
   // Entering a check point
   %obj.getDatablock().setCheckPoint(%obj, %trigger);
}

datablock TriggerData(FinishTrigger)
{
   tickPeriodMS = 100;
};

function FinishTrigger::onEnterTrigger(%this, %trigger, %obj)
{
   // Entering a finish point
   %obj.getDatablock().onFinishPoint(%obj);
}

//----------------- Sky Change Trigger
datablock TriggerData(SkyChangeTrigger)
{
   tickPeriodMS = 100;
};

function SkyChangeTrigger::onEnterTrigger(%this,%trigger,%obj)
{
   // Change Sky Material To Trigger Specified One
   echo("Changing Sky To: " @ %trigger.material);
   Sky.setSkyMaterial("marble/data/skies/" @ %trigger.material @ ".dml");

}

