//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------
// Following scripting (C) Tim Aste
// Please read the EULA and Documentation for more information on what this stuff does! *Seriously*


// Beam: Base Beam Portion
// ---------------
datablock ParticleData(BeamParticle)
{
   textureName          = "~/data/shapes/particles/orb";
   dragCoefficient      = 0.0;
   windCoefficient      = 0.0;
   gravityCoefficient   = 0.0;   // rises slowly
   inheritedVelFactor   = 1.00;
   constantAcceleration = 5;
   lifetimeMS           = 16000;
   lifetimeVarianceMS   = 0;
   useInvAlpha = true;
   spinRandomMin = 0.0;
   spinRandomMax = 0.0;

   colors[0]     = "0.4 0.5 1 1 ";
   colors[1]     = "0.9 0.95 1 0.5";
   colors[2]     = "1 1 1 0.0";

   sizes[0]      = 15.0;
   sizes[1]      = 10.0;
   sizes[2]      = 8.0;

   times[0]      = 0.0;
   times[1]      = 0.5;
   times[2]      = 1.0;
};

datablock ParticleEmitterData(BeamEmitter)
{
   ejectionPeriodMS = 50;
   periodVarianceMS = 0;

   ejectionVelocity = 0.57;
   velocityVariance = 0.00;

   thetaMin         = 0.0;
   thetaMax         = 0.0;  
   phiReferenceVelocity = 0.0;
   phiVariance = 1.0;

   particles = "BeamParticle";
};

datablock ParticleEmitterNodeData(BeamNode)
{
   timeMultiple = 1;
};

// Beam: Floating Rings (Unused)
// ----------------------------
datablock ParticleData(RingParticle)
{
   textureName          = "~/data/shapes/particles/ring";
   dragCoefficient      = 0.0;
   windCoefficient      = 0.0;
   gravityCoefficient   = -0.575;   // rises slowly
   inheritedVelFactor   = 0.00;
   lifetimeMS           = 9700;
   lifetimeVarianceMS   = 0;
   useInvAlpha = true;
   spinRandomMin = -90.0;
   spinRandomMax = 90.0;

   colors[0]     = "0.4 0.5 1 1 ";
   colors[1]     = "0.9 0.95 1 0.5";
   colors[2]     = "1 1 1 0.0";

   sizes[0]      = 15.0;
   sizes[1]      = 10.0;
   sizes[2]      = 8.0;

   times[0]      = 0.0;
   times[1]      = 0.5;
   times[2]      = 1.0;
};

datablock ParticleEmitterData(RingEmitter)
{
   ejectionPeriodMS = 200;
   periodVarianceMS = 0;

   ejectionVelocity = 0.07;
   velocityVariance = 0.00;

   thetaMin         = 1.0;
   thetaMax         = 100.0;  

   particles = "RingParticle";
};

datablock ParticleEmitterNodeData(RingNode)
{
   timeMultiple = 1;
};

// Beam: Flying Sparks from the Base Beam
// ----------------------------------------

datablock ParticleData(BeamSparkParticle)
{
   textureName          = "~/data/shapes/particles/burst";
   dragCoefficient      = 0.0;
   windCoefficient      = 0.0;
   gravityCoefficient   = -1.0;
   inheritedVelFactor   = 0.00;
   lifetimeMS           = 9000;
   lifetimeVarianceMS   = 0;
   useInvAlpha = false;
   spinRandomMin = -90.0;
   spinRandomMax = 90.0;

   colors[0]     = "0.4 0.5 1 1 ";
   colors[1]     = "0.9 0.95 1 0.5";
   colors[2]     = "1 1 1 0.0";
   
   sizes[0]      = 8.0;
   sizes[1]      = 2.0;
   sizes[2]      = 0.25;

   times[0]      = 0.0;
   times[1]      = 0.08;
   times[2]      = 0.5;
};

datablock ParticleEmitterData(BeamSparkEmitter)
{
   ejectionPeriodMS = 65;
   periodVarianceMS = 0;

   ejectionVelocity = 36;
   velocityVariance = 0.00;
   ejectionOffset = 0.0;

   thetaMin         = 1.0;
   thetaMax         = 35.0;  

   particles = "BeamSparkParticle";
};

datablock ParticleEmitterNodeData(BeamSparkNode)
{
   timeMultiple = 1;
};

