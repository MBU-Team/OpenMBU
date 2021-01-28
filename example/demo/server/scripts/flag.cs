//-----------------------------------------------------------------------------
// Torque Shader Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

datablock StaticShapeData(FlagPole)
{
   category = "Demo";
   shapeFile = "~/data/shapes/objects/flagpole.dts";
};

datablock ShapeBaseImageData(FlagPoleImage)
{
   shapeFile = "~/data/shapes/objects/flagpole.dts";
   mountPoint = 1;
   offset = "0 0 -1";
   emap = false;
};

