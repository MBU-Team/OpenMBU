//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------
#include "console/console.h"
#include "gfx/gBitmap.h"

#include "atlas/core/atlasFile.h"
#include "atlas/resource/atlasResourceTexTOC.h"


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
    virtual U32  getRowLength() = 0;
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
    JOCTET  mBuffer[8192];

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

        src->mStream->read(8192, (char*)src->mBuffer, &bytesRead);

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
        if (num_bytes > 0)
        {
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

ConsoleFunction(atlasGenerateTextureTOCFromLargeJPEG, void, 4, 4, "(jpegFile, treeDepth, outFile) "
    "Generate a texture TOC from a single large JPEG image. The image will be "
    "stored into a quadtree treeDepth deep (ie, cut into a grid treeDepth^2 tiles "
    "on a side). outFile is the name of a new .atlas file which will be created "
    "to store this data.")
{
    const char* imagePath = argv[1];
    const U32   treeDepth = dAtoi(argv[2]);
    const char* atlasPath = argv[3];

    // Let's initialize our streaming image source.
    StreamedJPEGReader sjr;

    if (!sjr.open(imagePath))
    {
        Con::errorf("atlasGenerateTextureTOCFromLargeJPEG - Could not open JPEG '%s'", imagePath);
        return;
    }

    Con::printf("atlasGenerateTextureTOCFromLargeJPEG - Successfully opened JPEG '%s' for reading.", imagePath);

    const U32 imageSize = sjr.getRowLength();
    const U32 tileCount = BIT(treeDepth - 1);
    const U32 tileSize = imageSize / BIT(treeDepth - 1);

    Con::printf("atlasGenerateTextureTOCFromLargeJPEG - Image is %dpx (hopefully square!).", imageSize);
    Con::printf("atlasGenerateTextureTOCFromLargeJPEG - Tree depth of %d mandates %d tiles on a side.", treeDepth, tileCount);
    Con::printf("atlasGenerateTextureTOCFromLargeJPEG - Therefore, tiles are %dpx wide.", tileSize);

    // Allocate a new AtlasFile.
    AtlasFile af;

    // Put a new textureTOC in it.
    AtlasResourceTexTOC* arttoc = new AtlasResourceTexTOC;
    arttoc->initializeTOC(treeDepth);
    af.registerTOC(arttoc);

    // Write TOCs out and get ready to do IO.
    if (!af.createNew(atlasPath))
    {
        Con::errorf("atlasGenerateTextureTOCFromLargeJPEG - Could not create new Atlas file '%s'", atlasPath);
    }

    Con::printf("atlasGenerateTextureTOCFromLargeJPEG - Atlas file '%s' created!", atlasPath);

    af.startLoaderThreads();
    Con::printf("atlasGenerateTextureTOCFromLargeJPEG - Initialized Atlas threads, now importing texture data...");

    // Ok, get ready to stream JPEG data in.

    // Allocate space for the row working area which is equal to the height of
    // a tile, and also load in the first chunk's worth of data.
    U32 workareaSize = sjr.getRowLength() * 3 * tileSize;
    Con::printf("atlasGenerateTextureTOCFromLargeJPEG - Allocating ~%d kb of JPEG decompression working area...", workareaSize / 1024);

    U8** rows = new U8 * [tileSize];
    for (S32 i = 0; i < tileSize; i++)
        rows[i] = new U8[sjr.getRowLength() * 3];

    Con::printf("atlasGenerateTextureTOCFromLargeJPEG - Generating %dpx by %dpx leaf tiles...", tileSize, tileSize);

    for (S32 i = 0; i < tileCount; i++)
    {
        // Read the next slice of rows.
        for (S32 k = 0; k < tileSize; k++)
            sjr.readRow(rows[k]);

        Con::printf("   - processing row %d...", i);

        // Process the row into individual tiles.
        for (S32 j = 0; j < tileCount; j++)
        {
            // Allocate a chunk...
            AtlasTexChunk* atc = new AtlasTexChunk;
            atc->mFormat = AtlasTexChunk::FormatPNG;

            // Copy from the JPEG working space to the bitmap.
            GBitmap* gb = new GBitmap(tileSize, tileSize);
            for (S32 k = 0; k < tileSize; k++)
                dMemcpy(gb->getAddress(0, k), rows[k] + j * tileSize * 3, 3 * tileSize);
            atc->bitmap = gb;

            // Get the stub.
            AtlasResourceTexStub* tStub = arttoc->getStub(treeDepth - 1, Point2I(i, j));

            // Store, then purge, the chunk.
            arttoc->instateNewChunk(tStub, atc, true);
            tStub->purge();
        }
    }

    Con::printf("atlasGenerateTextureTOCFromLargeJPEG - Now generating rest of quadtree...");

    arttoc->generate(RectI(0, 0, tileCount, tileCount));

    af.waitForPendingWrites();

    // Clean up memory.
    for (S32 i = 0; i < tileSize; i++)
    {
        SAFE_DELETE_ARRAY(rows[i]);
    }

    SAFE_DELETE_ARRAY(rows);

    Con::printf("atlasGenerateTextureTOCFromLargeJPEG - Done.");
}