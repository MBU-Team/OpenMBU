//-----------------------------------------------------------------------------
// Torque Game Engine
// 
// Copyright (c) 2001 GarageGames.Com
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


//-----------------------------------------------------------------------------

function StateShape::trigger(%this,%mesg)
{
   // Punt over to our datablock
   %this.getDatablock().trigger(%this,%mesg);
}

