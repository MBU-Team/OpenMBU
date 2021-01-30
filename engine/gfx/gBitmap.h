//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _GBITMAP_H_
#define _GBITMAP_H_

//Includes
#ifndef _PLATFORM_H_
#include "platform/platform.h"
#endif
#ifndef _RESMANAGER_H_
#include "core/resManager.h"
#endif
#ifndef _COLOR_H_
#include "core/color.h"
#endif

#include "gfx/gfxEnums.h" // For the format

//-------------------------------------- Forward decls.
class Stream;
class GPalette;

extern ResourceInstance* constructBitmapBMP(Stream& stream);
extern ResourceInstance* constructBitmapPNG(Stream& stream);
extern ResourceInstance* constructBitmapJPEG(Stream& stream);
extern ResourceInstance* constructBitmapGIF(Stream& stream);
extern ResourceInstance* constructBitmapMNG(Stream& stream);
extern ResourceInstance* constructBitmapDBM(Stream& stream);


//------------------------------------------------------------------------------
//-------------------------------------- GBitmap
//
class RectI;
class Point2I;

class GBitmap : public ResourceInstance
{
    //-------------------------------------- public enumerants and structures
public:
    enum Constants {
        c_maxMipLevels = 12 // Support REALLY big images
    };

public:

    bool sgAlreadyConverted;

    static GBitmap* load(const char* path);
    static ResourceObject* findBmpResource(const char* path, char** hackFileName = NULL);

    GBitmap();
    GBitmap(const GBitmap&);

    GBitmap(const U32  in_width,
        const U32  in_height,
        const bool in_extrudeMipLevels = false,
        const GFXFormat in_format = GFXFormatR8G8B8);

    virtual ~GBitmap();


    void allocateBitmap(const U32  in_width,
        const U32  in_height,
        const bool in_extrudeMipLevels = false,
        const GFXFormat in_format = GFXFormatR8G8B8);

    void extrudeMipLevels(bool clearBorders = false);
    void extrudeMipLevelsDetail();

    GBitmap* createPaddedBitmap();
    void convertToBGRx(bool forceSwap = false);

    void copyRect(const GBitmap* in, const RectI& srcRect, const Point2I& dstPoint);

    GFXFormat   getFormat() const;
    bool 		   setFormat(GFXFormat fmt);
    U32         getNumMipLevels() const;
    U32         getWidth(const U32 in_mipLevel = 0) const;
    U32         getHeight(const U32 in_mipLevel = 0) const;
    U32         getDepth(const U32 in_mipLevel = 0) const;

    U8* getAddress(const S32 in_x, const S32 in_y, const U32 mipLevel = U32(0));
    const U8* getAddress(const S32 in_x, const S32 in_y, const U32 mipLevel = U32(0)) const;

    const U8* getBits(const U32 in_mipLevel = 0) const;
    U8* getWritableBits(const U32 in_mipLevel = 0);

    ColorF sampleTexel(F32 u, F32 v);
    bool        getColor(const U32 x, const U32 y, ColorI& rColor) const;
    bool        setColor(const U32 x, const U32 y, ColorI& rColor);

    // Note that on set palette, the bitmap delete's its palette.
    GPalette const* getPalette() const;
    void            setPalette(GPalette* in_pPalette);

    //-------------------------------------- Internal data/operators
    static U32 sBitmapIdSource;

    void deleteImage();

    GFXFormat internalFormat;
public:

    //   bool preserveSize;
    U8* pBits;            // Master bytes
    U32 byteSize;
    U32 width;            // Top level w/h/d
    U32 height;
    U32 depth;
    U32 bytesPerPixel;

    U32 numMipLevels;
    U32 mipLevelOffsets[c_maxMipLevels];

    GPalette* pPalette;      // Note that this palette pointer is ALWAYS
                             //  owned by the bitmap, and will be
                             //  deleted on exit, or written out on a
                             //  write.

    //-------------------------------------- Input/Output interface
public:
    bool readJPEG(Stream& io_rStream);              // located in bitmapJpeg.cc
    bool writeJPEG(Stream& io_rStream) const;

    bool readPNG(Stream& io_rStream, bool useMemoryManager = false); // located in bitmapPng.cc
    bool writePNG(Stream& io_rStream, const bool compressHard = false) const;
    bool writePNGUncompressed(Stream& io_rStream) const;
    
