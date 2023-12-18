//-----------------------------------------------------------------------------
// Torque Shader Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// Functions that process commands sent from the server.


// This function is for chat messages only; it is invoked on the client when
// the server does a commandToClient with the tag ChatMessage.  (Cf. the
// functions chatMessage* in common/server/message.cs.)

// This just invokes onChatMessage, which the mod code must define.

function clientCmdChatMessage(%sender, %voice, %pitch, %msgString, %a1, %a2, %a3, %a4, %a5, %a6, %a7, %a8, %a9, %a10)
{
   onChatMessage(detag(%msgString), %voice, %pitch);
}


// Game event descriptions, which may or may not include text messages, can be
// sent using the message* functions in common/server/message.cs.  Those
// functions do commandToClient with the tag ServerMessage, which invokes the
// function below.

// For ServerMessage messages, the client can install callbacks that will be
// run, according to the "type" of the message.

function clientCmdServerMessage(%msgType, %msgString, %a1, %a2, %a3, %a4, %a5, %a6, %a7, %a8, %a9, %a10)
{
   // Get the message type; terminates at any whitespace.
   %tag = getWord(%msgType, 0);

   // First see if there is a callback installed that doesn't have a type;
   // if so, that callback is always executed when a message arrives.
   for (%i = 0; (%func = $MSGCB["", %i]) !$= ""; %i++) {
      call(%func, %msgType, %msgString, %a1, %a2, %a3, %a4, %a5, %a6, %a7, %a8, %a9, %a10);
   }

   // Next look for a callback for this particular type of ServerMessage.
   if (%tag !$= "") {
      for (%i = 0; (%func = $MSGCB[%tag, %i]) !$= ""; %i++) {
         call(%func, %msgType, %msgString, %a1, %a2, %a3, %a4, %a5, %a6, %a7, %a8, %a9, %a10);
      }
   }
}

// Called by the client to install a callback for a particular type of
// ServerMessage.
function addMessageCallback(%msgType, %func)
{
   for (%i = 0; (%afunc = $MSGCB[%msgType, %i]) !$= ""; %i++) {
      // If it already exists as a callback for this type,
      // nothing to do.
      if (%afunc $= %func) {
         return;
      }
   }
   // Set it up.
   $MSGCB[%msgType, %i] = %func;
}



// The following is the callback that will be executed for every ServerMessage,
// because we're going to install it without a specified type.  Any type-
// specific callbacks will be executed afterward.

// This just invokes onServerMessage, which the mod code must define.

function defaultMessageCallback(%msgType, %msgString, %a1, %a2, %a3, %a4, %a5, %a6, %a7, %a8, %a9, %a10)
{
   %msg = detag(%msgString);
   if (%msg !$= "" && %msg != 0)
      onServerMessage(%msg);
}

// Register that default message handler now.
addMessageCallback("", defaultMessageCallback);
