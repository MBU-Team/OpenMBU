//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _TELNETCONSOLE_H_
#define _TELNETCONSOLE_H_

#ifndef _CONSOLE_H_
#include "console/console.h"
#endif

/// Telnet admin console.
///
/// Torque supports remote access to its console. This is most useful when
/// running a dedicated server, as you can remotely administer the game
/// (for instance, kicking people). In the context of a MMORPG, this sort of
/// functionality would be useful for managing a server.
///
/// There are a number of products for Tribes2 which allow remote administration
/// via a nice GUI.
///
/// @section telnetconsole_use Using the Telnet Console
///
/// The TelnetConsole is designed to be used globally, so you don't instantiate
/// it like a normal class. Instead, you allow it to manage itself:
///
/// @code
///      // How to initialize the TelnetConsole.
///      TelnetConsole::create();
///
///      // How to shut down the TelnetConsole.
///      TelnetConsole::destroy();
/// @endcode
///
///
class TelnetConsole
{
   NetSocket mAcceptSocket;
   S32 mAcceptPort;

   enum {
      PasswordMaxLength = 32  ///< Maximum length of the telnet and listen passwords.
   };

   bool mRemoteEchoEnabled;
   char mTelnetPassword[PasswordMaxLength+1];
   char mListenPassword[PasswordMaxLength+1];
   ConsoleEvent mPostEvent;

   /// State of a TelnetClient.
   enum State
   {
      PasswordTryOne,      ///< Allow three password attempts.
      PasswordTryTwo,
      PasswordTryThree,
      DisconnectThisDude,  ///< If they've failed all three, disconnect them.
      FullAccessConnected, ///< They presented the telnetPassword, they get full access.
      ReadOnlyConnected    ///< They presented the listenPassword, they get read only access.
   };

   /// Represents a connection to the telnet console.
   ///
   /// This is also a linked list.
   struct TelnetClient
   {
      NetSocket socket;
      char curLine[Con::MaxLineLength];
      S32 curPos;
      S32 state;                       ///< State of the client.
                                       ///  @see TelnetConsole::State
      TelnetClient *nextClient;
   };
   TelnetClient *mClientList;
   TelnetConsole();
   ~TelnetConsole();

public:
   static void create();    ///< Initialize the telnet console.
   static void destroy();   ///< Shut down the telnet console.
   void process();          ///< Called by the main loop to let the console process commands
                            ///  and connections.

   /// Configure the parameter for the telnet console.
   ///
   /// @param    port           Port on which to listen for connections.
   /// @param    telnetPassword Password for full access to the console.
   /// @param    listenPassword Password for read-only access to the console.
   /// @param    remoteEcho     Enable/disable echoing input back to the client
   void setTelnetParameters(S32 port, const char *telnetPassword, const char *listenPassword, bool remoteEcho = false);

   /// Callback to handle a line from the console.
   ///
   /// @note This is used internally by the class; you
   ///       shouldn't need to call it.
   ///
   /// @see Con::addConsumer()
   void processConsoleLine(const char *line);
};

extern TelnetConsole *TelConsole;

#endif

