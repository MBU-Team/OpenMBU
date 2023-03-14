//-----------------------------------------------------------------------------
// Torque Game Engine
// 
// Copyright (c) 2001 GarageGames.Com
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// PowerUp base class
//-----------------------------------------------------------------------------

function PowerUp::onPickup(%this,%obj,%user,%amount)
{
   // Dont' pickup the power up if it's the same
   // one we already have.
   if (%user.powerUpData == %this)
      return false;

   // Grab it...
   %user.client.play2d(%this.pickupAudio);
   if (%this.powerUpId)
   {
      if(%obj.showHelpOnPickup)
      {
         %text = avar($Text::PullToUse, %this.useName);
         addHelpLine(%text , false);
      }
   
      %user.setPowerUp(%this);
   }
   Parent::onPickup(%this, %obj, %user, %amount);
   return true;
}

//-----------------------------------------------------------------------------
// PowerUp particle data
//-----------------------------------------------------------------------------

datablock ParticleData(BlastParticle)
{
   textureName          = "~/data/particles/twirl";
   dragCoefficient      = 0.0;
   gravityCoefficient   = -0.5;
   inheritedVelFactor   = 0.1;
   constantAcceleration = 1;
   lifetimeMS           = 1000;
   lifetimeVarianceMS   = 150;
   spinSpeed     = 90;
   spinRandomMin = -90.0;
   spinRandomMax =  90.0;

   //colors[0]     = "0 0.5 1 0";
   //colors[1]     = "0 0.6 1 1.0";
   //colors[2]     = "0 0.6 1 0.0";

   colors[0]     = "0.38 0.38 0.88 0";
   colors[1]     = "0.34 0.34 0.64 1";
   colors[2]     = "0.30 0.30 0.30 0";

   sizes[0]      = 0.25;
   sizes[1]      = 0.25;
   sizes[2]      = 0.5;

   times[0]      = 0;
   times[1]      = 0.75;
   times[2]      = 1.0;
};

datablock ParticleEmitterData(BlastEmitter)
{
   ejectionPeriodMS = 10;
   periodVarianceMS = 0;
   ejectionVelocity = 15.0;
   velocityVariance = 0.25;
   thetaMin         = 00.0;
   thetaMax         = 90.0;
   lifetimeMS       = 200;
   particles = "BlastParticle";
};

datablock ParticleData(SuperJumpParticle)
{
   textureName          = "~/data/particles/twirl";
   dragCoefficient      = 0.25;
   gravityCoefficient   = 0;
   inheritedVelFactor   = 0.1;
   constantAcceleration = 0;
   lifetimeMS           = 1000;
   lifetimeVarianceMS   = 150;
   spinSpeed     = 90;
   spinRandomMin = -90.0;
   spinRandomMax =  90.0;

   //colors[0]     = "0 0.5 1 0";
   //colors[1]     = "0 0.6 1 1.0";
   //colors[2]     = "0 0.6 1 0.0";

   colors[0]     = "0.38 0.38 0.88 0";
   colors[1]     = "0.34 0.34 0.64 1";
   colors[2]     = "0.30 0.30 0.30 0";

   sizes[0]      = 0.25;
   sizes[1]      = 0.25;
   sizes[2]      = 0.5;

   times[0]      = 0;
   times[1]      = 0.75;
   times[2]      = 1.0;
};

datablock ParticleEmitterData(MarbleSuperJumpEmitter)
{
   ejectionPeriodMS = 10;
   periodVarianceMS = 0;
   ejectionVelocity = 1.0;
   velocityVariance = 0.25;
   thetaMin         = 150.0;
   thetaMax         = 170.0;
   lifetimeMS       = 5000;
   particles = "SuperJumpParticle";
};

//-----------------------------------------------------------------------------

//OldParticle--------------------------------
//datablock ParticleData(SuperSpeedParticle)
//{
//   textureName          = "~/data/particles/spark";
//   dragCoefficient      = 0.25;
//   gravityCoefficient   = 0;
//   inheritedVelFactor   = 0.25;
//   constantAcceleration = 0;
//   lifetimeMS           = 1500;
//   lifetimeVarianceMS   = 150;
//
//   colors[0]     = "0.8 0.8 0 0";
//   colors[1]     = "0.8 0.8 0 1.0";
//  colors[2]     = "0.8 0.8 0 0.0";
//
//   sizes[0]      = 0.25;
//   sizes[1]      = 0.25;
//   sizes[2]      = 1.0;
//
//   times[0]      = 0;
//   times[1]      = 0.25;
//   times[2]      = 1.0;
//};

