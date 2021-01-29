//-----------------------------------------------------------------------------
// Torque Shader Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

//----------------------------------------
function StartMissionGui::onWake(%this)
{
   SM_missionList.clear();
   %i = 0;
   for(%file = findFirstFile($Server::MissionFileSpec); %file !$= ""; %file = findNextFile($Server::MissionFileSpec))  
      if (strStr(%file, "/CVS/") == -1)
         SM_missionList.addRow(%i++, getMissionDisplayName(%file) @ "\t" @ %file );
   SM_missionList.sort(0);
   SM_missionList.setSelectedRow(0);
   SM_missionList.scrollVisible(0);

   // Override the next page command...
   OverlayNextPage.command = "StartMissionGui.nextPage();";
}


//----------------------------------------
function StartMissionGui::nextPage(%this)
{
   if (StartMissionGuiCheck.getValue()) {
      Canvas.setContent(LoadingGui);
	   LOAD_MapName.setText( "Creating Server" );
   	LOAD_MapDescription.setText( "<font:Arial:16>Please wait while the local server is started... this server will be open to local lan and internet connections.");
      Canvas.repaint();
      %this.startServer();
   }
   else
      MainMenuGui.nextPage();
}

function StartMissionGui::startServer(%this)
{
   createServer("MultiPlayer", "demo/data/missions/"@$Client::GameType@".mis");
   %conn = new GameConnection(ServerConnection);
   RootGroup.add(ServerConnection);
   %conn.setConnectArgs($pref::Player::Name);
   %conn.setJoinPassword($Client::Password);
   %conn.connectLocal();
}


//----------------------------------------
function StartMissionGuiText::onWake(%this)
{
   // Indicate which text is to be displayed in the ML text control...
   %this.fileName = "demo/client/ui/missions/start_" @ $Client::GameType @ ".txt";
   echo("FileName: " @ %this.fileName);
   Parent::onWake(%this);
}


//----------------------------------------
function getMissionDisplayName( %missionFile ) 
{
   %file = new FileObject();
   
   %MissionInfoObject = "";
   
   if ( %file.openForRead( %missionFile ) ) {
		%inInfoBlock = false;
		
		while ( !%file.isEOF() ) {
			%line = %file.readLine();
			%line = trim( %line );
			
			if( %line $= "new ScriptObject(MissionInfo) {" )
				%inInfoBlock = true;
			else if( %inInfoBlock && %line $= "};" ) {
				%inInfoBlock = false;
				%MissionInfoObject = %MissionInfoObject @ %line; 
				break;
			}
			
			if( %inInfoBlock )
			   %MissionInfoObject = %MissionInfoObject @ %line @ " "; 	
		}
		
		%file.close();
	}
	%MissionInfoObject = "%MissionInfoObject = " @ %MissionInfoObject;
	eval( %MissionInfoObject );
	
   %file.delete();

   if( %MissionInfoObject.name !$= "" )
      return %MissionInfoObject.name;
   else
      return fileBase(%missionFile); 
}

