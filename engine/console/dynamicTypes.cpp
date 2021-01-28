//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------
#include "console/dynamicTypes.h"

// Init the globals.
ConsoleBaseType *ConsoleBaseType::smListHead = NULL;
S32              ConsoleBaseType::smConsoleTypeCount = 0;

// And, we also privately store the types lookup table.
VectorPtr<ConsoleBaseType*> gConsoleTypeTable;

ConsoleBaseType *ConsoleBaseType::getListHead()
{
   return smListHead;
}

void ConsoleBaseType::initialize()
{
   // Prep and empty the vector.
   gConsoleTypeTable.setSize(smConsoleTypeCount+1);
   dMemset(gConsoleTypeTable.address(), 0, sizeof(ConsoleBaseType*) * gConsoleTypeTable.size());

   // Walk the list and register each one with the console system.
   ConsoleBaseType *walk = getListHead();
   while(walk)
   {
      // Store a pointer to the type in the appropriate slot.
      const S32 id = walk->getTypeID();
      AssertFatal(gConsoleTypeTable[id]==NULL, "ConsoleBaseType::initialize - encountered a table slot that contained something!");
      gConsoleTypeTable[id] = walk;

      // Advance down the list...
      walk = walk->getListNext();
   }

   // Alright, we're all done here; we can now achieve fast lookups by ID.
}

ConsoleBaseType  *ConsoleBaseType::getType(const S32 typeID)
{
   return gConsoleTypeTable[typeID];
}
//-------------------------------------------------------------------------

ConsoleBaseType::ConsoleBaseType(const S32 size, S32 *idPtr, const char *aTypeName)
{
   // General initialization.
   mInspectorFieldType = NULL;

   // Store general info.
   mTypeSize = size;
   mTypeName = aTypeName;

   // Get our type ID and store it.
   mTypeID = smConsoleTypeCount++;
   *idPtr = mTypeID;

   // Link ourselves into the list.
   mListNext = smListHead;
   smListHead = this;

   // Alright, all done for now. Console initialization time code
   // takes us from having a list of general info and classes to
   // a fully initialized type table.
}

ConsoleBaseType::~ConsoleBaseType()
{
   // Nothing to do for now; we could unlink ourselves from the list, but why?
}