//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

function centerPrintAll( %message, %time, %lines )
{
   if( %lines $= "" || ((%lines > 3) || (%lines < 1)) )
      %lines = 1;
   
   %count = ClientGroup.getCount();
   for (%i = 0; %i < %count; %i++)
	{
		%cl = ClientGroup.getObject(%i);
      if( !%cl.isAIControlled() )
         commandToClient( %cl, 'centerPrint', %message, %time, %lines );
   }
}

function bottomPrintAll( %message, %time, %lines )
{
   if( %lines $= "" || ((%lines > 3) || (%lines < 1)) )
      %lines = 1;
   
   %count = ClientGroup.getCount();
	for (%i = 0; %i < %count; %i++)
	{
		%cl = ClientGroup.getObject(%i);
      if( !%cl.isAIControlled() )
         commandToClient( %cl, 'bottomPrint', %message, %time, %lines );
   }
}

//-------------------------------------------------------------------------------------------------------

function centerPrint( %client, %message, %time, %lines )
{
   if( %lines $= "" || ((%lines > 3) || (%lines < 1)) )
      %lines = 1;
      
   
   commandToClient( %client, 'CenterPrint', %message, %time, %lines );
}

function bottomPrint( %client, %message, %time, %lines )
{
   if( %lines $= "" || ((%lines > 3) || (%lines < 1)) )
      %lines = 1;

   commandToClient( %client, 'BottomPrint', %message, %time, %lines );
}

//-------------------------------------------------------------------------------------------------------

function clearCenterPrint( %client )
{
   commandToClient( %client, 'ClearCenterPrint');
}

function clearBottomPrint( %client )
{
   commandToClient( %client, 'ClearBottomPrint');
}

//-------------------------------------------------------------------------------------------------------

function clearCenterPrintAll()
{
	%count = ClientGroup.getCount();
	for (%i = 0; %i < %count; %i++)
	{
		%cl = ClientGroup.getObject(%i);
      if( !%cl.isAIControlled() )
         commandToClient( %cl, 'ClearCenterPrint');
   }
}

function clearBottomPrintAll()
{
	%count = ClientGroup.getCount();
	for (%i = 0; %i < %count; %i++)
	{
		%cl = ClientGroup.getObject(%i);
      if( !%cl.isAIControlled() )
         commandToClient( %cl, 'ClearBottomPrint');
   }
}