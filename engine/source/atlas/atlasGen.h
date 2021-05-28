//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
// Some portions of Atlas code derived from Thatcher Ulrich's original
// ChunkLOD implementation.
//-----------------------------------------------------------------------------

#ifndef _ATLASGEN_H_
#define _ATLASGEN_H_

#include "console/console.h"
#include "core/stream.h"
#include "core/tVector.h"
#include "math/mPoint.h"
#include "math/mBox.h"
#include "atlas/atlasHeightfield.h"
#include "atlas/atlasMesher.h"

class AtlasMesher;
class AtlasGenState;

/// Hardcoded size of per-chunk collision tree.
const U32 gAtlasColTreeDepth = 4;

/// This is just for data tracking purposes in AtlasGenStats. 256 is the deepest
/// level, so we use that. If you wanted to raise this, feel free.
const U32 gMaxChunkTreeLevels = 256;


/// Stat-tracking helper for AtlasMesher.
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
extern AtlasGenStats gChunkGenStats;

/// Adds activation level tracking functionality for AtlasHeightfield.
///
/// Activation levels are part of our BTT-based mesh decimation system. So if we
/// end up using a different/better mesh decimation algorithm, we can simply go
/// back to using AHFs.
class AtlasActivationHeightfield : public AtlasHeightfield
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

    void generateSkirt(AtlasMesher* cm, const Point2I a, const Point2I b, const S8 level);
    void generateBlock(AtlasMesher* cm, const S8 activationLevel, S32 logSize, Point2I c);
    void generateQuadrant(AtlasMesher* cm, AtlasGenState& s, Point2I l, Point2I t, Point2I r, S32 recursionLevel);

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
    AtlasActivationHeightfield(const U32 sizeLog2, const F32 sampleSpacing, const F32 heightScale);
    virtual ~AtlasActivationHeightfield();

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

