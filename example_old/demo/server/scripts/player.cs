//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

// Load dts shapes and merge animations
exec("~/data/shapes/spaceOrc/player.cs");

// Timeouts for corpse deletion.
$CorpseTimeoutValue = 22 * 1000;

// Damage Rate for entering Liquid
$DamageLava       = 0.01;
$DamageHotLava    = 0.01;
$DamageCrustyLava = 0.01;

//
$PlayerDeathAnim::TorsoFrontFallForward = 1;
$PlayerDeathAnim::TorsoFrontFallBack = 2;
$PlayerDeathAnim::TorsoBackFallForward = 3;
$PlayerDeathAnim::TorsoLeftSpinDeath = 4;
$PlayerDeathAnim::TorsoRightSpinDeath = 5;
$PlayerDeathAnim::LegsLeftGimp = 6;
$PlayerDeathAnim::LegsRightGimp = 7;
$PlayerDeathAnim::TorsoBackFallForward = 8;
$PlayerDeathAnim::HeadFrontDirect = 9;
$PlayerDeathAnim::HeadBackFallForward = 10;
$PlayerDeathAnim::ExplosionBlowBack = 11;


//----------------------------------------------------------------------------
// Player Audio Profiles
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------

datablock AudioProfile(DeathCrySound)
{
   fileName = "~/data/sound/orc_death.ogg";
   description = AudioClose3d;
   preload = true;
};

datablock AudioProfile(PainCrySound)
{
   fileName = "~/data/sound/orc_pain.ogg";
   description = AudioClose3d;
   preload = true;
};


//----------------------------------------------------------------------------

datablock AudioProfile(FootLightSoftSound)
{
   filename    = "~/data/sound/footstep_soft.ogg";
   description = AudioClosest3d;
   preload = true;
};

datablock AudioProfile(FootLightHardSound)
{
   filename    = "~/data/sound/footstep_hard.ogg";
   description = AudioClose3d;
   preload = true;
};

datablock AudioProfile(FootLightMetalSound)
{
   filename    = "~/data/sound/footstep_hard.ogg";
   description = AudioClose3d;
   preload = true;
};

datablock AudioProfile(FootLightSnowSound)
{
   filename    = "~/data/sound/footstep_soft.ogg";
   description = AudioClosest3d;
   preload = true;
};

datablock AudioProfile(FootLightShallowSplashSound)
{
   filename    = "~/data/sound/footstep_water.ogg";
   description = AudioClose3d;
   preload = true;
};

datablock AudioProfile(FootLightWadingSound)
{
   filename    = "~/data/sound/footstep_water.ogg";
   description = AudioClose3d;
   preload = true;
};

datablock AudioProfile(FootLightUnderwaterSound)
{
   filename    = "~/data/sound/footstep_water.ogg";
   description = AudioClosest3d;
   preload = true;
};

datablock AudioProfile(FootLightBubblesSound)
{
   filename    = "~/data/sound/replaceme.ogg";
   description = AudioClose3d;
   preload = true;
};

datablock AudioProfile(ArmorMoveBubblesSound)
{
   filename    = "~/data/sound/replaceme.ogg";
   description = AudioCloseLooping3d;
   preload = true;
};

datablock AudioProfile(WaterBreathMaleSound)
{
   filename    = "~/data/sound/replaceme.ogg";
   description = AudioClosestLooping3d;
   preload = true;
};


//----------------------------------------------------------------------------

datablock AudioProfile(ImpactLightSoftSound)
{
   filename    = "~/data/sound/replaceme.ogg";
   description = AudioClose3d;
   preload = true;
   effect = ImpactSoftEffect;
};

datablock AudioProfile(ImpactLightHardSound)
{
   filename    = "~/data/sound/replaceme.ogg";
   description = AudioClose3d;
   preload = true;
   effect = ImpactHardEffect;
};

datablock AudioProfile(ImpactLightMetalSound)
{
   filename    = "~/data/sound/replaceme.ogg";
   description = AudioClose3d;
   preload = true;
   effect = ImpactMetalEffect;
};

datablock AudioProfile(ImpactLightSnowSound)
{
   filename    = "~/data/sound/replaceme.ogg";
   description = AudioClosest3d;
   preload = true;
   effect = ImpactSnowEffect;
};

