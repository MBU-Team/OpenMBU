//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------
//#define ENABLE_DECIMATION

#ifdef ENABLE_DECIMATION
#include "MxQMetric.h"
#include "MxQMetric3.h"
#include "MxQSlim.h"
#include "MxStdModel.h"
#endif

#undef M_PI

#include "console/console.h"
#include "atlas/editor/atlasDiscreteMesh.h"
#include "util/safeDelete.h"

AtlasDiscreteMesh::AtlasDiscreteMesh()
{
    mVertexCount = 0;
    mIndexCount = 0;
    mPos = NULL;
    mNormal = NULL;
    mTex = NULL;
    mHasMorphData = false;
    mPosMorphOffset = NULL;
    mTexMorphOffset = NULL;
    mIndex = NULL;
    mOwnsData = true;
}

AtlasDiscreteMesh::~AtlasDiscreteMesh()
{
    if (mOwnsData)
    {
        SAFE_DELETE_ARRAY(mPos);
        SAFE_DELETE_ARRAY(mNormal);
        SAFE_DELETE_ARRAY(mTex);
        SAFE_DELETE_ARRAY(mPosMorphOffset);
        SAFE_DELETE_ARRAY(mTexMorphOffset);
        SAFE_DELETE_ARRAY(mIndex);
    }
}

void AtlasDiscreteMesh::combine(Vector<AtlasDiscreteMesh*>& meshes)
{
    // Concatenate all that data into us. We expect we're uninitialized.
    AssertISV(mVertexCount == 0, "AtlasDiscreteMesh::combine - already have something in me!");

    // Calculate our index and vertex counts.
    U32 idxCount = 0, vertCount = 0;

    for (S32 i = 0; i < meshes.size(); i++)
    {
        idxCount += meshes[i]->mIndexCount;
        vertCount += meshes[i]->mVertexCount;
    }

    // Prep our buffers.
    mIndexCount = idxCount;
    mIndex = new U16[idxCount];

    mVertexCount = vertCount;
    mPos = new Point3F[vertCount];
    mNormal = new Point3F[vertCount];
    mTex = new Point2F[vertCount];

    // Now, copy everything in.
    U16* idx = mIndex;
    Point3F* pos = mPos;
    Point3F* nrm = mNormal;
    Point2F* tex = mTex;
    U32 idxOffset = 0;

    for (S32 i = 0; i < meshes.size(); i++)
    {
        // Copy
        dMemcpy(idx, meshes[i]->mIndex, sizeof(U16) * meshes[i]->mIndexCount);
        dMemcpy(pos, meshes[i]->mPos, sizeof(Point3F) * meshes[i]->mVertexCount);
        dMemcpy(nrm, meshes[i]->mNormal, sizeof(Point3F) * meshes[i]->mVertexCount);
        dMemcpy(tex, meshes[i]->mTex, sizeof(Point2F) * meshes[i]->mVertexCount);

        // We have to walk our indices and offset the indices appropriately.
        if (idxOffset)
        {
            for (S32 j = 0; j < meshes[i]->mIndexCount; j++)
            {
                AssertFatal(idx[j] < meshes[i]->mVertexCount, "AtlasDiscreteMesh::combine - invalid idx offset.");
                idx[j] += idxOffset;
                AssertFatal(idx[j] < idxOffset + meshes[i]->mVertexCount, "AtlasDiscreteMesh::combine - invalid idx offset.");
            }
        }

        // Update pointers.
        idx += meshes[i]->mIndexCount;
        pos += meshes[i]->mVertexCount;
        nrm += meshes[i]->mVertexCount;
        tex += meshes[i]->mVertexCount;

        // And our offset, too.
        idxOffset += meshes[i]->mVertexCount;
    }

    // Ok, all done.
}

