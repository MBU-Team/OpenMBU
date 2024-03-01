//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "ts/tsShape.h"
#include "ts/tsShapeInstance.h"

// methods in this file are used for reading pre-version 19 shapes
// methods for reading/writing sequences still used...

#define OldPageSize 25000 // old page size must be mutliple of 4 so that we can always "over-read" up to next dword

struct OldAlloc
{
    static S32 sz32;
    static S32 cnt32;

    static S32 sz16;
    static S32 cnt16;

    static S32 sz8;
    static S32 cnt8;

    static S32 guard32;
    static S16 guard16;
    static S8 guard8;
};

S32 OldAlloc::sz32;
S32 OldAlloc::cnt32;
S32 OldAlloc::sz16;
S32 OldAlloc::cnt16;
S32 OldAlloc::sz8;
S32 OldAlloc::cnt8;
S32 OldAlloc::guard32;
S16 OldAlloc::guard16;
S8 OldAlloc::guard8;

S32 getDWordCount32()
{
    return OldAlloc::cnt32;
}

S32 getDWordCount16()
{
    if (OldAlloc::cnt16 & 1)
        return (OldAlloc::cnt16 + 2) >> 1;
    else
        return OldAlloc::cnt16 >> 1;
}

S32 getDWordCount8()
{
    if (OldAlloc::cnt8 & 3)
        return (OldAlloc::cnt8 + 4) >> 2;
    else
        return OldAlloc::cnt8 >> 2;
}

S32* oldInitAlloc32()
{
    OldAlloc::sz32 = 0;
    OldAlloc::cnt32 = 0;
    OldAlloc::guard32 = 0;
    return NULL;
}

S16* oldInitAlloc16()
{
    OldAlloc::sz16 = 0;
    OldAlloc::cnt16 = 0;
    OldAlloc::guard16 = 0;
    return NULL;
}

S8* oldInitAlloc8()
{
    OldAlloc::sz8 = 0;
    OldAlloc::cnt8 = 0;
    OldAlloc::guard8 = 0;
    return NULL;
}

S32 oldAllocOffset(S32*)
{
    return OldAlloc::cnt32;
}

S32 oldAllocOffset(S16*)
{
    return OldAlloc::cnt16;
}

S8* oldAlloc(S32*& addr, S32 count)
{
    if (OldAlloc::cnt32 + count > OldAlloc::sz32)
    {
        S32 numPages = 1 + ((OldAlloc::cnt32 + count) / OldPageSize);
        OldAlloc::sz32 = numPages * OldPageSize;
        S32* tmp = new S32[OldAlloc::sz32];
        if (addr)
            dMemcpy((U8*)tmp, (U8*)addr, OldAlloc::cnt32 * sizeof(U32));
        delete[] addr;
        addr = tmp;
    }
    S8* ret = (S8*)&addr[OldAlloc::cnt32];
    OldAlloc::cnt32 += count;
    return ret;
}

S8* oldAlloc(S16*& addr, S32 count)
{
    if (OldAlloc::cnt16 + count > OldAlloc::sz16)
    {
        S32 numPages = 1 + ((OldAlloc::cnt16 + count) / OldPageSize);
        OldAlloc::sz16 = numPages * OldPageSize;
        S16* tmp = new S16[OldAlloc::sz16];
        if (addr)
            dMemcpy((U8*)tmp, (U8*)addr, OldAlloc::cnt16 * sizeof(S16));
        delete[] addr;
        addr = tmp;
    }
    S8* ret = (S8*)&addr[OldAlloc::cnt16];
    OldAlloc::cnt16 += count;
    return ret;
}

S8* oldAlloc(S8*& addr, S32 count)
{
    if (OldAlloc::cnt8 + count > OldAlloc::sz8)
    {
        S32 numPages = 1 + ((OldAlloc::cnt8 + count) / OldPageSize);
        OldAlloc::sz8 = numPages * OldPageSize;
        S8* tmp = new S8[OldAlloc::sz8];
        if (addr)
            dMemcpy((U8*)tmp, (U8*)addr, OldAlloc::cnt8);
        delete[] addr;
        addr = tmp;
    }
    S8* ret = (S8*)&addr[OldAlloc::cnt8];
    OldAlloc::cnt8 += count;
    return ret;
}

S32 readAlloc32(Stream* s, S32*& memBuffer32)
{
    S32 tmp;
    s->read(sizeof(tmp), (U8*)&tmp);  // this won't flip, which is perfect.
    U32* ptr = (U32*)oldAlloc(memBuffer32, 1);
    *ptr = tmp;
    return (S32)convertLEndianToHost(tmp); // return flipped value for use.
}

S32 readAlloc32(Stream* s, S32& storage)
{
    S32 tmp;
    s->read(sizeof(tmp), (U8*)&tmp); // this won't flip, which is perfect.
    storage = tmp;
    return (S32)convertLEndianToHost(tmp); // return flipped value
}

void readAlloc(Stream* s, S32*& memBuffer32, S32 size)
{
    s->read(sizeof(S32) * size, oldAlloc(memBuffer32, size));
}

void readAlloc(Stream* s, S16*& memBuffer16, S32 size)
{
    s->read(sizeof(S16) * size, oldAlloc(memBuffer16, size));
}

void readAlloc(Stream* s, S8*& memBuffer8, S32 size)
{
    s->read(sizeof(S8) * size, oldAlloc(memBuffer8, size));
}

void oldAllocGuard(S32*& addr32, S16*& addr16, S8*& addr8)
{
    S32* ptr32 = (S32*)oldAlloc(addr32, 1);
    *ptr32 = convertLEndianToHost(OldAlloc::guard32++); // flip, since tsShape will expect it.

    S16* ptr16 = (S16*)oldAlloc(addr16, 1);
    *ptr16 = convertLEndianToHost(OldAlloc::guard16++); // flip, since tsShape will expect it.

    S8* ptr8 = (S8*)oldAlloc(addr8, 1);
    *ptr8 = OldAlloc::guard8++;
}

#define DebugGuard() oldAllocGuard(memBuffer32,memBuffer16,memBuffer8)

