//-----------------------------------------------------------------------------
// Torque Game Engine
//
// Copyright (c) 2001 GarageGames.Com
//-----------------------------------------------------------------------------
// Channel assignments (channel 0 is unused in-game).
$GuiAudioType     = 1;
$SimAudioType     = 1;
$MessageAudioType = 1;
$EffectAudioType = 1;
$MusicAudioType = 2;
$MusicVolume = 1;

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
new AudioDescription(ClientAudioLooping2D)
{
   volume = 1.0;
   isLooping = true;
   is3D = false;
   type = $EffectAudioType;
};
new AudioProfile(TimeTravelLoopSfx)
{
   filename    = "~/data/sound/TimeTravelActive.wav";
   description = ClientAudioLooping2d;
   preload = true;
};
new AudioProfile(AudioButtonDown)
{
   filename = "~/data/sound/ButtonPress.wav";
   description = "AudioGui";
	preload = true;
};
new AudioProfile(AudioButtonOver)
{
   filename = "~/data/sound/buttonOver.wav";
   description = "AudioGui";
   preload = true;
};

new AudioProfile(OptionsGuiTestSound)
{
   filename = "~/data/sound/buttonOver.wav";
   description = "AudioGui";
   preload = true;
};

new AudioDescription(AudioMusic)
{
   volume   = 1.0;
   isLooping = true;
   isStreaming = true;
   is3D     = false;
   type     = $MusicAudioType;
};

new AudioDescription(TimTranceDescription : AudioMusic)
{
   volume = 0.75;
};

new AudioProfile(TimTranceSfx)
{
   fileName = "~/data/sound/music/Tim_Trance.ogg";
   description = "TimTranceDescription";
   preload = false;
};

if (isObject(MusicPlayer))
{
   MusicPlayer.addTrack(TimTranceSfx);
}

function musicControlChanged(%haveControl)
{
   if(%haveControl)
      playMusic();
}