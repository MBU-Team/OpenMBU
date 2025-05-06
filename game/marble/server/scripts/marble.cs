//-----------------------------------------------------------------------------
// Torque Game Engine
//
// Copyright (c) 2001 GarageGames.Com
// Portions Copyright (c) 2001 by Sierra Online, Inc.
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------

datablock ParticleData(BounceParticle)
{
   textureName          = "~/data/particles/burst";
   dragCoeffiecient     = 0.5;
   gravityCoefficient   = -0.1;
   windCoefficient      = 0;
   inheritedVelFactor   = 0;
   constantAcceleration = -2;
   lifetimeMS           = 400;
   lifetimeVarianceMS   = 50;
   useInvAlpha =  false;
   spinSpeed     = 90;
   spinRandomMin = -90.0;
   spinRandomMax =  90.0;

   colors[0]     = "0.5 0.5 0.5 0.3";
   colors[1]     = "0.3 0.3 0.2 0.1";
   colors[2]     = "0.2 0.2 0.1 0.0";

   sizes[0]      = 0.8;
   sizes[1]      = 0.4;
   sizes[2]      = 0.2;

   times[0]      = 0;
   times[1]      = 0.75;
   times[2]      = 1.0;
};

datablock ParticleEmitterData(MarbleBounceEmitter)
{
   ejectionPeriodMS = 10;
   periodVarianceMS = 0;
   ejectionVelocity = 6.0;
   velocityVariance = 0.25;
   thetaMin         = 80.0;
   thetaMax         = 90.0;
   lifetimeMS       = 250;
   particles = "BounceParticle";
};

//-----------------------------------------------------------------------------

datablock ParticleData(TrailParticle)
{
   textureName          = "~/data/particles/burst";
   dragCoeffiecient     = 1.0;
   gravityCoefficient   = 0;
   windCoefficient      = 0;
   inheritedVelFactor   = 1;
   constantAcceleration = 0;
   lifetimeMS           = 100;
   lifetimeVarianceMS   = 10;
   useInvAlpha =  true;
   spinSpeed     = 0;

   colors[0]     = "1 1 0 0.0";
   colors[1]     = "1 1 0 1";
   colors[2]     = "1 1 1 0.0";

   sizes[0]      = 0.7;
   sizes[1]      = 0.4;
   sizes[2]      = 0.1;

   times[0]      = 0;
   times[1]      = 0.15;
   times[2]      = 1.0;
};

//datablock ParticleData(TrailParticle2)
//{
//   textureName          = "~/data/particles/star";
//   dragCoeffiecient     = 1.0;
//   gravityCoefficient   = 0;
//   windCoefficient      = 0;
//   inheritedVelFactor   = 0;
//   constantAcceleration = -2;
//   lifetimeMS           = 400;
//   lifetimeVarianceMS   = 100;
//   useInvAlpha =  true;
//   spinSpeed     = 90;
//   spinRandomMin = -90.0;
//   spinRandomMax =  90.0;
//
//   colors[0]     = "0.9 0.0 0.0 0.9";
//   colors[1]     = "0.9 0.9 0.0 0.9";
//   colors[2]     = "0.9 0.9 0.9 0.0";
//
//   sizes[0]      = 0.165;
//   sizes[1]      = 0.165;
//   sizes[2]      = 0.165;
//
//   times[0]      = 0;
//   times[1]      = 0.55;
//   times[2]      = 1.0;
//};

datablock ParticleData(TrailParticle2)
{
   textureName          = "~/data/particles/burst";
   dragCoefficient     = 0.0;
   gravityCoefficient   = 0;   
   inheritedVelFactor   = 0.00;
   lifetimeMS           = 2000;
   lifetimeVarianceMS   = 0;
   useInvAlpha = false;
   spinRandomMin = -90.0;
   spinRandomMax = 90.0;

   colors[0]     = "0.5 0.3 0.2 1.0";
   colors[1]     = "0.5 0.3 0.2 1.0";
   colors[2]     = "0.2 0.0 0.0 0.0";

   sizes[0]      = 0.6;
   sizes[1]      = 0.5;
   sizes[2]      = 0.1;

   times[0]      = 0.0;
   times[1]      = 0.5;
   times[2]      = 1.0;
};

