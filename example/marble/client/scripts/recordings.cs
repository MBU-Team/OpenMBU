//-----------------------------------------------------------------------------
// Torque Shader Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

function StartDemo( %id )
{
   %file = "marble/recordings/" @ %id @ ".rec";

   if( isObject(ServerConnection) )
   {
      RootTSCtrl.captureBackBuffer();
      Canvas.repaint();
      ServerConnection.delete();
   }
   
   new GameConnection(ServerConnection);
   RootGroup.add(ServerConnection);

   if(ServerConnection.playDemo(%file))
   {
      $playingDemo = true;
      ServerConnection.prepDemoPlayback();
   }
   else 
   {
      MessageBoxOK("Playback Failed", "Demo playback failed for file '" @ %file @ "'.");
      if (isObject(ServerConnection)) 
      {
         ServerConnection.delete();
      }
   }
}

function startDemoRecord()
{
   // make sure that current recording stream is stopped
   ServerConnection.stopRecording();
   
   // make sure we aren't playing a demo
   if(ServerConnection.isDemoPlaying())
      return;
   
   for(%i = 0; %i < 1000; %i++)
   {
      %num = %i;
      if(%num < 10)
         %num = "0" @ %num;
      if(%num < 100)
         %num = "0" @ %num;

      %file = "marble/recordings/demo" @ %num @ ".rec";
      if(!isfile(%file))
         break;
   }
   if(%i == 1000)
      return;

   $DemoFileName = %file;

   echo( "Recording to file [" @ $DemoFileName @ "].");

   ServerConnection.prepDemoRecord();
   ServerConnection.startRecording($DemoFileName);

   // make sure start worked
   if(!ServerConnection.isDemoRecording())
   {
      deleteFile($DemoFileName);
      error( "*** Failed to record to file [" @ $DemoFileName @ "].");
      $DemoFileName = "";
   }
}

function stopDemoRecord()
{
   // make sure we are recording
   if(ServerConnection.isDemoRecording())
   {
      error( "Recording file [" @ $DemoFileName @ "] finished.");
      ServerConnection.stopRecording();
   }
}

function demoPlaybackComplete()
{
   $playingDemo = false;
   $seeItPlayedDemo = false;
   
   RootTSCtrl.captureBackBuffer();
   Canvas.repaint();
   disconnect();
   createPreviewServer( "marble/data/missions/testMission.mis" );
}