datablock AudioProfile(ImpactLightWaterEasySound)
{
   filename    = "~/data/sound/replaceme.ogg";
   description = AudioClose3d;
   preload = true;
};

datablock AudioProfile(ImpactLightWaterMediumSound)
{
   filename    = "~/data/sound/replaceme.ogg";
   description = AudioClose3d;
   preload = true;
};

datablock AudioProfile(ImpactLightWaterHardSound)
{
   filename    = "~/data/sound/replaceme.ogg";
   description = AudioDefault3d;
   preload = true;
};

datablock AudioProfile(ExitingWaterLightSound)
{
   filename    = "~/data/sound/replaceme.ogg";
   description = AudioClose3d;
   preload = true;
};


//----------------------------------------------------------------------------
// Splash
//----------------------------------------------------------------------------

datablock ParticleData(PlayerSplashMist)
{
   dragCoefficient      = 2.0;
   gravityCoefficient   = -0.05;
   inheritedVelFactor   = 0.0;
   constantAcceleration = 0.0;
   lifetimeMS           = 400;
   lifetimeVarianceMS   = 100;
   useInvAlpha          = false;
   spinRandomMin        = -90.0;
   spinRandomMax        = 500.0;
   textureName          = "~/data/shapes/spaceOrc/splash";
   colors[0]     = "0.7 0.8 1.0 1.0";
   colors[1]     = "0.7 0.8 1.0 0.5";
   colors[2]     = "0.7 0.8 1.0 0.0";
   sizes[0]      = 0.5;
   sizes[1]      = 0.5;
   sizes[2]      = 0.8;
   times[0]      = 0.0;
   times[1]      = 0.5;
   times[2]      = 1.0;
};

datablock ParticleEmitterData(PlayerSplashMistEmitter)
{
   ejectionPeriodMS = 5;
   periodVarianceMS = 0;
   ejectionVelocity = 3.0;
   velocityVariance = 2.0;
   ejectionOffset   = 0.0;
   thetaMin         = 85;
   thetaMax         = 85;
   phiReferenceVel  = 0;
   phiVariance      = 360;
   overrideAdvance = false;
   lifetimeMS       = 250;
   particles = "PlayerSplashMist";
};


datablock ParticleData(PlayerBubbleParticle)
{
   dragCoefficient      = 0.0;
   gravityCoefficient   = -0.50;
   inheritedVelFactor   = 0.0;
   constantAcceleration = 0.0;
   lifetimeMS           = 400;
   lifetimeVarianceMS   = 100;
   useInvAlpha          = false;
   textureName          = "~/data/shapes/spaceOrc/splash";
   colors[0]     = "0.7 0.8 1.0 0.4";
   colors[1]     = "0.7 0.8 1.0 0.4";
   colors[2]     = "0.7 0.8 1.0 0.0";
   sizes[0]      = 0.1;
   sizes[1]      = 0.3;
   sizes[2]      = 0.3;
   times[0]      = 0.0;
   times[1]      = 0.5;
   times[2]      = 1.0;
};

datablock ParticleEmitterData(PlayerBubbleEmitter)
{
   ejectionPeriodMS = 1;
   periodVarianceMS = 0;
   ejectionVelocity = 2.0;
   ejectionOffset   = 0.5;
   velocityVariance = 0.5;
   thetaMin         = 0;
   thetaMax         = 80;
   phiReferenceVel  = 0;
   phiVariance      = 360;
   overrideAdvance = false;
   particles = "PlayerBubbleParticle";
};

datablock ParticleData(PlayerFoamParticle)
{
   dragCoefficient      = 2.0;
   gravityCoefficient   = -0.05;
   inheritedVelFactor   = 0.0;
   constantAcceleration = 0.0;
   lifetimeMS           = 400;
   lifetimeVarianceMS   = 100;
   useInvAlpha          = false;
   spinRandomMin        = -90.0;
   spinRandomMax        = 500.0;
   textureName          = "~/data/shapes/spaceOrc/splash";
   colors[0]     = "0.7 0.8 1.0 0.20";
   colors[1]     = "0.7 0.8 1.0 0.20";
   colors[2]     = "0.7 0.8 1.0 0.00";
   sizes[0]      = 0.2;
   sizes[1]      = 0.4;
   sizes[2]      = 1.6;
   times[0]      = 0.0;
   times[1]      = 0.5;
   times[2]      = 1.0;
};

