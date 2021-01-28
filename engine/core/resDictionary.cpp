//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "platform/platform.h"
#include "core/resManager.h"
#include "core/tAlgorithm.h"

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

ResDictionary::ResDictionary()
{
   entryCount = 0;
   hashTableSize = 1023; //DefaultTableSize;
   hashTable = new ResourceObject *[hashTableSize];
   S32 i;
   for(i = 0; i < hashTableSize; i++)
      hashTable[i] = NULL;
}

ResDictionary::~ResDictionary()
{
   // we assume the resources are purged before we destroy
   // the dictionary

   delete[] hashTable;
}

S32 ResDictionary::hash(StringTableEntry path, StringTableEntry file)
{
   return ((S32)((((dsize_t)path) >> 2) + (((dsize_t)file) >> 2) )) % hashTableSize;
}

void ResDictionary::insert(ResourceObject *obj, StringTableEntry path, StringTableEntry file)
{
   obj->name = file;
   obj->path = path;

   S32 idx = hash(path, file);
   obj->nextEntry = hashTable[idx];
   hashTable[idx] = obj;
   entryCount++;

   if(entryCount > hashTableSize) {
      ResourceObject *head = NULL, *temp, *walk;
      for(idx = 0; idx < hashTableSize;idx++) {
         walk = hashTable[idx];
         while(walk)
         {
            temp = walk->nextEntry;
            walk->nextEntry = head;
            head = walk;
            walk = temp;
         }
      }
      delete[] hashTable;
      hashTableSize = 2 * hashTableSize - 1;
      hashTable = new ResourceObject *[hashTableSize];
      for(idx = 0; idx < hashTableSize; idx++)
         hashTable[idx] = NULL;
      walk = head;
      while(walk)
      {
         temp = walk->nextEntry;
         idx = hash(walk);
         walk->nextEntry = hashTable[idx];
         hashTable[idx] = walk;
         walk = temp;
      }
   }
}

ResourceObject* ResDictionary::find(StringTableEntry path, StringTableEntry name)
{
   for(ResourceObject *walk = hashTable[hash(path, name)]; walk; walk = walk->nextEntry)
      if(walk->name == name && walk->path == path)
         return walk;
   return NULL;
}

ResourceObject* ResDictionary::find(StringTableEntry path, StringTableEntry name, StringTableEntry zipPath, StringTableEntry zipName)
{
   for(ResourceObject *walk = hashTable[hash(path, name)]; walk; walk = walk->nextEntry)
      if(walk->name == name && walk->path == path && walk->zipName == zipName && walk->zipPath == zipPath)
         return walk;
   return NULL;
}

ResourceObject* ResDictionary::find(StringTableEntry path, StringTableEntry name, U32 flags)
{
   for(ResourceObject *walk = hashTable[hash(path, name)]; walk; walk = walk->nextEntry)
      if(walk->name == name && walk->path == path && U32(walk->flags) == flags)
         return walk;
   return NULL;
}

void ResDictionary::pushBehind(ResourceObject *resObj, S32 flagMask)
{
   remove(resObj);
   entryCount++;
   ResourceObject **walk = &hashTable[hash(resObj)];
   for(; *walk; walk = &(*walk)->nextEntry)
   {
      if(!((*walk)->flags & flagMask))
      {
         resObj->nextEntry = *walk;
         *walk = resObj;
         return;
      }
   }
   resObj->nextEntry = NULL;
   *walk = resObj;
}

void ResDictionary::remove(ResourceObject *resObj)
{
   for(ResourceObject **walk = &hashTable[hash(resObj)]; *walk; walk = &(*walk)->nextEntry)
   {
      if(*walk == resObj)
      {
         entryCount--;
         *walk = resObj->nextEntry;
         return;
      }
   }
}
