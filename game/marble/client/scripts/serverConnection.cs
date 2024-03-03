//-----------------------------------------------------------------------------
// Torque Game Engine
// 
// Copyright (c) 2001 GarageGames.Com
// Portions Copyright (c) 2001 by Sierra Online, Inc.
//-----------------------------------------------------------------------------
// Functions dealing with connecting to a server

//-----------------------------------------------------------------------------
// Server connection error
//-----------------------------------------------------------------------------
addMessageCallback( 'MsgConnectionError', handleConnectionErrorMessage );
function handleConnectionErrorMessage(%msgType, %msgString, %msgError)
{
   // On connect the server transmits a message to display if there
   // are any problems with the connection.  Most connection errors
   // are game version differences, so hopefully the server message
   // will tell us where to get the latest version of the game.
   
   // JMQ: displaying text from a remote server is not localization-friendly.  Hardcode it to local version.
   //$ServerConnectionErrorMessage = %msgError;
   $ServerConnectionErrorMessage = $Text::ErrorCannotConnect;
}

//----------------------------------------------------------------------------
// GameConnection client callbacks
//----------------------------------------------------------------------------
function GameConnection::initialControlSet(%this)
{
   echo ("*** Initial Control Object");
   // The first control object has been set by the server
   // and we are now ready to go.
   
   // first check if the editor is active
   if (!Editor::checkActiveLoadDone())
      $LoadingDone = true;
      
   // we're loaded now, so render the real server
   if (RootGui.isAwake() && ServerConnection.gameState !$= "wait")
      RootGui.setContent(PlayGui);
}

function GameConnection::setLagIcon(%this, %state)
{
   if (%this.getAddress() $= "local")
      return;
   LagIcon.setVisible(%state $= "true");
}

function GameConnection::onConnectionAccepted(%this)
{
   // Called on the new connection object after connect() succeeds.
   LagIcon.setVisible(false);
   commandToServer('SetMarble', $pref::marbleIndex);
}

function GameConnection::onConnectionTimedOut(%this)
{
   // Called when an established connection times out
   RootGui.setContent(connErrorGui, $Text::ErrorConnectionLost);
}

function GameConnection::onConnectionDropped(%this, %msg)
{
   %this.dropped = true;
   // Established connection was dropped by the server
   if (%msg $= "CR_YOUAREKICKED")
      RootGui.setContent(connErrorGui, $Text::ErrorHostKickedYou, %msg);
   else if (%msg $= "ARB_FAILED")
      RootGui.setContent(connErrorGui, $Text::ErrorCannotConnect, %msg);
   else if (%msg $= "DEMO_OUTOFTIME")
   {
      $disconnectGui = "";
      disconnectedCleanup();
      UpsellGui.displayMPPostGame(false);
      UpsellGui.setBackToGui(MainMenuGui);
   }
   else
      RootGui.setContent(connErrorGui, $Text::ErrorHostKilled, %msg);
}

function GameConnection::onConnectionError(%this, %msg)
{
   // General connection error, usually raised by ghosted objects
   // initialization problems, such as missing files.  We'll display
   // the server's connection error message.
   RootGui.setContent(connErrorGui, $ServerConnectionErrorMessage, %msg);
}

//----------------------------------------------------------------------------
// Connection Failed Events
// These events aren't attached to a GameConnection object because they
// occur before one exists.
//----------------------------------------------------------------------------
function GameConnection::onConnectRequestRejected( %this, %msg )
{
   switch$(%msg)
   {
      case "CHR_PROTOCOL_LESS":
         %error = $Text::ErrorOutdatedClient;
      case "CHR_PROTOCOL_GREATER":
         %error = $Text::ErrorOutdatedServer;
      case "CR_INVALID_PROTOCOL_VERSION":
         %error = $Text::ErrorCannotConnect;
      case "CR_INVALID_CONNECT_PACKET":
         %error = $Text::ErrorSessionEnded;
      case "CR_YOUAREBANNED":
         %error = $Text::ErrorYouBanned;
      case "CR_YOUAREKICKED":
         %error = $Text::ErrorHostKickedYou;
      case "CR_SERVERFULL":
         %error = $Text::ErrorSessionGone;
      case "CR_DEMOUPGRADE":
         %error = $Text::ErrorDemoUpgrade;
      case "CR_DEMOREJECT":
         %error = $Text::ErrorCannotConnect;
      case "CHR_PASSWORD":
         // no password support on xbox
         %error = $Text::ErrorCannotConnect;
      case "CHR_PROTOCOL":
         %error = $Text::ErrorCannotConnect;
      case "CHR_CLASSCRC":
         %error = $Text::ErrorCannotConnect;
      case "CHR_INVALID_CHALLENGE_PACKET":
         %error = $Text::ErrorSessionEnded;
      default:
         %error = $Text::ErrorCannotConnect; 
   }

   RootGui.setContent(connErrorGui, %error, %msg);
}