datablock ParticleEmitterData(PlayerFoamEmitter)
{
   ejectionPeriodMS = 10;
   periodVarianceMS = 0;
   ejectionVelocity = 3.0;
   velocityVariance = 1.0;
   ejectionOffset   = 0.0;
   thetaMin         = 85;
   thetaMax         = 85;
   phiReferenceVel  = 0;
   phiVariance      = 360;
   overrideAdvance = false;
   particles = "PlayerFoamParticle";
};


datablock ParticleData( PlayerFoamDropletsParticle )
{
   dragCoefficient      = 1;
   gravityCoefficient   = 0.2;
   inheritedVelFactor   = 0.2;
   constantAcceleration = -0.0;
   lifetimeMS           = 600;
   lifetimeVarianceMS   = 0;
   textureName          = "~/data/shapes/spaceOrc/splash";
   colors[0]     = "0.7 0.8 1.0 1.0";
   colors[1]     = "0.7 0.8 1.0 0.5";
   colors[2]     = "0.7 0.8 1.0 0.0";
   sizes[0]      = 0.8;
   sizes[1]      = 0.3;
   sizes[2]      = 0.0;
   times[0]      = 0.0;
   times[1]      = 0.5;
   times[2]      = 1.0;
};

datablock ParticleEmitterData( PlayerFoamDropletsEmitter )
{
   ejectionPeriodMS = 7;
   periodVarianceMS = 0;
   ejectionVelocity = 2;
   velocityVariance = 1.0;
   ejectionOffset   = 0.0;
   thetaMin         = 60;
   thetaMax         = 80;
   phiReferenceVel  = 0;
   phiVariance      = 360;
   overrideAdvance = false;
   orientParticles  = true;
   particles = "PlayerFoamDropletsParticle";
};


datablock ParticleData( PlayerSplashParticle )
{
   dragCoefficient      = 1;
   gravityCoefficient   = 0.2;
   inheritedVelFactor   = 0.2;
   constantAcceleration = -0.0;
   lifetimeMS           = 600;
   lifetimeVarianceMS   = 0;
   colors[0]     = "0.7 0.8 1.0 1.0";
   colors[1]     = "0.7 0.8 1.0 0.5";
   colors[2]     = "0.7 0.8 1.0 0.0";
   sizes[0]      = 0.5;
   sizes[1]      = 0.5;
   sizes[2]      = 0.5;
   times[0]      = 0.0;
   times[1]      = 0.5;
   times[2]      = 1.0;
};

datablock ParticleEmitterData( PlayerSplashEmitter )
{
   ejectionPeriodMS = 1;
   periodVarianceMS = 0;
   ejectionVelocity = 3;
   velocityVariance = 1.0;
   ejectionOffset   = 0.0;
   thetaMin         = 60;
   thetaMax         = 80;
   phiReferenceVel  = 0;
   phiVariance      = 360;
   overrideAdvance = false;
   orientParticles  = true;
   lifetimeMS       = 100;
   particles = "PlayerSplashParticle";
};

// datablock SplashData(PlayerSplash)
// {
//    numSegments = 15;
//    ejectionFreq = 15;
//    ejectionAngle = 40;
//    ringLifetime = 0.5;
//    lifetimeMS = 300;
//    velocity = 4.0;
//    startRadius = 0.0;
//    acceleration = -3.0;
//    texWrap = 5.0;
//
//    texture = "~/data/shapes/spaceOrc/splash";
//
//    emitter[0] = PlayerSplashEmitter;
//    emitter[1] = PlayerSplashMistEmitter;
//
//    colors[0] = "0.7 0.8 1.0 0.0";
//    colors[1] = "0.7 0.8 1.0 0.3";
//    colors[2] = "0.7 0.8 1.0 0.7";
//    colors[3] = "0.7 0.8 1.0 0.0";
//    times[0] = 0.0;
//    times[1] = 0.4;
//    times[2] = 0.8;
//    times[3] = 1.0;
// };


