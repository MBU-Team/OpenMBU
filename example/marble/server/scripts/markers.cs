//-----------------------------------------------------------------------------
// Torque Game Engine
// 
// Copyright (c) 2001 GarageGames.Com
// Portions Copyright (c) 2001 by Sierra Online, Inc.
//-----------------------------------------------------------------------------

datablock MissionMarkerData(WayPointMarker)
{
   category = "Misc";
   shapeFile = "~/data/shapes/markers/octahedron.dts";
};

datablock MissionMarkerData(SpawnSphereMarker)
{
   category = "Misc";
   shapeFile = "~/data/shapes/markers/octahedron.dts";
};

datablock MissionMarkerData(GemSpawnSphereMarker)
{
   category = "Misc";
   shapeFile = "~/data/shapes/markers/octahedron.dts";
};

datablock MissionMarkerData(CameraSpawnSphereMarker)
{
   category = "Misc";
   shapeFile = "~/data/shapes/markers/octahedron.dts";
};


//------------------------------------------------------------------------------
// - serveral marker types may share MissionMarker datablock type
function MissionMarkerData::create(%block)
{
   switch$(%block)
   {
      case "WayPointMarker":
         %obj = new WayPoint() {
            dataBlock = %block;
         };
         return(%obj);
      case "SpawnSphereMarker":
         %obj = new SpawnSphere() {
            datablock = %block;
         };
         return(%obj);
      case "GemSpawnSphereMarker":
         %obj = new SpawnSphere() {
            datablock = %block;
         };
         return(%obj);
      case "CameraSpawnSphereMarker":
         %obj = new SpawnSphere() {
            datablock = %block;
         };
         return(%obj);
   }
   return(-1);
}

