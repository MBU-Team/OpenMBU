//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _CAMERA_H_
#define _CAMERA_H_

#ifndef _SHAPEBASE_H_
#include "game/shapeBase.h"
#endif

//----------------------------------------------------------------------------
struct CameraData : public ShapeBaseData {
    typedef ShapeBaseData Parent;

    //
    DECLARE_CONOBJECT(CameraData);
    static void initPersistFields();
    virtual void packData(BitStream* stream);
    virtual void unpackData(BitStream* stream);
};


//----------------------------------------------------------------------------
/// Implements a basic camera object.
class Camera : public ShapeBase
{
    typedef ShapeBase Parent;

    enum MaskBits {
        MoveMask = Parent::NextFreeMask,
        NextFreeMask = Parent::NextFreeMask << 1
    };

    struct StateDelta {
        Point3F pos;
        Point3F rot;
        VectorF posVec;
        VectorF rotVec;
    };
    Point3F mRot;
    StateDelta delta;

    static F32 mMovementSpeed;

    void setPosition(const Point3F& pos, const Point3F& viewRot);

    SimObjectPtr<GameBase> mOrbitObject;
    F32 mMinOrbitDist;
    F32 mMaxOrbitDist;
    F32 mCurOrbitDist;
    Point3F mPosition;
    bool mObservingClientObject;

    enum
    {
        StationaryMode = 0,

        FreeRotateMode = 1,
        FlyMode = 2,
        OrbitObjectMode = 3,
        OrbitPointMode = 4,

        CameraFirstMode = 0,
        CameraLastMode = 4
    };
    int mode;
    void setPosition(const Point3F& pos, const Point3F& viewRot, MatrixF* mat);
    void setTransform(const MatrixF& mat);
    F32 getCameraFov();
    F32 getDefaultCameraFov();
    bool isValidCameraFov(F32 fov);
    void setCameraFov(F32 fov);

    F32 getDamageFlash() const;
    F32 getWhiteOut() const;

public:
    DECLARE_CONOBJECT(Camera);

    Camera();
    ~Camera();
    static void initPersistFields();
    static void consoleInit();

    void onEditorEnable();
    void onEditorDisable();

    bool onAdd();
    void onRemove();
    void renderImage(SceneState* state);
    void processTick(const Move* move);
    void interpolateTick(F32 delta);
    void getCameraTransform(F32* pos, MatrixF* mat);

    void writePacketData(GameConnection* conn, BitStream* stream);
    void readPacketData(GameConnection* conn, BitStream* stream);
    U32  packUpdate(NetConnection* conn, U32 mask, BitStream* stream);
    void unpackUpdate(NetConnection* conn, BitStream* stream);
    Point3F& getPosition();
    void setFlyMode();
    void setOrbitMode(GameBase* obj, Point3F& pos, AngAxisF& rot,
        F32 minDist, F32 maxDist, F32 curDist, bool ownClientObject);
    void validateEyePoint(F32 pos, MatrixF* mat);
    void onDeleteNotify(SimObject* obj);

    GameBase* getOrbitObject() { return(mOrbitObject); }
    bool isObservingClientObject() { return(mObservingClientObject); }
};


#endif
