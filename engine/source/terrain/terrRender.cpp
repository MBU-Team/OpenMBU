//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------


#include "terrain/terrData.h"
#include "math/mMath.h"
#include "console/console.h"
#include "console/consoleTypes.h"
#include "gfx/gBitmap.h"
#include "terrain/terrRender.h"
#include "sceneGraph/sceneState.h"
#include "terrain/waterBlock.h"
#include "terrain/blender.h"
#include "core/frameAllocator.h"
#include "sceneGraph/sceneGraph.h"
#include "sceneGraph/sgUtil.h"
#include "platform/profiler.h"
#include "gfx/gfxDevice.h"
#include "materials/matInstance.h"
#include "terrain/sky.h"
#include "terrain/terrBatch.h"
#include "terrain/terrTexture.h"
#include "gfx/gfxCanon.h"
#include "lightingSystem/sgLightingModel.h"

MatrixF     TerrainRender::mCameraToObject;
SceneState* TerrainRender::mSceneState;

S32 TerrainRender::mSquareSize;
F32 TerrainRender::mScreenSize;
U32 TerrainRender::mFrameIndex;
F32 TerrainRender::mPixelError;
F32 TerrainRender::mScreenError;
F32 TerrainRender::mMinSquareSize;
F32 TerrainRender::mFarDistance;
S32 sgFogRejectedBlocks = 0;

TerrainBlock* TerrainRender::mCurrentBlock;
Point2F       TerrainRender::mBlockPos;
Point2I       TerrainRender::mBlockOffset;
Point2I       TerrainRender::mTerrainOffset;
PlaneF        TerrainRender::mClipPlane[MaxClipPlanes];
U32           TerrainRender::mNumClipPlanes = 4;
Point3F       TerrainRender::mCamPos;

U32           TerrainRender::mDynamicLightCount;
bool          TerrainRender::mEnableTerrainDynLights = true;
TerrLightInfo TerrainRender::mTerrainLights[MaxTerrainLights];
static        LightTriangle* sgCurrLightTris = NULL;

bool TerrainRender::mRenderingCommander = false;


void TerrainRender::init()
{
    mFrameIndex = 0;

    mScreenError = 4;
    mScreenSize = 45;
    mMinSquareSize = 4;
    mFarDistance = 500;

    Con::addVariable("screenSize", TypeF32, &mScreenSize);
    Con::addVariable("farDistance", TypeF32, &mFarDistance);

    // Goofy hack, remove me - BJG
    Con::addVariable("inCommanderMap", TypeBool, &mRenderingCommander);

    Con::addVariable("pref::Terrain::dynamicLights", TypeBool, &mEnableTerrainDynLights);
    Con::addVariable("pref::Terrain::screenError", TypeF32, &mScreenError);

    TerrBatch::init();
    TerrTexture::init();
}

void TerrainRender::shutdown()
{
    TerrTexture::shutdown();
}

void TerrainRender::buildClippingPlanes(bool flipClipPlanes)
{
    PROFILE_START(TerrainRender_buildClippingPlanes);

    F32 frustumParam[6];
    F64 realfrustumParam[6];

    GFX->getFrustum(&frustumParam[0], &frustumParam[1],
        &frustumParam[2], &frustumParam[3],
        &frustumParam[4], &frustumParam[5]);


    // Cast se can put them in the sg call.
    for (U32 i = 0; i < 6; i++)
        realfrustumParam[i] = frustumParam[i];

    Point3F osCamPoint(0, 0, 0);
    mCameraToObject.mulP(osCamPoint);
    sgComputeOSFrustumPlanes(realfrustumParam,
        mCameraToObject,
        osCamPoint,
        mClipPlane[4],
        mClipPlane[0],
        mClipPlane[1],
        mClipPlane[2],
        mClipPlane[3]);

    // no need
    mNumClipPlanes = 4;

    // near plane is needed as well...
    //PlaneF p(0, 1, 0, -frustumParam[4]);
    //mTransformPlane(mCameraToObject, Point3F(1,1,1), p, &mClipPlane[0]);

    if (flipClipPlanes) {
        mClipPlane[0].neg();
        mClipPlane[1].neg();
        mClipPlane[2].neg();
        mClipPlane[3].neg();
        mClipPlane[4].neg();
        mClipPlane[5].neg();
    }

    PROFILE_END();
}

S32 TerrainRender::testSquareVisibility(Point3F& min, Point3F& max, S32 mask, F32 expand)
{
    PROFILE_START(TerrainRender_testSquareVisibility);
    S32 retMask = 0;
    Point3F minPoint, maxPoint;
    for (S32 i = 0; i < mNumClipPlanes; i++)
    {
        if (mask & (1 << i))
        {
            if (mClipPlane[i].x > 0)
            {
                maxPoint.x = max.x;
                minPoint.x = min.x;
            }
            else
            {
                maxPoint.x = min.x;
                minPoint.x = max.x;
            }
            if (mClipPlane[i].y > 0)
            {
                maxPoint.y = max.y;
                minPoint.y = min.y;
            }
            else
            {
                maxPoint.y = min.y;
                minPoint.y = max.y;
            }
            if (mClipPlane[i].z > 0)
            {
                maxPoint.z = max.z;
                minPoint.z = min.z;
            }
            else
            {
                maxPoint.z = min.z;
                minPoint.z = max.z;
            }
            F32 maxDot = mDot(maxPoint, mClipPlane[i]);
            F32 minDot = mDot(minPoint, mClipPlane[i]);
            F32 planeD = mClipPlane[i].d;
            if (maxDot <= -(planeD + expand))
            {
                PROFILE_END();
                return -1;
            }
            if (minDot <= -planeD)
                retMask |= (1 << i);
        }
    }
    PROFILE_END();
    return retMask;
}

ChunkCornerPoint* TerrainRender::allocInitialPoint(Point3F pos)
{
    ChunkCornerPoint* ret = (ChunkCornerPoint*)FrameAllocator::alloc(sizeof(ChunkCornerPoint));
    ret->x = pos.x;
    ret->y = pos.y;
    ret->z = pos.z;
    ret->xfIndex = 0;

    return ret;
}

ChunkCornerPoint* TerrainRender::allocPoint(Point2I pos)
{
    ChunkCornerPoint* ret = (ChunkCornerPoint*)FrameAllocator::alloc(sizeof(ChunkCornerPoint));
    ret->x = pos.x * mSquareSize + mBlockPos.x;
    ret->y = pos.y * mSquareSize + mBlockPos.y;
    ret->z = fixedToFloat(mCurrentBlock->getHeight(pos.x, pos.y));
    ret->xfIndex = 0;

    return ret;
}

