//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "gBitmap.h"
#include "gPalette.h"
#include "core/stream.h"
#include "platform/platform.h"

// structures mirror those defined by the win32 API

struct RGBQUAD {
    U8 rgbBlue;
    U8 rgbGreen;
    U8 rgbRed;
    U8 rgbReserved;
};

struct BITMAPFILEHEADER {
    U16 bfType;
    U32 bfSize;
    U16 bfReserved1;
    U16 bfReserved2;
    U32 bfOffBits;
};

struct BITMAPINFOHEADER {
    U32 biSize;
    S32 biWidth;
    S32 biHeight;
    U16 biPlanes;
    U16 biBitCount;
    U32 biCompression;
    U32 biSizeImage;
    S32 biXPelsPerMeter;
    S32 biYPelsPerMeter;
    U32 biClrUsed;
    U32 biClrImportant;
};

// constants for the biCompression field
#define BI_RGB        0L
#define BI_RLE8       1L
#define BI_RLE4       2L
#define BI_BITFIELDS  3L


//------------------------------------------------------------------------------
//-------------------------------------- Supplimentary I/O (Partially located in
//                                                          bitmapPng.cc)
//

bool GBitmap::readMSBmp(Stream& stream)
{
    BITMAPINFOHEADER  bi;
    BITMAPFILEHEADER  bf;
    RGBQUAD           rgb[256];

    stream.read(&bf.bfType);
    stream.read(&bf.bfSize);
    stream.read(&bf.bfReserved1);
    stream.read(&bf.bfReserved2);
    stream.read(&bf.bfOffBits);

    stream.read(&bi.biSize);
    stream.read(&bi.biWidth);
    stream.read(&bi.biHeight);
    stream.read(&bi.biPlanes);
    stream.read(&bi.biBitCount);
    stream.read(&bi.biCompression);
    stream.read(&bi.biSizeImage);
    stream.read(&bi.biXPelsPerMeter);
    stream.read(&bi.biYPelsPerMeter);
    stream.read(&bi.biClrUsed);
    stream.read(&bi.biClrImportant);

    GFXFormat fmt = GFXFormatR8G8B8;
    if (bi.biBitCount == 8)
    {
        fmt = GFXFormatP8;
        if (!bi.biClrUsed)
            bi.biClrUsed = 256;
        stream.read(sizeof(RGBQUAD) * bi.biClrUsed, rgb);

        pPalette = new GPalette;
        for (U32 i = 0; i < 256; i++)
        {
            (pPalette->getColors())[i].set(rgb[i].rgbRed,
                rgb[i].rgbGreen,
                rgb[i].rgbBlue,
                255);
        }
    }
    U8* rowBuffer = new U8[bi.biWidth * 4];
    allocateBitmap(bi.biWidth, bi.biHeight, false, fmt);
    S32 width = getWidth();
    S32 height = getHeight();
    for (int i = 0; i < bi.biHeight; i++)
    {
        U8* rowDest = getAddress(0, height - i - 1);
        stream.read(bytesPerPixel * width, rowDest);
    }

    if (bytesPerPixel == 3) // do BGR swap
    {
        U8* ptr = getAddress(0, 0);
        for (int i = 0; i < width * height; i++)
        {
            U8 tmp = ptr[0];
            ptr[0] = ptr[2];
            ptr[2] = tmp;
            ptr += 3;
        }
    }
    delete[] rowBuffer;
    return true;
}

bool GBitmap::writeMSBmp(Stream& io_rStream) const
{

    RGBQUAD           rgb[256];
    BITMAPINFOHEADER  bi;
    BITMAPFILEHEADER  bf;

    bi.biSize = sizeof(BITMAPINFOHEADER);
    bi.biWidth = getWidth();
    bi.biHeight = getHeight();         //our data is top-down
    bi.biPlanes = 1;

    if (getFormat() == GFXFormatP8)
    {
        bi.biBitCount = 8;
        bi.biCompression = BI_RGB;
        bi.biClrUsed = 256;
        AssertFatal(pPalette != NULL, "Error, must have a palette");
    }
    else if (getFormat() == GFXFormatR8G8B8)
    {
        bi.biBitCount = 24;
        bi.biCompression = BI_RGB;
        bi.biClrUsed = 0;
    }
    else
        AssertISV(false, "GBitmap::writeMSBmp - only support P8 and R8G8B8 formats!");

    U32 bytesPP = bi.biBitCount >> 3;
    bi.biSizeImage = getWidth() * getHeight() * bytesPP;
    bi.biXPelsPerMeter = 0;
    bi.biYPelsPerMeter = 0;
    bi.biClrUsed = 0;
    bi.biClrImportant = 0;

    bf.bfType = makeFourCCTag('B', 'M', 0, 0);     //Type of file 'BM'
    bf.bfOffBits = sizeof(BITMAPINFOHEADER)
        + sizeof(BITMAPFILEHEADER)
        + (sizeof(RGBQUAD) * bi.biClrUsed);
    bf.bfSize = bf.bfOffBits + bi.biSizeImage;
    bf.bfReserved1 = 0;
    bf.bfReserved2 = 0;

    io_rStream.write(bf.bfType);
    io_rStream.write(bf.bfSize);
    io_rStream.write(bf.bfReserved1);
    io_rStream.write(bf.bfReserved2);
    io_rStream.write(bf.bfOffBits);

    io_rStream.write(bi.biSize);
    io_rStream.write(bi.biWidth);
    io_rStream.write(bi.biHeight);
    io_rStream.write(bi.biPlanes);
    io_rStream.write(bi.biBitCount);
    io_rStream.write(bi.biCompression);
    io_rStream.write(bi.biSizeImage);
    io_rStream.write(bi.biXPelsPerMeter);
    io_rStream.write(bi.biYPelsPerMeter);
    io_rStream.write(bi.biClrUsed);
    io_rStream.write(bi.biClrImportant);

    if (getFormat() == GFXFormatP8)
    {
        for (S32 ndx = 0; ndx < 256; ndx++)
        {
            rgb[ndx].rgbRed = pPalette->getColor(ndx).red;
            rgb[ndx].rgbGreen = pPalette->getColor(ndx).green;
            rgb[ndx].rgbBlue = pPalette->getColor(ndx).blue;
            rgb[ndx].rgbReserved = 0;
        }
        io_rStream.write(sizeof(RGBQUAD) * 256, (U8*)&rgb);
    }

    //write the bitmap bits
    U8* pMSUpsideDownBits = new U8[bi.biSizeImage];
    for (U32 i = 0; i < getHeight(); i++) {
        const U8* pSrc = getAddress(0, i);
        U8* pDst = pMSUpsideDownBits + (getHeight() - i - 1) * getWidth() * bytesPP;

        dMemcpy(pDst, pSrc, getWidth() * bytesPP);
    }
    io_rStream.write(bi.biSizeImage, pMSUpsideDownBits);
    delete[] pMSUpsideDownBits;

    return io_rStream.getStatus() == Stream::Ok;
}