void readAllocMesh(Stream* s, S32*& memBuffer32, S16*& memBuffer16, S8*& memBuffer8, U32 meshType)
{
    memBuffer8;

    if (meshType == TSMesh::NullMeshType)
        return;

    // standard mesh read

    DebugGuard();

    // numFrames, numMatFrames
    readAlloc(s, memBuffer32, 2);

    // parent mesh
    S32* ptr32 = (S32*)oldAlloc(memBuffer32, 1);
    *ptr32 = convertLEndianToHost(-1); // flip, since tsShape will flip back.

    // allocate memory for mBounds,mCenter, and mRadius...just filler, will be computed later
    oldAlloc(memBuffer32, 10);

    // read in verts
    S32 sz = readAlloc32(s, memBuffer32);
    readAlloc(s, memBuffer32, sz * 3);

    // read in tverts
    sz = readAlloc32(s, memBuffer32);
    readAlloc(s, memBuffer32, sz * 2);

    // read in normals
    s->read(&sz); // we could assume same as verts, but apparently in file.
    readAlloc(s, memBuffer32, sz * 3);

    // read in primitives
    sz = readAlloc32(s, memBuffer32);
    if (TSShape::smReadVersion < 18)
    {
        for (S32 j = 0; j < sz; j++)
        {
            S32 a, b;
            s->read(&a);
            s->read(&b);
            U16* ptr16 = (U16*)oldAlloc(memBuffer16, 2);
            ptr16[0] = convertLEndianToHost((S16)a); // flip, since tsShape will flip back.
            ptr16[1] = convertLEndianToHost((S16)b); // flip, since tsShape will flip back.
            readAlloc32(s, memBuffer32);
        }
    }
    else
    {
        for (S32 j = 0; j < sz; j++)
        {
            readAlloc(s, memBuffer16, 2);
            readAlloc(s, memBuffer32, 1);
        }
    }

    // read in indices
    sz = readAlloc32(s, memBuffer32);
    if (TSShape::smReadVersion < 18)
    {
        U32 idx;
        U16* idxPtr = (U16*)oldAlloc(memBuffer16, sz);
        for (S32 j = 0; j < sz; j++)
        {
            s->read(sizeof(idx), (U8*)&idx);
            idxPtr[j] = (U16)idx;
        }
    }
    else
        readAlloc(s, memBuffer16, sz);

    // mergeIndices...none
    ptr32 = (S32*)oldAlloc(memBuffer32, 1);
    *ptr32 = convertLEndianToHost((S32)0);

    // vertsPerFrame, flags
    readAlloc(s, memBuffer32, 2);

    DebugGuard();

    if (meshType == TSMesh::SkinMeshType)
    {
        // read in initial verts
        sz = readAlloc32(s, memBuffer32);
        readAlloc(s, memBuffer32, sz * 3);

        // read in initial normals
        s->read(&sz); // we assume same as verts
        readAlloc(s, memBuffer32, sz * 3);

        // read in initial transforms
        sz = readAlloc32(s, memBuffer32);
        readAlloc(s, memBuffer32, sz * 16);

        // read in vertexIndx
        sz = readAlloc32(s, memBuffer32);
        readAlloc(s, memBuffer32, sz);

        // read in boneIndex
        s->read(&sz); // toss
        readAlloc(s, memBuffer32, sz);

        // read in nodeIndex -- but let's move it...
        // vertexIndex, boneIndex, and weight have same # of entries
        // nodeIndex is different (it's number of bones)
        // first allocate room for weights, then read in nodeIndex list
        S32 weightStart = oldAllocOffset(memBuffer32);
        oldAlloc(memBuffer32, sz); // this is memory for the weights

        // actually read in node index
        sz = readAlloc32(s, memBuffer32);
        readAlloc(s, memBuffer32, sz); // read in nodeIndex array

        // read in weight
        s->read(&sz); // toss
        F32* ptr32 = (F32*)(memBuffer32 + weightStart);
        for (S32 i = 0; i < sz; i++)
            s->read(sizeof(F32), (U8*)(ptr32 + i));

        DebugGuard();
    }
    if (meshType == TSMesh::SortedMeshType)
    {
        // clusters...
        sz = readAlloc32(s, memBuffer32);
        readAlloc(s, memBuffer32, sz * 8);

        // start cluster...
        sz = readAlloc32(s, memBuffer32);
        readAlloc(s, memBuffer32, sz);

        // firstVert...
        sz = readAlloc32(s, memBuffer32);
        readAlloc(s, memBuffer32, sz);

        // numVerts...
        sz = readAlloc32(s, memBuffer32);
        readAlloc(s, memBuffer32, sz);

        // firstTVerts...
        sz = readAlloc32(s, memBuffer32);
        readAlloc(s, memBuffer32, sz);

        // always write z-depth?
        bool alwaysWriteZ;
        s->read(&alwaysWriteZ);
        S32* ptr32 = (S32*)oldAlloc(memBuffer32, 1);
        *ptr32 = alwaysWriteZ ? convertLEndianToHost(1) : convertLEndianToHost(0);  // flip, since tsShape will flip back.

        DebugGuard();
    }
    if (meshType == TSMesh::DecalMeshType)
    {
        // startPrimitive...
        sz = readAlloc32(s, memBuffer32);
        readAlloc(s, memBuffer32, sz);

        if (TSShape::smReadVersion >= 17)
        {
            // obsolete, so we read it all, but throw it all away.

            // startTVerts...
            s->read(&sz);
            S32 tmp;
            while (sz--)
                s->read(&tmp);

            // tvertIndex...
            s->read(&sz);
            while (sz--)
                s->read(&tmp);
        }

        // materialIndex...
        readAlloc32(s, memBuffer32);

        DebugGuard();
    }
}

// versioning hack...
Vector<S32> kfStart(__FILE__, __LINE__);

// NOTE: with version 17 and on there are no more keyframes...kept around for reading old shapes
struct Keyframe
{
    S32 firstNodeState;
    S32 firstObjectState;
    S32 firstDecalState;

    void read(Stream* s) { s->read(&firstNodeState); s->read(&firstObjectState); s->read(&firstDecalState); }
    void write(Stream* s) { s->write(firstNodeState); s->write(firstObjectState); s->write(firstDecalState); }
};

// read in and throw away
Vector<Keyframe> keyframes(__FILE__, __LINE__);