void TerrainRender::allocRenderEdges(U32 edgeCount, EdgeParent** dest, bool renderEdge)
{
    if (renderEdge)
    {
        for (U32 i = 0; i < edgeCount; i++)
        {
            ChunkEdge* edge = (ChunkEdge*)FrameAllocator::alloc(sizeof(ChunkEdge));
            edge->c1 = NULL;
            edge->c2 = NULL;
            edge->xfIndex = 0;
            edge->pointCount = 0;
            edge->pointIndex = 0;
            dest[i] = edge;
        }
    }
    else
    {
        for (U32 i = 0; i < edgeCount; i++)
        {
            ChunkScanEdge* edge = (ChunkScanEdge*)FrameAllocator::alloc(sizeof(ChunkScanEdge));
            edge->mp = NULL;
            dest[i] = edge;
        }
    }
}

void TerrainRender::subdivideChunkEdge(ChunkScanEdge* e, Point2I pos, bool chunkEdge)
{
    if (!e->mp)
    {
        allocRenderEdges(2, &e->e1, chunkEdge);
        e->mp = allocPoint(pos);

        e->e1->p1 = e->p1;
        e->e1->p2 = e->mp;
        e->e2->p1 = e->mp;
        e->e2->p2 = e->p2;
    }
}

F32 TerrainRender::getSquareDistance(const Point3F& minPoint, const Point3F& maxPoint, F32* zDiff)
{
    Point3F vec;
    if (mCamPos.z < minPoint.z)
        vec.z = minPoint.z - mCamPos.z;
    else if (mCamPos.z > maxPoint.z)
        vec.z = maxPoint.z - mCamPos.z;
    else
        vec.z = 0;

    if (mCamPos.x < minPoint.x)
        vec.x = minPoint.x - mCamPos.x;
    else if (mCamPos.x > maxPoint.x)
        vec.x = mCamPos.x - maxPoint.x;
    else
        vec.x = 0;

    if (mCamPos.y < minPoint.y)
        vec.y = minPoint.y - mCamPos.y;
    else if (mCamPos.y > maxPoint.y)
        vec.y = mCamPos.y - maxPoint.y;
    else
        vec.y = 0;

    *zDiff = vec.z;

    return vec.len();
}

void TerrainRender::emitTerrChunk(SquareStackNode* n, F32 squareDistance, U32 lightMask, bool farClip, bool drawDetails)
{
    //if(n->pos.x || n->pos.y)
    //   return;

    GridChunk* gc = mCurrentBlock->findChunk(n->pos);

    EmitChunk* chunk = (EmitChunk*)FrameAllocator::alloc(sizeof(EmitChunk));
    chunk->x = n->pos.x + mBlockOffset.x + mTerrainOffset.x;
    chunk->y = n->pos.y + mBlockOffset.y + mTerrainOffset.y;
    chunk->gridX = n->pos.x;
    chunk->gridY = n->pos.y;
    chunk->lightMask = lightMask;

    chunk->next = TerrTexture::mCurrentTexture->emitList;
    TerrTexture::mCurrentTexture->emitList = chunk;

    if (mRenderingCommander)
        return;

    chunk->edge[0] = (ChunkEdge*)n->top;
    chunk->edge[1] = (ChunkEdge*)n->right;
    chunk->edge[2] = (ChunkEdge*)n->bottom;
    chunk->edge[3] = (ChunkEdge*)n->left;

    chunk->edge[0]->c2 = chunk;
    chunk->edge[1]->c1 = chunk;
    chunk->edge[2]->c1 = chunk;
    chunk->edge[3]->c2 = chunk;


    // holes only in the primary terrain block
    if (gc->emptyFlags && mBlockPos.x == 0 && mBlockPos.y == 0)
        chunk->emptyFlags = gc->emptyFlags;
    else
        chunk->emptyFlags = 0;

    S32 subDivLevel;
    F32 growFactor = 0;

    F32 minSubdivideDistance = 1000000;
    chunk->clip = farClip;

    chunk->renderDetails = drawDetails;

    if (squareDistance < 1)
        subDivLevel = -1;
    else
    {
        for (subDivLevel = 2; subDivLevel >= 0; subDivLevel--)
        {
            F32 subdivideDistance = fixedToFloat(gc->heightDeviance[subDivLevel]) / mPixelError;
            if (subdivideDistance > minSubdivideDistance)
                subdivideDistance = minSubdivideDistance;

            if (squareDistance >= subdivideDistance)
                break;

            F32 clampDistance = subdivideDistance * 0.75;

            if (squareDistance > clampDistance)
            {
                growFactor = (squareDistance - clampDistance) / (0.25 * subdivideDistance);
                subDivLevel--;
                break;
            }

            minSubdivideDistance = clampDistance;
        }
    }
    chunk->subDivLevel = subDivLevel;
    chunk->growFactor = growFactor;
}

