//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _OBJECTTYPES_H_
#define _OBJECTTYPES_H_

#include "platform/types.h"

/// Types used for SimObject type masks (SimObject::mTypeMask)
///
/// @note If a new object type is added, don't forget to add it to the
///      consoleInit function in main.cpp.
enum SimObjectTypes
{
    /// @name Types used by the SceneObject class
    /// @{

    ///
    DefaultObjectType = 0,
    StaticObjectType = BIT(0),

    /// @}

    /// @name Basic Engine Types
    /// @{

    ///
    EnvironmentObjectType = BIT(1),
    TerrainObjectType = BIT(2),
    InteriorObjectType = BIT(3),
    WaterObjectType = BIT(4),
    TriggerObjectType = BIT(5),
    MarkerObjectType = BIT(6),
    AtlasObjectType = BIT(7),
    InteriorMapObjectType = BIT(8),
    DecalManagerObjectType = BIT(9),

    /// @}

    /// @name Game Types
    /// @{

    GameBaseObjectType = BIT(10),
    ShapeBaseObjectType = BIT(11),
    CameraObjectType = BIT(12),
    StaticShapeObjectType = BIT(13),
    PlayerObjectType = BIT(14),
    ItemObjectType = BIT(15),
    VehicleObjectType = BIT(16),
    VehicleBlockerObjectType = BIT(17),
    ProjectileObjectType = BIT(18),
    ExplosionObjectType = BIT(19),
    CorpseObjectType = BIT(20),
    DebrisObjectType = BIT(22),
    PhysicalZoneObjectType = BIT(23),
    StaticTSObjectType = BIT(24),
    AIObjectType = BIT(25),
    StaticRenderedObjectType = BIT(26),
    /// @}

    /// @name Other
    /// The following are allowed types that can be set on datablocks for static shapes
    /// @{

    ///
    DamagableItemObjectType = BIT(27),
    /// @}

    ShadowCasterObjectType = BIT(28)
};

#define STATIC_COLLISION_MASK   (   AtlasObjectType    | TerrainObjectType |  \
                                    InteriorObjectType | StaticObjectType  ) 

#define DAMAGEABLE_MASK  ( PlayerObjectType        |  \
                           VehicleObjectType       |  \
                           DamagableItemObjectType ) 

#endif
