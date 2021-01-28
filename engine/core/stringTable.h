//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _STRINGTABLE_H_
#define _STRINGTABLE_H_

#ifndef _PLATFORM_H_
#include "platform/platform.h"
#endif
#ifndef _DATACHUNKER_H_
#include "core/dataChunker.h"
#endif


//--------------------------------------
/// A global table for the hashing and tracking of strings.
///
/// Only one _StringTable is ever instantiated in Torque. It is accessible via the
/// global variable StringTable.
///
/// StringTable is used to manage strings in Torque. It performs the following tasks:
///      - Ensures that only one pointer is ever used for a given string (through
///        insert()).
///      - Allows the lookup of a string in the table.
///
/// @code
/// // Adding a string to the StringTable.
/// StringTableEntry mRoot;
/// mRoot = StringTable->insert(root);
///
/// // Looking up a string in the StringTable.
/// StringTableEntry stName = StringTable->lookupn(name, len);
///
/// // Comparing two strings in the StringTable (see below).
/// if(mRoot == stName) Con::printf("These strings are equal!");
/// @endcode
///
/// <b>But why is this useful, you ask?</b> Because every string that's run through the
/// StringTable is stored once and only once, every string has one and only one
/// pointer mapped to it. As a pointer is an integer value (usually an unsigned int),
/// so we can do several neat things:
///      - StringTableEntrys can be compared directly for equality, instead of using
///        the time-consuming dStrcmp() or dStricmp() function.
///      - For things like object names, we can avoid storing multiple copies of the
///        string containing the name. The StringTable ensures that we only ever store
///        one copy.
///      - When we're doing lookups by name (for instances, of resources), we can determine
///        if the object is even registered in the system by looking up its name in the
///        StringTable. Then, we can use the pointer as a hash key.
///
///  The scripting engine and the resource manager are the primary users of the
///  StringTable.
///
/// @note Be aware that the StringTable NEVER DEALLOCATES memory, so be careful when you
///       add strings to it. If you carelessly add many strings, you will end up wasting
///       space.
class _StringTable
{
private:
   /// @name Implementation details
   /// @{

   /// This is internal to the _StringTable class.
   struct Node
   {
      char *val;
      Node *next;
   };

   Node**      buckets;
   U32         numBuckets;
   U32         itemCount;
   DataChunker mempool;

  protected:
   static const U32 csm_stInitSize;

   _StringTable();
   ~_StringTable();

   /// @}
  public:

   /// Initialize StringTable.
   ///
   /// This is called at program start to initialize the StringTable global.
   static void create();

   /// Destroy the StringTable
   ///
   /// This is called at program end to destroy the StringTable global.
   static void destroy();

   /// Get a pointer from the string table, adding the string to the table
   /// if it was not already present.
   ///
   /// @param  string   String to check in the table (and add).
   /// @param  caseSens Determines whether case matters.
   StringTableEntry insert(const char *string, bool caseSens = false);

   /// Get a pointer from the string table, adding the string to the table
   /// if it was not already present.
   ///
   /// @param  string   String to check in the table (and add).
   /// @param  len      Length of the string in bytes.
   /// @param  caseSens Determines whether case matters.
   StringTableEntry insertn(const char *string, S32 len, bool caseSens = false);

   /// Get a pointer from the string table, NOT adding the string to the table
   /// if it was not already present.
   ///
   /// @param  string   String to check in the table (but not add).
   /// @param  caseSens Determines whether case matters.
   StringTableEntry lookup(const char *string, bool caseSens = false);

   /// Get a pointer from the string table, NOT adding the string to the table
   /// if it was not already present.
   ///
   /// @param  string   String to check in the table (but not add).
   /// @param  len      Length of string in bytes.
   /// @param  caseSens Determines whether case matters.
   StringTableEntry lookupn(const char *string, S32 len, bool caseSens = false);


   /// Resize the StringTable to be able to hold newSize items. This
   /// is called automatically by the StringTable when the table is
   /// full past a certain threshhold.
   ///
   /// @param newSize   Number of new items to allocate space for.
   void             resize(const U32 newSize);

   /// Hash a string into a U32.
   static U32 hashString(const char* in_pString);

   /// Hash a string of given length into a U32.
   static U32 hashStringn(const char* in_pString, S32 len);
};


extern _StringTable *StringTable;


#endif //_STRINGTABLE_H_

