//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "gfx/ddsFile.h"
#include "util/fourcc.h"
#include "gfx/gfxDevice.h"

S32 DDSFile::smExtantCopies = 0;

// These were brutally ripped from their home in the DX9 docs, yayz for me.
// The names are slightly changed from the "real" defines since not all
// platforms have them. I mean, to protect the innocent.
enum DDSSurfaceDescFlags
{
    DDSDCaps = 0x00000001l,
    DDSDHeight = 0x00000002l,
    DDSDWidth = 0x00000004l,
    DDSDPitch = 0x00000008l,
    DDSDPixelFormat = 0x00001000l,
    DDSDMipMapCount = 0x00020000l,
    DDSDLinearSize = 0x00080000l,
    DDSDDepth = 0x00800000l,
};

enum DDSPixelFormatFlags
{
    DDPFAlphaPixels = 0x00000001,
    DDPFFourCC = 0x00000004,
    DDPFRGB = 0x00000040,
};


enum DDSCapFlags
{
    DDSCAPSComplex = 0x00000008,
    DDSCAPSTexture = 0x00001000,
    DDSCAPSMipMap = 0x00400000,

    DDSCAPS2Cubemap = 0x00000200,
    DDSCAPS2Cubemap_POSITIVEX = 0x00000400,
    DDSCAPS2Cubemap_NEGATIVEX = 0x00000800,
    DDSCAPS2Cubemap_POSITIVEY = 0x00001000,
    DDSCAPS2Cubemap_NEGATIVEY = 0x00002000,
    DDSCAPS2Cubemap_POSITIVEZ = 0x00004000,
    DDSCAPS2Cubemap_NEGATIVEZ = 0x00008000,
    DDSCAPS2Volume = 0x00200000,
};

#define FOURCC_DXT1  (MAKEFOURCC('D','X','T','1'))
#define FOURCC_DXT2  (MAKEFOURCC('D','X','T','2'))
#define FOURCC_DXT3  (MAKEFOURCC('D','X','T','3'))
#define FOURCC_DXT4  (MAKEFOURCC('D','X','T','4'))
#define FOURCC_DXT5  (MAKEFOURCC('D','X','T','5'))

void DDSFile::clear()
{
    mFlags = 0;
    mHeight = mWidth = mDepth = mPitchOrLinearSize = mMipMapCount = 0;
    mFormat = GFXFormatR8G8B8;
}

U32 DDSFile::getSurfaceSize(U32 height, U32 width, U32 mipLevel)
{
    // Bump by the mip level.
    height = getMax(U32(1), height >> mipLevel);
    width = getMax(U32(1), width >> mipLevel);

    if (mFlags.test(CompressedData))
    {
        // From the directX docs:
        // max(1, width ÷ 4) x max(1, height ÷ 4) x 8(DXT1) or 16(DXT2-5)

        U32 sizeMultiple = 0;

        switch (mFormat)
        {
        case GFXFormatDXT1:
            sizeMultiple = 8;
            break;
        case GFXFormatDXT2:
        case GFXFormatDXT3:
        case GFXFormatDXT4:
        case GFXFormatDXT5:
            sizeMultiple = 16;
            break;
        default:
            AssertISV(false, "DDSFile::getSurfaceSize - invalid compressed texture format, we only support DXT1-5 right now.");
            break;
        }

        return getMax(U32(1), width / 4) * getMax(U32(1), height / 4) * sizeMultiple;
    }
    else
    {
        return height * width * mBytesPerPixel;
    }
}