AtlasDiscreteMesh* AtlasDiscreteMesh::decimate(U32 target)
{
#ifndef ENABLE_DECIMATION
    Con::errorf("AtlasDiscreteMesh::decimate - decimation not enabled, define ENABLE_DECIMATION!");
    return NULL;
#else

    // collapse rules:
    // 
    // side to top, to what vert (if any)
    //
    //	        center	edge	corner	never
    // center  	any	edge	corner	never
    // edge	   -	   line	corner	-
    // corner	-	   -	   -	      -
    // never  	-	   -	   -	      -

       // First, copy ourselves into a MxStdModel
    MxStdModel sm(mVertexCount, mIndexCount / 3);

    sm.texcoord_binding(MX_PERVERTEX);
    sm.normal_binding(MX_PERVERTEX);

    for (U32 i = 0; i < mVertexCount; i++)
    {
        sm.add_texcoord(mTex[i].x, mTex[i].y);
        sm.add_vertex(mPos[i].x, mPos[i].y, mPos[i].z);
        sm.add_normal(mNormal[i].x, mNormal[i].y, mNormal[i].z);
    }

    for (U32 i = 0; i < mIndexCount; i += 3)
        sm.add_face(mIndex[i + 0], mIndex[i + 1], mIndex[i + 2]);

    Con::printf("AtlasDiscreteMesh::decimate - starting with %d verts, %d indices, %d faces allocated.",
        sm.vert_count(), mIndexCount, sm.face_count());

    // Run it through the decimator.
    MxEdgeQSlim qslim(sm);

    qslim.placement_policy = MX_PLACE_ENDORMID; //MX_PLACE_OPTIMAL; <-- optimal causes f'ed up results!
    qslim.weighting_policy = MX_WEIGHT_AREA_AVG;
    qslim.compactness_ratio = 0.0;
    qslim.meshing_penalty = 1.0;
    qslim.will_join_only = false;

    // Now, calculate our constraints - basically take everything that's within
    // 0.1 meters of a bounding plane and mark it with that plane's IDs. Our
    // bounding planes are the 4 vertical faces of the bounding box.
    PlaneF planes[4];
    Box3F bounds = calcBounds();

    // Generate planes from three points... Up is z+, and the normal for the
    // plane should face towards the center (but it doesn't really matter, we
    // only ever take absolute distance).
    {
        Point3F a, b, c;
        a.x = bounds.min.x;
        a.y = bounds.min.y;
        a.z = bounds.min.z;

        b.x = bounds.max.x;
        b.y = bounds.min.y;
        b.z = bounds.min.z;

        c = a + Point3F(0, 0, 10);

        planes[0].set(a, b, c);
    }
    {
        Point3F a, b, c;
        a.x = bounds.max.x;
        a.y = bounds.min.y;
        a.z = bounds.min.z;

        b.x = bounds.max.x;
        b.y = bounds.max.y;
        b.z = bounds.min.z;

        c = a + Point3F(0, 0, 10);

        planes[1].set(a, b, c);
    }
    {
        Point3F a, b, c;
        a.x = bounds.max.x;
        a.y = bounds.max.y;
        a.z = bounds.min.z;

        b.x = bounds.min.x;
        b.y = bounds.max.y;
        b.z = bounds.min.z;

        c = a + Point3F(0, 0, 10);

        planes[2].set(a, b, c);
    }
    {
        Point3F a, b, c;
        a.x = bounds.min.x;
        a.y = bounds.max.y;
        a.z = bounds.min.z;

        b.x = bounds.min.x;
        b.y = bounds.min.y;
        b.z = bounds.min.z;

        c = a + Point3F(0, 0, 10);

        planes[3].set(a, b, c);
    }

    // Now categorize everything w/ constraints.
    for (U32 i = 0; i < mVertexCount; i++)
    {
        U8 bits = 0;
        for (S32 j = 0; j < 4; j++)
        {
            F32 d = mFabs(planes[j].distToPlane(mPos[i]));

            if (d < .1)
                bits |= BIT(j);
        }

        qslim.planarConstraints(i) = bits;
    }

    // Now we initialize and constrain...
    qslim.initialize();
    qslim.decimate(target);

    // Copy everything out to a new ADM
    AtlasDiscreteMesh* admOut = new AtlasDiscreteMesh();

    // First, mark stray vertices for removal
    for (U32 i = 0; i < sm.vert_count(); i++)
        if (sm.vertex_is_valid(i) && sm.neighbors(i).length() == 0)
            sm.vertex_mark_invalid(i);

    // Compact vertex array so only valid vertices remain
    sm.compact_vertices();

    // Copy verts out. Get a count, first.
    U32 vertCount = 0;

    for (U32 i = 0; i < sm.vert_count(); i++)
        if (sm.vertex_is_valid(i))
            vertCount++;

    admOut->mVertexCount = vertCount;
    admOut->mPos = new Point3F[vertCount];
    admOut->mTex = new Point2F[vertCount];
    admOut->mNormal = new Point3F[vertCount];

    for (S32 i = 0; i < vertCount; i++)
    {
        if (!sm.vertex_is_valid(i))
            continue;

        const MxVertex& v = qslim.model().vertex(i);
        const MxTexCoord& t = qslim.model().texcoord(i);
        const MxNormal& n = qslim.model().normal(i);

        admOut->mPos[i].x = v[0];
        admOut->mPos[i].y = v[1];
        admOut->mPos[i].z = v[2];

        admOut->mTex[i].x = t[0];
        admOut->mTex[i].y = t[1];

        admOut->mNormal[i].x = n[0];
        admOut->mNormal[i].y = n[1];
        admOut->mNormal[i].z = n[2];
    }

    // We have to ignore non-valid faces, so let's recount our idxCount.
    U32 idxCount = 0;

    for (U32 i = 0; i < sm.face_count(); i++)
        if (sm.face_is_valid(i))
            idxCount += 3;

    Con::printf("AtlasDiscreteMesh::decimate - %d out of %d verts, %d out of %d indices.",
        vertCount, sm.vert_count(), idxCount, sm.face_count() * 3);

    admOut->mIndexCount = idxCount;
    admOut->mIndex = new U16[idxCount];

    U16* idx = admOut->mIndex;

    for (S32 i = 0; i < sm.face_count(); i++)
    {
        // Skip invalid.
        if (!sm.face_is_valid(i))
            continue;

        const MxFace& f = sm.face(i);

        idx[0] = f[0];
        idx[1] = f[1];
        idx[2] = f[2];

        //Con::printf("                 face #%d (%d, %d, %d)", i, f[0], f[1], f[2]);

        idx += 3;
    }

    // Make sure our vert and index counts are accurate.
    admOut->mIndexCount = idx - admOut->mIndex;

    return admOut;
#endif
}

