//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _VERSION_H_
#define _VERSION_H_

/// Gets the specified version number.  The version number is specified as a global in version.cc
U32 getVersionNumber();
/// Gets the version number in string form
const char* getVersionString();
/// Gets the compile date and time
const char* getCompileTimeString();

#endif
