//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------
#include "platform/platform.h"
#include "core/fileStream.h"
#include "console/console.h"
#include "atlas/core/atlasFile.h"
#include "atlas/core/atlasBaseTOC.h"
#include "atlas/resource/atlasResourceGeomTOC.h"
#include "atlas/resource/atlasResourceTexTOC.h"
#include "atlas/editor/atlasDiscreteMesh.h"

class HeightfieldReader
{
public:
    HeightfieldReader(U32 size)
    {
        mSize.x = size;
        mSize.y = size;

        mHeights = new F32[size * size];
    }

    Point2I mSize;
    F32* mHeights;

    inline const U32 nodeIndex(const Point2I pos) const
    {
        return pos.x * mSize.x + pos.y;
    }

    inline F32 sample(const Point2I pos) const
    {
        return mHeights[nodeIndex(pos)];
    }

    bool read(Stream& s)
    {
        // Let's just blast it in...
        F32* cur = mHeights;

        for (S32 i = 0; i < mSize.x * mSize.y; i++)
        {
            U16 h;
            s.read(&h);

            *cur = F32(h) / 16.0;
            cur++;
        }

        return true;
    }
};

ConsoleFunction(atlasGenerateGeometryFromHeightfield, void, 6, 6,
    "(atlasOutFile, inHeightfield, hfSize, gridSpacing, patchSize) - generate terrain geometry "
    "to atlasOutFile based on the heightfield.")
{
    const U32 hfSize = dAtoi(argv[3]);
    const F32 gridSpacing = dAtof(argv[4]);
    const U32 tileSize = dAtoi(argv[5]);

    // Caculate treeDepth...
    const S32 treeDepth = getBinLog2(hfSize / tileSize) + 1;

    Con::errorf("***************************************************************");
    Con::errorf("");
    Con::errorf("BE AWARE - THIS FUNCTION IS STILL IN DEVELOPMENT AND WILL NOT GIVE USEFUL RESULTS.");
    Con::errorf("PLEASE USE THE atlasOldGenerateChunkFileFromRaw16 method AND importOldAtlasCHU");
    Con::errorf("TO GENERATE GEOMETRY FOR NOW.");
    Con::errorf("");
    Con::errorf("Now generating geometry...");
    Con::errorf("");
    Con::errorf("***************************************************************");

    Con::printf("atlasGenerateGeometryFromHeightfield - Preparing...");

    // Ok, we now have a live atlas file.
    AtlasFile af;

    // Put some TOCs into it.
    Con::printf("   o Preparing geometry TOC.");
    AtlasResourceGeomTOC* gtoc = new AtlasResourceGeomTOC();
    gtoc->initializeTOC(treeDepth);
    af.registerTOC(gtoc);

    Con::printf("   o Writing atlas file headers...");
    if (!af.createNew(argv[1]))
    {
        Con::errorf("atlasGenerateGeometryFromHeightfield - cannot create new atlas file '%s'", argv[1]);
        return;
    }

    Con::printf("   o Starting loader threads...");
    af.startLoaderThreads();

    // Generate a mesh from a source heightfield.

    // We have to generate proper tiles. The base level chunks are made
    // by us, and then combined by the usual generation process.

    // For now let's have the base LOD be at full detail, and let it simplify
    // down from there.

    // So we make them and stuff them into the TOC. Then we can just
    // kick off a standard rebuild.

    // Open the heightfield.
    Con::printf("   o Opening %dx%d heightfield ('%s') for processing...", hfSize + 1, hfSize + 1, argv[2]);

    FileStream fs;
    if (!fs.open(argv[2], FileStream::Read))
    {
        Con::errorf("atlasGenerateGeometryFromHeightfield - cannot open raw heightfield '%s'", argv[2]);
        return;
    }

    HeightfieldReader hr(hfSize + 1);
    hr.read(fs);

    // Ok, now let's start generating tiles...
    Con::printf("   o Generating index buffer...");

    Vector<GFXAtlasVert2> points;
    Vector<U16>           indices;
    GFXAtlasVert2         tmpV;

    // Let's reserve some space so we don't thrash memory.
    points.reserve(2 * (tileSize + 1) * (tileSize + 1));
    indices.reserve(6 * (tileSize) * (tileSize));

    // We can pregenerate the indices, as it's the same for every tile.
    for (S32 x = 0; x < tileSize; x++)
    {
        for (S32 y = 0; y < tileSize; y++)
        {
            // Two triangles...
            const U32 stride = tileSize + 1;
            indices.push_back((x + 0) * stride + (y + 0));
            indices.push_back((x + 0) * stride + (y + 1));
            indices.push_back((x + 1) * stride + (y + 1));

            indices.push_back((x + 1) * stride + (y + 0));
            indices.push_back((x + 0) * stride + (y + 0));
            indices.push_back((x + 1) * stride + (y + 1));
        }
    }

    Con::printf("   o Generating vertex data...");
    const F32 patchSize = gridSpacing * F32(hfSize / tileSize);

    for (S32 i = 0; i < hfSize / tileSize; i++)
    {
        for (S32 j = 0; j < hfSize / tileSize; j++)
        {
            points.clear();
            dMemset(&tmpV, 0, sizeof(tmpV));

            // generate points
            for (S32 x = 0; x <= tileSize; x++)
            {
                for (S32 y = 0; y <= tileSize; y++)
                {
                    tmpV.point.x = F32(x) / F32(tileSize) * patchSize + F32(i) * patchSize;
                    tmpV.point.y = F32(y) / F32(tileSize) * patchSize + F32(j) * patchSize;
                    tmpV.point.z = hr.sample(Point2I(i * tileSize, j * tileSize) + Point2I(x, y));

                    tmpV.texCoord.x = F32(x) / F32(tileSize);
                    tmpV.texCoord.y = F32(y) / F32(tileSize);

                    tmpV.normal.x = 0;
                    tmpV.normal.y = 0;
                    tmpV.normal.z = 1;

                    points.push_back(tmpV);
                }
            }

            // Prep a chunk & copy our stuff into it.
            AtlasGeomChunk* agc = new AtlasGeomChunk;

            // Indices...
            agc->mIndexCount = indices.size();
            agc->mIndex = new U16[indices.size()];
            dMemcpy(agc->mIndex, indices.address(), indices.size() * sizeof(U16));

            // Vertices...
            agc->mVertCount = points.size();
            agc->mVert = new GFXAtlasVert2[points.size()];
            dMemcpy(agc->mVert, points.address(), points.size() * sizeof(GFXAtlasVert2));

            // Update the bounds. This is a hack.
            AtlasDiscreteMesh* adm = agc->copyToDiscreteMesh();
            agc->mBounds = adm->calcBounds();
            delete adm;

            // And generate collision info.
            agc->generateCollision();

            // Get the stub.
            AtlasResourceGeomStub* gStub = gtoc->getStub(treeDepth - 1, Point2I(i, j));

            // And store the chunk.
            gtoc->instateNewChunk(gStub, agc, true);

            // Purge it once we're done so we don't use gigs of ram!
            gStub->purge();

            // All done, do next one!
        }
    }

    Con::printf("   o Calculating LOD hierarchy...");

    // Now that we're all done with our base chunks, let's regenerate.
    gtoc->generate(RectI(0, 0, hfSize / tileSize, hfSize / tileSize));

    // Let's wait a bit to make sure everything gets purged.
    af.waitForPendingWrites();

    Con::printf("   o Done!");
}
