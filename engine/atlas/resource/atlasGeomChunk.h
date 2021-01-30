//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------
#ifndef _ATLASGEOMCHUNK_H_
#define _ATLASGEOMCHUNK_H_

#include "atlas/core/atlasChunk.h"
#include "gfx/gfxDevice.h"
#include "gfx/gfxStructs.h"
#include "collision/convex.h"

class AbstractPolyList;
class AtlasDiscreteMesh;
class AtlasInstance2;

/// Atlas chunk subclass for geometry.
///
/// @ingroup AtlasResource
class AtlasGeomChunk : public AtlasChunk
{
public:
    AtlasGeomChunk();
    virtual ~AtlasGeomChunk();

    typedef AtlasChunk Parent;

    /// @name Raw data
    ///
    /// Raw data buffers, which are loaded directly from the file. They require
    /// no additional processing; once loaded they are OK to use.
    ///
    /// @{

    /// Number of indices used for rendering "fake" geometry - ie, skirts,
    /// which are present solely to help hide LOD changes. They should be
    /// discarded when generating new LOD levels and so forth.
    ///
    /// "Fake" geometry always lives at the end of the index buffer.
    ///
    /// Draw this much less to not draw skirts.
    U32 mSkirtSkipCount;

    U32 mIndexCount;
    U16* mIndex;
    U32 mVertCount;
    GFXAtlasVert2* mVert;

    /// Bounds of this chunk; duplicated in stub so we can get to it w/o
    /// paging.
    Box3F mBounds;

    /// Bounding box for this chunk's TC's.
    RectF mTCBounds;

    /// @}

    /// @name Resource Handles
    /// These are only instated when process() is called.
    ///
    /// @{

    ///
    GFXVertexBufferHandle<GFXAtlasVert2> mVB;
    GFXPrimitiveBufferHandle mPB;

    /// @}

    /// @name Collision
    ///
    /// We optionally store collision information for this chunk's geometry.
    ///
    /// @{

    /// A quadtree of ColNodes lets us do a quick broad filter against collision
    /// queries and efficiently reduce the number of triangles we have to
    /// individually check.
    struct ColNode
    {
        F32 min, max;

        inline const bool isEmpty() const
        {
            return min > max;
        }

        inline void update(ColNode& child)
        {
            min = getMin(child.min, min);
            max = getMax(child.max, max);
        }

        inline void update(F32 aMin, F32 aMax)
        {
            min = getMin(aMin, min);
            max = getMax(aMax, max);
        }

        inline void clear()
        {
            min = S16_MAX;
            max = S16_MIN;
        }

        bool read(Stream* s)
        {
            bool ok = true;
            ok &= s->read(&min);
            ok &= s->read(&max);

            AssertFatal(max >= min, "AtlasGeomChunk::ColNode::read - invalid min/max!");

            return ok;
        }

        bool write(Stream* s)
        {
            bool ok = true;
            ok &= s->write(min);
            ok &= s->write(max);
            return ok;
        }
    };

    U32 mColTreeDepth;
    ColNode* mColTree;   ///< The linearized quadtree for our collision info. If
                         ///  NULL, there isn't any collision info.

    Point3F* mColGeom;
    U16* mColIndices;

    // Store collision indices for the leaves of the tree.
    U16* mColIndicesOffsets;
    U16* mColIndicesBuffer;

    U32 mColOffsetCount;
    U32 mColIndexCount;

    /// Helper function to update our collision structures for this chunk.
    void generateCollision();

    /// @}

    virtual void write(Stream* s);
    virtual void read(Stream* s);

    void generate(AtlasChunk* children[4]);

    virtual U32 getHeadSentinel();
    virtual U32 getTailSentinel();

    virtual void process();

    void render();

    /// Calculate some useful information about our near/far points, used primarily
    /// for clipmaps.
    void calculatePoints(const Point3F& camPos, Point3F& outNearPos, Point3F& outFarPos, Point2F& outNearTC, Point2F& outFarTC);

    /// Copy ourselves out to a discrete mesh for processing.
    AtlasDiscreteMesh* copyToDiscreteMesh();

    /// Update ourselves from a discrete mesh.
    void copyFromDiscreteMesh(AtlasDiscreteMesh* adm);

    AtlasGeomChunk* generateCopy(S32 reformat = -1);

    Box3F getBounds();

    bool buildCollisionInfo(const Box3F& box, Convex* c, AbstractPolyList* poly, AtlasInstance2* object);
    void registerConvex(const U16 offset, Convex* convex, AtlasInstance2* object, const Box3F& queryBox);
    Convex mConvexList;

    /// @name Static Statistics
    ///
    /// We track some statistics about the file IO we are doing.
    /// @{

    ///
    static U32 smIndexBytesWritten;
    static U32 smVertexBytesWritten;
    static U32 smCollisionBytesWritten;
    static U32 smTotalBytesWritten;
    static U32 smChunksWritten;

    static void dumpIOStatistics();

    /// @}
};

/// Helper class to optimize collision index data.
///
/// @note This class has a long name. :(
/// @ingroup AtlasResource
class AtlasChunkGeomCollisionBufferGenerator
{
    U32 mGridSize;

    /// The triangle buffer. This encodes lists of triangles contained in each
    /// bin.
    Vector<U16> mTriangles;

    /// Stores the start of each bin's data.
    Vector<U16> mBinOffsets;

public:
    AtlasChunkGeomCollisionBufferGenerator(U32 gridSize);

    /// Reset the generator.
    void clear();

    /// Add a list to the buffer.
    void insertBinList(Point2I bin, Vector<U16>& binList);

    /// Update two buffers to contain index and offset data.
    void store(U32& indexCount, U16*& indices, U32& offsetCount, U16*& offsets);
};

#endif
