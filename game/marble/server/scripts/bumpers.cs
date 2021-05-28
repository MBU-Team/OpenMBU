//-----------------------------------------------------------------------------
// Torque Game Engine
//
// Copyright (c) 2001 GarageGames.Com
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------

datablock SFXProfile(BumperDing)
{
   filename    = "~/data/sound/bumperDing1.wav";
   description = AudioDefault3d;
   preload = true;
};

//-----------------------------------------------------------------------------

datablock StaticShapeData(RoundBumper)
{
   category = "Bumpers";
   className = "Bumper";
   shapeFile = "~/data/shapes/bumpers/pball_round.dts";
   scopeAlways = true;
   sound = BumperDing;
};

function RoundBumper::onAdd( %this, %obj )
{
   %obj.playThread( 0, "idle" );
}

function RoundBumper::onEndSequence( %this, %obj, %slot )
{
   // This means the activate sequence is done, so put back to idle
   %obj.stopThread( 0 );
   %obj.playThread( 0, "idle" );
}

function RoundBumper::onCollision( %this, %obj, %col ,%vec, %vecLen, %material )
{
   // Currently activates when any object hits it.
   //if( %material $= "BumperMaterial" ) 
   //{
      %obj.stopThread( 0 );
      %obj.playThread( 0, "activate" );
      %obj.playAudio( 0, %this.sound );
   //}
}
