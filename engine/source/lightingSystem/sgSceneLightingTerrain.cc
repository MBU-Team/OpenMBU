//-----------------------------------------------
// Synapse Gaming - Lighting System
// Copyright � Synapse Gaming 2003
// Written by John Kabus
//-----------------------------------------------
#ifdef TORQUE_TERRAIN
#include "editor/editTSCtrl.h"
#include "editor/worldEditor.h"
#include "game/shadow.h"
#include "game/vehicles/wheeledVehicle.h"
#include "game/gameConnection.h"
#include "sceneGraph/sceneGraph.h"
#include "terrain/terrRender.h"
#include "game/shapeBase.h"
#include "gui/core/guiCanvas.h"
#include "ts/tsShape.h"
#include "ts/tsShapeInstance.h"
#include "game/staticShape.h"
#include "game/tsStatic.h"
#include "collision/concretePolyList.h"
#include "lightingSystem/sgSceneLighting.h"
#include "lightingSystem/sgLightMap.h"
#include "lightingSystem/sgSceneLightingGlobals.h"

#define TERRAIN_OVERRANGE 2.0f

extern SceneLighting* gLighting;


/// adds the ability to bake point lights into terrain light maps.
void SceneLighting::TerrainProxy::sgAddUniversalPoint(LightInfo* light)
{
    // add the light...
    sgLights.push_back(light);
}

void SceneLighting::TerrainProxy::sgLightUniversalPoint(LightInfo* light, TerrainBlock* terrain)
{
    // only continue on the first light...
    if (sgLights.first() != light)
        return;

    // process the lighting...
    const F32 length = (terrain->getSquareSize() * TerrainBlock::BlockSize);
    const F32 halflength = 0 - (length * 0.5);

    // texel world space size...
    const F32 offset = length / ((F32)TerrainBlock::LightmapSize);

    sgTerrainLightMap* lightmap = new sgTerrainLightMap(TerrainBlock::LightmapSize, TerrainBlock::LightmapSize, terrain);
    lightmap->sgWorldPosition = Point3F(halflength, halflength, 0);
    lightmap->sgLightMapSVector = Point3F(offset, 0, 0);
    lightmap->sgLightMapTVector = Point3F(0, offset, 0);

    for (U32 i = 0; i < sgLights.size(); i++)
        lightmap->sgCalculateLighting(sgLights[i]);

    lightmap->sgMergeLighting(sgBakedLightmap);

    delete lightmap;
}

SceneLighting::TerrainProxy::TerrainProxy(SceneObject* obj) :
    Parent(obj)
{
    mLightmap = 0;

    sgBakedLightmap = 0;
}

SceneLighting::TerrainProxy::~TerrainProxy()
{
    delete[] mLightmap;

    if (sgBakedLightmap)
        delete[] sgBakedLightmap;
}

//-------------------------------------------------------------------------------
void SceneLighting::TerrainProxy::init()
{
    mLightmap = new ColorF[TerrainBlock::LightmapSize * TerrainBlock::LightmapSize];
    dMemset(mLightmap, 0, TerrainBlock::LightmapSize * TerrainBlock::LightmapSize * sizeof(ColorF));
    mShadowMask.setSize(TerrainBlock::BlockSize * TerrainBlock::BlockSize);

    sgBakedLightmap = new ColorF[TerrainBlock::LightmapSize * TerrainBlock::LightmapSize];
    dMemset(sgBakedLightmap, 0, TerrainBlock::LightmapSize * TerrainBlock::LightmapSize * sizeof(ColorF));
}

/// reroutes TerrainProxy::preLight for point light and TSStatic support.
bool SceneLighting::TerrainProxy::preLight(LightInfo* light)
{
    if (!bool(mObj))
        return(false);

    if ((light->mType != LightInfo::Vector) &&
        (light->mType != LightInfo::SGStaticPoint) &&
        (light->mType != LightInfo::SGStaticSpot))
        return(false);

    if ((light->mType == LightInfo::SGStaticPoint) || (light->mType == LightInfo::SGStaticSpot))
        sgAddUniversalPoint(light);

    mShadowMask.clear();
    return(true);
}

inline ColorF getValue(S32 row, S32 column, ColorF* lmap)
{
    while (row < 0)
        row += TerrainBlock::LightmapSize;
    row = row % TerrainBlock::LightmapSize;

    while (column < 0)
        column += TerrainBlock::LightmapSize;
    column = column % TerrainBlock::LightmapSize;

    U32 offset = row * TerrainBlock::LightmapSize + column;

    return lmap[offset];
}

