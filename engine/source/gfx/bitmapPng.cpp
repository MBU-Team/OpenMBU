//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "core/stream.h"
#include "core/fileStream.h"
#include "core/memstream.h"
#include "gBitmap.h"
#include "core/frameAllocator.h"

#define PNG_INTERNAL 1
#include <time.h>
#include "png.h"
#include "zlib.h"

#ifdef NULL
#undef NULL
#define NULL 0
#endif

// Our chunk signatures...
static png_byte DGL_CHUNK_dcCf[5] = { 100, 99, 67, 102, '\0' };
static png_byte DGL_CHUNK_dcCs[5] = { 100, 99, 67, 115, '\0' };

//-------------------------------------- Replacement I/O for standard LIBPng
//                                        functions.  we don't wanna use
//                                        FILE*'s...
static void pngReadDataFn(png_structp png_ptr,
    png_bytep   data,
    png_size_t  length)
{
    AssertFatal(png_ptr->io_ptr != NULL, "No stream?");

    Stream* strm = (Stream*)png_ptr->io_ptr;
    bool success = strm->read(length, data);
    AssertFatal(success, "pngReadDataFn - failed to read from stream!");
}


//--------------------------------------
static void pngWriteDataFn(png_structp png_ptr,
    png_bytep   data,
    png_size_t  length)
{
    AssertFatal(png_ptr->io_ptr != NULL, "No stream?");

    Stream* strm = (Stream*)png_ptr->io_ptr;
    bool success = strm->write(length, data);
    AssertFatal(success, "pngWriteDataFn - failed to write to stream!");
}


//--------------------------------------
static void pngFlushDataFn(png_structp /*png_ptr*/)
{
    //
}

static png_voidp pngMallocFn(png_structp /*png_ptr*/, png_size_t size)
{
    return FrameAllocator::alloc(size);
}

static void pngFreeFn(png_structp /*png_ptr*/, png_voidp /*mem*/)
{
}

static png_voidp pngRealMallocFn(png_structp /*png_ptr*/, png_size_t size)
{
    return (png_voidp)dMalloc(size);
}

static void pngRealFreeFn(png_structp /*png_ptr*/, png_voidp mem)
{
    dFree(mem);
}

//--------------------------------------
static void pngFatalErrorFn(png_structp     /*png_ptr*/,
    png_const_charp pMessage)
{
    AssertISV(false, avar("Error reading PNG file:\n %s", pMessage));
}


//--------------------------------------
static void pngWarningFn(png_structp, png_const_charp /*pMessage*/)
{
    //   AssertWarn(false, avar("Warning reading PNG file:\n %s", pMessage));
}


