//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _BVQUADTREE_H_
#define _BVQUADTREE_H_

//#define BV_QUADTREE_DEBUG

#ifndef _TVECTOR_H_
#include "core/tVector.h"
#endif
#ifndef _BITVECTOR_H_
#include "core/bitVector.h"
#endif
#ifndef _MPOINT_H_
#include "math/mPoint.h"
#endif

/// A bit vector quad tree, used to track flags for the terrain.
class BVQuadTree
{
protected:
    VectorPtr<BitVector*> mQTHierarchy;
    U32 mResolution;
public:
    BVQuadTree(BitVector* bv = NULL);
    ~BVQuadTree();

    bool isSet(const Point2F& pos, S32 level) const;
    bool isClear(const Point2F& pos, S32 level) const;

    void init(const BitVector& bv);
#ifdef BV_QUADTREE_DEBUG
    void dump() const;
#endif
    U32 countLevels() const { return(mQTHierarchy.size()); }
protected:
    void buildHierarchy(U32 level);
private:
    BVQuadTree(const BVQuadTree&);
    BVQuadTree& operator=(const BVQuadTree&);
};

#endif