// Beam: Beam Spiral
// ---------------
datablock ParticleData(FenceSparkParticle)
{
   textureName          = "~/data/shapes/particles/burst";
   dragCoefficient      = 0.0;
   windCoefficient      = 0.0;
   gravityCoefficient   = -1.0;
   inheritedVelFactor   = 0.00;
   lifetimeMS           = 15000;
   lifetimeVarianceMS   = 0;
   useInvAlpha = false;
   spinRandomMin = -90.0;
   spinRandomMax = 90.0;

   colors[0]     = "0 0 1 1 ";
   colors[1]     = "0.9 0.95 1 0.5";
   colors[2]     = "0 0 1 0.0";
   
   sizes[0]      = 1;
   sizes[1]      = 1.0;
   sizes[2]      = 1;

   times[0]      = 0.0;
   times[1]      = 0.08;
   times[2]      = 0.5;
};

datablock ParticleEmitterData(FenceSparkEmitter)
{
   ejectionPeriodMS = 3;
   periodVarianceMS = 0;

   ejectionVelocity = 0.1;
   velocityVariance = 0.00;
   ejectionOffset = 4.0;

   thetaMin         = 0.0;
   thetaMax         = 90.0;  
   phiReferenceVel = 360;
   phiVariance = 0;
   

   particles = "FenceSparkParticle";
};

datablock ParticleEmitterNodeData(FenceSparkNode)
{
   timeMultiple = 1;
};

// Barrier Wall: Barrier Beam
// ---------------
datablock ParticleData(FenceParticle)
{
   textureName          = "~/data/shapes/particles/burst";
   dragCoefficient      = 0.0;
   windCoefficient      = 0.0;
   gravityCoefficient   = 0.0;   
   inheritedVelFactor   = 0;
   constantAcceleration = 0;
   lifetimeMS           = 16800;
   lifetimeVarianceMS   = 0;
   useInvAlpha = false;
   spinRandomMin = -90.0;
   spinSpeed = 50.0;
   spinRandomMax = 90.0;

   colors[0]     = "0 0 0 1";
   colors[1]     = "0.500000 0.500000 1.000000 1.000000";
   colors[2]     = "0 0 0 1";

   sizes[0]      = 4;
   sizes[1]      = 5;
   sizes[2]      = 4;

   times[0]      = 0.0;
   times[1]      = 0.5;
   times[2]      = 1.0;
};

datablock ParticleEmitterData(FenceEmitter)
{
   ejectionPeriodMS = 75;
   periodVarianceMS = 0;

   ejectionVelocity = 5;
   velocityVariance = 0.00;

   thetaMin         = 0.0;
   thetaMax         = 0.0;  
   phiReferenceVelocity = 0.0;
   phiVariance = 0.0;

   particles = "FenceParticle";
};

datablock ParticleEmitterNodeData(FenceNode)
{
   timeMultiple = 1;
};

// Underwater: Dust cloud 
// ---------------
datablock ParticleData(WaterBeamParticle)
{
   textureName          = "~/data/shapes/particles/glare";
   dragCoefficient      = 0.0;
   windCoefficient      = 0.0;
   gravityCoefficient   = -0.1;
   inheritedVelFactor   = 0.00;
   lifetimeMS           = 15000.0;
   lifetimeVarianceMS   = 0;
   useInvAlpha = false;
   spinRandomMin = -90.0;
   spinRandomMax = 90.0;

   colors[0]     = "0 0 0 0.1 ";
   colors[1]     = "0.25 0.25 0.25 0.1";
   colors[2]     = "0 0 1 0.1";
   colors[3]     = "0 0 0 0.1";
   
   sizes[0]      = 30;
   sizes[1]      = 31.0;
   sizes[2]      = 30;
   sizes[3]      = 30;

   times[0]      = 0.0;
   times[1]      = 0.45;
   times[2]      = 0.8;
   times[3]      = 1.0;
};

datablock ParticleEmitterData(WaterBeamEmitter)
{
   ejectionPeriodMS = 50;
   periodVarianceMS = 0;

   ejectionVelocity = 5;
   velocityVariance = 0.00;
   ejectionOffset = 5.0;

   thetaMin         = 0.0;
   thetaMax         = 90.0;  
   phiReferenceVel = 360;
   phiVariance = 0;
   

   particles = "WaterBeamParticle";
};

datablock ParticleEmitterNodeData(WaterBeamNode)
{
   timeMultiple = 1;
};

// Waterfall: Landing Mist
// ---------------------------------------

