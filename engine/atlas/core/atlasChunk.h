//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------
#ifndef _ATLASCHUNK_H_
#define _ATLASCHUNK_H_

#include "platform/platform.h"
#include "core/stream.h"
#include "math/mMath.h"
#include "atlas/core/atlasDeferredIO.h"

class AtlasTOC;

/// Base class for paged Atlas data.
///
/// Atlas is primarily a system for paging data to/from disk. As a result
/// we draw a distinction between stubs (in-core data) and chunks
/// (out-of-core data). Stubs are always present and provide basic
/// information about the data in the chunk. For instance, a geometry
/// stub might store the bounds, while the corresponding geometry chunk
/// might store the vertex and primitive buffers.
///
/// @ingroup AtlasCore
class AtlasChunk
{
public:

    /// @name Static IO Interface
    ///
    /// These helper functions allow us to read and write AtlasChunks in a 
    /// standardized way. They deal with reading headers, comparing sentinels,
    /// filling in ADIOs, and so forth.
    ///
    /// Most of the time these functions are used for chunk IO, rather than
    /// a chunk's read/write.
    ///
    /// @{

    /// This is not currently used; consider prepareDeferredWrite().
    static U32 writeToStream(AtlasChunk* ac, Stream* s);
    static bool readFromStream(AtlasChunk* ac, Stream* s);

    /// Allocate and prepare a new ADIO which contains a serialized version of this
    /// chunk.
    static AtlasDeferredIO* prepareDeferredWrite(AtlasChunk* ac);

    /// @}

    ///
    AtlasChunk() {};
    virtual ~AtlasChunk();

    enum
    {
        /// Maximum size chunk we support reading/writing. This is just so we
        /// can have some fixed buffers and avoid some chicken-and-egg stuff.
        ///
        /// @note You may have to change this if you have large chunks.
        MaxChunkSize = 8 * 1024 * 1024,
    };

    /// Size in bytes of this chunk. First word written, so we can rapidly
    /// load the chunk from disk in a single block, then process from memory.
    U32 mChunkSize;

    /// Offset in file to previous chunk, if any. Used to backtrack for undo,
    /// history analysis, etc.
    U32 mPreviousChunk;

    /// The TOC to which we belong.
    AtlasTOC* mOwningTOC;

    /// Serialize this chunk to a stream.
    ///
    /// @see You probably want to use prepareDeferredWrite()
    virtual void write(Stream* s);

    /// Deserialize from a stream.
    ///
    /// @see You probably want to use readFromStream()
    virtual void read(Stream* s);

    /// Once the chunk has been loaded (in another thread), this is called in
    /// the main thread to prepare any complex resources. (For instance,
    /// a texture or VB, that depend on access to GFX to initialize.)
    virtual void process();

    /// Given four children, (re)generate the data for this one. Their
    /// order starts at topleft, clockwise, 0123.
    virtual void generate(AtlasChunk* children[4]);

    /// @name Sentinel Interface
    ///
    /// To make sure we don't have read/write mismatches, we write a sentinel
    /// value before and after the chunk. This is unique for each type to
    /// ensure we don't mis-read a chunk. Chunk sentinel functionality is
    /// implemented by writeToFile and readFromStream.
    ///
    /// @{

    virtual U32 getHeadSentinel();
    virtual U32 getTailSentinel();

    /// @}
};

#endif