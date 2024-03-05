//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "terrain/terrData.h"
#include "gfx/gBitmap.h"
#include "math/mMath.h"
#include "math/mathIO.h"
#include "core/fileStream.h"
#include "core/bitStream.h"
#include "console/consoleTypes.h"
#include "sceneGraph/sceneGraph.h"
#include "sceneGraph/sceneState.h"
#include "sim/netConnection.h"
#include "terrain/terrRender.h"
#include "terrain/blender.h"
#include "gfx/gfxDevice.h"
#include "gfx/primBuilder.h"
#include "terrain/sky.h"
#include "terrain/terrTexture.h"
#include "gfx/gfxEnums.h"
#include "terrain/blender.h"
#include "renderInstance/renderInstMgr.h"

extern bool gDGLRender;


IMPLEMENT_CO_NETOBJECT_V1(TerrainBlock);

//--------------------------------------
TerrainBlock::TerrainBlock()
{
    squareSize = 8;

    mBlender = NULL;
    lightMap = 0;

    for (S32 i = BlockShift; i >= 0; i--)
        gridMap[i] = NULL;

    heightMap = NULL;
    materialMap = NULL;
    mBaseMaterialMap = NULL;
    mMaterialFileName = NULL;
    mTypeMask = TerrainObjectType | StaticObjectType |
        StaticRenderedObjectType | ShadowCasterObjectType;
    mNetFlags.set(Ghostable | ScopeAlways);
    mCollideEmpty = false;
    mDetailTextureName = NULL;
    //CW - bump mapping stuff
    mBumpTextureName = NULL;
    mBumpScale = 1.0;
    mBumpOffset = 0.01;
    mZeroBumpScale = 8;
    //CW - end bump mapping stuff
    mCRC = 0;
    flagMap = 0;
    mTerrainPrimCount = 0;
    mTerrainVerts = 0;

    if (GFXDevice::devicePresent())
    {
        GFX->registerTexCallback(texManagerCallback, this, mTexCallback);
    }
}


//--------------------------------------
TerrainBlock::~TerrainBlock()
{
    if (GFXDevice::devicePresent())
    {
        GFX->unregisterTexCallback(mTexCallback);
    }

    delete lightMap;
}

//--------------------------------------
void TerrainBlock::texManagerCallback(GFXTexCallbackCode code, void* userData)
{
    TerrainBlock* localBlock = (TerrainBlock*)userData;

    if (code == GFXZombify)
    {
        // Clear the texture cache.
        AllocatedTexture::flushCache();
        localBlock->mBlender->zombify();
    }
    else if (code == GFXResurrect)
    {
        localBlock->mBlender->resurrect();
    }
}

//--------------------------------------
void TerrainBlock::setFile(Resource<TerrainFile> terr)
{
    mFile = terr;
    for (U32 i = 0; i <= BlockShift; i++)
        gridMap[i] = mFile->mGridMap[i];

    mBaseMaterialMap = mFile->mBaseMaterialMap;
    mMaterialFileName = mFile->mMaterialFileName;
    mChunkMap = mFile->mChunkMap;
    materialMap = mFile->mMaterialMap;
    heightMap = mFile->mHeightMap;
    flagMap = mFile->mFlagMap;
}


//--------------------------------------------------------------------------
bool TerrainBlock::save(const char* filename)
{
    return mFile->save(filename);
}


//--------------------------------------
static U16 calcDev(PlaneF& pl, Point3F& pt)
{
    F32 z = (pl.d + pl.x * pt.x + pl.y * pt.y) / -pl.z;
    F32 diff = z - pt.z;
    if (diff < 0)
        diff = -diff;

    if (diff > 0xFFFF)
        return 0xFFFF;
    else
        return U16(diff);
}

static U16 Umax(U16 u1, U16 u2)
{
    return u1 > u2 ? u1 : u2;
}

//------------------------------------------------------------------------------

bool TerrainBlock::unpackEmptySquares()
{
    U32 size = BlockSquareWidth * BlockSquareWidth;

    U32 i;
    for (i = 0; i < size; i++)
        materialMap[i].flags &= ~Material::Empty;

    // walk the pairs
    for (i = 0; i < mEmptySquareRuns.size(); i++)
    {
        U32 offset = mEmptySquareRuns[i] & 0xffff;
        U32 run = U32(mEmptySquareRuns[i]) >> 16;

        //
        for (U32 j = 0; j < run; j++)
        {
            if ((offset + j) >= size)
            {
                Con::errorf(ConsoleLogEntry::General, "TerrainBlock::unpackEmpties: invalid entry.");
                return(false);
            }
            materialMap[offset + j].flags |= Material::Empty;
        }
    }

    rebuildEmptyFlags();
    return(true);
}

void TerrainBlock::packEmptySquares()
{
    AssertFatal(isServerObject(), "TerrainBlock::packEmptySquares: client!");
    mEmptySquareRuns.clear();

    // walk the map
    U32 run = 0;
    U32 offset;

    U32 size = BlockSquareWidth * BlockSquareWidth;
    for (U32 i = 0; i < size; i++)
    {
        if (materialMap[i].flags & Material::Empty)
        {
            if (!run)
                offset = i;
            run++;
        }
        else if (run)
        {
            mEmptySquareRuns.push_back((run << 16) | offset);
            run = 0;

            // cap it
            if (mEmptySquareRuns.size() == MaxEmptyRunPairs)
                break;
        }
    }

    //
    if (run && mEmptySquareRuns.size() != MaxEmptyRunPairs)
        mEmptySquareRuns.push_back((run << 16) | offset);

    if (mEmptySquareRuns.size() == MaxEmptyRunPairs)
        Con::warnf(ConsoleLogEntry::General, "TerrainBlock::packEmptySquares: More than %d run pairs encountered.  Extras will not persist.", MaxEmptyRunPairs);

    //
    mNetFlags |= EmptyMask;
}

//------------------------------------------------------------------------------

void TerrainBlock::rebuildEmptyFlags()
{
    // rebuild entire maps empty flags!
    for (U32 y = 0; y < TerrainBlock::ChunkSquareWidth; y++)
    {
        for (U32 x = 0; x < TerrainBlock::ChunkSquareWidth; x++)
        {
            GridChunk& gc = *(mChunkMap + x + (y << TerrainBlock::ChunkShift));
            gc.emptyFlags = 0;
            U32 sx = x << TerrainBlock::ChunkDownShift;
            U32 sy = y << TerrainBlock::ChunkDownShift;
            for (U32 j = 0; j < 4; j++)
            {
                for (U32 i = 0; i < 4; i++)
                {
                    TerrainBlock::Material* mat = getMaterial(sx + i, sy + j);
                    if (mat->flags & TerrainBlock::Material::Empty)
                        gc.emptyFlags |= (1 << (j * 4 + i));
                }
            }
        }
    }

    for (S32 i = BlockShift; i >= 0; i--)
    {
        S32 squareCount = 1 << (BlockShift - i);
        S32 squareSize = (TerrainBlock::BlockSize) / squareCount;

        for (S32 squareX = 0; squareX < squareCount; squareX++)
        {
            for (S32 squareY = 0; squareY < squareCount; squareY++)
            {
                GridSquare* parent = NULL;
                if (i < BlockShift)
                    parent = findSquare(i + 1, Point2I(squareX * squareSize, squareY * squareSize));
                bool empty = true;

                for (S32 sizeX = 0; empty && sizeX <= squareSize; sizeX++)
                {
                    for (S32 sizeY = 0; empty && sizeY <= squareSize; sizeY++)
                    {
                        S32 x = squareX * squareSize + sizeX;
                        S32 y = squareY * squareSize + sizeY;

                        if (sizeX != squareSize && sizeY != squareSize)
                        {
                            TerrainBlock::Material* mat = getMaterial(x, y);
                            if (!(mat->flags & TerrainBlock::Material::Empty))
                                empty = false;
                        }
                    }
                }
                GridSquare* sq = findSquare(i, Point2I(squareX * squareSize, squareY * squareSize));
                if (empty)
                    sq->flags |= GridSquare::Empty;
                else
                    sq->flags &= ~GridSquare::Empty;
            }
        }
    }
}