datablock ParticleEmitterData(MarbleTrailEmitter)
{
   ejectionPeriodMS = 9;
   periodVarianceMS = 0;
   ejectionVelocity = 3.0;
   velocityVariance = 0.25;
   thetaMin         = 60.0;
   thetaMax         = 90.0;
   lifetimeMS       = 1000000;
   particles = TrailParticle2;
};

//-----------------------------------------------------------------------------
// ActivePowerUp
// 0 - no active powerup
// 1 - Super Jump
// 2 - Super Speed
// 3 - Super Bounce
// 4 - Indestructible

datablock SFXProfile(Bounce1Sfx)
{
   filename    = "~/data/sound/bouncehard1.wav";
   description = AudioClosest3d;
   preload = true;
};

datablock SFXProfile(Bounce2Sfx)
{
   filename    = "~/data/sound/bouncehard2.wav";
   description = AudioClosest3d;
   preload = true;
};

datablock SFXProfile(Bounce3Sfx)
{
   filename    = "~/data/sound/bouncehard3.wav";
   description = AudioClosest3d;
   preload = true;
};

datablock SFXProfile(Bounce4Sfx)
{
   filename    = "~/data/sound/bouncehard4.wav";
   description = AudioClosest3d;
   preload = true;
};

datablock SFXProfile(MegaBounce1Sfx)
{
   filename    = "~/data/sound/mega_bouncehard1.wav";
   description = AudioClose3d;
   preload = true;
};

datablock SFXProfile(MegaBounce2Sfx)
{
   filename    = "~/data/sound/mega_bouncehard2.wav";
   description = AudioClose3d;
   preload = true;
};

datablock SFXProfile(MegaBounce3Sfx)
{
   filename    = "~/data/sound/mega_bouncehard3.wav";
   description = AudioClose3d;
   preload = true;
};

datablock SFXProfile(MegaBounce4Sfx)
{
   filename    = "~/data/sound/mega_bouncehard4.wav";
   description = AudioClose3d;
   preload = true;
};

datablock SFXProfile(JumpSfx)
{
   filename    = "~/data/sound/Jump.wav";
   description = AudioClosest3d;
   preload = true;
};

datablock SFXProfile(RollingHardSfx)
{
   filename    = "~/data/sound/Rolling_Hard.wav";
   description = AudioClosestLooping3d;
   volume = 0.91;
   preload = true;
};

datablock SFXProfile(RollingMegaSfx)
{
   filename    = "~/data/sound/mega_roll.wav";
   description = AudioCloseLooping3d;
   volume = 0.91 * 0.85;
   preload = true;
};

datablock SFXProfile(RollingIceSfx)
{
   filename    = "~/data/sound/ice_roll.wav";
   description = AudioClosestLooping3d;
   volume = 0.91 * 0.85;
   preload = true;
};

datablock SFXProfile(SlippingSfx)
{
   filename    = "~/data/sound/Sliding.wav";
   description = AudioClosestLooping3d;
   volume = 0.60;
   preload = true;
};

datablock SFXProfile(MediumBounceEmotiveSfx)
{
	fileName = "~/data/soundEmotive/mbu_ow1.wav";
	description = AudioClose3d;
	preload = 1;
};

datablock SFXProfile(HardBounceEmotiveSfx)
{
	fileName = "~/data/soundEmotive/mbu_ow2.wav";
	description = AudioClose3d;
	preload = 1;
};

datablock SFXProfile(OOBEmotiveSfx)
{
	fileName = "~/data/soundEmotive/mbu_oob.wav";
	description = AudioClose3d;
	preload = 1;
};

datablock SFXProfile(SlipEmotiveSfx)
{
	fileName = "~/data/soundEmotive/mbu_wee.wav";
	description = AudioClose3d;
	preload = 1;
};

