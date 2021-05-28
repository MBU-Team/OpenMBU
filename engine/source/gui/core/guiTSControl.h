//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _GUITSCONTROL_H_
#define _GUITSCONTROL_H_

#ifndef _GUICONTROL_H_
#include "gui/core/guiControl.h"
#endif
#ifndef _MMATH_H_
#include "math/mMath.h"
#endif

#include "lightingSystem/sgDynamicRangeLighting.h"


struct CameraQuery
{
    SimObject* object = NULL;
    F32         nearPlane = 1.0f;
    F32         farPlane = 100.0f;
    F32         fov = 1.5707964f;
#ifdef MB_ULTRA
    F32         fovy = 1.5707964f;
#endif
    MatrixF     cameraMatrix = MatrixF(true);

    //Point3F position;
    //Point3F viewVector;
    //Point3F upVector;
};

class GuiTSCtrl : public GuiControl
{
    typedef GuiControl Parent;

    MatrixF     mSaveModelview;
    MatrixF     mSaveProjection;
    RectI       mSaveViewport;


    static U32     smFrameCount;
    F32            mCameraZRot;
    F32            mForceFOV;

    sgDRLSystem drlSystem;

public:
    CameraQuery    mLastCameraQuery;
    GuiTSCtrl();

    void onPreRender();
    void onRender(Point2I offset, const RectI& updateRect);
    virtual bool processCameraQuery(CameraQuery* query);
    virtual void renderWorld(const RectI& updateRect);

    static void initPersistFields();
    static void consoleInit();

    virtual void onRemove();

    bool project(const Point3F& pt, Point3F* dest); // returns screen space X,Y and Z for world space point
    bool unproject(const Point3F& pt, Point3F* dest); // returns world space point for X, Y and Z

    DECLARE_CONOBJECT(GuiTSCtrl);
};

#endif
