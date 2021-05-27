//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Misc. server commands avialable to clients
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------

function serverCmdToggleCamera(%client)
{
   %control = %client.getControlObject();
   if (%control == %client.player)
   {
      %client.camera.setTransform(%client.player.getTransform());
      %control = %client.camera;
      %control.mode = toggleCameraFly;
   }
   else
   {
      %control = %client.player;
      %control.mode = observerFly;
   }
   %client.setControlObject(%control);

   // Load associated gui control, this should be transmitted to the client.
   if (%control.gui.getId() != 0)
      Canvas.setContent(%control.gui);
}

function serverCmdIdleCamera(%client,%idle)
{
   %control = %client.getControlObject();
   if ((%idle && %control == %client.idleCamera) ||
         (!%idle && %control != %client.idleCamera))
      return;

   // Toggle idle mode
   if (%control == %client.idleCamera)
      %control = %client.player;
   else
      %control = %client.idleCamera;
   %client.setControlObject(%control);

   // Load associated gui control, this should be transmitted to the client.
   if (%control.gui.getId() != 0)
      Canvas.setContent(%control.gui);
}

function serverCmdDropPlayerAtCamera(%client)
{
   if ($Server::TestCheats || isObject(EditorGui))
   {
      %client.player.setTransform(%client.camera.getTransform());
      %client.player.setVelocity("0 0 0");
      %client.setControlObject(%client.player);
   }
}

function serverCmdDropCameraAtPlayer(%client)
{
   %client.camera.setTransform(%client.player.getEyeTransform());
   %client.camera.setVelocity("0 0 0");
   %client.setControlObject(%client.camera);
}


//-----------------------------------------------------------------------------

function serverCmdSuicide(%client)
{
   if (isObject(%client.player))
      %client.player.kill("Suicide");
}

function serverCmdPlayCel(%client,%anim)
{
   if (isObject(%client.player))
      %client.player.playCelAnimation(%anim);
}

function serverCmdPlayDeath(%client)
{
   if (isObject(%client.player))
      %client.player.playDeathAnimation();
}
