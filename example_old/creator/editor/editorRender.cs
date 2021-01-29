//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Console onEditorRender functions:
//------------------------------------------------------------------------------
// Functions:
//   - renderSphere([pos], [radius], <sphereLevel>);
//   - renderCircle([pos], [normal], [radius], <segments>);
//   - renderTriangle([pnt], [pnt], [pnt]);
//   - renderLine([start], [end], <thickness>);
//
// Variables:
//   - consoleFrameColor - line prims are rendered with this
//   - consoleFillColor
//   - consoleSphereLevel - level of polyhedron subdivision
//   - consoleCircleSegments
//   - consoleLineWidth
//------------------------------------------------------------------------------

function SpawnSphere::onEditorRender(%this, %editor, %selected, %expanded)
{
   if(%selected $= "true")
   {
      %editor.consoleFrameColor = "255 0 0";
      %editor.consoleFillColor = "0 15 0 15";
      %editor.renderSphere(%this.getWorldBoxCenter(), %this.radius, 1);
   }
}

//function Item::onEditorRender(%this, %editor, %selected, %expanded)
//{
//   if(%this.getDataBlock().getName() $= "MineDeployed")
//   {
//      %editor.consoleFillColor = "0 0 0 0";
//      %editor.consoleFrameColor = "255 0 0";
//      %editor.renderSphere(%this.getWorldBoxCenter(), 6, 1);
//   }
//}