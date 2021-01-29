//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------

datablock AudioProfile(HeavyRainSound)
{
   filename    = "~/data/sound/environment/ambient/rain.ogg";
   description = AudioLooping2d;
};

datablock PrecipitationData(HeavyRain)
{
   soundProfile = "HeavyRainSound";

   dropTexture = "~/data/environment/rain";
   dropShader = "PrecipShader";
   dropsPerSide = 4;
   
   splashTexture = "~/data/environment/water_splash";
   splashShader = "PrecipShader";
   splashesPerSide = 2;
};

 //-----------------------------------------------------------------------------

datablock AudioProfile(ThunderCrash1Sound)
{
   filename  = "~/data/sound/environment/ambient/thunder1.ogg";
   description = Audio2d;
};

datablock AudioProfile(ThunderCrash2Sound)
{
   filename  = "~/data/sound/environment/ambient/thunder2.ogg";
   description = Audio2d;
};

datablock AudioProfile(ThunderCrash3Sound)
{
   filename  = "~/data/sound/environment/ambient/thunder3.ogg";
   description = Audio2d;
};

datablock AudioProfile(ThunderCrash4Sound)
{
   filename  = "~/data/sound/environment/ambient/thunder4.ogg";
   description = Audio2d;
};

//datablock AudioProfile(LightningHitSound)
//{
//   filename  = "~/data/sound/crossbow_explosion.ogg";
//   description = AudioLightning3d;
//};

//datablock LightningData(LightningStorm)
//{
//   strikeTextures[0]  = "demo/data/environment/lightning1frame1";
//   strikeTextures[1]  = "demo/data/environment/lightning1frame2";
//   strikeTextures[2]  = "demo/data/environment/lightning1frame3";
//   
//   //strikeSound = LightningHitSound;
//   thunderSounds[0] = ThunderCrash1Sound;
//   thunderSounds[1] = ThunderCrash2Sound;
//   thunderSounds[2] = ThunderCrash3Sound;
//   thunderSounds[3] = ThunderCrash4Sound;
//};
//
//datablock volumeLightData(SimpleVolumeLight)
//{
//   texture = "~/data/textures/hikari.png";
//   LPDistance = "12";
//   shootDistance = "18";
//   xExtent = "8";
//   yExtent = "8";
//   subdivideU = "32";
//   subdivideV = "32";
//   footColor = "1.000000 1.000000 1.000000 0.200000";
//   tailColor = "0.20000  0.200000 0.200000 0.000000";
//
//};
