//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------
#ifndef _ATLASREQUESTNOTE_H_
#define _ATLASREQUESTNOTE_H_

#include "platform/platform.h"
#include "core/dataChunker.h"

/// Helper class for AtlasResourceStub book-keeping.
///
/// In order to properly page data in/out of the Atlas resource system,
/// we not only need to know how many instances (if any) need the specified
/// chunk, but who they are and what their current priority for it is.
///
/// To reconcile all of this book-keeping in order to determine a final
/// priority, as well as a "yea/nay" for keeping the resource paged in,
/// involves enough work we break it into its own helper class.
///
/// @note This system is plenty fast enough for scenarios with small (single-digit)
/// numbers of terrain instances. For more complex scenarios, a linked list
/// might not be enough, and may need to be replaced with a hash or map.
///
/// @ingroup AtlasCore
struct AtlasRequestHeader
{
    struct ARHNote
    {
        void* who;
        F32 priority;
        U32 reason;

        ARHNote* next;
    };

    static FreeListChunker<ARHNote> smChunker;
    static ARHNote* allocNote();
    static void freeNote(ARHNote* note);

    ARHNote* head;
    U32 refCount;

public:
    AtlasRequestHeader();
    ~AtlasRequestHeader();

    /// Note a request from someone. This either adds a new ARHNote, or
    /// updates one if it exists.
    void request(void* who, F32 priority, U32 reason);

    /// Cancel a request from someone. This will remove the appropriate
    /// ARHNote if it exists.
    void cancel(void* who, U32 reason);

    /// Calculate the total priority on this item. It's best to cache the
    /// result in a member so that it's not needlessly churned.
    const F32 calculateCumulativePriority() const;

    /// Get the current reference count to this stub, based on how many
    /// uncanceled requests there are.
    const U32 getRefCount() const
    {
        return refCount;
    }
};

#endif