//----------------------------------------------------------------------------
// Foot puffs
//----------------------------------------------------------------------------

datablock ParticleData(LightPuff)
{
   dragCoefficient      = 2.0;
   gravityCoefficient   = -0.01;
   inheritedVelFactor   = 0.6;
   constantAcceleration = 0.0;
   lifetimeMS           = 800;
   lifetimeVarianceMS   = 100;
   useInvAlpha          = true;
   spinRandomMin        = -35.0;
   spinRandomMax        = 35.0;
   colors[0]     = "1.0 1.0 1.0 1.0";
   colors[1]     = "1.0 1.0 1.0 0.0";
   sizes[0]      = 0.1;
   sizes[1]      = 0.8;
   times[0]      = 0.3;
   times[1]      = 1.0;
};

datablock ParticleEmitterData(LightPuffEmitter)
{
   ejectionPeriodMS = 35;
   periodVarianceMS = 10;
   ejectionVelocity = 0.2;
   velocityVariance = 0.1;
   ejectionOffset   = 0.0;
   thetaMin         = 20;
   thetaMax         = 60;
   phiReferenceVel  = 0;
   phiVariance      = 360;
   overrideAdvance = false;
   useEmitterColors = true;
   particles = "LightPuff";
};

//----------------------------------------------------------------------------
// Liftoff dust
//----------------------------------------------------------------------------

datablock ParticleData(LiftoffDust)
{
   dragCoefficient      = 1.0;
   gravityCoefficient   = -0.01;
   inheritedVelFactor   = 0.0;
   constantAcceleration = 0.0;
   lifetimeMS           = 1000;
   lifetimeVarianceMS   = 100;
   useInvAlpha          = true;
   spinRandomMin        = -90.0;
   spinRandomMax        = 500.0;
   colors[0]     = "1.0 1.0 1.0 1.0";
   sizes[0]      = 1.0;
   times[0]      = 1.0;
};

datablock ParticleEmitterData(LiftoffDustEmitter)
{
   ejectionPeriodMS = 5;
   periodVarianceMS = 0;
   ejectionVelocity = 2.0;
   velocityVariance = 0.0;
   ejectionOffset   = 0.0;
   thetaMin         = 90;
   thetaMax         = 90;
   phiReferenceVel  = 0;
   phiVariance      = 360;
   overrideAdvance = false;
   useEmitterColors = true;
   particles = "LiftoffDust";
};


//----------------------------------------------------------------------------

datablock DecalData(PlayerFootprint)
{
   sizeX       = 0.25;
   sizeY       = 0.25;
   textureName = "~/data/shapes/spaceOrc/footprint";
};

datablock DebrisData( PlayerDebris )
{
   explodeOnMaxBounce = false;

   elasticity = 0.15;
   friction = 0.5;

   lifetime = 4.0;
   lifetimeVariance = 0.0;

   minSpinSpeed = 40;
   maxSpinSpeed = 600;

   numBounces = 5;
   bounceVariance = 0;

   staticOnMaxBounce = true;
   gravModifier = 1.0;

   useRadiusMass = true;
   baseRadius = 1;

   velocity = 20.0;
   velocityVariance = 12.0;
};

