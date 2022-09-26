//-----------------------------------------------------------------------------
// Torque Engine
// 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

// Channel assignments (channel 0 is unused in-game).
$GuiAudioType     = 1;
$SimAudioType     = 1;
$MessageAudioType = 1;
$EffectAudioType = 1;
$MusicAudioType = 2;
$MusicVolume = 1;
$AudioChannelTypesDefined = 1;

/// Causes the system to do a one time autodetect
/// of an SFX provider and device at startup if the
/// provider is unset.
$SFX::autoDetect = true;

/// This initializes the sound system device from
/// the defaults in the $pref::SFX:: globals.
function sfxStartup()
{
   sfxShutdown();

   // Make sure we at least have a provider.
   if ( $pref::SFX::provider $= "" )
   {
      // If enabled autodetect a safe device.
      if ( $SFX::autoDetect )
         sfxAutodetect();
               
      return;
   }

   echo( "sfxStartup..." );

   // Start it up!
   %maxBuffers = $pref::SFX::useHardware ? -1 : $pref::SFX::maxSoftwareBuffers;
   if ( !sfxCreateDevice( $pref::SFX::provider, $pref::SFX::device, $pref::SFX::useHardware, %maxBuffers ) )
   {
      error( "   Failed to initialize device!\n\n" );
      $pref::SFX::provider = "";
      $pref::SFX::device   = "";
      
      if ( $SFX::autoDetect )
         sfxAutodetect();
         
      return;
   }

   // This returns a tab seperated string with
   // the initialized system info.
   %info = sfxGetDeviceInfo();
   $pref::SFX::provider       = getField( %info, 0 );
   $pref::SFX::device         = getField( %info, 1 );
   $pref::SFX::useHardware    = getField( %info, 2 );
   %useHardware               = $pref::SFX::useHardware ? "Yes" : "No";
   %maxBuffers                = getField( %info, 3 );
   
   echo( "   Provider: "    @ $pref::SFX::provider );
   echo( "   Device: "      @ $pref::SFX::device );
   echo( "   Hardware: "    @ %useHardware );
   echo( "   Buffers: "      @ %maxBuffers );

   sfxSetMasterVolume( $pref::SFX::masterVolume );

   for ( %channel = 1; %channel <= 8; %channel++ ) 
      sfxSetChannelVolume( %channel, $pref::SFX::channelVolume[ %channel ] );
}


/// Destroys the current sound system device.
function sfxShutdown()
{
   // We're assuming here that a null info 
   // string means that no device is loaded.
   if ( sfxGetDeviceInfo() $= "" )
      return;

   sfxDeleteDevice();
}


function sfxAutodetect()
{
   // We only want this to happen once.
   $SFX::autoDetect = false;   
   
   // Clear whatever previous provider 
   // and device we had.   
   $pref::SFX::provider = "";
   $pref::SFX::device = "";
      
   // Get all the available devices.
   %devices = sfxGetAvailableDevices();

   // Loop thru the found devices.
   %count = getRecordCount( %devices );
   for ( %i = 0; %i < %count; %i++ )
   {
      %info = getRecord( %devices, %i );

      // Skip the null provider.
      %provider = getField( %info, 0 );
      if ( stricmp( %provider, "null" ) == 0 )
         continue;
  
      // This is the one we'll use.
      $pref::SFX::provider       = %provider;
      $pref::SFX::device         = getField( %info, 1 );
      $pref::SFX::useHardware    = getField( %info, 2 );
      break;         
   }
      
   // By default we've decided to avoid hardware devices as
   // they are buggy and prone to problems.
   $pref::SFX::useHardware = false;
      
   // Try the newly found device.
   sfxStartup();
}


if ( !isObject( SFXPausedSet ) )
{
   /// The default paused source SimSet.
   new SimSet( SFXPausedSet );
   RootGroup.add( SFXPausedSet );
}


/// Pauses the playback of active sound sources.
/// 
/// @param %channels    An optional word list of channel indices or an empty 
///                     string to pause sources on all channels.
/// @param %pauseSet    An optional SimSet which is filled with the paused 
///                     sources.  If not specified the global SfxSourceGroup 
///                     is used.
///
function sfxPause( %channels, %pauseSet )
{
   // Did we get a set to populate?
   if ( !isObject( %pauseSet ) )
      %pauseSet = SFXPausedSet;
      
   %count = SFXSourceSet.getCount();
   for ( %i = 0; %i < %count; %i++ )
   {
      %source = SFXSourceSet.getObject( %i );

      %channel = %source.getChannel();
      if ( %channels $= "" || findWord( %channels, %channel ) != -1 )
      {
         %source.pause();
         %pauseSet.add( %source );
      }
   }
}


/// Resumes the playback of paused sound sources.
/// 
/// @param %pauseSet    An optional SimSet which contains the paused sound 
///                     sources to be resumed.  If not specified the global 
///                     SfxSourceGroup is used.
///
function sfxResume( %pauseSet )
{
   if ( !isObject( %pauseSet ) )
      %pauseSet = SFXPausedSet;
                  
   %count = %pauseSet.getCount();
   for ( %i = 0; %i < %count; %i++ )
   {
      %source = %pauseSet.getObject( %i );
      // Make sure the sound is actually paused, otherwise the sound will replay!
      if (%source.isPaused())
         %source.play();
   }

   // Clear our pause set... the caller is left
   // to clear his own if he passed one.
   %pauseSet.clear();
}
