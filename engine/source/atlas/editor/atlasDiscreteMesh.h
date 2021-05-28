//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------
#ifndef _ATLASDISCRETEMESH_H_
#define _ATLASDISCRETEMESH_H_

#include "core/tVector.h"
#include "math/mMath.h"

/// @defgroup AtlasEditor Atlas Editing
///
/// Functionality related to editing and generating Atlas terrains.
///
/// Much of this functionality is exposed as console commands and thus
/// does not show up here.
///
/// @ingroup Atlas

/// A discrete mesh, for use when modifying Atlas mesh data.
///
/// It deals with triangle lists.
///
/// @ingroup AtlasEditor
class AtlasDiscreteMesh
{
public:
    AtlasDiscreteMesh();
    ~AtlasDiscreteMesh();

    U32 mVertexCount;
    Point3F* mPos;
    Point3F* mNormal;
    Point2F* mTex;

    bool mHasMorphData;
    Point3F* mPosMorphOffset;
    Point2F* mTexMorphOffset;

    U32 mIndexCount;
    U16* mIndex;

    /// If we own data, then we'll delete it when we get destructed.
    bool mOwnsData;

    /// Make a new mesh that contains a simplified version of this one,
    /// decimated to target faces.
    ///
    /// Contains proper morph data.
    AtlasDiscreteMesh* decimate(U32 target);

    /// Combine a set of meshes into a new one. This just appends all their
    /// data together. A good followup step is to do a weld.
    void combine(Vector<AtlasDiscreteMesh*>& meshes);

    /// Merge verts within threshold units of one another. 
    void weld(F32 posThreshold, F32 texThreshold = 0.001f);

    ///  Apply the given remap table to our indices.
    void remap(const Vector<U16>& remapTable);

    /// Condition a mesh for better rendering.
    ///
    /// Stuff like:
    ///      - Optimize for vertex caching.
    ///      - Optimize so verts are present in order they were first drawn.
    ///      - and whatever else we can think of.
    void conditionMesh();

    /// Transform the texture coordinates by multiply and add on each axis.
    void transformTexCoords(Point2F s, Point2F t);

    /// Transform the positions by multiply and add on x and y axes.
    void transformPositions(Point2F s, Point2F t);

    /// Clip against a half-plane. Pass it pointers to to ADMs and it will
    /// fill them. If one is empty then it that side's data will be discarded.
    ///
    /// Properly generates interpolated texcoords. Ignores morph data.
    void clipAgainstPlane(PlaneF p, AtlasDiscreteMesh* front, AtlasDiscreteMesh* back);

    Box3F calcBounds();
};

#endif