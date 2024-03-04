//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "zlib.h"
#include "console/console.h"
#include "core/fileStream.h"
#include "core/resManager.h"
#include "util/fourcc.h"
#include "util/safeDelete.h"

#include "atlas/core/atlasFile.h"
#include "atlas/resource/atlasResourceGeomTOC.h"
#include "atlas/resource/atlasResourceTexTOC.h"

#include "atlas/editor/atlasDiscreteMesh.h"


struct OldCHUHeader
{
    U32 fourCC;
    S16 version;
    S16 treeDepth;
    F32 baseMaxError, verticalScale, leafDimensions;
    U32 chunkCount, colTreeDepth;
};

struct OldCHUStub
{
    U32 sentinel; // == 0xDEADBEEF
    S32 label;
    S32 neighbors[4];
    U8  level;
    S16 x, y;
    S16 min, max;
    U32 chunkOffset;
};

struct OldCHUChunk
{
    U32 sentinel; // == 0xbeef1234

    U16 vertCount;
    struct Vert
    {
        S16 x, y, z, morph;
    } *verts;

    S32 indexCount;
    U16* index;
    U32 triCount;
    U8 colDataFlag; // 0 or 1

    // if(colDataFlag)
    // {
    //for each coltree node (depth in tree)
    struct ColTreeNode
    {
        S16 min, max;
    };
    ColTreeNode* colTreeNodes;

    U32 sent2; //== 0xb33fd34d
    U16* colIndicesOffsets; // [colGridCount]
    U32 bufferSize;
    U16* colIndexBuffer; //[bufferSize]

    // }

    U32 post; // == 0xb1e2e3f4

    OldCHUChunk()
    {
        pos = NULL;
        morph = NULL;
        texCoord = NULL;
        verts = NULL;
        index = NULL;
        colIndicesOffsets = NULL;
        colIndexBuffer = NULL;
    }

    ~OldCHUChunk()
    {
        SAFE_DELETE_ARRAY(verts);
        SAFE_DELETE_ARRAY(index);
        SAFE_DELETE_ARRAY(colIndicesOffsets);
        SAFE_DELETE_ARRAY(colIndexBuffer);
        SAFE_DELETE_ARRAY(pos);
        SAFE_DELETE_ARRAY(morph);
        SAFE_DELETE_ARRAY(texCoord);
    }

    bool read(Stream& chuFs, U32 colTreeCount, U32 colTreeGridCount)
    {
        chuFs.read(&sentinel);
        if (sentinel != 0xbeef1234)
        {
            Con::errorf("OldCHUChunk::read - invalid sentinel(1)!");
            return false;
        }

        chuFs.read(&vertCount);
        verts = new Vert[vertCount];
        for (S32 i = 0; i < vertCount; i++)
        {
            chuFs.read(&verts[i].x);
            chuFs.read(&verts[i].y);
            chuFs.read(&verts[i].z);
            chuFs.read(&verts[i].morph);
        }

        chuFs.read(&indexCount);
        index = new U16[indexCount];
        for (S32 i = 0; i < indexCount; i++)
            chuFs.read(index + i);

        chuFs.read(&triCount);

        chuFs.read(&colDataFlag);

        if (colDataFlag == 1)
        {
            // Got collision data, read it in.

            colTreeNodes = new ColTreeNode[colTreeCount];
            for (S32 i = 0; i < colTreeCount; i++)
            {
                chuFs.read(&colTreeNodes[i].min);
                chuFs.read(&colTreeNodes[i].max);
            }

            chuFs.read(&sent2);
            if (sent2 != 0xb33fd34d)
            {
                Con::errorf("OldCHUChunk::read - invalid sentinel(2, collision)!");
                return false;
            }

            colIndicesOffsets = new U16[colTreeGridCount];
            for (S32 i = 0; i < colTreeGridCount; i++)
                chuFs.read(&colIndicesOffsets[i]);

            chuFs.read(&bufferSize);
            colIndexBuffer = new U16[bufferSize];
            for (S32 i = 0; i < bufferSize; i++)
                chuFs.read(&colIndexBuffer[i]);
        }
        else if (colDataFlag == 0)
        {
            // Do nothing...
        }
        else
        {
            Con::errorf("OldCHUChunk::read - unexpected value for colDataFlag (%d, not 0 or 1)!", colDataFlag);
            return false;
        }

        // Check footer sentinel.
        chuFs.read(&post);
        if (post != 0xb1e2e3f4)
        {
            Con::errorf("OldCHUChunk::read - invalid sentinel(3)!");
            return false;
        }

        return true;
    }