void TSShape::readOldShape(Stream* s,
    S32*& memBuffer32, S16*& memBuffer16, S8*& memBuffer8,
    S32& count32, S32& count16, S32& count8)
{
    S32 i, tmp;

    // first allocate some memory
    memBuffer32 = oldInitAlloc32();
    memBuffer16 = oldInitAlloc16();
    memBuffer8 = oldInitAlloc8();

    // allocate storage for vector counts
    oldAlloc(memBuffer32, 15);
    // contents of memBuffer32 might change, but eventually these will go here...
    // numNodes = memBuffer32[0];
    // numObjects = memBuffer32[1];
    // numDecals = memBuffer32[2];
    // numSubShapes = memBuffer32[3];
    // numIflMaterials = memBuffer32[4];
    // numNodeStates = memBuffer32[5];
    // numObjectStates = memBuffer32[6];
    // numDecalStates = memBuffer32[7];
    // numTriggers = memBuffer32[8];
    // numDetails = memBuffer32[9];
    // numMeshes = memBuffer32[10]
    // numSkins = memBuffer32[11];
    // numNames = memBuffer32[12];
    // mSmalletVisibleSize = memBuffer32[13];
    // mSmallestVisibleDL = memBuffer32[14];

    DebugGuard();

    // radius
    readAlloc(s, memBuffer32, 1);
    // tube radius
    readAlloc(s, memBuffer32, 1);
    // center
    readAlloc(s, memBuffer32, 3);
    // bounds
    readAlloc(s, memBuffer32, 6);

    DebugGuard();

    S32 numNodes = readAlloc32(s, memBuffer32[0]);
    S32 nodeStart = oldAllocOffset(memBuffer32);
    if (smReadVersion >= 17)
    {
        readAlloc(s, memBuffer32, numNodes * 2);
        // compute other 3 members at load time...
        // for now, allocate the storage and move things around
        oldAlloc(memBuffer32, numNodes * 3);
        S32* pNodeStart = memBuffer32 + nodeStart;
        for (i = numNodes - 1; i >= 0; i--)
            dMemmove(&pNodeStart[i * 5], &pNodeStart[i * 2], sizeof(S32) * 2);
    }
    else
    {
        // handle obsolete (bool) member
        for (i = 0; i < numNodes; i++)
        {
            readAlloc(s, memBuffer32, 2);
            bool obsolete;
            s->read(&obsolete);
            oldAlloc(memBuffer32, 3);
        }
    }

    DebugGuard();

    S32 numObjects = readAlloc32(s, memBuffer32[1]);
    S32 objectStart = oldAllocOffset(memBuffer32);
    readAlloc(s, memBuffer32, numObjects * 4);
    // compute other 2 members at load time...
    // for now, allocate storage and move things around
    oldAlloc(memBuffer32, numObjects * 2);
    S32* pObjectStart = memBuffer32 + objectStart;
    for (i = numObjects - 1; i >= 0; i--)
        dMemmove(&pObjectStart[i * 6], &pObjectStart[i * 4], sizeof(S32) * 4);

    DebugGuard();

    S32 numDecals = readAlloc32(s, memBuffer32[2]);
    S32 decalStart = oldAllocOffset(memBuffer32);
    readAlloc(s, memBuffer32, numDecals * 4);
    // compute other 1 member at load time...
    // for now, allocate storage and move things around
    oldAlloc(memBuffer32, numDecals);
    S32* pDecalStart = memBuffer32 + decalStart;
    for (i = numDecals - 1; i >= 0; i--)
        dMemmove(&pDecalStart[i * 5], &pDecalStart[i * 4], sizeof(S32) * 4);

    DebugGuard();

    S32 numIflMaterials = readAlloc32(s, memBuffer32[4]);
    S32 iflStart = oldAllocOffset(memBuffer32);
    readAlloc(s, memBuffer32, numIflMaterials * 2);
    // compute other 3 members at load time...
    // for now, allocate storage and move things around
    oldAlloc(memBuffer32, numIflMaterials * 3);
    S32* pIflStart = memBuffer32 + iflStart;
    for (i = numIflMaterials - 1; i >= 0; i--)
        dMemmove(&pIflStart[i * 5], &pIflStart[i * 2], sizeof(S32) * 2);

    DebugGuard();

    S32 numSubShapes = readAlloc32(s, memBuffer32[3]);
    S32 subShapeFirstStart = oldAllocOffset(memBuffer32);
    readAlloc(s, memBuffer32, numSubShapes); // subShapeFirstNode
    s->read(&tmp); // toss
    readAlloc(s, memBuffer32, numSubShapes); // subShapeFirstObject
    s->read(&tmp); // toss
    readAlloc(s, memBuffer32, numSubShapes); // subShapeFirstDecal

    DebugGuard();

    S32 subShapeNumStart = oldAllocOffset(memBuffer32);
    oldAlloc(memBuffer32, 3 * numSubShapes);  // allocate memory for subShapeNum* vectors

    DebugGuard();

    // compute subShapeNum* vectors
    S32* pSubShapeFirstStart = memBuffer32 + subShapeFirstStart;
    S32* pSubShapeNumStart = memBuffer32 + subShapeNumStart;
    S32 prev, first;
    for (i = 0; i < 3; i++)
    {
        prev = ((i == 0) ? numNodes : (i == 1 ? numObjects : numDecals));
        for (S32 j = numSubShapes - 1; j >= 0; j--)
        {
            first = convertLEndianToHost(pSubShapeFirstStart[j]); // flip to get value out
            pSubShapeNumStart[j] = prev - first;
            pSubShapeNumStart[j] = convertLEndianToHost(pSubShapeNumStart[j]); // flip to put value in.
            prev = first;
        }
        pSubShapeFirstStart += numSubShapes;
        pSubShapeNumStart += numSubShapes;
    }

    // if older than version 16, read in mesh index list for later use
    if (smReadVersion < 16)
    {
        S32 sz = readAlloc32(s, memBuffer32);
        readAlloc(s, memBuffer32, sz); // this is the meshIndexList
    }

    // if older than version 17, read in keyframes for later use
    // but don't add to shapes buffer because we'll be done with
    // them on exit from this method
    if (smReadVersion < 17)
    {
        S32 sz;
        s->read(&sz);
        keyframes.setSize(sz);
        for (i = 0; i < sz; i++)
            keyframes[i].read(s);
    }

    S32 numNodeStates = readAlloc32(s, memBuffer32[5]);
    S32 nodeStateStart32 = oldAllocOffset(memBuffer32);
    S32 nodeStateStart16 = oldAllocOffset(memBuffer16);
    for (i = 0; i < numNodeStates; i++)
    {
        readAlloc(s, memBuffer16, 4);     // read Quat16....rotation
        readAlloc(s, memBuffer32, 3);     // read Point3F...translation
    }

    DebugGuard();

    S32 numObjectStates = readAlloc32(s, memBuffer32[6]);
    S32 objectStateStart = oldAllocOffset(memBuffer32);
    readAlloc(s, memBuffer32, numObjectStates * 3);

    DebugGuard();

    S32 numDecalStates = readAlloc32(s, memBuffer32[7]);
    S32 decalStateStart = oldAllocOffset(memBuffer32);
    if (smReadVersion < 14)
    {
        // add in default decal state info
        S32* firstState = (S32*)oldAlloc(memBuffer32, numDecals);
        for (i = 0; i < numDecals; i++)
            firstState[i] = convertLEndianToHost(-1);
    }
    readAlloc(s, memBuffer32, numDecalStates);

    DebugGuard();

    S32 numTriggers = readAlloc32(s, memBuffer32[8]);
    readAlloc(s, memBuffer32, numTriggers * 2);

    DebugGuard();

    S32 numDetails = readAlloc32(s, memBuffer32[9]);
    S32 detailStart = oldAllocOffset(memBuffer32);
    readAlloc(s, memBuffer32, numDetails * 4);
    // compute other 3 members at load time...
    // for now, allocate storage and move things around
    oldAlloc(memBuffer32, numDetails * 3);
    S32* pDetailStart = memBuffer32 + detailStart;
    for (i = numDetails - 1; i >= 0; i--)
        dMemmove(&pDetailStart[i * 7], &pDetailStart[i * 4], 4 * sizeof(S32));
    // now find the smallest renderable detail level and size
    Detail* pd = (Detail*)pDetailStart;
    memBuffer32[13] = 0; // initialize to something valid
    memBuffer32[14] = 0; // initialize to something valid
    for (i = 0; i < numDetails; i++)
    {
        S32 dsize = (S32)convertLEndianToHost(pd[i].size);
        if (dsize >= 0.0f)
        {
            memBuffer32[13] = (S32)dsize;
            memBuffer32[14] = i;
        }
        pd[i].maxError = convertLEndianToHost(-1.0f);
        pd[i].averageError = convertLEndianToHost(-1.0f);
        pd[i].polyCount = convertLEndianToHost(0);
    }

    DebugGuard();

    // deal with sequences the old fashion way...
    kfStart.clear(); // for older shapes...
    S32 numSequences;
    s->read(&numSequences);
    sequences.setSize(numSequences);
    for (i = 0; i < numSequences; i++)
    {
        constructInPlace(&sequences[i]);
        sequences[i].read(s);
    }

    // read meshes
    S32 numMeshes = readAlloc32(s, memBuffer32[10]);
    for (i = 0; i < numMeshes; i++)
    {
        S32 meshType = readAlloc32(s, memBuffer32);
        readAllocMesh(s, memBuffer32, memBuffer16, memBuffer8, meshType);
    }

    DebugGuard();

    // names...
    S32 numNames = readAlloc32(s, memBuffer32[12]);
    for (i = 0; i < numNames; i++)
    {
        S32 sz;
        s->read(&sz);
        readAlloc(s, memBuffer8, sz);
        *oldAlloc(memBuffer8, 1) = 0; // end the string
    }

    DebugGuard();

    // material list...read the old fashion way
    S32 gotList;
    s->read(&gotList);
    if (gotList)
    {
        materialList = new TSMaterialList;
        materialList->read(*s);
        if (mExporterVersion < 116)
        {
            // for any material that is translucent and doesn't tile, add zero border property
            for (i = 0; i < (S32)materialList->getMaterialCount(); i++)
            {
                U32 flags = materialList->getFlags(i);
                if ((flags & TSMaterialList::Translucent) && !(flags & (TSMaterialList::S_Wrap | TSMaterialList::T_Wrap)))
                    materialList->setFlags(i, flags | TSMaterialList::MipMap_ZeroBorder);
            }
        }
    }
    else
        materialList = NULL;

    // allocate memory for these vectors here...filled in below
    S32 detailFirstSkinCount = oldAllocOffset(memBuffer32);
    oldAlloc(memBuffer32, numDetails);
    S32 detailNumSkinCount = oldAllocOffset(memBuffer32);
    oldAlloc(memBuffer32, numDetails);

    DebugGuard();

    // skins
    S32 numSkins = readAlloc32(s, memBuffer32[11]);
    for (i = 0; i < numSkins; i++)
        readAllocMesh(s, memBuffer32, memBuffer16, memBuffer8, TSMesh::SkinMeshType);

    DebugGuard();

    if (numSkins == 0)
    {
        S32* pSkinCounts = (S32*)memBuffer32 + detailFirstSkinCount;
        for (i = 0; i < 2 * numDetails; i++)
            pSkinCounts[i] = convertLEndianToHost((U32)0);
    }
    if (numSkins)
    {
        S32 sz;
        s->read(&sz);
        S32* pDetailFirstSkin = memBuffer32 + detailFirstSkinCount;
        S32* pDetailNumSkin = memBuffer32 + detailNumSkinCount;
        s->read(numDetails * sizeof(S32), (S8*)pDetailFirstSkin);
        S32 prev = numSkins;
        for (i = numDetails - 1; i >= 0; i--)
        {
            pDetailNumSkin[i] = convertLEndianToHost(prev - convertLEndianToHost(pDetailFirstSkin[i]));
            prev = convertLEndianToHost(pDetailFirstSkin[i]);
        }
    }

    DebugGuard();

    if (smReadVersion < 17)
    {
        for (i = 0; i < (S32)sequences.size(); i++)
        {
            Sequence& seq = sequences[i];
            rearrangeKeyframeData(seq, kfStart[i], (U8*)(memBuffer32 + nodeStateStart32), (U8*)(memBuffer16 + nodeStateStart16), (U8*)(memBuffer32 + objectStateStart), (U8*)(memBuffer32 + decalStateStart), 3 * sizeof(S32), 4 * sizeof(S16), 3 * sizeof(S32), sizeof(S32));
        }
    }

    count32 = getDWordCount32();
    count16 = getDWordCount16();
    count8 = getDWordCount8();
}

