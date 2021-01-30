//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------
#include "atlas/core/atlasRequestNote.h"

//-----------------------------------------------------------------------------

FreeListChunker<AtlasRequestHeader::ARHNote> AtlasRequestHeader::smChunker;

AtlasRequestHeader::ARHNote* AtlasRequestHeader::allocNote()
{
    return smChunker.alloc();
}

void AtlasRequestHeader::freeNote(ARHNote* note)
{
    smChunker.free(note);
}

//-----------------------------------------------------------------------------

AtlasRequestHeader::AtlasRequestHeader()
{
    head = NULL;
    refCount = 0;
}

AtlasRequestHeader::~AtlasRequestHeader()
{
    // Free everything on our list.
    ARHNote* arh = head;
    while (arh)
    {
        ARHNote* toBlast = arh;
        arh = arh->next;
        freeNote(toBlast);
    }
}

void AtlasRequestHeader::request(void* who, F32 priority, U32 reason)
{
    // Walk list and check for this request...
    ARHNote* arh = head;
    while (arh)
    {
        // Update on match.
        if (arh->who == who)
        {
            arh->priority = priority;
            arh->reason = reason;
            return;
        }

        // Or else keep iterating..
        arh = arh->next;
    }

    // Nuts, fell through. Let's allocate a new one.
    arh = allocNote();

    refCount++;

    // Set values on the note.
    arh->who = who;
    arh->priority = priority;
    arh->reason = reason;

    // Link it in.
    arh->next = head;
    head = arh;

    // And done.
}

void AtlasRequestHeader::cancel(void* who, U32 reason)
{
    // Got a cancel, walk the list and find ourselves, then remove from list.
    ARHNote** arh = &head;
    while (*arh)
    {
        // Delete if we match.
        if ((*arh)->who == who)
        {
            // Grab what we gotta free.
            ARHNote* dead = *arh;

            // Unlink.
            *arh = (*arh)->next;

            // And dealloc.
            freeNote(dead);

            refCount--;
            return;
        }

        // Advance...
        arh = &((*arh)->next);
    }

    //   AssertFatal(false, "AtlasRequestHeader::cancel - requester not in list!");
}

const F32 AtlasRequestHeader::calculateCumulativePriority() const
{
    // Walk the list and get max priority.
    F32 cumPri = 0.f;

    ARHNote* arh = head;
    while (arh)
    {
        cumPri = getMax(arh->priority, cumPri);
        arh = arh->next;
    }

    return cumPri;
}