datablock PlayerData(LightMaleHumanArmor)
{
   renderFirstPerson = false;
   emap = true;

   className = Armor;
   shapeFile = "~/data/shapes/spaceOrc/player.dts";
   cameraMaxDist = 3;
   computeCRC = true;

   canObserve = true;
   cmdCategory = "Clients";

   cameraDefaultFov = 90.0;
   cameraMinFov = 5.0;
   cameraMaxFov = 120.0;

   debrisShapeName = "~/data/shapes/spaceOrc/debris_player.dts";
   debris = playerDebris;

   aiAvoidThis = true;

   minLookAngle = -1.4;
   maxLookAngle = 1.4;
   maxFreelookAngle = 3.0;

   mass = 90;
   drag = 0.3;
   maxdrag = 0.4;
   density = 10;
   maxDamage = 100;
   maxEnergy =  60;
   repairRate = 0.33;
   energyPerDamagePoint = 75.0;

   rechargeRate = 0.256;

   runForce = 48 * 90;
   runEnergyDrain = 0;
   minRunEnergy = 0;
   maxForwardSpeed = 14;
   maxBackwardSpeed = 13;
   maxSideSpeed = 13;

   maxUnderwaterForwardSpeed = 8.4;
   maxUnderwaterBackwardSpeed = 7.8;
   maxUnderwaterSideSpeed = 7.8;

   jumpForce = 8.3 * 90;
   jumpEnergyDrain = 0;
   minJumpEnergy = 0;
   jumpDelay = 15;

   recoverDelay = 9;
   recoverRunForceScale = 1.2;

   minImpactSpeed = 45;
   speedDamageScale = 0.4;

   boundingBox = "1.2 1.2 2.3";
   pickupRadius = 0.75;

   // Damage location details
   boxNormalHeadPercentage       = 0.83;
   boxNormalTorsoPercentage      = 0.49;
   boxHeadLeftPercentage         = 0;
   boxHeadRightPercentage        = 1;
   boxHeadBackPercentage         = 0;
   boxHeadFrontPercentage        = 1;

   // Foot Prints
   decalData   = PlayerFootprint;
   decalOffset = 0.25;

   footPuffEmitter = LightPuffEmitter;
   footPuffNumParts = 10;
   footPuffRadius = 0.25;

   dustEmitter = LiftoffDustEmitter;

   //splash = PlayerSplash;
   splashVelocity = 4.0;
   splashAngle = 67.0;
   splashFreqMod = 300.0;
   splashVelEpsilon = 0.60;
   bubbleEmitTime = 0.4;
   splashEmitter[0] = PlayerFoamDropletsEmitter;
   splashEmitter[1] = PlayerFoamEmitter;
   splashEmitter[2] = PlayerBubbleEmitter;
   mediumSplashSoundVelocity = 10.0;
   hardSplashSoundVelocity = 20.0;
   exitSplashSoundVelocity = 5.0;

   // Controls over slope of runnable/jumpable surfaces
   runSurfaceAngle  = 70;
   jumpSurfaceAngle = 80;

   minJumpSpeed = 20;
   maxJumpSpeed = 30;

   horizMaxSpeed = 68;
   horizResistSpeed = 33;
   horizResistFactor = 0.35;

   upMaxSpeed = 80;
   upResistSpeed = 25;
   upResistFactor = 0.3;

   footstepSplashHeight = 0.35;

   //NOTE:  some sounds commented out until wav's are available

   // Footstep Sounds
   FootSoftSound        = FootLightSoftSound;
   FootHardSound        = FootLightHardSound;
   FootMetalSound       = FootLightMetalSound;
   FootSnowSound        = FootLightSnowSound;
   FootShallowSound     = FootLightShallowSplashSound;
   FootWadingSound      = FootLightWadingSound;
   FootUnderwaterSound  = FootLightUnderwaterSound;

   //FootBubblesSound     = FootLightBubblesSound;
   //movingBubblesSound   = ArmorMoveBubblesSound;
   //waterBreathSound     = WaterBreathMaleSound;

   //impactSoftSound      = ImpactLightSoftSound;
   //impactHardSound      = ImpactLightHardSound;
   //impactMetalSound     = ImpactLightMetalSound;
   //impactSnowSound      = ImpactLightSnowSound;

   //impactWaterEasy      = ImpactLightWaterEasySound;
   //impactWaterMedium    = ImpactLightWaterMediumSound;
   //impactWaterHard      = ImpactLightWaterHardSound;

   groundImpactMinSpeed    = 10.0;
   groundImpactShakeFreq   = "4.0 4.0 4.0";
   groundImpactShakeAmp    = "1.0 1.0 1.0";
   groundImpactShakeDuration = 0.8;
   groundImpactShakeFalloff = 10.0;

   //exitingWater         = ExitingWaterLightSound;

   observeParameters = "0.5 4.5 4.5";

   // Allowable Inventory Items
   maxInv[BulletAmmo] = 20;
   maxInv[HealthKit] = 1;
   maxInv[RifleAmmo] = 100;
   maxInv[CrossbowAmmo] = 50;
   maxInv[Crossbow] = 1;
   maxInv[Rifle] = 1;
};


