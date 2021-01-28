//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _CRC_H_
#define _CRC_H_

#define INITIAL_CRC_VALUE 0xffffffff

class Stream;

U32 calculateCRC(const void * buffer, S32 len, U32 crcVal = INITIAL_CRC_VALUE);
U32 calculateCRCStream(Stream *stream, U32 crcVal = INITIAL_CRC_VALUE);

#endif

