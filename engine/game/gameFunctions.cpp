//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "sim/sceneObject.h"
#include "core/fileStream.h"

void RegisterGameFunctions();

// query
SimpleQueryList gServerQueryList;
U32 gServerQueryIndex = 0;

//SERVER FUNCTIONS ONLY
ConsoleFunctionGroupBegin( Containers, "Spatial query functions. <b>Server side only!</b>");

ConsoleFunction(containerFindFirst, const char*, 6, 6, "(bitset type, Point3F point, float x, float y, float z)"
                "Find objects matching the bitmask type within a box centered at point, with extents x, y, z.\n\n"
                "Returns the first object found; thereafter, you can get more results using containerFindNext().")
{
   //find out what we're looking for
   U32 typeMask = U32(dAtoi(argv[1]));

   //find the center of the container volume
   Point3F origin(0, 0, 0);
   dSscanf(argv[2], "%g %g %g", &origin.x, &origin.y, &origin.z);

   //find the box dimensions
   Point3F size(0, 0, 0);
   size.x = mFabs(dAtof(argv[3]));
   size.y = mFabs(dAtof(argv[4]));
   size.z = mFabs(dAtof(argv[5]));

   //build the container volume
   Box3F queryBox;
   queryBox.min = origin;
   queryBox.max = origin;
   queryBox.min -= size;
   queryBox.max += size;

   //initialize the list, and do the query
   gServerQueryList.mList.clear();
   gServerContainer.findObjects(queryBox, typeMask, SimpleQueryList::insertionCallback, &gServerQueryList);

   //return the first element
   gServerQueryIndex = 0;
   char *buff = Con::getReturnBuffer(100);
   if (gServerQueryList.mList.size())
      dSprintf(buff, 100, "%d", gServerQueryList.mList[gServerQueryIndex++]->getId());
   else
      buff[0] = '\0';

   return buff;
}

ConsoleFunction( containerFindNext, const char*, 1, 1, "Get more results from a previous call to containerFindFirst().")
{
   //return the next element
   char *buff = Con::getReturnBuffer(100);
   if (gServerQueryIndex < gServerQueryList.mList.size())
      dSprintf(buff, 100, "%d", gServerQueryList.mList[gServerQueryIndex++]->getId());
   else
      buff[0] = '\0';

   return buff;
}

ConsoleFunctionGroupEnd( Containers );
