//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------
#ifndef _ATLASCONFIGCHUNK_H_
#define _ATLASCONFIGCHUNK_H_

#include "atlas/core/atlasChunk.h"

/// A dictionary of configuration data.
///
/// This is a simple dictionary class to store configuration data in an
/// Atlas file. For now it is a dictionary of strings.
///
/// @ingroup AtlasResource
class AtlasConfigChunk : public AtlasChunk
{
    struct DictEntry
    {
        const char* key;
        const char* val;
        DictEntry* nextEntry;
    };

    DictEntry* mEntryList;

public:
    AtlasConfigChunk();
    ~AtlasConfigChunk();

    typedef AtlasChunk Parent;

    virtual void read(Stream* s);
    virtual void write(Stream* s);

    virtual void process();

    void generate(AtlasChunk* children[4])
    {
        AssertISV(false, "AtlasConfigChunk::generate - Not a quadtree chunk type!");
    }

    AtlasConfigChunk* generateCopy(S32 reformat = -1);
    virtual U32 getHeadSentinel();
    virtual U32 getTailSentinel();

    void setEntry(const char* key, const char* v);
    bool clearEntry(const char* key);
    bool getEntry(const char* key, const char*& v);
};

#endif