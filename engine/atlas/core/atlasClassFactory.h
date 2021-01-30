//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------
#ifndef _ATLASCLASSFACTORY_
#define _ATLASCLASSFACTORY_

#include "atlas/core/atlasBaseTOC.h"

class AtlasTextureSchema;

/// Helper class to manage RTTI for a bunch of different possible fields.
///
/// This is pretty much suboptimal, but I don't really want to try to turn
/// my TOCs and so forth into ConsoleObject to use that system. So we have
/// this.
///
/// @ingroup AtlasCore
class AtlasClassFactory
{
public:
    /// Get a new instance of a TOC class given a name.
    static AtlasTOC* factoryTOC(const char* name);

    /// Get the name of a TOC class given an instance.
    static const char* getTOCName(AtlasTOC* toc);

    /// Map schema names to schema instances.
    static AtlasTextureSchema* factorySchema(const char* name);

    /// Map schema instances to schema names.
    static const char* getSchemaName(AtlasTextureSchema* schema);
};

#endif