//-----------------------------------------------------------------------------
// Torque Shader Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Cursor Control
//------------------------------------------------------------------------------

$cursorControlled = true;

function cursorOff()
{
   if ( $cursorControlled )
      lockMouse(true);
   Canvas.cursorOff();
}

function cursorOn()
{
   if ( $cursorControlled )
      lockMouse(false);
   Canvas.cursorOn();
   Canvas.setCursor(DefaultCursor); 
}

// In the CanvasCursor package we add some additional functionality to the
// built-in GuiCanvas class, of which the global Canvas object is an instance.
// In this case, the behavior we want is for the cursor to automatically display,
// except when the only guis visible want no cursor - most notably,
// in the case of the example, the play gui.
// In order to override or extend an existing class, we use the script package
// feature.

package CanvasCursor
{

// The checkCursor method iterates through all the root controls on the canvas
// (basically the content control and any visible dialogs).  If all of them
// have the .noCursor attribute set, the cursor is turned off, otherwise it is
// turned on.

function GuiCanvas::checkCursor(%this)
{
   %cursorShouldBeOn = false;
   for(%i = 0; %i < %this.getCount(); %i++)
   {
      %control = %this.getObject(%i);
      if(%control.noCursor $= "")
      {
         %cursorShouldBeOn = true;
         break;
      }
   }
   if(%cursorShouldBeOn != %this.isCursorOn())
   {
      if(%cursorShouldBeOn)
         cursorOn();
      else
         cursorOff();
   }
}

// below, all functions which can alter which content controls are visible
// are extended to call the checkCursor function.  For package'd functions,
// the Parent:: call refers to the original implementation of that function,
// or a version that was declared in a previously activated package.
// In this case the parent calls should point to the built in versions
// of GuiCanvas functions.

function GuiCanvas::setContent(%this, %ctrl)
{
   Parent::setContent(%this, %ctrl);
   %this.checkCursor();
}

function GuiCanvas::pushDialog(%this, %ctrl, %layer)
{
   Parent::pushDialog(%this, %ctrl, %layer);
   %this.checkCursor();
}

function GuiCanvas::popDialog(%this, %ctrl)
{
   Parent::popDialog(%this, %ctrl);
   %this.checkCursor();
}

function GuiCanvas::popLayer(%this, %layer)
{
   Parent::popLayer(%this, %layer);
   %this.checkCursor();
}

};

// activate the package when the script loads.
activatePackage(CanvasCursor);
