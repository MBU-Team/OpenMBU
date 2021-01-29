//-----------------------------------------------------------------------------
// FireWorks, currently implemented using the particle engine.
//-----------------------------------------------------------------------------

datablock ParticleData(FireWorkSmoke)
{
   textureName          = "~/data/particles/saturn";
   dragCoefficient      = 1;
   gravityCoefficient   = 0;
   inheritedVelFactor   = 0;
   windCoefficient      = 0;
   constantAcceleration = 0;
   lifetimeMS           = 2000;
   lifetimeVarianceMS   = 200;
   spinSpeed     = 0;
   spinRandomMin = -90.0;
   spinRandomMax =  90.0;
   useInvAlpha   = true;

   colors[0]     = "1 1 0 0";
   colors[1]     = "1 0 0 1.0";
   colors[2]     = "1 0 0 0.0";

   sizes[0]      = 0.1;
   sizes[1]      = 0.2;
   sizes[2]      = 0.3;

   times[0]      = 0.0;
   times[1]      = 0.2;
   times[2]      = 1.0;
};

datablock ParticleEmitterData(FireWorkSmokeEmitter)
{
   ejectionPeriodMS = 100;
   periodVarianceMS = 0;
   ejectionVelocity = 1;
   velocityVariance = 0.2;
   ejectionOffset   = 0.75;
   thetaMin         = 0;
   thetaMax         = 90;
   phiReferenceVel  = 0;
   phiVariance      = 360;
   //overrideAdvances = false;
   //orientParticles  = true;
   lifetimeMS       = 5000;
   particles = "FireWorkSmoke";
};


//-----------------------------------------------------------------------------

datablock ParticleData(RedFireWorkSpark)
{
   textureName          = "~/data/particles/star";
   dragCoefficient      = 0;
   gravityCoefficient   = 0.0;
   windCoefficient      = 0;
   inheritedVelFactor   = 0.2;
   constantAcceleration = 0.0;
   lifetimeMS           = 500;
   lifetimeVarianceMS   = 50;
   useInvAlpha   = true;

   colors[0]     = "1 1 0 1.0";
   colors[1]     = "1 1 0 1.0";
   colors[2]     = "1 0 0 0.0";

   sizes[0]      = 0.2;
   sizes[1]      = 0.2;
   sizes[2]      = 0.2;

   times[0]      = 0.0;
   times[1]      = 0.5;
   times[2]      = 1.0;
};

datablock ParticleEmitterData(RedFireWorkSparkEmitter)
{
   ejectionPeriodMS = 15;
   periodVarianceMS = 0;
   ejectionVelocity = 1;
   velocityVariance = 0.25;
   ejectionOffset   = 0.0;
   thetaMin         = 0;
   thetaMax         = 180;
   phiReferenceVel  = 0;
   phiVariance      = 360;
   overrideAdvances = false;
   orientParticles  = false;
   lifetimeMS       = 300;
   particles = "RedFireWorkSpark";
};

datablock ExplosionData(RedFireWorkSparkExplosion)
{
   emitter[0] = RedFireWorkSparkEmitter;

   // Turned off..
   shakeCamera = false;
   impulseRadius = 0;
   lightStartRadius = 0;
   lightEndRadius = 0;
};


//-----------------------------------------------------------------------------

datablock ParticleData(RedFireWorkTrail)
{
   textureName          = "~/data/particles/spark";
   dragCoefficient      = 1;
   gravityCoefficient   = 0;
   inheritedVelFactor   = 0;
   windCoefficient      = 0;
   constantAcceleration = 0;
   lifetimeMS           = 600;
   lifetimeVarianceMS   = 100;
   spinSpeed     = 0;
   spinRandomMin = -90.0;
   spinRandomMax =  90.0;
   useInvAlpha   = true;

   colors[0]     = "1 1 0 1.0";
   colors[1]     = "1 0 0 1.0";
   colors[2]     = "1 0 0 0.0";

   sizes[0]      = 0.1;
   sizes[1]      = 0.05;
   sizes[2]      = 0.01;

   times[0]      = 0.0;
   times[1]      = 0.5;
   times[2]      = 1.0;
};

