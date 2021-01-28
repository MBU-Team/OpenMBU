//-----------------------------------------------------------------------------
// Torque Shader Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

function showSetSpeed(%speed)
{
   if(%speed)
      $showMovementSpeed = %speed;
}

function showMoveleft(%val)
{
   $showLeftAction = %val;
}

function showMoveright(%val)
{
   $showRightAction = %val;
}

function showMoveforward(%val)
{
   $showForwardAction = %val;
}

function showMovebackward(%val)
{
   $showBackwardAction = %val;
}

function showMoveup(%val)
{
   $showUpAction = %val;
}

function showMovedown(%val)
{
   $showDownAction = %val;
}

function showYaw(%val)
{
   $showYaw += %val * 0.01;
}

function showPitch(%val)
{
   $showPitch += %val * 0.01;
}

function toggleMouse()
{
   if(Canvas.isCursorOn())
      CursorOff();
   else
      CursorOn();
}

new ActionMap(showMoveMap);
showMoveMap.bind(keyboard, a, showMoveleft);
showMoveMap.bind(keyboard, d, showMoveright);
showMoveMap.bind(keyboard, w, showMoveforward);
showMoveMap.bind(keyboard, s, showMovebackward);
showMoveMap.bind(keyboard, e, showMoveup);
showMoveMap.bind(keyboard, c, showMovedown);
showMoveMap.bind(keyboard, z, showTurnLeft);
showMoveMap.bind(keyboard, x, showTurnRight);

showMoveMap.bind(keyboard, 1, S,  0.10, showSetSpeed);
showMoveMap.bind(keyboard, 2, S,  0.25, showSetSpeed);
showMoveMap.bind(keyboard, 3, S,  0.50, showSetSpeed);
showMoveMap.bind(keyboard, 4, S,  1.00, showSetSpeed);
showMoveMap.bind(keyboard, 5, S,  1.50, showSetSpeed);
showMoveMap.bind(keyboard, 6, S,  2.00, showSetSpeed);
showMoveMap.bind(keyboard, 7, S,  3.00, showSetSpeed);
showMoveMap.bind(keyboard, 8, S,  5.00, showSetSpeed);
showMoveMap.bind(keyboard, 9, S, 10.00, showSetSpeed);
showMoveMap.bind(keyboard, 0, S, 20.00, showSetSpeed);

showMoveMap.bind(mouse, xaxis, showYaw);
showMoveMap.bind(mouse, yaxis, showPitch);


//------------------------------------------------------------------------------
// Misc.
//------------------------------------------------------------------------------

GlobalActionMap.bind(keyboard, "tilde", toggleConsole);
GlobalActionMap.bindCmd(keyboard, "alt enter", "", "toggleFullScreen();");
