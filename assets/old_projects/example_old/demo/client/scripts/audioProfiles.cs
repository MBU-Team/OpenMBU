//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

// Channel assignments (channel 0 is unused in-game).

$GuiAudioType     = 1;
$SimAudioType     = 2;
$MessageAudioType = 3;

new AudioDescription(AudioGui)
{
   volume   = 1.0;
   isLooping= false;
   is3D     = false;
   type     = $GuiAudioType;
};

new AudioDescription(AudioMessage)
{
   volume   = 1.0;
   isLooping= false;
   is3D     = false;
   type     = $MessageAudioType;
};

new AudioProfile(AudioButtonOver)
{
   filename = "~/data/sound/gui/buttonOver.ogg";
   description = "AudioGui";
   preload = true;
};

new AudioProfile(AudioStartup)
{
   filename = "~/data/sound/gui/startup.ogg";
   description = "AudioGui";
   preload = true;
};
