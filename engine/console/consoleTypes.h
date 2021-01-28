//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _CONSOLETYPES_H_
#define _CONSOLETYPES_H_

#ifndef _DYNAMIC_CONSOLETYPES_H_
#include "console/dynamicTypes.h"
#endif

#ifndef Offset
#if defined(TORQUE_COMPILER_GCC) && (__GNUC__ > 3) || ((__GNUC__ == 3) && (__GNUC_MINOR__ >= 1))
#define Offset(m,T) ((int)(&((T *)1)->m) - 1)
#else
#define Offset(x, cls) ((dsize_t)((const char *)&(((cls *)0)->x)-(const char *)0))
#endif
#endif

// Define Core Console Types
DefineConsoleType( TypeF32 )
DefineConsoleType( TypeS8 )
DefineConsoleType( TypeS32 )
DefineConsoleType( TypeS32Vector )
DefineConsoleType( TypeBool )
DefineConsoleType( TypeBoolVector )
DefineConsoleType( TypeF32Vector )
DefineConsoleType( TypeString )
DefineConsoleType( TypeCaseString )
DefineConsoleType( TypeFilename )
DefineConsoleType( TypeEnum )
DefineConsoleType( TypeFlag )
DefineConsoleType( TypeColorI )
DefineConsoleType( TypeColorF )
DefineConsoleType( TypeSimObjectPtr )

DefineConsoleType( TypeShader )
DefineConsoleType( TypeCustomMaterial )
DefineConsoleType( TypeCubemap )
DefineConsoleType( TypeProjectileDataPtr )
DefineConsoleType( TypeParticleEmitterDataPtr );

/*
   // Game types
   TypeGameBaseDataPtr,
   TypeExplosionDataPtr,
   TypeShockwaveDataPtr,
   TypeSplashDataPtr,
   TypeEnergyProjectileDataPtr,
   TypeBombProjectileDataPtr,
   TypeParticleEmitterDataPtr,
   TypeAudioDescriptionPtr,
   TypeAudioProfilePtr,
   TypeTriggerPolyhedron,
   TypeProjectileDataPtr,
   TypeCannedChatItemPtr,
   TypeWayPointTeam,
   TypeDebrisDataPtr,
   TypeCommanderIconDataPtr,
   TypeDecalDataPtr,
   TypeEffectProfilePtr,
   TypeAudioEnvironmentPtr,
   TypeAudioSampleEnvironmentPtr,
   TypeShader,
   TypeCustomMaterial,
   TypeCubemap,

   NumConsoleTypes
};

*/


#endif
