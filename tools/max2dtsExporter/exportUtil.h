//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _EXPORT_UTIL_H_
#define _EXPORT_UTIL_H_

#pragma pack(push,8)
#include <MAX.H>
#pragma pack(pop)

#define ExportUtil_CLASS_ID   Class_ID(0x5959933e, 0x7673c15a)
extern ClassDesc * GetExportUtilDesc();
extern Interface * GetMaxInterface();

#endif // _EXPORT_UTIL_H_
