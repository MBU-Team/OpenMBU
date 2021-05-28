//-----------------------------------------------------------------------------
// Torque Shader Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------

function ServerPlay2D(%profile)
{
   // Play the given sound profile on every client.
   // The sounds will be transmitted as an event, not attached to any object.
   for(%idx = 0; %idx < ClientGroup.getCount(); %idx++)
      ClientGroup.getObject(%idx).play2D(%profile);
}

function ServerPlay3D(%profile,%transform)
{
   // Play the given sound profile at the given position on every client
   // The sound will be transmitted as an event, not attached to any object.
   for(%idx = 0; %idx < ClientGroup.getCount(); %idx++)
      ClientGroup.getObject(%idx).play3D(%profile,%transform);
}

