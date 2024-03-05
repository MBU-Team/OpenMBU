//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
// Some portions of Atlas code derived from Thatcher Ulrich's original
// ChunkLOD implementation.
//-----------------------------------------------------------------------------

#include "atlas/atlasTQTFile.h"
#include "console/console.h"
#include "core/resManager.h"
#include "core/stream.h"
#include "core/memstream.h"
#include "math/mPoint.h"
#include "gfx/ddsFile.h"
#include "core/frameAllocator.h"
#include "platform/profiler.h"
#include "util/safeDelete.h"

bool AtlasTQTFile::smDumpGeneratedTextures = false;

//------------------------------------------------------------------------

AtlasTQTFile::AtlasTQTFile(const char* file)
{
    // Open the file
    Con::printf("Loading TQT '%s'...", file);
    mStream = ResourceManager->openStream(file);

    AssertISV(mStream, "No TQT file found!");

    // How big is it?
    mFileSize = mStream->getStreamSize();
    Con::printf("   - Filesize = %d", mFileSize);

    // Load the header...
    U32 magic;
    mStream->read(&magic);
    mStream->read(&mVersion);
    mStream->read(&mTreeDepth);
    mStream->read(&mTileSize);

    Con::printf("   - Version %d, depth=%d, tile size=%d", mVersion, mTreeDepth, mTileSize);

    // And load the TOC...
    U32 size = getNodeCount(mTreeDepth);
    U32 val;

    mOffsets.setSize(size);
    for (U32 i = 0; i < size; i++)
    {
        mStream->read(&val);
        mOffsets[i] = val;
    }
}

AtlasTQTFile::~AtlasTQTFile()
{
    if (mStream)
        ResourceManager->closeStream(mStream);
}

U32 AtlasTQTFile::getTileOffset(const U32 level, const Point2I pos) const
{
    Point2I localPos = pos;

    // Make sure we're within the space of the TQT.
    while (localPos.x >= BIT(level))
        localPos.x -= BIT(level);
    while (localPos.y >= BIT(level))
        localPos.y -= BIT(level);
    while (localPos.x < 0)
        localPos.x += BIT(level);
    while (localPos.y < 0)
        localPos.y += BIT(level);

    AssertFatal(level < mTreeDepth, "AtlasTQTFile::getTileOffset - Level was out of range!");

    // Figure out our offset index...
    U32 index = getNodeIndex(level, localPos.x, localPos.y);

    AssertFatal(index < mOffsets.size(), "AtlasTQTFile::getTileOffset - Invalid index calculated!");

    // Ok, we got it...
    return mOffsets[index];
}

U32 AtlasTQTFile::getTileLengthGuess(const U32 level, const Point2I pos) const
{
    Point2I localPos = pos;

    // Make sure we're within the space of the TQT.
    while (localPos.x >= BIT(level))
        localPos.x -= BIT(level);
    while (localPos.y >= BIT(level))
        localPos.y -= BIT(level);
    while (localPos.x < 0)
        localPos.x += BIT(level);
    while (localPos.y < 0)
        localPos.y += BIT(level);

    AssertFatal(level < mTreeDepth, "AtlasTQTFile::getTileOffset - Level was out of range!");

    // Figure out our offset index...
    U32 index = getNodeIndex(level, localPos.x, localPos.y);

    AssertFatal(index < mOffsets.size(), "AtlasTQTFile::getTileOffset - Invalid index calculated!");

    // Now we know where we are, so let's figure where we end...
    U32 offset = mOffsets[index];

    U32 nextOffset = mFileSize;

    // Since we don't write out in-order, we have to walk the whole list and
    // find the next highest offset on the list (or use the file size)
    for (S32 i = 0; i < mOffsets.size(); i++)
    {
        if (mOffsets[i] > offset && mOffsets[i] < nextOffset)
        {
            nextOffset = mOffsets[i];
        }
    }

    // And delta is length!
    return nextOffset - offset;
}

