//-----------------------------------------------------------------------------
// Torque Shader Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

function MessageCallback(%dlg,%callback)
{
   Canvas.popDialog(%dlg);
   eval(%callback);
}

// MBSetText resizes the message window, based on the change in size of the text
// area.

function MBSetText(%text, %frame, %msg)
{
   %ext = %text.getExtent();

   %text.setText("<just:center>" @ %msg);
   %text.forceReflow();

   %newExtent = %text.getExtent();

   %deltaY = getWord(%newExtent, 1) - getWord(%ext, 1);
   %windowPos = %frame.getPosition();
   %windowExt = %frame.getExtent();

   %frame.resize(getWord(%windowPos, 0), getWord(%windowPos, 1) - (%deltaY / 2), getWord(%windowExt, 0), getWord(%windowExt, 1) + %deltaY);
}

//-----------------------------------------------------------------------------
// MessageBox OK
//-----------------------------------------------------------------------------

function MessageBoxOK( %title, %message, %callback )
{
	MBOKFrame.setText( %title );
   Canvas.pushDialog( MessageBoxOKDlg );
   MBSetText(MBOKText, MBOKFrame, %message);
   MessageBoxOKDlg.callback = %callback;
}

//------------------------------------------------------------------------------
function MessageBoxOKDlg::onSleep( %this )
{
   %this.callback = "";
}

//------------------------------------------------------------------------------
// MessageBox OK/Cancel dialog:
//------------------------------------------------------------------------------

function MessageBoxOKCancel( %title, %message, %callback, %cancelCallback )
{
	MBOKCancelFrame.setText( %title );
   Canvas.pushDialog( MessageBoxOKCancelDlg );
   MBSetText(MBOKCancelText, MBOKCancelFrame, %message);
	MessageBoxOKCancelDlg.callback = %callback;
	MessageBoxOKCancelDlg.cancelCallback = %cancelCallback;
}

//------------------------------------------------------------------------------
function MessageBoxOKCancelDlg::onSleep( %this )
{
   %this.callback = "";
}

//------------------------------------------------------------------------------
// MessageBox Yes/No dialog:
//------------------------------------------------------------------------------

function MessageBoxYesNo( %title, %message, %yesCallback, %noCallback )
{
	MBYesNoFrame.setText( %title );
   Canvas.pushDialog( MessageBoxYesNoDlg );
   MBSetText(MBYesNoText, MBYesNoFrame, %message);
	MessageBoxYesNoDlg.yesCallBack = %yesCallback;
	MessageBoxYesNoDlg.noCallback = %noCallBack;
}

//------------------------------------------------------------------------------
function MessageBoxYesNoDlg::onSleep( %this )
{
   %this.yesCallback = "";
   %this.noCallback = "";
}

//------------------------------------------------------------------------------
// Message popup dialog:
//------------------------------------------------------------------------------

function MessagePopup( %title, %message, %delay )
{
   // Currently two lines max.
   MessagePopFrame.setText( %title );
   Canvas.pushDialog( MessagePopupDlg );
   MBSetText(MessagePopText, MessagePopFrame, %message);
   if ( %delay !$= "" )
      schedule( %delay, 0, CloseMessagePopup );
}

//------------------------------------------------------------------------------

function CloseMessagePopup()
{
   Canvas.popDialog( MessagePopupDlg );
}