//------------------------------------------------------------------------------

void TerrainBlock::setHeight(const Point2I& pos, float height)
{
    // set the height
    U16 ht = floatToFixed(height);
    *((U16*)getHeightAddress(pos.x, pos.y)) = ht;
}


inline void getMinMax(U16& min, U16& max, U16 height)
{
    if (height < min)
        min = height;
    if (height > max)
        max = height;
}

inline void checkSquareMinMax(GridSquare* parent, GridSquare* child)
{
    if (parent->minHeight > child->minHeight)
        parent->minHeight = child->minHeight;
    if (parent->maxHeight < child->maxHeight)
        parent->maxHeight = child->maxHeight;
}

void TerrainBlock::updateGridMaterials(Point2I min, Point2I max)
{
    // ok:
    // build the chunk materials flags for all the chunks in the bounding rect
    // ((min - 1) >> ChunkDownShift) up to ((max + ChunkWidth) >> ChunkDownShift)

    // we have to make sure to cover boundary conditions as as stated above
    // since, for example, x = 0 affects 2 chunks

    for (S32 y = min.y - 1; y < max.y + 1; y++)
    {
        for (S32 x = min.x - 1; x < max.x + 1; x++)
        {
            GridSquare* sq = findSquare(0, Point2I(x, y));
            sq->flags &= (GridSquare::MaterialStart - 1);

            S32 xpl = (x + 1) & TerrainBlock::BlockMask;
            S32 ypl = (y + 1) & TerrainBlock::BlockMask;

            for (U32 i = 0; i < TerrainBlock::MaterialGroups; i++)
            {
                if (mFile->mMaterialAlphaMap[i] != NULL)
                {
                    U32 mapVal = (mFile->mMaterialAlphaMap[i][(y << TerrainBlock::BlockShift) + x] |
                        mFile->mMaterialAlphaMap[i][(ypl << TerrainBlock::BlockShift) + x] |
                        mFile->mMaterialAlphaMap[i][(ypl << TerrainBlock::BlockShift) + xpl] |
                        mFile->mMaterialAlphaMap[i][(y << TerrainBlock::BlockShift) + xpl]);
                    if (mapVal)
                        sq->flags |= (GridSquare::MaterialStart << i);
                }
            }
        }
    }

    for (S32 y = min.y - 2; y < max.y + 2; y += 2)
    {
        for (S32 x = min.x - 2; x < max.x + 2; x += 2)
        {
            GridSquare* sq = findSquare(1, Point2I(x, y));
            GridSquare* s1 = findSquare(0, Point2I(x, y));
            GridSquare* s2 = findSquare(0, Point2I(x + 1, y));
            GridSquare* s3 = findSquare(0, Point2I(x, y + 1));
            GridSquare* s4 = findSquare(0, Point2I(x + 1, y + 1));
            sq->flags |= (s1->flags | s2->flags | s3->flags | s4->flags) & ~(GridSquare::MaterialStart - 1);
        }
    }

    AllocatedTexture::flushCacheRect(RectI(min, max - min));
    mBlender->updateOpacity();
}

void TerrainBlock::updateGrid(Point2I min, Point2I max)
{
    // ok:
    // build the chunk deviance for all the chunks in the bounding rect,
    // ((min - 1) >> ChunkDownShift) up to ((max + ChunkWidth) >> ChunkDownShift)

    // update the chunk deviances for the affected chunks
    // we have to make sure to cover boundary conditions as as stated above
    // since, for example, x = 0 affects 2 chunks

    for (S32 x = (min.x - 1) >> ChunkDownShift; x < (max.x + ChunkSize) >> ChunkDownShift; x++)
    {
        for (S32 y = (min.y - 1) >> ChunkDownShift; y < (max.y + ChunkSize) >> ChunkDownShift; y++)
        {
            S32 px = x;
            S32 py = y;
            if (px < 0)
                px += BlockSize >> ChunkDownShift;
            if (py < 0)
                py += BlockSize >> ChunkDownShift;

            buildChunkDeviance(px, py);
        }
    }

    // ok the chunk deviances are rebuilt... now rebuild the affected area
    // of the grid map:

    // here's how it works:
    // for the current terrain renderer we only care about
    // the minHeight and maxHeight on the GridSquare
    // so we do one pass through, updating minHeight and maxHeight
    // on the level 0 squares, then we loop up the grid map from 1 to
    // the top, expanding the bounding boxes as necessary.
    // this should end up being way, way, way, way faster for the terrain
    // editor

    for (S32 y = min.y - 1; y < max.y + 1; y++)
    {
        for (S32 x = min.x - 1; x < max.x + 1; x++)
        {
            S32 px = x;
            S32 py = y;
            if (px < 0)
                px += BlockSize;
            if (py < 0)
                py += BlockSize;

            GridSquare* sq = findSquare(0, px, py);

            sq->minHeight = 0xFFFF;
            sq->maxHeight = 0;

            getMinMax(sq->minHeight, sq->maxHeight, getHeight(x, y));
            getMinMax(sq->minHeight, sq->maxHeight, getHeight(x + 1, y));
            getMinMax(sq->minHeight, sq->maxHeight, getHeight(x, y + 1));
            getMinMax(sq->minHeight, sq->maxHeight, getHeight(x + 1, y + 1));
        }
    }

    // ok, all the level 0 grid squares are updated:
    // now update all the parent grid squares that need to be updated:

    for (S32 level = 1; level <= TerrainBlock::BlockShift; level++)
    {
        S32 size = 1 << level;
        S32 halfSize = size >> 1;
        for (S32 y = (min.y - 1) >> level; y < (max.y + size) >> level; y++)
        {
            for (S32 x = (min.x - 1) >> level; x < (max.x + size) >> level; x++)
            {
                S32 px = x << level;
                S32 py = y << level;

                GridSquare* square = findSquare(level, px, py);
                square->minHeight = 0xFFFF;
                square->maxHeight = 0;

                checkSquareMinMax(square, findSquare(level - 1, px, py));
                checkSquareMinMax(square, findSquare(level - 1, px + halfSize, py));
                checkSquareMinMax(square, findSquare(level - 1, px, py + halfSize));
                checkSquareMinMax(square, findSquare(level - 1, px + halfSize, py + halfSize));
            }
        }
    }
}


