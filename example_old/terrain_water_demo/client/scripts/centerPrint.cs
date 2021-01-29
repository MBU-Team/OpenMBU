//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

$centerPrintActive = 0;
$bottomPrintActive = 0;

// Selectable window sizes
$CenterPrintSizes[1] = 20;
$CenterPrintSizes[2] = 36;
$CenterPrintSizes[3] = 56;

// time is specified in seconds
function clientCmdCenterPrint( %message, %time, %size )
{
   // if centerprint already visible, reset text and time.
   if ($centerPrintActive) {   
      if (centerPrintDlg.removePrint !$= "")
         cancel(centerPrintDlg.removePrint);
   }
   else {
      CenterPrintDlg.visible = 1;
      $centerPrintActive = 1;
   }

   CenterPrintText.setText( "<just:center>" @ %message );
   CenterPrintDlg.extent = firstWord(CenterPrintDlg.extent) @ " " @ $CenterPrintSizes[%size];
   
   if (%time > 0)
      centerPrintDlg.removePrint = schedule( ( %time * 1000 ), 0, "clientCmdClearCenterPrint" );
}

// time is specified in seconds
function clientCmdBottomPrint( %message, %time, %size )
{
   // if bottomprint already visible, reset text and time.
   if ($bottomPrintActive) {   
      if( bottomPrintDlg.removePrint !$= "")
         cancel(bottomPrintDlg.removePrint);
   }
   else {
      bottomPrintDlg.setVisible(true);
      $bottomPrintActive = 1;
   }
   
   bottomPrintText.setText( "<just:center>" @ %message );
   bottomPrintDlg.extent = firstWord(bottomPrintDlg.extent) @ " " @ $CenterPrintSizes[%size];

   if (%time > 0)
      bottomPrintDlg.removePrint = schedule( ( %time * 1000 ), 0, "clientCmdClearbottomPrint" );
}

function BottomPrintText::onResize(%this, %width, %height)
{
   %this.position = "0 0";
}

function CenterPrintText::onResize(%this, %width, %height)
{
   %this.position = "0 0";
}

//-------------------------------------------------------------------------------------------------------

function clientCmdClearCenterPrint()
{
   $centerPrintActive = 0;
   CenterPrintDlg.visible = 0;
   CenterPrintDlg.removePrint = "";
}

function clientCmdClearBottomPrint()
{
   $bottomPrintActive = 0;
   BottomPrintDlg.visible = 0;
   BottomPrintDlg.removePrint = "";
}