datablock MarbleData(DefaultMarble)
{
   shapeFile = "~/data/shapes/balls/marble01.dts";
   emap = true;
   renderFirstPerson = true;
   maxRollVelocity = 15;
   angularAcceleration = 75;
   brakingAcceleration = 30;
   gravity = 20;
   //size = 1.5; moved from MarbleData to Marble
   megaSize = 2.25; //1.5 * 2.25; // is now multiplied by size automatically
   mass = 1;
   staticFriction = 1.1;
   kineticFriction = 0.7;
   bounceKineticFriction = 0.2;
   maxDotSlide = 0.5;
   bounceRestitution = 0.5;
   jumpImpulse = 7.5;
   maxForceRadius = 50;
   
   blastRechargeTime = 36000;
   maxNaturalBlastRecharge = 30000;

   bounce1 = Bounce1Sfx;
   bounce2 = Bounce2Sfx;
   bounce3 = Bounce3Sfx;
   bounce4 = Bounce4Sfx;
   megabounce1 = MegaBounce1Sfx;
   megabounce2 = MegaBounce2Sfx;
   megabounce3 = MegaBounce3Sfx;
   megabounce4 = MegaBounce4Sfx;

   rollHardSound = RollingHardSfx;
   rollMegaSound = RollingMegaSfx;
   rollIceSound = RollingIceSfx;
   slipSound = SlippingSfx;
   jumpSound = JumpSfx;
   
   // Emitters
   minTrailSpeed = 15;            // Trail threshold
   trailEmitter = MarbleTrailEmitter;
   
   minBounceSpeed = 3;           // Bounce threshold
   bounceEmitter = MarbleBounceEmitter;
   softBounceImpactShakeAmp = "0.025 0.025 0.025";
   softBounceImpactShakeDuration = 0.6;
   softBounceImpactShakeFalloff = 6;
   softBounceImpactShakeFreq = "13 0 0";
   
   minHardBounceSpeed = 9;
   hardBounceEmitter = MarbleHardBounceEmitter;
   hardBounceImpactShakeAmp = "0.2 0.2 0.2";
   hardBounceImpactShakeDuration = 1.1;
   hardBounceImpactShakeFalloff = 5;
   hardBounceImpactShakeFreq = "10 3 3";
   
   minMediumBounceSpeed = 6;
   mediumBounceEmitter = MarbleMediumBounceEmitter;
   mediumBounceImpactShakeAmp = "0.1 0.1 0.1";
   mediumBounceImpactShakeDuration = 0.8;
   mediumBounceImpactShakeFalloff = 6;
   mediumBounceImpactShakeFreq = "12 1 1";
   
   slipEmotiveThreshold = 4.5;
   
   cameraDecay = 4;
   cameraLag = 0.2; //0.1;
   cameraLagMaxOffset = 4.2; //2.2;

   powerUps = PowerUpDefs;

   // Allowable Inventory Items
   maxInv[SuperJumpItem] = 20;
   maxInv[SuperSpeedItem] = 20;
   maxInv[SuperBounceItem] = 20;
   maxInv[IndestructibleItem] = 20;
   maxInv[TimeTravelItem] = 20;
//   maxInv[GoodiesItem] = 10;

   dynamicReflection = true;
};

datablock MarbleData(MarbleOne : DefaultMarble)
{
   shapeFile = "~/data/shapes/balls/marble01.dts";
};

datablock MarbleData(MarbleTwo : DefaultMarble)
{
   shapeFile = "~/data/shapes/balls/marble03.dts";
};

datablock MarbleData(MarbleThree : DefaultMarble)
{
   shapeFile = "~/data/shapes/balls/marble04.dts";
};

datablock MarbleData(MarbleFour : DefaultMarble)
{
   shapeFile = "~/data/shapes/balls/marble05.dts";
};

datablock MarbleData(MarbleFive : DefaultMarble)
{
   shapeFile = "~/data/shapes/balls/marble06.dts";
};

datablock MarbleData(MarbleSix : DefaultMarble)
{
   shapeFile = "~/data/shapes/balls/marble07.dts";
};

datablock MarbleData(MarbleSeven : DefaultMarble)
{
   shapeFile = "~/data/shapes/balls/marble12.dts";
};

datablock MarbleData(MarbleEight : DefaultMarble)
{
   shapeFile = "~/data/shapes/balls/marble15.dts";
};

datablock MarbleData(MarbleNine : DefaultMarble)
{
   shapeFile = "~/data/shapes/balls/marble02.dts";
};

datablock MarbleData(MarbleTen : DefaultMarble)
{
   shapeFile = "~/data/shapes/balls/marble26.dts";
};

