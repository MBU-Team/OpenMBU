//-----------------------------------------------------------------------------
// Torque Shader Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

//------------------------------------------------------------------------------
function LoadingGui::onAdd(%this)
{
   %this.qLineCount = 0;
}

//------------------------------------------------------------------------------
function LoadingGui::onWake(%this)
{
   // Play sound...
   CloseMessagePopup();
   
   // Push the menu system
   Canvas.pushdialog(OverlayDlg);
   
   // Initialize overlay buttons
   OverlayQuitPage.setVisible(false);
   OverlayNextPage.setVisible(false);
   OverlayPrevPage.setVisible(true);
   OverlayPrevPage.command = "disconnect();";

   //
   LoadingProgressTxt.setValue("WAITING FOR SERVER");
}

//------------------------------------------------------------------------------
function LoadingGui::onSleep(%this)
{
   // Clear the load info:
   if ( %this.qLineCount !$= "" )
   {
      for ( %line = 0; %line < %this.qLineCount; %line++ )
         %this.qLine[%line] = "";
   }      
   %this.qLineCount = 0;

   LOAD_MapName.setText( "" );
   LOAD_MapDescription.setText( "" );
   LoadingProgress.setValue( 0 );

   // Stop sound...
}
