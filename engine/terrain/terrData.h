//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _TERRDATA_H_
#define _TERRDATA_H_

#ifndef _PLATFORM_H_
#include "platform/platform.h"
#endif
#ifndef _MPOINT_H_
#include "math/mPoint.h"
#endif
#ifndef _SCENEOBJECT_H_
#include "sim/sceneObject.h"
#endif
#ifndef _RESMANAGER_H_
#include "core/resManager.h"
#endif
#ifndef _CONVEX_H_
#include "collision/convex.h"
#endif

#include "materials/materialList.h"
#include "gfx/gfxDevice.h"

class GBitmap;
class TerrainFile;
class TerrainBlock;
class ColorF;
class Blender;


//--------------------------------------------------------------------------

class TerrainConvex : public Convex
{
    friend class TerrainBlock;
    TerrainConvex* square;     ///< Alternate convex if square is concave
    bool halfA;                ///< Which half of square
    bool split45;              ///< Square split pattern
    U32 squareId;              ///< Used to match squares
    U32 material;
    Point3F point[4];          ///< 3-4 vertices
    VectorF normal[2];
    Box3F box;                 ///< Bounding box

public:
    TerrainConvex() { mType = TerrainConvexType; }
    TerrainConvex(const TerrainConvex& cv) {
        mType = TerrainConvexType;

        // Only a partial copy...
        mObject = cv.mObject;
        split45 = cv.split45;
        squareId = cv.squareId;
        material = cv.material;
        point[0] = cv.point[0];
        point[1] = cv.point[1];
        point[2] = cv.point[2];
        point[3] = cv.point[3];
        normal[0] = cv.normal[0];
        normal[1] = cv.normal[1];
        box = cv.box;
    }
    Box3F getBoundingBox() const;
    Box3F getBoundingBox(const MatrixF& mat, const Point3F& scale) const;
    Point3F support(const VectorF& v) const;
    void getFeatures(const MatrixF& mat, const VectorF& n, ConvexFeature* cf);
    void getPolyList(AbstractPolyList* list);
};



//--------------------------------------------------------------------------
struct GridSquare
{
    U16 minHeight;
    U16 maxHeight;
    U16 heightDeviance;
    U16 flags;
    enum {
        Split45 = 1,
        Empty = 2,
        HasEmpty = 4,
        MaterialShift = 3,
        MaterialStart = 8,
        Material0 = 8,
        Material1 = 16,
        Material2 = 32,
        Material3 = 64,
    };
};

struct GridChunk
{
    U16 heightDeviance[3]; // levels 0-1, 1-2, 2
    U16 emptyFlags;
};


//--------------------------------------------------------------------------
class TerrainBlock : public SceneObject
{
    typedef SceneObject Parent;

public:
    struct Material {
        enum Flags {
            Plain = 0,
            Rotate = 1,
            FlipX = 2,
            FlipXRotate = 3,
            FlipY = 4,
            FlipYRotate = 5,
            FlipXY = 6,
            FlipXYRotate = 7,
            RotateMask = 7,
            Empty = 8,
            Modified = BIT(7),

            // must not clobber TerrainFile::MATERIAL_GROUP_MASK bits!
            PersistMask = BIT(7)
        };
        U8 flags;
        U8 index;
    };
    enum {
        BlockSize = 256,
        BlockShift = 8,
        LightmapSize = 1024,
        LightmapShift = 10,
        ChunkSquareWidth = 64,
        ChunkSize = 4,
        ChunkDownShift = 2,
        ChunkShift = BlockShift - ChunkDownShift,
        BlockSquareWidth = 256,
        SquareMaxPoints = 1024,
        BlockMask = 255,
        GridMapSize = 0x15555,
        FlagMapWidth = 128, ///< Flags that map is for 2x2 squares.
        FlagMapMask = 127,
        MaxMipLevel = 6,
        NumBaseTextures = 16,
        MaterialGroups = 8,
        MaxEmptyRunPairs = 100
    };
    enum UpdateMaskBits {
        InitMask = 1,
        VisibilityMask = 2,
        EmptyMask = 4,
    };

