//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------
#include "atlas/core/atlasChunk.h"
#include "util/safeDelete.h"
#include "core/memstream.h"
#include "console/console.h"
#include "util/fourcc.h"

AtlasChunk::~AtlasChunk()
{
}

U32 AtlasChunk::getHeadSentinel()
{
    AssertFatal(false, "AtlasChunk::getHeadSentinel - should always be overriden!");
    return -1;
}

U32 AtlasChunk::getTailSentinel()
{
    AssertFatal(false, "AtlasChunk::getTailSentinel - should always be overriden!");
    return -1;
}

void AtlasChunk::process()
{
}

void AtlasChunk::generate(AtlasChunk* children[4])
{
}

void AtlasChunk::write(Stream* s)
{
    s->write(mPreviousChunk);
}

void AtlasChunk::read(Stream* s)
{
    s->read(&mPreviousChunk);
}

U32 AtlasChunk::writeToStream(AtlasChunk* ac, Stream* s)
{
    AssertISV(false, "AtlasChunk::writeToStream - Synch me up with prepareDeferredWrite before you use me.");
    return -1;

    // Get a buffer, and have the chunk write into it.
    U8* data = new U8[MaxChunkSize];
    MemStream dataStream(MaxChunkSize, data);
    ac->write(&dataStream);

    // Write master sentinel...
    s->write(U32(MAKEFOURCC('A', 'T', 'S', 'P')));

    // Now write size and the buffered data.
    U32 chunkSize = dataStream.getPosition();
    s->write(chunkSize);

    // write head sentinel for this chunk.
    s->write(ac->getHeadSentinel());

    // and write data.
    s->write(chunkSize, data);

    // Write tail sentinel.
    s->write(ac->getTailSentinel());

    // All done!
    delete[] data;
    return chunkSize;
}

bool AtlasChunk::readFromStream(AtlasChunk* ac, Stream* s)
{
    // First, read the size and allocate a buffer.

    // Let's do a sentinel check.
    U32 sent1;
    s->read(&sent1);
    AssertISV(sent1 == MAKEFOURCC('A', 'T', 'S', 'P'), "AtlasChunk::readFromStream - (sent1) invalid chunk master sentinel!");

    s->read(&ac->mChunkSize);

    // And now validate the chunk-type-sentinel.
    U32 sent2;
    s->read(&sent2);
    AssertISV(sent2 == ac->getHeadSentinel(), "AtlasChunk::readFromStream - (sent2) invalid chunk head sentinel!");

    U8* data = new U8[ac->mChunkSize];

    // Read next chunksize bytes into the buffer for later processing.
    s->read(ac->mChunkSize, data);

    // Check end sentinel.
    U32 sent3;
    s->read(&sent3);
    AssertISV(sent3 == ac->getTailSentinel(), "AtlasChunk::readFromStream - (sent3) invalid chunk tail sentinel!");

    // And tell the chunk to read from that buffer...
    MemStream dataStream(ac->mChunkSize, data);
    ac->read(&dataStream);

    // Clean up memory.
    delete[] data;

    // All done!
    return true;
}

//-----------------------------------------------------------------------------

AtlasDeferredIO* AtlasChunk::prepareDeferredWrite(AtlasChunk* ac)
{
    // Get a buffer, and have the chunk write into it.
    U8* data = new U8[MaxChunkSize];
    MemStream dataStream(MaxChunkSize, data);

    // Write master sentinel...
    dataStream.write(U32(MAKEFOURCC('A', 'T', 'S', 'P')));

    // Now write size and the buffered data.
    U32 chunkStart = dataStream.getPosition();
    dataStream.write(U32(0)); // Dummy length.

    // write head sentinel.
    dataStream.write(ac->getHeadSentinel());

    // and write data.
    ac->write(&dataStream);

    // Write tail sentinel.
    dataStream.write(ac->getTailSentinel());

    // Backfill the length.
    U32 chunkEnd = dataStream.getPosition();
    dataStream.setPosition(chunkStart);
    dataStream.write((chunkEnd - (chunkStart + 8)) - 4); // Also compensate for sentinel.

    // All done - now populate the ADIO.
    AtlasDeferredIO* adio = new AtlasDeferredIO(AtlasDeferredIO::DeferredWrite);
    adio->flags.set(AtlasDeferredIO::DataIsArray);
    adio->flags.set(AtlasDeferredIO::DeleteDataOnComplete);
    adio->flags.set(AtlasDeferredIO::DeleteStructOnComplete);
    adio->flags.set(AtlasDeferredIO::WriteToEndOfStream);

    adio->length = chunkEnd;
    adio->data = data;

    return adio;
}