DDSFile* AtlasTQTFile::loadDDS(const U32 level, const Point2I pos) const
{
    mStream->setPosition(getTileOffset(level, pos));

    // Read the whole thing, and put it into memory...
    PROFILE_START(ld_read1);
    U32 ddsSize;
    mStream->read(&ddsSize);
    PROFILE_END();


    PROFILE_START(ld_read2);
    // Allocate temporary space.
    U8* ddsData = new U8[ddsSize];

    // Read into temp space.
    mStream->read(ddsSize, ddsData);
    PROFILE_END();

    // Now load it into a DDSFile.
    PROFILE_START(ld_readDDS);
    MemStream ddsBuf(ddsSize, ddsData, true, false);
    DDSFile* dds = new DDSFile();

    if (!dds->read(ddsBuf))
    {
        Con::errorf("AtlasTQTFile::loadDDS - encountered problem reading DDS (%d, %d@%d)", pos.x, pos.y, level);
        SAFE_DELETE(dds);
    }

    PROFILE_END();

    PROFILE_START(ld_cleanupMem);
    // Clean up temp space. A bit lame we allocate this...
    delete[] ddsData;
    PROFILE_END();

    return dds;
}

GBitmap* AtlasTQTFile::loadBitmap(const U32 level, const Point2I pos) const
{
    AssertISV(false, "AtlasTQTFile::loadBitmap - This should be re-implemented someday.");
    /*
       // We never go below level 1, so bump us down a bit.
       //level = level + 1;
       Point2I localPos = pos;

       // Make sure we're within the space of the TQT.
       while(localPos.x >= BIT(level))
          localPos.x -= BIT(level);
       while(localPos.y >= BIT(level))
          localPos.y -= BIT(level);
       while(localPos.x < 0)
          localPos.x += BIT(level);
       while(localPos.y < 0)
          localPos.y += BIT(level);

       AssertFatal(level < mTreeDepth, "AtlasTQTFile::loadImage - Level was out of range!");

       // Figure out our offset index...
       U32 index = getNodeIndex(level, localPos.x, localPos.y);

       AssertFatal(index < mOffsets.size(), "AtlasTQTFile::loadImage - Invalid index calculated!");

       // Ok, load some data...
       U32 offset = mOffsets[index];
       MemStream jpegBuf(mFileSize - offset, (U8*)mData + offset, true, false);

       GBitmap *bmp = new GBitmap();

       bmp->readJPEG(jpegBuf);

       return bmp; */
    return NULL;
}

//------------------------------------------------------------------------

extern "C" {
#include <jpeglib.h>
}
#include "core/fileio.h"
#include "core/fileStream.h"
#include "gfx/gfxDevice.h"

class IStreamedImageReader
{
public:
    virtual bool open(const char* image) = 0;
    virtual U32 getRowLength() = 0;
    virtual bool readRow(U8* buffer) = 0;
    virtual void close() = 0;
};

class StreamedJPEGReader : public IStreamedImageReader
{
private:
    /// JPEG context information.
    jpeg_source_mgr        mSource;
    jpeg_decompress_struct	mCinfo;
    jpeg_error_mgr         mJerr;

    bool    mStartOfFile;
    File* mStream;
    JOCTET  mBuffer[4096];

    static void initSource(j_decompress_ptr cinfo)
    {
        StreamedJPEGReader* src = (StreamedJPEGReader*)cinfo->client_data;
        AssertISV(src, "StreamedJPEGReader::initSource - 'this' is missing!");

        src->mStartOfFile = true;
    }

    static boolean	fillInputBuffer(j_decompress_ptr cinfo)
        // Read data into our input buffer.  Client calls this
        // when it needs more data from the file.
    {
        StreamedJPEGReader* src = (StreamedJPEGReader*)cinfo->client_data;
        AssertISV(src, "StreamedJPEGReader::fillInputBuffer - 'this' is missing!");

        U32 bytesRead = -1;

        src->mStream->read(4096, (char*)src->mBuffer, &bytesRead);

        if (bytesRead <= 0)
        {
            // Is the file completely empty?
            AssertISV(!src->mStartOfFile, "StreamedJPEGReader::fillInputBuffer - Empty source stream!");

            // Insert a fake EOI marker.
            src->mBuffer[0] = (JOCTET)0xFF;
            src->mBuffer[1] = (JOCTET)JPEG_EOI;
            bytesRead = 2;
        }

        // Expose buffer state to clients.
        src->mSource.next_input_byte = src->mBuffer;
        src->mSource.bytes_in_buffer = bytesRead;
        src->mStartOfFile = false;

        return true;
    }

    static void	skipInputData(j_decompress_ptr cinfo, long num_bytes)
        // Called by client when it wants to advance past some
        // uninteresting data.
    {
        StreamedJPEGReader* src = (StreamedJPEGReader*)cinfo->client_data;
        AssertISV(src, "StreamedJPEGReader::skipInputData - 'this' is missing!");

        // According to jpeg docs, large skips are
        // infrequent.  So let's just do it the simple
        // way.
        if (num_bytes > 0) {
            while (num_bytes > (long)src->mSource.bytes_in_buffer)
            {
                num_bytes -= (long)src->mSource.bytes_in_buffer;
                fillInputBuffer(cinfo);
            }
            // Handle remainder.
            src->mSource.next_input_byte += (size_t)num_bytes;
            src->mSource.bytes_in_buffer -= (size_t)num_bytes;
        }
    }