//NewParticle--------------------------------
datablock ParticleData(SuperSpeedParticle)
{
   textureName          = "~/data/particles/smoke";
   dragCoefficient     = 4.0;
   windCoefficient     = 0.0;
   gravityCoefficient   = 0;   
   inheritedVelFactor   = 0.00;
   lifetimeMS           = 2000;
   lifetimeVarianceMS   = 750;
   useInvAlpha = false;
   spinRandomMin = 0.0;
   spinRandomMax = 150.0;
   spinSpeed     = 15.0;

   colors[0]     = "0.42 0.42 0.38 0.1";
   colors[1]     = "0.34 0.34 0.34 0.1";
   colors[2]     = "0.30 0.30 0.30 0.1";

   sizes[0]      = 0.5;
   sizes[1]      = 1;
   sizes[2]      = 2;

   times[0]      = 0.0;
   times[1]      = 0.5;
   times[2]      = 1.0;
};

datablock ParticleEmitterData(MarbleSuperSpeedEmitter)
{
   ejectionPeriodMS = 5;
   periodVarianceMS = 0;
   ejectionVelocity = 1.0;
   velocityVariance = 0.25;
   thetaMin         = 130.0;
   thetaMax         = 170.0;
   lifetimeMS       = 5000;
   particles = "SuperSpeedParticle";
};

//-----------------------------------------------------------------------------

// Unused
//datablock ParticleEmitterData(MarbleSuperBounceEmitter)
//{
   //ejectionPeriodMS = 20;
   //periodVarianceMS = 0;
   //ejectionVelocity = 3.0;
   //velocityVariance = 0.25;
   //thetaMin         = 80.0;
   //thetaMax         = 90.0;
   //lifetimeMS       = 250;
   //particles = "MarbleStar";
//};

//-----------------------------------------------------------------------------

// Unused
//datablock ParticleEmitterData(MarbleShockAbsorberEmitter)
//{
   //ejectionPeriodMS = 20;
   //periodVarianceMS = 0;
   //ejectionVelocity = 3.0;
   //velocityVariance = 0.25;
   //thetaMin         = 80.0;
   //thetaMax         = 90.0;
   //lifetimeMS       = 250;
   //particles = "MarbleStar";
//};

//-----------------------------------------------------------------------------

// Unused
//datablock ParticleEmitterData(MarbleHelicopterEmitter)
//{
   //ejectionPeriodMS = 20;
   //periodVarianceMS = 0;
   //ejectionVelocity = 3.0;
   //velocityVariance = 0.25;
   //thetaMin         = 80.0;
   //thetaMax         = 90.0;
   //lifetimeMS       = 5000;
   //particles = "MarbleStar";
//};

//-----------------------------------------------------------------------------
// Superjump powerUp
//-----------------------------------------------------------------------------

datablock SFXProfile(doSuperJumpSfx)
{
   filename    = "~/data/sound/use_superjump.wav";
   description = AudioClose3d;
   preload = true;
};

datablock SFXProfile(PuSuperJumpVoiceSfx)
{
   filename    = "~/data/sound/super_jump.wav";
   description = Audio2D;
   preload = true;
};

datablock ItemData(SuperJumpItem)
{
   // Mission editor category
   category = "Powerups";
   className = "PowerUp";
   powerUpId = 1;

   activeAudio = DoSuperJumpSfx;
   pickupAudio = PuSuperJumpVoiceSfx;

   // Basic Item properties
   shapeFile = "~/data/shapes/items/superjump.dts";
   bmpFile = "powerup_jump.png";
   mass = 1;
   friction = 1;
   elasticity = 0.3;
   emap = false;

   // Dynamic properties defined by the scripts
   pickupText = $Text::ASuperJump;
   useName = $Text::UseSuperJump;
   maxInventory = 1;
};

//-----------------------------------------------------------------------------
// Superspeed powerUp
//-----------------------------------------------------------------------------

datablock SFXProfile(doSuperSpeedSfx)
{
   filename    = "~/data/sound/use_speed.wav";
   description = AudioClose3d;
   preload = true;
};

