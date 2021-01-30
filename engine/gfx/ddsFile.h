//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _DDSFILE_H_
#define _DDSFILE_H_

#include "console/console.h"
#include "gfx/gfxStructs.h"
#include "core/stream.h"
#include "core/tVector.h"

struct DDSFile
{
    enum DDSFlags
    {
        ComplexFlag = BIT(0), ///< Indicates this includes a mipchain, cubemap, or
                              ///  volume texture, ie, isn't a plain old bitmap.
                              MipMapsFlag = BIT(1), ///< Indicates we have a mipmap chain in the file.
                              CubeMapFlag = BIT(2), ///< Indicates we are a cubemap. Requires all six faces.
                              VolumeFlag = BIT(3), ///< Indicates we are a volume texture.

                              PitchSizeFlag = BIT(4),  ///< Cue as to how to interpret our pitchlinear value.
                              LinearSizeFlag = BIT(5), ///< Cue as to how to interpret our pitchlinear value.

                              RGBData = BIT(6), ///< Indicates that this is straight out RGBA data.
                              CompressedData = BIT(7), ///< Indicates that this is compressed or otherwise
                                                       ///  exotic data.
    };

    BitSet32    mFlags;
    U32         mHeight;
    U32         mWidth;
    U32         mDepth;
    U32         mPitchOrLinearSize;
    U32         mMipMapCount;

    GFXFormat   mFormat;
    U32         mBytesPerPixel; ///< Ignored if we're a compressed texture.
    U32         mFourCC;

    struct SurfaceData
    {
        SurfaceData()
        {
        }

        ~SurfaceData()
        {
            // Free our mips!
            for (S32 i = 0; i < mMips.size(); i++)
                delete[] mMips[i];
        }

        Vector<void*> mMips;

        // Helper function to read in a mipchain.
        bool readMipChain();

        void dumpImage(DDSFile* dds, U32 mip, const char* file);

        /// Reads the next mip level - you are responsible for supplying the proper
        /// parameters. This is mostly a convenience thing.
        void addNextMip(DDSFile* dds, Stream& s, U32 height, U32 width, U32 mipLevel);
    };

    Vector<SurfaceData*> mSurfaces;

    /// Clear all our information; used before reading.
    void clear();

    /// For our current format etc., what is the size of a surface with the
    /// given dimensions?
    U32 getSurfaceSize(U32 height, U32 width, U32 mipLevel = 0);

    /// Read a DDS header from the stream.
    bool readHeader(Stream& s);
    bool read(Stream& s);
    bool read(const char* filename);

    const U32 getWidth(const U32 mipLevel = 0)  const
    {
        return getMax(U32(1), mWidth >> mipLevel);
    }
    const U32 getHeight(const U32 mipLevel = 0) const
    {
        return getMax(U32(1), mHeight >> mipLevel);
    }
    const U32 getDepth(const U32 mipLevel = 0)  const
    {
        return getMax(U32(1), mDepth >> mipLevel);
    }

    const U32 getPitch(const U32 mipLevel = 0) const
    {
        if (mFlags.test(CompressedData))
        {
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
                AssertISV(false, "DDSFile::getPitch - invalid compressed texture format, we only support DXT1-5 right now.");
                break;
            }

            // Maybe need to be DWORD aligned?
            U32 align = getMax(U32(1), getWidth(mipLevel) / 4) * sizeMultiple;;
            align += 3; align >>= 2; align <<= 2;
            return align;

        }
        else
        {
            return getWidth(mipLevel) * mBytesPerPixel;
        }
    }

    // For debugging fun!
    static S32 smExtantCopies;

    DDSFile()
    {
        smExtantCopies++;
    }

    ~DDSFile()
    {
        smExtantCopies--;

        // Free our surfaces!
        for (S32 i = 0; i < mSurfaces.size(); i++)
            delete mSurfaces[i];
    }
};

#endif