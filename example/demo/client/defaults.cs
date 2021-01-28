//-----------------------------------------------------------------------------
// Torque Shader Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

// The master server is declared with the server defaults, which is
// loaded on both clients & dedicated servers.  If the server mod
// is not loaded on a client, then the master must be defined. 
// $pref::Master[0] = "2:master.garagegames.com:28002";

$pref::Player::Name = "Visitor";
$pref::Player::defaultFov = 90;
$pref::Player::zoomSpeed = 0;

$pref::Net::LagThreshold = 400;
$pref::Net::Port = 28000;

$pref::shadows = "2";
$pref::HudMessageLogSize = 40;
$pref::ChatHudLength = 1;


$pref::Input::LinkMouseSensitivity = 1;
$pref::Input::MouseEnabled = 0;
$pref::Input::JoystickEnabled = 0;
$pref::Input::KeyboardTurnSpeed = 0.1;

$pref::sceneLighting::cacheSize = 20000;
$pref::sceneLighting::purgeMethod = "lastCreated";
$pref::sceneLighting::cacheLighting = 1;
$pref::sceneLighting::terrainGenerateLevel = 1;

$pref::ts::detailAdjust = 0.45;

$pref::Terrain::DynamicLights = 1;
$pref::Interior::TexturedFog = 0;

$pref::Video::displayDevice = "D3D";
$pref::Video::allowOpenGL = 0;
$pref::Video::allowD3D = 1;
$pref::Video::preferOpenGL = 0;
$pref::Video::appliedPref = 0;
$pref::Video::disableVerticalSync = 1;
$pref::Video::monitorNum = 0;
$pref::Video::fullScreen = "0";
$Pref::Video::Resolution = "800 600 32";
$pref::Video::forcePixVersion = 0;
$pref::Video::forcedPixVersion = 1.1;
$pref::Video::shaderPath = "shaders/procedural";
$pref::Video::killFramesAhead = true;
$pref::Video::disableCubemapping = 0;
$pref::Video::disableNormalmapping = 0;
$pref::Video::disablePixSpecular = 0;

$pref::Video::screenShotSession = 0;
$pref::Video::screenShotFormat = "PNG";

$pref::OpenGL::force16BitTexture = "0";
$pref::OpenGL::forcePalettedTexture = "0";
$pref::OpenGL::maxHardwareLights = 3;
$pref::VisibleDistanceMod = 1.0;

$pref::Audio::driver = "OpenAL";
$pref::Audio::forceMaxDistanceUpdate = 0;
$pref::Audio::environmentEnabled = 0;
$pref::Audio::masterVolume   = 0.8;
$pref::Audio::channelVolume1 = 0.8;
$pref::Audio::channelVolume2 = 0.8;
$pref::Audio::channelVolume3 = 0.8;
$pref::Audio::channelVolume4 = 0.8;
$pref::Audio::channelVolume5 = 0.8;
$pref::Audio::channelVolume6 = 0.8;
$pref::Audio::channelVolume7 = 0.8;
$pref::Audio::channelVolume8 = 0.8;

$pref::Editor::visibleDistance = "1000";

$pref::LightManager::sgAtlasMaxDynamicLights = "16";
$pref::LightManager::sgDynamicShadowDetailSize = "0";
$pref::LightManager::sgDynamicShadowQuality = "0";
$pref::LightManager::sgLightingProfileAllowShadows = "1";
$pref::LightManager::sgLightingProfileQuality = "0";
$pref::LightManager::sgMaxBestLights = "10";
$pref::LightManager::sgMultipleDynamicShadows = "0";
$pref::LightManager::sgShowCacheStats = "0";
$pref::LightManager::sgUseBloom = "";
$pref::LightManager::sgUseDRLHighDynamicRange = "0";
$pref::LightManager::sgUseDynamicRangeLighting = "0";
$pref::LightManager::sgUseDynamicShadows = "1";
$pref::LightManager::sgUseToneMapping = "";

$pref::TextureManager::qualityMode = "0";
$pref::TextureManager::reductionLevel = "1";
$pref::TextureManager::scaleThreshold = "134217729";