datablock ParticleData(Falls1Particle)
{
   textureName          = "~/data/shapes/particles/orb";
   dragCoefficient      = 0.0;
   windCoefficient      = 0.0;
   gravityCoefficient   = -0.5;
   inheritedVelFactor   = 0.00;
   lifetimeMS           = 3000;
   lifetimeVarianceMS   = 0;
   useInvAlpha = false;
   spinRandomMin = -90.0;
   spinRandomMax = 90.0;

   colors[0]     = "1.000000 1.000000 1.000000 0.03";
   colors[1]     = "1.000000 1.000000 1.000000 0.05";
   colors[2]     = "1.000000 1.000000 1.000000 0.03";
   colors[3]     = "1.000000 1.000000 1.000000 0.01";  
   
   sizes[0]      = 25;
   sizes[1]      = 35;
   sizes[2]      = 45;
   sizes[3]      = 65;

   times[0]      = 0.0;
   times[1]      = 0.3;
   times[2]      = 0.5;
   times[3]      = 1.0;
};

datablock ParticleEmitterData(Falls1Emitter)
{
   ejectionPeriodMS = 35;
   periodVarianceMS = 0;

   ejectionVelocity = 20;
   velocityVariance = 0.00;
   ejectionOffset = 3.0;

   thetaMin         = 1.0;
   thetaMax         = 35.0;  

   particles = "Falls1Particle";
};

datablock ParticleEmitterNodeData(Falls1Node)
{
   timeMultiple = 1;
};

// WaterFall: Thick Water Portion
// ---------------
datablock ParticleData(Falls2Particle)
{
   textureName          = "~/data/shapes/particles/splash2";
   dragCoefficient      = 0.0;
   windCoefficient      = 0.0;
   gravityCoefficient   = 2.0;   // rises slowly
   inheritedVelFactor   = 0.00;
   constantAcceleration = 0;
   lifetimeMS           = 5000;
   lifetimeVarianceMS   = 0;
   useInvAlpha = true;
   spinRandomMin = 0.0;
   spinRandomMax = 0.0;

   colors[0]     = "0.600000 0.600000 1.000000 0.300000";
   colors[1]     = "0.600000 0.600000 1.000000 0.300000";
   colors[2]     = "0.600000 0.600000 1.000000 0.300000";
   colors[3]     = "0.600000 0.600000 1.000000 0.300000";

   sizes[0]      = 10.0;
   sizes[1]      = 15.0;
   sizes[2]      = 23.0;
   sizes[3]      = 30.0;

   times[0]      = 0.0;
   times[1]      = 0.5;
   times[2]      = 0.6;
   times[3]      = 1.0;
};

datablock ParticleEmitterData(Falls2Emitter)
{
   ejectionPeriodMS = 75;
   periodVarianceMS = 0;

   ejectionVelocity = 10.00;
   velocityVariance = 0.00;

   thetaMin         = 0.0;
   thetaMax         = 3.0;  
   phiReferenceVel  = 360.0;
   phiVariance = 1.0;

   particles = "Falls2Particle";
};

datablock ParticleEmitterNodeData(Falls2Node)
{
   timeMultiple = 1;
};

// Waterfall: Thick Falling Water Portion
// ----------------------------------------

datablock ParticleData(Falls3Particle)
{
   textureName          = "~/data/shapes/particles/splash3";
   dragCoefficient      = 0.0;
   windCoefficient      = 0.0;
   gravityCoefficient   = 1.0;
   inheritedVelFactor   = 0.00;
   lifetimeMS           = 9000;
   lifetimeVarianceMS   = 0;
   useInvAlpha = false;
   spinRandomMin = -90.0;
   spinRandomMax = 90.0;

   colors[0]     = "0.400000 0.400000 0.600000 0.300000";
   colors[1]     = "0.400000 0.400000 0.600000 0.500000";
   colors[2]     = "0.400000 0.400000 0.600000 0.700000";
   colors[3]     = "0.400000 0.400000 0.600000 1.000000";
   
   sizes[0]      = 35;
   sizes[1]      = 45;
   sizes[2]      = 50;
   sizes[3]      = 50;

   times[0]      = 0.0;
   times[1]      = 0.4;
   times[2]      = 0.65;
   times[3]      = 1.0;
};

datablock ParticleEmitterData(Falls3Emitter)
{
   ejectionPeriodMS = 175;
   periodVarianceMS = 0;

   ejectionVelocity = 5;
   velocityVariance = 0.00;
   ejectionOffset = 0.0;

   thetaMin         = 1.0;
   thetaMax         = 35.0;  

   particles = "Falls3Particle";
};

datablock ParticleEmitterNodeData(Falls3Node)
{
   timeMultiple = 1;
};

// Waterfall: Fast richocheting water drops
// ----------------------------------------