//--------------------------------------
bool TerrainBlock::getHeight(const Point2F& pos, F32* height)
{
    float invSquareSize = 1.0f / (float)squareSize;
    float xp = pos.x * invSquareSize;
    float yp = pos.y * invSquareSize;
    int x = (S32)mFloor(xp);
    int y = (S32)mFloor(yp);
    xp -= (float)x;
    yp -= (float)y;
    x &= BlockMask;
    y &= BlockMask;
    GridSquare* gs = findSquare(0, Point2I(x, y));

    if (gs->flags & GridSquare::Empty)
        return false;

    float zBottomLeft = fixedToFloat(getHeight(x, y));
    float zBottomRight = fixedToFloat(getHeight(x + 1, y));
    float zTopLeft = fixedToFloat(getHeight(x, y + 1));
    float zTopRight = fixedToFloat(getHeight(x + 1, y + 1));

    if (gs->flags & GridSquare::Split45)
    {
        if (xp > yp)
            // bottom half
            *height = zBottomLeft + xp * (zBottomRight - zBottomLeft) + yp * (zTopRight - zBottomRight);
        else
            // top half
            *height = zBottomLeft + xp * (zTopRight - zTopLeft) + yp * (zTopLeft - zBottomLeft);
    }
    else
    {
        if (1.0f - xp > yp)
            // bottom half
            *height = zBottomRight + (1.0f - xp) * (zBottomLeft - zBottomRight) + yp * (zTopLeft - zBottomLeft);
        else
            // top half
            *height = zBottomRight + (1.0f - xp) * (zTopLeft - zTopRight) + yp * (zTopRight - zBottomRight);
    }
    return true;
}

bool TerrainBlock::getNormal(const Point2F& pos, Point3F* normal, bool normalize)
{
    float invSquareSize = 1.0f / (float)squareSize;
    float xp = pos.x * invSquareSize;
    float yp = pos.y * invSquareSize;
    int x = (S32)mFloor(xp);
    int y = (S32)mFloor(yp);
    xp -= (float)x;
    yp -= (float)y;
    x &= BlockMask;
    y &= BlockMask;
    GridSquare* gs = findSquare(0, Point2I(x, y));

    if (gs->flags & GridSquare::Empty)
        return false;

    float zBottomLeft = fixedToFloat(getHeight(x, y));
    float zBottomRight = fixedToFloat(getHeight(x + 1, y));
    float zTopLeft = fixedToFloat(getHeight(x, y + 1));
    float zTopRight = fixedToFloat(getHeight(x + 1, y + 1));

    if (gs->flags & GridSquare::Split45)
    {
        if (xp > yp)
            // bottom half
            normal->set(zBottomLeft - zBottomRight, zBottomRight - zTopRight, squareSize);
        else
            // top half
            normal->set(zTopLeft - zTopRight, zBottomLeft - zTopLeft, squareSize);
    }
    else
    {
        if (1.0f - xp > yp)
            // bottom half
            normal->set(zBottomLeft - zBottomRight, zBottomLeft - zTopLeft, squareSize);
        else
            // top half
            normal->set(zTopLeft - zTopRight, zBottomRight - zTopRight, squareSize);
    }
    if (normalize)
        normal->normalize();
    return true;
}

bool TerrainBlock::getNormalAndHeight(const Point2F& pos, Point3F* normal, F32* height, bool normalize)
{
    float invSquareSize = 1.0f / (float)squareSize;
    float xp = pos.x * invSquareSize;
    float yp = pos.y * invSquareSize;
    int x = (S32)mFloor(xp);
    int y = (S32)mFloor(yp);
    xp -= (float)x;
    yp -= (float)y;
    x &= BlockMask;
    y &= BlockMask;
    GridSquare* gs = findSquare(0, Point2I(x, y));

    if (gs->flags & GridSquare::Empty)
        return false;

    float zBottomLeft = fixedToFloat(getHeight(x, y));
    float zBottomRight = fixedToFloat(getHeight(x + 1, y));
    float zTopLeft = fixedToFloat(getHeight(x, y + 1));
    float zTopRight = fixedToFloat(getHeight(x + 1, y + 1));

    if (gs->flags & GridSquare::Split45)
    {
        if (xp > yp)
        {
            // bottom half
            normal->set(zBottomLeft - zBottomRight, zBottomRight - zTopRight, squareSize);
            *height = zBottomLeft + xp * (zBottomRight - zBottomLeft) + yp * (zTopRight - zBottomRight);
        }
        else
        {
            // top half
            normal->set(zTopLeft - zTopRight, zBottomLeft - zTopLeft, squareSize);
            *height = zBottomLeft + xp * (zTopRight - zTopLeft) + yp * (zTopLeft - zBottomLeft);
        }
    }
    else
    {
        if (1.0f - xp > yp)
        {
            // bottom half
            normal->set(zBottomLeft - zBottomRight, zBottomLeft - zTopLeft, squareSize);
            *height = zBottomRight + (1.0f - xp) * (zBottomLeft - zBottomRight) + yp * (zTopLeft - zBottomLeft);
        }
        else
        {
            // top half
            normal->set(zTopLeft - zTopRight, zBottomRight - zTopRight, squareSize);
            *height = zBottomRight + (1.0f - xp) * (zTopLeft - zTopRight) + yp * (zTopRight - zBottomRight);
        }
    }
    if (normalize)
        normal->normalize();
    return true;
}

//--------------------------------------


//--------------------------------------
void TerrainBlock::setBaseMaterials(S32 argc, const char* argv[])
{
    for (S32 i = 0; i < argc; i++)
        mMaterialFileName[i] = StringTable->insert(argv[i]);
    for (S32 j = argc; j < MaterialGroups; j++)
        mMaterialFileName[j] = NULL;
}

//------------------------------------------------------------------------------

//--------------------------------------

bool TerrainBlock::buildMaterialMap()
{
    AllocatedTexture::flushCache();
    return initMMXBlender();
}

bool TerrainBlock::initMMXBlender()
{
    // DMMNOTE: come back to this
    delete mBlender;
    mBlender = NULL;

    char fileBuf[256];

    U32 validMaterials = 0;
    S32 i;
    for (i = 0; i < MaterialGroups; i++) {
        if (mMaterialFileName[i] && *mMaterialFileName[i])
            validMaterials++;
        else
            break;
    }
    AssertFatal(validMaterials != 0, "Error, must have SOME materials here!");

    // Submit alphamaps
    U8* alphaMaterials[MaterialGroups];
    dMemset(alphaMaterials, 0, sizeof(alphaMaterials));
    for (i = 0; i < validMaterials; i++) {
        if (getMaterialAlphaMap(i) == NULL) {
            AssertFatal(getMaterialAlphaMap(i) != NULL, "Error, need an alpha map here!");
            return false;
        }
        alphaMaterials[i] = getMaterialAlphaMap(i);
    }

    mBlender = new Blender(this); // validMaterials, 5, alphaMaterials);

    // Ok, we have validMaterials set correctly
    bool matsValid = true;
    for (i = 0; i < validMaterials; i++) {
        AssertFatal(mMaterialFileName[i] && *mMaterialFileName[i], "Error, something wacky here");
        StringTableEntry fn = mMaterialFileName[i];

        GFXTexHandle matBitmap(fn, &GFXDefaultStaticDiffuseProfile);
        mBlender->addSourceTexture(i, matBitmap, getMaterialAlphaMap(i));
    }

    // Invalidate the lightmap texture
    lightMapTex = NULL;

    mBlender->updateOpacity();

    // ok, if the material list load failed...
    // if this is a local connection, we'll assume that's ok
    // and just have white textures...
    // otherwise we want to return false.
    return matsValid || bool(mServerObject);

}

