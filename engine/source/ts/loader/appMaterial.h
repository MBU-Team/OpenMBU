//-----------------------------------------------------------------------------
// TS Shape Loader
// Copyright (C) 2008 GarageGames.com, Inc.
//-----------------------------------------------------------------------------
#pragma once
#include <string>
typedef std::string String;
#ifndef _MATERIAL_H_
#include "materials/matInstance.h"
#endif
#include <ts/collada/colladaUtils.h>

struct AppMaterial
{
   U32 flags;
   F32 reflectance;

   AppMaterial() : flags(0), reflectance(1.0f) { }
   virtual ~AppMaterial() {}
   
   virtual String getName() { return "unnamed"; }
   virtual U32 getFlags() { return flags; }
   virtual F32 getReflectance() { return reflectance; }
};