datablock ParticleData(Falls4Particle)
{
   textureName          = "~/data/shapes/particles/fleck";
   dragCoefficient      = 0.0;
   windCoefficient      = 0.0;
   gravityCoefficient   = 1.25;
   inheritedVelFactor   = 0.00;
   lifetimeMS           = 3000;
   lifetimeVarianceMS   = 0;
   useInvAlpha = false;
   spinRandomMin = -90.0;
   spinRandomMax = 90.0;

   colors[0]     = "0.200000 0.400000 0.600000 0.300000";
   colors[1]     = "0.600000 0.600000 1.000000 0.600000";
   colors[2]     = "0.600000 0.600000 1.000000 0.600000";
  
   
   sizes[0]      = 3.75;
   sizes[1]      = 4.0;
   sizes[2]      = 4.25;

   times[0]      = 0.0;
   times[1]      = 0.08;
   times[2]      = 0.5;
};

datablock ParticleEmitterData(Falls4Emitter)
{
   ejectionPeriodMS = 15;
   periodVarianceMS = 0;

   ejectionVelocity = 15;
   velocityVariance = 0.00;
   ejectionOffset = 0.0;

   thetaMin         = 1.0;
   thetaMax         = 35.0;  

   particles = "Falls4Particle";
};

datablock ParticleEmitterNodeData(Falls4Node)
{
   timeMultiple = 1;
};

// Underwater: Sunlight shining through water beams
// ----------------------------------------

datablock ParticleData(WaterBeam2Particle)
{
   textureName          = "~/data/shapes/particles/waterbeam";
   dragCoefficient      = 0.0;
   windCoefficient      = 0.0;
   gravityCoefficient   = 0.0;   // rises slowly
   inheritedVelFactor   = 0.00;
   constantAcceleration = 0;
   lifetimeMS           = 5000;
   lifetimeVarianceMS   = 0;
   useInvAlpha = true;
   spinRandomMin = 0.0;
   spinRandomMax = 0.0;


   colors[0]     = "0.600000 0.600000 1.000000 0.100000";
   colors[1]     = "0.600000 0.600000 1.000000 0.300000";
   colors[2]     = "0.600000 0.600000 1.000000 0.300000";
   colors[3]     = "0.600000 0.600000 1.000000 0.100000";

   sizes[0]      = 33.0;
   sizes[1]      = 45.0;
   sizes[2]      = 45.0;
   sizes[3]      = 33.0;

   times[0]      = 0.0;
   times[1]      = 0.25;
   times[2]      = 0.75;
   times[3]      = 1.0;
};

datablock ParticleEmitterData(WaterBeam2Emitter)
{
   ejectionPeriodMS = 75;
   periodVarianceMS = 25;

   ejectionVelocity = 35.00;
   velocityVariance = 0.00;

   thetaMin         = 0.0;
   thetaMax         = 0.0;  
   phiReferenceVel  = 360.0;
   phiVariance = 1.0;

   particles = "WaterBeam2Particle";
};

datablock ParticleEmitterNodeData(WaterBeam2Node)
{
   timeMultiple = 1;
};

// Black Forest: Red Fire Flies of the Devil!!
// ----------------------------------------

datablock ParticleData(DarkFireFliesParticle)
{
   textureName          = "~/data/shapes/particles/firefly";
   dragCoefficient      = 0.0;
   windCoefficient      = 5.0;
   gravityCoefficient   = 0.0;
   inheritedVelFactor   = 0.00;
   lifetimeMS           = 8000;
   lifetimeVarianceMS   = 0;
   useInvAlpha = false;
   spinRandomMin = -90.0;
   spinRandomMax = 90.0;

   colors[0]     = "0 0 0 0 ";
   colors[1]     = "1 0 0 1";
   colors[2]     = "1 1 0 1";
   colors[3]     = "0 0 0 0";
   
   sizes[0]      = 0.0;
   sizes[1]      = 0.15;
   sizes[2]      = 0.2;
   sizes[3]      = 0.0;
   
   times[0]      = 0.0;
   times[1]      = 0.1;
   times[2]      = 0.5;
   times[3]      = 1.0;
   
};

datablock ParticleEmitterData(DarkFireFliesEmitter)
{
   ejectionPeriodMS = 300;
   periodVarianceMS = 0;

   ejectionVelocity = 3;
   velocityVariance = 1.00;
   ejectionOffset = 1.0;

   thetaMin         = 75.0;
   thetaMax         = 90.0;
   
   phiReferenceVel  = 360.00;
   phiVariance      = 360.00;

   particles = "DarkFireFliesParticle";
};

datablock ParticleEmitterNodeData(DarkFireFliesNode)
{
   timeMultiple = 1;
};

