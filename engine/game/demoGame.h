//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _TORQUEGAME_H_
#define _TORQUEGAME_H_

#ifndef _GAMEINTERFACE_H_
#include "platform/gameInterface.h"
#endif

/// Implementation of GameInterface for the demo app.
class DemoGame : public GameInterface
{
public:
   void textureKill();
   void textureResurrect();
   void refreshWindow();

   int main(int argc, const char **argv);

   void processPacketReceiveEvent(PacketReceiveEvent *event);
   void processMouseMoveEvent(MouseMoveEvent *event);
   void processInputEvent(InputEvent *event);
   void processQuitEvent();
   void processTimeEvent(TimeEvent *event);
   void processConsoleEvent(ConsoleEvent *event);
   void processConnectedAcceptEvent(ConnectedAcceptEvent *event);
   void processConnectedReceiveEvent(ConnectedReceiveEvent *event);
   void processConnectedNotifyEvent(ConnectedNotifyEvent *event);
};

#endif