datablock ParticleEmitterData(RedFireWorkTrailEmitter)
{
   ejectionPeriodMS = 30;
   periodVarianceMS = 0;
   ejectionVelocity = 0.1;
   velocityVariance = 0.0;
   ejectionOffset   = 0.0;
   thetaMin         = 170;
   thetaMax         = 180;
   phiReferenceVel  = 0;
   phiVariance      = 360;
   //overrideAdvances = false;
   //orientParticles  = true;
   lifetimeMS       = 5000;
   particles = "RedFireWorkTrail";
};

datablock DebrisData(RedFireWork)
{
   //shapeFile = "file";
   texture = "~data/particles/spark";
   emitters = "RedFireWorkTrailEmitter";
   
   explosion = RedFireWorkSparkExplosion;
   elasticity = 0.2;
   friction = 1;
   numBounces = 1;
   bounceVariance = 0;
   explodeOnMaxBounce = true;
   staticOnMaxBounce = false;
   snapOnMaxBounce = false;
   minSpinSpeed = 0;
   maxSpinSpeed = 0;
   render2D = false;
   lifetime = 1.5;
   lifetimeVariance = 0.4;
   velocity = 2;
   velocityVariance = 0.5;
   fade = false;
   useRadiusMass = false;
   baseRadius = 0.2;
   gravModifier = 0.05;
   terminalVelocity = 6;
   ignoreWater = true;
};

datablock ExplosionData(RedFireWorkExplosion)
{
   //soundProfile = ExplodeSfx;
   lifeTimeMS = 1200;
   offset = 0.1;
   
   debris = RedFireWork;
   debrisThetaMin = 0;
   debrisThetaMax = 90;
   debrisPhiMin = 0;
   debrisPhiMax = 360;
   debrisNum = 10;
   debrisNumVariance = 2;
   debrisVelocity = 3;
   debrisVelocityVariance = 0.5;

   // Misc.
   shakeCamera = false;
   impulseRadius = 0;
   lightStartRadius = 0;
   lightEndRadius = 0;
};


//-----------------------------------------------------------------------------

datablock ParticleData(BlueFireWorkSpark)
{
   textureName          = "~/data/particles/bubble";
   dragCoefficient      = 0;
   gravityCoefficient   = 0.0;
   windCoefficient      = 0;
   inheritedVelFactor   = 0.2;
   constantAcceleration = 0.0;
   lifetimeMS           = 2000;
   lifetimeVarianceMS   = 200;
   useInvAlpha   = true;

   colors[0]     = "0 0 1 1.0";
   colors[1]     = "0.5 0.5 1 1.0";
   colors[2]     = "1 1 1 0.0";

   sizes[0]      = 0.2;
   sizes[1]      = 0.2;
   sizes[2]      = 0.2;

   times[0]      = 0.0;
   times[1]      = 0.5;
   times[2]      = 1.0;
};

datablock ParticleEmitterData(BlueFireWorkSparkEmitter)
{
   ejectionPeriodMS = 60;
   periodVarianceMS = 0;
   ejectionVelocity = 0.5;
   velocityVariance = 0.25;
   ejectionOffset   = 0.0;
   thetaMin         = 0;
   thetaMax         = 180;
   phiReferenceVel  = 0;
   phiVariance      = 360;
   overrideAdvances = false;
   orientParticles  = false;
   lifetimeMS       = 300;
   particles = "BlueFireWorkSpark";
};

datablock ExplosionData(BlueFireWorkSparkExplosion)
{
   emitter[0] = BlueFireWorkSparkEmitter;

   // Turned off..
   shakeCamera = false;
   impulseRadius = 0;
   lightStartRadius = 0;
   lightEndRadius = 0;
};


//-----------------------------------------------------------------------------