// Disregarding below comments, an optimized variant of Ulrich's quadtree
// technique is actually pretty darn fast.
//
// We tried peano curve ordering, but this particular implementation is
// about 30% the speed of a straight lookup - if we had a faster/tighter
// implementation we might get a speedup, but I haven't the time to speed
// this up - BJG
//
// See numbers below, test was done using a 4096x4096 RAW heightfield and using
// Platform::random() to choose a random location, then do 8 height queries in
// the vicinity. Divide number of queries by 8 to see how many actual locations
// we tested.
/* SNIPPET 1
U32 buffOffset = 0;

for(S32 offset=mFieldShift; offset >= 0; offset--)
{
U8 xBit = (pos.x >> offset) & 1;
U8 yBit = (pos.y >> offset) & 1;

// Ok, now we have to figure out how much of an offset to add...
U8 value = (xBit << 1) | (yBit);

// Figure out which square we're in and how many entries are in the square, and offset
// to the beginning of it.
buffOffset += value * BIT(offset) * BIT(offset);
}

return buffOffset; */
/* SNIPPET 2
S32	l1 = lowest_one(pos.x | pos.y);
S32	depth = mFieldShift - l1 - 1;

S32	base = 0x55555555 & ((1 << depth*2) - 1);	// total node count in all levels above ours.
S32	shift = l1 + 1;

// Effective coords within this node's level.
S32	col = pos.x >> shift;
S32	row = pos.y >> shift;

return base + (row << depth) + col;
*/
//
//
//Ulrich implementation (snippet 2 above, optimized lowestOne to one op!)
// Did 33488928 queries in 765ms. Approx 43776.376471 queries/ms. Total=-1236945694
// Did 33488928 queries in 766ms. Approx 43719.227154 queries/ms. Total=-1203876672
// Did 33488928 queries in 766ms. Approx 43719.227154 queries/ms. Total=-1209497510
// Did 33488928 queries in 765ms. Approx 43776.376471 queries/ms. Total=-1292392390
// Did 33488928 queries in 750ms. Approx 44651.904000 queries/ms. Total=-1275192840
// Did 33488928 queries in 750ms. Approx 44651.904000 queries/ms. Total=-1237366386
// Did 33488928 queries in 766ms. Approx 43719.227154 queries/ms. Total=-1222232875
// Did 33488928 queries in 766ms. Approx 43719.227154 queries/ms. Total=-1269413604
// Did 33488928 queries in 766ms. Approx 43719.227154 queries/ms. Total=-1250863903
// Did 33488928 queries in 765ms. Approx 43776.376471 queries/ms. Total=-1235606091
// Did 33488928 queries in 766ms. Approx 43719.227154 queries/ms. Total=-1240341192
// Did 33488928 queries in 750ms. Approx 44651.904000 queries/ms. Total=-1261473984
// Did 33488928 queries in 750ms. Approx 44651.904000 queries/ms. Total=-1245423119
// Did 33488928 queries in 766ms. Approx 43719.227154 queries/ms. Total=-1275370609
// Did 33488928 queries in 765ms. Approx 43776.376471 queries/ms. Total=-1286488400
// Did 33488928 queries in 765ms. Approx 43776.376471 queries/ms. Total=-1260934751
// Did 33488928 queries in 766ms. Approx 43719.227154 queries/ms. Total=-1200136910
// Did 33488928 queries in 766ms. Approx 43719.227154 queries/ms. Total=-1272612475
// Did 33488928 queries in 750ms. Approx 44651.904000 queries/ms. Total=-1244523982
// Did 33488928 queries in 750ms. Approx 44651.904000 queries/ms. Total=-1251101185
// Did 33488928 queries in 750ms. Approx 44651.904000 queries/ms. Total=-1245452813
// Did 33488928 queries in 766ms. Approx 43719.227154 queries/ms. Total=-1251480301
// Did 33488928 queries in 766ms. Approx 43719.227154 queries/ms. Total=-1269054571
// Did 33488928 queries in 766ms. Approx 43719.227154 queries/ms. Total=-1243086156
// Did 33488928 queries in 765ms. Approx 43776.376471 queries/ms. Total=-1258120808
// Did 33488928 queries in 750ms. Approx 44651.904000 queries/ms. Total=-1237640686
// Did 33488928 queries in 750ms. Approx 44651.904000 queries/ms. Total=-1216895920
// Did 33488928 queries in 750ms. Approx 44651.904000 queries/ms. Total=-1269057376
// Did 33488928 queries in 766ms. Approx 43719.227154 queries/ms. Total=-1270712479
// Did 33488928 queries in 765ms. Approx 43776.376471 queries/ms. Total=-1262008932
// Did 33488928 queries in 765ms. Approx 43776.376471 queries/ms. Total=-1230496425
// Did 33488928 queries in 766ms. Approx 43719.227154 queries/ms. Total=-1199361141
// Did 33488928 queries in 766ms. Approx 43719.227154 queries/ms. Total=-1231752539
// Did 33488928 queries in 765ms. Approx 43776.376471 queries/ms. Total=-1232618946
// Did 33488928 queries in 765ms. Approx 43776.376471 queries/ms. Total=-1289550383
// Did 33488928 queries in 750ms. Approx 44651.904000 queries/ms. Total=-1260232797
// Did 33488928 queries in 750ms. Approx 44651.904000 queries/ms. Total=-1203499829
// Did 33488928 queries in 765ms. Approx 43776.376471 queries/ms. Total=-1247050171
// Did 33488928 queries in 765ms. Approx 43776.376471 queries/ms. Total=-1281592074
// Did 33488928 queries in 766ms. Approx 43719.227154 queries/ms. Total=-1237792172
// Did 33488928 queries in 766ms. Approx 43719.227154 queries/ms. Total=-1231737115
// Did 33488928 queries in 765ms. Approx 43776.376471 queries/ms. Total=-1241229425
// Did 33488928 queries in 765ms. Approx 43776.376471 queries/ms. Total=-1264867882
// Did 33488928 queries in 765ms. Approx 43776.376471 queries/ms. Total=-1200840459
// Did 33488928 queries in 765ms. Approx 43776.376471 queries/ms. Total=-1233780943
// Did 33488928 queries in 765ms. Approx 43776.376471 queries/ms. Total=-1245747106
// Did 33488928 queries in 781ms. Approx 42879.549296 queries/ms. Total=-1220967029
// Did 33488928 queries in 812ms. Approx 41242.522167 queries/ms. Total=-1236224141
// Did 33488928 queries in 907ms. Approx 36922.743109 queries/ms. Total=-1224348733
// Did 33488928 queries in 750ms. Approx 44651.904000 queries/ms. Total=-1238213789
//
//Bitmap implementation
// Did 33488928 queries in 1125ms. Approx 29767.936000 queries/ms. Total=79016161
// Did 33488928 queries in 1110ms. Approx 30170.205405 queries/ms. Total=4257816
// Did 33488928 queries in 1125ms. Approx 29767.936000 queries/ms. Total=-174934744
// Did 33488928 queries in 1110ms. Approx 30170.205405 queries/ms. Total=-135875315
// Did 33488928 queries in 1125ms. Approx 29767.936000 queries/ms. Total=-75700463
// Did 33488928 queries in 1109ms. Approx 30197.410280 queries/ms. Total=-35175714
// Did 33488928 queries in 1109ms. Approx 30197.410280 queries/ms. Total=-22406448
// Did 33488928 queries in 1125ms. Approx 29767.936000 queries/ms. Total=-123973505
// Did 33488928 queries in 1109ms. Approx 30197.410280 queries/ms. Total=-94731505
// Did 33488928 queries in 1125ms. Approx 29767.936000 queries/ms. Total=30638677
// Did 33488928 queries in 1109ms. Approx 30197.410280 queries/ms. Total=-227981117
// Did 33488928 queries in 1125ms. Approx 29767.936000 queries/ms. Total=-183785231
// Did 33488928 queries in 1110ms. Approx 30170.205405 queries/ms. Total=-12858778
// Did 33488928 queries in 1110ms. Approx 30170.205405 queries/ms. Total=-16283202
// Did 33488928 queries in 1125ms. Approx 29767.936000 queries/ms. Total=74251268
// Did 33488928 queries in 1109ms. Approx 30197.410280 queries/ms. Total=-73333555
// Did 33488928 queries in 1109ms. Approx 30197.410280 queries/ms. Total=-80884408
// Did 33488928 queries in 1109ms. Approx 30197.410280 queries/ms. Total=61646045
// Did 33488928 queries in 1109ms. Approx 30197.410280 queries/ms. Total=16146099
// Did 33488928 queries in 1125ms. Approx 29767.936000 queries/ms. Total=-130808963
// Did 33488928 queries in 1109ms. Approx 30197.410280 queries/ms. Total=181617626
// Did 33488928 queries in 1125ms. Approx 29767.936000 queries/ms. Total=-124397736
// Did 33488928 queries in 1110ms. Approx 30170.205405 queries/ms. Total=-21226813
// Did 33488928 queries in 1125ms. Approx 29767.936000 queries/ms. Total=-62303637
// Did 33488928 queries in 1110ms. Approx 30170.205405 queries/ms. Total=9355037
// Did 33488928 queries in 1125ms. Approx 29767.936000 queries/ms. Total=-68131936
// Did 33488928 queries in 1110ms. Approx 30170.205405 queries/ms. Total=-45309678
// Did 33488928 queries in 1109ms. Approx 30197.410280 queries/ms. Total=49539278
// Did 33488928 queries in 1109ms. Approx 30197.410280 queries/ms. Total=-38191501
// Did 33488928 queries in 1109ms. Approx 30197.410280 queries/ms. Total=-57184782
// Did 33488928 queries in 1109ms. Approx 30197.410280 queries/ms. Total=-169681680
// Did 33488928 queries in 1109ms. Approx 30197.410280 queries/ms. Total=-52417964
// Did 33488928 queries in 1109ms. Approx 30197.410280 queries/ms. Total=-61128656
// Did 33488928 queries in 1109ms. Approx 30197.410280 queries/ms. Total=-132887319
// Did 33488928 queries in 1125ms. Approx 29767.936000 queries/ms. Total=28391448
// Did 33488928 queries in 1110ms. Approx 30170.205405 queries/ms. Total=-121594865
// Did 33488928 queries in 1109ms. Approx 30197.410280 queries/ms. Total=31822692
// Did 33488928 queries in 1110ms. Approx 30170.205405 queries/ms. Total=31047383
// Did 33488928 queries in 1109ms. Approx 30197.410280 queries/ms. Total=-131085207
// Did 33488928 queries in 1110ms. Approx 30170.205405 queries/ms. Total=-76026366
// Did 33488928 queries in 1109ms. Approx 30197.410280 queries/ms. Total=64103651
// Did 33488928 queries in 1125ms. Approx 29767.936000 queries/ms. Total=149862972
// Did 33488928 queries in 1109ms. Approx 30197.410280 queries/ms. Total=-108706761
// Did 33488928 queries in 1125ms. Approx 29767.936000 queries/ms. Total=-87208965
// Did 33488928 queries in 1109ms. Approx 30197.410280 queries/ms. Total=-64603715
// Did 33488928 queries in 1110ms. Approx 30170.205405 queries/ms. Total=81841371
// Did 33488928 queries in 1109ms. Approx 30197.410280 queries/ms. Total=-107508960
// Did 33488928 queries in 1110ms. Approx 30170.205405 queries/ms. Total=-254068469
// Did 33488928 queries in 1125ms. Approx 29767.936000 queries/ms. Total=15558466
// Did 33488928 queries in 1110ms. Approx 30170.205405 queries/ms. Total=-169699357
//
//Ulrich implementation (snippet 2 above, unoptimized lowest_one)

