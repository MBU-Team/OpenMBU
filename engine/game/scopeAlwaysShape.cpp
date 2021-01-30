//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "game/staticShape.h"
#include "game/gameConnection.h"

class ScopeAlwaysShape : public StaticShape
{
    typedef StaticShape Parent;

public:
    ScopeAlwaysShape();
    static void initPersistFields();
    DECLARE_CONOBJECT(ScopeAlwaysShape);
};

ScopeAlwaysShape::ScopeAlwaysShape()
{
    mNetFlags.set(Ghostable | ScopeAlways);
    mTypeMask |= ShadowCasterObjectType;
}

void ScopeAlwaysShape::initPersistFields()
{
    Parent::initPersistFields();
}

IMPLEMENT_CO_NETOBJECT_V1(ScopeAlwaysShape);