void TerrainRender::processBlock(SceneState*, EdgeParent* topEdge, EdgeParent* rightEdge, EdgeParent* bottomEdge, EdgeParent* leftEdge)
{
    SquareStackNode stack[TerrainBlock::BlockShift * 4];
    Point3F minPoint, maxPoint;

    // Set up the root node of the stack...
    stack[0].level = TerrainBlock::BlockShift;
    stack[0].clipFlags = ((1 << mNumClipPlanes) - 1) | FarSphereMask;  // test all the planes
    stack[0].pos.set(0, 0);
    stack[0].top = topEdge;
    stack[0].right = rightEdge;
    stack[0].bottom = bottomEdge;
    stack[0].left = leftEdge;
    stack[0].lightMask = (1 << mDynamicLightCount) - 1; // test all the lights
    stack[0].texAllocated = false;

    // St up fog...
    Vector<SceneState::FogBand>* posFog = mSceneState->getPosFogBands();
    Vector<SceneState::FogBand>* negFog = mSceneState->getNegFogBands();
    bool clipAbove = posFog->size() > 0 && (*posFog)[0].isFog == false;
    bool clipBelow = negFog->size() > 0 && (*negFog)[0].isFog == false;
    bool clipOn = posFog->size() > 0 && (*posFog)[0].isFog == true;

    if (posFog->size() != 0 || negFog->size() != 0)
        stack[0].clipFlags |= FogPlaneBoxMask;

    // Initialize the stack
    S32 curStackSize = 1;
    F32 squareDistance;

    while (curStackSize)
    {
        SquareStackNode* n = stack + curStackSize - 1;

        // see if it's visible
        GridSquare* sq = mCurrentBlock->findSquare(n->level, n->pos);

        // Calculate bounding points...
        minPoint.set(mSquareSize * n->pos.x + mBlockPos.x,
            mSquareSize * n->pos.y + mBlockPos.y,
            fixedToFloat(sq->minHeight));
        maxPoint.set(minPoint.x + (mSquareSize << n->level),
            minPoint.y + (mSquareSize << n->level),
            fixedToFloat(sq->maxHeight));

        // holes only in the primary terrain block
        if ((sq->flags & GridSquare::Empty) && mBlockPos.x == 0 && mBlockPos.y == 0)
        {
            curStackSize--;
            continue;
        }

        F32 zDiff;
        squareDistance = getSquareDistance(minPoint, maxPoint, &zDiff);

        S32 nextClipFlags = 0;

        if (n->clipFlags)
        {

            if (n->clipFlags & FogPlaneBoxMask)
            {
                F32 camZ = mCamPos.z;
                bool boxBelow = camZ > maxPoint.z;
                bool boxAbove = camZ < minPoint.z;
                bool boxOn = !(boxAbove || boxBelow);
                if (clipOn ||
                    (clipAbove && boxAbove && (maxPoint.z - camZ > (*posFog)[0].cap)) ||
                    (clipBelow && boxBelow && (camZ - minPoint.z > (*negFog)[0].cap)) ||
                    (boxOn && ((clipAbove && maxPoint.z - camZ > (*posFog)[0].cap) ||
                        (clipBelow && camZ - minPoint.z > (*negFog)[0].cap))))
                {
                    // Using the fxSunLight can cause the "sky" to extend down below the camera.
                    // To avoid the sun showing through, the terrain must always be rendered.
                    // If the fxSunLight is not being used, the following optimization can be
                    // uncommented.
#if 0
                    if (boxBelow && !mSceneState->isBoxFogVisible(squareDistance, maxPoint.z, minPoint.z))
                    {
                        // Totally fogged terrain tiles can be thrown out as long as they are
                        // below the camera. If they are ubove, the sky will show through the
                        // fog.
                        curStackSize--;
                        continue;
                    }
#endif
                    nextClipFlags |= FogPlaneBoxMask;
                }
            }

            if (n->clipFlags & FarSphereMask)
            {
                // Reject stuff outside our visible range...
                if (squareDistance >= mFarDistance)
                {
                    curStackSize--;
                    continue;
                }

                // Otherwise clip againse the bounding sphere...
                S32 dblSquareSz = mSquareSize << (n->level + 1);

                // We add a "fudge factor" to get the furthest possible point of the square...
                if (squareDistance + (maxPoint.z - minPoint.z) + dblSquareSz > mFarDistance)
                    nextClipFlags |= FarSphereMask;
            }


            // zDelta for screen error height deviance.
            F32 zDelta = squareDistance * mPixelError;
            minPoint.z -= zDelta;
            maxPoint.z += zDelta;

            nextClipFlags |= testSquareVisibility(minPoint, maxPoint, n->clipFlags, mSquareSize);
            if (nextClipFlags == -1)
            {
                // trivially rejected, so pop it off the stack
                curStackSize--;
                continue;
            }

        }

        if (!n->texAllocated)
        {
            S32 squareSz = mSquareSize << n->level;
            // first check the level - if its 3 or less, we have to make a bitmap:
            // level 3 == 8x8 square - 8x8 * 16x16 == 128x128
            if (n->level > 6)
                goto notexalloc;

            S32 mipLevel = TerrainTextureMipLevel;
            if (!mRenderingCommander)
            {
                if (n->level > TerrTexture::mTextureMinSquareSize + 2)
                {
                    // get the mip level of the square and see if we're in range
                    if (squareDistance > 0.001)
                    {
                        F32 size = GFX->projectRadius(squareDistance + (squareSz >> 1), squareSz);
                        mipLevel = getPower((S32)(size * 0.75));
                        if (mipLevel > TerrainTextureMipLevel) // too big for this square
                            goto notexalloc;
                    }
                    else
                        goto notexalloc;
                }
            }

            AllocatedTexture::alloc(n->pos, n->level);
            n->texAllocated = true;

            // What the heck is this here for?
            if (mRenderingCommander) // level == 6
            {
                emitTerrChunk(n, 0, 0, 0, 0);
                curStackSize--;
                continue;
            }
        }

        // This is here so we can skip allocating a texture if we like...
    notexalloc:

        // Proceed with life as normal...

        // If we have been marked as lit we need to see if that still holds true...
        if (n->lightMask)
            n->lightMask = testSquareLights(sq, n->level, n->pos, n->lightMask);

        if (n->level == 2)
        {
            // We've tesselated far enough, stop here.
            AssertFatal(n->texAllocated, "Invalid texture index.");

            emitTerrChunk(n, squareDistance, n->lightMask, nextClipFlags & FarSphereMask, false);
            curStackSize--;
            continue;
        }

        // Ok, we have to tesselate further down...

        bool allocChunkEdges = (n->level == 3);

        Point2I pos = n->pos;

        ChunkScanEdge* top = (ChunkScanEdge*)n->top;
        ChunkScanEdge* right = (ChunkScanEdge*)n->right;
        ChunkScanEdge* bottom = (ChunkScanEdge*)n->bottom;
        ChunkScanEdge* left = (ChunkScanEdge*)n->left;

        // subdivide this square and throw it on the stack
        S32 squareOneSize = 1 << n->level;
        S32 squareHalfSize = squareOneSize >> 1;

        ChunkCornerPoint* midPoint = allocPoint(Point2I(pos.x + squareHalfSize, pos.y + squareHalfSize));
        S32 nextLevel = n->level - 1;

        subdivideChunkEdge(top, Point2I(pos.x + squareHalfSize, pos.y + squareOneSize), allocChunkEdges);
        subdivideChunkEdge(right, Point2I(pos.x + squareOneSize, pos.y + squareHalfSize), allocChunkEdges);
        subdivideChunkEdge(bottom, Point2I(pos.x + squareHalfSize, pos.y), allocChunkEdges);
        subdivideChunkEdge(left, Point2I(pos.x, pos.y + squareHalfSize), allocChunkEdges);

        // cross edges go top, right, bottom, left
        EdgeParent* crossEdges[4];
        allocRenderEdges(4, crossEdges, allocChunkEdges);
        crossEdges[0]->p1 = top->mp;
        crossEdges[0]->p2 = midPoint;
        crossEdges[1]->p1 = midPoint;
        crossEdges[1]->p2 = right->mp;
        crossEdges[2]->p1 = midPoint;
        crossEdges[2]->p2 = bottom->mp;
        crossEdges[3]->p1 = left->mp;
        crossEdges[3]->p2 = midPoint;

        n->level = nextLevel;
        n->clipFlags = nextClipFlags;

        // Set up data for our sub-squares
        for (S32 i = 1; i < 4; i++)
        {
            n[i].level = nextLevel;
            n[i].clipFlags = nextClipFlags;
            n[i].lightMask = n->lightMask;
            n[i].texAllocated = n->texAllocated;
        }

        // push in reverse order of processing.
        n[3].pos = pos;
        n[3].top = crossEdges[3];
        n[3].right = crossEdges[2];
        n[3].bottom = bottom->e1;
        n[3].left = left->e2;

        n[2].pos.set(pos.x + squareHalfSize, pos.y);
        n[2].top = crossEdges[1];
        n[2].right = right->e2;
        n[2].bottom = bottom->e2;
        n[2].left = crossEdges[2];

        n[1].pos.set(pos.x, pos.y + squareHalfSize);
        n[1].top = top->e1;
        n[1].right = crossEdges[0];
        n[1].bottom = crossEdges[3];
        n[1].left = left->e1;

        n[0].pos.set(pos.x + squareHalfSize, pos.y + squareHalfSize);
        n[0].top = top->e2;
        n[0].right = right->e1;
        n[0].bottom = crossEdges[1];
        n[0].left = crossEdges[0];

        curStackSize += 3;
    }
}

