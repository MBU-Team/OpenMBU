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
#define Offset(x, cls) offsetof(cls, x)
#endif

// Define Core Console Types
DefineConsoleType(TypeF32)
DefineConsoleType(TypeS8)
DefineConsoleType(TypeS32)
DefineConsoleType(TypeS32Vector)
DefineConsoleType(TypeBool)
DefineConsoleType(TypeBoolVector)
DefineConsoleType(TypeF32Vector)
DefineConsoleType(TypeString)
DefineConsoleType(TypeCaseString)
DefineConsoleType(TypeFilename)
DefineConsoleType(TypeEnum)
DefineConsoleType(TypeFlag)
DefineConsoleType(TypeColorI)
DefineConsoleType(TypeColorF)
DefineConsoleType(TypeSimObjectPtr)

DefineConsoleType(TypeShader)
DefineConsoleType(TypeCustomMaterial)
DefineConsoleType(TypeCubemap)
DefineConsoleType(TypeProjectileDataPtr)
DefineConsoleType(TypeParticleEmitterDataPtr);

DefineConsoleType(TypeGUID)

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