datablock SFXProfile(PuSuperSpeedVoiceSfx)
{
   filename    = "~/data/sound/super_speed.wav";
   description = Audio2D;
   preload = true;
};

datablock ItemData(SuperSpeedItem)
{
   // Mission editor category
   category = "Powerups";
   className = "PowerUp";
   powerUpId = 2;

   activeAudio = DoSuperSpeedSfx;
   pickupAudio = PuSuperSpeedVoiceSfx;

   // Basic Item properties
   shapeFile = "~/data/shapes/items/superspeed.dts";
   bmpFile = "powerup_speed.png";
   mass = 1;
   friction = 1;
   elasticity = 0.3;
   emap = false;

   // Dynamic properties defined by the scripts
   pickupText = $Text::ASuperSpeed;
   useName = $Text::UseSuperSpeed;
   maxInventory = 1;
};

function SuperSpeedItem::onAdd(%this, %obj)
{
   %obj.playThread(0,"Ambient");
}   

//-----------------------------------------------------------------------------
// Helicopter powerUp
//-----------------------------------------------------------------------------

datablock SFXProfile(PuGyrocopterVoiceSfx)
{
   filename    = "~/data/sound/gyrocopter.wav";
   description = Audio2D;
   preload = true;
};

datablock ItemData(HelicopterItem)
{
   // Mission editor category
   category = "Powerups";
   className = "PowerUp";
   powerUpId = 5;

   pickupAudio = PuGyrocopterVoiceSfx;

   // Basic Item properties
   shapeFile = "~/data/shapes/images/helicopter.dts";
   bmpFile = "powerup_copter.png";
   mass = 1;
   friction = 1;
   elasticity = 0.3;

   // Dynamic properties defined by the scripts
   pickupText = $Text::AGyrocopter;
   useName = $Text::UseGyrocopter;
   maxInventory = 1;
};

datablock SFXProfile(HelicopterLoopSfx)
{
   filename    = "~/data/sound/Use_Gyrocopter.wav";
   description = AudioClosestLooping3d;
   preload = true;
};

datablock ShapeBaseImageData(HelicopterImage)
{
   // Basic Item properties
   shapeFile = "~/data/shapes/images/helicopter_image.dts";
   emap = true;
   mountPoint = 0;
   offset = "0 0 0";
   stateName[0]                     = "Rotate";
   stateSequence[0]                 = "rotate";
   stateSound[0] = HelicopterLoopSfx;
   ignoreMountRotation = true;
};

//-----------------------------------------------------------------------------
// Blast marble powerUp
//-----------------------------------------------------------------------------

datablock SFXProfile(doBlastSfx)
{
   filename    = "~/data/sound/use_blast.wav";
   description = AudioClose3d;
   preload = true;
};

datablock SFXProfile(PuBlastVoiceSfx)
{
   filename    = "~/data/sound/ultrablast.wav";
   description = Audio2D;
   preload = true;
};

datablock ShapeBaseImageData(BlastImage)
{
   // Basic Item properties
   shapeFile = "~/data/shapes/images/distort.dts";
   emap = false;
   mountPoint = 0;
   offset = "0 0 0";
   stateName[0]                     = "Grow";
   stateSequence[0]                 = "grow";
//   stateSound[0] = doBlastSfx;
   ignoreMountRotation = true;
};


datablock ItemData(BlastItem)
{
   // Mission editor category
   category = "Powerups";
   className = "PowerUp";
   powerUpId = 6;

   activeAudio = DoBlastSfx;
   pickupAudio = PuBlastVoiceSfx;

   // Basic Item properties
   shapeFile = "~/data/shapes/images/blast.dts";
   bmpFile = "powerup_blast.png";
   mass = 1;
   friction = 1;
   elasticity = 0.3;
   
   // Dynamic properties defined by the scripts
   pickupText = $Text::ABlast;
   useName = $Text::UseBlast;
   maxInventory = 1;
};


function BlastItem::onAdd(%this, %obj)
{
   %obj.playThread(0,"Ambient");
   %obj.rotate = 0;
}   


//-----------------------------------------------------------------------------
// Mega marble powerUp
//-----------------------------------------------------------------------------

