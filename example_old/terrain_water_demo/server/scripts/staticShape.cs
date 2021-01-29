//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Hook into the mission editor.

function StaticShapeData::create(%data)
{
   // The mission editor invokes this method when it wants to create
   // an object of the given datablock type.
   %obj = new StaticShape() {
      dataBlock = %data;
   };
   return %obj;
}


