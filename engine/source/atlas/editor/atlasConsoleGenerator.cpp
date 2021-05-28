//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------
#include "console/console.h"
#include "atlas/core/atlasFile.h"
#include "atlas/core/atlasResourceTOC.h"
#include "atlas/resource/atlasResourceGeomTOC.h"
#include "atlas/resource/atlasResourceTexTOC.h"
#include "atlas/resource/atlasResourceConfigTOC.h"

ConsoleFunction(atlasGenerateBlenderTerrain, bool, 5, 20, "(outFile, inGeometryFile, inOpacityFile, inLightmapFile, virtualTexSize, sourceImage0, sourceImage1, sourceImage2, ..., sourceImageN)")
{
    Con::printf("atlasGenerateBlenderTerrain - Getting ready to produce blender terrain...");

    Resource<AtlasFile> inGeometryFile, inOpacityFile, inLightmapFile;

    // Try to load everything.
    Con::printf("   o Opening '%s' for geometry...", argv[2]);
    inGeometryFile = AtlasFile::load(argv[2]);
    if (inGeometryFile.isNull())
    {
        Con::errorf("atlasGenerateBlenderTerrain - unable to open '%s' for input!", argv[2]);
        return false;
    }

    Con::printf("   o Opening '%s' for opacity data...", argv[3]);
    inOpacityFile = AtlasFile::load(argv[3]);
    if (inOpacityFile.isNull())
    {
        Con::errorf("atlasGenerateBlenderTerrain - unable to open '%s' for input!", argv[3]);
        return false;
    }

    Con::printf("   o Opening '%s' for lightmap data...", argv[4]);
    inLightmapFile = AtlasFile::load(argv[4]);
    if (inLightmapFile.isNull())
    {
        Con::errorf("atlasGenerateBlenderTerrain - unable to open '%s' for input!", argv[4]);
        return false;
    }

    Con::printf("   o Copying TOCs...");

    // Now create the new file.
    AtlasFile outFile;

    // Copy the TOCs into the new file.

    // Geometry first.
    AtlasResourceGeomTOC* originalArgtoc = NULL;
    if (!inGeometryFile->getTocBySlot(0, originalArgtoc))
    {
        Con::errorf("atlasGenerateBlenderTerrain - inGeometryFile does not have a Geometry TOC.");
        return false;
    }

    AtlasResourceGeomTOC* argtoc = new AtlasResourceGeomTOC;
    argtoc->copyFromTOC(originalArgtoc);
    outFile.registerTOC(argtoc);

    // Now do the textures...

    // Opacity first.
    AtlasResourceTexTOC* originalArttocOM = NULL;
    if (!inOpacityFile->getTocBySlot(0, originalArttocOM))
    {
        Con::errorf("atlasGenerateBlenderTerrain - inOpacityFile does not have a Texture TOC.");
        return false;
    }

    AtlasResourceTexTOC* arttocOM = new AtlasResourceTexTOC;
    arttocOM->copyFromTOC(originalArttocOM);
    outFile.registerTOC(arttocOM);

    // Lightmap second.
    AtlasResourceTexTOC* originalArttocLM = NULL;
    if (!inLightmapFile->getTocBySlot(0, originalArttocLM))
    {
        Con::errorf("atlasGenerateBlenderTerrain - inLightmapFile does not have a Texture TOC.");
        return false;
    }

    AtlasResourceTexTOC* arttocLM = new AtlasResourceTexTOC;
    arttocLM->copyFromTOC(originalArttocLM);
    outFile.registerTOC(arttocLM);

    // We also need a config TOC.
    Con::printf("   o Generating config TOC...");
    AtlasResourceConfigTOC* arctoc = new AtlasResourceConfigTOC;
    arctoc->initialize(1);
    outFile.registerTOC(arctoc->castToResourceTOC());

    // And we're set, write it out.
    Con::printf("   o Outputting new .atlas file headers...");
    if (!outFile.createNew(argv[1]))
    {
        Con::errorf("atlasGenerateBlenderTerrain - unable to open '%s' for output!", argv[1]);
        return false;
    }

    // We have to copy the chunks, too. Start up threads so we can write.
    Con::printf("   o Starting loader threads...");
    outFile.startLoaderThreads();

    Con::printf("   o Copying geometry chunks...");
    originalArgtoc->copyChunksToTOC(argtoc);

    Con::printf("   o Copying opacity map chunks...");
    originalArttocOM->copyChunksToTOC(arttocOM);

    Con::printf("   o Copying lightmap chunks...");
    originalArttocLM->copyChunksToTOC(arttocLM);

    // Finally, generate some configuration data.
    Con::printf("   o Generating config chunks...");
    AtlasConfigChunk* acc = new AtlasConfigChunk;
    acc->setEntry("type", "blender");
    acc->setEntry("opacityMapSlot", "0");
    acc->setEntry("shadowMapSlot", "1");

    // Validate the texture size.
    S32 virtTexSize = dAtoi(argv[5]);
    if (!isPow2(virtTexSize))
    {
        S32 nextLowest = BIT(getFastBinLog2(U32(virtTexSize)));
        S32 nextHighest = BIT(getFastBinLog2(U32(virtTexSize)) + 1);

        Con::errorf("Virtual texture size specified was not a power of 2. Did you mean %d or %d?", nextLowest, nextHighest);
        Con::errorf("ABORTED!");
        return false;
    }

    acc->setEntry("virtualTexSize", argv[5]);
    Con::printf("      - Virtual texture size is %d", virtTexSize);

    // How many source images are specified?
    S32 numSrcImages = argc - 6;
    Con::printf("      - Found %d source images specified.", numSrcImages);

    acc->setEntry("sourceImageCount", avar("%d", numSrcImages));
    for (S32 i = 0; i < numSrcImages; i++)
    {
        Con::printf("         * Source #%d = '%s'", i, argv[6 + i]);
        acc->setEntry(avar("sourceImage%d", i), argv[6 + i]);
    }

    arctoc->addConfig("schema", acc);

    // Let everything serialize...
    outFile.waitForPendingWrites();

    // Successful, indicate such!
    Con::printf("   o Done!");
    return true;
}