datablock SFXProfile(DoMegaMarbleSfx)
{
   filename    = "~/data/sound/use_mega.wav";
   description = AudioClose3d;
   preload = true;
};

datablock SFXProfile(PuMegaMarbleVoiceSfx)
{
   filename    = "~/data/sound/mega_marble.wav";
   description = Audio2D;
   preload = true;
};

datablock SFXProfile(ShrinkMegaMarbleSfx)
{
   filename    = "~/data/sound/MegaShrink.wav";
   description = AudioClose3d;
   preload = true;
};

datablock ItemData(MegaMarbleItem)
{
   // Mission editor category
   category = "Powerups";
   className = "PowerUp";
   powerUpId = 7;

   activeAudio = DoMegaMarbleSfx;
   pickupAudio = PuMegaMarbleVoiceSfx;

   // Basic Item properties
   shapeFile = "~/data/shapes/images/grow.dts";
   bmpFile = "powerup_mega.png";
   mass = 1;
   friction = 1;
   elasticity = 0.3;

   // Dynamic properties defined by the scripts
   pickupText = $Text::AMegaMarble;
   useName = $Text::UseMegaMarble;
   maxInventory = 1;
};

function MegaMarbleItem::onAdd(%this, %obj)
{
   %obj.playThread(0,"ambient");
}   


//-----------------------------------------------------------------------------
// Special non-inventory power ups
//-----------------------------------------------------------------------------

datablock SFXProfile(PuTimeTravelVoiceSfx)
{
   filename    = "~/data/sound/time_travel.wav";
   description = Audio2D;
   preload = true;
};

datablock ItemData(TimeTravelItem)
{
   // Mission editor category
   category = "Powerups";
   className = "PowerUp";

   // Basic Item properties
   pickupAudio = PuTimeTravelVoiceSfx;
   shapeFile = "~/data/shapes/items/timetravel.dts";
   mass = 6;
   friction = 1;
   elasticity = 0.3;
   emap = false;

   // Dynamic properties defined by the scripts
   noRespawn = true;
   pickupText = $Text::ATimeTravelBonus;
   maxInventory = 1;
};

function TimeTravelItem::getPickupTextData(%this, %obj)
{
  if(%obj.timeBonus !$= "")
      %time = %obj.timeBonus / 1000;
  else
      %time = $Game::TimeTravelBonus / 1000;
  return %time;
}

function TimeTravelItem::onPickup(%this,%obj,%user,%amount)
{
   Parent::onPickup(%this, %obj, %user, %amount);
   if(%obj.timeBonus !$= "")
      %user.client.incBonusTime(%obj.timeBonus);
   else
      %user.client.incBonusTime($Game::TimeTravelBonus);
}

//-----------------------------------------------------------------------------

datablock SFXProfile(PuAntiGravityVoiceSfx)
{
   filename    = "~/data/sound/pu_gravity.wav";
   description = Audio2D;
   preload = true;
};

datablock ItemData(AntiGravityItem)
{
   // Mission editor category
   category = "Powerups";
   className = "PowerUp";

   pickupAudio = PuAntiGravityVoiceSfx;
   pickupText = $Text::AGravityMod;

   // Basic Item properties
   shapeFile = "~/data/shapes/items/antiGravity.dts";
   mass = 1;
   friction = 1;
   elasticity = 0.3;
   emap = false;

   // Dynamic properties defined by the scripts
   maxInventory = 1;
};

function AntiGravityItem::onAdd(%this, %obj)
{
   %obj.playThread(0,"Ambient");
}   

function AntiGravityItem::onPickup(%this,%obj,%user,%amount)
{
   %rotation = getWords(%obj.getTransform(),3);
   %ortho = vectorOrthoBasis(%rotation);
   %down = getWords(%ortho,6);
   if (VectorDot(%user.getGravityDir(),%down)<0.9)
   {
      Parent::onPickup(%this, %obj, %user, %amount);
      %user.setGravityDir(%ortho);
   }
}

//-----------------------------------------------------------------------------
// Inventory version of time travel powerup for multi-player

datablock ItemData(TimeTravelItem_MP : TimeTravelItem)
{
   powerupId = 8;
   noRespawn = false;
   bmpFile = "powerup_timetravel.png";
};

//-----------------------------------------------------------------------------
// Random powerup