void sgProcessLightMap(TerrainBlock* terrain, ColorF* mLightmap, ColorF* sgBakedLightmap)
{
    ColorF color;
    if (terrain->lightMap) delete terrain->lightMap;
    terrain->lightMap = new GBitmap(TerrainBlock::LightmapSize, TerrainBlock::LightmapSize, 0, GFXFormatR8G8B8);

    // Blur...
    F32 kernel[3][3] = { 1, 2, 1, 2, 4 - 1, 2, 1, 2, 1 };
    F32 modifier = 1;
    F32 divisor = 0;
    for (U32 i = 0; i < 3; i++)
    {
        for (U32 j = 0; j < 3; j++)
        {
            if (i == 1 && j == 1)
                kernel[i][j] = 1 + kernel[i][j] * modifier;
            else
                kernel[i][j] = kernel[i][j] * modifier;
            divisor += kernel[i][j];
        }
    }

    F32 edgeDivisor = divisor - (kernel[0][0] + kernel[0][1] + kernel[0][2]);
    F32 doubleEdgeDivisor = kernel[1][1] + kernel[1][2] + kernel[2][1] + kernel[2][2];
    for (U32 i = 0; i < TerrainBlock::LightmapSize; i++)
    {
        for (U32 j = 0; j < TerrainBlock::LightmapSize; j++)
        {
            ColorF val;
            val = getValue(i - 1, j - 1, mLightmap) * kernel[0][0];
            val += getValue(i - 1, j, mLightmap) * kernel[0][1];
            val += getValue(i - 1, j + 1, mLightmap) * kernel[0][2];
            val += getValue(i, j - 1, mLightmap) * kernel[1][0];
            val += getValue(i, j, mLightmap) * kernel[1][1];
            val += getValue(i, j + 1, mLightmap) * kernel[1][2];
            val += getValue(i + 1, j - 1, mLightmap) * kernel[2][0];
            val += getValue(i + 1, j, mLightmap) * kernel[2][1];
            val += getValue(i + 1, j + 1, mLightmap) * kernel[2][2];

            U32 edge = 0;
            if (j == 0 || j == TerrainBlock::LightmapSize - 1)
                edge++;
            if (i == 0 || i == TerrainBlock::LightmapSize - 1)
                edge++;

            if (!edge)
                val = val / divisor;
            else
                val = mLightmap[(i)*TerrainBlock::LightmapSize + (j)];

            mLightmap[(i * TerrainBlock::LightmapSize) + j] = val;
        }
    }

    U8* lPtr = terrain->lightMap->getAddress(0, 0);
    for (U32 i = 0; i < (TerrainBlock::LightmapSize * TerrainBlock::LightmapSize); i++)
    {
        color.red = mLightmap[i].red + sgBakedLightmap[i].red;
        color.green = mLightmap[i].green + sgBakedLightmap[i].green;
        color.blue = mLightmap[i].blue + sgBakedLightmap[i].blue;
        color.clamp();
        lPtr[i * 3 + 0] = color.red * 255;
        lPtr[i * 3 + 1] = color.green * 255;
        lPtr[i * 3 + 2] = color.blue * 255;
    }
}

/// reroutes TerrainProxy::postLight for point light and TSStatic support.
void SceneLighting::TerrainProxy::postLight(bool lastLight)
{
    TerrainBlock* terrain = getObject();
    if ((!terrain) || (!lastLight))
        return;

    // set the lightmap...
    //ColorF *sgBakedLightmap = sgBakedLightmap;
    sgProcessLightMap(terrain, mLightmap, sgBakedLightmap);

    terrain->buildMaterialMap();
}

/// reroutes TerrainProxy::light for point light and TSStatic support.
void SceneLighting::TerrainProxy::light(LightInfo* light)
{
    TerrainBlock* terrain = getObject();
    if (!terrain)
        return;

    AssertFatal(((light->mType == LightInfo::Vector) ||
        (light->mType == LightInfo::SGStaticPoint) ||
        (light->mType == LightInfo::SGStaticSpot)), "wrong light type");

    //S32 time = Platform::getRealMilliseconds();


    if ((light->mType == LightInfo::SGStaticPoint) || (light->mType == LightInfo::SGStaticSpot))
    {
        //do baked point light here...
        sgLightUniversalPoint(light, terrain);

        //Con::printf("    = terrain lit in %3.3f seconds (sg)", (Platform::getRealMilliseconds()-time)/1000.f);
        return;
    }

    // reset
    mShadowVolume = new ShadowVolumeBSP;

    if ((light->mType == LightInfo::Vector) && LightManager::sgAllowShadows())
    {
        // build interior shadow volume
        for (ObjectProxy** itr = gLighting->mLitObjects.begin(); itr != gLighting->mLitObjects.end(); itr++)
        {
            if (!gLighting->isInterior((*itr)->mObj))
                continue;

            if (markInteriorShadow(static_cast<InteriorProxy*>(*itr)))
                gLighting->addInterior(mShadowVolume, *static_cast<InteriorProxy*>(*itr), light, SceneLighting::SHADOW_DETAIL);
        }

        // Static object support...
        Vector<SceneObject*>   objects;
        getCurrentClientContainer()->findObjects(ShadowCasterObjectType, sgFindObjectsCallback, &objects);

        // build static shadow volume
        for (U32 o = 0; o < objects.size(); o++)
        {
            SceneObject* obj = objects[o];
            if (gLighting->sgIsCorrectStaticObjectType(obj))
            {
                if (sgMarkStaticShadow(this, obj, light))
                    gLighting->addStatic(this, mShadowVolume, obj, light, SceneLighting::SHADOW_DETAIL);
            }
        }
    }

    //--------------------

    lightVector(light);

    delete mShadowVolume;

    //Con::printf("    = terrain lit in %3.3f seconds", (Platform::getRealMilliseconds()-time)/1000.f);
}

