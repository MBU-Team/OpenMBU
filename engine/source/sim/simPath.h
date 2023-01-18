//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _SIMPATH_H_
#define _SIMPATH_H_

#ifndef _SCENEOBJECT_H_
#include "sim/sceneObject.h"
#endif

//--------------------------------------------------------------------------
/// A path!
class Path : public SimGroup
{
    friend class InteriorInstance;

    typedef SimGroup Parent;

public:
    enum : U32 {
        NoPathIndex = 0xFFFFFFFF
    };


private:
    U32 mPathIndex;
    bool mIsLooping;

protected:
    bool onAdd();
    void onRemove();

public:
    Path();
    ~Path();

    void addObject(SimObject*);
    void removeObject(SimObject*);

    void sortMarkers();
    void updatePath();
    bool isLooping() { return mIsLooping; }
    U32 getPathIndex() const;

    DECLARE_CONOBJECT(Path);
    static void initPersistFields();
};


//--------------------------------------------------------------------------
class Marker : public SceneObject
{
    typedef SceneObject Parent;
    friend class Path;

public:
    enum {
        SmoothingTypeLinear,
        SmoothingTypeSpline,
        SmoothingTypeAccelerate,
    };

    enum {
        KnotTypeNormal,
        KnotTypePositionOnly,
        KnotTypeKink,
    };


    U32   mSeqNum;
    U32   mSmoothingType;
    U32   mKnotType;

    U32   mMSToNext;

    // Rendering
protected:
    bool prepRenderImage(SceneState* state, const U32 stateKey, const U32 startZone, const bool modifyBaseZoneState);
    void renderObject(SceneState* state);

protected:
    bool onAdd();
    void onRemove();
    void onGroupAdd();

    void onEditorEnable();
    void onEditorDisable();


public:
    Marker();
    ~Marker();

    DECLARE_CONOBJECT(Marker);
    static void initPersistFields();
    void inspectPostApply();

    U32  packUpdate(NetConnection* conn, U32 mask, BitStream* stream);
    void unpackUpdate(NetConnection* conn, BitStream* stream);
};

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
inline U32 Path::getPathIndex() const
{
    return mPathIndex;
}


#endif // _H_PATH