ConsoleFunction(atlasGenerateUniqueTerrain, bool, 4, 4, "(out, geometry, texture)"
    "Generate a terrain with a unique texture schema using the "
    "specified geometry and texture atlas files.")
{
    Con::printf("atlasGenerateUniqueTerrain - getting ready to generate a terrain with a unique schema...");

    Resource<AtlasFile> inGeometryFile, inTextureFile;

    // Load the geometry data.
    Con::printf("   o Opening '%s' for geometry...", argv[2]);
    inGeometryFile = AtlasFile::load(argv[2]);
    if (inGeometryFile.isNull())
    {
        Con::errorf("atlasGenerateUniqueTerrain - unable to open '%s' for input!", argv[2]);
        return false;
    }

    // Load the texture data.
    Con::printf("   o Opening '%s' for texture data...", argv[3]);
    inTextureFile = AtlasFile::load(argv[3]);
    if (inTextureFile.isNull())
    {
        Con::errorf("atlasGenerateUniqueTerrain - unable to open '%s' for input!", argv[3]);
        return false;
    }

    // Copy the TOCs.
    AtlasResourceGeomTOC* argtoc = NULL;
    AtlasResourceTexTOC* arttoc = NULL;

    Con::printf("   o Locating geometry TOC...");
    if (!inGeometryFile->getTocBySlot(0, argtoc))
    {
        Con::errorf("atlasGenerateUniqueTerrain - unable to find geometry data in '%s'", argv[2]);
        return false;
    }

    Con::printf("   o Locating texture TOC...");
    if (!inTextureFile->getTocBySlot(0, arttoc))
    {
        Con::errorf("atlasGenerateUniqueTerrain - unable to find texture data in '%s'", argv[3]);
        return false;
    }

    // Create the new Atlas file and its TOCs.
    AtlasFile outFile;

    Con::printf("   o Copying & registering geometry TOC...");
    AtlasResourceGeomTOC* newArgToc = new AtlasResourceGeomTOC;
    newArgToc->copyFromTOC(argtoc);
    outFile.registerTOC(newArgToc);

    Con::printf("   o Copying & registering texture TOC...");
    AtlasResourceTexTOC* newArtToc = new AtlasResourceTexTOC;
    newArtToc->copyFromTOC(arttoc);
    outFile.registerTOC(newArtToc);

    Con::printf("   o Registering config TOC...");
    AtlasResourceConfigTOC* arctoc = new AtlasResourceConfigTOC;
    arctoc->initialize(1);
    outFile.registerTOC(arctoc->castToResourceTOC());

    // Output the new atlas file.
    Con::printf("   o Creating atlas file '%s'...", argv[1]);
    if (!outFile.createNew(argv[1]))
    {
        Con::errorf("atlasGenerateUniqueTerrain - unable to output atlas file '%s'", argv[1]);
        return false;
    }

    Con::printf("   o Preparing for Atlas IO...");
    outFile.startLoaderThreads();

    // Generate config chunk.
    Con::printf("   o Generating texture schema configuration chunk...");

    AtlasConfigChunk* acc = new AtlasConfigChunk;
    acc->setEntry("type", "unique");
    acc->setEntry("textureSlot", "0");

    arctoc->addConfig("schema", acc);

    // Copy chunks...
    Con::printf("   o Copying geometry chunks...");
    argtoc->copyChunksToTOC(newArgToc);

    Con::printf("   o Copying texture chunks...");
    arttoc->copyChunksToTOC(newArtToc);

    // Let everything serialize...
    outFile.waitForPendingWrites();

    // Done!
    Con::printf("   o Done!");
    return true;
}

