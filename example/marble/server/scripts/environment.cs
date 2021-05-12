
//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------
// Following scripting (C) Tim Aste
// Please read the EULA and Documentation for more information on what this stuff does! *Seriously*

// Audio Profiles

datablock SFXProfile(Warzone)
{
   filename    = "~/data/environment/warzone.wav";
   description = AudioLooping2D;
   volume   = 5.0;
};

datablock SFXProfile(Blackforest)
{
   filename    = "~/data/environment/blackforest.wav";
   description = AudioLooping2D;
   volume   = 5.0;
};

datablock SFXProfile(Underwater)
{
   filename    = "~/data/environment/underwater.wav";
   description = AudioLooping2D;
};

datablock SFXProfile(Highland)
{
   filename    = "~/data/environment/highland.wav";
   description = AudioLooping2D;
};

datablock SFXProfile(Chemical)
{
   filename    = "~/data/environment/chemical.wav";
   description = AudioLooping2D;
};

datablock SFXProfile(Afrika)
{
   filename    = "~/data/environment/Afrika.wav";
   description = AudioLooping2D;
};

// Begin Environment Datablocks

// Chemical: Floating Dark Dust Specks

datablock PrecipitationData(dustspecks)
{
   dropTexture = "~/data/environment/dusty2";
   dropSize = 0.15;
   useTrueBillboards = false;
   splashMS = 250;
};


//Chemical: Volumetric Fog Effect Layer

datablock PrecipitationData(fluff)
{
   soundProfile = "Chemical";
   dropTexture = "~/data/environment/fluff";
   dropSize = 5.25;
   useTrueBillboards = false;
   splashMS = 250;
};


//Highland: Floating Flower Particles (White)

datablock PrecipitationData(specks)
{
   soundProfile = "Highland";
   dropTexture = "~/data/environment/spark";
   dropSize = 0.5;
   useTrueBillboards = false;
   splashMS = 250;
};

// Underwater: Bubble Layer One

datablock PrecipitationData(bubble1)
{
   soundProfile = "Underwater";
   dropTexture = "~/data/environment/bubbles.png";
   dropSize = 0.35;
   useTrueBillboards = false;
   splashMS = 250;
};

// Underwater: Bubble Layer Two

datablock PrecipitationData(bubble2)
{
   dropTexture = "~/data/environment/bubbles.png";
   dropSize = 0.55;
   useTrueBillboards = false;
   splashMS = 250;
};

// Underwater: Bubble Layer Three

datablock PrecipitationData(bubble3)
{
   dropTexture = "~/data/environment/bubbles.png";
   dropSize = 0.15;
   useTrueBillboards = false;
   splashMS = 250;
};

// Warzone: Haze Effect Layer

datablock PrecipitationData(WarStorm)
{
   dropTexture = "~/data/environment/warhaze";
   splashTexture = "~/data/environment/warhaze2";
   dropSize = 10;
   splashSize = 2;
   useTrueBillboards = false;
   splashMS = 250;
};

// Warzone: Dust Effect Layer One

datablock PrecipitationData(Dusty)
{
   dropTexture = "~/data/environment/dusty";
   splashTexture = "~/data/environment/dusty";
   dropSize = 0.25;
   splashSize = 0.25;
   useTrueBillboards = false;
   splashMS = 250;
};

// Warzone: Dust Effect Layer Two

datablock PrecipitationData(Dusty2)
{
   dropTexture = "~/data/environment/dusty2";
   splashTexture = "~/data/environment/dusty";
   dropSize = 0.25;
   splashSize = 0.25;
   useTrueBillboards = false;
   splashMS = 250;
};

// Warzone: Black Rain Layer

datablock PrecipitationData(DeathRain)
{
   soundProfile = "Warzone";

   dropTexture = "~/data/environment/deathrain";
   splashTexture = "~/data/environment/deathrain_splash";
   dropSize = 0.35;
   splashSize = 0.1;
   useTrueBillboards = false;
   splashMS = 500;
};

// Afrika: Small sand flips

datablock PrecipitationData(afrikaspecks)
{
   soundProfile = "Afrika";
   dropTexture = "~/data/environment/dusty2";
   dropSize = 0.05;
   useTrueBillboards = false;
   splashMS = 250;
};

// Black Forest: Floating Leaves

datablock PrecipitationData(Leaves)
{
   soundProfile = "Blackforest";

   dropTexture = "~/data/environment/leaves";
   splashTexture = "~/data/environment/deathrain_splash";
   dropSize = 0.35;
   splashSize = 0.1;
   useTrueBillboards = false;
   splashMS = 500;
};

