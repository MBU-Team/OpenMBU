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

datablock SFXDescription(AudioDefault3d)
{
   volume   = 1.0;
   isLooping= false;

   is3D     = true;
   ReferenceDistance= 20.0;
   MaxDistance= 100.0;
   channel     = $EffectAudioType;
};

datablock SFXDescription(AudioClose3d)
{
   volume   = 1.0;
   isLooping= false;

   is3D     = true;
   ReferenceDistance= 10.0;
   MaxDistance= 60.0;
   channel     = $EffectAudioType;
};

datablock SFXDescription(AudioClosest3d)
{
   volume   = 1.0;
   isLooping= false;

   is3D     = true;
   ReferenceDistance= 5.0;
   MaxDistance= 30.0;
   channel     = $EffectAudioType;
};


//-----------------------------------------------------------------------------
// Looping sounds

datablock SFXDescription(AudioDefaultLooping3d)
{
   volume   = 1.0;
   isLooping= true;

   is3D     = true;
   ReferenceDistance= 20.0;
   MaxDistance= 100.0;
   channel     = $EffectAudioType;
};

datablock SFXDescription(AudioCloseLooping3d)
{
   volume   = 1.0;
   isLooping= true;

   is3D     = true;
   ReferenceDistance= 10.0;
   MaxDistance= 50.0;
   channel     = $EffectAudioType;
};

datablock SFXDescription(AudioClosestLooping3d)
{
   volume   = 1.0;
   isLooping= true;

   is3D     = true;
   ReferenceDistance= 5.0;
   MaxDistance= 30.0;
   channel     = $EffectAudioType;
};

datablock SFXDescription(QuieterAudioClosestLooping3d)
{
   volume   = 0.5;
   isLooping= true;

   is3D     = true;
   ReferenceDistance= 5.0;
   MaxDistance= 30.0;
   channel     = $EffectAudioType;
};

datablock SFXDescription(Quieter3D)
{
   volume   = 0.60;
   isLooping= false;

   is3D     = true;
   ReferenceDistance= 20.0;
   MaxDistance= 100.0;
   channel     = $EffectAudioType;
};

//-----------------------------------------------------------------------------
// 2d sounds
//-----------------------------------------------------------------------------

// Used for non-looping environmental sounds (like power on, power off)
datablock SFXDescription(Audio2D)
{
   volume = 1.0;
   isLooping = false;
   is3D = false;
   channel = $EffectAudioType;
};

// Used for Looping Environmental Sounds
datablock SFXDescription(AudioLooping2D)
{
   volume = 1.0;
   isLooping = true;
   is3D = false;
   channel = $EffectAudioType;
};


//-----------------------------------------------------------------------------
// Ready - Set - Get Rolling

datablock SFXProfile(pauseSfx)
{
   filename    = "~/data/sound/level_text.wav";
   description = Audio2D;
   preload = true;
};

datablock SFXProfile(easterNewSfx)
{
   filename    = "~/data/sound/easter_egg.wav";
   description = Audio2D;
   preload = true;
};

datablock SFXProfile(easterNotNewSfx)
{
   filename    = "~/data/sound/pu_easter.wav";
   description = Audio2D;
   preload = true;
};

datablock SFXProfile(spawnSfx)
{
   filename    = "~/data/sound/spawn_alternate.wav";
   description = AudioClose3d;
   preload = true;
};

datablock SFXProfile(pickupSfx)
{
   filename    = "~/data/sound/pickup.wav";
   description = AudioClosest3d;
   preload = true;
};

datablock SFXProfile(HelpDingSfx)
{
   filename    = "~/data/sound/InfoTutorial.wav";
   description = Audio2D;
   preload = true;
};

datablock SFXProfile(ReadyVoiceSfx)
{
   filename    = "~/data/sound/ready.wav";
   description = Audio2D;
   preload = true;
};

datablock SFXProfile(SetVoiceSfx)
{
   filename    = "~/data/sound/set.wav";
   description = Audio2D;
   preload = true;
};

datablock SFXProfile(CheckpointSfx)
{
   filename = "~/data/sound/checkpoint.wav";
   description = Audio2D;
   preload = true;
};

datablock SFXProfile(GetRollingVoiceSfx)
{
   filename    = "~/data/sound/go.wav";
   description = Audio2D;
   preload = true;
};

datablock SFXProfile(NewHighScoreSfx)
{
   filename    = "~/data/sound/new_high_score";
   description = Audio2D;
   preload = true;
};

datablock SFXProfile(AchievementSfx)
{
   filename    = "~/data/sound/new_achievement";
   description = Audio2D;
   preload = true;
};

datablock SFXProfile(AllAchievementSfx)
{
   filename    = "~/data/sound/all_achievement";
   description = Audio2D;
   preload = true;
};

datablock SFXProfile(WonRaceSfx)
{
   filename    = "~/data/sound/finish.wav";
   description = Audio2D;
   volume = 0.55;
   preload = true;
};

datablock SFXProfile(MissingGemsSfx)
{
   filename    = "~/data/sound/missingGems.wav";
   description = Audio2D;
   preload = true;
};

//datablock SFXProfile(jumpSfx)
//{
   //filename    = "~/data/sound/bounce.wav";
   //description = AudioDefault3d;
   //preload = true;
//};

datablock SFXProfile(bounceSfx)
{
   filename    = "~/data/sound/bounce.wav";
   description = AudioDefault3d;
   preload = true;
};

//-----------------------------------------------------------------------------
// Multiplayer

datablock SFXProfile(PlayerJoinSfx)
{
   filename    = "~/data/sound/spawn_alternate.wav";
   description = Audio2D;
   volume = 0.5;
   preload = true;
};

datablock SFXProfile(PlayerDropSfx)
{
   filename    = "~/data/sound/InfoTutorial.wav";
   volume = 0.5;
   description = Audio2D;
   preload = true;
};

//-----------------------------------------------------------------------------
// Misc

datablock SFXProfile(PenaltyVoiceSfx)
{
   filename    = "~/data/sound/penalty.wav";
   description = AudioDefault3d;
   preload = true;
};

// IGC HACK -pw
// datablock SFXProfile(OutOfBoundsVoiceSfx)
// {
   // filename    = "~/data/sound/whoosh.wav";
   // description = Audio2D;
   // preload = true;
// };

datablock SFXProfile(DestroyedVoiceSfx)
{
   filename    = "~/data/sound/destroyedVoice.wav";
   description = AudioDefault3d;
   preload = true;
};