//------------------------------------------------------------------------------
S32 SceneLighting::TerrainProxy::testSquare(const Point3F& min, const Point3F& max, S32 mask, F32 expand, const Vector<PlaneF>& clipPlanes)
{
    expand = 0;
    S32 retMask = 0;
    Point3F minPoint, maxPoint;
    for (S32 i = 0; i < clipPlanes.size(); i++)
    {
        if (mask & (1 << i))
        {
            if (clipPlanes[i].x > 0)
            {
                maxPoint.x = max.x;
                minPoint.x = min.x;
            }
            else
            {
                maxPoint.x = min.x;
                minPoint.x = max.x;
            }
            if (clipPlanes[i].y > 0)
            {
                maxPoint.y = max.y;
                minPoint.y = min.y;
            }
            else
            {
                maxPoint.y = min.y;
                minPoint.y = max.y;
            }
            if (clipPlanes[i].z > 0)
            {
                maxPoint.z = max.z;
                minPoint.z = min.z;
            }
            else
            {
                maxPoint.z = min.z;
                minPoint.z = max.z;
            }
            F32 maxDot = mDot(maxPoint, clipPlanes[i]);
            F32 minDot = mDot(minPoint, clipPlanes[i]);
            F32 planeD = clipPlanes[i].d;
            if (maxDot <= -(planeD + expand))
                return(U16(-1));
            if (minDot <= -planeD)
                retMask |= (1 << i);
        }
    }
    return(retMask);
}

bool SceneLighting::TerrainProxy::getShadowedSquares(const Vector<PlaneF>& clipPlanes, Vector<U16>& shadowList)
{
    TerrainBlock* terrain = getObject();
    if (!terrain)
        return(false);

    SquareStackNode stack[TerrainBlock::BlockShift * 4];

    stack[0].mLevel = TerrainBlock::BlockShift;
    stack[0].mClipFlags = 0xff;
    stack[0].mPos.set(0, 0);

    U32 stackSize = 1;

    Point3F blockPos;
    terrain->getTransform().getColumn(3, &blockPos);
    S32 squareSize = terrain->getSquareSize();

    bool marked = false;

    // push through all the levels of the quadtree
    while (stackSize)
    {
        SquareStackNode* node = &stack[stackSize - 1];

        S32 clipFlags = node->mClipFlags;
        Point2I pos = node->mPos;
        GridSquare* sq = terrain->findSquare(node->mLevel, pos);

        Point3F minPoint, maxPoint;
        minPoint.set(squareSize * pos.x + blockPos.x,
            squareSize * pos.y + blockPos.y,
            fixedToFloat(sq->minHeight));
        maxPoint.set(minPoint.x + (squareSize << node->mLevel),
            minPoint.y + (squareSize << node->mLevel),
            fixedToFloat(sq->maxHeight));

        // test the square against the current level
        if (clipFlags)
        {
            clipFlags = testSquare(minPoint, maxPoint, clipFlags, squareSize, clipPlanes);
            if (clipFlags == U16(-1))
            {
                stackSize--;
                continue;
            }
        }

        // shadowed?
        if (node->mLevel == 0)
        {
            marked = true;
            shadowList.push_back(pos.x + (pos.y << TerrainBlock::BlockShift));
            stackSize--;
            continue;
        }

        // setup the next level of squares
        U8 nextLevel = node->mLevel - 1;
        S32 squareHalfSize = 1 << nextLevel;

        for (U32 i = 0; i < 4; i++)
        {
            node[i].mLevel = nextLevel;
            node[i].mClipFlags = clipFlags;
        }

        node[3].mPos = pos;
        node[2].mPos.set(pos.x + squareHalfSize, pos.y);
        node[1].mPos.set(pos.x, pos.y + squareHalfSize);
        node[0].mPos.set(pos.x + squareHalfSize, pos.y + squareHalfSize);

        stackSize += 3;
    }

    return(marked);
}

bool SceneLighting::TerrainProxy::markInteriorShadow(InteriorProxy* proxy)
{
    U32 i;
    // setup the clip planes: add the test and the lit planes
    Vector<PlaneF> clipPlanes;
    clipPlanes = proxy->mTerrainTestPlanes;
    for (i = 0; i < proxy->mLitBoxSurfaces.size(); i++)
        clipPlanes.push_back(proxy->mLitBoxSurfaces[i]->mPlane);

    Vector<U16> shadowList;
    if (!getShadowedSquares(clipPlanes, shadowList))
        return(false);

    // set the correct bit
    for (i = 0; i < shadowList.size(); i++)
        mShadowMask.set(shadowList[i]);

    return(true);
}

