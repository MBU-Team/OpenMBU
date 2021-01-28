//-----------------------------------------------------------------------------
// Torque Shader Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// Enter Chat Message Hud
//----------------------------------------------------------------------------

//------------------------------------------------------------------------------

function MessageHud::open(%this)
{
   %offset = 6;

   if(%this.isVisible())
      return;

   if(%this.isTeamMsg)
      %text = "TEAM:";
   else
      %text = "GLOBAL:";

   MessageHud_Text.setValue(%text);
   
   %windowPos = "0 " @ ( getWord( outerChatHud.position, 1 ) + getWord( outerChatHud.extent, 1 ) + 1 );
   %windowExt = getWord( OuterChatHud.extent, 0 ) @ " " @ getWord( MessageHud_Frame.extent, 1 );
   
   %textExtent = getWord(MessageHud_Text.extent, 0) + 14;
   %ctrlExtent = getWord(MessageHud_Frame.extent, 0);

   Canvas.pushDialog(%this);
   
   messageHud_Frame.position = %windowPos;
   messageHud_Frame.extent = %windowExt;
   MessageHud_Edit.position = setWord(MessageHud_Edit.position, 0, %textExtent + %offset);
   MessageHud_Edit.extent = setWord(MessageHud_Edit.extent, 0, %ctrlExtent - %textExtent - (2 * %offset));

   %this.setVisible(true);
   deactivateKeyboard();
   MessageHud_Edit.makeFirstResponder(true);
}

//------------------------------------------------------------------------------

function MessageHud::close(%this)
{
   if(!%this.isVisible())
      return;
      
   Canvas.popDialog(%this);
   %this.setVisible(false);
   if ( $enableDirectInput )
      activateKeyboard();
   MessageHud_Edit.setValue("");
}


//------------------------------------------------------------------------------

function MessageHud::toggleState(%this)
{
   if(%this.isVisible())
      %this.close();
   else
      %this.open();
}

//------------------------------------------------------------------------------

function MessageHud_Edit::onEscape(%this)
{
   MessageHud.close();
}

//------------------------------------------------------------------------------

function MessageHud_Edit::eval(%this)
{
   %text = trim(%this.getValue());
   if(%text !$= "")
   {
      if(MessageHud.isTeamMsg)
         commandToServer('teamMessageSent', %text);
      else
         commandToServer('messageSent', %text);
   }

   MessageHud.close();
}

   
//----------------------------------------------------------------------------
// MessageHud key handlers

function toggleMessageHud(%make)
{
   if(%make)
   {
      MessageHud.isTeamMsg = false;
      MessageHud.toggleState();
   }
}

function teamMessageHud(%make)
{
   if(%make)
   {
      MessageHud.isTeamMsg = true;
      MessageHud.toggleState();
   }
}
