//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _TSSTATIC_H_
#define _TSSTATIC_H_

#ifndef _SCENEOBJECT_H_
#include "sim/sceneObject.h"
#endif
#ifndef _CONVEX_H_
#include "collision/convex.h"
#endif
#ifndef _RESMANAGER_H_
#include "core/resManager.h"
#endif

class TSShape;
class TSShapeInstance;
class TSStatic;
class Shadow;

//--------------------------------------------------------------------------
class TSStaticConvex : public Convex
{
    typedef Convex Parent;
    friend class TSStatic;

protected:
    TSStatic* pStatic;
    MatrixF* nodeTransform;

public:
    U32       hullId;
    Box3F     box;

public:
    TSStaticConvex() { mType = TSStaticConvexType; nodeTransform = 0; }
    TSStaticConvex(const TSStaticConvex& cv) {
        mType = TSStaticConvexType;
        mObject = cv.mObject;
        pStatic = cv.pStatic;
        nodeTransform = cv.nodeTransform;
        hullId = cv.hullId;
        box = box;
    }

    void findNodeTransform();
    const MatrixF& getTransform() const;
    Box3F getBoundingBox() const;
    Box3F getBoundingBox(const MatrixF& mat, const Point3F& scale) const;
    Point3F      support(const VectorF& v) const;
    void         getFeatures(const MatrixF& mat, const VectorF& n, ConvexFeature* cf);
    void         getPolyList(AbstractPolyList* list);
};

//--------------------------------------------------------------------------
class TSStatic : public SceneObject
{
    typedef SceneObject Parent;
    friend class TSStaticConvex;
#ifdef MB_ULTRA
    friend class Item;
#endif

    static U32 smUniqueIdentifier;

    enum Constants {
        MaxCollisionShapes = 8
    };

    enum MaskBits {
        advancedStaticOptionsMask = Parent::NextFreeMask,
        UpdateCollisionMask = Parent::NextFreeMask << 1,
        NextFreeMask = Parent::NextFreeMask << 1
    };

public: 
    enum CollisionType
    {
        None = 0,
        Bounds = 1,
        CollisionMesh = 2,
        VisibleMesh = 3
    };

protected:
    bool onAdd();
    void onRemove();

    // Collision
    void prepCollision();
    bool castRay(const Point3F& start, const Point3F& end, RayInfo* info);
    bool buildPolyList(AbstractPolyList* polyList, const Box3F& box, const SphereF& sphere);
    void buildConvex(const Box3F& box, Convex* convex);
protected:
    Convex* mConvexList;

    StringTableEntry  mShapeName;
    U32               mShapeHash;
    Resource<TSShape> mShape;
    TSShapeInstance* mShapeInstance;
    Shadow* mShadow;

    CollisionType     mCollisionType;

    // Rendering
protected:
    bool prepRenderImage(SceneState* state, const U32 stateKey, const U32 startZone, const bool modifyBaseZoneState = false);
    void renderObject(SceneState* state);
    //void renderShadow     ( F32 dist, F32 fogAmount);
    void setTransform(const MatrixF& mat);

public:
    Vector<S32>            mCollisionDetails;
    Vector<S32>            mLOSDetails;

    TSStatic();
    ~TSStatic();

    DECLARE_CONOBJECT(TSStatic);
    static void initPersistFields();

    U32  packUpdate(NetConnection* conn, U32 mask, BitStream* stream);
    void unpackUpdate(NetConnection* conn, BitStream* stream);


    void inspectPostApply();
    
    CollisionType getCollisionType() { return mCollisionType; }

    Resource<TSShape> getShape() const { return mShape; }
    StringTableEntry getShapeFileName() { return mShapeName; }

    TSShapeInstance* getShapeInstance() const { return mShapeInstance; }

    const Vector<S32>& getCollisionDetails() const { return mCollisionDetails; }

    const Vector<S32>& getLOSDetails() const { return mLOSDetails; }

    void setShapeName(const char* shapeName);
    void setSequence(const char* sequenceName);
};

#endif // _H_TSSTATIC