void TSShape::rearrangeKeyframeData(Sequence& seq, S32 startKeyframe, U8* pRot, U8* pTrans, U8* pos, U8* pds, S32 rotSize, S32 tranSize, S32 osSize, S32 dsSize)
{
    // count nodes, objects, and decals...
    S32 numNodes = 0;
    S32 numObjects = 0;
    S32 numDecals = 0;
    S32 numKeyframes = seq.numKeyframes;
    S32 j;
    TSIntegerSet objectMembership = seq.frameMatters;
    objectMembership.overlap(seq.matFrameMatters);
    objectMembership.overlap(seq.visMatters);
    for (j = 0; j < MAX_TS_SET_SIZE; j++)
    {
        if (seq.rotationMatters.test(j)) // we're old sequence, so same as translationMatters
            numNodes++;
        if (objectMembership.test(j))
            numObjects++;
        if (seq.decalMatters.test(j))
            numDecals++;
    }

    // fill in default size and location info...
    if (!pRot && numKeyframes * numNodes)
        pRot = (U8*)&nodeRotations[0];
    if (!pTrans && numKeyframes * numNodes)
        pTrans = (U8*)&nodeTranslations[0];
    if (rotSize < 0)
        rotSize = sizeof(Quat16);
    if (tranSize < 0)
        tranSize = sizeof(Point3F);
    if (!pos && numKeyframes * numObjects)
        pos = (U8*)&objectStates[0];
    if (osSize < 0)
        osSize = sizeof(ObjectState);
    if (!pds && numKeyframes * numDecals)
        pds = (U8*)&decalStates[0];
    if (dsSize < 0)
        dsSize = sizeof(DecalState);

    if (seq.numKeyframes)
    {
        seq.baseRotation = numNodes ? keyframes[startKeyframe].firstNodeState : 0;
        seq.baseTranslation = numNodes ? keyframes[startKeyframe].firstNodeState : 0;
        seq.baseObjectState = numObjects ? keyframes[startKeyframe].firstObjectState : 0;
        seq.baseDecalState = numDecals ? keyframes[startKeyframe].firstDecalState : 0;
        rearrangeStates(seq.baseRotation, numKeyframes, numNodes, pRot, rotSize);
        rearrangeStates(seq.baseTranslation, numKeyframes, numNodes, pTrans, tranSize);
        rearrangeStates(seq.baseObjectState, numKeyframes, numObjects, pos, osSize);
        rearrangeStates(seq.baseDecalState, numKeyframes, numDecals, pds, dsSize);
    }
}