//--------------------------------------
bool GBitmap::readPNG(Stream& io_rStream, bool useMemoryManager /* = false */)
{
    static const U32 cs_headerBytesChecked = 8;

    U8 header[cs_headerBytesChecked];
    io_rStream.read(cs_headerBytesChecked, header);

    bool isPng = png_check_sig(header, cs_headerBytesChecked) != 0;
    if (isPng == false)
    {
        AssertWarn(false, "GBitmap::readPNG: stream doesn't contain a PNG");
        return false;
    }

    U32 prevWaterMark = FrameAllocator::getWaterMark();
    png_structp png_ptr = png_create_read_struct_2(PNG_LIBPNG_VER_STRING,
        NULL,
        pngFatalErrorFn,
        pngWarningFn,
        NULL,
        //useMemoryManager ? pngRealMallocFn : pngMallocFn,
        //useMemoryManager ? pngRealFreeFn : pngFreeFn);
        pngRealMallocFn,
        pngRealFreeFn);

    if (png_ptr == NULL)
    {
        //if (!useMemoryManager)
            FrameAllocator::setWaterMark(prevWaterMark);
        return false;
    }

    png_infop info_ptr = png_create_info_struct(png_ptr);
    if (info_ptr == NULL)
    {
        png_destroy_read_struct(&png_ptr,
            (png_infopp)NULL,
            (png_infopp)NULL);

        //if (!useMemoryManager)
            FrameAllocator::setWaterMark(prevWaterMark);

        return false;
    }

    png_infop end_info = png_create_info_struct(png_ptr);
    if (end_info == NULL)
    {
        png_destroy_read_struct(&png_ptr,
            &info_ptr,
            (png_infopp)NULL);

        //if (!useMemoryManager)
            FrameAllocator::setWaterMark(prevWaterMark);

        return false;
    }

    png_set_read_fn(png_ptr, &io_rStream, pngReadDataFn);

    // Read off the info on the image.
    png_set_sig_bytes(png_ptr, cs_headerBytesChecked);
    png_read_info(png_ptr, info_ptr);

    // OK, at this point, if we have reached it ok, then we can reset the
    //  image to accept the new data...
    //
    deleteImage();

    png_uint_32 width;
    png_uint_32 height;
    S32 bit_depth;
    S32 color_type;

    png_get_IHDR(png_ptr, info_ptr,
        &width, &height,             // obv.
        &bit_depth, &color_type,     // obv.
        NULL,                        // interlace
        NULL,                        // compression_type
        NULL);                       // filter_type

     // First, handle the color transformations.  We need this to read in the
     //  data as RGB or RGBA, _always_, with a maximal channel width of 8 bits.
     //
    bool transAlpha = false;
    GFXFormat format = GFXFormatR8G8B8;

    // Strip off any 16 bit info
    //
    if (bit_depth == 16)
    {
        png_set_strip_16(png_ptr);
    }

    // Expand a transparency channel into a full alpha channel...
    //
    if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS))
    {
        png_set_expand(png_ptr);
        transAlpha = true;
    }

    if (color_type == PNG_COLOR_TYPE_PALETTE)
    {
        png_set_expand(png_ptr);
        format = transAlpha ? GFXFormatR8G8B8A8 : GFXFormatR8G8B8;
    }
    else if (color_type == PNG_COLOR_TYPE_GRAY)
    {
        png_set_expand(png_ptr);
        //png_set_gray_to_rgb(png_ptr);
        format = GFXFormatA8; //transAlpha ? RGBA : RGB;
    }
    else if (color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
    {
        png_set_expand(png_ptr);
        png_set_gray_to_rgb(png_ptr);
        format = GFXFormatR8G8B8A8;
    }
    else if (color_type == PNG_COLOR_TYPE_RGB)
    {
        format = transAlpha ? GFXFormatR8G8B8A8 : GFXFormatR8G8B8;
        png_set_expand(png_ptr);
    }
    else if (color_type == PNG_COLOR_TYPE_RGB_ALPHA)
    {
        png_set_expand(png_ptr);
        format = GFXFormatR8G8B8A8;
    }

    // Update the info pointer with the result of the transformations
    //  above...
    png_read_update_info(png_ptr, info_ptr);

    png_uint_32 rowBytes = png_get_rowbytes(png_ptr, info_ptr);
    if (format == GFXFormatR8G8B8)
    {
        AssertFatal(rowBytes == width * 3,
            "Error, our rowbytes are incorrect for this transform... (3)");
    }
    else if (format == GFXFormatR8G8B8A8)
    {
        AssertFatal(rowBytes == width * 4,
            "Error, our rowbytes are incorrect for this transform... (4)");
    }

    // actually allocate the bitmap space...
    allocateBitmap(width, height,
        false,            // don't extrude miplevels...
        format);          // use determined format...

     // Set up the row pointers...
    // AssertISV(height <= csgMaxRowPointers, "Error, cannot load pngs taller than 1024 pixels!");
    png_bytep* rowPointers = new png_bytep[height]; //  sRowPointers;
    U8* pBase = (U8*)getBits();
    for (U32 i = 0; i < height; i++)
        rowPointers[i] = pBase + (i * rowBytes);

    // And actually read the image!
    png_read_image(png_ptr, rowPointers);

    // We're outta here, destroy the png structs, and release the lock
    //  as quickly as possible...
    //png_read_end(png_ptr, end_info);
    png_read_end(png_ptr, NULL);
    png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
    delete[] rowPointers;

    // Ok, the image is read in, now we need to finish up the initialization,
    //  which means: setting up the detailing members, init'ing the palette
    //  key, etc...
    //
    // actually, all of that was handled by allocateBitmap, so we're outta here
    //
    //if (!useMemoryManager)
        FrameAllocator::setWaterMark(prevWaterMark);

    return true;
}


