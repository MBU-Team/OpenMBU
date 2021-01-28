//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (c) 2002 GarageGames.Com
//-----------------------------------------------------------------------------

#ifndef _NETSTRINGTABLE_H_
#define _NETSTRINGTABLE_H_

#ifndef _DATACHUNKER_H_
#include "core/dataChunker.h"
#endif
#ifndef _CONSOLE_H_
#include "console/console.h"
#endif

class NetConnection;

class StringHandle;
extern U32 GameAddTaggedString(const char *string);

class NetStringTable
{
   friend class StringHandle;
   friend U32 GameAddTaggedString(const char *string);

#ifdef TORQUE_DEBUG_NET
   friend class RemoteCommandEvent;
#endif

   enum Constants {
      InitialSize = 16,
      InvalidEntry = 0xFFFFFFFF,
      HashTableSize = 2128,
      DataChunkerSize = 65536
   };
   struct Entry
   {
      char *string;
      U32 refCount;
      U32 scriptRefCount;
      U32 next;
      U32 link;
      U32 prevLink;
      U32 seq;
   };
   U32 size;
   U32 firstFree;
   U32 firstValid;
   U32 sequenceCount;

   Entry *table;
   U32 hashTable[HashTableSize];
   DataChunker *allocator;

    NetStringTable();
   ~NetStringTable();

   U32         addString(const char *string);

// XA: Moved this ones to public to avoid using the friend_ConsoleMethod hack.
public:
   const char *lookupString(U32 id);
   void        removeString(U32 id, bool script = false);
   void        incStringRefScript(U32 id);

private:
   void        incStringRef(U32 id);

   void repack();
public:
   static void create();
   static void destroy();

   static void expandString(StringHandle &string, char *buf, U32 bufSize, U32 argc, const char **argv);

#if defined(TORQUE_DEBUG)
   void dumpToConsole();
#endif // DEBUG
};

extern NetStringTable *gNetStringTable;

class StringHandle
{
   U32 index;
public:
   StringHandle() { index = 0; }
   StringHandle(const StringHandle &string) {
      index = string.index;
      if(index)
         gNetStringTable->incStringRef(index);
   }
   StringHandle(const char *string) {
      index = gNetStringTable->addString(string);
   }
   StringHandle(U32 initIndex)
   {
      index = initIndex;
      if(index)
         gNetStringTable->incStringRef(index);
   }
   ~StringHandle()
   {
      if(index)
         gNetStringTable->removeString(index);
   }

   void setFromIndex(U32 newIndex)
   {
      if(index)
         gNetStringTable->removeString(index);
      index = newIndex;
   }

   bool operator==(const StringHandle &s) const { return index == s.index; }
   bool operator!=(const StringHandle &s) const { return index != s.index; }

   StringHandle &operator=(const StringHandle &s)
   {
      if(index)
         gNetStringTable->removeString(index);
      index = s.index;
      if(index)
         gNetStringTable->incStringRef(index);
     return *this;
   }
   const char *getString()
   {
      if(index)
         return gNetStringTable->lookupString(index);
      else
         return NULL;
   }
   bool isNull() { return index == 0; }
   bool isValidString() { return index != 0; }
   U32 getIndex() { return index; }
};

#endif
