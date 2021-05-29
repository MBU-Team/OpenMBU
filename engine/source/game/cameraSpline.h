//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------


#ifndef _CAMERASPLINE_H_
#define _CAMERASPLINE_H_

#include "math/mMath.h"
#include "core/tVector.h"

//----------------------------------------------------------------------------
class CameraSpline
{
public:
    struct Knot
    {
    public:
        Point3F mPosition;
        QuatF   mRotation;
        F32     mSpeed;    /// in meters per second
        enum Type {
            NORMAL,
            POSITION_ONLY,
            KINK,
            NUM_TYPE_BITS = 2
        }mType;

        enum Path {
            LINEAR,
            SPLINE,
            NUM_PATH_BITS = 1
        }mPath;

        F32   mDistance;
        Knot* prev;
        Knot* next;

        Knot() {};
        Knot(const Knot& k);
        Knot(const Point3F& p, const QuatF& r, F32 s, Knot::Type type = NORMAL, Knot::Path path = SPLINE);
    };


    CameraSpline();
    ~CameraSpline();

    bool isEmpty() { return (mFront == NULL); }
    S32 size() { return mSize; }
    Knot* remove(Knot* w);
    void removeAll();

    Knot* front() { return mFront; }
    Knot* back() { return (mFront == NULL) ? NULL : mFront->prev; }

    void push_back(Knot* w);
    void push_front(Knot* w) { push_back(w); mFront = w; mIsMapDirty = true; }

    Knot* getKnot(S32 i);
    Knot* next(Knot* k) { return (k->next == mFront) ? k : k->next; }
    Knot* prev(Knot* k) { return (k == mFront) ? k : k->prev; }

    F32 advanceTime(F32 t, S32 delta_ms);
    F32 advanceDist(F32 t, F32 meters);
    void value(F32 t, Knot* result, bool skip_rotation = false);

    F32 getDistance(F32 t);
    F32 getTime(F32 d);

    void renderTimeMap();


private:
    Knot* mFront;
    S32 mSize;
    bool mIsMapDirty;

    struct TimeMap {
        F32 mTime;
        F32 mDistance;
    };

    Vector<TimeMap> mTimeMap;
    void buildTimeMap();
};




#endif