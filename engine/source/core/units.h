//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _COREUNITS_H_
#define _COREUNITS_H_

#include "platform/platform.h"

extern const char* getUnit(const char* string, U32 index, const char* set);
extern const char* getUnits(const char* string, S32 startIndex, S32 endIndex, const char* set);
extern U32 getUnitCount(const char* string, const char* set);
extern const char* setUnit(const char* string, U32 index, const char* replace, const char* set);
extern const char* removeUnit(const char* string, U32 index, const char* set);
extern U32 findUnit(const char* string, const char* find, const char* set);

#endif // _COREUNITS_H_