datablock ParticleData(BlueFireWorkTrail)
{
   textureName          = "~/data/particles/spark";
   dragCoefficient      = 1;
   gravityCoefficient   = 0;
   inheritedVelFactor   = 0;
   windCoefficient      = 0;
   constantAcceleration = 0;
   lifetimeMS           = 600;
   lifetimeVarianceMS   = 100;
   spinSpeed     = 0;
   spinRandomMin = -90.0;
   spinRandomMax =  90.0;
   useInvAlpha   = true;

   colors[0]     = "0 0 1 1.0";
   colors[1]     = "0.5 0.5 1 1.0";
   colors[2]     = "1 1 1 0.0";

   sizes[0]      = 0.1;
   sizes[1]      = 0.05;
   sizes[2]      = 0.01;

   times[0]      = 0.0;
   times[1]      = 0.5;
   times[2]      = 1.0;
};

datablock ParticleEmitterData(BlueFireWorkTrailEmitter)
{
   ejectionPeriodMS = 30;
   periodVarianceMS = 0;
   ejectionVelocity = 0.1;
   velocityVariance = 0.0;
   ejectionOffset   = 0.0;
   thetaMin         = 170;
   thetaMax         = 180;
   phiReferenceVel  = 0;
   phiVariance      = 360;
   //overrideAdvances = false;
   //orientParticles  = true;
   lifetimeMS       = 5000;
   particles = "BlueFireWorkTrail";
};

datablock DebrisData(BlueFireWork)
{
   //shapeFile = "file";
   texture = "~data/particles/spark";
   emitters = "BlueFireWorkTrailEmitter";
   
   explosion = BlueFireWorkSparkExplosion;
   elasticity = 0.2;
   friction = 1;
   numBounces = 1;
   bounceVariance = 0;
   explodeOnMaxBounce = true;
   staticOnMaxBounce = false;
   snapOnMaxBounce = false;
   minSpinSpeed = 0;
   maxSpinSpeed = 0;
   render2D = false;
   lifetime = 1.5;
   lifetimeVariance = 0.4;
   velocity = 2;
   velocityVariance = 0.5;
   fade = false;
   useRadiusMass = false;
   baseRadius = 0.2;
   gravModifier = 0.05;
   terminalVelocity = 6;
   ignoreWater = true;
};

datablock ExplosionData(BlueFireWorkExplosion)
{
   //soundProfile = ExplodeSfx;
   lifeTimeMS = 1200;
   offset = 0.2;
   
   debris = BlueFireWork;
   debrisThetaMin = 0;
   debrisThetaMax = 90;
   debrisPhiMin = 0;
   debrisPhiMax = 360;
   debrisNum = 10;
   debrisNumVariance = 2;
   debrisVelocity = 3;
   debrisVelocityVariance = 0.5;

   // Misc.
   shakeCamera = false;
   impulseRadius = 0;
   lightStartRadius = 0;
   lightEndRadius = 0;
};


datablock ParticleEmitterNodeData(FireWorkNode)
{
   timeMultiple = 1;
};

//-----------------------------------------------------------------------------

function serverStartFireWorks(%pad)
{
   // Create the cleanup group
   if (!isObject(FireWorks)) {
      new SimGroup(FireWorks);
      MissionCleanup.add(FireWorks);
   }

   // Create a ParticleNode to run the emitter
   %position = %pad.getPosition();
   %rotation = %pad.rotation;

   %obj = new ParticleEmitterNode(){
      datablock = FireWorkNode;
      emitter = FireWorkSmokeEmitter;
      position = %position;
      rotation = %rotation;
   };
   FireWorks.add(%obj);
   
   // Create the explosions on clients -- explosions don't ghost, so need commandToClient
   for( %clientIndex = 0; %clientIndex < ClientGroup.getCount(); %clientIndex++ )
   {
      %cl = ClientGroup.getObject( %clientIndex );
      commandToClient(%cl, 'launchWave',0,%pad.getPosition(),%pad.rotation,RedFireWorkExplosion.getId(),BlueFireWorkExplosion.getId());
   }
}

function serverEndFireWorks()
{
   if (isObject(FireWorks))
      FireWorks.delete();
   cancel($Game::FireWorkSchedule);
}