// Warzone: Fire Effect One (Small & Light Background Layer)
// ----------------------------------------

datablock ParticleData(FiyahParticle)
{
   textureName          = "~/data/shapes/particles/smoke";
   dragCoefficient      = 0.0;
   windCoefficient      = 0.0;
   gravityCoefficient   = -0.075;   // rises slowly
   inheritedVelFactor   = 0.00;
   lifetimeMS           = 5000;
   lifetimeVarianceMS   = 0;
   useInvAlpha = false;
   spinRandomMin = -90.0;
   spinRandomMax = 90.0;

   colors[0]     = "0.400000 0.300000 0.000000 0.100000";
   colors[1]     = "0.200000 0.050000 0.000000 0.200000";
   colors[2]     = "0.100000 0.000000 0.000000 0.000000";

   sizes[0]      = 5.9;
   sizes[1]      = 6.75;
   sizes[2]      = 8.3;

   times[0]      = 0.0;
   times[1]      = 0.5;
   times[2]      = 1.0;
};

datablock ParticleEmitterData(FiyahEmitter)
{
   ejectionPeriodMS = 200;
   periodVarianceMS = 0;

   ejectionVelocity = 0.07;
   velocityVariance = 0.00;

   thetaMin         = 1.0;
   thetaMax         = 100.0;  

   particles = "FiyahParticle";
};

datablock ParticleEmitterNodeData(FiyahNode)
{
   timeMultiple = 1;
};

// Warzone: Fire Effect Two (Tiny)
// ----------------------------------------

datablock ParticleData(FiyahTwoParticle)
{
   textureName          = "~/data/shapes/particles/smoke";
   dragCoefficient     = 0.0;
   gravityCoefficient   = -0.05;   // rises slowly
   inheritedVelFactor   = 0.00;
   lifetimeMS           = 2000;
   lifetimeVarianceMS   = 0;
   useInvAlpha = false;
   spinRandomMin = -90.0;
   spinRandomMax = 90.0;

   colors[0]     = "0.6 0.2 0.0 1.0";
   colors[1]     = "0.6 0.2 0.0 1.0";
   colors[2]     = "0.2 0.0 0.0 0.0";

   sizes[0]      = 0.6;
   sizes[1]      = 0.5;
   sizes[2]      = 0.1;

   times[0]      = 0.0;
   times[1]      = 0.5;
   times[2]      = 1.0;
};

datablock ParticleEmitterData(FiyahTwoEmitter)
{
   ejectionPeriodMS = 200;
   periodVarianceMS = 0;

   ejectionVelocity = 0.07;
   velocityVariance = 0.00;

   thetaMin         = 1.0;
   thetaMax         = 100.0;  

   particles = "FiyahTwoParticle";
};

datablock ParticleEmitterNodeData(FiyahTwoNode)
{
   timeMultiple = 1;
};

// Warzone: Fire Effect Three (Normal Size)
// ----------------------------------------

datablock ParticleData(FireBigParticle)
{
   textureName          = "~/data/shapes/particles/smoke";
   dragCoefficient     = 0.0;
   gravityCoefficient   = -0.15;   // rises slowly
   inheritedVelFactor   = 0.00;
   lifetimeMS           = 2000;
   lifetimeVarianceMS   = 0;
   useInvAlpha = false;
   spinRandomMin = -90.0;
   spinRandomMax = 90.0;

   colors[0]     = "0.6 0.2 0.0 1.0";
   colors[1]     = "0.6 0.2 0.0 1.0";
   colors[2]     = "0.2 0.0 0.0 0.0";

   sizes[0]      = 2.0;
   sizes[1]      = 1.6;
   sizes[2]      = 0.75;

   times[0]      = 0.0;
   times[1]      = 0.5;
   times[2]      = 1.0;
};

datablock ParticleEmitterData(FireBigEmitter)
{
   ejectionPeriodMS = 200;
   periodVarianceMS = 0;

   ejectionVelocity = 0.07;
   velocityVariance = 0.00;

   thetaMin         = 2.0;
   thetaMax         = 180.0;  

   particles = "FireBigParticle";
};

datablock ParticleEmitterNodeData(FireBigNode)
{
   timeMultiple = 1;
};

// Warzone: Steam
// ----------------------------------------