datablock MarbleData(MarbleEleven : DefaultMarble)
{
   shapeFile = "~/data/shapes/balls/marble27.dts";
};

datablock MarbleData(MarbleTwelve : DefaultMarble)
{
   shapeFile = "~/data/shapes/balls/marble28.dts";
};

datablock MarbleData(MarbleThirteen : DefaultMarble)
{
   shapeFile = "~/data/shapes/balls/marble29.dts";
};

datablock MarbleData(MarbleFourteen : DefaultMarble)
{
   shapeFile = "~/data/shapes/balls/marble30.dts";
};

datablock MarbleData(MarbleFifteen : DefaultMarble)
{
   shapeFile = "~/data/shapes/balls/marble11.dts";
};

datablock MarbleData(MarbleSixteen : DefaultMarble)
{
   shapeFile = "~/data/shapes/balls/marble18.dts";
};

datablock MarbleData(MarbleSeventeen : DefaultMarble)
{
   shapeFile = "~/data/shapes/balls/marble20.dts";
};

datablock MarbleData(MarbleEightteen : DefaultMarble)
{
   shapeFile = "~/data/shapes/balls/marble33.dts";
};

datablock MarbleData(MarbleNineteen : DefaultMarble)
{
   shapeFile = "~/data/shapes/balls/marble34.dts";
};

datablock MarbleData(MarbleTwenty : DefaultMarble)
{
   shapeFile = "~/data/shapes/balls/marble09.dts";
};

datablock MarbleData(MarbleTwentyOne : DefaultMarble)
{
   shapeFile = "~/data/shapes/balls/marble13.dts";
};

datablock MarbleData(MarbleTwentyTwo : DefaultMarble)
{
   shapeFile = "~/data/shapes/balls/marble14.dts";
};

datablock MarbleData(MarbleTwentyThree : DefaultMarble)
{
   shapeFile = "~/data/shapes/balls/marble17.dts";
};

datablock MarbleData(MarbleTwentyFour : DefaultMarble)
{
   shapeFile = "~/data/shapes/balls/marble19.dts";
};

datablock MarbleData(MarbleTwentyFive : DefaultMarble)
{
   shapeFile = "~/data/shapes/balls/marble21.dts";
};

datablock MarbleData(MarbleTwentySix : DefaultMarble)
{
   shapeFile = "~/data/shapes/balls/marble22.dts";
};

datablock MarbleData(MarbleTwentySeven : DefaultMarble)
{
   shapeFile = "~/data/shapes/balls/marble23.dts";
};

datablock MarbleData(MarbleTwentyEight : DefaultMarble)
{
   shapeFile = "~/data/shapes/balls/marble24.dts";
};

datablock MarbleData(MarbleTwentyNine : DefaultMarble)
{
   shapeFile = "~/data/shapes/balls/marble25.dts";
};

datablock MarbleData(MarbleThirty : DefaultMarble)
{
   shapeFile = "~/data/shapes/balls/marble31.dts";
};

datablock MarbleData(MarbleThirtyOne : DefaultMarble)
{
   shapeFile = "~/data/shapes/balls/marble32.dts";
};

datablock MarbleData(MarbleThirtyTwo : DefaultMarble)
{
   shapeFile = "~/data/shapes/balls/marble08.dts";
};

datablock MarbleData(MarbleThirtyThree : DefaultMarble)
{
   shapeFile = "~/data/shapes/balls/marble10.dts";
};

datablock MarbleData(MarbleThirtyFour : DefaultMarble)
{
   shapeFile = "~/data/shapes/balls/marble16.dts";
};

datablock MarbleData(MarbleThirtyFive : DefaultMarble)
{
   shapeFile = "~/data/shapes/balls/marble35.dts";
};

datablock MarbleData(MarbleThirtySix : DefaultMarble)
{
   shapeFile = "~/data/shapes/balls/marble36.dts";
};

datablock MarbleData(MarbleThirtySeven : DefaultMarble)
{
   shapeFile = "~/data/shapes/balls/marble37.dts";
};

//-----------------------------------------------------------------------------

function MarbleData::onAdd(%this, %obj)
{
   echo("New Marble: " @ %obj);
}