void TerrainBlock::getMaterialAlpha(Point2I pos, U8 alphas[MaterialGroups])
{
    pos.x &= BlockMask;
    pos.y &= BlockMask;
    U32 offset = pos.x + pos.y * BlockSize;
    for (S32 i = 0; i < MaterialGroups; i++)
    {
        U8* map = mFile->mMaterialAlphaMap[i];
        if (map)
            alphas[i] = map[offset];
        else
            alphas[i] = 0;
    }
}

void TerrainBlock::setMaterialAlpha(Point2I pos, const U8 alphas[MaterialGroups])
{
    pos.x &= BlockMask;
    pos.y &= BlockMask;
    U32 offset = pos.x + pos.y * BlockSize;
    for (S32 i = 0; i < MaterialGroups; i++)
    {
        U8* map = mFile->mMaterialAlphaMap[i];
        if (map)
            map[offset] = alphas[i];
    }
}

S32 TerrainBlock::getMaterialAlphaIndex(const char* materialName)
{

    for (S32 i = 0; i < MaterialGroups; i++)
        if (mMaterialFileName[i] && *mMaterialFileName[i] && !dStricmp(materialName, mMaterialFileName[i]))
            return i;
    // ok, it wasn't in the list
    // see if we can add it:
    for (S32 i = 0; i < MaterialGroups; i++)
    {
        if (!mMaterialFileName[i] || !*mMaterialFileName[i])
        {
            mMaterialFileName[i] = StringTable->insert(materialName);
            mFile->mMaterialAlphaMap[i] = new U8[TerrainBlock::BlockSize * TerrainBlock::BlockSize];
            dMemset(mFile->mMaterialAlphaMap[i], 0, TerrainBlock::BlockSize * TerrainBlock::BlockSize);
            buildMaterialMap();
            return i;
        }
    }
    return -1;
}

//------------------------------------------------------------------------------

void TerrainBlock::refreshMaterialLists()
{
}

//------------------------------------------------------------------------------

void TerrainBlock::onEditorEnable()
{
    // need to load in the material base material lists
    if (isClientObject())
        refreshMaterialLists();
}

void TerrainBlock::onEditorDisable()
{

}

//------------------------------------------------------------------------------

bool TerrainBlock::onAdd()
{
    if (!Parent::onAdd())
        return false;

    setPosition(Point3F(-squareSize * (BlockSize >> 1), -squareSize * (BlockSize >> 1), 0));

    Resource<TerrainFile> terr = ResourceManager->load(mTerrFileName, true);
    if (!bool(terr))
    {
        if (isClientObject())
            NetConnection::setLastError("You are missing a file needed to play this mission: %s", mTerrFileName);
        return false;
    }

    setFile(terr);

    StringTableEntry fn = mMaterialFileName[0];
    if (!dStrncmp(fn, "terrain.", 8))
        fn += 8;

    char nameBuff[512];
    dStrcpy(nameBuff, mTerrFileName);
    char* p = dStrrchr(nameBuff, '/');
    if (p) p++;
    else p = nameBuff;
    dStrcat(p, fn);

    mObjBox.min.set(-1e8, -1e8, -1e8);
    mObjBox.max.set(1e8, 1e8, 1e8);
    resetWorldBox();
    setRenderTransform(mObjToWorld);

    if (isClientObject())
    {
        if (mCRC != terr.getCRC())
        {
            NetConnection::setLastError("Your terrain file doesn't match the version that is running on the server.");
            return false;
        }

        lightMap = new GBitmap(LightmapSize, LightmapSize, false, GFXFormatR8G8B8);

        if (!buildMaterialMap())
            return false;

        mDynLightTexture = GFXTexHandle("common/lighting/lightFalloffMono", &GFXDefaultStaticDiffuseProfile);

    }
    else
        mCRC = terr.getCRC();

    addToScene();

    if (!unpackEmptySquares())
        return(false);

    return true;
}

//--------------------------------------
void TerrainBlock::onRemove()
{
    delete mBlender;
    mBlender = NULL;

    removeFromScene();

    AllocatedTexture::flushCache();
    Parent::onRemove();
}


//--------------------------------------
//--------------------------------------------------------------------------
bool TerrainBlock::prepRenderImage(SceneState* state, const U32 stateKey,
    const U32 /*startZone*/, const bool /*modifyBaseState*/)
{
    if (isLastState(state, stateKey))
        return false;
    setLastState(state, stateKey);

    // This should be sufficient for most objects that don't manage zones, and
    //  don't need to return a specialized RenderImage...
    bool render = false;
    if (state->isTerrainOverridden() == false)
        render = state->isObjectRendered(this);
    else
        render = true;

    if (render == true)
    {
        RenderInst* ri = gRenderInstManager.allocInst();
        ri->obj = this;
        ri->state = state;
        ri->type = RenderInstManager::RIT_Object;
        gRenderInstManager.addInst(ri);
    }

    return false;
}

void TerrainBlock::buildChunkDeviance(S32 x, S32 y)
{
    mFile->buildChunkDeviance(x, y);
}

void TerrainBlock::buildGridMap()
{
    mFile->buildGridMap();
}

//------------------------------------------------------------------------------
void TerrainBlock::setTransform(const MatrixF& mat)
{
    Parent::setTransform(mat);

    // Since the terrain is a static object, it's render transform changes 1 to 1
    //  with it's collision transform
    setRenderTransform(mat);
}


