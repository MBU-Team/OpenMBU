//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------
#include "core/stringTable.h"
#include "atlas/resource/atlasConfigChunk.h"
#include "util/fourcc.h"

AtlasConfigChunk::AtlasConfigChunk()
{
    mEntryList = NULL;
}

AtlasConfigChunk::~AtlasConfigChunk()
{
    // Clear the list.
    DictEntry* walk = mEntryList;
    while (walk)
    {
        DictEntry* tmp = walk;
        walk = walk->nextEntry;
        delete tmp;
    }
}

void AtlasConfigChunk::process()
{
    // Do nothing; this ain't no GFX object!
}

U32 AtlasConfigChunk::getHeadSentinel()
{
    return MAKEFOURCC('C', 'f', 'g', 'H');
}

U32 AtlasConfigChunk::getTailSentinel()
{
    return MAKEFOURCC('C', 'f', 'g', 'T');
}

void AtlasConfigChunk::write(Stream* s)
{
    Parent::write(s);

    // Walk the list, serialize our entries. Get a count first.

    U32 entCount = 0;
    DictEntry* w = mEntryList;

    while (w)
        entCount++, w = w->nextEntry;
    s->write(entCount);

    // Reset and write out specific data.
    w = mEntryList;
    while (w)
    {
        s->writeLongString(4096, w->key);
        s->writeLongString(4096, w->val);
        w = w->nextEntry;
    }
}

void AtlasConfigChunk::read(Stream* s)
{
    Parent::read(s);

    U32 entCount = 0;
    s->read(&entCount);

    char keyBuff[4097], valBuff[4097];

    for (U32 i = 0; i < entCount; i++)
    {
        s->readLongString(4096, keyBuff);
        s->readLongString(4096, valBuff);
        setEntry(keyBuff, valBuff);
    }
}

bool AtlasConfigChunk::getEntry(const char* key, const char*& v)
{
    DictEntry* walk = mEntryList;
    while (walk)
    {
        if (!dStricmp(walk->key, key))
        {
            v = walk->val;
            return true;
        }

        walk = walk->nextEntry;
    }

    return false;
}

void AtlasConfigChunk::setEntry(const char* key, const char* v)
{
    // Walk the list and on match, reset.
    DictEntry* walk = mEntryList;
    while (walk)
    {
        if (walk->key == key)
        {
            walk->val = StringTable->insert(v, true);
            return;
        }

        walk = walk->nextEntry;
    }

    // Otherwise add a new entry.
    walk = new DictEntry;
    walk->key = StringTable->insert(key, true);
    walk->val = StringTable->insert(v, true);

    walk->nextEntry = mEntryList;
    mEntryList = walk;
}

bool AtlasConfigChunk::clearEntry(const char* key)
{
    DictEntry** walk = &mEntryList;
    while (*walk)
    {
        if ((*walk)->key == key)
        {
            // Delete and exit.
            DictEntry* tmp = *walk;
            *walk = (*walk)->nextEntry;
            delete tmp;
            return true;
        }

        // Consider next one.
        walk = &(*walk)->nextEntry;
    }

    //  No hit, ret 
    return false;
}

AtlasConfigChunk* AtlasConfigChunk::generateCopy(S32 reformat)
{
    AssertISV(false, "AtlasConfigChunk::generateCopy - not implemented atm.");
    return NULL;
}