    static void termSource(j_decompress_ptr cinfo)
        // Terminate the source.  Make sure we get deleted.
    {
        StreamedJPEGReader* src = (StreamedJPEGReader*)cinfo->client_data;
        AssertISV(src, "StreamedJPEGReader::termSource - 'this' is missing!");

        // @@ it's kind of bogus to be deleting here
        // -- term_source happens at the end of
        // reading an image, but we're probably going
        // to want to init a source and use it to read
        // many images, without reallocating our
        // buffer.
        //delete src;
        //cinfo->src = NULL;
    }

    static int jpegErrorFn(void* client_data)
    {
        StreamedJPEGReader* stream = (StreamedJPEGReader*)client_data;
        AssertFatal(stream != NULL, "StreamedJPEGReader::jpegErrorFn - No stream.");
        return (stream->mStream->getStatus() != File::Ok);
    }

public:

    StreamedJPEGReader()
    {
        mStartOfFile = true;
        mStream = NULL;

        // Fill in function pointers...
        mSource.init_source = initSource;
        mSource.fill_input_buffer = fillInputBuffer;
        mSource.skip_input_data = skipInputData;
        mSource.resync_to_restart = jpeg_resync_to_restart;	// use default method
        mSource.term_source = termSource;
        mSource.bytes_in_buffer = 0;
        mSource.next_input_byte = NULL;

        dMemset(&mCinfo, 0, sizeof(mCinfo));
    }

    ~StreamedJPEGReader()
    {
        SAFE_DELETE(mStream);
    }

    virtual bool open(const char* image)
    {
        SAFE_DELETE(mStream);

        mStartOfFile = true;
        mStream = new File();

        if (mStream->open(image, File::Read) != File::Ok)
        {
            delete mStream;
            return false;
        }

        // Now we can initialize the JPEG decompression object.
        jpeg_create_decompress(&mCinfo);

        // Bit of a hack, we're ok as long as jpeg_source_mgr comes FIRST.
        mCinfo.src = &mSource; // (jpeg_source_mgr*)this;
        mCinfo.err = jpeg_std_error(&mJerr);    // set up the normal JPEG error routines.
        mCinfo.client_data = (void*)this;    // set the stream into the client_data

        // Read file header, set default decompression parameters
        jpeg_read_header(&mCinfo, true);

        jpeg_start_decompress(&mCinfo);

        return true;
    }

    virtual U32 getRowLength()
    {
        return mCinfo.output_width;
    }

    virtual bool readRow(U8* buffer)
    {
        AssertFatal(mCinfo.output_scanline < mCinfo.output_height, "StreamedJPEGReader - overran image!");

        if (!jpeg_read_scanlines(&mCinfo, &buffer, 1))
        {
            AssertFatal(false, "StreamedJPEGReader - failed to read scanline from image!");
            return false;
        }

        return true;
    }

    virtual void close()
    {
        jpeg_finish_decompress(&mCinfo);
        jpeg_destroy_decompress(&mCinfo);
    }

};

void AtlasTQTFile::copy(AtlasTQTFile* tqt, Stream& s, U32 srcLevel, Point2I srcPos, U32 dstLevel, Point2I dstPos)
{
    /*   // Save the new offset. For now, we always copy from a tree
       mOffsets[getNodeIndex(dstLevel, dstPos.x, dstPos.y)] = s.getPosition();

       // Get a stream on this node.
       U32 offset = tqt->mOffsets[tqt->getNodeIndex(srcLevel, srcPos.x, srcPos.y)];
       MemStream copyS(tqt->mFileSize - offset, (U8*)tqt->mData + offset, true, false);

       // And write data out to our stream.
       U32 length;
       copyS.read(&length);
       s.write(length);

       Con::printf("  [%d, (%d, %d)] --> [%d, (%d, %d)] Size = %d", srcLevel, srcPos.x, srcPos.y, dstLevel, dstPos.x, dstPos.y, length);

       {
          FrameAllocatorMarker tmpAlloc;
          U8 *tmp = (U8*)tmpAlloc.alloc(length);

          copyS.read(length, tmp);
          s.write(length, tmp);
       }

       // And recurse, if we haven't bottomed out.
       if(srcLevel + 1 < tqt->mTreeDepth)
       {
          copy(tqt, s, srcLevel + 1, (srcPos*2) + Point2I(0,0), dstLevel + 1, (dstPos*2) + Point2I(0,0));
          copy(tqt, s, srcLevel + 1, (srcPos*2) + Point2I(0,1), dstLevel + 1, (dstPos*2) + Point2I(0,1));
          copy(tqt, s, srcLevel + 1, (srcPos*2) + Point2I(1,0), dstLevel + 1, (dstPos*2) + Point2I(1,0));
          copy(tqt, s, srcLevel + 1, (srcPos*2) + Point2I(1,1), dstLevel + 1, (dstPos*2) + Point2I(1,1));
       } */
}

