//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "core/stream.h"
#include "core/fileStream.h"
#include "core/memstream.h"
#include "gBitmap.h"

#include "gif_lib.h"



//-------------------------------------- Replacement I/O for standard LIBjpeg
//                                        functions.  we don't wanna use
//                                        FILE*'s...
static int gifReadDataFn(GifFileType* gifinfo, GifByteType* data, int length)
{
    Stream* stream = (Stream*)gifinfo->UserData;
    AssertFatal(stream != NULL, "jpegReadDataFn::No stream.");
    int pos = stream->getPosition();
    if (stream->read(length, data))
        return length;

    if (stream->getStatus() == Stream::EOS)
        return (stream->getPosition() - pos);
    else
        return 0;
}


//--------------------------------------
static int gifWriteDataFn(GifFileType* gifinfo, GifByteType* data, int length)
{
    Stream* stream = (Stream*)gifinfo->UserData;
    AssertFatal(stream != NULL, "jpegWriteDataFn::No stream.");
    if (stream->write(length, data))
        return length;
    else
        return 0;
}


//--------------------------------------
bool GBitmap::readGIF(Stream& stream)
{
    GifFileType* gifinfo = DGifOpen((void*)&stream, gifReadDataFn);
    if (!gifinfo)
        return false;

    GifRecordType recordType;
    do
    {
        if (DGifGetRecordType(gifinfo, &recordType) == GIF_ERROR)
            break;

        if (recordType == IMAGE_DESC_RECORD_TYPE)
        {
            if (DGifGetImageDesc(gifinfo) == GIF_ERROR)
                break;

            GFXFormat format = (gifinfo->SBackGroundColor == 0) ? GFXFormatR8G8B8 : GFXFormatR8G8B8A8;
            allocateBitmap(gifinfo->SWidth, gifinfo->SHeight, false, format);

            U32 gwidth = gifinfo->Image.Width ? gifinfo->Image.Width : width;
            U32 gheight = gifinfo->Image.Height ? gifinfo->Image.Height : height;
            U32 gifSize = gwidth * gheight;
            U8* data = new U8[gifSize];

            if (DGifGetLine(gifinfo, data, gifSize) != GIF_ERROR)
            {
                // use the global or local color table ?
                GifColorType* color = gifinfo->SColorMap->Colors;
                if (gifinfo->Image.ColorMap)
                    color = gifinfo->Image.ColorMap->Colors;

                if (color)
                {
                    U8* dst = getAddress(gifinfo->Image.Left, gifinfo->Image.Top);
                    U8* src = data;
                    U32 right = gifinfo->Image.Left + gwidth;
                    U32 bottom = gifinfo->Image.Top + gheight;
                    U32 next = (width - gwidth) * bytesPerPixel;

                    if (format == GFXFormatR8G8B8A8)
                    {
                        for (U32 y = gifinfo->Image.Top; y < bottom; y++)
                        {
                            for (U32 x = gifinfo->Image.Left; x < right; x++, src++)
                            {
                                if (*src == gifinfo->SBackGroundColor)
                                {
                                    // this is a transparent pixel
                                    dst[0] = 0;    // red
                                    dst[1] = 0;    // green
                                    dst[2] = 0;    // blue
                                    dst[3] = 0;    // alpha
                                }
                                else
                                {
                                    dst[0] = color[*src].Red;
                                    dst[1] = color[*src].Green;
                                    dst[2] = color[*src].Blue;
                                    dst[3] = 0;    // alpha
                                }
                                dst += bytesPerPixel;
                            }
                            dst += next;
                        }
                    }
                    else
                    {
                        for (U32 y = gifinfo->Image.Top; y < bottom; y++)
                        {
                            for (U32 x = gifinfo->Image.Left; x < right; x++, src++)
                            {
                                dst[0] = color[*src].Red;
                                dst[1] = color[*src].Green;
                                dst[2] = color[*src].Blue;
                                dst += bytesPerPixel;
                            }
                            dst += next;
                        }
                    }
                    delete[] data;
                    DGifCloseFile(gifinfo);
                    return true;
                }
            }
            // failure
            delete[] data;
            break;
        }
        else if (recordType == EXTENSION_RECORD_TYPE)
        {
            GifByteType* extension;
            S32 extCode;

            // Skip any extension blocks in file
            if (DGifGetExtension(gifinfo, &extCode, &extension) != GIF_ERROR)
            {
                while (extension != NULL)
                {
                    if (DGifGetExtensionNext(gifinfo, &extension) == GIF_ERROR)
                        return false;
                }
            }
            else
                return false;
        }

        break;

    } while (recordType != TERMINATE_RECORD_TYPE);


    DGifCloseFile(gifinfo);
    return false;
}


//--------------------------------------------------------------------------
bool GBitmap::writeGIF(Stream&) const
{
    return false;
}