//--------------------------------------------------------------------------
bool GBitmap::_writePNG(Stream& stream,
    const U32 compressionLevel,
    const U32 strategy,
    const U32 filter) const
{
    // ONLY RGB bitmap writing supported at this time!
    AssertFatal(getFormat() == GFXFormatR8G8B8 || getFormat() == GFXFormatR8G8B8A8 || getFormat() == GFXFormatA8, "GBitmap::writePNG: ONLY RGB bitmap writing supported at this time.");
    if (internalFormat != GFXFormatR8G8B8 && internalFormat != GFXFormatR8G8B8A8 && internalFormat != GFXFormatA8)
        return (false);

#define MAX_HEIGHT 4096

    if (height >= MAX_HEIGHT)
        return (false);

    png_structp png_ptr = png_create_write_struct_2(PNG_LIBPNG_VER_STRING,
        NULL,
        pngFatalErrorFn,
        pngWarningFn,
        NULL,
        pngMallocFn,
        pngFreeFn);
    if (png_ptr == NULL)
        return (false);

    png_infop info_ptr = png_create_info_struct(png_ptr);
    if (info_ptr == NULL)
    {
        png_destroy_write_struct(&png_ptr, (png_infopp)NULL);
        return false;
    }

    png_set_write_fn(png_ptr, &stream, pngWriteDataFn, pngFlushDataFn);

    // Set the compression level, image filters, and compression strategy...
    png_ptr->flags |= PNG_FLAG_ZLIB_CUSTOM_STRATEGY;
    png_ptr->zlib_strategy = strategy;
    png_set_compression_window_bits(png_ptr, 15);
    png_set_compression_level(png_ptr, compressionLevel);
    png_set_filter(png_ptr, 0, filter);

    // Set the image information here.  Width and height are up to 2^31,
    // bit_depth is one of 1, 2, 4, 8, or 16, but valid values also depend on
    // the color_type selected. color_type is one of PNG_COLOR_TYPE_GRAY,
    // PNG_COLOR_TYPE_GRAY_ALPHA, PNG_COLOR_TYPE_PALETTE, PNG_COLOR_TYPE_RGB,
    // or PNG_COLOR_TYPE_RGB_ALPHA.  interlace is either PNG_INTERLACE_NONE or
    // PNG_INTERLACE_ADAM7, and the compression_type and filter_type MUST
    // currently be PNG_COMPRESSION_TYPE_BASE and PNG_FILTER_TYPE_BASE. REQUIRED

    if (getFormat() == GFXFormatR8G8B8) {
        png_set_IHDR(png_ptr, info_ptr,
            width, height,               // the width & height
            8, PNG_COLOR_TYPE_RGB,       // bit_depth, color_type,
            NULL,                        // no interlace
            NULL,                        // compression type
            NULL);                       // filter type
    }
    else if (getFormat() == GFXFormatR8G8B8A8) {
        png_set_IHDR(png_ptr, info_ptr,
            width, height,               // the width & height
            8, PNG_COLOR_TYPE_RGB_ALPHA, // bit_depth, color_type,
            NULL,                        // no interlace
            NULL,                        // compression type
            NULL);                       // filter type
    }
    else if (getFormat() == GFXFormatA8) {
        png_set_IHDR(png_ptr, info_ptr,
            width, height,               // the width & height
            8, PNG_COLOR_TYPE_GRAY,      // bit_depth, color_type,
            NULL,                        // no interlace
            NULL,                        // compression type
            NULL);                       // filter type
    }

    png_write_info(png_ptr, info_ptr);
    png_bytep row_pointers[MAX_HEIGHT];
    for (U32 i = 0; i < height; i++)
        row_pointers[i] = const_cast<png_bytep>(getAddress(0, i));

    png_write_image(png_ptr, row_pointers);

    // Write S3TC data if present...
    // Write FXT1 data if present...

    png_write_end(png_ptr, info_ptr);
    png_destroy_write_struct(&png_ptr, (png_infopp)NULL);

    return true;
}


