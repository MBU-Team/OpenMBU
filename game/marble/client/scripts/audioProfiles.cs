//-----------------------------------------------------------------------------
// Torque Game Engine
//
// Copyright (c) 2001 GarageGames.Com
//-----------------------------------------------------------------------------

if (!$AudioChannelTypesDefined)
{
    error("Audio channel types are not defined, all sounds will use same channel");
}

new SFXDescription(AudioGui)
{
   volume   = 1.0;
   isLooping= false;
   is3D     = false;
   channel     = $GuiAudioType;
};
new SFXDescription(AudioMessage)
{
   volume   = 1.0;
   isLooping= false;
   is3D     = false;
   channel     = $MessageAudioType;
};
new SFXDescription(ClientAudioLooping2D)
{
   volume = 1.0;
   isLooping = true;
   is3D = false;
   channel = $EffectAudioType;
};
new SFXProfile(TimeTravelLoopSfx)
{
   filename    = "~/data/sound/TimeTravelActive.wav";
   description = ClientAudioLooping2d;
   preload = true;
};
new SFXProfile(AudioButtonDown)
{
   filename = "~/data/sound/ButtonPress.wav";
   description = "AudioGui";
	preload = true;
};
new SFXProfile(AudioButtonOver)
{
   filename = "~/data/sound/buttonOver.wav";
   description = "AudioGui";
   preload = true;
};

new SFXProfile(OptionsGuiTestSound)
{
   filename = "~/data/sound/buttonOver.wav";
   description = "AudioGui";
   preload = true;
};

new SFXDescription(AudioMusic)
{
   volume   = 1.0;
   isLooping = true;
   isStreaming = true;
   is3D     = false;
   channel     = $MusicAudioType;
};

new SFXDescription(TimTranceDescription : AudioMusic)
{
   volume = 0.75;
};

new SFXProfile(TimTranceSfx)
{
   fileName = "~/data/sound/music/Tim_Trance.ogg";
   description = "TimTranceDescription";
   //preload = false;
   preload = true;
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