//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _TERRRENDER_H_
#define _TERRRENDER_H_

#ifndef _COLOR_H_
#include "core/color.h"
#endif
#ifndef _TERRDATA_H_
#include "terrain/terrData.h"
#endif

#include "gfx/gfxTextureHandle.h"
#include "materials/matInstance.h"

struct EmitChunk;
struct AllocatedTexture;

struct EdgePoint : public Point3F
{
    Point3F vn;
};

struct ChunkCornerPoint : public EdgePoint
{
    U32 pointIndex;
    U32 xfIndex;
};

struct EdgeParent
{
    ChunkCornerPoint* p1, * p2;
};

struct ChunkScanEdge : public EdgeParent
{
    ChunkCornerPoint* mp;
    EdgeParent* e1, * e2;
};

struct ChunkEdge : public EdgeParent
{
    U32 xfIndex;
    U16 pointIndex;
    U16 pointCount;

    EdgePoint pt[3];
    EmitChunk* c1, * c2;
};

struct EmitChunk
{
    ChunkEdge* edge[4];
    S32 subDivLevel;
    F32 growFactor;
    S32 x, y;
    S32 gridX, gridY;
    U32 emptyFlags;
    bool clip;
    U32 lightMask;
    EmitChunk* next;
    bool renderDetails;
};

struct SquareStackNode
{
    U32     clipFlags;
    U32     lightMask;
    Point2I pos;
    U32     level;
    bool    texAllocated;
    EdgeParent* top, * right, * bottom, * left;
};

struct TerrLightInfo
{
    Point3F pos;       ///< world position
    F32 radius;        ///< radius of the light
    F32 radiusSquared; ///< radius^2
    F32 r, g, b;

    F32 distSquared; // distance to camera

    LightInfo* light;
};

struct LightingChunkInfo
{
    EmitChunk* chunk;
    GFXTexHandle* texture;
    Point4F texGenS;
    Point4F texGenT;
};

enum EmptyFlags {
    SquareEmpty_0_0 = BIT(0),
    SquareEmpty_1_0 = BIT(1),
    SquareEmpty_2_0 = BIT(2),
    SquareEmpty_3_0 = BIT(3),
    SquareEmpty_0_1 = BIT(4),
    SquareEmpty_1_1 = BIT(5),
    SquareEmpty_2_1 = BIT(6),
    SquareEmpty_3_1 = BIT(7),
    SquareEmpty_0_2 = BIT(8),
    SquareEmpty_1_2 = BIT(9),
    SquareEmpty_2_2 = BIT(10),
    SquareEmpty_3_2 = BIT(11),
    SquareEmpty_0_3 = BIT(12),
    SquareEmpty_1_3 = BIT(13),
    SquareEmpty_2_3 = BIT(14),
    SquareEmpty_3_3 = BIT(15),
    CornerEmpty_0_0 = SquareEmpty_0_0 | SquareEmpty_1_0 | SquareEmpty_0_1 | SquareEmpty_1_1,
    CornerEmpty_1_0 = SquareEmpty_2_0 | SquareEmpty_3_0 | SquareEmpty_2_1 | SquareEmpty_3_1,
    CornerEmpty_0_1 = SquareEmpty_0_2 | SquareEmpty_1_2 | SquareEmpty_0_3 | SquareEmpty_1_3,
    CornerEmpty_1_1 = SquareEmpty_2_2 | SquareEmpty_3_2 | SquareEmpty_2_3 | SquareEmpty_3_3,
};

struct RenderPoint : public Point3F
{
};

struct LightTriangle {
    ColorF  color;

    Point2F texco1;
    Point3F point1;
    Point2F texco2;
    Point3F point2;
    Point2F texco3;
    Point3F point3;

    LightTriangle* next;
    U32            flags;  // 0 if inactive
};

enum TerrConstants {
    MaxClipPlanes = 8, ///< left, right, top, bottom - don't need far tho...
    MaxTerrainMaterials = 256,

    EdgeStackSize = 1024, ///< value for water/terrain edge stack size.
    MaxDetailLevel = 9,
    MaxMipLevel = 8,
    MaxTerrainLights = 64,
    MaxVisibleLights = 31,
    ClipPlaneMask = (1 << MaxClipPlanes) - 1,
    FarSphereMask = 0x80000000,
    FogPlaneBoxMask = 0x40000000,
    VertexBufferSize = 65 * 65 + 1000,
    AllocatedTextureCount = 16 + 64 + 256 + 1024 + 4096,

    TerrainTextureMipLevel = 6, ///< mip level of generated textures
 //   TerrainTextureSize     = 1 << TerrainTextureMipLevel, ///< size of generated textures
 //   SmallMipLevel          = 6
};

class SceneState;

inline U32 getPower(U32 x)
{
    F32 floatValue = F32(x);
    return (*((U32*)&floatValue) >> 23) - 127;
}


struct TerrainRender
{
    static MatrixF mCameraToObject;

    static SceneState* mSceneState;

    static TerrainBlock* mCurrentBlock;
    static S32 mSquareSize;
    static F32 mScreenSize;
    static U32 mFrameIndex;
    static U32 mNumClipPlanes;

    static Point2F mBlockPos;
    static Point2I mBlockOffset;
    static Point2I mTerrainOffset;

    static PlaneF mClipPlane[MaxClipPlanes];
    static Point3F mCamPos;
    static U32 mDynamicLightCount;
    static bool mEnableTerrainDynLights;

    static F32 mPixelError;

    static TerrLightInfo mTerrainLights[MaxTerrainLights];
    static F32 mScreenError;
    static F32 mMinSquareSize;
    static F32 mFarDistance;
    static F32 mInvFarDistance;
    static F32 mInvHeightRange;
    static bool mRenderingCommander;

    // Control...
    static void init();
    static void shutdown();

    // Points...
    static ChunkCornerPoint* allocInitialPoint(Point3F pos);
    static ChunkCornerPoint* allocPoint(Point2I pos);

    // Edges...
    static void allocRenderEdges(U32 edgeCount, EdgeParent** dest, bool renderEdge);
    static void fixEdge(ChunkEdge* edge, S32 x, S32 y, S32 dx, S32 dy);
    static void subdivideChunkEdge(ChunkScanEdge* e, Point2I pos, bool chunkEdge);

    // Core functionality...
    static void processBlock(SceneState* state, EdgeParent* topEdge, EdgeParent* rightEdge, EdgeParent* bottomEdge, EdgeParent* leftEdge);
    static void emitTerrChunk(SquareStackNode* n, F32 squareDistance, U32 lightMask, bool farClip, bool useDetails);
    static void renderBlock(TerrainBlock*, SceneState* state, MatInstance* m, SceneGraphData& sgData);
    static void renderChunkNormal(EmitChunk* chunk);
    static void renderChunkCommander(EmitChunk* chunk);

    // Helper functions...
    static U32 testSquareLights(GridSquare* sq, S32 level, Point2I pos, U32 lightMask);
    static S32 testSquareVisibility(Point3F& min, Point3F& max, S32 clipMask, F32 expand);

    static F32 getSquareDistance(const Point3F& minPoint, const Point3F& maxPoint,
        F32* zDiff);

    static void buildLightArray();
    static void buildClippingPlanes(bool flipClipPlanes);

};

#endif
