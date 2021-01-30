//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _ATLASGEN_H_
#define _ATLASGEN_H_

#include "console/console.h"
#include "core/stream.h"
#include "core/tVector.h"
#include "math/mPoint.h"
#include "math/mBox.h"
#include "atlas/editor/atlasOldHeightfield.h"
#include "atlas/editor/atlasOldMesher.h"

class AtlasOldMesher;
class AtlasOldGenState;


/// @defgroup AtlasEditorOld Old Atlas Editor Code
///
/// Old, and hopefully soon to be deprecated editor code. This is primarily
/// a version of the original Atlas terrain mesher, kept until we finish a
/// new and significantly better mesher.
///
/// Much of this functionality is exposed as console commands and thus
/// does not show up here.
///
/// @ingroup AtlasEditor



/// Hardcoded size of per-chunk collision tree.
///
/// @ingroup AtlasEditorOld
const U32 gAtlasColTreeDepth = 4;

/// This is just for data tracking purposes in AtlasGenStats. 256 is the deepest
/// level, so we use that. If you wanted to raise this, feel free.
///
/// @ingroup AtlasEditorOld
const U32 gMaxChunkTreeLevels = 256;


/// Stat-tracking helper for AtlasOldMesher.
///
/// @ingroup AtlasEditorOld
struct AtlasGenStats
{
    U32 inputVertices;
    U32 outputVertices;
    U32 outputRealTriangles;
    U32 outputDegenerateTriangles;
    U32 outputChunks;
    U32 outputSize;

    U32 outputChunkSize[gMaxChunkTreeLevels];
    U32 outputChunkCount[gMaxChunkTreeLevels];

    void clear()
    {
        inputVertices = 0;
        outputVertices = 0;
        outputRealTriangles = 0;
        outputDegenerateTriangles = 0;
        outputChunks = 0;
        outputSize = 0;

        for (S32 i = 0; i < gMaxChunkTreeLevels; i++)
            outputChunkSize[i] = outputChunkCount[i] = 0;
    }

    AtlasGenStats()
    {
        clear();
    }

    void noteChunkStats(U32 level, U32 size)
    {
        outputChunkCount[level]++;
        outputChunkSize[level] += size;
    }

    void printChunkStats()
    {
        Con::printf("=== Chunk Statistics ===\\");
        Con::printf("\\===== Level    Count    Avg. Size");
        for (S32 i = 0; i < gMaxChunkTreeLevels; i++)
            if (outputChunkCount[i])
                Con::printf("           %2d       %5d      %10f", i, outputChunkCount[i], F32(outputChunkSize[i]) / F32(outputChunkCount[i]));
    }
};

/// Stat tracking for the chunk generation process.
///
/// @ingroup AtlasEditorOld
extern AtlasGenStats gOldChunkGenStats;

/// Adds activation level tracking functionality for AtlasOldHeightfield.
///
/// Activation levels are part of our BTT-based mesh decimation system. So if we
/// end up using a different/better mesh decimation algorithm, we can simply go
/// back to using AHFs.
///
/// @ingroup AtlasEditorOld
class AtlasOldActivationHeightfield : public AtlasOldHeightfield
{
protected:
    S8* mActivationLevels;

    /// Returns the height of the query point (x,z) within the triangle (a, r, l),
    /// as tesselated to the specified LOD.
    ///
    /// Return value is in heightfield discrete coords.  To get meters,
    /// multiply by the heightfield's vertical scale.
    HeightType heightQuery(S8 level, Point2I pos, Point2I a, Point2I r, Point2I l);

    /// Append an empty table-of-contents for a fully-populated quadtree,
    /// and rewind the stream to the start of the contents.  Use this to
    /// make room for TOC at the beginning of the file, while bulk
    /// vert/index data gets appended after the TOC. (Helper function, the data
    /// is later filled in by the data generation process.)
    void generateEmptyTOC(Stream* s, S32 treeDepth);

    /// Given a square of data, with northwest corner at pos and
    /// comprising ((1<<logSize)+1) verts along each axis, this function
    /// generates the mesh using verts which are active at the given level.
    ///
    /// If we're not at the base level (level > 0), then also recurses to
    /// quadtree child nodes and generates their data.
    void generateNodeData(Stream* s, Point2I pos, S32 log_size, S32 level, HeightType min = 0, HeightType max = 0);

    void generateSkirt(AtlasOldMesher* cm, const Point2I a, const Point2I b, const S8 level);
    void generateBlock(AtlasOldMesher* cm, const S8 activationLevel, S32 logSize, Point2I c);
    void generateQuadrant(AtlasOldMesher* cm, AtlasOldGenState& s, Point2I l, Point2I t, Point2I r, S32 recursionLevel);

    /// Does a quadtree descent through the heightfield, in the square with
    /// center at (cx, cz) and size of (2 ^ (level + 1) + 1).  Descends
    /// until level == target_level, and then propagates this square's
    /// child center verts to the corresponding edge vert, and the edge
    /// verts to the center.  Essentially the quadtree meshing update
    /// dependency graph per Thatcher Ulrich's Gamasutra article.  Must call
    /// this with successively increasing target_level to get correct
    /// propagation.
    void propagateActivationLevel(Point2I c, S8 level, S8 targetLevel);

    /// Debugging function -- verifies that activation level dependencies
    /// are correct throughout the tree.
    S8 checkPropagation(Point2I c, S8 level);

    /// Given the triangle, computes an error value and activation level
    /// for its base vertex, and recurses to child triangles.
    void update(F32 baseMaxError, Point2I a, Point2I r, Point2I l);

public:
    AtlasOldActivationHeightfield(const U32 sizeLog2, const F32 sampleSpacing, const F32 heightScale);
    virtual ~AtlasOldActivationHeightfield();

    // Activation Level Manipulation
    S8 getLevel(Point2I pos);
    void setLevel(Point2I pos, S8 level);
    void activate(Point2I pos, S8 level);

    /// Returns the height of the mesh as simplified to the specified level
    /// of detail.
    ///
    /// @return value is in heightfield discrete coords.  To get meters,
    /// multiply by the heightfield's vertical scale.
    HeightType getHeightAtLOD(Point2I pos, S8 level);

    /// Generate LOD chunks from the heightfield.
    /// 
    /// @param tree_depth Determines the depth of the chunk quadtree.
    /// @param base_max_error Specifies the maximum allowed geometric vertex error,
    ///                       at the finest level of detail.
    void generateChunkFile(Stream* s, S8 treeDepth, F32 baseMaxError);

    /// If set, do a variety of checks during generation to ensure we have valid
    /// activation levels and so forth. Also tells NVTriStrip to validate what we
    /// push through it.
    static bool smDoChecks;

    /// If set, set a debug flag in the file header and insert sentinel values
    /// in the datastream to ensure we've made valid data.
    static bool smGenerateDebugData;

    bool dumpHeightAtLOD(S8 level, const char* filename);
    bool dumpActivationLevels(const char* filename);
};

#endif