void TSShape::rearrangeStates(S32 start, S32 a, S32 b, U8* dat, S32 size)
{
    // have to account for different packing size...
    U8* copy = new U8[a * b * size];
    dMemcpy(copy, &dat[start * size], a * b * size);
    for (S32 i = 0; i < a; i++)
        for (S32 j = 0; j < b; j++)
            dMemcpy(dat + size * (start + j * a + i), copy + size * (i * b + j), size);
    delete[] copy;
}

//-------------------------------------------------
// put old skins into object list
//-------------------------------------------------

void TSShape::fixupOldSkins(S32 numMeshes, S32 numSkins, S32 numDetails, S32* detailFirstSkin, S32* detailNumSkins)
{
#if !defined(TORQUE_MAX_LIB)
    // this method not necessary in exporter, and a couple lines won't compile for exporter
    if (!objects.address() || !meshes.address() || !numSkins)
        // not ready for this yet, will catch it on the next pass
        return;
    S32 numObjects = objects.size();
    TSObject* newObjects = objects.address() + objects.size();
    TSSkinMesh** skins = (TSSkinMesh**)&meshes[numMeshes];
    Vector<TSSkinMesh*> skinsCopy;
    // Note: newObjects has as much free space as we need, so we just need to keep track of the
    //       number of objects we use and then update objects.size
    S32 numSkinObjects = 0;
    S32 skinsUsed = 0;
    S32 emptySkins = 0;
    S32 i;
    for (i = 0; i < numSkins; i++)
        if (skins[i] == NULL)
            emptySkins++; // probably never, but just in case
    while (skinsUsed < numSkins - emptySkins)
    {
        objects.push_back(TSObject());
        TSObject& object = newObjects[numSkinObjects++];
        
        object.nameIndex = 0; // no name
        object.numMeshes = 0;
        object.startMeshIndex = numMeshes + skinsCopy.size();
        object.nodeIndex = -1;
        object.nextSibling = -1;
        object.firstDecal = -1;
        for (S32 dl = 0; dl < numDetails; dl++)
        {
            // find one mesh per detail to add to this object
            // don't really need to be versions of the same object
            i = 0;
            while (i < detailFirstSkin[dl] || detailFirstSkin[dl] < 0)
                i++;
            for (; i < numSkins && i < detailFirstSkin[dl] + detailNumSkins[dl]; i++)
            {
                if (skins[i])
                {
                    // found an unused skin... copy it to skinsCopy and set to NULL
                    skinsCopy.push_back(skins[i]);
                    skins[i] = NULL;
                    object.numMeshes++;
                    skinsUsed++;
                    break;
                }
            }
            if (i == numSkins || i == detailFirstSkin[dl] + detailNumSkins[dl])
            {
                skinsCopy.push_back(NULL);
                object.numMeshes++;
            }
        }
        // exit above loop with one skin per detail...despose of trailing null meshes
        while (!skinsCopy.empty() && skinsCopy.last() == NULL)
        {
            skinsCopy.decrement();
            object.numMeshes--;
        }
        // if no meshes, don't need object
        if (!object.numMeshes)
        {
            objects.pop_back();
            numSkinObjects--;
        }
    }
    dMemcpy(skins, skinsCopy.address(), skinsCopy.size() * sizeof(TSSkinMesh*));

    if (subShapeFirstObject.size() == 1)
        // as long as only one subshape, we'll now be rendered
        subShapeNumObjects[0] += numSkinObjects;

    // now for something ugly -- we've added somoe objects to hold the skins...
    // now we have to add default states for those objects
    // we also have to increment base states on all the sequences that are loaded
    dMemmove(objectStates.address() + numObjects + numSkinObjects, objectStates.address() + numObjects, (objectStates.size() - numObjects) * sizeof(ObjectState));
    for (i = numObjects; i < numObjects + numSkinObjects; i++)
    {
        objectStates[i].vis = 1.0f;
        objectStates[i].frameIndex = 0;
        objectStates[i].matFrameIndex = 0;
    }
    for (i = 0; i < sequences.size(); i++)
    {
        sequences[i].baseObjectState += numSkinObjects;
    }
#endif
}

//-------------------------------------------------
// some macros used for read/write
//-------------------------------------------------

// write a vector of structs (minus the first 'm')
#define writeVectorStructMinus(a,m) \
{\
   s->write(a.size() - m); \
   for (S32 i=m;i<a.size();i++) \
      a[i].write(s); \
}

// write a vector of simple types (minus the first 'm')
#define writeVectorSimpleMinus(a,m) \
{\
   s->write(a.size() - m); \
   for (S32 i=m;i<a.size();i++) \
      s->write(a[i]); \
}

// same as above with m=0
#define writeVectorStruct(a) writeVectorStructMinus(a,0)
#define writeVectorSimple(a) writeVectorSimpleMinus(a,0)

// read a vector of structs -- over-writing any existing data
#define readVectorStruct(a) \
{ \
   S32 sz; \
   s->read(&sz); \
   a.setSize(sz); \
   for (S32 i=0;i<sz;i++) \
      a[i].read(s); \
}

// read a vector of simple types -- over-writing any existing data
#define readVectorSimple(a) \
{ \
   S32 sz; \
   s->read(&sz); \
   a.setSize(sz); \
   for (S32 i=0;i<sz;i++) \
      s->read(&a[i]); \
}