//---------------------------------------------------------------
//---------------------------------------------------------------
// Root block render function
//---------------------------------------------------------------
//---------------------------------------------------------------

void TerrainRender::fixEdge(ChunkEdge* edge, S32 x, S32 y, S32 dx, S32 dy)
{
    PROFILE_START(TR_fixEdge);

    S32 minLevel, maxLevel;
    F32 growFactor;

    if (edge->c1)
    {
        minLevel = edge->c1->subDivLevel;
        maxLevel = edge->c1->subDivLevel;
        growFactor = edge->c1->growFactor;
        if (edge->c2)
        {
            if (edge->c2->subDivLevel < minLevel)
                minLevel = edge->c2->subDivLevel;
            else if (edge->c2->subDivLevel > maxLevel)
            {
                maxLevel = edge->c2->subDivLevel;
                growFactor = edge->c2->growFactor;
            }
            else if (edge->c2->growFactor > growFactor)
                growFactor = edge->c2->growFactor;
        }
    }
    else
    {
        minLevel = maxLevel = edge->c2->subDivLevel;
        growFactor = edge->c2->growFactor;
    }
    if (minLevel == 2)
    {
        edge->pointCount = 0;
        PROFILE_END();
        return;
    }

    // get the mid heights
    EdgePoint* pmid = &edge->pt[1];
    ChunkCornerPoint* p1 = edge->p1;
    ChunkCornerPoint* p2 = edge->p2;

    pmid->x = (p1->x + p2->x) * 0.5;
    pmid->y = (p1->y + p2->y) * 0.5;

    if (maxLevel == 2)
    {
        // pure interp
        pmid->z = (p1->z + p2->z) * 0.5;

        if (minLevel >= 0)
        {
            edge->pointCount = 1;
            PROFILE_END();
            return;
        }
    }
    else
    {
        pmid->z = fixedToFloat(mCurrentBlock->getHeight(x + dx + dx, y + dy + dy));
        if (maxLevel == 1)
            pmid->z = pmid->z + growFactor * (((p1->z + p2->z) * 0.5) - pmid->z);

        if (minLevel >= 0)
        {
            edge->pointCount = 1;
            PROFILE_END();
            return;
        }
    }

    // last case - minLevel == -1, midPoint calc'd
    edge->pointCount = 3;
    EdgePoint* pm1 = &edge->pt[0];
    EdgePoint* pm2 = &edge->pt[2];

    pm1->x = (p1->x + pmid->x) * 0.5;
    pm1->y = (p1->y + pmid->y) * 0.5;
    pm2->x = (p2->x + pmid->x) * 0.5;
    pm2->y = (p2->y + pmid->y) * 0.5;

    if (maxLevel != -1)
    {
        pm1->z = (p1->z + pmid->z) * 0.5;
        pm2->z = (p2->z + pmid->z) * 0.5;
        PROFILE_END();
        return;
    }

    // compute the real deals:
    pm1->z = fixedToFloat(mCurrentBlock->getHeight(x + dx, y + dy));
    pm2->z = fixedToFloat(mCurrentBlock->getHeight(x + dx + dx + dx, y + dy + dy + dy));

    if (growFactor)
    {
        pm1->z = pm1->z + growFactor * (((p1->z + pmid->z) * 0.5) - pm1->z);
        pm2->z = pm2->z + growFactor * (((p2->z + pmid->z) * 0.5) - pm2->z);
    }
    PROFILE_END();
}

/*
void buildLightTri(LightTriangle* pTri, TerrLightInfo* pInfo)
{
// Get the plane normal
Point3F normal;
mCross((pTri->point1 - pTri->point2), (pTri->point3 - pTri->point2), &normal);
if (normal.lenSquared() < 1e-7)
{
pTri->flags = 0;
return;
}


PlaneF plane(pTri->point2, normal);  // Assumes that mPlane.h normalizes incoming point

Point3F centerPoint;
F32 d = plane.distToPlane(pInfo->pos);
centerPoint = pInfo->pos - plane * d;
d = mFabs(d);
if (d >= pInfo->radius) {
pTri->flags = 0;
return;
}

F32 mr = mSqrt(pInfo->radiusSquared - d*d);

Point3F normalS;
Point3F normalT;
mCross(plane, Point3F(0, 1, 0), &normalS);
mCross(plane, normalS, &normalT);
PlaneF splane(centerPoint, normalS); // Assumes that mPlane.h normalizes incoming point
PlaneF tplane(centerPoint, normalT); // Assumes that mPlane.h normalizes incoming point

pTri->color.red   = pInfo->r;
pTri->color.green = pInfo->g;
pTri->color.blue  = pInfo->b;
pTri->color.alpha = (pInfo->radius - d) / pInfo->radius;

pTri->texco1.set(((splane.distToPlane(pTri->point1) / mr) + 1.0) / 2.0,
((tplane.distToPlane(pTri->point1) / mr) + 1.0) / 2.0);
pTri->texco2.set(((splane.distToPlane(pTri->point2) / mr) + 1.0) / 2.0,
((tplane.distToPlane(pTri->point2) / mr) + 1.0) / 2.0);
pTri->texco3.set(((splane.distToPlane(pTri->point3) / mr) + 1.0) / 2.0,
((tplane.distToPlane(pTri->point3) / mr) + 1.0) / 2.0);

pTri->flags = 1;
}
*/