function GameConnection::onConnectRequestTimedOut(%this)
{
   RootGui.setContent(connErrorGui, $Text::ErrorConnectionLost);
}

//------------------------------------------------------------------------------
// Grace peroid changes for patch -pw

function clearClientGracePeroid()
{
   if( isEventPending( $Client::graceSchedule ) )
      cancel( $Client::graceSchedule );
      
   $Client::isInGracePeroid = false;
   $Client::willfullDisconnect = false;
   
   echo( "Client (" @ $Player::ClientId @ ") grace peroid reset." );
}

function checkClientGracePeroid()
{
   $Client::isInGracePeroid = false;
   echo( "Client (" @ $Player::ClientId @ ") grace peroid check, NO LONGER IN GRACE PEROID!!." );
}

function resetClientGracePeroid()
{
   $Client::isInGracePeroid = true;
   
   if( isEventPending( $Client::graceSchedule ) )
      cancel( $Client::graceSchedule );
   
   schedule( $Client::gracePeroidMS, 0, "checkClientGracePeroid" );
   
   echo( "Client (" @ $Player::ClientId @ ") grace peroid reset, " @ $Client::gracePeroidMS @ " MS until reset." );
}

//-----------------------------------------------------------------------------
// Disconnect
//-----------------------------------------------------------------------------
function disconnect()
{
   // clean up stats sessions
   if (XBLiveIsStatsSessionActive())
   {
      // store the fact that I am dropping (may need it for ratings calculation)
      echo("disconnect(): I dropped");
      
      // Changed for patch -pw
      if( $Client::isInGracePeroid || XBLiveIsRanked() )
         clientAddDroppedClient($Player::ClientId, $Player::XBLiveId);
      else if( !$Client::isInGracePeroid && $Client::willfullDisconnect )
         zeroMyClientScoreCuzICheat();
      
      if ($Client::currentGameCounts)
         clientWriteMultiplayerScores();
      echo("disconnect(): Ending stats session and cleaning up stats");
      XBLiveEndStatsSession();
      clientCleanupStats();
   }

   // Delete the connection if it's still there.
   if (isObject(ServerConnection))
      ServerConnection.delete();
   disconnectedCleanup();
   // Call destroyServer in case we're hosting
   // JMQ: we don't destroy our local server - just leave it running
   //destroyServer();
}
function disconnectedCleanup()
{
   // clean up stats sessions
   if (XBLiveIsStatsSessionActive())
   {
      if (!$Server::Hosting)
      {
         echo("disconnectedCleanup(): host dropped me");
         // if the connection was dropped and the game was in play state, the host may have
         // ended the game early, so track that for ratings calculation
         if (isObject(ServerConnection) && ServerConnection.dropped &&
             ServerConnection.gameState $= "play")
            clientAddDroppedClient($Host::ClientId, $Host::XBLiveId);
      }
      
      if ($Client::currentGameCounts)
         clientWriteMultiplayerScores();
      echo("disconnectedCleanup(): Ending stats session and cleaning up stats");
      XBLiveEndStatsSession();
   }
   clientCleanupStats();
   
   if (isObject(StatsUserRatings_Level))
      StatsUserRatings_Level.clear();
   if (isObject(StatsUserRatings_Overall))
      StatsUserRatings_Overall.clear();
      
   // Clear misc script stuff
   HudMessageVector.clear();
   //if (isObject(MusicPlayer))
   //   MusicPlayer.stop();
   //
   LagIcon.setVisible(false);
   PlayerListGui.clear();
   LobbyGui.clear();
   
   // Clear all print messages
   clientCmdclearBottomPrint();
   clientCmdClearCenterPrint();
   
   if ($Server::Hosting)
   {
      // nothing special for host
   }
   else if ($Client::connectedMultiplayer)
   {
      XBLiveSetRichPresence(XBLiveGetSignInPort(), 0, "", "");
      XBLiveDisconnect();
      if (isObject($disconnectGui))
         RootGui.setContent($disconnectGui);
      $reconnectSched = schedule(0,0,connectToPreviewServer);
   }
   
   // re-enable Live if it was disabled
   XBLiveSetEnabled(true);
   
   // Dump anything we're not using
   clearTextureHolds();
   purgeResources();
   //stopDemo();
   
   unlockControler();
   
   // Call destroyServer in case we're hosting
   // JMQ: we don't destroy our local server - just leave it running
   //schedule(0,0,destroyServer);
}

