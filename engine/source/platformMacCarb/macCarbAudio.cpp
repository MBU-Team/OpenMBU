//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "platformMacCarb/platformMacCarb.h"
#include "platform/platformAL.h"

namespace Audio
{

/*!  The MacOS X build links against the OpenAL framework.
     It can be built to use either an internal framework, or the system framework.
     Since OpenAL is weak-linked in at compile time, we don't need to init anything.
     Stub it out...
*/
bool OpenALDLLInit() {  return true; }

/*!   Stubbed out, see the note on OpenALDLLInit().  
*/
void OpenALDLLShutdown() { }   


} // namespace Audio
