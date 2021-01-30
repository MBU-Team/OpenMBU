//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------
#include "atlas/resource/atlasTexChunk.h"
#include "util/safeDelete.h"
#include "gfx/gfxDevice.h"

AtlasTexChunk::AtlasTexChunk()
{
    mFormat = FormatPNG;
    bitmap = NULL;
    dds = NULL;
}

AtlasTexChunk::~AtlasTexChunk()
{
    SAFE_DELETE(bitmap);
    SAFE_DELETE(dds);
}

void AtlasTexChunk::read(Stream* s)
{
    AssertISV(s, "AtlasTexChunk::read - no stream present!");

    // Read the format.
    U8 f;
    s->read(&f);
    mFormat = (TexFormat)f;

    AssertFatal(!(bitmap || dds), "AtlasTexChunk::read - already have a bitmap/dds loaded!");

    bool readSuccess = false;

    switch (f)
    {
    case FormatJPEG:
        bitmap = new GBitmap();
        readSuccess = bitmap->readJPEG(*s);
        break;

    case FormatPNG:
        bitmap = new GBitmap();
        readSuccess = bitmap->readPNG(*s, true);
        break;

    case FormatDDS:
        dds = new DDSFile();
        readSuccess = dds->read(*s);
        break;

    default:
        AssertISV(false, "AtlasTexChunk::read - unknown format!");
        break;
    }

    // For runtime we have to convert endianness.
    if (bitmap)
        bitmap->convertToBGRx();

    AssertISV(readSuccess, "AtlasTexChunk::read - failed to read image format!");
}

void AtlasTexChunk::write(Stream* s)
{
    AssertISV(s, "AtlasTexChunk::write - no stream present!");

    // Write our format, then a texture-specific encoding.
    U8 f = mFormat;
    s->write(f);

    bool writeSuccess = false;

    switch (f)
    {
    case FormatJPEG:
        AssertISV(bitmap, "AtlasTexChunk::write - (JPEG) no bitmap is present!");
        writeSuccess = bitmap->writeJPEG(*s);
        break;

    case FormatPNG:
        AssertISV(bitmap, "AtlasTexChunk::write - (PNG) no bitmap is present!");
        writeSuccess = bitmap->writePNG(*s, false);
        break;

    case FormatDDS:
        AssertISV(dds, "AtlasTexChunk::write - (DDS) no DDS is present!");
        writeDDS(s);
        writeSuccess = true;
        break;

    default:
        AssertISV(false, "AtlasTexChunk::write - unknown format!");
        break;
    }

    AssertISV(writeSuccess, "AtlasTexChunk::write - failed writing bitmap!");
}

void AtlasTexChunk::process()
{
}

void AtlasTexChunk::generate(AtlasChunk* genericChildren[4])
{
    // Cast things so they're convenient.
    AtlasTexChunk* children[4];
    for (S32 i = 0; i < 4; i++)
        children[i] = (AtlasTexChunk*)genericChildren[i];

    // Combine the four children, downsampling them as needed.
    mFormat = children[0]->mFormat;

    if (isBitmapTexFormat(mFormat))
    {
        // Do it in the manner most befitting a GBitmap.

        // Prepare our own GBitmap to write into.
        U32 tileSize = children[0]->bitmap->getWidth();
        bitmap = new GBitmap(tileSize, tileSize, true, children[0]->bitmap->getFormat());

        // Now, bitblt the children into it. Start at topleft, CW. We cheat and use one of their mips.
        for (S32 i = 0; i < 4; i++)
        {
            // Make sure we've got mips, then copy from the second one.
            GBitmap* childBitmap = children[i]->bitmap;

            // Swap back before we muck with it. We'll have to do some
            // cleanup when we have in-engine editing.
            if (childBitmap->sgAlreadyConverted)
                childBitmap->convertToBGRx(true);

            childBitmap->extrudeMipLevels();

            const U8 offsets[4][2] =
            {
               {0,0},
               {1,0},
               {0,1},
               {1,1},
            };

            const S32 xOffset = offsets[i][0];
            const S32 yOffset = offsets[i][1];

            U32 halfTile = (tileSize >> 1);
            AssertFatal(childBitmap->getHeight(1) == halfTile,
                "AtlasTexChunk::generate - tile size mismatch!");

            for (S32 y = 0; y < halfTile; y++)
            {
                // Copy a row at a time.
                dMemcpy(
                    bitmap->getAddress(xOffset * halfTile, yOffset * halfTile + y),
                    childBitmap->getAddress(0, y, 1),
                    childBitmap->bytesPerPixel * halfTile);
            }

            // And we're done.
        }

        // Yay, all done!
    }
    else
    {
        // Do it in the manner most befitting a DDS. (ie, make D3D do it for now.)
        AssertISV(false, "AtlasTexChunk::generate - DDS LOD generation not currently supported.");

    }
}

AtlasTexChunk* AtlasTexChunk::generateCopy(S32 reformat)
{
    AtlasTexChunk* atc = new AtlasTexChunk;

    // Support changing the format as we go.
    if (reformat > -1)
        atc->mFormat = (TexFormat)reformat;
    else
        atc->mFormat = mFormat;

    if (bitmap)
    {
        if (bitmap->sgAlreadyConverted)
            bitmap->convertToBGRx(true);

        atc->bitmap = new GBitmap(*bitmap);
    }

    if (dds)
        atc->dds = new DDSFile(*dds);

    return atc;
}

U32 AtlasTexChunk::getHeadSentinel()
{
    return MAKEFOURCC('T', 'e', 'x', 'H');
}

U32 AtlasTexChunk::getTailSentinel()
{
    return MAKEFOURCC('T', 'e', 'x', 'T');
}