void TerrainBlock::renderObject(SceneState* state, RenderInst* ri)
{
    MatrixF proj = GFX->getProjectionMatrix();
    MatrixF cleanProj = proj;
    RectI viewport = GFX->getViewport();

    if (state->isTerrainOverridden())
        state->setupBaseProjection();
    else
        state->setupObjectProjection(this);


    if (getCurrentClientSceneGraph()->isReflectPass())
    {
        GFX->setCullMode(GFXCullCW);
    }
    else
    {
        GFX->setCullMode(GFXCullCCW);
    }


    GFX->pushWorldMatrix();
    GFX->multWorld(mRenderObjToWorld);

    // Set up scenegraph data
    SceneGraphData sgData;

    // Fogging...
    sgData.useFog = true;
    sgData.fogTex = getCurrentClientSceneGraph()->getFogTexture();
    sgData.fogHeightOffset = getCurrentClientSceneGraph()->getFogHeightOffset();
    sgData.fogInvHeightRange = getCurrentClientSceneGraph()->getFogInvHeightRange();
    sgData.visDist = getCurrentClientSceneGraph()->getVisibleDistanceMod();

    // Set up world transform
    MatrixF world = GFX->getWorldMatrix();
    proj.mul(world);
    proj.transpose();
    GFX->setVertexShaderConstF(0, (float*)&proj, 4);

    // Store object and camera transform data
    sgData.objTrans = getRenderTransform();
    sgData.camPos = state->getCameraPosition();


    getCurrentClientSceneGraph()->getLightManager()->sgSetupLights(this, state->getCameraPosition(),
        Point3F(0, 0, 0), state->getVisibleDistance(), MaxVisibleLights);
    //gClientSceneGraph->getLightManager()->sgGetBestLights();


    // Actually render the thing.
    MatInstance tMat("TerrainMaterial");
    TerrainRender::renderBlock(this, state, &tMat, sgData);


    getCurrentClientSceneGraph()->getLightManager()->sgResetLights();


    // Clean up after...
    GFX->setCullMode(GFXCullNone);
    GFX->setBaseRenderState();

    GFX->popWorldMatrix();

    GFX->setViewport(viewport);
    GFX->setProjectionMatrix(cleanProj);
}

//--------------------------------------
void TerrainBlock::initPersistFields()
{
    Parent::initPersistFields();

    addGroup("Media");
    addField("detailTexture", TypeFilename, Offset(mDetailTextureName, TerrainBlock));
    addField("terrainFile", TypeFilename, Offset(mTerrFileName, TerrainBlock));
    //CW - bump mapping stuff
    addField("bumpTexture", TypeFilename, Offset(mBumpTextureName, TerrainBlock));
    //CW - end bump mapping stuff
    endGroup("Media");

    addGroup("Misc");
    addField("squareSize", TypeS32, Offset(squareSize, TerrainBlock));
    addField("emptySquares", TypeS32Vector, Offset(mEmptySquareRuns, TerrainBlock));
    //CW - bump mapping stuff
    addField("bumpScale", TypeF32, Offset(mBumpScale, TerrainBlock));
    addField("bumpOffset", TypeF32, Offset(mBumpOffset, TerrainBlock));
    addField("zeroBumpScale", TypeS32, Offset(mZeroBumpScale, TerrainBlock));
    //CW - end bump mapping stuff
    endGroup("Misc");

    removeField("position");
}

//--------------------------------------
U32 TerrainBlock::packUpdate(NetConnection*, U32 mask, BitStream* stream)
{
    if (stream->writeFlag(mask & InitMask))
    {
        stream->write(mCRC);
        stream->writeString(mTerrFileName);
        stream->writeString(mDetailTextureName);
        //CW - bump mapping stuff
        stream->writeString(mBumpTextureName);
        //don't have negative bump scale and don't have one too small
        if (mBumpScale <= 0)
        {
            Con::errorf("Bump scale cannot be less than or equal to 0.  Setting to 0.0001.");
            mBumpScale = 0.0001;
        }
        stream->write(mBumpScale);
        stream->write(mBumpOffset);
        stream->write(mZeroBumpScale);
        //CW - end bump mapping stuff
        stream->write(squareSize);

        // write out the empty rle vector
        stream->write(mEmptySquareRuns.size());
        for (U32 i = 0; i < mEmptySquareRuns.size(); i++)
            stream->write(mEmptySquareRuns[i]);
    }
    else // normal update
    {
        if (stream->writeFlag(mask & EmptyMask))
        {
            // write out the empty rle vector
            stream->write(mEmptySquareRuns.size());
            for (U32 i = 0; i < mEmptySquareRuns.size(); i++)
                stream->write(mEmptySquareRuns[i]);
        }
    }

    return 0;
}

void TerrainBlock::unpackUpdate(NetConnection*, BitStream* stream)
{
    if (stream->readFlag())  // init
    {
        stream->read(&mCRC);
        mTerrFileName = stream->readSTString();
        mDetailTextureName = stream->readSTString();
        //CW - bump mapping stuff
        mBumpTextureName = stream->readSTString();
        stream->read(&mBumpScale);
        stream->read(&mBumpOffset);
        stream->read(&mZeroBumpScale);
        //CW - end bump mapping stuff
        stream->read(&squareSize);

        // read in the empty rle
        U32 size;
        stream->read(&size);
        mEmptySquareRuns.setSize(size);
        for (U32 i = 0; i < size; i++)
            stream->read(&mEmptySquareRuns[i]);
    }
    else // normal update
    {
        if (stream->readFlag())  // empty
        {
            // read in the empty rle
            U32 size;
            stream->read(&size);
            mEmptySquareRuns.setSize(size);
            for (U32 i = 0; i < size; i++)
                stream->read(&mEmptySquareRuns[i]);

            //
            if (materialMap)
                unpackEmptySquares();
        }
    }
}

//--------------------------------------
ConsoleFunction(makeTestTerrain, void, 2, 10, "(string fileName, ...) - makes a test terrain file - arguments after the fileName are the names of the initial terrain materials.")
{
    TerrainFile* file = new TerrainFile;
    S32 nMaterialTypes;
    argc -= 2;

    // Load materials
    if (argc > 1)
    {
        nMaterialTypes = argc;
        for (S32 i = 0; i < TerrainBlock::MaterialGroups && i < argc; i++)
        {
            char material[256];
            char* ext;
            dStrcpy(material, argv[i + 2]);
            ext = dStrrchr(material, '.');
            if (ext)
                *ext = 0;
            file->mMaterialFileName[i] = StringTable->insert(material);
        }
    }
    else
    {
        nMaterialTypes = 1;
        file->mMaterialFileName[0] = StringTable->insert("Default");
    }

    // create circular cone in the middle of the map:
    S32 i, j;
    for (i = 0; i < TerrainBlock::BlockSize; i++)
    {
        for (j = 0; j < TerrainBlock::BlockSize; j++)
        {
            S32 x = i & 0x7f;
            S32 y = j & 0x7f;

            F32 dist = mSqrt((64 - x) * (64 - x) + (64 - y) * (64 - y));
            dist /= 64.0f;

            if (dist > 1)
                dist = 1;

            U32 offset = i + (j << TerrainBlock::BlockShift);
            file->mHeightMap[offset] = (U16)((1 - dist) * (1 - dist) * 20000);
            file->mBaseMaterialMap[offset] = 0;
        }
    }

    //
    char filename[256];
    dStrcpy(filename, argv[1]);
    char* ext = dStrrchr(filename, '.');
    if (!ext || dStricmp(ext, ".ter") != 0)
        dStrcat(filename, ".ter");
    file->save(filename);
    delete file;
}


//--------------------------------------
ConsoleMethod(TerrainBlock, save, bool, 3, 3, "(string fileName) - saves the terrain block's terrain file to the specified file name.")
{
    char filename[256];
    dStrcpy(filename, argv[2]);
    char* ext = dStrrchr(filename, '.');
    if (!ext || dStricmp(ext, ".ter") != 0)
        dStrcat(filename, ".ter");
    return static_cast<TerrainBlock*>(object)->save(filename);
}