    bool readMNG(Stream& io_rStream);               // located in bitmapMng.cc
    bool writeMNG(Stream& io_rStream) const;

    bool readMSBmp(Stream& io_rStream);             // located in bitmapBmp.cc
    bool writeMSBmp(Stream& io_rStream) const;      // located in bitmapBmp.cc

    bool readGIF(Stream& io_rStream);               // located in bitmapGIF.cc
    bool writeGIF(Stream& io_rStream) const;        // located in bitmapGIF.cc

    bool read(Stream& io_rStream);
    bool write(Stream& io_rStream) const;

    bool writePNGDebug(char* name) const;

private:
    bool _writePNG(Stream& stream, const U32, const U32, const U32) const;

    static const U32 csFileVersion;
};

//------------------------------------------------------------------------------
//-------------------------------------- Inlines
//

inline GFXFormat GBitmap::getFormat() const
{
    return internalFormat;
}

inline U32 GBitmap::getNumMipLevels() const
{
    return numMipLevels;
}

inline U32 GBitmap::getWidth(const U32 in_mipLevel) const
{
    AssertFatal(in_mipLevel < numMipLevels,
        avar("GBitmap::getWidth: mip level out of range: (%d, %d)",
            in_mipLevel, numMipLevels));

    U32 retVal = width >> in_mipLevel;

    return (retVal != 0) ? retVal : 1;
}

inline U32 GBitmap::getHeight(const U32 in_mipLevel) const
{
    AssertFatal(in_mipLevel < numMipLevels,
        avar("Bitmap::getHeight: mip level out of range: (%d, %d)",
            in_mipLevel, numMipLevels));

    U32 retVal = height >> in_mipLevel;

    return (retVal != 0) ? retVal : 1;
}

inline U32 GBitmap::getDepth(const U32 in_mipLevel) const
{
    AssertFatal(in_mipLevel < numMipLevels,
        avar("Bitmap::getDepth: mip level out of range: (%d, %d)",
            in_mipLevel, numMipLevels));

    U32 retVal = depth >> in_mipLevel;

    return (retVal != 0) ? retVal : 1;
}

inline const GPalette* GBitmap::getPalette() const
{
    AssertFatal(getFormat() == GFXFormatP8,
        "Error, incorrect internal format to return a palette");

    return pPalette;
}

inline const U8* GBitmap::getBits(const U32 in_mipLevel) const
{
    AssertFatal(in_mipLevel < numMipLevels,
        avar("GBitmap::getBits: mip level out of range: (%d, %d)",
            in_mipLevel, numMipLevels));

    return &pBits[mipLevelOffsets[in_mipLevel]];
}

inline U8* GBitmap::getWritableBits(const U32 in_mipLevel)
{
    AssertFatal(in_mipLevel < numMipLevels,
        avar("GBitmap::getWritableBits: mip level out of range: (%d, %d)",
            in_mipLevel, numMipLevels));

    return &pBits[mipLevelOffsets[in_mipLevel]];
}

inline U8* GBitmap::getAddress(const S32 in_x, const S32 in_y, const U32 mipLevel)
{
    return (getWritableBits(mipLevel) + ((in_y * getWidth(mipLevel)) + in_x) * bytesPerPixel);
}

inline const U8* GBitmap::getAddress(const S32 in_x, const S32 in_y, const U32 mipLevel) const
{
    return (getBits(mipLevel) + ((in_y * getWidth(mipLevel)) + in_x) * bytesPerPixel);
}

extern void (*bitmapExtrude5551)(const void* srcMip, void* mip, U32 height, U32 width);
extern void (*bitmapExtrudeRGB)(const void* srcMip, void* mip, U32 height, U32 width);
extern void (*bitmapConvertRGB_to_5551)(U8* src, U32 pixels);
extern void (*bitmapConvertRGB_to_1555)(U8* src, U32 pixels);
extern void (*bitmapConvertRGB_to_RGBX)(U8** src, U32 pixels);
extern void (*bitmapExtrudePaletted)(const void* srcMip, void* mip, U32 height, U32 width);

void bitmapExtrudeRGB_c(const void* srcMip, void* mip, U32 height, U32 width);

#endif //_GBITMAP_H_
