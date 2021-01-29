//-----------------------------------------------------------------------------
// Torque Shader Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

//----------------------------------------
function JoinServerGui::onWake()
{
   // Double check the status. Tried setting this the control
   // inactive to start with, but that didn't seem to work.
   //JS_joinServer.setActive(JS_serverList.rowCount() > 0);
   
   
   // Override the next page command...
   OverlayNextPage.setVisible(true);
   OverlayNextPage.command = "JoinServerGui.join();";
   
   // Set our query and mission types to the demo game type
   $Client::GameTypeQuery = $Client::GameType;
   $Client::MissionTypeQuery = "any";
}

//----------------------------------------
function JoinServerGui::query(%this)
{
   queryMasterServer(
      0,          // Query flags
      $Client::GameTypeQuery,       // gameTypes
      $Client::MissionTypeQuery,    // missionType
      0,          // minPlayers
      100,        // maxPlayers
      0,          // maxBots
      2,          // regionMask
      0,          // maxPing
      100,        // minCPU
      0           // filterFlags
      );
}

//----------------------------------------
function JoinServerGui::queryLan(%this)
{
   queryLANServers(
      28000,      // lanPort for local queries
      0,          // Query flags
      $Client::GameTypeQuery,       // gameTypes
      $Client::MissionTypeQuery,    // missionType
      0,          // minPlayers
      100,        // maxPlayers
      0,          // maxBots
      2,          // regionMask
      0,          // maxPing
      100,        // minCPU
      0           // filterFlags
      );
}

//----------------------------------------
function JoinServerGui::cancel(%this)
{
   cancelServerQuery();
   JS_queryStatus.setVisible(false);
}


//----------------------------------------
function JoinServerGui::join(%this)
{
   cancelServerQuery();
   %id = JS_serverList.getSelectedId();

   // The server info index is stored in the row along with the
   // rest of displayed info.
   %index = getField(JS_serverList.getRowTextById(%id),6);
   if (setServerInfo(%index)) {
      %conn = new GameConnection(ServerConnection);
      %conn.setConnectArgs($pref::Player::Name);
      %conn.setJoinPassword($Client::Password);
      %conn.connect($ServerInfo::Address);
   }
   else
      MessageBoxOk("No Server Selected","You need to select a server. Press either of the Query buttons to list available servers or go back to the previous page to create your own.");
}

//----------------------------------------
function JoinServerGui::exit(%this)
{
   cancelServerQuery();
   Canvas.setContent(StartMissionGui);   
}

//----------------------------------------
function JoinServerGui::update(%this)
{
   // Copy the servers into the server list.
   JS_queryStatus.setVisible(false);
   JS_serverList.clear();
   %sc = getServerCount();
   for (%i = 0; %i < %sc; %i++) {
      setServerInfo(%i);
      JS_serverList.addRow(%i,
         $ServerInfo::Name TAB
         $ServerInfo::Ping TAB
         $ServerInfo::PlayerCount @ "/" @ $ServerInfo::MaxPlayers TAB
         %i);  // ServerInfo index stored also
   }
   JS_serverList.sort(0);
   JS_serverList.setSelectedRow(0);
   JS_serverList.scrollVisible(0);

   //JS_joinServer.setActive(JS_serverList.rowCount() > 0);
} 

//----------------------------------------
function onServerQueryStatus(%status, %msg, %value)
{
   // Update query status
   // States: start, update, ping, query, done
   // value = % (0-1) done for ping and query states
   if (!JS_queryStatus.isVisible())
      JS_queryStatus.setVisible(true);

   switch$ (%status) {
      case "start":
         //JS_joinServer.setActive(false);
         JS_queryMaster.setActive(false);
         JS_statusText.setText(%msg);
         JS_statusBar.setValue(0);
         JS_serverList.clear();

      case "ping":
         JS_statusText.setText("Ping Servers");
         JS_statusBar.setValue(%value);

      case "query":
         JS_statusText.setText("Query Servers");
         JS_statusBar.setValue(%value);

      case "done":
         JS_queryMaster.setActive(true);
         JS_queryStatus.setVisible(false);
         JoinServerGui.update();
   }
}

