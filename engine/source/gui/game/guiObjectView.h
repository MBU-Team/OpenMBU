//-----------------------------------------------------------------------------
// Torque Game Engine Advanced
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _GUIOBJECTVIEW_H_
#define _GUIOBJECTVIEW_H_

#include "gui/core/guiTSControl.h"
#include "ts/tsShapeInstance.h"

class LightInfo;

class GuiObjectView : public GuiTSCtrl
{
private:
    typedef GuiTSCtrl Parent;

protected:
    enum MouseState
    {
        None,
        Rotating,
        Zooming
    };

    MouseState mMouseState;

    TSShapeInstance *mModel;
    TSShapeInstance *mMountedModel;
    U32 mSkinTag;

    Point3F mCameraPos;
    MatrixF mCameraMatrix;
    EulerF mCameraRot;
    EulerF mCameraRotSpeed;
    Point3F mOrbitPos;
    S32 mMountNode;

    TSThread *runThread;
    S32 lastRenderTime;
    S32 mAnimationSeq;

    Point2I mLastMousePoint;

    LightInfo *mFakeSun;

    StringTableEntry mModelName;
    StringTableEntry mSkinName;
    bool mAutoSize;

public:
    void onSleep();
    bool onWake();

    void onMouseEnter(const GuiEvent &event);

    void onMouseLeave(const GuiEvent &event);

    void onMouseDown(const GuiEvent &event);

    void onMouseUp(const GuiEvent &event);

    void onMouseDragged(const GuiEvent &event);

    void onRightMouseDown(const GuiEvent &event);

    void onRightMouseUp(const GuiEvent &event);

    void onRightMouseDragged(const GuiEvent &event);

    void setObjectModel(const char *modelName, const char* skinName);
    void setEmpty();

    void setObjectAnimation(S32 index);

    void setMountedObject(const char *modelName, S32 mountPoint);

    void getMountedObjTransform(MatrixF *mat);

    /// Sets the distance at which the camera orbits the object. Clamped to the
    /// acceptable range defined in the class by min and max orbit distances.
    ///
    /// \param distance The distance to set the orbit to (will be clamped).
    void setOrbitDistance(F32 distance);

    bool processCameraQuery(CameraQuery *query);

    void renderWorld(const RectI &updateRect);

    DECLARE_CONOBJECT(GuiObjectView);

    GuiObjectView();

    ~GuiObjectView();

    static void initPersistFields();

private:
    F32 mMaxOrbitDist;
    F32 mMinOrbitDist;
    F32 mOrbitDist;

    static const S32 MAX_ANIMATIONS = 6;    ///< Maximum number of animations for the primary model displayed in this control
    static const S32 NO_NODE = -1;   ///< Indicates there is no node with a mounted object
};

#endif