ConsoleFunction(getTerrainHeight, F32, 2, 2, "(Point2I pos) - gets the terrain height at the specified position.")
{
    Point2F pos;
    F32 height = 0.0f;
    dSscanf(argv[1], "%f %f", &pos.x, &pos.y);
    TerrainBlock* terrain = dynamic_cast<TerrainBlock*>(Sim::findObject("Terrain"));
    if (terrain)
        if (terrain->isServerObject())
        {
            Point3F offset;
            terrain->getTransform().getColumn(3, &offset);
            pos -= Point2F(offset.x, offset.y);
            terrain->getHeight(pos, &height);
        }
    return height;
}

ConsoleMethod(TerrainBlock, setTextureScript, void, 3, 3, "(string script) - sets the texture script associated with this terrain file.")
{
    TerrainBlock* terr = (TerrainBlock*)object;
    terr->getFile()->setTextureScript(argv[2]);
}

ConsoleMethod(TerrainBlock, setHeightfieldScript, void, 3, 3, "(string script) - sets the heightfield script associated with this terrain file.")
{
    TerrainBlock* terr = (TerrainBlock*)object;
    terr->getFile()->setHeightfieldScript(argv[2]);
}

ConsoleMethod(TerrainBlock, getTextureScript, const char*, 2, 2, "() - gets the texture script associated with the terrain file.")
{
    TerrainBlock* terr = (TerrainBlock*)object;
    return terr->getFile()->getTextureScript();
}

ConsoleMethod(TerrainBlock, getHeightfieldScript, const char*, 2, 2, "() - gets the heightfield script associated with the terrain file.")
{
    TerrainBlock* terr = (TerrainBlock*)object;
    return terr->getFile()->getHeightfieldScript();
}


void TerrainBlock::flushCache()
{
    AllocatedTexture::flushCache();
}

//--------------------------------------
TerrainFile::TerrainFile()
{
    for (S32 i = 0; i < TerrainBlock::MaterialGroups; i++)
    {
        mMaterialFileName[i] = NULL;
        mMaterialAlphaMap[i] = NULL;
    }
    mTextureScript = 0;
    mHeightfieldScript = 0;
}

TerrainFile::~TerrainFile()
{
    for (S32 i = 0; i < TerrainBlock::MaterialGroups; i++) {
        delete[] mMaterialAlphaMap[i];
        mMaterialAlphaMap[i] = NULL;
    }
    dFree(mTextureScript);
    dFree(mHeightfieldScript);
}

void TerrainFile::setTextureScript(const char* script)
{
    dFree(mTextureScript);
    mTextureScript = dStrdup(script);
}

void TerrainFile::setHeightfieldScript(const char* script)
{
    dFree(mHeightfieldScript);
    mHeightfieldScript = dStrdup(script);
}

const char* TerrainFile::getTextureScript()
{
    return mTextureScript ? mTextureScript : "";
}

const char* TerrainFile::getHeightfieldScript()
{
    return mHeightfieldScript ? mHeightfieldScript : "";
}

//--------------------------------------
bool TerrainFile::save(const char* filename)
{
    Stream* writeFile;
    if (!ResourceManager->openFileForWrite(writeFile, filename))
        return false;

    // write the VERSION and HeightField
    writeFile->write((U8)FILE_VERSION);
    for (S32 i = 0; i < (TerrainBlock::BlockSize * TerrainBlock::BlockSize); i++)
        writeFile->write(mHeightMap[i]);

    // write the material group map, after merging the flags...
    TerrainBlock::Material* materialMap = (TerrainBlock::Material*)mMaterialMap;
    for (S32 j = 0; j < (TerrainBlock::BlockSize * TerrainBlock::BlockSize); j++)
    {
        U8 val = mBaseMaterialMap[j];
        val |= materialMap[j].flags & TerrainBlock::Material::PersistMask;
        writeFile->write(val);
    }

    // write the MaterialList Info
    S32 k;
    for (k = 0; k < TerrainBlock::MaterialGroups; k++)
    {
        // ok, only write out the material string if there is a non-zero
        // alpha material:
        if (mMaterialFileName[k] && mMaterialFileName[k][0])
        {
            S32 n;
            for (n = 0; n < TerrainBlock::BlockSize * TerrainBlock::BlockSize; n++)
                if (mMaterialAlphaMap[k][n])
                    break;
            if (n == TerrainBlock::BlockSize * TerrainBlock::BlockSize)
                mMaterialFileName[k] = 0;
        }
        writeFile->writeString(mMaterialFileName[k]);
    }
    for (k = 0; k < TerrainBlock::MaterialGroups; k++) {
        if (mMaterialFileName[k] && mMaterialFileName[k][0]) {
            AssertFatal(mMaterialAlphaMap[k] != NULL, "Error, must have a material map here!");
            writeFile->write(TerrainBlock::BlockSize * TerrainBlock::BlockSize, mMaterialAlphaMap[k]);
        }
    }
    if (mTextureScript)
    {
        U32 len = dStrlen(mTextureScript);
        writeFile->write(len);
        writeFile->write(len, mTextureScript);
    }
    else
        writeFile->write(U32(0));

    if (mHeightfieldScript)
    {
        U32 len = dStrlen(mHeightfieldScript);
        writeFile->write(len);
        writeFile->write(len, mHeightfieldScript);
    }
    else
        writeFile->write(U32(0));

    bool result = (writeFile->getStatus() == FileStream::Ok);
    delete writeFile;

    return result;
}

//--------------------------------------

void TerrainFile::heightDevLine(U32 p1x, U32 p1y, U32 p2x, U32 p2y, U32 pmx, U32 pmy, U16* devPtr)
{
    S32 h1 = getHeight(p1x, p1y);
    S32 h2 = getHeight(p2x, p2y);
    S32 hm = getHeight(pmx, pmy);
    S32 dev = ((h2 + h1) >> 1) - hm;
    if (dev < 0)
        dev = -dev;
    if (dev > *devPtr)
        *devPtr = dev;
}