//-------------------------------------------------------------------------------
// BUGS: does not work with x or y directions of 0
//    : light dir of 0.1, 0.3, -0.8 causes strange behavior
void SceneLighting::TerrainProxy::lightVector(LightInfo* light)
{
    TerrainBlock* terrain = getObject();
    if (!terrain)
        return;

    Point3F lightDir = light->mDirection;
    lightDir.normalize();

    ColorF& ambient = light->mAmbient;
    ColorF& lightColor = light->mColor;

    if (lightDir.x == 0 && lightDir.y == 0)
        return;

    S32 generateLevel = Con::getIntVariable("$pref::sceneLighting::terrainGenerateLevel", 4);
    generateLevel = mClamp(generateLevel, 0, 4);

    bool allowLexelSplits = Con::getBoolVariable("$pref::sceneLighting::terrainAllowLexelSplits", false);

    U32 generateDim = TerrainBlock::LightmapSize << generateLevel;
    U32 generateShift = TerrainBlock::LightmapShift + generateLevel;
    U32 generateMask = generateDim - 1;

    F32 zStep;
    F32 frac;

    Point2I blockColStep;
    Point2I blockRowStep;
    Point2I blockFirstPos;
    Point2I lmFirstPos;

    F32 terrainDim = F32(terrain->getSquareSize()) * F32(TerrainBlock::BlockSize);
    F32 stepSize = F32(terrain->getSquareSize()) / F32(generateDim / TerrainBlock::BlockSize);

    if (mFabs(lightDir.x) >= mFabs(lightDir.y))
    {
        if (lightDir.x > 0)
        {
            zStep = lightDir.z / lightDir.x;
            frac = lightDir.y / lightDir.x;

            blockColStep.set(1, 0);
            blockRowStep.set(0, 1);
            blockFirstPos.set(0, 0);
            lmFirstPos.set(0, 0);
        }
        else
        {
            zStep = -lightDir.z / lightDir.x;
            frac = -lightDir.y / lightDir.x;

            blockColStep.set(-1, 0);
            blockRowStep.set(0, 1);
            blockFirstPos.set(255, 0);
            lmFirstPos.set(TerrainBlock::LightmapSize - 1, 0);
        }
    }
    else
    {
        if (lightDir.y > 0)
        {
            zStep = lightDir.z / lightDir.y;
            frac = lightDir.x / lightDir.y;

            blockColStep.set(0, 1);
            blockRowStep.set(1, 0);
            blockFirstPos.set(0, 0);
            lmFirstPos.set(0, 0);
        }
        else
        {
            zStep = -lightDir.z / lightDir.y;
            frac = -lightDir.x / lightDir.y;

            blockColStep.set(0, -1);
            blockRowStep.set(1, 0);
            blockFirstPos.set(0, 255);
            lmFirstPos.set(0, TerrainBlock::LightmapSize - 1);
        }
    }
    zStep *= stepSize;

    F32* heightArray = new F32[generateDim];

    S32 fracStep = -1;
    if (frac < 0)
    {
        fracStep = 1;
        frac = -frac;
    }

    F32* nextHeightArray = new F32[generateDim];
    F32 oneMinusFrac = 1 - frac;

    U32 blockShift = generateShift - TerrainBlock::BlockShift;
    U32 lightmapShift = generateShift - TerrainBlock::LightmapShift;

    U32 blockStep = 1 << blockShift;
    U32 blockMask = blockStep - 1;
    U32 lightmapStep = 1 << lightmapShift;
    U32 lightmapNormalOffset = lightmapStep >> 1;
    U32 lightmapMask = lightmapStep - 1;

    Point2I bp = blockFirstPos;
    F32 terrainHeights[2][TerrainBlock::BlockSize];
    U32 i;

    F32* pTerrainHeights = static_cast<F32*>(terrainHeights[0]);
    F32* pNextTerrainHeights = static_cast<F32*>(terrainHeights[1]);

    // get first set of heights
    for (i = 0; i < TerrainBlock::BlockSize; i++) {
        pTerrainHeights[i] = fixedToFloat(terrain->getHeight(bp.x, bp.y));
        bp += blockRowStep;
    }

    // get second set of heights
    bp = blockFirstPos + blockColStep;
    for (i = 0; i < TerrainBlock::BlockSize; i++) {
        pNextTerrainHeights[i] = fixedToFloat(terrain->getHeight(bp.x, bp.y));
        bp += blockRowStep;
    }

    F32 heightStep = 1.f / blockStep;

    F32 terrainZRowStep[TerrainBlock::BlockSize];
    F32 nextTerrainZRowStep[TerrainBlock::BlockSize];
    F32 terrainZColStep[TerrainBlock::BlockSize];

    // fill in the row steps
    for (i = 0; i < TerrainBlock::BlockSize; i++)
    {
        terrainZRowStep[i] = (pTerrainHeights[(i + 1) & TerrainBlock::BlockMask] - pTerrainHeights[i]) * heightStep;
        nextTerrainZRowStep[i] = (pNextTerrainHeights[(i + 1) & TerrainBlock::BlockMask] - pNextTerrainHeights[i]) * heightStep;
        terrainZColStep[i] = (pNextTerrainHeights[i] - pTerrainHeights[i]) * heightStep;
    }

    // get first row of process heights
    for (i = 0; i < generateDim; i++)
    {
        U32 bi = i >> blockShift;
        heightArray[i] = pTerrainHeights[bi] + (i & blockMask) * terrainZRowStep[bi];
    }

    bp = blockFirstPos;
    if (generateDim == TerrainBlock::BlockSize)
        bp += blockColStep;

    // generate the initial run
    U32 x, y;
    for (x = 1; x < generateDim; x++)
    {
        U32 xmask = x & blockMask;

        // generate new height step rows?
        if (!xmask)
        {
            F32* tmp = pTerrainHeights;
            pTerrainHeights = pNextTerrainHeights;
            pNextTerrainHeights = tmp;

            bp += blockColStep;

            Point2I bwalk = bp;
            for (i = 0; i < TerrainBlock::BlockSize; i++, bwalk += blockRowStep)
                pNextTerrainHeights[i] = fixedToFloat(terrain->getHeight(bwalk.x, bwalk.y));

            // fill in the row steps
            for (i = 0; i < TerrainBlock::BlockSize; i++)
            {
                terrainZRowStep[i] = (pTerrainHeights[(i + 1) & TerrainBlock::BlockMask] - pTerrainHeights[i]) * heightStep;
                nextTerrainZRowStep[i] = (pNextTerrainHeights[(i + 1) & TerrainBlock::BlockMask] - pNextTerrainHeights[i]) * heightStep;
                terrainZColStep[i] = (pNextTerrainHeights[i] - pTerrainHeights[i]) * heightStep;
            }
        }

        Point2I bwalk = bp - blockRowStep;
        for (y = 0; y < generateDim; y++)
        {
            U32 ymask = y & blockMask;
            if (!ymask)
                bwalk += blockRowStep;

            U32 bi = y >> blockShift;
            U32 binext = (bi + 1) & TerrainBlock::BlockMask;

            F32 height;

            // 135?
            if ((bwalk.x ^ bwalk.y) & 1)
            {
                U32 xsub = blockStep - xmask;
                if (xsub > ymask) // bottom
                    height = pTerrainHeights[bi] + xmask * terrainZColStep[bi] +
                    ymask * terrainZRowStep[bi];
                else // top
                    height = pNextTerrainHeights[bi] - xmask * terrainZColStep[binext] +
                    ymask * nextTerrainZRowStep[bi];
            }
            else
            {
                if (xmask > ymask) // bottom
                    height = pTerrainHeights[bi] + xmask * terrainZColStep[bi] +
                    ymask * nextTerrainZRowStep[bi];
                else // top
                    height = pTerrainHeights[bi] + xmask * terrainZColStep[binext] +
                    ymask * terrainZRowStep[bi];
            }

            F32 intHeight = heightArray[y] * oneMinusFrac + heightArray[(y + fracStep) & generateMask] * frac + zStep;
            nextHeightArray[y] = getMax(height, intHeight);
        }

        // swap the height rows
        F32* tmp = heightArray;
        heightArray = nextHeightArray;
        nextHeightArray = tmp;
    }

    F32 squareSize = terrain->getSquareSize();
    F32 lexelDim = squareSize * F32(TerrainBlock::BlockSize) / F32(TerrainBlock::LightmapSize);

    // calculate normal runs
    Point3F normals[2][TerrainBlock::BlockSize];

    Point3F* pNormals = static_cast<Point3F*>(normals[0]);
    Point3F* pNextNormals = static_cast<Point3F*>(normals[1]);

    // calculate the normal lookup table
    F32* normTable = new F32[blockStep * blockStep * 4];

    Point2F corners[4] = {
        Point2F(0.f, 0.f),
            Point2F(1.f, 0.f),
            Point2F(1.f, 1.f),
            Point2F(0.f, 1.f)
    };

    U32 idx = 0;
    F32 step = 1.f / blockStep;
    Point2F pos(0.f, 0.f);

    // fill it
    for (x = 0; x < blockStep; x++, pos.x += step, pos.y = 0.f) {
        for (y = 0; y < blockStep; y++, pos.y += step) {
            for (i = 0; i < 4; i++, idx++)
                normTable[idx] = 1.f - getMin(Point2F(pos - corners[i]).len(), 1.f);
        }
    }

    // fill first column
    bp = blockFirstPos;
    for (x = 0; x < TerrainBlock::BlockSize; x++)
    {
        pNextTerrainHeights[x] = fixedToFloat(terrain->getHeight(bp.x, bp.y));
        Point2F pos(bp.x * squareSize, bp.y * squareSize);
        terrain->getNormal(pos, &pNextNormals[x]);
        bp += blockRowStep;
    }

    // get swapped on first pass
    pTerrainHeights = static_cast<F32*>(terrainHeights[1]);
    pNextTerrainHeights = static_cast<F32*>(terrainHeights[0]);

    // get the world offset of the terrain
    const MatrixF& transform = terrain->getTransform();
    Point3F worldOffset;
    transform.getColumn(3, &worldOffset);

    F32 ratio = F32(1 << lightmapShift);
    F32 ratioSquared = ratio * ratio;
    F32 inverseRatioSquared = 1.f / ratioSquared;

    F32 lightScale[TerrainBlock::LightmapSize];

    // walk it...
    bp = blockFirstPos - blockColStep;
    for (x = 0; x < generateDim; x++)
    {
        U32 xmask = x & blockMask;
        U32 lxmask = x & lightmapMask;

        // generate new runs?
        if (!xmask)
        {
            bp += blockColStep;

            // do the normals
            Point3F* temp = pNormals;
            pNormals = pNextNormals;
            pNextNormals = temp;

            // fill the row
            Point2I bwalk = bp + blockColStep;
            for (i = 0; i < TerrainBlock::BlockSize; i++)
            {
                Point2F pos(bwalk.x * squareSize, bwalk.y * squareSize);
                terrain->getNormal(pos, &pNextNormals[i]);
                bwalk += blockRowStep;
            }

            // do the heights
            F32* tmp = pTerrainHeights;
            pTerrainHeights = pNextTerrainHeights;
            pNextTerrainHeights = tmp;

            bwalk = bp + blockColStep;
            for (i = 0; i < TerrainBlock::BlockSize; i++, bwalk += blockRowStep)
                pNextTerrainHeights[i] = fixedToFloat(terrain->getHeight(bwalk.x, bwalk.y));

            // fill in the row steps
            for (i = 0; i < TerrainBlock::BlockSize; i++)
            {
                terrainZRowStep[i] = (pTerrainHeights[(i + 1) & TerrainBlock::BlockMask] - pTerrainHeights[i]) * heightStep;
                nextTerrainZRowStep[i] = (pNextTerrainHeights[(i + 1) & TerrainBlock::BlockMask] - pNextTerrainHeights[i]) * heightStep;
                terrainZColStep[i] = (pNextTerrainHeights[i] - pTerrainHeights[i]) * heightStep;
            }
        }

        // reset the light scale table
        if (!lxmask)
            for (i = 0; i < TerrainBlock::LightmapSize; i++)
                lightScale[i] = 1.f;

        Point2I bwalk = bp - blockRowStep;
        for (y = 0; y < generateDim; y++)
        {
            U32 lymask = y & lightmapMask;
            U32 ymask = y & blockMask;
            if (!ymask)
                bwalk += blockRowStep;

            U32 bi = y >> blockShift;
            U32 binext = (bi + 1) & TerrainBlock::BlockMask;

            F32 height;
            F32 xstep, ystep;

            // 135?
            if ((bwalk.x ^ bwalk.y) & 1)
            {
                U32 xsub = blockStep - xmask;
                if (xsub > ymask) // bottom
                {
                    xstep = terrainZColStep[bi];
                    ystep = terrainZRowStep[bi];
                    height = pTerrainHeights[bi] + xmask * xstep + ymask * ystep;
                }
                else // top
                {
                    xstep = -terrainZColStep[binext];
                    ystep = nextTerrainZRowStep[bi];
                    height = pNextTerrainHeights[bi] + xsub * xstep + ymask * ystep;
                }
            }
            else
            {
                if (xmask > ymask) // bottom
                {
                    xstep = terrainZColStep[bi];
                    ystep = nextTerrainZRowStep[bi];
                    height = pTerrainHeights[bi] + xmask * xstep + ymask * ystep;
                }
                else // top
                {
                    xstep = terrainZColStep[binext];
                    ystep = terrainZRowStep[bi];
                    height = pTerrainHeights[bi] + xmask * xstep + ymask * ystep;
                }
            }

            F32 intHeight = heightArray[y] * oneMinusFrac + heightArray[(y + fracStep) & generateMask] * frac + zStep;

            U32 lsi = y >> lightmapShift;

            Point2I lmPos = lmFirstPos + blockColStep * (x >> lightmapShift) + blockRowStep * lsi;

            ColorF& col = mLightmap[lmPos.x + (lmPos.y << TerrainBlock::LightmapShift)];

            // lexel shaded by an interior?
            Point2I terrPos = lmPos;
            terrPos.x >>= TerrainBlock::LightmapShift - TerrainBlock::BlockShift;
            terrPos.y >>= TerrainBlock::LightmapShift - TerrainBlock::BlockShift;

            if (!lxmask && !lymask && mShadowMask.test(terrPos.x + (terrPos.y << TerrainBlock::BlockShift)))
            {
                U32 idx = (xmask + lightmapNormalOffset + ((ymask + lightmapNormalOffset) << blockShift)) << 2;

                // get the normal for this lexel
                Point3F normal = pNormals[bi] * normTable[idx++];
                normal += pNormals[binext] * normTable[idx++];
                normal += pNextNormals[binext] * normTable[idx++];
                normal += pNextNormals[bi] * normTable[idx];
                normal.normalize();

                nextHeightArray[y] = height;
                F32 colorScale = -mDot(normal, lightDir);

                if (colorScale > 0.f)
                {
                    // split lexels which cross the square split?
                    if (allowLexelSplits)
                    {
                        // jff:todo
                    }
                    else
                    {
                        Point2F wPos((lmPos.x) * lexelDim + worldOffset.x,
                            (lmPos.y) * lexelDim + worldOffset.y);

                        F32 xh = xstep * ratio;
                        F32 yh = ystep * ratio;

                        ShadowVolumeBSP::SVPoly* poly = mShadowVolume->createPoly();
                        poly->mWindingCount = 4;
                        poly->mWinding[0].set(wPos.x, wPos.y, height);
                        poly->mWinding[1].set(wPos.x + lexelDim, wPos.y, height + xh);
                        poly->mWinding[2].set(wPos.x + lexelDim, wPos.y + lexelDim, height + xh + yh);
                        poly->mWinding[3].set(wPos.x, wPos.y + lexelDim, height + yh);
                        poly->mPlane.set(poly->mWinding[0], poly->mWinding[1], poly->mWinding[2]);

                        F32 lexelSize = mShadowVolume->getPolySurfaceArea(poly);
                        F32 intensity = mShadowVolume->getClippedSurfaceArea(poly) / lexelSize;
                        lightScale[lsi] = mClampF(intensity, 0.f, 1.f);
                    }
                }
                else
                    lightScale[lsi] = 0.f;
            }

            // non shadowed?
            if (height >= intHeight)
            {
                U32 idx = (xmask + (ymask << blockShift)) << 2;

                Point3F normal = pNormals[bi] * normTable[idx++];
                normal += pNormals[binext] * normTable[idx++];
                normal += pNextNormals[binext] * normTable[idx++];
                normal += pNextNormals[bi] * normTable[idx];
                normal.normalize();

                nextHeightArray[y] = height;
                F32 colorScale = -mDot(normal, lightDir);

                if (colorScale > 0.f)
                    col = ambient + lightColor * colorScale * lightScale[lsi];
                else
                    col = ambient;
            }
            else
            {
                nextHeightArray[y] = intHeight;
                col = ambient;
            }
        }

        F32* tmp = heightArray;
        heightArray = nextHeightArray;
        nextHeightArray = tmp;
    }

    // set the proper color
    for (i = 0; i < TerrainBlock::LightmapSize * TerrainBlock::LightmapSize; i++)
    {
        //mLightmap[i] *= inverseRatioSquared;
        mLightmap[i] /= TERRAIN_OVERRANGE;
        mLightmap[i].clamp();
    }

    delete[] normTable;
    delete[] heightArray;
    delete[] nextHeightArray;
}