//--------------------------------------------------------------------------
bool GBitmap::writePNG(Stream& stream, const bool compressHard) const
{
    U32 waterMark = FrameAllocator::getWaterMark();

    if (compressHard == false) {
        bool retVal = _writePNG(stream, 6, 0, PNG_ALL_FILTERS);
        FrameAllocator::setWaterMark(waterMark);
        return retVal;
    }
    else {
        U8* buffer = new U8[1 << 22]; // 4 Megs.  Should be enough...
        MemStream* pMemStream = new MemStream(1 << 22, buffer, false, true);

        // We have to try the potentially useful compression methods here.

        const U32 zStrategies[] = { Z_DEFAULT_STRATEGY,
           Z_FILTERED };
        const U32 pngFilters[] = { PNG_FILTER_NONE,
           PNG_FILTER_SUB,
           PNG_FILTER_UP,
           PNG_FILTER_AVG,
           PNG_FILTER_PAETH,
           PNG_ALL_FILTERS };

        U32 minSize = 0xFFFFFFFF;
        U32 bestStrategy = 0xFFFFFFFF;
        U32 bestFilter = 0xFFFFFFFF;
        U32 bestCLevel = 0xFFFFFFFF;

        for (U32 cl = 0; cl <= 9; cl++)
        {
            for (U32 zs = 0; zs < 2; zs++)
            {
                for (U32 pf = 0; pf < 6; pf++)
                {
                    pMemStream->setPosition(0);

                    U32 waterMarkInner = FrameAllocator::getWaterMark();

                    if (_writePNG(*pMemStream, cl, zStrategies[zs], pngFilters[pf]) == false)
                        AssertFatal(false, "Handle this error!");

                    FrameAllocator::setWaterMark(waterMarkInner);

                    if (pMemStream->getPosition() < minSize)
                    {
                        minSize = pMemStream->getPosition();
                        bestStrategy = zs;
                        bestFilter = pf;
                        bestCLevel = cl;
                    }
                }
            }
        }
        AssertFatal(minSize != 0xFFFFFFFF, "Error, no best found?");

        delete pMemStream;
        delete[] buffer;


        bool retVal = _writePNG(stream,
            bestCLevel,
            zStrategies[bestStrategy],
            pngFilters[bestFilter]);
        FrameAllocator::setWaterMark(waterMark);
        return retVal;
    }
}

//--------------------------------------------------------------------------
bool GBitmap::writePNGUncompressed(Stream& stream) const
{
    U32 waterMark = FrameAllocator::getWaterMark();

    bool retVal = _writePNG(stream, 0, 0, PNG_FILTER_NONE);
    FrameAllocator::setWaterMark(waterMark);
    return retVal;
}

