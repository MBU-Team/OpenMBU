//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "terrain/bvQuadTree.h"
#include "console/console.h"

BVQuadTree::BVQuadTree(BitVector* bv)
{
    if (bv != NULL)
        init(*bv);
    else
    {
        BitVector localBV;
        localBV.setSize(4);
        localBV.set();
        init(localBV);
    }
}

BVQuadTree::~BVQuadTree()
{
    while (mQTHierarchy.size() > 0)
    {
        delete mQTHierarchy.last();
        mQTHierarchy.pop_back();
    }
}

bool BVQuadTree::isSet(const Point2F& pos, S32 level) const
{
    AssertFatal(pos.x >= 0. && pos.x <= 1., "BVQuadTree::isSet: x must be in range [0,1]");
    AssertFatal(pos.y >= 0. && pos.y <= 1., "BVQuadTree::isSet: y must be in range [0,1]");
    AssertFatal(level >= 0, "BVQuadTree:isSet: level must be greater than or equal to zero");

    if (level >= mQTHierarchy.size())
        // force to be within resolution of QT
        level = mQTHierarchy.size() - 1;
    if (level < 0)
        level = 0;

    F32 dimension = F32(1 << level);
    U32 offset = U32((F32(U32(pos.y * dimension)) + pos.x) * dimension);
    return(mQTHierarchy[level]->test(offset));
}

bool BVQuadTree::isClear(const Point2F& pos, S32 level) const
{
    AssertFatal(pos.x >= 0. && pos.x <= 1., "BVQuadTree::isClear: x must be in range [0,1]");
    AssertFatal(pos.y >= 0. && pos.y <= 1., "BVQuadTree::isClear: y must be in range [0,1]");
    AssertFatal(level >= 0, "BVQuadTree:isClear: level must be greater than or equal to zero");

    if (level >= mQTHierarchy.size())
        // force to be within resolution of QT
        level = mQTHierarchy.size() - 1;
    if (level < 0)
        level = 0;

    F32 dimension = F32(1 << level);
    U32 offset = U32((F32(U32(pos.y * dimension)) + pos.x) * dimension);
    return(mQTHierarchy[level]->test(offset) == false);
}

/* Initialize the quadtree with the provided bit vector. Note that the bit vector
 * denotes data at each corner of the quadtree cell. Hence the dimension for the
 * deepest quadtree level must be 1 less than that of the bit vector.
 */
void BVQuadTree::init(const BitVector& bv)
{
    while (mQTHierarchy.size() > 0)
    {
        delete mQTHierarchy.last();
        mQTHierarchy.pop_back();
    }

    // get the width/height of the square bit vector
    U32 bvDim = (U32)mSqrt((F32)bv.getSize());
    U32 qtDim = bvDim - 1;                                // here's where we correct dimension...

    AssertFatal(((mSqrt((F32)bv.getSize()) - 1) == (F32)qtDim) && (isPow2(qtDim) == true), "BVQuadTree::init: bit vector size must be power of 4");

    // find the power of two we're starting at
    mResolution = qtDim;
    U32 level = 0;
    while ((1 << (level + 1)) <= qtDim)
        level++;
    BitVector* initBV = new BitVector;

    AssertFatal(initBV != NULL, "BVQuadTree::init: failed to allocate highest detail bit vector");

    initBV->setSize(qtDim * qtDim);
    initBV->clear();
    for (S32 i = 0; i < qtDim; i++)
        for (S32 j = 0; j < qtDim; j++)
        {
            S32 k = i * bvDim + j;
            if (bv.test(k) || bv.test(k + 1) || bv.test(k + bvDim) || bv.test(k + bvDim + 1))
                initBV->set(i * qtDim + j);
        }
    mQTHierarchy.push_back(initBV);
    if (level > 0)
        buildHierarchy(level - 1);
}

#ifdef BV_QUADTREE_DEBUG
void BVQuadTree::dump() const
{
    char str[256];
    U32 strlen;
    for (U32 i = 0; i < mQTHierarchy.size(); i++)
    {
        U32 dimension = 1 << i;
        Con::printf("level %d:", i);
        for (U32 y = 0; y < dimension; y++)
        {
            U32 yOffset = y * dimension;
            str[0] = '\0';
            for (U32 x = 0; x < dimension; x++)
            {
                U32 offset = yOffset + x;
                strlen = dStrlen(str);
                if (strlen < 252)
                    dSprintf(str + strlen, 256 - strlen, mQTHierarchy[i]->isSet(offset) ? "1  " : "0  ");
                else
                {
                    dSprintf(str + strlen, 256 - strlen, "...");
                    break;
                }
            }
            Con::printf("%s", str);
        }
    }
}
#endif

void BVQuadTree::buildHierarchy(U32 level)
{
    BitVector* priorBV = mQTHierarchy.first();
    BitVector* levelBV = new BitVector;

    AssertFatal(levelBV != NULL, "BVQuadTree::buildHierarchy: failed to allocate bit vector");

    U32 levelDim = (1 << level);
    levelBV->setSize(levelDim * levelDim);
    levelBV->clear();
    U32 yOffset, offset, yPriorOffset, priorOffset;
    // COULD THIS BE DONE WITH A SINGLE LOOP?
    for (U32 y = 0; y < levelDim; y++)
    {
        yOffset = y * levelDim;
        yPriorOffset = yOffset << 2;
        for (U32 x = 0; x < levelDim; x++)
        {
            offset = yOffset + x;
            priorOffset = yPriorOffset + (x << 1);
            if (priorBV->test(priorOffset) || priorBV->test(priorOffset + 1) ||
                priorBV->test(priorOffset + (levelDim << 1)) ||
                priorBV->test(priorOffset + (levelDim << 1) + 1))
                levelBV->set(offset);
        }
    }
    mQTHierarchy.push_front(levelBV);
    if (level > 0)
        buildHierarchy(level - 1);
}
