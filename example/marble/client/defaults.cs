//-----------------------------------------------------------------------------
// Torque Game Engine
// 
// Copyright (c) 2001 GarageGames.Com
//-----------------------------------------------------------------------------
// The master server is declared with the server defaults, which is
// loaded on both clients & dedicated servers.  If the server mod
// is not loaded on a client, then the master must be defined. 
//$pref::Master[0] = "2:v12master.dyndns.org:28002";
$pref::highScoreName = "Alex";
$pref::Player::Name = "Alex";
$pref::Player::defaultFov = 90;
$pref::Player::zoomSpeed = 0;
$pref::Net::LagThreshold = 400;
$pref::shadows = "2";
$pref::HudMessageLogSize = 40;
$pref::ChatHudLength = 1;
$pref::useStencilShadows = true;
$pref::Input::LinkMouseSensitivity = 1;
// DInput keyboard, mouse, and joystick prefs
$pref::Input::KeyboardEnabled = 1;
$pref::Input::MouseEnabled = 1;
$pref::Input::JoystickEnabled = 0;
$pref::Input::KeyboardTurnSpeed = 0.025;
$pref::Input::MouseSensitivity = 0.005;
$pref::Input::InvertYAxis = false;
$pref::Input::AlwaysFreeLook = false;
$pref::sceneLighting::cacheSize = 20000;
$pref::sceneLighting::purgeMethod = "lastCreated";
$pref::sceneLighting::cacheLighting = 1;
$pref::sceneLighting::terrainGenerateLevel = 1;
$pref::Terrain::DynamicLights = 1;
$pref::Interior::TexturedFog = 0;
// Note: folowing moved to example/main.cs because of moved canvas init
//$pref::Video::displayDevice = "D3D";
//$pref::Video::allowOpenGL = 1;
//$pref::Video::allowD3D = 1;
//$pref::Video::preferOpenGL = 1;
//$pref::Video::appliedPref = 0;
//$pref::Video::disableVerticalSync = 1;
//$pref::Video::monitorNum = 0;
//$pref::Video::resolution = "800 600 32";
//$pref::Video::wideScreen = false;
//$pref::Video::windowedRes = "1280 720";
//$pref::Video::fullScreen = "0";
$pref::OpenGL::force16BitTexture = "0";
$pref::OpenGL::forcePalettedTexture = "0";
$pref::OpenGL::maxHardwareLights = 3;
$pref::VisibleDistanceMod = 1.0;
$pref::Audio::driver = "OpenAL";
$pref::Audio::forceMaxDistanceUpdate = 0;
$pref::Audio::environmentEnabled = 0;
$pref::Audio::masterVolume   = 0.0;
$pref::Audio::channelVolume1 = 1.0;
$pref::Audio::channelVolume2 = 0.7;
$pref::Audio::channelVolume3 = 0.8;
$pref::Audio::channelVolume4 = 0.8;
$pref::Audio::channelVolume5 = 0.8;
$pref::Audio::channelVolume6 = 0.8;
$pref::Audio::channelVolume7 = 0.8;
$pref::Audio::channelVolume8 = 0.8;
$pref::Client::AutoStart = true;
$pref::Client::AutoStartMission = "marble/data/missions/beginner/Urban Jungle/urban.mis";
$pref::Client::AutoStartMissionIndex = 21;

$Client::MatchMode = 0; // standard, 1 = ranked
$Client::UseXBLiveMatchMaking = !isPCBuild();
//$Client::UseXBLiveMatchMaking = false;
$Option::DefaultMusicVolume = 35;
$Option::DefaultFXVolume = 100;
$pref::Option::MusicVolume = $Option::DefaultMusicVolume;
$pref::Option::FXVolume = $Option::DefaultFXVolume;

$Shell::gameModeFilter = 0; // currently not possible to wildcard game mode
$Shell::maxPlayersFilter = -1;

$Demo::DefaultStartingTime = 7 * 60 * 1000; // 7 minutes
$Demo::TimeRemaining = $Demo::DefaultStartingTime; 
//$Demo::TimeRemaining = 10 * 1000; // 10 seconds for testing