//--------------------------------------------------------------------------
U32 SceneLighting::TerrainProxy::getResourceCRC()
{
    TerrainBlock* terrain = getObject();
    if (!terrain)
        return(0);
    return(terrain->getCRC());
}

//--------------------------------------------------------------------------
bool SceneLighting::TerrainProxy::setPersistInfo(PersistInfo::PersistChunk* info)
{
    if (!Parent::setPersistInfo(info))
        return(false);

    PersistInfo::TerrainChunk* chunk = dynamic_cast<PersistInfo::TerrainChunk*>(info);
    AssertFatal(chunk, "SceneLighting::TerrainProxy::setPersistInfo: invalid info chunk!");

    TerrainBlock* terrain = getObject();
    if (!terrain || !terrain->lightMap)
        return(false);

    if (terrain->lightMap) delete terrain->lightMap;

    terrain->lightMap = new GBitmap(*chunk->mLightmap);

    terrain->buildMaterialMap();
    return(true);
}

bool SceneLighting::TerrainProxy::getPersistInfo(PersistInfo::PersistChunk* info)
{
    if (!Parent::getPersistInfo(info))
        return(false);

    PersistInfo::TerrainChunk* chunk = dynamic_cast<PersistInfo::TerrainChunk*>(info);
    AssertFatal(chunk, "SceneLighting::TerrainProxy::getPersistInfo: invalid info chunk!");

    TerrainBlock* terrain = getObject();
    if (!terrain || !terrain->lightMap)
        return(false);

    if (chunk->mLightmap) delete chunk->mLightmap;

    chunk->mLightmap = new GBitmap(*terrain->lightMap);

    return(true);
}