datablock ParticleData(SteamParticle)
{
   textureName          = "~/data/shapes/particles/smoke";
   dragCoefficient     = 4.0;
   windCoefficient     = 0.0;
   gravityCoefficient   = -1.05;   // rises slowly
   inheritedVelFactor   = 0.00;
   lifetimeMS           = 6000;
   lifetimeVarianceMS   = 750;
   useInvAlpha = false;
   spinRandomMin = 0.0;
   spinRandomMax = 150.0;
   spinSpeed     = 15.0;

   colors[0]     = "0.38 0.38 0.38 0.1";
   colors[1]     = "0.34 0.34 0.34 0.1";
   colors[2]     = "0.30 0.30 0.30 0.1";

   sizes[0]      = 5;
   sizes[1]      = 10;
   sizes[2]      = 15;

   times[0]      = 0.0;
   times[1]      = 0.5;
   times[2]      = 1.0;
};

datablock ParticleEmitterData(SteamEmitter)
{
   ejectionPeriodMS = 200;
   periodVarianceMS = 100;

   ejectionVelocity = 1.50;
   velocityVariance = 0.50;

   thetaMin         = 0.0;
   thetaMax         = 0.0;  

   particles = "SteamParticle";
};

datablock ParticleEmitterNodeData(SteamNode)
{
   timeMultiple = 1;
};

// Warzone: ????????????????
// ----------------------------------------

datablock ParticleData(TriggeredFireParticle)
{
   textureName          = "~/data/shapes/particles/smoke";
   dragCoefficient     = 0.0;
   gravityCoefficient   = -0.075;   // rises slowly
   inheritedVelFactor   = 0.00;
   lifetimeMS           = 2000;
   lifetimeVarianceMS   = 0;
   useInvAlpha = false;
   spinRandomMin = -90.0;
   spinRandomMax = 90.0;

   colors[0]     = "0.0 0.0 0.0 1.0";
   colors[1]     = "0.0 0.0 0.0 1.0";
   colors[2]     = "0.0 0.0 0.0 0.0";

   sizes[0]      = 5.9;
   sizes[1]      = 5.75;
   sizes[2]      = 5.3;

   times[0]      = 0.0;
   times[1]      = 0.5;
   times[2]      = 1.0;
};

datablock ParticleEmitterData(TriggeredFireEmitter)
{
   ejectionPeriodMS = 500;
   periodVarianceMS = 0;

   ejectionVelocity = 0.07;
   velocityVariance = 0.00;

   thetaMin         = 1.0;
   thetaMax         = 100.0;  

   particles = "TriggeredFireParticle";
};

datablock ParticleEmitterNodeData(TriggeredFireNode)
{
   timeMultiple = 1;
};

// Warzone: Huge Fire Inferno
// ----------------------------------------

datablock ParticleData(HugeFireParticle)
{
   textureName          = "~/data/shapes/particles/smoke";
   dragCoefficient      = 0.0;
   windCoefficient      = 3.0;
   gravityCoefficient   = -0.375;   // rises slowly
   inheritedVelFactor   = 0.00;
   lifetimeMS           = 8700;
   lifetimeVarianceMS   = 1000;
   useInvAlpha = false;
   spinRandomMin = -90.0;
   spinRandomMax = 90.0;

   colors[0]     = "0.6 0.2 0.0 0.4";
   colors[1]     = "0.4 0.1 0.0 0.2";
   colors[2]     = "0.2 0.0 0.0 0.0";

   sizes[0]      = 45.9;
   sizes[1]      = 30.75;
   sizes[2]      = 20.3;

   times[0]      = 0.0;
   times[1]      = 0.5;
   times[2]      = 1.0;
};

datablock ParticleEmitterData(HugeFireEmitter)
{
   ejectionPeriodMS = 200;
   periodVarianceMS = 0;

   ejectionVelocity = 0.07;
   velocityVariance = 0.00;

   thetaMin         = 1.0;
   thetaMax         = 100.0;  

   particles = "HugeFireParticle";
};

datablock ParticleEmitterNodeData(HugeFireNode)
{
   timeMultiple = 1;
};

// Warzone: Black Smoke Effect (Huge)
// ----------------------------------------

datablock ParticleData(HugeBlackParticle)
{
   textureName          = "~/data/shapes/particles/smoke2";
   dragCoefficient      = 0.0;
   windCoefficient      = 15.0;
   gravityCoefficient   = -1.375;   // rises slowly
   inheritedVelFactor   = 0.00;
   lifetimeMS           = 5700;
   lifetimeVarianceMS   = 1000;
   useInvAlpha = true;
   spinRandomMin = -90.0;
   spinRandomMax = 90.0;

   colors[0]     = "1 1 1 0.4";
   colors[1]     = "1 1 1 0.2";
   colors[2]     = "1 1 1 0.0";

   sizes[0]      = 20.3;
   sizes[1]      = 30.75;
   sizes[2]      = 45.0;

   times[0]      = 0.0;
   times[1]      = 0.5;
   times[2]      = 1.0;
};

