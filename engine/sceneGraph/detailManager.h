//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _DETAILMANAGER_H_
#define _DETAILMANAGER_H_

#ifndef _PLATFORMASSERT_H_
#include "platform/platformAssert.h"
#endif
#ifndef _MMATH_H_
#include "math/mMath.h"
#endif
#ifndef _TVECTOR_H_
#include "core/tVector.h"
#endif

#define MAX_BUMP 4

class TSShapeInstance;
class TSPartInstance;

class DetailManager
{
public:

    struct DetailData
    {
        S32 tag;
        TSShapeInstance* shapeInstance; // either this
        TSPartInstance* partInstance;   // or this
        S16 bump[MAX_BUMP];
        S16 dls[MAX_BUMP];
        S32 dl;
        F32 intraDL;
        S32 prevDL;
        F32 pixelSize;
        F32 priority;
    };

    struct DetailProfile
    {
        S32 skipFirst; // for how many rounds do we want to skip "bumping" first detail
        S32 skipMiddle; // for how many rounds do we want to skip "bumping" every middle detail
        S32 skipLast;  // for how many rounds do we want to skip "bumping" last detail
    };

    static DetailProfile smDefaultProfile;

private:

    static DetailManager* smDetailManager;
    static DetailManager* get() { AssertFatal(smDetailManager, "DetailManager::get"); return smDetailManager; }

    S32 mTag;
    S32 mPolyLimit; // our poly budget...computed at the start of each frame based on detail scale
    bool mInPrepRender;

    S32 mPolyCount; // current poly count
    S32 mBumpPolyCount[MAX_BUMP]; // number of polys we save at each "bump" level

    Vector<DetailData*> mDetailData;
    Vector<DetailData*> mFreeDetailData;

    void begin();
    void end();

    void selectPotential(TSShapeInstance* si, F32 dist, F32 invScale, const DetailProfile* dp = &smDefaultProfile);
    bool selectCurrent(TSShapeInstance* si);

    void selectPotential(TSPartInstance* pi, F32 dist, F32 invScale, const DetailProfile* dp = &smDefaultProfile);
    bool selectCurrent(TSPartInstance* pi);

    void bumpOne(DetailData*, S32 bump);
    void bumpAll(S32 bump);

    void computePriority(DetailData*, S32 bump);
    DetailData* getNewDetailData();

public:

    DetailManager();
    ~DetailManager();

    static F32 smDetailScale; // user can adjust this

    static S32 smMaxPolyLimit; // app sets this once -- or uses default value
    static S32 smMinPolyLimit; // app sets this once -- or uses default value
    static S32 smLimitRange;   // app sets this once -- or uses default value

    static S32 smPolysTriedToRender;  // not used here, but can be read for misc. uses
    static S32 smPolysDidRender;      // not used here, but can be read for misc. uses

    static void init() { AssertFatal(!smDetailManager, "DetailManger::init"); smDetailManager = new DetailManager; }
    static void shutdown() { delete smDetailManager; smDetailManager = NULL; }

    static void beginPrepRender() { get()->begin(); }
    static void endPrepRender() { get()->end(); }

    static void selectPotentialDetails(TSShapeInstance* si, F32 dist, F32 invScale) { get()->selectPotential(si, dist, invScale); }
    static bool selectCurrentDetail(TSShapeInstance* si) { return get()->selectCurrent(si); }

    static void selectPotentialDetails(TSPartInstance* pi, F32 dist, F32 invScale) { get()->selectPotential(pi, dist, invScale); }
    static bool selectCurrentDetail(TSPartInstance* pi) { return get()->selectCurrent(pi); }
#ifdef TORQUE_DEBUG
    void clean();
    static void Clean() { if (smDetailManager) smDetailManager->clean(); }
#endif
};

#endif