function MarbleData::onTrigger(%this, %obj, %triggerNum, %val)
{
}


//-----------------------------------------------------------------------------

function MarbleData::onCollision(%this,%obj,%col)
{
   // JMQ: workaround: skip hidden objects  
   if (%col.isHidden())
      return;
  
   // Try and pickup all items
   if (%col.getClassName() $= "Item")
   {
      %data = %col.getDatablock();
      %obj.pickup(%col,1);
   }
}


//-----------------------------------------------------------------------------
// The following event callbacks are punted over to the connection
// for processing

function MarbleData::onEnterPad(%this,%object)
{
   %object.client.onEnterPad();
}

function MarbleData::onLeavePad(%this, %object)
{
   %object.client.onLeavePad();
}

function MarbleData::onStartPenalty(%this, %object)
{
   %object.client.onStartPenalty();
}

function MarbleData::onOutOfBounds(%this, %object)
{
   %object.client.onOutOfBounds();
}

function MarbleData::setCheckpoint(%this, %object, %check)
{
   %object.client.setCheckpoint(%check);
}

function MarbleData::onFinishPoint(%this, %object)
{
   %object.client.onFinishPoint();
}


//-----------------------------------------------------------------------------
// Marble object
//-----------------------------------------------------------------------------

function Marble::setPowerUp(%this,%item,%reset)
{
   if (%item.powerUpId != 6)
   {
      // Hack: don't set powerup gui if blast item
      commandToClient(%this.client,'setPowerup',%item.bmpFile);
      %this.powerUpData = %item;
   }
   %this.setPowerUpId(%item.powerUpId,%reset);
}

function Marble::getPowerUp(%this)
{
   return %this.powerUpData;
}

function Marble::onPowerUpUsed(%obj)
{
   commandToClient(%obj.client,'setPowerup',"");
   %obj.playAudio(0, %obj.powerUpData.activeAudio);
   %obj.powerUpData = "";
}

function Marble::onBlastUsed(%obj)
{
   %obj.playAudio(0, DoBlastSfx);
}

function Marble::onTimeFreeze(%this,%timeFreeze)
{
   %this.client.incBonusTime(%timeFreeze);
}

function Marble::onPowerUpExpired( %obj, %powerUpId )
{
   if( %powerUpId == 7 )
      %obj.playAudio( 0, ShrinkMegaMarbleSfx );
}

//-----------------------------------------------------------------------------

function marbleVel()
{
   return $MarbleVelocity;
}

function metricsMarble()
{
   Canvas.pushDialog(FrameOverlayGui, 1000);
   TextOverlayControl.setValue("$MarbleVelocity");
}

function Marble::onEmotive(%this, %emotive)
{
    if (!$Marble::UseEmotives)
        return;

	if (%emotive $= "Bounce1" || %emotive $= "Bounce2" || %emotive $= "Bounce3" || %emotive $= "Bounce4")
	{
		return;
	}

	if (%emotive $= "Slipping")
	{
       // TODO: apparently playAudio doesn't work on client side in OpenMBU?
       //%this.playAudio(0, SlipEmotiveSfx);
       //sfxPlay(SlipEmotiveSfx, %this.getPosition());
	}
	else if (%emotive $= "OOB")
	{
        // TODO: apparently playAudio doesn't work on client side in OpenMBU?
		//%this.playAudio(0, OOBEmotiveSfx);
        sfxPlay(OOBEmotiveSfx, %this.getPosition());
	}
	else if (%emotive $= "Win")
	{

	}
	else if (%emotive $= "SoftBounce")
	{

	}
	else if (%emotive $= "MediumBounce")
	{
        // TODO: apparently playAudio doesn't work on client side in OpenMBU?
		//%this.playAudio(0, MediumBounceEmotiveSfx);
        sfxPlay(MediumBounceEmotiveSfx, %this.getPosition());
	}
	else if (%emotive $= "HardBounce")
	{
        // MBXP Star Spinning animation:
		//%this.playRingThread(1, 0.5, 0.1, 300);
        
        // TODO: apparently playAudio doesn't work on client side in OpenMBU?
		//%this.playAudio(0, HardBounceEmotiveSfx);
        sfxPlay(HardBounceEmotiveSfx, %this.getPosition());
	}
}
