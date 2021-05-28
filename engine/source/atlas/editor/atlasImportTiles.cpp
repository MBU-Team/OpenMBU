//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------
#include "console/console.h"
#include "gfx/gBitmap.h"

#include "atlas/core/atlasFile.h"
#include "atlas/resource/atlasResourceTexTOC.h"

ConsoleFunction(atlasGenerateTextureTOCFromTiles, void, 5, 5, "(leafCount, tileMask, outFile, outFormat) - "
    "Generate a texture TOC from a set of tiles. leafCount is the size of the grid "
    "of tiles on a side. tileMask is the path for the tiles (no extension) with %d "
    "for x and y, ie, 'demo/alpha/Alpha1_x%dy%d'. outFile is the file to output a "
    "new .atlas file to.")
{
    U32 leafSize = dAtoi(argv[1]);
    U32 treeDepth = getBinLog2(leafSize) + 1;
    const char* tileMask = argv[2];
    const char* filePath = argv[3];
    U32 outFormat = dAtoi(argv[4]);

    if (!isPow2(leafSize))
    {
        Con::errorf("atlasGenerateTextureTOCFromTiles - leafSize is not a power of 2!");
        return;
    }

    Con::printf("atlasGenerateTextureTOCFromTiles - Initializing atlas file '%s'...", filePath);

    // Allocate a new AtlasFile.
    AtlasFile af;

    // Put a new textureTOC in it.
    AtlasResourceTexTOC* arttoc = new AtlasResourceTexTOC;
    arttoc->initializeTOC(treeDepth);
    af.registerTOC(arttoc);

    // Write TOCs out and get ready to do IO.
    af.createNew(filePath);
    af.startLoaderThreads();

    Con::printf("atlasGenerateTextureTOCFromTiles - Atlas started, processing tiles for %d deep tree...", treeDepth);

    const char* formatName;
    switch (outFormat)
    {
    case AtlasTexChunk::FormatJPEG:
        formatName = "JPEG";
        break;

    case AtlasTexChunk::FormatPNG:
        formatName = "PNG";
        break;

    case AtlasTexChunk::FormatDDS:
        formatName = "DDS (DXT)";
        break;

    default:
        Con::errorf("atlasGenerateTextureTOCFromTiles - Unknown file format ID! Aborting... (use 0-2).");
        return;
    }

    Con::printf("atlasGenerateTextureTOCFromTiles - Storing tiles in %s format...", formatName);

    // Now, load the base chunks in - ie, the ones we have exact tiles for.
    char buff[512];
    for (S32 x = 0; x < leafSize; x++)
    {
        Con::printf("atlasGenerateTextureTOCFromTiles - processing row %d...", x);

        for (S32 y = 0; y < leafSize; y++)
        {
            // Load the image...
            S32 imgX, imgY;
            imgX = y;
            imgY = ((leafSize - 1) - x);

            dSprintf(buff, 512, tileMask, imgX, imgY);
            GBitmap* bmp = GBitmap::load(buff);

            if (!bmp)
            {
                Con::errorf("atlasGenerateTextureTOCFromTiles - unable to open tile '%s'! Aborting!", buff);
                return;
            }

            // Create a chunk to store this data in.
            AtlasTexChunk* atc = new AtlasTexChunk;
            atc->mFormat = (AtlasTexChunk::TexFormat)outFormat;
            atc->bitmap = bmp;

            // Get the stub.
            AtlasResourceTexStub* tStub = arttoc->getStub(treeDepth - 1, Point2I(x, y));

            // And store the chunk.
            arttoc->instateNewChunk(tStub, atc, true);

            // Purge it once we're done so we don't use gigs of ram!
            tStub->purge();
        }
    }

    arttoc->generate(RectI(0, 0, leafSize, leafSize));

    af.waitForPendingWrites();

    Con::printf("atlasGenerateTextureTOCFromTiles - Done.");
}