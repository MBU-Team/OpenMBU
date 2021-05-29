//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _OPTIMIZEDPOLYLIST_H_
#define _OPTIMIZEDPOLYLIST_H_

#ifndef _ABSTRACTPOLYLIST_H_
#include "collision/abstractPolyList.h"
#endif

#define DEV 0.01

/// A concrete, renderable PolyList
///
/// This class is used to store geometry from a PolyList query.
///
/// It allows you to render this data, as well.
///
/// @see AbstractPolyList
class OptimizedPolyList : public AbstractPolyList
{
public:

    struct Poly
    {
        S32 plane;
        SceneObject* object;
        S32 material;
        U32 vertexStart;
        U32 vertexCount;
        U32 surfaceKey;

        U32 triangleLightingStartIndex;

        Poly() { plane = -1; vertexCount = 0; material = -1; };
    };

    struct FullPoly
    {
        PlaneF plane;
        SceneObject* object;
        S32 material;
        U32 surfaceKey;

        Vector<U32> indexes;
        Vector<Point3F> vertexes;
    };

    enum
    {
        NonShared = 0,
        Concave,
        Convex
    };

    struct Edge
    {
        U32 type;
        S32 vertexes[2];
        S32 faces[2];
    };

    struct TriangleLighting
    {
        U32 lightMapId;
        PlaneF lightMapEquationX;
        PlaneF lightMapEquationY;
    };

    Vector<TriangleLighting> mTriangleLightingList;

    const TriangleLighting* getTriangleLighting(const U32 index)
    {
        if (index >= mTriangleLightingList.size())
            return NULL;
        return &mTriangleLightingList[index];
    }

    typedef Vector<PlaneF>  PlaneList;
    typedef Vector<Point3F> VertexList;
    typedef Vector<Poly>    PolyList;
    typedef Vector<U32>     IndexList;
    typedef Vector<Edge>    EdgeList;

    PolyList    mPolyList;
    VertexList  mVertexList;
    IndexList   mIndexList;
    PlaneList   mPlaneList;
    EdgeList    mEdgeList;

    PlaneList   mPolyPlaneList;

public:
    OptimizedPolyList();
    ~OptimizedPolyList();
    void clear();

    // Virtual methods
    U32  addPoint(const Point3F& p);
    U32  addPlane(const PlaneF& plane);
    void begin(U32 material, U32 surfaceKey);
    void plane(U32 v1, U32 v2, U32 v3);
    void plane(const PlaneF& p);
    void plane(const U32 index);
    void vertex(U32 vi);
    void end();

    inline bool isEqual(Point3F& a, Point3F& b)
    {
        return((mFabs(a.x - b.x) < DEV) &&
            (mFabs(a.y - b.y) < DEV) &&
            (mFabs(a.z - b.z) < DEV));
    }

    void copyPolyToList(OptimizedPolyList* target, U32 pdx) const;
    void copyPolyToList(OptimizedPolyList* target, const FullPoly& poly) const;
    FullPoly getFullPoly(U32 pdx);

    void render();
    bool isEmpty() const;

protected:
    const PlaneF& getIndexedPlane(const U32 index);
};

#endif  // _OPTIMIZEDPOLYLIST_H_
