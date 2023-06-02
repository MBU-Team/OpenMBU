//-----------------------------------------------------------------------------
// Torque Game Engine
// 
// Copyright (c) 2001 GarageGames.Com
// Portions Copyright (c) 2001 by Sierra Online, Inc.
//-----------------------------------------------------------------------------

// List of master servers to query, each one is tried in order
// until one responds
$Server::RegionMask = 2;
$Server::Master[0] = "2:master.openmbu.com:28002"; //"2:master.garagegames.com:28002";

$Server::DisplayOnMaster = "Always"; //"Never"; // Xbox should not use GG master server
// Information about the server
$Pref::Server::missionId = 0;
$Pref::Server::Name = "MB2 Test Server";
$Pref::Server::Info = "This is a MB2 Test Server.";

// The connection error message is transmitted to the client immediatly
// on connection, if any further error occures during the connection
// process, such as network traffic mismatch, or missing files, this error
// message is display. This message should be replaced with information
// usefull to the client, such as the url or ftp address of where the
// latest version of the game can be obtained.
$Pref::Server::ConnectionError = $Text::ErrorCannotConnect;

// The network port is also defined by the client, this value 
// overrides pref::net::port for dedicated servers
$Pref::Server::Port = 28000;

// If the password is set, clients must provide it in order
// to connect to the server
$Pref::Server::Password = "";

// Password for admin clients
$Pref::Server::AdminPassword = "";

// Misc server settings.
$Pref::Server::TimeLimit = 20;               // In minutes
$Pref::Server::KickBanTime = 300;            // specified in seconds
$Pref::Server::BanTime = 1800;               // specified in seconds
$Pref::Server::FloodProtectionEnabled = 1;
$Pref::Server::MaxChatLen = 120;


$Server::AbsMaxPlayers = 8;
$Server::HighBandwidthMin = 100000; // minimum kilo bits per second up and down required for "high bandwidth" (5,6 players with no warning)
$Server::MaxPlayers_LowBandwidth = 5;
$Server::MaxPlayers_HighBandwidth = $Server::AbsMaxPlayers;
$Pref::Server::MaxPlayers = 5; //$Server::MaxPlayers_HighBandwidth; // CHANGED FOR PATCH!!

$Server::GemGroupRadius = 20;
$Server::MaxGemsPerGroup = 4;

$Server::BandwidthLimit[0] = "131072 6"; // at least 128kbps for 7+ players
$Server::BandwidthLimit[1] = "98394 4"; // at least 96kbps for 5+ players
$Server::NumBandwidthLimits = 2;