//----------------------------------------------------------------------------
// Armor Datablock methods
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------

function Armor::onAdd(%this,%obj)
{
   // Vehicle timeout
   %obj.mountVehicle = true;

   // Default dynamic armor stats
   %obj.setRechargeRate(%this.rechargeRate);
   %obj.setRepairRate(0);
}

function Armor::onRemove(%this, %obj)
{
   if (%obj.client.player == %obj)
      %obj.client.player = 0;
}

function Armor::onNewDataBlock(%this,%obj)
{
}


//----------------------------------------------------------------------------

function Armor::onMount(%this,%obj,%vehicle,%node)
{
   if (%node == 0)  {
      %obj.setTransform("0 0 0 0 0 1 0");
      %obj.setActionThread(%vehicle.getDatablock().mountPose[%node],true,true);
      %obj.lastWeapon = %obj.getMountedImage($WeaponSlot);

      %obj.unmountImage($WeaponSlot);
      %obj.setControlObject(%vehicle);
   }
}

function Armor::onUnmount( %this, %obj, %vehicle, %node )
{
   if (%node == 0)
      %obj.mountImage(%obj.lastWeapon, $WeaponSlot);
}

function Armor::doDismount(%this, %obj, %forced)
{
   // This function is called by player.cc when the jump trigger
   // is true while mounted
   if (!%obj.isMounted())
      return;

   // Position above dismount point
   %pos    = getWords(%obj.getTransform(), 0, 2);
   %oldPos = %pos;
   %vec[0] = " 0  0  1";
   %vec[1] = " 0  0  1";
   %vec[2] = " 0  0 -1";
   %vec[3] = " 1  0  0";
   %vec[4] = "-1  0  0";
   %impulseVec  = "0 0 0";
   %vec[0] = MatrixMulVector( %obj.getTransform(), %vec[0]);

   // Make sure the point is valid
   %pos = "0 0 0";
   %numAttempts = 5;
   %success     = -1;
   for (%i = 0; %i < %numAttempts; %i++) {
      %pos = VectorAdd(%oldPos, VectorScale(%vec[%i], 3));
      if (%obj.checkDismountPoint(%oldPos, %pos)) {
         %success = %i;
         %impulseVec = %vec[%i];
         break;
      }
   }
   if (%forced && %success == -1)
      %pos = %oldPos;

   %obj.mountVehicle = false;
   %obj.schedule(4000, "mountVehicles", true);

   // Unmount from node, and give player control.
   %obj.unMount();
   %this.onUnMount(%obj);
   %obj.setControlObject(%obj);

   // Position above dismount point
   %obj.setTransform(%pos);
   %obj.applyImpulse(%pos, VectorScale(%impulseVec, %obj.getDataBlock().mass));
}


//----------------------------------------------------------------------------

function Armor::onCollision(%this,%obj,%col)
{
   if (%obj.getState() $= "Dead")
      return;

   // Try and pickup all items
   if (%col.getClassName() $= "Item") {
      %obj.pickup(%col);
      return;
   }

   // Mount vehicles
   if (%col.getDataBlock().className $= WheeledVehicleData && %obj.mountVehicle &&
         %obj.getState() $= "Move" && %col.mountable) {

      // Only mount drivers for now.
      %node = 0;
      %col.mountObject(%obj,%node);
      %obj.mVehicle = %col;
   }
}

function Armor::onImpact(%this, %obj, %collidedObject, %vec, %vecLen)
{
   %obj.damage(0, VectorAdd(%obj.getPosition(),%vec),
      %vecLen * %this.speedDamageScale, "Impact");
}


//----------------------------------------------------------------------------

function Armor::damage(%this, %obj, %sourceObject, %position, %damage, %damageType)
{
   if (%obj.getState() $= "Dead")
      return;
   %obj.applyDamage(%damage);
   %location = "Body";

   // Deal with client callbacks here because we don't have this
   // information in the onDamage or onDisable methods
   %client = %obj.client;
   %sourceClient = %sourceObject ? %sourceObject.client : 0;

   if (%obj.getState() $= "Dead")
      %client.onDeath(%sourceObject, %sourceClient, %damageType, %location);
}