    Point3F* pos;
    Point3F* morph;
    Point2F* texCoord;

    /// Given loaded data, go through and generate vertex arrays suitable
    /// for use in the new Atlas infrastructure.
    void generateVertexArrays(F32 verticalScale, F32 chunkSize, Point2F chunkOffset, Point2F terrainSize)
    {
        SAFE_DELETE_ARRAY(pos);
        SAFE_DELETE_ARRAY(morph);
        SAFE_DELETE_ARRAY(texCoord);

        pos = new Point3F[vertCount];
        morph = new Point3F[vertCount];
        texCoord = new Point2F[vertCount];

        for (S32 i = 0; i < vertCount; i++)
        {
            // Decompress pos.
            pos[i].set(
                ((F32(verts[i].x) / F32(1 << 14)) * 0.5 + 0.5) * chunkSize + chunkOffset.x,
                ((F32(verts[i].y) / F32(1 << 14)) * 0.5 + 0.5) * chunkSize + chunkOffset.y,
                F32(verts[i].z) * verticalScale
            );

            // We need to generate our texcoords in context of the whole terrain.
            // 
            // Take our corrected position (above) and cook into "our" section
            // of the terrain, based on the terrain bounds.
            texCoord[i].set(pos[i].y / terrainSize.y, pos[i].x / terrainSize.x);

            // Decompress and expand morph.
            morph[i].set(0, 0, F32(verts[i].morph) * verticalScale);
        }
    }
};

struct OldTQTHeader
{
    U32 fourCC;
    U32 version;
    U32 treeDepth;
    U32 tileSize;
};

ConsoleFunction(importOldAtlasTQT, void, 3, 3, "(oldTQT, newAtlasFile)")
{
    // Let's open our input files.
    Stream* tqtFs;
    if (!ResourceManager->openFileForWrite(tqtFs, argv[1], FileStream::Read))
    {
        Con::errorf("importOldAtlasTQT - could not open tqt file '%s' for read.", argv[1]);
        return;
    }

    Con::printf("importOldAtlasTQT - opened '%s', processing...", argv[1]);

    // Now do the TQT header.
    OldTQTHeader tqtHeader;
    tqtFs->read(&tqtHeader.fourCC);
    if (tqtHeader.fourCC != MAKEFOURCC('t', 'q', 't', 0))
    {
        Con::warnf("importOldAtlasTQT - might have invalid tqt fourcc.");
    }

    tqtFs->read(&tqtHeader.version);
    tqtFs->read(&tqtHeader.treeDepth);
    tqtFs->read(&tqtHeader.tileSize);

    // Allocate a new AtlasFile and create TOCs with the settings from our
    // source headers.
    AtlasFile af;

    AtlasResourceTexTOC* arttoc = new AtlasResourceTexTOC();
    arttoc->initializeTOC(tqtHeader.treeDepth);
    af.registerTOC(arttoc);

    if (!af.createNew(argv[2]))
    {
        Con::errorf("importOldAtlasTQT - could not create new Atlas file '%s'", argv[2]);
        delete tqtFs;
        return;
    }

    Con::printf("importOldAtlasTQT - created new Atlas file '%s'...", argv[2]);

    af.startLoaderThreads();

    Con::printf("importOldAtlasTQT - initialized Atlas file, converting...");

    // TQT is easy, as we have a list of offsets, and each chunk has length + a DDS.
    Vector<U32> tqtOffsets;
    U32 tqtCount = arttoc->getNodeCount(tqtHeader.treeDepth);
    tqtOffsets.reserve(tqtCount);

    for (S32 i = 0; i < tqtCount; i++)
        tqtFs->read(&tqtOffsets[i]);

    Con::printf("importOldAtlasTQT - instating textures...");

    // Same deal as before - walk the quadtree, instate new data.
    for (S32 i = 0; i < tqtHeader.treeDepth; i++)
    {
        for (S32 x = 0; x < BIT(i); x++)
        {
            for (S32 y = 0; y < BIT(i); y++)
            {
                U32 ni = arttoc->getNodeIndex(i, Point2I(y, x));
                AtlasResourceTexStub* arts = arttoc->getStub(i, Point2I(x, y));
                U32 offset = tqtOffsets[ni];

                // Ok, we have our stubs, so let's do some load and process.
                tqtFs.setPosition(offset);

                // Read size, then data into a buffer.
                U32 chunkSize;
                tqtFs->read(&chunkSize);

                U8* texBuffer = new U8[chunkSize];
                tqtFs->read(chunkSize, texBuffer);

                // We have a loaded chunk, so let's convert it to our new chunk format.
                AtlasTexChunk* agt = new AtlasTexChunk;
                agt->mFormat = AtlasTexChunk::FormatJPEG;
                agt->bitmap = AtlasTexChunk::loadDDSIntoGBitmap(texBuffer, chunkSize);

                delete[] texBuffer;

                arttoc->instateNewChunk(arts, agt, true);

                // And kill it so we save memory.
                arts->purge();
            }
        }

        Con::printf(" wrote all of level %d (%dx%d)", i, BIT(i), BIT(i));
    }

    // Wait for everything to finish serializing, then return.
    af.waitForPendingWrites();

    Con::printf("importOldAtlasTQT - Done");

    delete tqtFs;
}