    Blender* mBlender;

    GFXTexHandle baseTextures[NumBaseTextures];
    GFXTexHandle mBaseMaterials[MaterialGroups];
    GFXTexHandle mAlphaMaterials[MaterialGroups];

    GBitmap* lightMap;
    GFXTexHandle lightMapTex;

    GFXTexHandle getLightMapTexture()
    {
        if (!lightMap)
            return NULL;

        if (!lightMapTex.isValid())
            lightMapTex.set(lightMap, &GFXDefaultPersistentProfile, false);

        return lightMapTex;
    }

    StringTableEntry* mMaterialFileName; ///< Array from the file.

    GFXTexHandle mDynLightTexture;

    static void texManagerCallback(GFXTexCallbackCode code, void* userData);
    S32 mTexCallback;

    U8* mBaseMaterialMap;
    Material* materialMap;
    // fixed point height values
    U16* heightMap;
    U16* flagMap;
    StringTableEntry mDetailTextureName;
    GFXTexHandle mDetailTextureHandle;

    GFXVertexBufferHandle<GFXVertexPCT> mTerrainVerts;
    U32                                 mTerrainPrimCount;

    //CW - bump mapping stuff
    StringTableEntry mBumpTextureName;
    //	TextureHandle mBumpTextureHandle;
    //	TextureHandle mInvertedBumpTextureHandle;
    F32 mBumpScale;
    F32 mBumpOffset;
    U32 mZeroBumpScale;
    //CW - end bump mapping stuff
    StringTableEntry mTerrFileName;
    Vector<S32> mEmptySquareRuns;

    S32 mMPMIndex[10];

    S32 mVertexBuffer;

private:
    Resource<TerrainFile> mFile;
    GridSquare* gridMap[BlockShift + 1];
    GridChunk* mChunkMap;
    U32   mCRC;

public:

    TerrainBlock();
    ~TerrainBlock();
    void buildChunkDeviance(S32 x, S32 y);
    void buildGridMap();
    U32 getCRC() { return(mCRC); }
    Resource<TerrainFile> getFile() { return mFile; };

    bool onAdd();
    void onRemove();

    void refreshMaterialLists();
    void onEditorEnable();
    void onEditorDisable();

    void rebuildEmptyFlags();
    bool unpackEmptySquares();
    void packEmptySquares();

    //   TextureHandle getDetailTextureHandle();
    //CW - bump mapping stuff
    //	TextureHandle getBumpTextureHandle();
    //	TextureHandle getInvertedBumpTextureHandle();
    //CW - end bump mapping stuff
    Material* getMaterial(U32 x, U32 y);
    GridSquare* findSquare(U32 level, Point2I pos);
    GridSquare* findSquare(U32 level, S32 x, S32 y);
    GridChunk* findChunk(Point2I pos);

    void setHeight(const Point2I& pos, float height);
    void updateGrid(Point2I min, Point2I max);
    void updateGridMaterials(Point2I min, Point2I max);

    U16 getHeight(U32 x, U32 y) { return heightMap[(x & BlockMask) + ((y & BlockMask) << BlockShift)]; }
    U16* getHeightAddress(U32 x, U32 y) { return &heightMap[(x & BlockMask) + ((y & BlockMask) << BlockShift)]; }
    void setBaseMaterial(U32 x, U32 y, U8 matGroup);

    U8* getMaterialAlphaMap(U32 matIndex);
    U8* getBaseMaterialAddress(U32 x, U32 y);
    U8  getBaseMaterial(U32 x, U32 y);
    U16* getFlagMapPtr(S32 x, S32 y);

    void getMaterialAlpha(Point2I pos, U8 alphas[MaterialGroups]);
    void setMaterialAlpha(Point2I pos, const U8 alphas[MaterialGroups]);