function Armor::onDamage(%this, %obj, %delta)
{
   // This method is invoked by the ShapeBase code whenever the
   // object's damage level changes.
   if (%delta > 0 && %obj.getState() !$= "Dead") {

      // Increment the flash based on the amount.
      %flash = %obj.getDamageFlash() + ((%delta / %this.maxDamage) * 2);
      if (%flash > 0.75)
         %flash = 0.75;
      %obj.setDamageFlash(%flash);

      // If the pain is excessive, let's hear about it.
      if (%delta > 10)
         %obj.playPain();
   }
}

function Armor::onDisabled(%this,%obj,%state)
{
   // The player object sets the "disabled" state when damage exceeds
   // it's maxDamage value.  This is method is invoked by ShapeBase
   // state mangement code.

   // If we want to deal with the damage information that actually
   // caused this death, then we would have to move this code into
   // the script "damage" method.
   %obj.playDeathCry();
   %obj.playDeathAnimation();
   %obj.setDamageFlash(0.75);

   // Release the main weapon trigger
   %obj.setImageTrigger(0,false);

   // Schedule corpse removal.  Just keeping the place clean.
   %obj.schedule($CorpseTimeoutValue - 1000, "startFade", 1000, 0, true);
   %obj.schedule($CorpseTimeoutValue, "delete");
}


//-----------------------------------------------------------------------------

function Armor::onLeaveMissionArea(%this, %obj)
{
   // Inform the client
   %obj.client.onLeaveMissionArea();
}

function Armor::onEnterMissionArea(%this, %obj)
{
   // Inform the client
   %obj.client.onEnterMissionArea();
}

//-----------------------------------------------------------------------------

function Armor::onEnterLiquid(%this, %obj, %coverage, %type)
{
   switch(%type)
   {
      case 0: //Water
      case 1: //Ocean Water
      case 2: //River Water
      case 3: //Stagnant Water
      case 4: //Lava
         %obj.setDamageDt(%this, $DamageLava, "Lava");
      case 5: //Hot Lava
         %obj.setDamageDt(%this, $DamageHotLava, "Lava");
      case 6: //Crusty Lava
         %obj.setDamageDt(%this, $DamageCrustyLava, "Lava");
      case 7: //Quick Sand
   }
}

function Armor::onLeaveLiquid(%this, %obj, %type)
{
   %obj.clearDamageDt();
}


//-----------------------------------------------------------------------------

function Armor::onTrigger(%this, %obj, %triggerNum, %val)
{
   // This method is invoked when the player receives a trigger
   // move event.  The player automatically triggers slot 0 and
   // slot one off of triggers # 0 & 1.  Trigger # 2 is also used
   // as the jump key.
}


//-----------------------------------------------------------------------------
// Player methods
//-----------------------------------------------------------------------------

//----------------------------------------------------------------------------

function Player::kill(%this, %damageType)
{
   %this.damage(0, %this.getPosition(), 10000, %damageType);
}


//----------------------------------------------------------------------------

function Player::mountVehicles(%this,%bool)
{
   // If set to false, this variable disables vehicle mounting.
   %this.mountVehicle = %bool;
}

function Player::isPilot(%this)
{
   %vehicle = %this.getObjectMount();
   // There are two "if" statements to avoid a script warning.
   if (%vehicle)
      if (%vehicle.getMountNodeObject(0) == %this)
         return true;
   return false;
}


//----------------------------------------------------------------------------

function Player::playDeathAnimation(%this)
{
   if (%this.deathIdx++ > 11)
      %this.deathIdx = 1;
   %this.setActionThread("Death" @ %this.deathIdx);
}

function Player::playCelAnimation(%this,%anim)
{
   if (%this.getState() !$= "Dead")
      %this.setActionThread("cel"@%anim);
}


//----------------------------------------------------------------------------

function Player::playDeathCry( %this )
{
   %this.playAudio(0,DeathCrySound);
}

function Player::playPain( %this )
{
   %this.playAudio(0,PainCrySound);
}