/// adds TSStatic objects as shadow casters.
bool SceneLighting::TerrainProxy::sgMarkStaticShadow(void* terrainproxy,
    SceneObject* sceneobject, LightInfo* light)
{
    SceneLighting::TerrainProxy* terrain = (SceneLighting::TerrainProxy*)terrainproxy;
    if ((!terrain) || (!sceneobject))
        return false;

    // create shadow volume of the bounding box of this object
    if (light->mType != LightInfo::Vector)
        return(false);

    Vector<ShadowVolumeBSP::SVPoly*> shadowvolumesurfaces;
    Vector<PlaneF> shadowplanes;

    const Box3F& objBox = sceneobject->getObjBox();
    const MatrixF& objTransform = sceneobject->getTransform();
    const VectorF& objScale = sceneobject->getScale();

    // grab the surfaces which form the shadow volume
    U32 numPlanes = 0;
    PlaneF testPlanes[3];
    U32 planeIndices[3];

    // grab the bounding planes which face the light
    U32 i;
    for (i = 0; (i < 6) && (numPlanes < 3); i++)
    {
        PlaneF plane;
        plane.x = BoxNormals[i].x;
        plane.y = BoxNormals[i].y;
        plane.z = BoxNormals[i].z;

        if (i & 1)
            plane.d = (((const float*)objBox.min)[(i - 1) >> 1]);
        else
            plane.d = -(((const float*)objBox.max)[i >> 1]);

        // project
        mTransformPlane(objTransform, objScale, plane, &testPlanes[numPlanes]);

        planeIndices[numPlanes] = i;

        if (mDot(testPlanes[numPlanes], light->mDirection) < gParellelVectorThresh)
            numPlanes++;
    }
    AssertFatal(numPlanes, "SceneLighting::InteriorProxy::preLight: no planes found");

    // project the points
    Point3F projPnts[8];
    for (i = 0; i < 8; i++)
    {
        Point3F pnt;
        pnt.set(BoxPnts[i].x ? objBox.max.x : objBox.min.x,
            BoxPnts[i].y ? objBox.max.y : objBox.min.y,
            BoxPnts[i].z ? objBox.max.z : objBox.min.z);

        // scale it
        pnt.convolve(objScale);
        objTransform.mulP(pnt, &projPnts[i]);
    }

    ShadowVolumeBSP* mBoxShadowBSP = new ShadowVolumeBSP;

    // insert the shadow volumes for the surfaces
    for (i = 0; i < numPlanes; i++)
    {
        ShadowVolumeBSP::SVPoly* poly = mBoxShadowBSP->createPoly();
        poly->mWindingCount = 4;

        U32 j;
        for (j = 0; j < 4; j++)
            poly->mWinding[j] = projPnts[BoxVerts[planeIndices[i]][j]];

        testPlanes[i].neg();
        poly->mPlane = testPlanes[i];

        mBoxShadowBSP->buildPolyVolume(poly, light);
        shadowvolumesurfaces.push_back(mBoxShadowBSP->copyPoly(poly));
        mBoxShadowBSP->insertPoly(poly);

        // create the opposite planes for testing against terrain
        Point3F pnts[3];
        for (j = 0; j < 3; j++)
            pnts[j] = projPnts[BoxVerts[planeIndices[i] ^ 1][j]];
    }

    // grab the unique planes for terrain checks
    for (i = 0; i < numPlanes; i++)
    {
        U32 mask = 0;
        for (U32 j = 0; j < numPlanes; j++)
            mask |= BoxSharedEdgeMask[planeIndices[i]][planeIndices[j]];

        ShadowVolumeBSP::SVNode* traverse = mBoxShadowBSP->getShadowVolume(shadowvolumesurfaces[i]->mShadowVolume);
        while (traverse->mFront)
        {
            if (!(mask & 1))
                shadowplanes.push_back(mBoxShadowBSP->getPlane(traverse->mPlaneIndex));

            mask >>= 1;
            traverse = traverse->mFront;
        }
    }

    // there will be 2 duplicate node planes if there were only 2 planes lit
    if (numPlanes == 2)
    {
        for (S32 i = 0; i < shadowvolumesurfaces.size(); i++)
            for (U32 j = 0; j < shadowvolumesurfaces.size(); j++)
            {
                if (i == j)
                    continue;

                if ((mDot(shadowplanes[i], shadowplanes[j]) > gPlaneNormThresh) &&
                    (mFabs(shadowplanes[i].d - shadowplanes[j].d) < gPlaneDistThresh))
                {
                    shadowplanes.erase(i);
                    i--;
                    break;
                }
            }
    }

    // setup the clip planes: add the test and the lit planes
    Vector<PlaneF> clipPlanes;
    clipPlanes = shadowplanes;
    for (i = 0; i < shadowvolumesurfaces.size(); i++)
        clipPlanes.push_back(shadowvolumesurfaces[i]->mPlane);

    Vector<U16> shadowList;
    if (!terrain->getShadowedSquares(clipPlanes, shadowList))
    {
        delete mBoxShadowBSP;
        return(false);
    }

    // set the correct bit
    for (i = 0; i < shadowList.size(); i++)
        terrain->mShadowMask.set(shadowList[i]);

    delete mBoxShadowBSP;
    return(true);
}
#endif