void AtlasTQTFile::merge(AtlasTQTFile* tqt00, AtlasTQTFile* tqt01, AtlasTQTFile* tqt10, AtlasTQTFile* tqt11, const char* outFile)
{
    // Set up our properties and initialize some offset space.
    mVersion = 2;

    // Our tree depth is one higher than that of our children...
    mTreeDepth = tqt00->mTreeDepth + 1;
    mTileSize = tqt00->mTileSize;

    // Set up our tile offsets.
    mOffsets.setSize(getNodeCount(mTreeDepth));
    dMemset(mOffsets.address(), 0, sizeof(mOffsets[0]) * mOffsets.size());

    // Open the file for output.
    Stream* s;
    if (!ResourceManager->openFileForWrite(s, outFile, File::ReadWrite))
    {
        Con::errorf("AtlasTQTFile::merge - could not open '%s' for generation (read AND write).", outFile);
        return;
    }

    // Write a header. Some parts of this will get rewritten later (ie, the TOC)
    s->write(4, "tqt\0");
    s->write(mVersion);
    s->write(mTreeDepth);
    s->write(mTileSize);

    U32 tocOffset = s->getPosition();

    s->write(sizeof(mOffsets[0]) * mOffsets.size(), mOffsets.address());

    // Ok, copy everything from the children.
    copy(tqt00, s, 0, Point2I(0, 0), 1, Point2I(0, 0));
    copy(tqt01, s, 0, Point2I(0, 0), 1, Point2I(0, 1));
    copy(tqt10, s, 0, Point2I(0, 0), 1, Point2I(1, 0));
    copy(tqt11, s, 0, Point2I(0, 0), 1, Point2I(1, 1));

    // Now we've copied everything from the chillun, let's make sure we have all
    // the upper nodes we need.
    generateInnerTiles(s);

    // Update the TOC.
    s->setPosition(tocOffset);
    for (S32 i = 0; i < mOffsets.size(); i++)
        s->write(mOffsets[i]);

    // Finally, close up.
    delete s;
}