ConsoleFunction(atlasDumpFileInfo, void, 3, 3, "(atlasFile)"
    "Dump information about the specified Atlas file, including "
    "TOC and efficiency information.")
{
    Con::printf("atlasDumpFileInfo - not implemented yet -- BJG");

    // Open the AtlasFile.

    // Dump header summary

    // Dump each TOC
       // Type
       // Type-specific info.
       // Dump per-TOC chunk statistics

    // Dump general TOC statistics.
}

ConsoleFunction(atlasConvertTextureTOC, bool, 4, 4, "(inFile, outFile, format)"
    "Convert the single texture TOC contained in inFile to be "
    "format format, and write the results into new atlas file "
    "outFile. Options for format are: 0=JPEG,1=PNG,2=DDS.")
{
    Con::printf("atlasConvertTextureTOC - Opening input atlas file '%s'...", argv[1]);

    Resource<AtlasFile> afIn = AtlasFile::load(argv[1]);

    if (afIn.isNull())
    {
        Con::errorf("atlasConvertTextureTOC - cannot open atlas file '%s'!", argv[1]);
        return false;
    }

    // Grab the AtlasResourceTexTOC...
    Con::printf("atlasConvertTextureTOC - Locating texture TOC...");
    AtlasResourceTexTOC* arttoc;

    if (!afIn->getTocBySlot(0, arttoc))
    {
        Con::errorf("atlasConvertTextureTOC - cannot load texture TOC from atlas file '%s'!", argv[1]);
        return false;
    }

    // Alright, new atlas file and let's copy into it...
    Con::printf("atlasConvertTextureTOC - Allocating new AtlasFile...");
    AtlasFile afOut;

    Con::printf("atlasConvertTextureTOC - Copying texture TOC...");
    AtlasResourceTexTOC* arttocNew = new AtlasResourceTexTOC;
    arttocNew->copyFromTOC(arttoc);
    afOut.registerTOC(arttocNew);

    Con::printf("atlasConvertTextureTOC - Creating new atlas file '%s'...", argv[2]);
    if (!afOut.createNew(argv[2]))
    {
        Con::errorf("atlasConvertTextureTOC - cannot create new atlas file '%s'!", argv[2]);
        SAFE_DELETE(arttocNew);
        return false;
    }


    Con::printf("atlasConvertTextureTOC - Starting threaded IO...");
    afOut.startLoaderThreads();

    //  And copy our data
    Con::printf("atlasConvertTextureTOC - Copying & reformatting texture chunks...");
    arttoc->copyChunksToTOC(arttocNew, dAtoi(argv[3]));

    afOut.waitForPendingWrites();

    Con::printf("atlasConvertTextureTOC - Done!");
    return true;
}