void TerrainFile::buildChunkDeviance(S32 x, S32 y)
{
    GridChunk& gc = *(mChunkMap + x + (y << TerrainBlock::ChunkShift));
    gc.emptyFlags = 0;
    U32 sx = x << TerrainBlock::ChunkDownShift;
    U32 sy = y << TerrainBlock::ChunkDownShift;

    gc.heightDeviance[0] = 0;

    heightDevLine(sx, sy, sx + 2, sy, sx + 1, sy, &gc.heightDeviance[0]);
    heightDevLine(sx + 2, sy, sx + 4, sy, sx + 3, sy, &gc.heightDeviance[0]);

    heightDevLine(sx, sy + 2, sx + 2, sy + 2, sx + 1, sy + 2, &gc.heightDeviance[0]);
    heightDevLine(sx + 2, sy + 2, sx + 4, sy + 2, sx + 3, sy + 2, &gc.heightDeviance[0]);

    heightDevLine(sx, sy + 4, sx + 2, sy + 4, sx + 1, sy + 4, &gc.heightDeviance[0]);
    heightDevLine(sx + 2, sy + 4, sx + 4, sy + 4, sx + 3, sy + 4, &gc.heightDeviance[0]);

    heightDevLine(sx, sy, sx, sy + 2, sx, sy + 1, &gc.heightDeviance[0]);
    heightDevLine(sx + 2, sy, sx + 2, sy + 2, sx + 2, sy + 1, &gc.heightDeviance[0]);
    heightDevLine(sx + 4, sy, sx + 4, sy + 2, sx + 4, sy + 1, &gc.heightDeviance[0]);

    heightDevLine(sx, sy + 2, sx, sy + 4, sx, sy + 3, &gc.heightDeviance[0]);
    heightDevLine(sx + 2, sy + 2, sx + 2, sy + 4, sx + 2, sy + 3, &gc.heightDeviance[0]);
    heightDevLine(sx + 4, sy + 2, sx + 4, sy + 4, sx + 4, sy + 3, &gc.heightDeviance[0]);

    gc.heightDeviance[1] = gc.heightDeviance[0];

    heightDevLine(sx, sy, sx + 2, sy + 2, sx + 1, sy + 1, &gc.heightDeviance[1]);
    heightDevLine(sx + 2, sy, sx, sy + 2, sx + 1, sy + 1, &gc.heightDeviance[1]);

    heightDevLine(sx + 2, sy, sx + 4, sy + 2, sx + 3, sy + 1, &gc.heightDeviance[1]);
    heightDevLine(sx + 2, sy + 2, sx + 4, sy, sx + 3, sy + 1, &gc.heightDeviance[1]);

    heightDevLine(sx, sy + 2, sx + 2, sy + 4, sx + 1, sy + 3, &gc.heightDeviance[1]);
    heightDevLine(sx + 2, sy + 4, sx, sy + 2, sx + 1, sy + 3, &gc.heightDeviance[1]);

    heightDevLine(sx + 2, sy + 2, sx + 4, sy + 4, sx + 3, sy + 3, &gc.heightDeviance[1]);
    heightDevLine(sx + 2, sy + 4, sx + 4, sy + 2, sx + 3, sy + 3, &gc.heightDeviance[1]);

    gc.heightDeviance[2] = gc.heightDeviance[1];

    heightDevLine(sx, sy, sx + 4, sy, sx + 2, sy, &gc.heightDeviance[2]);
    heightDevLine(sx, sy + 4, sx + 4, sy + 4, sx + 2, sy + 4, &gc.heightDeviance[2]);
    heightDevLine(sx, sy, sx, sy + 4, sx, sy + 2, &gc.heightDeviance[2]);
    heightDevLine(sx + 4, sy, sx + 4, sy + 4, sx + 4, sy + 2, &gc.heightDeviance[2]);

    for (U32 j = 0; j < 4; j++)
    {
        for (U32 i = 0; i < 4; i++)
        {
            TerrainBlock::Material* mat = getMaterial(sx + i, sy + j);
            if (mat->flags & TerrainBlock::Material::Empty)
                gc.emptyFlags |= (1 << (j * 4 + i));
        }
    }
}

void TerrainFile::buildGridMap()
{
    S32 y;
    for (y = 0; y < TerrainBlock::ChunkSquareWidth; y++)
        for (U32 x = 0; x < TerrainBlock::ChunkSquareWidth; x++)
            buildChunkDeviance(x, y);

    GridSquare* gs = mGridMapBase;
    S32 i;
    for (i = TerrainBlock::BlockShift; i >= 0; i--)
    {
        mGridMap[i] = gs;
        gs += 1 << (2 * (TerrainBlock::BlockShift - i));
    }
    for (i = TerrainBlock::BlockShift; i >= 0; i--)
    {
        S32 squareCount = 1 << (TerrainBlock::BlockShift - i);
        S32 squareSize = (TerrainBlock::BlockSize) / squareCount;

        for (S32 squareX = 0; squareX < squareCount; squareX++)
        {
            for (S32 squareY = 0; squareY < squareCount; squareY++)
            {
                U16 min = 0xFFFF;
                U16 max = 0;
                U16 mindev45 = 0;
                U16 mindev135 = 0;

                Point3F p1, p2, p3, p4;

                // determine max error for both possible splits.
                PlaneF pl1, pl2, pl3, pl4;

                p1.set(0, 0, getHeight(squareX * squareSize, squareY * squareSize));
                p2.set(0, squareSize, getHeight(squareX * squareSize, squareY * squareSize + squareSize));
                p3.set(squareSize, squareSize, getHeight(squareX * squareSize + squareSize, squareY * squareSize + squareSize));
                p4.set(squareSize, 0, getHeight(squareX * squareSize + squareSize, squareY * squareSize));

                // pl1, pl2 = split45, pl3, pl4 = split135
                pl1.set(p1, p2, p3);
                pl2.set(p1, p3, p4);
                pl3.set(p1, p2, p4);
                pl4.set(p2, p3, p4);
                bool parentSplit45 = false;
                GridSquare* parent = NULL;
                if (i < TerrainBlock::BlockShift)
                {
                    GridSquare* parent;
                    parent = findSquare(i + 1, Point2I(squareX * squareSize, squareY * squareSize));
                    parentSplit45 = parent->flags & GridSquare::Split45;
                }
                bool empty = true;
                bool hasEmpty = false;

                for (S32 sizeX = 0; sizeX <= squareSize; sizeX++)
                {
                    for (S32 sizeY = 0; sizeY <= squareSize; sizeY++)
                    {
                        S32 x = squareX * squareSize + sizeX;
                        S32 y = squareY * squareSize + sizeY;

                        if (sizeX != squareSize && sizeY != squareSize)
                        {
                            TerrainBlock::Material* mat = getMaterial(x, y);
                            if (!(mat->flags & TerrainBlock::Material::Empty))
                                empty = false;
                            else
                                hasEmpty = true;
                        }
                        U16 ht = getHeight(x, y);

                        if (ht < min)
                            min = ht;
                        if (ht > max)
                            max = ht;
                        Point3F pt(sizeX, sizeY, ht);
                        U16 dev;

                        if (sizeX < sizeY)
                            dev = calcDev(pl1, pt);
                        else if (sizeX > sizeY)
                            dev = calcDev(pl2, pt);
                        else
                            dev = Umax(calcDev(pl1, pt), calcDev(pl2, pt));

                        if (dev > mindev45)
                            mindev45 = dev;

                        if (sizeX + sizeY < squareSize)
                            dev = calcDev(pl3, pt);
                        else if (sizeX + sizeY > squareSize)
                            dev = calcDev(pl4, pt);
                        else
                            dev = Umax(calcDev(pl3, pt), calcDev(pl4, pt));

                        if (dev > mindev135)
                            mindev135 = dev;
                    }
                }
                GridSquare* sq = findSquare(i, Point2I(squareX * squareSize, squareY * squareSize));
                sq->minHeight = min;
                sq->maxHeight = max;

                sq->flags = empty ? GridSquare::Empty : 0;
                if (hasEmpty)
                    sq->flags |= GridSquare::HasEmpty;

                bool shouldSplit45 = ((squareX ^ squareY) & 1) == 0;
                bool split45;

                //split45 = shouldSplit45;
                if (i == 0)
                    split45 = shouldSplit45;
                else if (i < 4 && shouldSplit45 == parentSplit45)
                    split45 = shouldSplit45;
                else
                    split45 = mindev45 < mindev135;

                //split45 = shouldSplit45;
                if (split45)
                {
                    sq->flags |= GridSquare::Split45;
                    sq->heightDeviance = mindev45;
                }
                else
                    sq->heightDeviance = mindev135;
                if (parent)
                    if (parent->heightDeviance < sq->heightDeviance)
                        parent->heightDeviance = sq->heightDeviance;
            }
        }
    }
    for (y = 0; y < TerrainBlock::BlockSize; y++)
    {
        for (S32 x = 0; x < TerrainBlock::BlockSize; x++)
        {
            GridSquare* sq = findSquare(0, Point2I(x, y));
            S32 xpl = (x + 1) & TerrainBlock::BlockMask;
            S32 ypl = (y + 1) & TerrainBlock::BlockMask;
            for (U32 i = 0; i < TerrainBlock::MaterialGroups; i++)
            {
                if (mMaterialAlphaMap[i] != NULL) {
                    U32 mapVal = (mMaterialAlphaMap[i][(y << TerrainBlock::BlockShift) + x] +
                        mMaterialAlphaMap[i][(ypl << TerrainBlock::BlockShift) + x] +
                        mMaterialAlphaMap[i][(ypl << TerrainBlock::BlockShift) + xpl] +
                        mMaterialAlphaMap[i][(y << TerrainBlock::BlockShift) + xpl]);
                    if (mapVal)
                        sq->flags |= (GridSquare::MaterialStart << i);
                }
            }
        }
    }
    for (y = 0; y < TerrainBlock::BlockSize; y += 2)
    {
        for (S32 x = 0; x < TerrainBlock::BlockSize; x += 2)
        {
            GridSquare* sq = findSquare(1, Point2I(x, y));
            GridSquare* s1 = findSquare(0, Point2I(x, y));
            GridSquare* s2 = findSquare(0, Point2I(x + 1, y));
            GridSquare* s3 = findSquare(0, Point2I(x, y + 1));
            GridSquare* s4 = findSquare(0, Point2I(x + 1, y + 1));
            sq->flags |= (s1->flags | s2->flags | s3->flags | s4->flags) & ~(GridSquare::MaterialStart - 1);
        }
    }
    GridSquare* s = findSquare(1, Point2I(0, 0));
    U16* dflags = mFlagMap;
    U16* eflags = mFlagMap + TerrainBlock::FlagMapWidth * TerrainBlock::FlagMapWidth;

    for (; dflags != eflags; s++, dflags++)
        *dflags = s->flags;

}

