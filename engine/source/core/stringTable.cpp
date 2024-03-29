//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "platform/platform.h"
#include "core/stringTable.h"

_StringTable* StringTable = NULL;
const U32 _StringTable::csm_stInitSize = 29;

//---------------------------------------------------------------
//
// StringTable functions
//
//---------------------------------------------------------------

namespace {
    bool sgInitTable = true;
    U8   sgHashTable[256];

    void initTolowerTable()
    {
        for (U32 i = 0; i < 256; i++) {
            U8 c = dTolower(i);
            sgHashTable[i] = c * c;
        }

        sgInitTable = false;
    }

} // namespace {}

U32 _StringTable::hashString(const char* str)
{
    if (sgInitTable)
        initTolowerTable();

    if (!str) return -1;

    U32 ret = 0;
    char c;
    while ((c = *str++) != 0) {
        ret <<= 1;
        ret ^= sgHashTable[static_cast<U8>(c)];
    }
    return ret;
}

U32 _StringTable::hashStringn(const char* str, S32 len)
{
    if (sgInitTable)
        initTolowerTable();

    U32 ret = 0;
    char c;
    while ((c = *str++) != 0 && len--) {
        ret <<= 1;
        ret ^= sgHashTable[c];
    }
    return ret;
}

//--------------------------------------
_StringTable::_StringTable()
{
    buckets = (Node**)dMalloc(csm_stInitSize * sizeof(Node*));
    for (U32 i = 0; i < csm_stInitSize; i++) {
        buckets[i] = 0;
    }

    numBuckets = csm_stInitSize;
    itemCount = 0;
}

//--------------------------------------
_StringTable::~_StringTable()
{
    dFree(buckets);
}


//--------------------------------------
void _StringTable::create()
{
    AssertFatal(StringTable == NULL, "StringTable::create: StringTable already exists.");
    StringTable = new _StringTable;
}


//--------------------------------------
void _StringTable::destroy()
{
    AssertFatal(StringTable != NULL, "StringTable::destroy: StringTable does not exist.");
    delete StringTable;
    StringTable = NULL;
}


//--------------------------------------
StringTableEntry _StringTable::insert(const char* val, const bool  caseSens)
{
    Node** walk, * temp;
    U32 key = hashString(val);
    walk = &buckets[key % numBuckets];
    while ((temp = *walk) != NULL) {
        if (caseSens && !dStrcmp(temp->val, val))
            return temp->val;
        else if (!caseSens && !dStricmp(temp->val, val))
            return temp->val;
        walk = &(temp->next);
    }
    char* ret = 0;
    if (!*walk) {
        *walk = (Node*)mempool.alloc(sizeof(Node));
        (*walk)->next = 0;
        (*walk)->val = (char*)mempool.alloc(dStrlen(val) + 1);
        dStrcpy((*walk)->val, val);
        ret = (*walk)->val;
        itemCount++;
    }
    if (itemCount > 2 * numBuckets) {
        resize(4 * numBuckets - 1);
    }
    return ret;
}

//--------------------------------------
StringTableEntry _StringTable::insertn(const char* src, S32 len, const bool  caseSens)
{
    char val[256];
    AssertFatal(len < 255, "Invalid string to insertn");
    dStrncpy(val, src, len);
    val[len] = 0;
    return insert(val, caseSens);
}

//--------------------------------------
StringTableEntry _StringTable::lookup(const char* val, const bool  caseSens)
{
    Node** walk, * temp;
    U32 key = hashString(val);
    walk = &buckets[key % numBuckets];
    while ((temp = *walk) != NULL) {
        if (caseSens && !dStrcmp(temp->val, val))
            return temp->val;
        else if (!caseSens && !dStricmp(temp->val, val))
            return temp->val;
        walk = &(temp->next);
    }
    return NULL;
}

//--------------------------------------
StringTableEntry _StringTable::lookupn(const char* val, S32 len, const bool  caseSens)
{
    Node** walk, * temp;
    U32 key = hashStringn(val, len);
    walk = &buckets[key % numBuckets];
    while ((temp = *walk) != NULL) {
        if (caseSens && !dStrncmp(temp->val, val, len) && temp->val[len] == 0)
            return temp->val;
        else if (!caseSens && !dStrnicmp(temp->val, val, len) && temp->val[len] == 0)
            return temp->val;
        walk = &(temp->next);
    }
    return NULL;
}

//--------------------------------------
void _StringTable::resize(const U32 newSize)
{
    Node* head = NULL, * walk, * temp;
    U32 i;
    // reverse individual bucket lists
    // we do this because new strings are added at the end of bucket
    // lists so that case sens strings are always after their
    // corresponding case insens strings

    for (i = 0; i < numBuckets; i++) {
        walk = buckets[i];
        while (walk)
        {
            temp = walk->next;
            walk->next = head;
            head = walk;
            walk = temp;
        }
    }
    buckets = (Node**)dRealloc(buckets, newSize * sizeof(Node));
    for (i = 0; i < newSize; i++) {
        buckets[i] = 0;
    }
    numBuckets = newSize;
    walk = head;
    while (walk) {
        U32 key;
        Node* temp = walk;

        walk = walk->next;
        key = hashString(temp->val);
        temp->next = buckets[key % newSize];
        buckets[key % newSize] = temp;
    }
}