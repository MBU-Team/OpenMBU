//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------
#include "atlas/core/atlasClassFactory.h"
#include "atlas/core/atlasBaseTOC.h"
#include "atlas/resource/atlasResourceGeomTOC.h"
#include "atlas/resource/atlasResourceTexTOC.h"
#include "atlas/resource/atlasResourceConfigTOC.h"


AtlasTOC* AtlasClassFactory::factoryTOC(const char* name)
{
    if (!dStricmp(name, "Geom"))
        return new AtlasResourceGeomTOC();
    else if (!dStricmp(name, "Tex"))
        return new AtlasResourceTexTOC();
    else if (!dStricmp(name, "Config"))
        return (new AtlasResourceConfigTOC())->castToResourceTOC();
    else
        return NULL;
}

const char* AtlasClassFactory::getTOCName(AtlasTOC* toc)
{
    if (dynamic_cast<AtlasResourceGeomTOC*>(toc))
        return "Geom";
    else if (dynamic_cast<AtlasResourceTexTOC*>(toc))
        return "Tex";
    else if (dynamic_cast<AtlasResourceConfigTOC*>(toc))
        return "Config";
    else
        return NULL;
}

AtlasTextureSchema* AtlasClassFactory::factorySchema(const char* name)
{
    AssertISV(false, "not currently implemented.");
    return NULL;
}

const char* AtlasClassFactory::getSchemaName(AtlasTextureSchema* schema)
{
    AssertISV(false, "not currently implemented.");
    return NULL;
}