//--------------------------------------------------------------------------
#include "console/console.h"
bool GBitmap::writePNGDebug(char* name) const
{
    // ONLY RGB bitmap writing supported at this time!
    AssertFatal(getFormat() == GFXFormatR8G8B8 || getFormat() == GFXFormatR8G8B8A8 || getFormat() == GFXFormatA8, "GBitmap::writePNG: ONLY RGB bitmap writing supported at this time.");
    if (internalFormat != GFXFormatR8G8B8 && internalFormat != GFXFormatR8G8B8A8 && internalFormat != GFXFormatA8)
        return (false);

    // Check height
#define MAX_HEIGHT 4096
    if (height >= MAX_HEIGHT)
        return (false);

    // Set the image information here.  Width and height are up to 2^31,
    // bit_depth is one of 1, 2, 4, 8, or 16, but valid values also depend on
    // the color_type selected. color_type is one of PNG_COLOR_TYPE_GRAY,
    // PNG_COLOR_TYPE_GRAY_ALPHA, PNG_COLOR_TYPE_PALETTE, PNG_COLOR_TYPE_RGB,
    // or PNG_COLOR_TYPE_RGB_ALPHA.  interlace is either PNG_INTERLACE_NONE or
    // PNG_INTERLACE_ADAM7, and the compression_type and filter_type MUST
    // currently be PNG_COMPRESSION_TYPE_BASE and PNG_FILTER_TYPE_BASE. REQUIRED

    png_bytep row_pointers[MAX_HEIGHT];
    char bitmapNameBuf[256];
    for (U32 i = 0; i < getNumMipLevels(); i++)
    {
        // Open stream
        U32 waterMark = FrameAllocator::getWaterMark();
        dSprintf(bitmapNameBuf, sizeof(bitmapNameBuf), "demo/%s_%d.png", name, i);
        Con::printf("Writing texture to : '%s'", bitmapNameBuf);
        Stream* stream;

        if (ResourceManager->openFileForWrite(stream, bitmapNameBuf))
        {
            png_structp png_ptr = png_create_write_struct_2(PNG_LIBPNG_VER_STRING,
                NULL,
                pngFatalErrorFn,
                pngWarningFn,
                NULL,
                pngMallocFn,
                pngFreeFn);

            if (png_ptr == NULL)
            {
                FrameAllocator::setWaterMark(waterMark);
                return false;
            }

            png_infop info_ptr = png_create_info_struct(png_ptr);

            if (info_ptr == NULL)
            {
                png_destroy_write_struct(&png_ptr, (png_infopp)NULL);
                FrameAllocator::setWaterMark(waterMark);
                return false;
            }

            png_set_write_fn(png_ptr, stream, pngWriteDataFn, pngFlushDataFn);

            // Set the compression level, image filters, and compression strategy...
            png_ptr->flags |= PNG_FLAG_ZLIB_CUSTOM_STRATEGY;
            png_ptr->zlib_strategy = 0;
            png_set_compression_window_bits(png_ptr, 15);
            png_set_compression_level(png_ptr, 0);
            png_set_filter(png_ptr, 0, PNG_FILTER_NONE);

            if (getFormat() == GFXFormatR8G8B8)
            {
                png_set_IHDR(png_ptr, info_ptr,
                    getWidth(i), getHeight(i),               // the width & height
                    8, PNG_COLOR_TYPE_RGB,       // bit_depth, color_type,
                    NULL,                        // no interlace
                    NULL,                        // compression type
                    NULL);                       // filter type
            }
            else if (getFormat() == GFXFormatR8G8B8A8)
            {
                png_set_IHDR(png_ptr, info_ptr,
                    getWidth(i), getHeight(i),               // the width & height
                    8, PNG_COLOR_TYPE_RGB_ALPHA, // bit_depth, color_type,
                    NULL,                        // no interlace
                    NULL,                        // compression type
                    NULL);                       // filter type
            }
            else if (getFormat() == GFXFormatA8)
            {
                png_set_IHDR(png_ptr, info_ptr,
                    getWidth(i), getHeight(i),               // the width & height
                    8, PNG_COLOR_TYPE_GRAY,      // bit_depth, color_type,
                    NULL,                        // no interlace
                    NULL,                        // compression type
                    NULL);                       // filter type
            }

            png_write_info(png_ptr, info_ptr);
            for (U32 j = 0; j < getHeight(i); j++)
                row_pointers[j] = const_cast<png_bytep>(getAddress(0, j, i));

            png_write_image(png_ptr, row_pointers);

            // Write S3TC data if present...
            // Write FXT1 data if present...

            png_write_end(png_ptr, info_ptr);
            png_destroy_write_struct(&png_ptr, (png_infopp)NULL);

            // Close stream
            delete stream;
        }

        FrameAllocator::setWaterMark(waterMark);
    }

    return true;
}