void AtlasTQTFile::createTQT(const char* sourceImage, const char* outputTQT, U32 treeDepth, U32 tileSize)
{
    if (!GFX->devicePresent())
    {
        Con::errorf("AtlasTQTFile::createTQT - No GFX device present; can't generate TQT without one currently.");
        return;
    }

    // Set up the streaming image source.
    StreamedJPEGReader reader;

    if (!reader.open(sourceImage))
    {
        Con::errorf("AtlasTQTFile::createTQT - Could not load '%s'.", sourceImage);
        return;
    }

    // Set up fields for write.
    mVersion = 2;
    mTreeDepth = treeDepth;
    mTileSize = tileSize;

    // Ok, check that we're sane.
    if (BIT(mTreeDepth - 1) * mTileSize != reader.getRowLength())
    {
        Con::errorf("AtlasTQTFile::createTQT - Invalid treeDepth/tileSize (2 ^ (%d - 1) * %d != %d)", mTreeDepth, mTileSize, reader.getRowLength());
        return;
    }

    // Allocate and clear our offsets.
    mOffsets.setSize(getNodeCount(treeDepth));
    dMemset(mOffsets.address(), 0, sizeof(mOffsets[0]) * mOffsets.size());

    // Open the file for output.
    Stream* s;
    if (!ResourceManager->openFileForWrite(s, outputTQT, File::ReadWrite))
    {
        Con::errorf("AtlasTQTFile::createTQT - could not open '%s' for generation (read AND write).", outputTQT);
        return;
    }

    // Write a header. Some parts of this will get rewritten later (ie, the TOC)
    s->write(4, "tqt\0");
    s->write(mVersion);
    s->write(mTreeDepth);
    s->write(mTileSize);

    U32 tocOffset = s->getPosition();

    s->write(sizeof(mOffsets[0]) * mOffsets.size(), mOffsets.address());

    // Allocate space for the row working area which is equal to the height of
    // a tile, and also load in the first chunk's worth of data.
    U32 workareaSize = reader.getRowLength() * 3 * tileSize;
    Con::printf("Allocating ~%d kb of JPEG decompression working area...", workareaSize / 1024);

    U8** rows = new U8 * [tileSize];
    for (S32 i = 0; i < tileSize; i++)
        rows[i] = new U8[reader.getRowLength() * 3];

    // Generate our leaf tiles...

    U8* tileWorkspace = new U8[mTileSize * mTileSize * 3];

    S32 leafGridSize = reader.getRowLength() / tileSize;

    Con::printf("Generating %dx%d leaf tiles...", leafGridSize, leafGridSize);

    for (S32 i = 0; i < leafGridSize; i++)
    {
        // Read the next slice.
        for (S32 k = 0; k < mTileSize; k++)
        {
            reader.readRow(rows[k]);

            // And we have to CONVERT the row because Microsoft SUCKS.
            for (S32 q = 0; q < leafGridSize * mTileSize; q++)
            {
                // Turn RGB into BGR.
                U8* c = rows[k] + q * 3;
                U8 tmp = c[0]; c[0] = c[2]; c[2] = tmp;
            }
        }

        // Process the row into individual tiles.
        for (S32 j = 0; j < leafGridSize; j++)
        {
            // Copy into the tile working space.
            for (S32 k = 0; k < mTileSize; k++)
                dMemcpy((void*)&tileWorkspace[k * mTileSize * 3], rows[k] + j * mTileSize * 3, 3 * mTileSize);

            // Process and save the image.
            U32 tileIdx = getNodeIndex(mTreeDepth - 1, i, j);
            mOffsets[tileIdx] = writeLeafTile(tileWorkspace, s, mTreeDepth - 1, Point2I(i, j));
        }
    }

    // Just to be sure, let's make sure there are no zero offsets in the lowest level.
    for (S32 i = 0; i < leafGridSize; i++)
        for (S32 j = 0; j < leafGridSize; j++)
            AssertFatal(mOffsets[getNodeIndex(mTreeDepth - 1, i, j)], "Null leaf tile!");

    // Free reading related working space.
    for (S32 i = 0; i < tileSize; i++)
        delete[] rows[i];
    delete[] rows;

    // Now we have written a bunch of leaf tiles. It is time to process the
    // remaining inner tiles, recursively, cuz it's fun.
    generateInnerTiles(s);

    // Free working space for the tiles.
    delete[] tileWorkspace;

    // Update the TOC.
    s->setPosition(tocOffset);
    for (S32 i = 0; i < mOffsets.size(); i++)
        s->write(mOffsets[i]);

    // Finally, close the reader and the TQT file.

    reader.close();
    delete s;
}



ConsoleFunction(generateTQT, void, 5, 5, "(filename sourceImage, filename tgtFile, treeDepth, tileSize)"
    "Generate a TQT file with the specified parameters from the given source image.")
{
    AtlasTQTFile* tqt = new AtlasTQTFile();
    tqt->createTQT(argv[1], argv[2], dAtoi(argv[3]), dAtoi(argv[4]));
    delete tqt;

    Con::printf("  Done!");
}

ConsoleFunction(mergeTQT, void, 6, 6, "(filename tqt00, filename tqt01, filename tqt10, filename tqt11, filename outputTQT)"
    "Merge the four supplied TQT files into one larger one. NOT CURRENTLY WORKING.")
{
    // Load the four supplied TQT files.
    AtlasTQTFile* tqt00 = new AtlasTQTFile(argv[1]);
    AtlasTQTFile* tqt01 = new AtlasTQTFile(argv[2]);
    AtlasTQTFile* tqt10 = new AtlasTQTFile(argv[3]);
    AtlasTQTFile* tqt11 = new AtlasTQTFile(argv[4]);

    // And merge them into... ONE MASTER TQT OMG
    AtlasTQTFile* outTqt = new AtlasTQTFile();

    outTqt->merge(tqt00, tqt01, tqt10, tqt11, argv[5]);

    // Clean up, don't be a slob...
    delete tqt00;
    delete tqt01;
    delete tqt10;
    delete tqt11;
    delete outTqt;
}