function fmsHostCleanup()
{
   if ($EnableFMS)
   {
      // Delete everything
      if (isObject($ServerGroup))
         $ServerGroup.delete();
      $ServerGroup = new SimGroup(ServerGroup);
      
      if (isObject(MissionGroup))
         MissionGroup.delete();
      
      if (isObject(MissionCleanup))
         MissionCleanup.delete();
 
      // Flip back to SinglePlayerMode
      // The current mission should still be unhidden
      setSinglePlayerMode(true);
 
      // need to end the current game
      commandToServer('SetWaitState');
      
      // Make sure our camera gets set on the server
      GameMissionInfo.setCamera();
   }
}

function enterLobbyMode()
{
   // clean up our server if we are hosting
   if ($Server::Hosting)
   {
      allowConnections(true); // we may have turned off connections if in arbitrated mode
      
      cancel($Server::ArbSched);
      
      updateAvgPlayerCount();
      
      // clean up stats sessions
      if( XBLiveIsStatsSessionActive() && !XBLiveIsRanked() && ClientGroup.getCount() > 1 )
      {
         echo("enterLobbyMode(): Host ended game (considered a drop)");
         
         if( $Client::willfullDisconnect && !$Client::isInGracePeroid && !XBLiveIsRanked() )
            zeroMyClientScoreCuzICheat();
         else
            clientAddDroppedClient($Host::ClientId, $Host::XBLiveId);
         
         if ($Client::currentGameCounts)
            clientWriteMultiplayerScores();
         echo("enterLobbyMode(): Ending stats session and cleaning up stats");
         XBLiveEndStatsSession();
         clientCleanupStats();
      }
      
      destroyGame();
      
      fmsHostCleanup();
      
      // reset clients and tell them to go into lobby
      for (%i = 0; %i < ClientGroup.getCount(); %i++)
      {
         %client = ClientGroup.getObject(%i);
         %client.enterWaitState();
         messageClient(%client, 'MsgClientUpdateLobbyStatus', "", true );
      }
   }
}

function enterPreviewMode(%clientDropCode)
{
   // clean up our server if we are hosting
   if ($Server::Hosting)
   {
      cancel($Server::ArbSched);
      
      destroyGame();
      
      // tell everyone that I'm trashing the game
      // clean up stats sessions
      if (XBLiveIsStatsSessionActive())
      {
         echo("enterPreviewMode(): Host dropped");
         
         if( $Client::willfullDisconnect && !$Client::isInGracePeroid && !XBLiveIsRanked() && ClientGroup.getCount() > 1 )
            zeroMyClientScoreCuzICheat();
         else
            clientAddDroppedClient($Host::ClientId, $Host::XBLiveId);
         
         if ($Client::currentGameCounts)
            clientWriteMultiplayerScores();
         echo("enterPreviewMode(): Ending stats session and cleaning up stats");
         XBLiveEndStatsSession();
         clientCleanupStats();
      }
      
      // evict remote clients.  move local connection to front so that we don't delete it.
      ClientGroup.bringToFront(LocalClientConnection);
      while (ClientGroup.getCount() > 1)
      {
         %client = ClientGroup.getObject(1);
         %client.delete(%clientDropCode);
      }
      
      fmsHostCleanup();

      // reset local to wait state
      LocalClientConnection.enterWaitState();
      
      // clean up XBOX Live server records.  this must happen after we delete the client connections.
      // if we do it before, we will not be able to send the disconnect packets, 
      // and the clients will hang/timeout.
	   XBLiveDeleteHostedMatch();
      
      // clean up client side state from last game 
      disconnectedCleanup();
      
      stopMultiplayerMode();
      
      if (isObject($disconnectGui))
         RootGui.setContent($disconnectGui);
   }
}

function GameConnection::onConnectStatus(%this, %statusCode)
{
    if (RootGui.contentGui == MissionLoadingGui && !$Server::Hosting)
    {
        switch (%statusCode)
        {
            case 1:
                RootGui.setCenterText("Punching hole in NAT.");
            case 2:
                RootGui.setCenterText("Hole punching successful, trying to connect.");
            case 3:
                RootGui.setCenterText("Connecting..");
            case 4:
                RootGui.setCenterText("Trying relay.");
        }
    }
}