// read a vector of structs -- append to any existing data
#define appendVectorStruct(a) \
{ \
   S32 sz; \
   S32 oldSz = a.size(); \
   s->read(&sz); \
   a.setSize(oldSz + sz); \
   for (S32 i=0;i<sz;i++) \
      a[i + oldSz].read(s); \
}

// read a vector of simple types -- append to any existing data
#define appendVectorSimple(a) \
{ \
   S32 sz; \
   S32 oldSz = a.size(); \
   s->read(&sz); \
   a.setSize(oldSz + sz); \
   for (S32 i=0;i<sz;i++) \
      s->read(&a[i + oldSz]); \
}

//-------------------------------------------------
// export all sequences
//-------------------------------------------------
void TSShape::exportSequences(Stream* s)
{
    // write version
    s->write(smVersion);

    S32 i, sz;

    // write node names
    // -- this is how we will map imported sequence nodes to shape nodes
    sz = nodes.size();
    s->write(sz);
    for (i = 0; i < nodes.size(); i++)
        writeName(s, nodes[i].nameIndex);

    // legacy write -- write zero objects, don't pretend to support object export anymore
    s->write(0);

    // on import, we will need to adjust keyframe data based on number of
    // nodes/objects in this shape...number of nodes can be inferred from
    // above, but number of objects cannot be.  Write that quantity here:
    s->write(objects.size());

    // write node states -- skip default node states
    s->write(nodeRotations.size());
    for (i = 0; i < nodeRotations.size(); i++)
    {
        s->write(nodeRotations[i].x);
        s->write(nodeRotations[i].y);
        s->write(nodeRotations[i].z);
        s->write(nodeRotations[i].w);
    }
    s->write(nodeTranslations.size());
    for (i = 0; i < nodeTranslations.size(); i++)
    {
        s->write(nodeTranslations[i].x);
        s->write(nodeTranslations[i].y);
        s->write(nodeTranslations[i].z);
    }
    s->write(nodeUniformScales.size());
    for (i = 0; i < nodeUniformScales.size(); i++)
        s->write(nodeUniformScales[i]);
    s->write(nodeAlignedScales.size());
    for (i = 0; i < nodeAlignedScales.size(); i++)
    {
        s->write(nodeAlignedScales[i].x);
        s->write(nodeAlignedScales[i].y);
        s->write(nodeAlignedScales[i].z);
    }
    s->write(nodeArbitraryScaleRots.size());
    for (i = 0; i < nodeArbitraryScaleRots.size(); i++)
    {
        s->write(nodeArbitraryScaleRots[i].x);
        s->write(nodeArbitraryScaleRots[i].y);
        s->write(nodeArbitraryScaleRots[i].z);
        s->write(nodeArbitraryScaleRots[i].w);
    }
    for (i = 0; i < nodeArbitraryScaleFactors.size(); i++)
    {
        s->write(nodeArbitraryScaleFactors[i].x);
        s->write(nodeArbitraryScaleFactors[i].y);
        s->write(nodeArbitraryScaleFactors[i].z);
    }
    s->write(groundTranslations.size());
    for (i = 0; i < groundTranslations.size(); i++)
    {
        s->write(groundTranslations[i].x);
        s->write(groundTranslations[i].y);
        s->write(groundTranslations[i].z);
    }
    for (i = 0; i < groundRotations.size(); i++)
    {
        s->write(groundRotations[i].x);
        s->write(groundRotations[i].y);
        s->write(groundRotations[i].z);
        s->write(groundRotations[i].w);
    }

    // write object states -- legacy..no object states
    s->write((S32)0);

    // write sequences
    s->write(sequences.size());
    for (i = 0; i < sequences.size(); i++)
    {
        Sequence& seq = sequences[i];

        // first write sequence name
        writeName(s, seq.nameIndex);

        // now write the sequence itself
        seq.write(s, false); // false --> don't write name index
    }

    // write out all the triggers...
    s->write(triggers.size());
    for (i = 0; i < triggers.size(); i++)
    {
        s->write(triggers[i].state);
        s->write(triggers[i].pos);
    }
}