    // a more useful getHeight for the public...
    bool getHeight(const Point2F& pos, F32* height);
    bool getNormal(const Point2F& pos, Point3F* normal, bool normalize = true);
    bool getNormalAndHeight(const Point2F& pos, Point3F* normal, F32* height, bool normalize = true);

    // only the editor currently uses this method - should always be using a ray to collide with
    bool collideBox(const Point3F& start, const Point3F& end, RayInfo* info) { return(castRay(start, end, info)); }
    S32 getMaterialAlphaIndex(const char* materialName);

private:
    S32 squareSize;

public:
    void setFile(Resource<TerrainFile> file);
    bool save(const char* filename);

    static void flushCache();
    //void relight(const ColorF &lightColor, const ColorF &ambient, const Point3F &lightDir);

    S32 getSquareSize();

    //--------------------------------------
    // SceneGraph functions...

protected:
    void setTransform(const MatrixF& mat);
    bool prepRenderImage(SceneState* state, const U32 stateKey, const U32 startZone, const bool modifyBaseZoneState = false);
    void renderObject(SceneState* state, RenderInst* ri);

    //--------------------------------------
    // collision info
private:
    BSPTree* mTree;
    S32 mHeightMin;
    S32 mHeightMax;
public:
    void buildConvex(const Box3F& box, Convex* convex);
    bool buildPolyList(AbstractPolyList* polyList, const Box3F& box, const SphereF& sphere);
    //   BSPNode *buildCollisionBSP(BSPTree *tree, const Box3F &box, const SphereF &sphere);
    bool castRay(const Point3F& start, const Point3F& end, RayInfo* info);
    bool castRayI(const Point3F& start, const Point3F& end, RayInfo* info, bool emptyCollide);
    bool castRayBlock(const Point3F& pStart, const Point3F& pEnd, Point2I blockPos, U32 level, F32 invDeltaX, F32 invDeltaY, F32 startT, F32 endT, RayInfo* info, bool);
private:
    BSPNode* buildSquareTree(S32 y, S32 x);
    BSPNode* buildXTree(S32 y, S32 xStart, S32 xEnd);

public:
    bool buildMaterialMap();
    void buildMipMap();

    void setBaseMaterials(S32 argc, const char* argv[]);
    bool loadBaseMaterials();
    bool initMMXBlender();

    // private helper
private:
    bool        mCollideEmpty;

public:
    DECLARE_CONOBJECT(TerrainBlock);
    static void initPersistFields();
    U32 packUpdate(NetConnection* conn, U32 mask, BitStream* stream);
    void unpackUpdate(NetConnection* conn, BitStream* stream);
};



//--------------------------------------
class TerrainFile : public ResourceInstance
{
public:
    enum Constants {
        FILE_VERSION = 3,
        MATERIAL_GROUP_MASK = 0x7
    };

    TerrainFile();
    ~TerrainFile();
    U16 mHeightMap[TerrainBlock::BlockSize * TerrainBlock::BlockSize];
    U8  mBaseMaterialMap[TerrainBlock::BlockSize * TerrainBlock::BlockSize];
    GridSquare mGridMapBase[TerrainBlock::GridMapSize];
    GridSquare* mGridMap[TerrainBlock::BlockShift + 1];
    GridChunk mChunkMap[TerrainBlock::ChunkSquareWidth * TerrainBlock::ChunkSquareWidth];
    U16 mFlagMap[TerrainBlock::FlagMapWidth * TerrainBlock::FlagMapWidth];
    char* mTextureScript;
    char* mHeightfieldScript;

    TerrainBlock::Material mMaterialMap[TerrainBlock::BlockSquareWidth * TerrainBlock::BlockSquareWidth];

    // DMMNOTE: This loads all the alpha maps, whether or not they are used.  Possible to
    //  restrict to only the used versions?
    StringTableEntry mMaterialFileName[TerrainBlock::MaterialGroups];
    U8* mMaterialAlphaMap[TerrainBlock::MaterialGroups];