void TerrainRender::renderChunkCommander(EmitChunk* chunk)
{
    // Emit our points
    U32 ll = TerrBatch::mCurVertex;

    for (U32 y = 0; y <= 64; y += 4)                   // 16 steps
        for (U32 x = (y & 4) ? 4 : 0; x <= 64; x += 8)  // 8 steps
            TerrBatch::emit(chunk->x + x, chunk->y + y); // 128 vertices

      // Emit a mesh overlaying 'em (768 indices)
    for (U32 y = 0; y < 8; y++)
    {
        for (U32 x = 0; x < 8; x++)
        {
            TerrBatch::emitTriangle(ll + 9, ll, ll + 17);
            TerrBatch::emitTriangle(ll + 9, ll + 17, ll + 18);
            TerrBatch::emitTriangle(ll + 9, ll + 18, ll + 1);
            TerrBatch::emitTriangle(ll + 9, ll + 1, ll);
            ll++;
        }
        ll += 9;
    }

}

void TerrainRender::renderChunkNormal(EmitChunk* chunk)
{
    PROFILE_START(TerrainRender_renderChunkNormal);

    PROFILE_START(TR_chunkFormer);
    // Emits a max of 96 indices
    // Emits a max of 25 vertices
    ChunkEdge* e0 = chunk->edge[0];
    ChunkEdge* e1 = chunk->edge[1];
    ChunkEdge* e2 = chunk->edge[2];
    ChunkEdge* e3 = chunk->edge[3];

    PROFILE_START(TR_e0fix);
    if (e0->xfIndex != TerrBatch::mCurXF)
    {
        if (!e0->xfIndex)
            fixEdge(e0, chunk->x, chunk->y + 4, 1, 0);
        TerrBatch::emit(*e0);
    }
    PROFILE_END();
    PROFILE_START(TR_e1fix);
    if (e1->xfIndex != TerrBatch::mCurXF)
    {
        if (!e1->xfIndex)
            fixEdge(e1, chunk->x + 4, chunk->y + 4, 0, -1);
        TerrBatch::emit(*e1);
    }
    PROFILE_END();
    PROFILE_START(TR_e2fix);
    if (e2->xfIndex != TerrBatch::mCurXF)
    {
        if (!e2->xfIndex)
            fixEdge(e2, chunk->x, chunk->y, 1, 0);
        TerrBatch::emit(*e2);
    }
    PROFILE_END();
    PROFILE_START(TR_e3fix);
    if (e3->xfIndex != TerrBatch::mCurXF)
    {
        if (!e3->xfIndex)
            fixEdge(e3, chunk->x, chunk->y + 4, 0, -1);
        TerrBatch::emit(*e3);
    }
    PROFILE_END();

    // Edges above, give us 12 verts
    // Corners below are 4 more
    PROFILE_START(TR_corners);

    U16 p0 = TerrBatch::emit(e0->p1);
    U16 p1 = TerrBatch::emit(e0->p2);
    U16 p2 = TerrBatch::emit(e2->p2);
    U16 p3 = TerrBatch::emit(e2->p1);

    PROFILE_END();

    PROFILE_START(TR_center);
    // build the interior points (one more vert):
    U16 ip0 = TerrBatch::emit(chunk->x + 2, chunk->y + 2);
    F32 growFactor = chunk->growFactor;

    PROFILE_END();

    PROFILE_END();

    // So now we have a total of 17 verts

    PROFILE_START(TR_chunkLatter);
    if (chunk->subDivLevel >= 1)
    {
        PROFILE_START(TR_subDivLvl1Plus);
        // just emit the fan for the whole square:
        S32 i;

        if (e0->pointCount)
        {
            TerrBatch::emitTriangle(ip0, p0, e0->pointIndex);  // 3 indices
            for (i = 1; i < e0->pointCount; i++)
                TerrBatch::emitFanStep(e0->pointIndex + i);     // 9 i
            TerrBatch::emitFanStep(p1);                        // 3 i
        }
        else {
            TerrBatch::emitTriangle(ip0, p0, p1);
        }

        for (i = 0; i < e1->pointCount; i++)
            TerrBatch::emitFanStep(e1->pointIndex + i);        // 9 indices

        TerrBatch::emitFanStep(p2);                           // 3 indices
        for (i = e2->pointCount - 1; i >= 0; i--)
            TerrBatch::emitFanStep(e2->pointIndex + i);        // 6 i

        TerrBatch::emitFanStep(p3);                           // 3 i
        for (i = e3->pointCount - 1; i >= 0; i--)
            TerrBatch::emitFanStep(e3->pointIndex + i);        // 6 i

        TerrBatch::emitFanStep(p0);                           // 3 i

        // Total indices for this path:                          42 indices
        PROFILE_END();
    }
    else
    {
        if (chunk->subDivLevel == 0)
        {
            PROFILE_START(TR_subDivLvl0);
            // Add yet four verts more to the list! (now we're at 21)
            U32 ip1 = TerrBatch::emitInterp(p0, ip0, chunk->x + 1, chunk->y + 3, growFactor);
            U32 ip2 = TerrBatch::emitInterp(p1, ip0, chunk->x + 3, chunk->y + 3, growFactor);
            U32 ip3 = TerrBatch::emitInterp(p2, ip0, chunk->x + 3, chunk->y + 1, growFactor);
            U32 ip4 = TerrBatch::emitInterp(p3, ip0, chunk->x + 1, chunk->y + 1, growFactor);

            // emit the 4 fans:

            U32 indexStart;

            if ((chunk->emptyFlags & CornerEmpty_0_1) != CornerEmpty_0_1)
            {

                TerrBatch::emitTriangle(ip1, p0, e0->pointIndex);        // 3

                if (e0->pointCount == 3)
                    TerrBatch::emitFanStep(e0->pointIndex + 1);          // 3

                TerrBatch::emitFanStep(ip0);                            // 3

                if (e3->pointCount == 1)
                    TerrBatch::emitFanStep(e3->pointIndex);
                else
                {
                    TerrBatch::emitFanStep(e3->pointIndex + 1);        // 3
                    TerrBatch::emitFanStep(e3->pointIndex);            // 3
                }

                TerrBatch::emitFanStep(p0);                           // 3

                // Total emitted indices                                    18
            }

            if ((chunk->emptyFlags & CornerEmpty_1_1) != CornerEmpty_1_1)
            {
                TerrBatch::emitTriangle(ip2, p1, e1->pointIndex);

                if (e1->pointCount == 3)
                    TerrBatch::emitFanStep(e1->pointIndex + 1);

                TerrBatch::emitFanStep(ip0);

                if (e0->pointCount == 1)
                    TerrBatch::emitFanStep(e0->pointIndex);
                else
                {
                    TerrBatch::emitFanStep(e0->pointIndex + 1);
                    TerrBatch::emitFanStep(e0->pointIndex + 2);
                }
                TerrBatch::emitFanStep(p1);
            }

            if ((chunk->emptyFlags & CornerEmpty_1_0) != CornerEmpty_1_0)
            {

                if (e2->pointCount == 1)
                    TerrBatch::emitTriangle(ip3, p2, e2->pointIndex);
                else
                {
                    TerrBatch::emitTriangle(ip3, p2, e2->pointIndex + 2);
                    TerrBatch::emitFanStep(e2->pointIndex + 1);
                }

                TerrBatch::emitFanStep(ip0);

                if (e1->pointCount == 1)
                    TerrBatch::emitFanStep(e1->pointIndex);
                else
                {
                    TerrBatch::emitFanStep(e1->pointIndex + 1);
                    TerrBatch::emitFanStep(e1->pointIndex + 2);
                }
                TerrBatch::emitFanStep(p2);
            }

            if ((chunk->emptyFlags & CornerEmpty_0_0) != CornerEmpty_0_0)
            {

                if (e3->pointCount == 1)
                    TerrBatch::emitTriangle(ip4, p3, e3->pointIndex);
                else
                {
                    TerrBatch::emitTriangle(ip4, p3, e3->pointIndex + 2);
                    TerrBatch::emitFanStep(e3->pointIndex + 1);
                }
                TerrBatch::emitFanStep(ip0);

                if (e2->pointCount == 1)
                    TerrBatch::emitFanStep(e2->pointIndex);
                else
                {
                    TerrBatch::emitFanStep(e2->pointIndex + 1);
                    TerrBatch::emitFanStep(e2->pointIndex);
                }
                TerrBatch::emitFanStep(p3);
            }

            // Four fans of 18 indices =                                  72 indices

            PROFILE_END();
        }
        else
        {
            PROFILE_START(TR_levelMinusOne);
            // subDiv == -1
            U32 ip1 = TerrBatch::emit(chunk->x + 1, chunk->y + 3);
            U32 ip2 = TerrBatch::emit(chunk->x + 3, chunk->y + 3);
            U32 ip3 = TerrBatch::emit(chunk->x + 3, chunk->y + 1);
            U32 ip4 = TerrBatch::emit(chunk->x + 1, chunk->y + 1);
            U32 ip5 = TerrBatch::emitInterp(e0->pointIndex + 1, ip0, chunk->x + 2, chunk->y + 3, growFactor);
            U32 ip6 = TerrBatch::emitInterp(e1->pointIndex + 1, ip0, chunk->x + 3, chunk->y + 2, growFactor);
            U32 ip7 = TerrBatch::emitInterp(e2->pointIndex + 1, ip0, chunk->x + 2, chunk->y + 1, growFactor);
            U32 ip8 = TerrBatch::emitInterp(e3->pointIndex + 1, ip0, chunk->x + 1, chunk->y + 2, growFactor);

            // Or eight more points, in which case we hit 25 verts total..

            // now do the squares:
            U16* ib;

            if (chunk->emptyFlags & CornerEmpty_0_1)
            {
                if ((chunk->emptyFlags & CornerEmpty_0_1) != CornerEmpty_0_1)
                {
                    if (!(chunk->emptyFlags & SquareEmpty_0_3))
                    {
                        TerrBatch::emitTriangle(ip1, e3->pointIndex, p0);
                        TerrBatch::emitTriangle(ip1, p0, e0->pointIndex);
                    }
                    if (!(chunk->emptyFlags & SquareEmpty_1_3))
                    {
                        TerrBatch::emitTriangle(ip1, e0->pointIndex, e0->pointIndex + 1);
                        TerrBatch::emitTriangle(ip1, e0->pointIndex + 1, ip5);
                    }
                    if (!(chunk->emptyFlags & SquareEmpty_1_2))
                    {
                        TerrBatch::emitTriangle(ip1, ip5, ip0);
                        TerrBatch::emitTriangle(ip1, ip0, ip8);
                    }
                    if (!(chunk->emptyFlags & SquareEmpty_0_2))
                    {
                        TerrBatch::emitTriangle(ip1, ip8, e3->pointIndex + 1);
                        TerrBatch::emitTriangle(ip1, e3->pointIndex + 1, e3->pointIndex);
                    }
                }
                // 24 indices on this path ^^^
            }
            else
            {
                TerrBatch::emitTriangle(ip1, p0, e0->pointIndex);
                TerrBatch::emitFanStep(e0->pointIndex + 1);
                TerrBatch::emitFanStep(ip5);
                TerrBatch::emitFanStep(ip0);
                TerrBatch::emitFanStep(ip8);
                TerrBatch::emitFanStep(e3->pointIndex + 1);
                TerrBatch::emitFanStep(e3->pointIndex);
                TerrBatch::emitFanStep(p0);
                // And 24 here.
            }

            if (chunk->emptyFlags & CornerEmpty_1_1)
            {
                if ((chunk->emptyFlags & CornerEmpty_1_1) != CornerEmpty_1_1)
                {

                    if (!(chunk->emptyFlags & SquareEmpty_3_3))
                    {
                        TerrBatch::emitTriangle(ip2, e0->pointIndex + 2, p1);
                        TerrBatch::emitTriangle(ip2, p1, e1->pointIndex);
                    }
                    if (!(chunk->emptyFlags & SquareEmpty_3_2))
                    {
                        TerrBatch::emitTriangle(ip2, e1->pointIndex, e1->pointIndex + 1);
                        TerrBatch::emitTriangle(ip2, e1->pointIndex + 1, ip6);
                    }
                    if (!(chunk->emptyFlags & SquareEmpty_2_2))
                    {
                        TerrBatch::emitTriangle(ip2, ip6, ip0);
                        TerrBatch::emitTriangle(ip2, ip0, ip5);
                    }
                    if (!(chunk->emptyFlags & SquareEmpty_2_3))
                    {
                        TerrBatch::emitTriangle(ip2, ip5, e0->pointIndex + 1);
                        TerrBatch::emitTriangle(ip2, e0->pointIndex + 1, e0->pointIndex + 2);
                    }
                }
            }
            else
            {
                TerrBatch::emitTriangle(ip2, p1, e1->pointIndex);
                TerrBatch::emitFanStep(e1->pointIndex + 1);
                TerrBatch::emitFanStep(ip6);
                TerrBatch::emitFanStep(ip0);
                TerrBatch::emitFanStep(ip5);
                TerrBatch::emitFanStep(e0->pointIndex + 1);
                TerrBatch::emitFanStep(e0->pointIndex + 2);
                TerrBatch::emitFanStep(p1);
            }

            if (chunk->emptyFlags & CornerEmpty_1_0)
            {
                if ((chunk->emptyFlags & CornerEmpty_1_0) != CornerEmpty_1_0)
                {

                    if (!(chunk->emptyFlags & SquareEmpty_3_0))
                    {
                        TerrBatch::emitTriangle(ip3, e1->pointIndex + 2, p2);
                        TerrBatch::emitTriangle(ip3, p2, e2->pointIndex + 2);
                    }
                    if (!(chunk->emptyFlags & SquareEmpty_2_0))
                    {
                        TerrBatch::emitTriangle(ip3, e2->pointIndex + 2, e2->pointIndex + 1);
                        TerrBatch::emitTriangle(ip3, e2->pointIndex + 1, ip7);
                    }
                    if (!(chunk->emptyFlags & SquareEmpty_2_1))
                    {
                        TerrBatch::emitTriangle(ip3, ip7, ip0);
                        TerrBatch::emitTriangle(ip3, ip0, ip6);
                    }
                    if (!(chunk->emptyFlags & SquareEmpty_3_1))
                    {
                        TerrBatch::emitTriangle(ip3, ip6, e1->pointIndex + 1);
                        TerrBatch::emitTriangle(ip3, e1->pointIndex + 1, e1->pointIndex + 2);
                    }
                }
            }
            else
            {
                TerrBatch::emitTriangle(ip3, p2, e2->pointIndex + 2);
                TerrBatch::emitFanStep(e2->pointIndex + 1);
                TerrBatch::emitFanStep(ip7);
                TerrBatch::emitFanStep(ip0);
                TerrBatch::emitFanStep(ip6);
                TerrBatch::emitFanStep(e1->pointIndex + 1);
                TerrBatch::emitFanStep(e1->pointIndex + 2);
                TerrBatch::emitFanStep(p2);

            }

            if (chunk->emptyFlags & CornerEmpty_0_0)
            {
                if ((chunk->emptyFlags & CornerEmpty_0_0) != CornerEmpty_0_0)
                {
                    if (!(chunk->emptyFlags & SquareEmpty_0_0))
                    {
                        TerrBatch::emitTriangle(ip4, e2->pointIndex, p3);
                        TerrBatch::emitTriangle(ip4, p3, e3->pointIndex + 2);
                    }
                    if (!(chunk->emptyFlags & SquareEmpty_0_1))
                    {
                        TerrBatch::emitTriangle(ip4, e3->pointIndex + 2, e3->pointIndex + 1);
                        TerrBatch::emitTriangle(ip4, e3->pointIndex + 1, ip8);
                    }
                    if (!(chunk->emptyFlags & SquareEmpty_1_1))
                    {
                        TerrBatch::emitTriangle(ip4, ip8, ip0);
                        TerrBatch::emitTriangle(ip4, ip0, ip7);
                    }
                    if (!(chunk->emptyFlags & SquareEmpty_1_0))
                    {
                        TerrBatch::emitTriangle(ip4, ip7, e2->pointIndex + 1);
                        TerrBatch::emitTriangle(ip4, e2->pointIndex + 1, e2->pointIndex);
                    }
                }
            }
            else
            {
                TerrBatch::emitTriangle(ip4, p3, e3->pointIndex + 2);
                TerrBatch::emitFanStep(e3->pointIndex + 1);
                TerrBatch::emitFanStep(ip8);
                TerrBatch::emitFanStep(ip0);
                TerrBatch::emitFanStep(ip7);
                TerrBatch::emitFanStep(e2->pointIndex + 1);
                TerrBatch::emitFanStep(e2->pointIndex);
                TerrBatch::emitFanStep(p3);
            }


            PROFILE_END();
        }

        // 4 chunks with 24 indices each:                        96 indices
    }

    PROFILE_END();
    PROFILE_END();
}