//-------------------------------------------------
// import sequences into existing shape
//-------------------------------------------------
bool TSShape::importSequences(Stream* s)
{
    // write version
    s->read(&smReadVersion);
    if (smReadVersion > smVersion)
    {
        // error -- don't support future version yet :>
        Con::errorf(ConsoleLogEntry::General,
            "Sequence import failed:  shape exporter newer than running executable.");
        return false;
    }

    Vector<S32> nodeMap;   // node index of each node from imported sequences
    Vector<S32> objectMap; // object index of objects from imported sequences
    VECTOR_SET_ASSOCIATION(nodeMap);
    VECTOR_SET_ASSOCIATION(objectMap);

    S32 i, sz;

    // read node names
    // -- this is how we will map imported sequence nodes to our nodes
    s->read(&sz);
    nodeMap.setSize(sz);
    Vector<S32> checkForDups;
    for (i = 0; i < sz; i++)
    {
        U32 startSize = names.size();
        S32 nameIndex = readName(s, true);
        U32 count = 0;
        if (nameIndex >= 0)
        {
            while (checkForDups.size() < nameIndex + 1)
                checkForDups.push_back(0);
            count = checkForDups[nameIndex]++;
        }
        if (count)
        {
            // not first time this name came up...look for later instance of the node
            S32 j;
            nodeMap[i] = -1;
            for (j = 0; j < nodes.size(); j++)
            {
                if (nodes[j].nameIndex == nameIndex && count-- == 0)
                    break;
            }
            nodeMap[i] = j;
            if (j == nodes.size())
            {
                Con::errorf(ConsoleLogEntry::General, "Sequence import failed: sequence node \"%s\" duplicated more in sequence than in shape.", names[nameIndex]);
                return false;
            }
        }
        else
            nodeMap[i] = findNode(nameIndex);
        if (nodeMap[i] < 0)
        {
            // error -- node found in sequence but not shape
            Con::errorf(ConsoleLogEntry::General,
                "Sequence import failed:  sequence node \"%s\" not found in base shape.", names[nameIndex]);
            if (names.size() != startSize)
            {
                names.decrement();
                AssertFatal(names.size() == startSize, "TSShape::importSequence: assertion failed");
            }
            return false;
        }
    }

    // read the following size, but won't do anything with it...legacy:  was going to support
    // import of sequences that animate objects...we don't...
    s->read(&sz); sz;

    // before reading keyframes, take note of a couple numbers
    S32 oldShapeNumObjects;
    s->read(&oldShapeNumObjects);

    // read keyframes
    if (smReadVersion < 17)
    {
        keyframes.clear();
        appendVectorStruct(keyframes);
    }

    // adjust all the new keyframes
    S32 adjNodeRots = smReadVersion < 22 ? nodeRotations.size() - nodeMap.size() : nodeRotations.size();
    S32 adjNodeTrans = smReadVersion < 22 ? nodeTranslations.size() - nodeMap.size() : nodeTranslations.size();
    S32 adjNodeScales1 = nodeUniformScales.size();
    S32 adjNodeScales2 = nodeAlignedScales.size();
    S32 adjNodeScales3 = nodeArbitraryScaleFactors.size();
    S32 adjObjectStates = objectStates.size() - oldShapeNumObjects;
    S32 adjGroundStates = smReadVersion < 22 ? 0 : groundTranslations.size(); // groundTrans==groundRot
    for (i = 0; i < keyframes.size(); i++)
    {
        // have keyframes only for old shapes...use adjNodeRots for adjustment
        // since same as adjNodeScales...
        keyframes[i].firstNodeState += adjNodeRots;
        keyframes[i].firstObjectState += adjObjectStates;
    }

    // add these node states to our own
    if (smReadVersion > 21)
    {
        s->read(&sz);
        S32 oldSz = nodeRotations.size();
        nodeRotations.setSize(sz + oldSz);
        for (i = oldSz; i < oldSz + sz; i++)
        {
            s->read(&nodeRotations[i].x);
            s->read(&nodeRotations[i].y);
            s->read(&nodeRotations[i].z);
            s->read(&nodeRotations[i].w);
        }
        s->read(&sz);
        oldSz = nodeTranslations.size();
        nodeTranslations.setSize(sz + oldSz);
        for (i = oldSz; i < sz + oldSz; i++)
        {
            s->read(&nodeTranslations[i].x);
            s->read(&nodeTranslations[i].y);
            s->read(&nodeTranslations[i].z);
        }
        s->read(&sz);
        oldSz = nodeUniformScales.size();
        nodeUniformScales.setSize(sz + oldSz);
        for (i = oldSz; i < sz + oldSz; i++)
            s->read(&nodeUniformScales[i]);
        s->read(&sz);
        oldSz = nodeAlignedScales.size();
        nodeAlignedScales.setSize(sz + oldSz);
        for (i = oldSz; i < sz + oldSz; i++)
        {
            s->read(&nodeAlignedScales[i].x);
            s->read(&nodeAlignedScales[i].y);
            s->read(&nodeAlignedScales[i].z);
        }
        s->read(&sz);
        oldSz = nodeArbitraryScaleRots.size();
        nodeArbitraryScaleRots.setSize(sz + oldSz);
        for (i = oldSz; i < sz + oldSz; i++)
        {
            s->read(&nodeArbitraryScaleRots[i].x);
            s->read(&nodeArbitraryScaleRots[i].y);
            s->read(&nodeArbitraryScaleRots[i].z);
            s->read(&nodeArbitraryScaleRots[i].w);
        }
        oldSz = nodeArbitraryScaleFactors.size();
        nodeArbitraryScaleFactors.setSize(sz + oldSz);
        for (i = oldSz; i < sz + oldSz; i++)
        {
            s->read(&nodeArbitraryScaleFactors[i].x);
            s->read(&nodeArbitraryScaleFactors[i].y);
            s->read(&nodeArbitraryScaleFactors[i].z);
        }
        s->read(&sz);
        oldSz = groundTranslations.size();
        groundTranslations.setSize(sz + oldSz);
        for (i = oldSz; i < sz + oldSz; i++)
        {
            s->read(&groundTranslations[i].x);
            s->read(&groundTranslations[i].y);
            s->read(&groundTranslations[i].z);
        }
        groundRotations.setSize(sz + oldSz);
        for (i = oldSz; i < sz + oldSz; i++)
        {
            s->read(&groundRotations[i].x);
            s->read(&groundRotations[i].y);
            s->read(&groundRotations[i].z);
            s->read(&groundRotations[i].w);
        }
    }
    else
    {
        s->read(&sz);
        S32 oldSz1 = nodeRotations.size();
        S32 oldSz2 = nodeTranslations.size();
        nodeRotations.setSize(oldSz1 + sz);
        nodeTranslations.setSize(oldSz2 + sz);
        for (i = 0; i < sz; i++)
        {
            s->read(&nodeRotations[i + oldSz1].x);
            s->read(&nodeRotations[i + oldSz1].y);
            s->read(&nodeRotations[i + oldSz1].z);
            s->read(&nodeRotations[i + oldSz1].w);
            s->read(&nodeTranslations[i + oldSz2].x);
            s->read(&nodeTranslations[i + oldSz2].y);
            s->read(&nodeTranslations[i + oldSz2].z);
        }
    }

    // add these object states to our own -- shouldn't be any...assume it
    s->read(&sz);

    // read sequences
    s->read(&sz);
    S32 startSeqNum = sequences.size();
    kfStart.clear(); // versioning hack -- holds start of range of keyframes used by each sequence loaded
    for (i = 0; i < sz; i++)
    {
        sequences.increment();
        Sequence& seq = sequences.last();
        constructInPlace(&seq);

        // read name
        seq.nameIndex = readName(s, true);

        // read the rest of the sequence
        seq.read(s, false);
        if (smReadVersion > 21)
        {
            seq.baseRotation += adjNodeRots;
            seq.baseTranslation += adjNodeTrans;
            if (seq.animatesUniformScale())
                seq.baseScale += adjNodeScales1;
            else if (seq.animatesAlignedScale())
                seq.baseScale += adjNodeScales2;
            else if (seq.animatesArbitraryScale())
                seq.baseScale += adjNodeScales3;
        }
        else if (smReadVersion >= 17)
        {
            seq.baseRotation += adjNodeRots; // == adjNodeTrans
            seq.baseTranslation += adjNodeTrans; // == adjNodeTrans
        }

        // not quite so easy...
        // now we have to remap nodes from shape the sequence came from to this shape
        // that's where nodeMap comes in handy...
        // ditto for the objects.

        // first the nodes
        S32 j;
        TSIntegerSet newMembership1;
        TSIntegerSet newMembership2;
        TSIntegerSet newMembership3;
        for (j = 0; j < (S32)nodeMap.size(); j++)
        {
            if (seq.translationMatters.test(j))
                newMembership1.set(nodeMap[j]);
            if (seq.rotationMatters.test(j))
                newMembership2.set(nodeMap[j]);
            if (seq.scaleMatters.test(j))
                newMembership3.set(nodeMap[j]);
        }
        seq.translationMatters = newMembership1;
        seq.rotationMatters = newMembership2;
        seq.scaleMatters = newMembership3;

        // adjust trigger numbers...we'll read triggers after sequences...
        seq.firstTrigger += triggers.size();

        // finally, adjust ground transform's nodes states
        seq.firstGroundFrame += adjGroundStates;
    }
    if (smReadVersion < 17)
        for (i = startSeqNum; i < sequences.size(); i++)
            // rearrange some data and add some info to the sequences
            rearrangeKeyframeData(sequences[i], kfStart[i - startSeqNum]);

    if (smReadVersion < 22)
    {
        for (i = startSeqNum; i < sequences.size(); i++)
        {
            // move ground transform data to ground vectors
            Sequence& seq = sequences[i];
            S32 oldSz = groundTranslations.size();
            groundTranslations.setSize(oldSz + seq.numGroundFrames);
            groundRotations.setSize(oldSz + seq.numGroundFrames);
            for (S32 j = 0; j < seq.numGroundFrames; j++)
            {
                groundTranslations[j + oldSz] = nodeTranslations[seq.firstGroundFrame + adjNodeTrans + j];
                groundRotations[j + oldSz] = nodeRotations[seq.firstGroundFrame + adjNodeRots + j];
            }
            seq.firstGroundFrame = oldSz;
        }
    }

    // add the new triggers
    if (smReadVersion > 8)
    {
        S32 sz;
        S32 oldSz = triggers.size();
        s->read(&sz);
        triggers.setSize(oldSz + sz);
        for (S32 i = 0; i < sz; i++)
        {
            s->read(&triggers[i + oldSz].state);
            s->read(&triggers[i + oldSz].pos);
        }
    }

    if (smInitOnRead)
        init();

    return true;
}