    bool save(const char* filename);
    void buildChunkDeviance(S32 x, S32 y);
    void buildGridMap();
    void heightDevLine(U32 p1x, U32 p1y, U32 p2x, U32 p2y, U32 pmx, U32 pmy, U16* devPtr);

    inline GridSquare* findSquare(U32 level, Point2I pos)
    {
        return mGridMap[level] + (pos.x >> level) + ((pos.y >> level) << (TerrainBlock::BlockShift - level));
    }

    inline U16 getHeight(U32 x, U32 y)
    {
        return mHeightMap[(x & TerrainBlock::BlockMask) + ((y & TerrainBlock::BlockMask) << TerrainBlock::BlockShift)];
    }

    inline TerrainBlock::Material* getMaterial(U32 x, U32 y)
    {
        return &mMaterialMap[(x & TerrainBlock::BlockMask) + ((y & TerrainBlock::BlockMask) << TerrainBlock::BlockShift)];
    }

    void setTextureScript(const char* script);
    void setHeightfieldScript(const char* script);
    const char* getTextureScript();
    const char* getHeightfieldScript();
};



//--------------------------------------------------------------------------
inline U16* TerrainBlock::getFlagMapPtr(S32 x, S32 y)
{
    return flagMap + ((x >> 1) & TerrainBlock::FlagMapMask) +
        ((y >> 1) & TerrainBlock::FlagMapMask) * TerrainBlock::FlagMapWidth;
}

inline GridSquare* TerrainBlock::findSquare(U32 level, S32 x, S32 y)
{
    return gridMap[level] + ((x & TerrainBlock::BlockMask) >> level) + (((y & TerrainBlock::BlockMask) >> level) << (TerrainBlock::BlockShift - level));
}

inline GridSquare* TerrainBlock::findSquare(U32 level, Point2I pos)
{
    return gridMap[level] + (pos.x >> level) + ((pos.y >> level) << (BlockShift - level));
}

inline GridChunk* TerrainBlock::findChunk(Point2I pos)
{
    return mChunkMap + (pos.x >> ChunkDownShift) + ((pos.y >> ChunkDownShift) << ChunkShift);
}

inline TerrainBlock::Material* TerrainBlock::getMaterial(U32 x, U32 y)
{
    return materialMap + x + (y << BlockShift);
}

/*
inline TextureHandle TerrainBlock::getDetailTextureHandle()
{
   return mDetailTextureHandle;
}
*/

inline S32 TerrainBlock::getSquareSize()
{
    return squareSize;
}

inline U8 TerrainBlock::getBaseMaterial(U32 x, U32 y)
{
    return mBaseMaterialMap[(x & BlockMask) + ((y & BlockMask) << BlockShift)];
}

inline U8* TerrainBlock::getBaseMaterialAddress(U32 x, U32 y)
{
    return &mBaseMaterialMap[(x & BlockMask) + ((y & BlockMask) << BlockShift)];
}

inline U8* TerrainBlock::getMaterialAlphaMap(U32 matIndex)
{
    if (mFile->mMaterialAlphaMap[matIndex] == NULL) {
        mFile->mMaterialAlphaMap[matIndex] = new U8[TerrainBlock::BlockSize * TerrainBlock::BlockSize];
        dMemset(mFile->mMaterialAlphaMap[matIndex], 0, TerrainBlock::BlockSize * TerrainBlock::BlockSize);
    }

    return mFile->mMaterialAlphaMap[matIndex];
}

// 11.5 fixed point - gives us a height range from 0->2048 in 1/32 inc

inline F32 fixedToFloat(U16 val)
{
    return F32(val) * 0.03125f;
}

inline U16 floatToFixed(F32 val)
{
    return U16(val * 32.0);
}

extern ResourceInstance* constructTerrainFile(Stream& stream);

#endif
