#ifndef _CONVEXBRUSH_H_
#define _CONVEXBRUSH_H_

#include "platform/platform.h"
#include "core/tVector.h"
#include "math/mPoint.h"
#include "math/mBox.h"
#include "math/mPlane.h"
#include "collision/abstractPolyList.h"
#include "collision/optimizedPolyList.h"
#include "sim/sceneObject.h"
#include "interior/interiorMapRes.h"

#define WINDINGSCALE 10000

class ConvexBrush
{
public:
    // Constructor
    ConvexBrush();
    // Destructor
    ~ConvexBrush();

    enum
    {
        Front = 0,
        Back = 1,
        On = 2,
        Split = 3,
        Unknown = 4
    };

    enum
    {
        Unprocessed = 0,
        Good,
        BadWinding,
        ShortPlanes,
        BadFaces,
        Malformed,
        Concave,
        Deleted
    };

    // Supporting structures
    struct TexInfo
    {
        StringTableEntry texture;
        PlaneF   texGens[2];
        F32      scale[2];
        F32      rot;
        F32      texDiv[2];
    };

    // The brushes owner entity
    InteriorMapResource::Entity* mOwner;

    // Bounding box
    Box3F             mBounds;
    Point3F           mCentroid;
    MatrixF           mTransform;
    Point3F           mScale;
    QuatF             mRotation;

    // lighting info...
    MatrixF mLightingTransform;

    // Some useful values
    F32 mBrushScale;
    S32 mID;
    U32 mType;
    U32 mStatus;
    U32 mPrevStatus;
    bool mSelected;
    StringTableEntry mDebugInfo;

    // Scratch buffer storage
    MatrixF mTransformScratch;
    QuatF   mRotationScratch;


    // Data
    OptimizedPolyList mFaces;
    Vector<TexInfo>   mTexInfos;

    // Setup functions
    bool addPlane(PlaneF pln);
    bool addFace(PlaneF pln, PlaneF texGen[2], F32 scale[2], char* texture);
    bool addFace(PlaneF pln, PlaneF texGen[2], F32 scale[2], char* texture, U32 matIdx);
    bool addFace(PlaneF pln, PlaneF texGen[2], F32 scale[2], U32 matIdx);
    bool processBrush();
    bool setScale(F32 scale);

    // Utility functions
    bool selfClip();
    bool intersectPlanes(const PlaneF& plane1, const PlaneF& plane2, const PlaneF& plane3, Point3D* pOutput);
    bool createWinding(const Vector<U32>& rPoints, const Point3F& normal, Vector<U32>& pWinding);
    void addEdge(U16 zero, U16 one, U32 face);
    bool generateEdgelist();
    void validateWindings();
    bool validateEdges();
    void addIndex(Vector<U32>& indices, U32 index);
    void calcCentroid();
    void setupTransform();
    void resetBrush();

    U32 getPolySide(S32 side);
    U32 whichSide(PlaneF pln);
    U32 whichSide(PlaneF pln, U32 faceIndex);
    bool isInsideBox(PlaneF left, PlaneF right, PlaneF top, PlaneF bottom);
    bool splitBrush(PlaneF pln, ConvexBrush* fbrush, ConvexBrush* bbrush);
    bool calcBounds();
    bool castRay(const Point3F& s, const Point3F& e, RayInfo* info);

    OptimizedPolyList getIntersectingPolys(OptimizedPolyList* list);
    OptimizedPolyList getNonIntersectingPolys(OptimizedPolyList* list);
    bool isPolyInside(OptimizedPolyList* list, U32 pdx);

    bool getPolyList(AbstractPolyList* list);

    // Debug render functions
    bool render(bool genColors);
    bool renderFace(U32 face, bool renderLighting);
    bool renderEdges(ColorF color);
};

#endif
