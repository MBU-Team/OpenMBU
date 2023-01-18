//-----------------------------------------------------------------------------
// Torque Shader Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Loading info is text displayed on the client side while the mission
// is being loaded.  This information is extracted from the mission file
// and sent to each the client as it joins.
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// clearLoadInfo
//
// Clears the mission info stored
//------------------------------------------------------------------------------
function clearLoadInfo() {
   if (isObject(MissionInfo))
      MissionInfo.delete();
}

//------------------------------------------------------------------------------
// buildLoadInfo
//
// Extract the map description from the .mis file
//------------------------------------------------------------------------------
function buildLoadInfo( %mission ) {
	clearLoadInfo();

	%infoObject = "";
	%file = new FileObject();

	if ( %file.openForRead( %mission ) ) {
		%inInfoBlock = false;
		
		while ( !%file.isEOF() ) {
			%line = %file.readLine();
			%line = trim( %line );
			
			if( %line $= "new ScriptObject(MissionInfo) {" )
				%inInfoBlock = true;
			else if( %inInfoBlock && %line $= "};" ) {
				%inInfoBlock = false;
				%infoObject = %infoObject @ %line; 
				break;
			}
			
			if( %inInfoBlock )
			   %infoObject = %infoObject @ %line @ " ";
		}
		
		%file.close();
	}

   // Will create the object "MissionInfo"
	eval( %infoObject );
	%file.delete();
}

//------------------------------------------------------------------------------
// dumpLoadInfo
//
// Echo the mission information to the console
//------------------------------------------------------------------------------
function dumpLoadInfo()
{
	echo( "Mission Name: " @ getMissionNameFromNameVar(MissionInfo) );
   echo( "Mission Description:" );
   
   for( %i = 0; MissionInfo.desc[%i] !$= ""; %i++ )
      echo ("   " @ MissionInfo.desc[%i]);
}

//------------------------------------------------------------------------------
// sendLoadInfoToClient
//
// Sends mission description to the client
//------------------------------------------------------------------------------
function sendLoadInfoToClient( %client )
{
   messageClient( %client, 'MsgLoadInfo', "", MissionInfo.name );

   // Send Mission Description a line at a time
   for( %i = 0; MissionInfo.desc[%i] !$= ""; %i++ )
     messageClient( %client, 'MsgLoadDescripition', "", MissionInfo.desc[%i] );

   messageClient( %client, 'MsgLoadInfoDone' );
}