datablock ParticleEmitterData(HugeBlackEmitter)
{
   ejectionPeriodMS = 200;
   periodVarianceMS = 0;

   ejectionVelocity = 0.07;
   velocityVariance = 0.00;

   thetaMin         = 1.0;
   thetaMax         = 100.0;  

   particles = "HugeBlackParticle";
};

datablock ParticleEmitterNodeData(HugeBlackNode)
{
   timeMultiple = 1;
};


// Warzone: Small Fire Sparks
// ----------------------------------------

datablock ParticleData(FireSparkParticle)
{
   textureName          = "~/data/shapes/particles/spark";
   dragCoefficient      = 0.0;
   windCoefficient      = 0.0;
   gravityCoefficient   = -0.05;   // rises slowly
   inheritedVelFactor   = 0.00;
   lifetimeMS           = 5000;
   lifetimeVarianceMS   = 0;
   useInvAlpha = false;
   spinRandomMin = -90.0;
   spinRandomMax = 90.0;

   colors[0]     = "1.000000 0.800000 0.000000 0.800000";
   colors[1]     = "1.000000 0.700000 0.000000 0.800000";
   colors[2]     = "1.000000 0.000000 0.000000 0.200000";

   sizes[0]      = 0.05;
   sizes[1]      = 0.1;
   sizes[2]      = 0.05;

   times[0]      = 0.0;
   times[1]      = 0.5;
   times[2]      = 1.0;
};

datablock ParticleEmitterData(FireSparkEmitter)
{
   ejectionPeriodMS = 100;
   periodVarianceMS = 0;

   ejectionVelocity = 0.75;
   velocityVariance = 0.00;
   ejectionOffset = 2.0;

   thetaMin         = 1.0;
   thetaMax         = 100.0;  

   particles = "FireSparkParticle";
};

datablock ParticleEmitterNodeData(FireSparkNode)
{
   timeMultiple = 1;
};


// Warzone: Alternative Fire Effect
// ----------------------------------------

datablock ParticleData(FireParticle)
{
   textureName          = "~/data/shapes/particles/smoke";
   dragCoefficient      = 0.0;
   windCoefficient      = 0.0;
   gravityCoefficient   = -0.05;   // rises slowly
   inheritedVelFactor   = 0.00;
   lifetimeMS           = 5000;
   lifetimeVarianceMS   = 1000;
   useInvAlpha = false;
   spinRandomMin = -90.0;
   spinRandomMax = 90.0;
   spinSpeed = 1.0;

   colors[0]     = "0.2 0.2 0.0 0.2";
   colors[1]     = "0.6 0.2 0.0 0.2";
   colors[2]     = "0.4 0.0 0.0 0.1";
   colors[3]     = "0.1 0.04 0.0 0.3";

   sizes[0]      = 0.5;
   sizes[1]      = 4.0;
   sizes[2]      = 5.0;
   sizes[3]      = 6.0;

   times[0]      = 0.0;
   times[1]      = 0.1;
   times[2]      = 0.2;
   times[3]      = 0.3;
};

datablock ParticleEmitterData(FireEmitter)
{
   ejectionPeriodMS = 50;
   periodVarianceMS = 0;

   ejectionVelocity = 0.55;
   velocityVariance = 0.00;
   ejectionOffset = 1.0;
   

   thetaMin         = 1.0;
   thetaMax         = 100.0;  

   particles = "FireParticle";
};

datablock ParticleEmitterNodeData(FireNode)
{
   timeMultiple = 1;
};

// Warzone: Grey Smoke
// ----------------------------------------

datablock ParticleData(FlameSmoke)
{
   textureName          = "~/data/shapes/particles/smoke";
   dragCoefficient     = 0.0;
   gravityCoefficient   = -0.1;   // rises slowly
   windCoefficient      = 0;
   inheritedVelFactor   = 0.00;
   lifetimeMS           = 6000;
   lifetimeVarianceMS   = 250;
   useInvAlpha = false;
   spinRandomMin = -30.0;
   spinRandomMax = 30.0;

   colors[0]     = "0.2 0.2 0.2 0.25";
   colors[1]     = "0.1 0.1 0.1 0.25";
   colors[2]     = "0.05 0.05 0.05 0.25";

   sizes[0]      = 5.25;
   sizes[1]      = 5.5;
   sizes[2]      = 6.0;

   times[0]      = 0.0;
   times[1]      = 0.5;
   times[2]      = 1;
};