// Did 33488928 queries in 1781ms. Approx 18803.440764 queries/ms. Total=46535122
// Did 33488928 queries in 1797ms. Approx 18636.020033 queries/ms. Total=113398323
// Did 33488928 queries in 1781ms. Approx 18803.440764 queries/ms. Total=37233619
// Did 33488928 queries in 1797ms. Approx 18636.020033 queries/ms. Total=-144274863
// Did 33488928 queries in 1797ms. Approx 18636.020033 queries/ms. Total=-103234102
// Did 33488928 queries in 1797ms. Approx 18636.020033 queries/ms. Total=-44389009
// Did 33488928 queries in 1781ms. Approx 18803.440764 queries/ms. Total=-1565398
// Did 33488928 queries in 1781ms. Approx 18803.440764 queries/ms. Total=8444675
// Did 33488928 queries in 1796ms. Approx 18646.396437 queries/ms. Total=-91586691
// Did 33488928 queries in 1781ms. Approx 18803.440764 queries/ms. Total=-62375626
// Did 33488928 queries in 1781ms. Approx 18803.440764 queries/ms. Total=63011724
// Did 33488928 queries in 1797ms. Approx 18636.020033 queries/ms. Total=-196598465
// Did 33488928 queries in 1797ms. Approx 18636.020033 queries/ms. Total=-150587331
// Did 33488928 queries in 1781ms. Approx 18803.440764 queries/ms. Total=18701061
// Did 33488928 queries in 1782ms. Approx 18792.888889 queries/ms. Total=15892112
// Did 33488928 queries in 1781ms. Approx 18803.440764 queries/ms. Total=106463772
// Did 33488928 queries in 1813ms. Approx 18471.554330 queries/ms. Total=-42068349
// Did 33488928 queries in 1812ms. Approx 18481.748344 queries/ms. Total=-47652196
// Did 33488928 queries in 1796ms. Approx 18646.396437 queries/ms. Total=94732599
// Did 33488928 queries in 1797ms. Approx 18636.020033 queries/ms. Total=48899770
// Did 33488928 queries in 1781ms. Approx 18803.440764 queries/ms. Total=-99202040
// Did 33488928 queries in 1781ms. Approx 18803.440764 queries/ms. Total=213865826
// Did 33488928 queries in 2047ms. Approx 16360.003908 queries/ms. Total=-91430509
// Did 33488928 queries in 1781ms. Approx 18803.440764 queries/ms. Total=12634391
// Did 33488928 queries in 1797ms. Approx 18636.020033 queries/ms. Total=-31315514
// Did 33488928 queries in 1796ms. Approx 18646.396437 queries/ms. Total=39020605
// Did 33488928 queries in 1797ms. Approx 18636.020033 queries/ms. Total=-34275646
// Did 33488928 queries in 1813ms. Approx 18471.554330 queries/ms. Total=-13184248
// Did 33488928 queries in 1812ms. Approx 18481.748344 queries/ms. Total=82738032
// Did 33488928 queries in 1968ms. Approx 17016.731707 queries/ms. Total=-7691697
// Did 33488928 queries in 1797ms. Approx 18636.020033 queries/ms. Total=-24485327
// Did 33488928 queries in 1875ms. Approx 17860.761600 queries/ms. Total=-135542864
// Did 33488928 queries in 1906ms. Approx 17570.266527 queries/ms. Total=-22245195
// Did 33488928 queries in 1844ms. Approx 18161.023861 queries/ms. Total=-28286997
// Did 33488928 queries in 1796ms. Approx 18646.396437 queries/ms. Total=-102080331
// Did 33488928 queries in 1781ms. Approx 18803.440764 queries/ms. Total=60160742
// Did 33488928 queries in 1781ms. Approx 18803.440764 queries/ms. Total=-89845011
// Did 33488928 queries in 1797ms. Approx 18636.020033 queries/ms. Total=63153711
// Did 33488928 queries in 1797ms. Approx 18636.020033 queries/ms. Total=63066674
// Did 33488928 queries in 1844ms. Approx 18161.023861 queries/ms. Total=-100672955
// Did 33488928 queries in 1812ms. Approx 18481.748344 queries/ms. Total=-43583591
// Did 33488928 queries in 1843ms. Approx 18170.877916 queries/ms. Total=97836825
// Did 33488928 queries in 1781ms. Approx 18803.440764 queries/ms. Total=181539612
// Did 33488928 queries in 1781ms. Approx 18803.440764 queries/ms. Total=-74726674
// Did 33488928 queries in 1782ms. Approx 18792.888889 queries/ms. Total=-55707963
// Did 33488928 queries in 1797ms. Approx 18636.020033 queries/ms. Total=-31864668
// Did 33488928 queries in 1781ms. Approx 18803.440764 queries/ms. Total=112349940
// Did 33488928 queries in 1797ms. Approx 18636.020033 queries/ms. Total=-72865781
// Did 33488928 queries in 1781ms. Approx 18803.440764 queries/ms. Total=-221997272
// Did 33488928 queries in 5750ms. Approx 5824.161391 queries/ms. Total=46598972
//
//Peano implementation (snippet 1 above)
// Did 33488928 queries in 2734ms. Approx 12249.059254 queries/ms. Total=14615556
// Did 33488928 queries in 2750ms. Approx 12177.792000 queries/ms. Total=79016161
// Did 33488928 queries in 2750ms. Approx 12177.792000 queries/ms. Total=4257816
// Did 33488928 queries in 2735ms. Approx 12244.580622 queries/ms. Total=-174934744
// Did 33488928 queries in 2735ms. Approx 12244.580622 queries/ms. Total=-135875315
// Did 33488928 queries in 2750ms. Approx 12177.792000 queries/ms. Total=-75700463
// Did 33488928 queries in 2750ms. Approx 12177.792000 queries/ms. Total=-35175714
// Did 33488928 queries in 2750ms. Approx 12177.792000 queries/ms. Total=-22406448
// Did 33488928 queries in 3031ms. Approx 11048.805015 queries/ms. Total=-123973505
// Did 33488928 queries in 2735ms. Approx 12244.580622 queries/ms. Total=-94731505
// Did 33488928 queries in 2750ms. Approx 12177.792000 queries/ms. Total=30638677
// Did 33488928 queries in 2735ms. Approx 12244.580622 queries/ms. Total=-227981117
// Did 33488928 queries in 2734ms. Approx 12249.059254 queries/ms. Total=-183785231
// Did 33488928 queries in 2735ms. Approx 12244.580622 queries/ms. Total=-12858778
// Did 33488928 queries in 2734ms. Approx 12249.059254 queries/ms. Total=-16283202
// Did 33488928 queries in 2734ms. Approx 12249.059254 queries/ms. Total=74251268
// Did 33488928 queries in 2734ms. Approx 12249.059254 queries/ms. Total=-73333555
// Did 33488928 queries in 2734ms. Approx 12249.059254 queries/ms. Total=-80884408
// Did 33488928 queries in 2735ms. Approx 12244.580622 queries/ms. Total=61646045
// Did 33488928 queries in 2734ms. Approx 12249.059254 queries/ms. Total=16146099
// Did 33488928 queries in 2750ms. Approx 12177.792000 queries/ms. Total=-130808963
// Did 33488928 queries in 2734ms. Approx 12249.059254 queries/ms. Total=181617626
// Did 33488928 queries in 2734ms. Approx 12249.059254 queries/ms. Total=-124397736
// Did 33488928 queries in 2750ms. Approx 12177.792000 queries/ms. Total=-21226813
// Did 33488928 queries in 2734ms. Approx 12249.059254 queries/ms. Total=-62303637
// Did 33488928 queries in 2735ms. Approx 12244.580622 queries/ms. Total=9355037
// Did 33488928 queries in 2750ms. Approx 12177.792000 queries/ms. Total=-68131936
// Did 33488928 queries in 2750ms. Approx 12177.792000 queries/ms. Total=-45309678
// Did 33488928 queries in 2750ms. Approx 12177.792000 queries/ms. Total=49539278
// Did 33488928 queries in 2734ms. Approx 12249.059254 queries/ms. Total=-38191501
// Did 33488928 queries in 2735ms. Approx 12244.580622 queries/ms. Total=-57184782
// Did 33488928 queries in 2735ms. Approx 12244.580622 queries/ms. Total=-169681680
// Did 33488928 queries in 2735ms. Approx 12244.580622 queries/ms. Total=-52417964
// Did 33488928 queries in 2734ms. Approx 12249.059254 queries/ms. Total=-61128656
// Did 33488928 queries in 2750ms. Approx 12177.792000 queries/ms. Total=-132887319
// Did 33488928 queries in 2734ms. Approx 12249.059254 queries/ms. Total=28391448
// Did 33488928 queries in 2734ms. Approx 12249.059254 queries/ms. Total=-121594865
// Did 33488928 queries in 2750ms. Approx 12177.792000 queries/ms. Total=31822692
// Did 33488928 queries in 2734ms. Approx 12249.059254 queries/ms. Total=31047383
// Did 33488928 queries in 2735ms. Approx 12244.580622 queries/ms. Total=-131085207
// Did 33488928 queries in 2750ms. Approx 12177.792000 queries/ms. Total=-76026366
// Did 33488928 queries in 2735ms. Approx 12244.580622 queries/ms. Total=64103651
// Did 33488928 queries in 2750ms. Approx 12177.792000 queries/ms. Total=149862972
// Did 33488928 queries in 2734ms. Approx 12249.059254 queries/ms. Total=-108706761
// Did 33488928 queries in 2735ms. Approx 12244.580622 queries/ms. Total=-87208965
// Did 33488928 queries in 2735ms. Approx 12244.580622 queries/ms. Total=-64603715
// Did 33488928 queries in 2734ms. Approx 12249.059254 queries/ms. Total=81841371
// Did 33488928 queries in 2734ms. Approx 12249.059254 queries/ms. Total=-107508960
// Did 33488928 queries in 2734ms. Approx 12249.059254 queries/ms. Total=-254068469
// Did 33488928 queries in 2750ms. Approx 12177.792000 queries/ms. Total=15558466