//-----------------------------------------------------------------------------
// Torque Game Engine
//
// Copyright (c) 2001 GarageGames.Com
// Portions Copyright (c) 2001 by Sierra Online, Inc.
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------

datablock SFXProfile(TrapDoorOpenSfx)
{
   filename    = "~/data/sound/TrapDoorOpen.wav";
   description = Quieter3D;
   preload = true;
};


datablock StaticShapeData(TrapDoor)
{
   className = "Trap";
   category = "Hazards";
   shapeFile = "~/data/shapes/hazards/trapdoor.dts";
   resetTime = 5000;
   scopeAlways = true;
};

function TrapDoor::onAdd(%this, %obj)
{
   %obj.open = false;
   %obj.timeout = 200;
   if (%obj.resetTime $= "")
      %obj.resetTime = "Default";
}

function TrapDoor::onCollision(%this,%obj,%col)
{
   if (!%obj.open) {
      // pause before opening - give marble a chance to get off
      %this.open(%obj);
      %obj.open = true;

      // Schedule the button reset
      %resetTime = (%obj.resetTime $= "Default")? %this.resetTime: %obj.resetTime;
      if (%resetTime)
         %this.schedule(%resetTime,close,%obj);
   }
}

function Trapdoor::open(%this, %obj)
{
   %obj.playThread(0,"fall",1,%obj.timeout);
   %obj.playAudio(0,TrapDoorOpenSfx);
   %obj.open = true;
}

function Trapdoor::close(%this, %obj)
{
   %obj.setThreadDir(0,false);
   %obj.playAudio(0,TrapDoorOpenSfx);
   %obj.open = false;
}


//-----------------------------------------------------------------------------
datablock SFXProfile(DuctFanSfx)
{
   filename    = "~/data/sound/Fan_loop.wav";
   description = QuieterAudioClosestLooping3d;
   preload = true;
};


datablock StaticShapeData(DuctFan)
{
   className = "Fan";
   category = "Hazards";
   shapeFile = "~/data/shapes/hazards/ductfan.dts";
   scopeAlways = true;

   forceType[0] = Cone;       // Force type {Spherical, Field, Cone}
   forceNode[0] = 0;          // Shape node transform
   forceStrength[0] = 36;     // Force to apply
   forceRadius[0] = 10;       // Max radius
   forceArc[0] = 0.7;         // Cos angle

   powerOn = true;         // Default state
};

function DuctFan::onRemove(%this,%obj)
{
   %obj.stopAudio(0);
}

function DuctFan::onAdd(%this,%obj)
{
   error( "DUCT FAN ADDED: " @ %obj );
   if (%this.powerOn)
   {
      %obj.playAudio(0, DuctFanSfx);
      %obj.playThread(0,"spin");
   }
   %obj.setPoweredState(%this.powerOn);
}

function DuctFan::onTrigger(%this,%obj,%mesg)
{
   if (%mesg)
   {
      %obj.playAudio(0, DuctFanSfx);
      %obj.playThread(0,"spin");
   }
   else
   {
      %obj.stopThread(0);
      %obj.stopAudio(0);
   }
   %obj.setPoweredState(%mesg);
}

// Fan Particle Effect
// ----------------------------------------

datablock ParticleData(FanAirParticle)
{
   textureName          = "~/data/particles/fleck";
   dragCoefficient      = 0.0;
   gravityCoefficient   = 0.0;
   inheritedVelFactor   = 0.0;
   constantAcceleration = 20.0;
   lifetimeMS           = 2000;
   lifetimeVarianceMS   = 0;
   useInvAlpha          = false;
   spinRandomMin        = 0.0;
   spinRandomMax        = 360.0;
   spinSpeed            = 550.0;
   windCoefficient      = 0.0;

   colors[0]     = "0.2 0.2 0.2 0.0";
   colors[1]     = "0.8 0.8 0.8 0.5";
   colors[2]     = "1.0 1.0 1.0 1.0";
   colors[3]     = "0.2 0.2 0.2 0.0";

   sizes[0]      = 0.0;
   sizes[1]      = 0.25;
   sizes[2]      = 0.5;
   sizes[3]      = 0.0;

   times[0]      = 0.0;
   times[1]      = 0.2;
   times[2]      = 0.5;
   times[3]      = 1.0;
};

datablock ParticleEmitterData(FanAirEmitter)
{
   ejectionPeriodMS = 200;
   periodVarianceMS = 0;

   ejectionVelocity = 0.25;
   velocityVariance = 0.17;

   thetaMin         = 75.0;
   thetaMax         = 90.0;  
   phiReferenceVel  = 0.0;
   phiVariance      = 25.0;

   particles = "FanAirParticle";
};

datablock ParticleEmitterNodeData(FanAirNode)
{
   timeMultiple = 1;
};