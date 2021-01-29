//-----------------------------------------------------------------------------
// Torque Game Engine
// 
// Copyright (c) 2001 GarageGames.Com
// Portions Copyright (c) 2001 by Sierra Online, Inc.
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// 3D Sounds
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Single shot sounds

datablock AudioDescription(AudioDefault3d)
{
   volume   = 1.0;
   isLooping= false;

   is3D     = true;
   ReferenceDistance= 20.0;
   MaxDistance= 100.0;
   type     = $EffectAudioType;
};

datablock AudioDescription(AudioClose3d)
{
   volume   = 1.0;
   isLooping= false;

   is3D     = true;
   ReferenceDistance= 10.0;
   MaxDistance= 60.0;
   type     = $EffectAudioType;
};

datablock AudioDescription(AudioClosest3d)
{
   volume   = 1.0;
   isLooping= false;

   is3D     = true;
   ReferenceDistance= 5.0;
   MaxDistance= 30.0;
   type     = $EffectAudioType;
};


//-----------------------------------------------------------------------------
// Looping sounds

datablock AudioDescription(AudioDefaultLooping3d)
{
   volume   = 1.0;
   isLooping= true;

   is3D     = true;
   ReferenceDistance= 20.0;
   MaxDistance= 100.0;
   type     = $EffectAudioType;
};

datablock AudioDescription(AudioCloseLooping3d)
{
   volume   = 1.0;
   isLooping= true;

   is3D     = true;
   ReferenceDistance= 10.0;
   MaxDistance= 50.0;
   type     = $EffectAudioType;
};

datablock AudioDescription(AudioClosestLooping3d)
{
   volume   = 1.0;
   isLooping= true;

   is3D     = true;
   ReferenceDistance= 5.0;
   MaxDistance= 30.0;
   type     = $EffectAudioType;
};

datablock AudioDescription(QuieterAudioClosestLooping3d)
{
   volume   = 0.5;
   isLooping= true;

   is3D     = true;
   ReferenceDistance= 5.0;
   MaxDistance= 30.0;
   type     = $EffectAudioType;
};

datablock AudioDescription(Quieter3D)
{
   volume   = 0.60;
   isLooping= false;

   is3D     = true;
   ReferenceDistance= 20.0;
   MaxDistance= 100.0;
   type     = $EffectAudioType;
};

//-----------------------------------------------------------------------------
// 2d sounds
//-----------------------------------------------------------------------------

// Used for non-looping environmental sounds (like power on, power off)
datablock AudioDescription(Audio2D)
{
   volume = 1.0;
   isLooping = false;
   is3D = false;
   type = $EffectAudioType;
};

// Used for Looping Environmental Sounds
datablock AudioDescription(AudioLooping2D)
{
   volume = 1.0;
   isLooping = true;
   is3D = false;
   type = $EffectAudioType;
};


//-----------------------------------------------------------------------------
// Ready - Set - Get Rolling

datablock AudioProfile(pauseSfx)
{
   filename    = "~/data/sound/level_text.wav";
   description = Audio2D;
   preload = true;
};

datablock AudioProfile(easterNewSfx)
{
   filename    = "~/data/sound/easter_egg.wav";
   description = Audio2D;
   preload = true;
};

datablock AudioProfile(easterNotNewSfx)
{
   filename    = "~/data/sound/pu_easter.wav";
   description = Audio2D;
   preload = true;
};

datablock AudioProfile(spawnSfx)
{
   filename    = "~/data/sound/spawn_alternate.wav";
   description = AudioClose3d;
   preload = true;
};

datablock AudioProfile(pickupSfx)
{
   filename    = "~/data/sound/pickup.wav";
   description = AudioClosest3d;
   preload = true;
};

datablock AudioProfile(HelpDingSfx)
{
   filename    = "~/data/sound/InfoTutorial.wav";
   description = Audio2D;
   preload = true;
};

datablock AudioProfile(ReadyVoiceSfx)
{
   filename    = "~/data/sound/ready.wav";
   description = Audio2D;
   preload = true;
};

datablock AudioProfile(SetVoiceSfx)
{
   filename    = "~/data/sound/set.wav";
   description = Audio2D;
   preload = true;
};

datablock AudioProfile(CheckpointSfx)
{
   filename = "~/data/sound/checkpoint.wav";
   description = Audio2D;
   preload = true;
};

datablock AudioProfile(GetRollingVoiceSfx)
{
   filename    = "~/data/sound/go.wav";
   description = Audio2D;
   preload = true;
};

datablock AudioProfile(WonRaceSfx)
{
   filename    = "~/data/sound/finish.wav";
   description = Audio2D;
   volume = 0.55;
   preload = true;
};

datablock AudioProfile(MissingGemsSfx)
{
   filename    = "~/data/sound/missingGems.wav";
   description = Audio2D;
   preload = true;
};

datablock AudioProfile(jumpSfx)
{
   filename    = "~/data/sound/bounce.wav";
   description = AudioDefault3d;
   preload = true;
};

datablock AudioProfile(bounceSfx)
{
   filename    = "~/data/sound/bounce.wav";
   description = AudioDefault3d;
   preload = true;
};

//-----------------------------------------------------------------------------
// Multiplayer

datablock AudioProfile(PlayerJoinSfx)
{
   filename    = "~/data/sound/spawn_alternate.wav";
   description = Audio2D;
   volume = 0.5;
   preload = true;
};

datablock AudioProfile(PlayerDropSfx)
{
   filename    = "~/data/sound/InfoTutorial.wav";
   volume = 0.5;
   description = Audio2D;
   preload = true;
};

//-----------------------------------------------------------------------------
// Misc

datablock AudioProfile(PenaltyVoiceSfx)
{
   filename    = "~/data/sound/penalty.wav";
   description = AudioDefault3d;
   preload = true;
};

// IGC HACK -pw
// datablock AudioProfile(OutOfBoundsVoiceSfx)
// {
   // filename    = "~/data/sound/whoosh.wav";
   // description = Audio2D;
   // preload = true;
// };

datablock AudioProfile(DestroyedVoiceSfx)
{
   filename    = "~/data/sound/destroyedVoice.wav";
   description = AudioDefault3d;
   preload = true;
};