void TerrainRender::renderBlock(TerrainBlock* block, SceneState* state, MatInstance* m, SceneGraphData& sgData)
{
    PROFILE_START(TerrainRender);

    PROFILE_START(TerrainRenderSetup);
    U32 storedWaterMark = FrameAllocator::getWaterMark();

    {
        GFX_Canonizer("TerrainRender::renderBlock", __FILE__, __LINE__);
        mFrameIndex++;
        mSceneState = state;
        mFarDistance = state->getVisibleDistance();

        mCameraToObject = GFX->getWorldMatrix();
        mCameraToObject.inverse();
        mCameraToObject.getColumn(3, &mCamPos);

        mCurrentBlock = block;
        mSquareSize = block->getSquareSize();

        // compute pixelError
        if (mScreenError >= 0.001)
            mPixelError = 1 / GFX->projectRadius(mScreenError, 1);
        else
            mPixelError = 0.000001;

        buildClippingPlanes(state->mFlipCull);
        buildLightArray();

        F32 worldToScreenScale = GFX->projectRadius(1, 1);
        F32 blockSize = mSquareSize * TerrainBlock::BlockSquareWidth;

        // Calculate the potentially viewable area...
        S32 xStart = (S32)mFloor((mCamPos.x - mFarDistance) / blockSize);
        S32 xEnd = (S32)mCeil((mCamPos.x + mFarDistance) / blockSize);
        S32 yStart = (S32)mFloor((mCamPos.y - mFarDistance) / blockSize);
        S32 yEnd = (S32)mCeil((mCamPos.y + mFarDistance) / blockSize);
        S32 xExt = (S32)(xEnd - xStart);
        S32 yExt = (S32)(yEnd - yStart);

        PROFILE_END();
        PROFILE_START(TerrainRenderRecurse);

        // Come up with a dummy height to work with...
        F32 height = fixedToFloat(block->getHeight(0, 0));

        EdgeParent** bottomEdges = (EdgeParent**)FrameAllocator::alloc(sizeof(ChunkScanEdge*) * xExt);

        TerrainRender::allocRenderEdges(xExt, bottomEdges, false);

        ChunkCornerPoint* prevCorner = TerrainRender::allocInitialPoint(
            Point3F(
                xStart * blockSize,
                yStart * blockSize,
                height
            )
        );

        mTerrainOffset.set(xStart * TerrainBlock::BlockSquareWidth,
            yStart * TerrainBlock::BlockSquareWidth);

        for (S32 x = 0; x < xExt; x++)
        {
            bottomEdges[x]->p1 = prevCorner;

            prevCorner = TerrainRender::allocInitialPoint(
                Point3F(
                    (xStart + x) * blockSize,
                    (yStart)*blockSize,
                    height
                )
            );

            bottomEdges[x]->p2 = prevCorner;
        }

        for (S32 y = 0; y < yExt; y++)
        {
            // allocate the left edge:
            EdgeParent* left;
            TerrainRender::allocRenderEdges(1, &left, false);
            left->p1 = TerrainRender::allocInitialPoint(Point3F(xStart * blockSize, (yStart + y + 1) * blockSize, height));
            left->p2 = bottomEdges[0]->p1;

            for (S32 x = 0; x < xExt; x++)
            {
                // Allocate the right,...
                EdgeParent* right;
                TerrainRender::allocRenderEdges(1, &right, false);
                right->p1 = TerrainRender::allocInitialPoint(
                    Point3F(
                        (xStart + x + 1) * blockSize,
                        (yStart + y + 1) * blockSize,
                        height
                    )
                );
                right->p2 = bottomEdges[x]->p2;

                // Allocate the top...
                EdgeParent* top;
                TerrainRender::allocRenderEdges(1, &top, false);
                top->p1 = left->p1;
                top->p2 = right->p1;

                // Process this block...

                //    -  set up for processing the block...
                mBlockOffset.set(x << TerrainBlock::BlockShift,
                    y << TerrainBlock::BlockShift);

                mBlockPos.set((xStart + x) * blockSize,
                    (yStart + y) * blockSize);

                sgFogRejectedBlocks = 0;

                //    - Do it...
                TerrainRender::processBlock(state, top, right, bottomEdges[x], left);

                // Clean up...
                left = right;
                bottomEdges[x] = top;
            }
        }

        PROFILE_END();

    }

    PROFILE_START(TerrainRenderEmit);

    // Set up for the render...
    AllocatedTexture::trimFreeList();
    sgCurrLightTris = NULL;
    AllocatedTexture* step;

    U32 emitCount = 0;
    Vector<LightingChunkInfo> geomchunks;

    // light map used in final render, not in blend...
    GFX->setTextureStageAlphaOp(2, GFXTOPModulate);
    GFX->setTexture(2, block->lightMapTex);
    // light map uv gen...
    Point4F terrainsize((1.0 / (block->getSquareSize() * TerrainBlock::BlockSize)),
        0, 0, 0);
    GFX->setVertexShaderConstF(54, (float*)terrainsize, 1);

    while (step = TerrTexture::getNextFrameTexture())
    {
        PROFILE_START(TerrainRenderStep);

        EmitChunk* sq;

        for (sq = step->emitList; sq; sq = sq->next)
        {
            TerrBatch::begin();
            PROFILE_START(TerrainRenderStepChunk);

            // Emit the appropriate geometry for our rendering mode...
            if (mRenderingCommander)
                renderChunkCommander(sq);
            else
                renderChunkNormal(sq);

            geomchunks.increment();
            geomchunks.last().chunk = sq;
            geomchunks.last().texture = &step->handle;
            geomchunks.last().texGenS = TerrBatch::getTexGenS();
            geomchunks.last().texGenT = TerrBatch::getTexGenT();

            emitCount++;

            PROFILE_END();
            TerrBatch::end(m, sgData, step->handle);
        }

        PROFILE_END();
    }

    // force batch...
    TerrBatch::end(m, sgData, NULL, true);

    GFX->setTextureStageAlphaOp(2, GFXTOPDisable);


#ifdef TEMP_ENABLE_TERRAIN_RENDER
    // render dynamic light materials...
    if (m && (!mRenderingCommander))
    {
        for (U32 i = 0; i < mDynamicLightCount; i++)
        {
            TerrLightInfo& tlight = mTerrainLights[i];
            LightInfo* light = tlight.light;
            // get the best dynamic lighting material for this light...
            MatInstance* dmat = MatInstance::getDynamicLightingMaterial(m, light, false);
            if (!dmat)
                continue;
            // get the model...
            sgLightingModel& lightingmodel = sgLightingModelManager::sgGetLightingModel(
                light->sgLightingModelName);
            lightingmodel.sgSetState(light);
            // get the info...
            F32 maxrad = lightingmodel.sgGetMaxRadius(true);
            light->sgTempModelInfo[0] = 0.5f / maxrad;

            // get the dynamic lighting texture...
            if ((light->mType == LightInfo::Spot) || (light->mType == LightInfo::SGStaticSpot))
                sgData.dynamicLight = lightingmodel.sgGetDynamicLightingTextureSpot();
            else
                sgData.dynamicLight = lightingmodel.sgGetDynamicLightingTextureOmni();
            // reset the model...
            lightingmodel.sgResetState();
            dMemcpy(&sgData.light, light, sizeof(LightInfo));

            bool rendered = false;
            for (U32 c = 0; c < geomchunks.size(); c++)
            {
                EmitChunk* sq = geomchunks[c].chunk;
                if (!(sq->lightMask & (1 << i)))
                    continue;

                TerrBatch::setTexGen(geomchunks[c].texGenS,
                    geomchunks[c].texGenT);

                TerrBatch::begin();
                PROFILE_START(TerrainRenderDynamicStepChunk);

                renderChunkNormal(sq);
                emitCount++;

                PROFILE_END();
                TerrBatch::end(dmat, sgData, (*geomchunks[c].texture));

                rendered = true;
            }

            // force batch...
            if (rendered)
                TerrBatch::end(dmat, sgData, NULL, true, true);
        }
    }
#endif

    //   Con::printf("Emitted %d chunks", emitCount);

       // Force the batcher to emit all its geometry
       //TerrBatch::end(m, sgData, NULL, true);

    FrameAllocator::setWaterMark(storedWaterMark);

    PROFILE_END();
    PROFILE_END();
}



//---------------------------------------------------------------
