//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _MATHTYPES_H_
#define _MATHTYPES_H_

#ifndef _DYNAMIC_CONSOLETYPES_H_
#include "console/dynamicTypes.h"
#endif

void RegisterMathFunctions(void);

// Define Math Console Types
DefineConsoleType( TypePoint2I )
DefineConsoleType( TypePoint2F )
DefineConsoleType( TypePoint3F )
DefineConsoleType( TypePoint4F )
DefineConsoleType( TypeRectI )
DefineConsoleType( TypeRectF )
DefineConsoleType( TypeMatrixPosition )
DefineConsoleType( TypeMatrixRotation )
DefineConsoleType( TypeBox3F )


#endif