bool DDSFile::readHeader(Stream& s)
{
    U32 tmp;

    // Read the FOURCC
    s.read(&tmp);

    if (tmp != MAKEFOURCC('D', 'D', 'S', ' '))
    {
        Con::errorf("DDSFile::readHeader - unexpected magic number, wanted 'DDS '!");
        return false;
    }

    // Read the size of the header.
    s.read(&tmp);

    if (tmp != 124)
    {
        Con::errorf("DDSFile::readHeader - incorrect header size. Expected 124 bytes.");
        return false;
    }

    // Read some flags...
    U32 ddsdFlags;
    s.read(&ddsdFlags);

    // "Always include DDSD_CAPS, DDSD_PIXELFORMAT, DDSD_WIDTH, DDSD_HEIGHT."
    if (!(ddsdFlags & (DDSDCaps | DDSDPixelFormat | DDSDWidth | DDSDHeight)))
    {
        Con::errorf("DDSFile::readHeader - incorrect surface description flags.");
        return false;
    }

    // Read height and width (always present)
    s.read(&mHeight);
    s.read(&mWidth);

    // Read pitch or linear size.

    // First make sure we have valid flags (either linear size or pitch).
    if ((ddsdFlags & (DDSDLinearSize | DDSDPitch)) == (DDSDLinearSize | DDSDPitch))
    {
        // Both are invalid!
        Con::errorf("DDSFile::readHeader - encountered both DDSD_LINEARSIZE and DDSD_PITCH!");
        return false;
    }

    // Ok, some flags are set, so let's do some reading.
    s.read(&mPitchOrLinearSize);

    if (ddsdFlags & DDSDLinearSize)
    {
        mFlags.set(LinearSizeFlag);
    }
    else if (ddsdFlags & DDSDPitch)
    {
        mFlags.set(PitchSizeFlag);
    }
    else
    {
        // Neither set! This appears to be depressingly common.
        // Con::warnf("DDSFile::readHeader - encountered neither DDSD_LINEARSIZE nor DDSD_PITCH!");
    }

    // Do we need to read depth? If so, we are a volume texture!
    s.read(&mDepth);

    if (ddsdFlags & DDSDDepth)
    {
        mFlags.set(VolumeFlag);
    }
    else
    {
        // Wipe it if the flag wasn't set!
        mDepth = 0;
    }

    // Deal with mips!
    s.read(&mMipMapCount);

    if (ddsdFlags & DDSDMipMapCount)
    {
        mFlags.set(MipMapsFlag);
    }
    else
    {
        // Wipe it if the flag wasn't set!
        mMipMapCount = 1;
    }

    // Deal with 11 DWORDS of reserved space (this reserved space brought to
    // you by DirectDraw and the letters F and U).
    for (U32 i = 0; i < 11; i++)
        s.read(&tmp);

    // Now we're onto the pixel format!
    s.read(&tmp);

    if (tmp != 32)
    {
        Con::errorf("DDSFile::readHeader - pixel format chunk has unexpected size!");
        return false;
    }

    U32 ddpfFlags;

    s.read(&ddpfFlags);

    // Read the next few values so we can deal with them all in one go.
    U32 pfFourCC, pfBitCount, pfRMask, pfGMask, pfBMask, pfAlphaMask;

    s.read(&pfFourCC);
    s.read(&pfBitCount);
    s.read(&pfRMask);
    s.read(&pfGMask);
    s.read(&pfBMask);
    s.read(&pfAlphaMask);

    // Sanity check flags...
    if (!(ddpfFlags & (DDPFRGB | DDPFFourCC)))
    {
        Con::errorf("DDSFile::readHeader - incoherent pixel flags, neither RGB nor FourCC!");
        return false;
    }

    // For now let's just dump the header info.
    if (ddpfFlags & DDPFRGB)
    {
        mFlags.set(RGBData);

        Con::printf("RGB Pixel format of DDS:");
        Con::printf("   bitcount = %d (16, 24, 32)", pfBitCount);
        mBytesPerPixel = pfBitCount / 8;
        Con::printf("   red   mask = %x", pfRMask);
        Con::printf("   green mask = %x", pfGMask);
        Con::printf("   blue  mask = %x", pfBMask);

        bool hasAlpha = false;

        if (ddpfFlags & DDPFAlphaPixels)
        {
            hasAlpha = true;
            Con::printf("   alpha mask = %x", pfAlphaMask);
        }
        else
        {
            Con::printf("   no alpha.");
        }

        // Try to match a format.
        if (hasAlpha)
        {
            // If it has alpha it is one of...
            // GFXFormatR8G8B8A8
            // GFXFormatR5G5B5A1
            // GFXFormatA8

            if (pfBitCount == 32)
                mFormat = GFXFormatR8G8B8A8;
            else if (pfBitCount == 16)
                mFormat = GFXFormatR5G5B5A1;
            else if (pfBitCount == 8)
                mFormat = GFXFormatA8;
            else
            {
                Con::errorf("DDSFile::readHeader - unable to match alpha RGB format!");
                return false;
            }
        }
        else
        {
            // Otherwise it is one of...
            // GFXFormatR8G8B8
            // GFXFormatR8G8B8X8
            // GFXFormatR5G6B5
            // GFXFormatP8
            // GFXFormatL8

            if (pfBitCount == 24)
                mFormat = GFXFormatR8G8B8;
            else if (pfBitCount == 32)
                mFormat = GFXFormatR8G8B8X8;
            else if (pfBitCount == 16)
                mFormat = GFXFormatR5G6B5;
            else if (pfBitCount == 8)
            {
                // Either palette or luminance... And DDS doesn't support palette.
                mFormat = GFXFormatL8;
            }
            else
            {
                Con::errorf("DDSFile::readHeader - unable to match non-alpha RGB format!");
                return false;
            }
        }


        // Sweet, all done.
    }
    else if (ddpfFlags & DDPFFourCC)
    {
        mFlags.set(CompressedData);

        /*      Con::printf("FourCC Pixel format of DDS:");
              Con::printf("   fourcc = '%c%c%c%c'", ((U8*)&pfFourCC)[0], ((U8*)&pfFourCC)[1], ((U8*)&pfFourCC)[2], ((U8*)&pfFourCC)[3]); */

              // Ok, make a format determination.
        switch (pfFourCC)
        {
        case FOURCC_DXT1:
            mFormat = GFXFormatDXT1;
            break;
        case FOURCC_DXT2:
            mFormat = GFXFormatDXT2;
            break;
        case FOURCC_DXT3:
            mFormat = GFXFormatDXT3;
            break;
        case FOURCC_DXT4:
            mFormat = GFXFormatDXT4;
            break;
        case FOURCC_DXT5:
            mFormat = GFXFormatDXT5;
            break;
        default:
            Con::errorf("DDSFile::readHeader - unknown fourcc = '%c%c%c%c'", ((U8*)&pfFourCC)[0], ((U8*)&pfFourCC)[1], ((U8*)&pfFourCC)[2], ((U8*)&pfFourCC)[3]);
            break;
        }

    }

    // Deal with final caps bits... Is this really necessary?

    U32 caps1, caps2;
    s.read(&caps1);
    s.read(&caps2);
    s.read(&tmp);
    s.read(&tmp); // More icky reserved space.

    // Screw caps1.
    // if(!(caps1 & DDSCAPS_TEXTURE)))
    // {
    // }

    // Caps2 has cubemap/volume info. Care about that.
    if (caps2 & DDSCAPS2Cubemap)
    {
        mFlags.set(CubeMapFlag);
    }

    // MS has ANOTHER reserved word here. This one particularly sucks.
    s.read(&tmp);

    return true;
}