//--------------------------------------
ResourceInstance* constructTerrainFile(Stream& stream)
{
    U8 version;
    stream.read(&version);
    if (version > TerrainFile::FILE_VERSION)
        return NULL;

    if (version != TerrainFile::FILE_VERSION) {
        Con::errorf(" ****************************************************");
        Con::errorf(" ****************************************************");
        Con::errorf(" ****************************************************");
        Con::errorf(" PLEASE RESAVE THE TERRAIN FILE FOR THIS MISSION!  THANKS!");
        Con::errorf(" ****************************************************");
        Con::errorf(" ****************************************************");
        Con::errorf(" ****************************************************");
    }

    TerrainFile* ret = new TerrainFile;
    // read the HeightField
    for (S32 i = 0; i < (TerrainBlock::BlockSize * TerrainBlock::BlockSize); i++)
        stream.read(&ret->mHeightMap[i]);

    // read the material group map and flags...
    dMemset(ret->mMaterialMap, 0, sizeof(ret->mMaterialMap));
    TerrainBlock::Material* materialMap = (TerrainBlock::Material*)ret->mMaterialMap;

    AssertFatal(!(TerrainBlock::Material::PersistMask & TerrainFile::MATERIAL_GROUP_MASK),
        "Doh! We have flag clobberage...");

    for (S32 j = 0; j < (TerrainBlock::BlockSize * TerrainBlock::BlockSize); j++)
    {
        U8 val;
        stream.read(&val);

        //
        ret->mBaseMaterialMap[j] = val & TerrainFile::MATERIAL_GROUP_MASK;
        materialMap[j].flags = val & TerrainBlock::Material::PersistMask;
    }

    // read the MaterialList Info
    S32 k, maxMaterials = TerrainBlock::MaterialGroups;
    for (k = 0; k < maxMaterials;)
    {
        ret->mMaterialFileName[k] = stream.readSTString(true);
        if (ret->mMaterialFileName[k] && ret->mMaterialFileName[k][0])
            k++;
        else
            maxMaterials--;
    }
    for (; k < TerrainBlock::MaterialGroups; k++)
        ret->mMaterialFileName[k] = NULL;


    if (version == 1)
    {
        for (S32 j = 0; j < (TerrainBlock::BlockSize * TerrainBlock::BlockSize); j++) {
            if (ret->mMaterialAlphaMap[ret->mBaseMaterialMap[j]] == NULL) {
                ret->mMaterialAlphaMap[ret->mBaseMaterialMap[j]] = new U8[TerrainBlock::BlockSize * TerrainBlock::BlockSize];
                dMemset(ret->mMaterialAlphaMap[ret->mBaseMaterialMap[j]], 0, TerrainBlock::BlockSize * TerrainBlock::BlockSize);
            }

            ret->mMaterialAlphaMap[ret->mBaseMaterialMap[j]][j] = 255;
        }
    }
    else
    {
        for (S32 k = 0; k < TerrainBlock::MaterialGroups; k++) {
            if (ret->mMaterialFileName[k] && ret->mMaterialFileName[k][0]) {
                AssertFatal(ret->mMaterialAlphaMap[k] == NULL, "Bad assumption.  There should be no alpha map at this point...");
                ret->mMaterialAlphaMap[k] = new U8[TerrainBlock::BlockSize * TerrainBlock::BlockSize];
                stream.read(TerrainBlock::BlockSize * TerrainBlock::BlockSize, ret->mMaterialAlphaMap[k]);
            }
        }
    }
    if (version >= 3)
    {
        U32 len;
        stream.read(&len);
        ret->mTextureScript = (char*)dMalloc(len + 1);
        stream.read(len, ret->mTextureScript);
        ret->mTextureScript[len] = 0;

        stream.read(&len);
        ret->mHeightfieldScript = (char*)dMalloc(len + 1);
        stream.read(len, ret->mHeightfieldScript);
        ret->mHeightfieldScript[len] = 0;
    }
    else
    {
        ret->mTextureScript = 0;
        ret->mHeightfieldScript = 0;
    }

    ret->buildGridMap();
    return ret;
}

void TerrainBlock::setBaseMaterial(U32 /*x*/, U32 /*y*/, U8 /*matGroup*/)
{

}

