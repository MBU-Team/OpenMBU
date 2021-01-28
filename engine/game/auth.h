//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _AUTH_H_
#define _AUTH_H_

#ifndef _EVENT_H_
#include "platform/event.h"
#endif

/// Formerly contained a certificate, showing that something was valid.
class Auth2Certificate
{
   U32 xxx;
};

/// Formerly contained data indicating whether a user is valid.
struct AuthInfo
{
   enum {
      MaxNameLen = 31,
   };

   bool valid;
   char name[MaxNameLen + 1];
};

/// Formerly validated the server's authentication info.
inline bool validateAuthenticatedServer()
{
   return true;
}

/// Formerly validated the client's authentication info.
inline bool validateAuthenticatedClient()
{
   return true;
}

#endif