bool DDSFile::read(const char* filename)
{
    Stream* s = ResourceManager->openStream(filename);

    bool res = read(*s);

    // Clean up!
    ResourceManager->closeStream(s);

    return res;
}

bool DDSFile::read(Stream& s)
{
    if (!readHeader(s))
    {
        Con::errorf("DDSFile::read - error reading header!");
        return false;
    }

    // At this point we know what sort of image we contain. So we should
    // allocate some buffers, and read it in.

    // How many surfaces are we talking about?
    if (mFlags.test(CubeMapFlag))
    {
        // Do something with cubemaps.
    }
    else if (mFlags.test(VolumeFlag))
    {
        // Do something with volume
    }
    else
    {
        // It's a plain old texture.

        // First allocate a SurfaceData to stick this in.
        mSurfaces.push_back(new SurfaceData());

        // Load the main image.
        mSurfaces.last()->addNextMip(this, s, mHeight, mWidth, 0);

        // Load however many mips there are.
        for (S32 i = 1; i < mMipMapCount; i++)
            mSurfaces.last()->addNextMip(this, s, mHeight, mWidth, i);

        // Ok, we're done.
    }

    return true;
}
ConsoleFunction(readDDS, void, 2, 2, "(file ddsFile)")
{
    DDSFile dds;
    dds.read(argv[1]);
}

void DDSFile::SurfaceData::dumpImage(DDSFile* dds, U32 mip, const char* file)
{
    GBitmap* foo = new GBitmap(dds->mWidth >> mip, dds->mHeight >> mip, false, dds->mFormat);

    // Copy our data in.
    dMemcpy(foo->getWritableBits(), mMips[mip], dds->getSurfaceSize(dds->mHeight, dds->mWidth, mip));

    // Write it out.
    foo->writePNGDebug((char*)file);

    // Clean up.
    delete foo;
}

void DDSFile::SurfaceData::addNextMip(DDSFile* dds, Stream& s, U32 height, U32 width, U32 mipLevel)
{
    // First, advance the stream to the next DWORD (ie, 4 byte aligned)
    /*U32 align = s.getPosition();
    align += 3; align >>=2; align <<=2;
    s.setPosition(align); */

    U32 size = dds->getSurfaceSize(height, width, mipLevel);
    mMips.push_back(new U8[size]);
    if (!s.read(size, mMips.last()))
        Con::errorf("DDSFile::SurfaceData::addNextMip - failed to read mip!");

    // If it's not compressed, let's dump this mip...
    /*if(!dds->mFlags.test(CompressedData))
       dumpImage(dds, mipLevel, avar("%xmip%d", this, mipLevel)); */
}

ConsoleFunction(getExtantDDSFiles, S32, 1, 1, "() - Returns active DDSes in memory!")
{
    return DDSFile::smExtantCopies;
}