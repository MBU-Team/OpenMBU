//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _ATLASINSTANCEGEOMTOC_H_
#define _ATLASINSTANCEGEOMTOC_H_

#include "atlas/runtime/atlasInstanceTOC.h"
#include "atlas/resource/atlasResourceGeomTOC.h"
#include "util/frustrumCuller.h"

class RayInfo;
class Convex;
class AtlasConfigChunk;
class AtlasInstance2;
class AtlasClipMapBatcher;

/// Atlas instance stub subclass for geometry data.
///
/// @ingroup AtlasRuntime
class AtlasInstanceGeomStub : public AtlasInstanceStub<AtlasGeomChunk, AtlasInstanceGeomStub>
{
public:
    AtlasInstanceGeomStub()
    {
        mBounds.min.set(0, 0, 0);
        mBounds.max.set(0, 0, 0);
        mLastMorph = 0;
    }

    /// Bounds of this chunk.
    Box3F mBounds;

    /// Track last morph parameter to help ensure smooth LOD transitions.
    F32 mLastMorph;
};

/// Atlas instance TOC for geometry.
///
/// Most of our rendering and LOD implementation for Atlas lives in this class.
/// 
/// @ingroup AtlasRuntime
class AtlasInstanceGeomTOC : public AtlasInstanceTOC<AtlasInstanceGeomStub, AtlasResourceGeomTOC>
{
    typedef AtlasInstanceTOC<AtlasInstanceGeomStub, AtlasResourceGeomTOC> Parent;

    void doSplit(StubType* s);
    bool attemptSplit(StubType* s);
    void warmUpData(StubType* s, F32 priority);

    inline U16 computeLod(const Box3F& bounds) const
    {
        const F32 d = getMax(.00001f, mCuller.getBoxDistance(bounds) / mResourceTOC->mDistance_LODMax);
        return mClamp(((mTreeDepth << 8) - 1) - S32(d * 256.f), 0, 0xFFFF);
    }

    void updateBounds(AtlasInstanceGeomStub* stub, U32 i, S32 x, S32 y)
    {
        // This seems silly...
        stub->mBounds = getResourceStub(stub)->mBounds;

        // Assert children bounds are valid.
        if (!stub->hasChildren())
            return;

        for (S32 j = 0; j < 4; j++)
        {
            AssertWarn(stub->mBounds.isContained(stub->mChildren[j]->mBounds),
                avar("AtlasInstanceGeomTOC::updateBounds - invalid child bounds! %d@(%d, %d)", i, x, y));
        }
    }

    struct StackNode
    {
        U32 clipMask;
        StubType* stub;
    };

    /// @see loadCollisionLeafChunks()
    bool mLeafCollisionMode;

    /// Used to perform frustrum clipping.
    FrustrumCuller mCuller;

public:
    class RenderedStub
    {
    public:
        AtlasResourceGeomTOC::StubType* stub;
        GFXTexHandle texture;

        RenderedStub()
        {
            dMemset(this, 0, sizeof(RenderedStub));
        }
    };

    AtlasInstanceGeomTOC()
    {
        mIsQuadtree = true;
        mLeafCollisionMode = false;
    }

    ~AtlasInstanceGeomTOC()
    {
    }

    void clear(const F32 deltaT);
    void processLOD(SceneState* s);
    void renderBounds(S32 debugMode);
    void renderGeometry(AtlasClipMapBatcher* cmb);
    void renderGeometry(Vector<AtlasInstanceGeomTOC::RenderedStub>& renderedStubs);

    virtual void requestLoad(AtlasInstanceGeomStub* stub, U32 reason, F32 priority);
    virtual void cancelLoadRequest(AtlasInstanceGeomStub* stub, U32 reason);

    /// Load leaf chunks to fulfill collision requests. This also jiggers our
    /// load semantics so we don't unload them inadvertently.
    void loadCollisionLeafChunks();

    /// Cast a ray. (Done in file space).
    bool castRay(const Point3F& start, const Point3F& end, RayInfo* info, bool emptyCollide);

    /// This is a unified internal collision interface for Atlas.
    ///
    /// We end up recursing by bounding box down into the chunks, so it makes more
    /// sense to have one set of functions that take different terminal actions
    /// rather than having to live with two sets of code. AtlasInstance2::buildConvex()
    /// and AtlasInstance2::buildPolyList() methods simply wrap this, and it fills in
    /// what is appropriate. If you were super-clever you'd go and do your poly
    /// and convex queries simultaneously.
    ///
    /// @param localMat is used to transform everything from objectspace to worldspace.
    bool buildCollisionInfo(const Box3F& box, Convex* c, AbstractPolyList* poly, AtlasInstance2* object);
};

#endif