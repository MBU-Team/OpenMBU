//-----------------------------------------------------------------------------
// Torque Shader Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _CONVERT_H_
#define _CONVERT_H_

#include "interior/interiorMap.h"
#include "map2dif plus/csgBrush.h"
#include "map2dif plus/morianBasics.h"
#include "map2dif plus/editGeometry.h"
#include "map2dif plus/entityTypes.h"
#include "console/console.h"

void loadTextures(InteriorMap* map);
void convertInteriorMap(InteriorMap* map);
void getBrushes(InteriorMap* map);
void getWorldSpawn(InteriorMap* map);
void getEntities(InteriorMap* map);

#endif //_CONVERT_H_