//-------------------------------------------------
// read/write sequence
//-------------------------------------------------
void TSShape::Sequence::read(Stream* s, bool readNameIndex)
{
    if (readNameIndex)
        s->read(&nameIndex);
    flags = 0;
    if (TSShape::smReadVersion > 21)
        s->read(&flags);
    else
        flags = 0;
    if (TSShape::smReadVersion < 17)
    {
        // prior to version 17 we had a vector of keyframes and needed to track range of keyframes not just number
        S32 startKeyframe, endKeyframe;
        s->read(&startKeyframe);
        s->read(&endKeyframe);
        numKeyframes = endKeyframe - startKeyframe;
        kfStart.push_back(startKeyframe);
    }
    else
        // just need number of keyframes...
        s->read(&numKeyframes);
    s->read(&duration);

    if (TSShape::smReadVersion < 22)
    {
        bool tmp;
        s->read(&tmp);
        if (tmp)
            flags |= Blend;
        s->read(&tmp);
        if (tmp)
            flags |= Cyclic;
        s->read(&tmp);
        if (tmp)
            flags |= MakePath;
    }

    s->read(&priority);
    s->read(&firstGroundFrame);
    s->read(&numGroundFrames);
    if (TSShape::smReadVersion > 21)
    {
        s->read(&baseRotation);
        s->read(&baseTranslation);
        s->read(&baseScale);
        s->read(&baseObjectState);
        s->read(&baseDecalState);
    }
    else if (TSShape::smReadVersion >= 17)
    {
        s->read(&baseRotation);
        baseTranslation = baseRotation;
        s->read(&baseObjectState);
        s->read(&baseDecalState);
    }
    if (TSShape::smReadVersion > 8)
    {
        s->read(&firstTrigger);
        s->read(&numTriggers);
    }
    else
    {
        firstTrigger = 0;
        numTriggers = 0;
    }
    if (TSShape::smReadVersion > 7)
        s->read(&toolBegin);
    else
        toolBegin = 0.0f;

    // now the membership sets:
    rotationMatters.read(s);
    if (TSShape::smReadVersion < 22)
        translationMatters = rotationMatters;
    else
    {
        translationMatters.read(s);
        scaleMatters.read(s);
    }
    if (TSShape::smReadVersion < 17)
    {
        TSIntegerSet objectMembership; // obsolete
        objectMembership.read(s);
    }
    if (TSShape::smReadVersion > 10)
        decalMatters.read(s);
    if (TSShape::smReadVersion > 5)
        iflMatters.read(s);
    visMatters.read(s);
    frameMatters.read(s);
    matFrameMatters.read(s);
    if (TSShape::smReadVersion < 17)
    {
        // obsolete...
        TSIntegerSet nodeTransformStatic;
        nodeTransformStatic.read(s);
    }

    dirtyFlags = 0;
    if (rotationMatters.testAll() || translationMatters.testAll() || scaleMatters.testAll())
        dirtyFlags |= TSShapeInstance::TransformDirty;
    if (visMatters.testAll())
        dirtyFlags |= TSShapeInstance::VisDirty;
    if (frameMatters.testAll())
        dirtyFlags |= TSShapeInstance::FrameDirty;
    if (matFrameMatters.testAll())
        dirtyFlags |= TSShapeInstance::MatFrameDirty;
    if (decalMatters.testAll())
        dirtyFlags |= TSShapeInstance::DecalDirty;
    if (iflMatters.testAll())
        dirtyFlags |= TSShapeInstance::IflDirty;
}

void TSShape::Sequence::write(Stream* s, bool writeNameIndex)
{
    if (writeNameIndex)
        s->write(nameIndex);
    s->write(flags);
    s->write(numKeyframes);
    s->write(duration);
    s->write(priority);
    s->write(firstGroundFrame);
    s->write(numGroundFrames);
    s->write(baseRotation);
    s->write(baseTranslation);
    s->write(baseScale);
    s->write(baseObjectState);
    s->write(baseDecalState);
    s->write(firstTrigger);
    s->write(numTriggers);
    s->write(toolBegin);

    // now the membership sets:
    rotationMatters.write(s);
    translationMatters.write(s);
    scaleMatters.write(s);
    decalMatters.write(s);
    iflMatters.write(s);
    visMatters.write(s);
    frameMatters.write(s);
    matFrameMatters.write(s);
}

void TSShape::writeName(Stream* s, S32 nameIndex)
{
    const char* name = "";
    if (nameIndex >= 0)
        name = names[nameIndex];
    S32 sz = (S32)dStrlen(name);
    s->write(sz);
    if (sz)
        s->write(sz * sizeof(char), name);
}

S32 TSShape::readName(Stream* s, bool addName)
{
    static char buffer[256];
    S32 sz;
    S32 nameIndex = -1;
    s->read(&sz);
    if (sz)
    {
        s->read(sz * sizeof(char), buffer);
        buffer[sz] = '\0';
        nameIndex = findName(buffer);
        if (nameIndex < 0 && addName)
        {
            nameIndex = names.size();
            names.increment();
            names.last() = StringTable->insert(buffer, false); // case insensitive
        }
    }
    return nameIndex;
}
