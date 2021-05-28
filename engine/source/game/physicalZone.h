//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _H_PHYSICALZONE
#define _H_PHYSICALZONE

#ifndef _SCENEOBJECT_H_
#include "sim/sceneObject.h"
#endif
#ifndef _EARLYOUTPOLYLIST_H_
#include "collision/earlyOutPolyList.h"
#endif


class Convex;

// -------------------------------------------------------------------------
class PhysicalZone : public SceneObject
{
    typedef SceneObject Parent;

protected:
    bool onAdd();
    void onRemove();

    enum UpdateMasks {
        InitialUpdateMask = 1 << 0,
        ActiveMask = 1 << 1
    };

public:
    void setTransform(const MatrixF& mat);

protected:
    F32        mVelocityMod;
    F32        mGravityMod;
    Point3F    mAppliedForce;

    // Basically ripped from trigger
    Polyhedron           mPolyhedron;
    EarlyOutPolyList     mClippedList;

    bool mActive;

    Convex* mConvexList;
    void buildConvex(const Box3F& box, Convex* convex);

public:
    PhysicalZone();
    ~PhysicalZone();

    F32 getVelocityMod() const { return mVelocityMod; }
    F32 getGravityMod()  const { return mGravityMod; }
    const Point3F& getForce() const { return mAppliedForce; }

    void setPolyhedron(const Polyhedron&);
    bool testObject(SceneObject*);

    void activate();
    void deactivate();
    bool isActive() const { return mActive; }

    DECLARE_CONOBJECT(PhysicalZone);
    static void initPersistFields();

    U32  packUpdate(NetConnection* conn, U32 mask, BitStream* stream);
    void unpackUpdate(NetConnection* conn, BitStream* stream);
};

#endif // _H_PHYSICALZONE

