//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
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
   type     = $SimAudioType;
};

datablock AudioDescription(AudioClose3d)
{
   volume   = 1.0;
   isLooping= false;

   is3D     = true;
   ReferenceDistance= 10.0;
   MaxDistance= 60.0;
   type     = $SimAudioType;
};

datablock AudioDescription(AudioClosest3d)
{
   volume   = 1.0;
   isLooping= false;

   is3D     = true;
   ReferenceDistance= 5.0;
   MaxDistance= 30.0;
   type     = $SimAudioType;
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
   type     = $SimAudioType;
};

datablock AudioDescription(AudioCloseLooping3d)
{
   volume   = 1.0;
   isLooping= true;

   is3D     = true;
   ReferenceDistance= 10.0;
   MaxDistance= 50.0;
   type     = $SimAudioType;
};

datablock AudioDescription(AudioClosestLooping3d)
{
   volume   = 1.0;
   isLooping= true;

   is3D     = true;
   ReferenceDistance= 5.0;
   MaxDistance= 30.0;
   type     = $SimAudioType;
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
   type = $SimAudioType;
};

// Used for Looping Environmental Sounds
datablock AudioDescription(AudioLooping2D)
{
   volume = 1.0;
   isLooping = true;
   is3D = false;
   type = $SimAudioType;
};


