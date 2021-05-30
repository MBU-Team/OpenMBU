//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------
#ifndef _ATLASDEFERREDIO_H_
#define _ATLASDEFERREDIO_H_

#include "platform/platform.h"
#include "core/bitSet.h"

class AtlasFile;
class AtlasTOC;
class Stream;

/// Basic structure for describing an IO operation on an AtlasDeferredFile.
///
/// Fill one of these out, and pass it to AtlasDeferredFile::queue to perform
/// the operation. You can optionally specify a callback to receive when the
/// IO is completed.
///
/// Operations are either deferred or immediate. If they are immediate, then
/// they occur as soon as the ADIO is passed to the queue() method. Otherwise
/// it happens when the worker thread gets to it.
///
/// @ingroup AtlasCore
struct AtlasDeferredIO
{
    enum ActionTypes
    {
        UnknownAction, ///< Make it easier to catch uninitialized fields.
        DeferredRead, DeferredWrite,
        ImmediateRead, ImmediateWrite,
    } action;

    U32 offset, length;
    void* data;

    /// Various flags to control the behavior of the deferred IO system.
    enum ADIOFlags
    {
        /// Can we delete this ADIO when we're done with it?
        DeleteStructOnComplete = BIT(0),

        /// Can we delete the referenced data when we're done?
        DeleteDataOnComplete = BIT(1),

        /// If this is set, use delete[] to delete the data; otherwise
        /// delete is used.
        DataIsArray = BIT(2),

        /// Is this ADIO completed?
        IsCompleted = BIT(3),

        /// Should we do a callback when we're done with this operation?
        DoCallback = BIT(4),

        /// Should we write to the end of the file/stream, wherever that is,
        /// or use a specific offset?
        WriteToEndOfStream = BIT(5),

        /// If set, we complete the IO operation (ie, do callbacks and
        /// deletions) on AtlasDeferredFile::sync(). Otherwise, we do it
        /// as soon as the IO operation completes. Make sure you're not touching
        /// threads the wrong way with this!
        CompleteOnSync = BIT(6),
    };

    /// One or more flags from ADIOFlags.
    BitSet32 flags;

    typedef void (AtlasTOC::* CallbackPtr)(dsize_t key, AtlasDeferredIO* adio);

    /// You may specify a value to pass to the callback (besides a pointer
    /// to this ADIO).
    dsize_t callbackKeyA;

    /// What TOC to do this callback on.
    AtlasTOC* cbObj;

    /// And a pointer to the member to callback to.
    CallbackPtr callback;

    /// @group internalInterface Internal Calls
    /// These calls are internal to the deferred IO system.
    /// @{

    /// Perform callback, deletion activities to complete this deferred IO
    /// operation.
    void complete();

    /// Perform the operation described by this ADIO. Best called from a worker
    /// thread.
    void doAction(Stream* s);

    /// @}


    /// Constructor; pass it the action you want to perform.
    AtlasDeferredIO(ActionTypes action);
    ~AtlasDeferredIO();

    /// Helper function to set things up for a write operation.
    void setWriteBuffer(U32 _offset, U32 _length, void* _data)
    {
        offset = _offset;
        length = _length;
        data = _data;
    }

    /// Helper function to set things up for a read operation. If you don't
    /// pass a buffer to read to, a new one will be allocated right before
    /// we read.
    void setReadBuffer(U32 _offset, U32 _length, void* _data = NULL)
    {
        offset = _offset;
        length = _length;
        data = _data;
    }

    /// Specify a callback on a TOC. Automatically sets the appropriate
    /// flags and members.
    void setCallback(AtlasTOC* toc, CallbackPtr cb, dsize_t key)
    {
        flags.set(DoCallback);
        cbObj = toc;
        callback = cb;
        callbackKeyA = key;
    }

    /// True if this ADIO is actually an immediate operation.
    const bool isImmediate() const
    {
        return (action == ImmediateRead || action == ImmediateWrite);
    }

    /// Returns true if this is a write operation.
    const bool isWrite() const
    {
        return (action == ImmediateWrite || action == DeferredWrite);
    }
};

#endif