ConsoleFunction(importOldAtlasCHU, void, 3, 3, "(oldCHU, newAtlasFile)")
{
    // Let's open our input files.
    Stream* chuFs;
    if (!ResourceManager->openFileForWrite(chuFs, argv[1], FileStream::Read))
    {
        Con::errorf("importOldAtlasCHU - could not open chu file '%s' for read.", argv[1]);
        return;
    }

    Con::printf("importOldAtlasCHU - opened '%s' for conversion.", argv[1]);

    // Now, get the header information so we can create our TOCs.

    // First do the CHU header.
    OldCHUHeader chuHeader;

    chuFs->read(&chuHeader.fourCC);
    if (chuHeader.fourCC != MAKEFOURCC('C', 'H', 'U', '3'))
    {
        Con::errorf("importOldAtlasCHU - invalid fourCC on CHU.");
        delete chuFs;
        return;
    }

    chuFs->read(&chuHeader.version);
    if (chuHeader.version != 400)
    {
        Con::errorf("importOldAtlasCHU - expected CHU version 400, encountered version %d", chuHeader.version);
        delete chuFs;
        return;
    }

    chuFs->read(&chuHeader.treeDepth);
    chuFs->read(&chuHeader.baseMaxError);
    chuFs->read(&chuHeader.verticalScale);
    chuFs->read(&chuHeader.leafDimensions);
    chuFs->read(&chuHeader.chunkCount);
    chuFs->read(&chuHeader.colTreeDepth);

    // Allocate a new AtlasFile and create TOCs with the settings from our
    // source headers.
    AtlasFile af;

    AtlasResourceGeomTOC* argtoc = new AtlasResourceGeomTOC();
    argtoc->initializeTOC(chuHeader.treeDepth);
    af.registerTOC(argtoc);

    if (!af.createNew(argv[2]))
    {
        Con::errorf("importOldAtlasCHU - could not create new Atlas file '%s'", argv[2]);
        delete chuFs;
        return;
    }

    Con::printf("importOldAtlasCHU - created new Atlas file '%s'", argv[2]);

    af.startLoaderThreads();

    Con::printf("importOldAtlasCHU - Atlas file initialized, converting...");

    // Now, copy chunks from the CHU in. First read all the stubs.
    Vector<OldCHUStub> chuStubs;
    chuStubs.reserve(chuHeader.chunkCount);

    for (U32 i = 0; i < chuHeader.chunkCount; i++)
    {
        chuStubs.increment();
        OldCHUStub& ocs = chuStubs.last();

        chuFs->read(&ocs.sentinel);

        if (ocs.sentinel != 0xDEADBEEF)
        {
            Con::errorf("importOldAtlasCHU - invalid chunk sentinel in CHU. (chunk #%d)", i);
            delete chuFs;
            return;
        }

        chuFs->read(&ocs.label);

        for (S32 i = 0; i < 4; i++)
            chuFs->read(&ocs.neighbors[i]);

        chuFs->read(&ocs.level);
        chuFs->read(&ocs.x);
        chuFs->read(&ocs.y);
        chuFs->read(&ocs.min);
        chuFs->read(&ocs.max);
        chuFs->read(&ocs.chunkOffset);
    }

    Con::printf("importOldAtlasCHU - Headers read, remapping chunks...");

    // We have to do a remap step as the headers get written to the CHU in any
    // order.
    Vector<OldCHUStub*> chuStubMap;
    chuStubMap.reserve(chuStubs.size());

    for (S32 i = 0; i < chuStubs.size(); i++)
        chuStubMap[chuStubs[i].label] = &chuStubs[i];

    // Ok, remapped, now we go through and instate each chunk. We do this by
    // walking the whole quadtree, getting the corresponding old, generating
    // a chunk, and instating it into the appropriate new.

    Con::printf("importOldAtlasCHU - Importing geometry chunks...");

    Point2F terrainSize;
    terrainSize.x = terrainSize.y = chuHeader.leafDimensions * F32(BIT(chuHeader.treeDepth - 1));

    // Actually, they're the same index schemes which simplifies things a bit.
    for (S32 level = chuHeader.treeDepth - 1; level >= 0; level--)
    {
        Con::printf("importOldAtlasCHU -    level %d chunks...", level);
        for (S32 x = 0; x < BIT(level); x++)
        {
            for (S32 y = 0; y < BIT(level); y++)
            {
                U32 ni = argtoc->getNodeIndex(level, Point2I(y, x)); // Note x/y flip! -- BJG
                AtlasResourceGeomStub* args = argtoc->getStub(level, Point2I(x, y));
                OldCHUStub* oldStub = chuStubMap[ni];

                // Ok, we have our stubs, so let's do some load and process.
                chuFs->setPosition(oldStub->chunkOffset);
                OldCHUChunk curChunk;
                if (!curChunk.read(*chuFs, argtoc->getNodeCount(chuHeader.colTreeDepth),
                    BIT(chuHeader.colTreeDepth - 1) * BIT(chuHeader.colTreeDepth - 1)))
                {
                    Con::errorf("importOldAtlasCHU - error reading chunk #%d (%d,%d@%d)!", ni, level, x, y);
                    delete chuFs;
                    return;
                }

                // We have a loaded chunk, so let's convert it to our new chunk format.

                // We generate our own collision data, so we can skip the existing
                // data in the .chu.

                // Grab the geometry, stuff it into an ADM, and install the ADM
                // in a chunk.
                AtlasDiscreteMesh adm;
                adm.mOwnsData = false;
                adm.mHasMorphData = true;
                adm.mIndexCount = curChunk.indexCount;
                adm.mIndex = curChunk.index;
                adm.mVertexCount = curChunk.vertCount;

                // Generate our vertex data.
                F32 chunkSize = chuHeader.leafDimensions * BIT((chuHeader.treeDepth - 1) - oldStub->level);

                curChunk.generateVertexArrays(
                    chuHeader.verticalScale, chunkSize,
                    Point2F(chunkSize * oldStub->x, chunkSize * oldStub->y),
                    terrainSize);

                adm.mPos = curChunk.pos;
                adm.mPosMorphOffset = curChunk.morph;
                adm.mTex = curChunk.texCoord;

                // Set some defaults for morph and normal data.
                adm.mTexMorphOffset = new Point2F[curChunk.vertCount];
                adm.mNormal = new Point3F[curChunk.vertCount];
                for (S32 i = 0; i < curChunk.vertCount; i++)
                {
                    adm.mTexMorphOffset[i].set(0, 0);
                    adm.mNormal[i].set(0, 0, 1);
                }

                // Then instate the chunk and potentially update the stub.
                AtlasGeomChunk* agc = new AtlasGeomChunk;
                agc->copyFromDiscreteMesh(&adm);

                // Update bounds from children as well, if appropriate.
                args->mBounds = adm.calcBounds();
                if (level < chuHeader.treeDepth - 1)
                {
                    const U8 offsets[4][2] =
                    {
                       {0,0},
                       {0,1},
                       {1,1},
                       {1,0},
                    };

                    for (S32 j = 0; j < 4; j++)
                    {
                        AtlasResourceGeomStub* childArgs =
                            argtoc->getStub(level + 1,
                                Point2I((x * 2) + offsets[j][0], (y * 2) + offsets[j][1]));

                        args->mBounds.min.setMin(childArgs->mBounds.min);
                        args->mBounds.max.setMax(childArgs->mBounds.max);

                        AssertFatal(args->mBounds.isContained(childArgs->mBounds),
                            "importOldAtlasCHU - child not contained by parent!");

                    }
                }

                // Make sure we match stub and chunk bounds.
                agc->mBounds = args->mBounds;

                // Update collision if needed. We MUST do this after other bound
                // update operations, as collision lookup info is encoded based 
                // on the bounds.
                if (level == chuHeader.treeDepth - 1)
                    agc->generateCollision();

                // Instate the chunk
                argtoc->instateNewChunk(args, agc, true);

                // And clean up once we're done, minimize memory consumption.
                args->purge();
            }
        }
    }

    // Wait for everything to finish serializing, then return.
    af.waitForPendingWrites();

    Con::printf("importOldAtlasCHU - Done");

    delete chuFs;
}