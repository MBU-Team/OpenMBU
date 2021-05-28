//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
// Some portions of Atlas code derived from Thatcher Ulrich's original
// ChunkLOD implementation.
//-----------------------------------------------------------------------------

#ifndef _ATLASGEOMINFO_H_
#define _ATLASGEOMINFO_H_

#include "platform/platform.h"
#include "gfx/gfxDevice.h"
#include "gfx/gfxStructs.h"
#include "materials/matInstance.h"

#ifndef _CONVEX_H_
#include "collision/convex.h"
#endif

class AtlasResource;

/// Specific information paged in from the chunk file by a AtlasResource
/// for a AtlasResourceEntry. This optionally contains collision info
/// based on the file generation parameters.
struct AtlasResourceInfo
{
    /// We store a quadtree of those to optimize collision queries. Not all 
    /// AtlasResourceInfos get collision info.
    struct ColNode
    {
        S16 min, max;

        inline bool isEmpty()
        {
            return min > max;
        }

        inline void update(ColNode& child)
        {
            min = getMin(child.min, min);
            max = getMax(child.max, max);
        }

        inline void update(S16 aMin, S16 aMax)
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
            s->read(&min);
            s->read(&max);

            AssertFatal(max >= min, "ChunkGeomInfo::ColNode::read - invalid min/max!");

            return true;
        }
    };

    ColNode* mColTree;   ///< The linearized quadtree for our collision info. If
                         ///  NULL, there isn't any collision info.
    Point3F* mColGeom;
    U16* mColIndices;

    /// Store collision indices for the leaves of the tree.
    U16* mColIndicesOffsets;
    U16* mColIndicesBuffer;


    U32                                 mTriCount;
    GFXVertexBufferHandle<GFXAtlasVert> mVert;
    GFXPrimitiveBufferHandle            mPrim;

    /// Temporary pointers to our loaded but unprepared data. These get wiped
    /// and copied into the above containers when the time comes.
    U16          mRawVertexCount;
    GFXAtlasVert* mRawVerts;
    S32          mRawIndexCount;
    U16* mRawIndices;

    /// Our owning AtlasResource.
    AtlasResource* mData;

    /// Used to track all the extant convexes from this AtlasResurceInfo
    Convex mConvexList;

    /// Register a convex for a given triangle triangle in our collision buffer.
    ///
    /// This exists to abstract all the various checks and updates that have to
    /// occur when we consider a convex for creation.
    void registerConvex(const MatrixF* localMat, U16 offset, Convex* convex);

public:
    AtlasResourceInfo();
    ~AtlasResourceInfo();

    /// Read from a stream and create the buffers. This is run by the
    /// loader thread.
    void read(Stream* s, AtlasResource* tree);

    /// Helper function to prepare loaded data for use. This is run by the main
    /// thread and thus has access to the GFX layer; it creates vertex buffers.
    void prepare();

    /// Render this chunk!
    void render();

    /// Return collision info about this chunk. localMat is used to transform all
    /// the stuff we return - presumably from chunk space to world space.
    ///
    /// @see ChunkGeomInfo::buildCollisionInfo
    bool buildCollisionInfo(const MatrixF* localMat, const Box3F& box, Convex* c, AbstractPolyList* poly);
};

#endif