void AtlasDiscreteMesh::transformPositions(Point2F s, Point2F t)
{
    for (S32 i = 0; i < mVertexCount; i++)
    {
        mPos[i].x = mPos[i].x * s.x + s.y;
        mPos[i].y = mPos[i].y * t.x + t.y;
    }
}

void AtlasDiscreteMesh::transformTexCoords(Point2F s, Point2F t)
{
    for (S32 i = 0; i < mVertexCount; i++)
    {
        mTex[i].x = mTex[i].x * s.x + s.y;
        mTex[i].y = mTex[i].y * t.x + t.y;
    }
}

void AtlasDiscreteMesh::weld(F32 posThreshold, F32 texThreshold /* = 0.001f */)
{
    // Let's do a naive check.

    // Square so we can compare against squares.
    posThreshold *= posThreshold;
    texThreshold *= texThreshold;

    // Set up our remap table.
    Vector<U16> idxRemap;
    idxRemap.setSize(mVertexCount);

    for (S32 i = 0; i < mVertexCount; i++)
        idxRemap[i] = i;

    U32 skipped = 0;

    // Now check each vert to see if it's unique or if we have to remap it.
    for (U32 i = 0; i < mVertexCount; i++)
    {
        const Point3F& curPos = mPos[i];
        const Point2F& curTex = mTex[i];

        F32 minPos = F32_MAX;
        F32 minTex = F32_MAX;

        // If we match another item in the list (before us) then note its
        // index in the remap table. Otherwise, just note our current index.
        for (S32 j = 0; j < mVertexCount; j++)
        {
            // Skip comparing against ourselves.
            if (i == j)
                continue;

            minPos = getMin(minPos, (mPos[j] - curPos).lenSquared());
            minTex = getMin(minTex, (mTex[j] - curTex).lenSquared());

            // Skip if it's too far away.
            if ((mPos[j] - curPos).lenSquared() > posThreshold)
                continue;

            // Skip if it's textured differently.
            if ((mTex[j] - curTex).lenSquared() > texThreshold)
                continue;

            // Onoez, a match! Note it and skip to next.
            skipped++;
            idxRemap[i] = j;
            goto continueLoop;
        }

        // No matches, so let it stay unique.
        idxRemap[i] = i;

        // And continue on...
    continueLoop:
        continue;
    }

    // Alright, we've simplified everything. Let's apply the remap table and call
    // it good.
    remap(idxRemap);

    Con::printf("AtlasDiscreteMesh::weld - killed %d verts", skipped);
}

void AtlasDiscreteMesh::remap(const Vector<U16>& remapTable)
{
    AssertISV(remapTable.size() == mVertexCount, "AtlasDiscreteMesh::remap - remap table not the right size!");

    // Take each index, run it through the remap table, and store the result.
    for (U32 i = 0; i < mIndexCount; i++)
        mIndex[i] = remapTable[mIndex[i]];
}

Box3F AtlasDiscreteMesh::calcBounds()
{
    Box3F res;
    res.min.set(F32_MAX, F32_MAX, F32_MAX);
    res.max.set(-F32_MAX, -F32_MAX, -F32_MAX);

    for (S32 i = 0; i < mVertexCount; i++)
    {
        res.min.setMin(mPos[i]);
        res.max.setMax(mPos[i]);
    }

    return res;
}