// Unused
//datablock SFXProfile(PuRandomVoiceSfx)
//{
   //filename    = "~/data/sound/puRandomVoice.wav";
   //description = Audio2D;
   //preload = true;
//};
//
//datablock ItemData(RandomPowerUpItem)
//{
   //// Mission editor category
   //category = "Powerups";
   //className = "PowerUp";
//
   //// Basic Item properties
   //shapeFile = "~/data/shapes/items/random.dts";
   //mass = 1;
   //friction = 1;
   //elasticity = 0.3;
   //emap = false;
//
   //// Dynamic properties defined by the scripts
   //noRespawn = false;
   //maxInventory = 1;
//};
//
//function RandomPowerUpItem::onPickup(%this,%obj,%user,%amount)
//{
   //%pupIdx = getRandom(1,5);
   //switch (%pupIdx)
   //{
      //case 1:
         //%pup = SuperJumpItem;
      //case 2:
         //%pup = SuperSpeedItem;
      //case 3:
         //%pup = HelicopterItem;
      //case 4:
         //%pup = BlastItem;
      //case 5:
         //%pup = MegaMarbleItem;
   //}
    //return PowerUp::onPickup(%pup.getId(),%obj,%user,%amount);
//}

//-----------------------------------------------------------------------------
// power-up parameters
//-----------------------------------------------------------------------------

datablock PowerUpData(PowerUpDefs)
{
   // This datablock holds the properties
   // of all the powerups.
   
   // Possible properties of powerups.
   // Note: leave value alone to accept default behavior.
   //    boostDir -- direction that boost applies
   //    boostAmount -- impulse of boost
   //    boostMassless -- whether boost is massless or not
   //    airAccel -- modify air acceleration by this factor
   //    gravityMod -- modify gravity by this factor
   //    bounce -- change bounce restitution to this value
   //    repulseMax -- apply up to this much force to other marbles
   //    repulseDist -- max distance at which repulse force is applied
   //    massScale -- scale mass by this amount
   //    sizeScale -- scale size by this amount
   //    activateTime -- time, in ms, for powerup to be activated and have an effect

   // Blast Ability -- triggered by energy level not powerup
   image[0] = BlastImage;
   emitter[0] = BlastEmitter;
   boostDir[0] = "0 0 1";
   boostAmount[0] = 8; // small hop to get off surface
   duration[0] = 384;
   repulseMax[0] = 60;
   repulseDist[0] = 10;
   activateTime[0] = 150;

   // Super Jump
   emitter[1] = MarbleSuperJumpEmitter; 		
   duration[1] = 1000;
   boostDir[1] = "0 0 1";
   boostAmount[1] = 20;
   boostMassless[1] = 0.7;
   activateTime[1] = 0;
   
   // Super Speed
   emitter[2] = MarbleSuperSpeedEmitter;
   duration[2] = 1000;
   boostDir[2] = "0 1 0";
   boostAmount[2] = 25;
   boostMassless[2] = 0.7;
   activateTime[2] = 100;

   // Super Bounce
   //image[3] = SuperBounceImage;
   //duration[3] = 5000;
   //bounce[3] = 0.9;
   //activateTime[3] = 0;

   // Shock Absorber
   //image[4] = ShockAbsorberImage;
   //duration[4] = 5000;
   //boost[4] = 0.01;
   //activateTime[4] = 0;

   // Helicopter
   image[5] = HelicopterImage;
   duration[5] = 5000;
   gravityMod[5] = 0.25;
   airAccel[5] = 2;
   activateTime[5] = 70;

   // Blast
//   image[6] = BlastImage;
   duration[6] = 400;
   blastRecharge[6] = true;
   
   // Mega marble
   image[7] = MegaMarbleImage;
   duration[7] = 10000;
   boostAmount[7] = 5; // small hop to get off surface
   massScale[7] = 5;
   sizeScale[7] = 2.25;
   boostDir[7] = "0 0 1";
   activateTime[7] = 100;
   
   // Time freeze marble
   image[8] = TimeFreezeImage;
   duration[8] = 5000;
   timeFreeze[8] = 5000;
   activateTime[8] = 0;

   // currently unused...
   //emitter[3] = MarbleSuperBounceEmitter;
   //emitter[4] = MarbleShockAbsorberEmitter;
   //emitter[5] = MarbleHelicopterEmitter;
};
