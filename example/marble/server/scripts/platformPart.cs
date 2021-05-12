//******************************************************************************
//* Conveyor data blocks
//******************************************************************************

datablock SFXProfile(PlatformLowering)
{
   filename    = "~/data/sound/PlatformLowering.wav";
   description = AudioDefaultLooping3d;
   preload = true;
};

datablock PlatformPartData(DefaultPlatformPart)
{
   platformSpeed = 1;

   shapeFile = "~/data/shapes/platform-lower.dts";
};

//******************************************************************************
function DefaultPlatformPart::onAdd(%this, %obj)
{
   echo("New Platform (lowering): " @ %obj);

   %obj.playThread(0,"Lower",1);
   %obj.stopThread(0);
}

//function DefaultPlatformPart::onCollision(%this, %obj, %colObj)
//{
//   echo("Platform (lowering) collision: " @ %obj);
//}


function DefaultPlatformPart::onButtonChange(%this, %obj, %state)
{
   echo(" Platform (lowering) got a button change message: " @ %state);

   if (%obj.AnimState == 0)
   {
      %obj.AnimState = 1;
      %platformSpeed = %obj.getDataBlock().platformSpeed;

      if (%state == 1)
      {
         %obj.playThread(0,"Lower",%platformSpeed);
         %obj.setThreadDir(0,true);

         %obj.playAudio(0,PlatformLowering);
      }
   }

}


function DefaultPlatformPart::onEndSequence(%this, %obj, %slot)
{
   %obj.stopAudio(0);
}