datablock ParticleEmitterData(FlameSmokeEmitter)
{
   ejectionPeriodMS = 40;
   periodVarianceMS = 5;

   ejectionVelocity = 0.25;
   velocityVariance = 0.10;

   thetaMin         = 0.0;
   thetaMax         = 90.0; 

   particles = FlameSmoke;
};

datablock ParticleEmitterNodeData(FlameSmokeNode)
{
   timeMultiple = 1;
};

// Warzone: Huge Grey Smoke
// ----------------------------------------

datablock ParticleData(HugeFlameSmoke)
{
   textureName          = "~/data/shapes/particles/smoke";
   dragCoefficient     = 0.0;
   gravityCoefficient   = -0.8;   // rises slowly
   windCoefficient      = 6;
   inheritedVelFactor   = 0.00;
   lifetimeMS           = 6000;
   lifetimeVarianceMS   = 250;
   useInvAlpha = false;
   spinRandomMin = -30.0;
   spinRandomMax = 30.0;

   colors[0]     = "0.2 0.2 0.2 0.25";
   colors[1]     = "0.1 0.1 0.1 0.25";
   colors[2]     = "0.05 0.05 0.05 0.25";

   sizes[0]      = 25.25;
   sizes[1]      = 35.5;
   sizes[2]      = 46.0;

   times[0]      = 0.0;
   times[1]      = 0.5;
   times[2]      = 1;
};

datablock ParticleEmitterData(HugeFlameSmokeEmitter)
{
   ejectionPeriodMS = 40;
   periodVarianceMS = 5;

   ejectionVelocity = 0.25;
   velocityVariance = 0.10;

   thetaMin         = 0.0;
   thetaMax         = 90.0; 

   particles = HugeFlameSmoke;
};

datablock ParticleEmitterNodeData(HugeFlameSmokeNode)
{
   timeMultiple = 1;
};

// Warzone: Water Dripping
// ----------------------------------------

datablock ParticleData(Drip)
{
   textureName          = "~/data/shapes/particles/drip";
   dragCoefficient     = 0.0;
   gravityCoefficient   = 0.6; // rises slowly
   windCoefficient      = 0;
   inheritedVelFactor   = 0.00;
   lifetimeMS           = 1000;
   lifetimeVarianceMS   = 250;
   useInvAlpha = false;
   spinRandomMin = 0.0;
   spinRandomMax = 0.0;

   colors[0]     = "0.5 0.5 0.5 0.4";
   colors[1]     = "0.3 0.3 0.3 0.2";
   colors[2]     = "0.1 0.1 0.1 0.1";

   sizes[0]      = 1.0;
   sizes[1]      = 0.5;
   sizes[2]      = 0.25;

   times[0]      = 0;
   times[1]      = 1;
   times[2]      = 1.25;
};

datablock ParticleEmitterData(DripEmitter)
{
   ejectionPeriodMS = 40;
   periodVarianceMS = 5;

   ejectionVelocity = 0.25;
   velocityVariance = 0.10;

   thetaMin         = 0.0;
   thetaMax         = 0.0;

   particles = Drip;
};

datablock ParticleEmitterNodeData(DripEmitterNode)
{
   timeMultiple = 1;
};

// Warzone: Dust Trail
// ----------------------------------------

datablock ParticleData(DustTrail)
{
   textureName          = "~/data/shapes/particles/smoke";
   dragCoefficient     = 0.0;
   gravityCoefficient   = 0.15;   // rises slowly
   windCoefficient      = 35;
   inheritedVelFactor   = 0.00;
   lifetimeMS           = 6000;
   lifetimeVarianceMS   = 250;
   useInvAlpha = false;
   spinRandomMin = -30.0;
   spinRandomMax = 30.0;

   colors[0]     = "0.450000 0.250000 0.250000 0.250000";
   colors[1]     = "0.450000 0.350000 0.250000 0.150000";
   colors[2]     = "0.750000 0.550000 0.250000 0.150000";
   
   sizes[0]      = 1;
   sizes[1]      = 15.5;
   sizes[2]      = 35.0;

   times[0]      = 0.0;
   times[1]      = 0.5;
   times[2]      = 1;
};

datablock ParticleEmitterData(DustTrailEmitter)
{
   ejectionPeriodMS = 50;
   periodVarianceMS = 5;

   ejectionVelocity = 0.00;
   velocityVariance = 0.0;

   thetaMin         = 0.0;
   thetaMax         = 90.0; 

   particles = DustTrail;
};

datablock ParticleEmitterNodeData(DustTrailNode)
{
   timeMultiple = 1;
};