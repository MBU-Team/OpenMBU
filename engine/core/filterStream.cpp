//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "core/filterStream.h"

FilterStream::~FilterStream()
{
   //
}

bool FilterStream::_read(const U32 in_numBytes, void* out_pBuffer)
{
   AssertFatal(getStream() != NULL, "Error no stream to pass to");

   bool success = getStream()->read(in_numBytes, out_pBuffer);

   setStatus(getStream()->getStatus());
   return success;
}


bool FilterStream::_write(const U32, const void*)
{
   AssertFatal(false, "No writing allowed to filter");
   return false;
}

bool FilterStream::hasCapability(const Capability in_streamCap) const
{
   // Fool the compiler.  We know better...
   FilterStream* ncThis = const_cast<FilterStream*>(this);
   AssertFatal(ncThis->getStream() != NULL, "Error no stream to pass to");

   return ncThis->getStream()->hasCapability(in_streamCap);
}

U32 FilterStream::getPosition() const
{
   // Fool the compiler.  We know better...
   FilterStream* ncThis = const_cast<FilterStream*>(this);
   AssertFatal(ncThis->getStream() != NULL, "Error no stream to pass to");

   return ncThis->getStream()->getPosition();
}

bool FilterStream::setPosition(const U32 in_newPosition)
{
   AssertFatal(getStream() != NULL, "Error no stream to pass to");

   return getStream()->setPosition(in_newPosition);
}

U32 FilterStream::getStreamSize()
{
   AssertFatal(getStream() != NULL, "Error no stream to pass to");

   return getStream()->getStreamSize();
}

